//Copyright 2015 <>< Charles Lohr, See LICENSE file.
//WS2812 sender that abuses the I2S interface on the WS2812.

#ifndef _WS2812I2S_TEST
#define _WS2812I2S_TEST

//Stuff that should be for the header:

#include <c_types.h>

#define WS_BLOCKSIZE 4000

void ICACHE_FLASH_ATTR ir_init();
void ICACHE_FLASH_ATTR ir_push( uint8_t * buffer, uint16_t buffersize ); //Buffersize = Nr LEDs * 3j

#endif

