/*
 * FreeRTOS Kernel V10.5.1
 * Copyright (C) 2021 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * SPDX-License-Identifier: MIT
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * https://www.FreeRTOS.org
 * https://github.com/FreeRTOS
 *
 */

/* CMOC/Turbo9 port, derived from GCC/HCS12 port by Jefferson L Smith, 2005 */

/* Scheduler includes. */
#include <cmoc.h>
#include "FreeRTOS.h"
#include "task.h"

/*-----------------------------------------------------------
 * Implementation of functions defined in portable.h for the Turbo9 port.
 *----------------------------------------------------------*/


/*
 * Configure a timer to generate the RTOS tick at the frequency specified
 * within FreeRTOSConfig.h.
 */
static void prvSetupTimerInterrupt( void );

/* NOTE: Interrupt service routines must be in non-banked memory - as does the
scheduler startup function. */
#define ATTR_NEAR

/* Manual context switch function.  This is the SWI ISR. */
// __attribute__((interrupt))
void ATTR_NEAR vPortYield( void );

/* Tick context switch function.  This is the timer ISR. */
// __attribute__((interrupt))
void ATTR_NEAR vPortTickInterrupt( void );

/* Function in non-banked memory which actually switches to first task. */
BaseType_t ATTR_NEAR xStartSchedulerNear( void );

/* Calls to portENTER_CRITICAL() can be nested.  When they are nested the
critical section should not be left (i.e. interrupts should not be re-enabled)
until the nesting depth reaches 0.  This variable simply tracks the nesting
depth.  Each task maintains it's own critical nesting depth variable so
uxCriticalNesting is saved and restored from the task stack during a context
switch. */
volatile UBaseType_t uxCriticalNesting = 0x80;  // un-initialized

/*-----------------------------------------------------------*/

/*
 * See header file for description.
 */
StackType_t *pxPortInitialiseStack( StackType_t *pxTopOfStack, TaskFunction_t pxCode, void *pvParameters )
{
	/* Build the initial 6809 interrupt frame that portRESTORE_CONTEXT + RTI
	   will consume when the task first runs.  Frame layout from [S] upward:

	     [S+0]  CC        [S+1]  A        [S+2]  B        [S+3]  DP
	     [S+4..5]  X      [S+6..7]  Y     [S+8..9]  U     [S+10..11]  PC

	   One extra byte below [S] holds the critical-nesting count, which
	   portRESTORE_CONTEXT pops (via PULS A) before issuing RTI.

	   Bytes are pushed from highest address down; the last push becomes [S]
	   after portRESTORE_CONTEXT pops the nesting count. */

	/* PC: big-endian — MSB at lower address ([S+10]), LSB at higher ([S+11]). */
	*pxTopOfStack   = ( StackType_t ) *( ((StackType_t *) (&pxCode) ) + 1 ); /* PC_LSB */
	*--pxTopOfStack = ( StackType_t ) *( ((StackType_t *) (&pxCode) ) + 0 ); /* PC_MSB */

	/* U, Y, X: debug sentinels only — values do not affect task execution. */
	*--pxTopOfStack = ( StackType_t ) 0xff;
	*--pxTopOfStack = ( StackType_t ) 0xee;

	*--pxTopOfStack = ( StackType_t ) 0xff;
	*--pxTopOfStack = ( StackType_t ) 0xee;

	*--pxTopOfStack = ( StackType_t ) 0xdd;
	*--pxTopOfStack = ( StackType_t ) 0xcc;

	/* D (A:B) = pvParameters.  CMOC passes the first 16-bit argument in D
	   where A holds the MSB and B holds the LSB.  RTI restores A from [S+1]
	   and B from [S+2], so they must be pushed in the order: DP, B, A
	   (DP lands at [S+3], B at [S+2], A at [S+1]). */
	*--pxTopOfStack = ( StackType_t ) 0x00;                                         /* DP */
	*--pxTopOfStack = ( StackType_t ) *( ((StackType_t *) (&pvParameters) ) + 1 );  /* B = LSB */
	*--pxTopOfStack = ( StackType_t ) *( ((StackType_t *) (&pvParameters) ) + 0 );  /* A = MSB */

	/* CCR: S bit set (stop disabled), I bit clear (IRQ enabled on task start). */
	*--pxTopOfStack = ( StackType_t ) 0x80;

	/* Critical nesting depth 0: not in a critical section. */
	*--pxTopOfStack = ( StackType_t ) 0x00;

	return pxTopOfStack;
}
/*-----------------------------------------------------------*/

void vPortEndScheduler( void )
{
	/* It is unlikely that the port will get stopped. */
}
/*-----------------------------------------------------------*/

static void prvSetupTimerInterrupt( void )
{
	char *timerStatus = 0xFF02;
	char *timerControl = 0xFF03;

	/* set the IRQ and SWI vectors */
	int *orgIRQAddress = 0xFFF8;
	int *orgSWIAddress = 0xFFFA;

 portDISABLE_INTERRUPTS()
	*orgIRQAddress = vPortTickInterrupt;
	*orgSWIAddress = vPortYield;

	// enable interrupt on timer fire
	*timerControl |= 0x01;	

portENABLE_INTERRUPTS()
}

/*-----------------------------------------------------------*/
BaseType_t xPortStartScheduler( void )
{
	/* Configure the timer that will generate the RTOS tick.  Interrupts are
	disabled when this function is called. */
	prvSetupTimerInterrupt();

	/* Restore the context of the first task. */
	portRESTORE_CONTEXT();

    asm {
        rti
    }

	/* Should not get here! */
	return pdFALSE;
}
/*-----------------------------------------------------------*/

/*
 * Context switch functions.  These are interrupt service routines.
 */

/*
 * Manual context switch forced by calling portYIELD().  This is the SWI
 * handler.
 */
void vPortYield( void )
{
	portISR_HEAD();
	/* NOTE: This is the trap routine (swi) although not defined as a trap.
	   It will fill the stack the same way as an ISR in order to mix preemption
	   and cooperative yield. */

	portSAVE_CONTEXT();
	vTaskSwitchContext();
	portRESTORE_CONTEXT();

	portISR_TAIL();
}
/*-----------------------------------------------------------*/

/*
 * RTOS tick interrupt service routine.  If the cooperative scheduler is
 * being used then this simply increments the tick count.  If the
 * preemptive scheduler is being used a context switch can occur.
 */
void vPortTickInterrupt( void )
{
	portISR_HEAD();

        asm {
         lda $ff02  get timer status register
		 ora #$01   set bit
		 sta $ff02  and write it out to clear interrupt
    }
	#if configUSE_PREEMPTION == 1
	{
		/* A context switch might happen so save the context. */
		portSAVE_CONTEXT();

		/* Increment the tick ... */
		if( xTaskIncrementTick() != pdFALSE )
		{
			/* A context switch is necessary. */
			vTaskSwitchContext();
		}

		/* Restore the context of a task - which may be a different task
		to that interrupted. */
		portRESTORE_CONTEXT();
	}
	#else
	{
		xTaskIncrementTick();
	}
	#endif


	portISR_TAIL();

}

