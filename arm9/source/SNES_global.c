/***********************************************************/
/* This source is part of SNEmulDS                         */
/* ------------------------------------------------------- */
/* (c) 1997-1999, 2006-2007 archeide, All rights reserved. */
/***********************************************************/
/*
This program is free software; you can redistribute it and/or 
modify it under the terms of the GNU General Public License as 
published by the Free Software Foundation; either version 2 of 
the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, 
but WITHOUT ANY WARRANTY; without even the implied warranty of 
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the 
GNU General Public License for more details.
*/

#include <nds.h>
#include "snes.h"
#include "opcodes.h"
#include "apu.h"
#include "gfx.h"
#include "cfg.h"
#include "memmap.h"

struct s_snes	SNES;
struct s_cpu	CPU;
struct s_snescore	SNESC;
struct s_gfx	GFX;
struct s_cfg	CFG;
volatile u8 snes_ram_bsram[0x20000+0x6000];    //128K SNES RAM + 8K (Big) SNES SRAM
volatile u8 snes_vram[0x010000];

IN_DTCM 
u8 * rom_page = (u8*)&rom_buffer[ROM_STATIC_SIZE*1];        //second slot of rombuffer

volatile u8 rom_buffer[ROM_MAX_SIZE];

void resetMemory2_ARM9() 
{
 	register int i;
    //clear out ARM9 DMA channels
	for (i=0; i<4; i++) {
		DMA_CR(i) = 0;
		DMA_SRC(i) = 0;
		DMA_DEST(i) = 0;
		TIMER_CR(i) = 0;
		TIMER_DATA(i) = 0;
	}
 
	VRAM_CR = 0x80808080;
	VRAM_E_CR = 0x80;
	VRAM_F_CR = 0x80;
	VRAM_G_CR = 0x80;
	VRAM_H_CR = 0x80;
	VRAM_I_CR = 0x80;
	
	// clear vram
	u16 * vram = (u16 *)0x6800000;
	zeroMemory(vram, 656 * 1024 );
	// clear video palette
	zeroMemory( BG_PALETTE, 2048 );//BG_PALETTE[0] = RGB15(1,1,1);
	zeroMemory( BG_PALETTE_SUB, 2048 );
	// clear video object attribution memory
	zeroMemory( OAM, 2048 );
	zeroMemory( OAM_SUB, 2048 );
	// clear video object data memory
	zeroMemory( SPRITE_GFX, 128 * 1024 );
	zeroMemory( SPRITE_GFX_SUB, 128 * 1024 );
	// clear main display registers
	zeroMemory( (void*)0x04000000, 0x6c );
	// clear sub display registers
	zeroMemory( (void*)0x04001000, 0x6c );
	
	// clear maths registers
	zeroMemory( (void*)0x04000280, 0x40 );
 
	REG_DISPSTAT = 0;
	videoSetMode(0);
	videoSetModeSub(0);
	VRAM_A_CR = 0;
	VRAM_B_CR = 0;
	VRAM_C_CR = 0;
	VRAM_D_CR = 0;
	VRAM_E_CR = 0;
	VRAM_F_CR = 0;
	VRAM_G_CR = 0;
	VRAM_H_CR = 0;
	VRAM_I_CR = 0;
	VRAM_CR   = 0x03000000;
	REG_POWERCNT  = 0x820F;
    
	//set shared ram to ARM7
	WRAM_CR = 0x03; 
}
