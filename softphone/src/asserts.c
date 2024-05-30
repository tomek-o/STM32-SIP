#include "asserts.h"
#include <stdio.h>

/**
  * @brief  This function is executed in case of error occurrence.
  */
void Error_Handler(void)
{
    printf("\n\nError_Handler!\n\n");
    __asm volatile ("bkpt 0");
    while (1)
    {
    }
}

void __assert_func(const char *_file, int _line, const char *_func, const char *_expr )
{
    printf("ASSERT: file %s, line %d, func %s: %s\n", _file , _line, _func?_func:"NULL", _expr);
    volatile int loop = 1;
    __asm volatile ("bkpt 0");
    do
    {
        ;
    }
    while( loop );
}
