/*
 * main_test.c — Port verification tests for the Turbo9 FreeRTOS port.
 *
 * After building freertos_test.img and running it in the Turbo9 simulator,
 * inspect the test_results[] array at its linked address (visible in the
 * .map file or by searching the image for the sentinel pattern 0x00,0x00).
 *
 * Result codes:
 *   0x00 = TEST_PENDING  (never reached the check)
 *   0x55 = TEST_PASS
 *   0xFF = TEST_FAIL
 *
 * test_results[0] = overall (PASS only if all sub-tests pass)
 * test_results[1] = parameter-passing test
 * test_results[2] = yield / context-switch stress test
 *
 * The supervisor task writes the results then executes BGND to halt the
 * simulator, leaving the result bytes in a stable, readable state.
 *
 * Tests and what they catch
 * -------------------------
 * 1. Parameter passing (test_results[1])
 *    Creates three tasks each with a distinct pvParameters value.  Each task
 *    reads back (uint16_t)pvParameters and stores it in param_received[].
 *    The bug: before the port fix both A and B were loaded with the LSB of
 *    pvParameters and DP received the MSB — so a task receiving 0xAB01 would
 *    see D=0x0101 instead of 0xAB01.  The test detects any such mismatch.
 *
 * 2. Yield / context-switch stress (test_results[2])
 *    Two tasks each call vTaskDelay() YIELD_COUNT times with 64-byte stacks.
 *    Before the CWAI fix, every yield to a task injected an extra 12-byte
 *    interrupt frame on that task's stack, quickly exhausting a 64-byte
 *    stack and causing a crash or hang before the counters reach YIELD_COUNT.
 */

#include <cmoc.h>
#include "FreeRTOS.h"
#include "task.h"
#include "cpu.h"

/* -----------------------------------------------------------------------
   Result codes
   ----------------------------------------------------------------------- */
#define TEST_PENDING  0x00
#define TEST_PASS     0x55
#define TEST_FAIL     0xFF

volatile uint8_t  test_results[4];   /* [0]=overall [1]=param [2]=yield [3]=unused */
volatile uint16_t param_received[3]; /* values received by the three param tasks  */

/* -----------------------------------------------------------------------
   Test 1 – 16-bit parameter passing
   Each task stores (uint16_t)pvParameters to a slot in param_received[].
   The supervisor later compares all three slots against the known values.
   ----------------------------------------------------------------------- */

void param_task_0(void *pvParameters)
{
    param_received[0] = (uint16_t)pvParameters;
    vTaskDelete(NULL);
}

void param_task_1(void *pvParameters)
{
    param_received[1] = (uint16_t)pvParameters;
    vTaskDelete(NULL);
}

void param_task_2(void *pvParameters)
{
    param_received[2] = (uint16_t)pvParameters;
    vTaskDelete(NULL);
}

/* -----------------------------------------------------------------------
   Test 2 – yield / context-switch stress
   Each task calls vTaskDelay() YIELD_COUNT times using a 64-byte stack.
   With the CWAI frame-leak every yield injects 12 extra bytes; five yields
   exhaust a 64-byte stack (5 * 12 = 60 bytes just for frames) so the
   tasks would never reach YIELD_COUNT if the bug were present.
   ----------------------------------------------------------------------- */

#define YIELD_COUNT 30

volatile uint8_t yield_hits_a;
volatile uint8_t yield_hits_b;

void yield_task_a(void *pvParameters)
{
    uint8_t i;
    for (i = 0; i < YIELD_COUNT; i++) {
        yield_hits_a++;
        vTaskDelay(1);
    }
    vTaskDelete(NULL);
}

void yield_task_b(void *pvParameters)
{
    uint8_t i;
    for (i = 0; i < YIELD_COUNT; i++) {
        yield_hits_b++;
        vTaskDelay(2);
    }
    vTaskDelete(NULL);
}

/* -----------------------------------------------------------------------
   Supervisor – waits for all tasks to finish, writes results, halts.
   ----------------------------------------------------------------------- */

void supervisor_task(void *pvParameters)
{
    /* Wait long enough for both yield tasks to complete:
       task_b needs YIELD_COUNT * 2 ticks; add a margin for scheduling jitter. */
    vTaskDelay(YIELD_COUNT * 3 + 20);

    /* --- Test 1: parameter passing --- */
    if (param_received[0] == 0xAB01 &&
        param_received[1] == 0x1234 &&
        param_received[2] == 0xFF00) {
        test_results[1] = TEST_PASS;
    } else {
        test_results[1] = TEST_FAIL;
    }

    /* --- Test 2: yield stress --- */
    if (yield_hits_a == YIELD_COUNT && yield_hits_b == YIELD_COUNT) {
        test_results[2] = TEST_PASS;
    } else {
        test_results[2] = TEST_FAIL;
    }

    /* --- Overall --- */
    test_results[0] = (test_results[1] == TEST_PASS &&
                       test_results[2] == TEST_PASS)
                      ? TEST_PASS : TEST_FAIL;

    /* Halt the Turbo9 simulator so the result bytes are stable.
       BGND (begin-debug) causes the simulator to enter its monitor/halt state.
       The infinite loop below is a fallback if BGND does not halt execution. */
    asm { bgnd }
    for (;;);
}

/* -----------------------------------------------------------------------
   Stack-overflow hook — called by FreeRTOS if configCHECK_FOR_STACK_OVERFLOW
   is enabled.  Marks overall result FAIL so the failure is detectable even
   if the overflowing task never reaches its counter check.
   ----------------------------------------------------------------------- */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask;
    (void)pcTaskName;
    test_results[0] = TEST_FAIL;
    asm { bgnd }
    for (;;);
}

/* -----------------------------------------------------------------------
   Entry point
   ----------------------------------------------------------------------- */

int ATTR_BANK0 main(void)
{
    test_results[0] = TEST_PENDING;
    test_results[1] = TEST_PENDING;
    test_results[2] = TEST_PENDING;
    test_results[3] = TEST_PENDING;
    param_received[0] = 0;
    param_received[1] = 0;
    param_received[2] = 0;
    yield_hits_a = 0;
    yield_hits_b = 0;

    /* Parameter-passing tasks: high priority so they run before supervisor wakes. */
    xTaskCreate(param_task_0, "PT0", 64, (void *)0xAB01, 4, NULL);
    xTaskCreate(param_task_1, "PT1", 64, (void *)0x1234, 4, NULL);
    xTaskCreate(param_task_2, "PT2", 64, (void *)0xFF00, 4, NULL);

    /* Yield-stress tasks: mid priority, 64-byte stacks deliberately tight. */
    xTaskCreate(yield_task_a, "YTA", 64, NULL, 3, NULL);
    xTaskCreate(yield_task_b, "YTB", 64, NULL, 3, NULL);

    /* Supervisor: lowest priority, larger stack for FreeRTOS bookkeeping. */
    xTaskCreate(supervisor_task, "SUP", 128, NULL, 1, NULL);

    vTaskStartScheduler();
    for (;;);
    return 0;
}
