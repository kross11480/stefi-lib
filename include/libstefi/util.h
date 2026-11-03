#pragma once

#include <stdint.h>
#include <string.h>
#include <stdio.h>

#define BIT(num) (1 << (num))
#define GET_BITS(value, pos, num_bits) ( (value >> pos) & ((1U << num_bits) - 1) )
#define countof(array) sizeof(array)/sizeof(array[0])
void soft_delay_ms(uint32_t time_in_ms);
void soft_delay_us(uint32_t time_in_us);


#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1: __FILE__ )
#define TEXTIFY(x) #x

#ifndef assert
#define assert(expr) \
    do { \
        if(!(expr)) { \
        printf("Failed :%s, %d: %s\r\n", __FILENAME__, __LINE__, TEXTIFY(expr)); \
        while(1); \
        } \
       } \
    while(0);
#endif