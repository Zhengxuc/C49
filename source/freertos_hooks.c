/* freertos_hooks.c - FreeRTOS 钩子函数实现 */
#include "FreeRTOS.h"

#if configUSE_IDLE_HOOK == 1
void vApplicationIdleHook( void )
{
    /* 空闲钩子 */
}
#endif

#if configUSE_TICK_HOOK == 1
void vApplicationTickHook( void )
{
    /* 时钟滴答钩子 */
}
#endif

#if configUSE_MALLOC_FAILED_HOOK == 1
void vApplicationMallocFailedHook( void )
{
    /* 内存分配失败钩子 */
    for( ;; );
}
#endif

#if configCHECK_FOR_STACK_OVERFLOW != 0
void vApplicationStackOverflowHook( TaskHandle_t xTask, char *pcTaskName )
{
   /* 栈溢出钩子 */
   ( void ) xTask;
   ( void ) pcTaskName;
   for( ;; );
}
#endif
