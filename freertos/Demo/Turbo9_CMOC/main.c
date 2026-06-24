
/*
 * FreeRTOS V202212.01
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
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

#define LAUNCH_TERMINAL_TASK

/*
 * main() creates all the application tasks, then starts the scheduler.
 */

/* Kernel includes. */
#include <cmoc.h>
#include "FreeRTOS.h"
#include "task.h"
#include "cpu.h"

#define HYPER9_TERM_OUT ( ( volatile unsigned char * ) 0xFF00 )

static void hyper9_putchar( unsigned char c )
{
    *HYPER9_TERM_OUT = c;
}

static void hyper9_puts( const char *text )
{
    while( *text != '\0' )
    {
        hyper9_putchar( ( unsigned char ) *text );
        text++;
    }
}

#ifdef LAUNCH_TERMINAL_TASK
void terminal_task(void *parameters) {
    ( void ) parameters;

    hyper9_puts( "FreeRTOS Turbo9 demo starting\r\n" );

    while (1) {
        hyper9_puts( "FreeRTOS task heartbeat\r\n" );
        vTaskDelay(60);
    }
}
#endif

/* This is called from startup. */
int ATTR_BANK0 main ( void )
{
    BaseType_t ret;
    
    #ifdef LAUNCH_TERMINAL_TASK
        TaskHandle_t t1;
    
        ret = xTaskCreate(   terminal_task,
                                        "TERM",
                                        64,
                                        NULL,
                                        1,
                                        ( TaskHandle_t * ) &t1 );

        if( ret != pdPASS )
        {
            hyper9_puts( "xTaskCreate failed\r\n" );
            for( ;; );
        }
    #endif

	vTaskStartScheduler();
          
	/* Should not reach here! */
	for( ;; );
	return 0;
}
/*-----------------------------------------------------------*/
