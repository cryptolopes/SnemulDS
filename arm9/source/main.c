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
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <nds/ndstypes.h>
#include <nds/memory.h>
#include <fcntl.h>

#include "gui.h"

#include "fs.h"
#include "snes.h"
#include "gfx.h"
#include "cfg.h"
#include "apu.h"

#include "ram.h"

#include "conf.h"
#include "snemul_str.h"
#include "frontend.h"
#include "main.h"

#include "dldi.h"


#include "ppu.h"

#include <string.h>
#include <nds/dma.h>
#include <nds/ndstypes.h>
#include <nds/arm9/sassert.h>
#include <nds/arm9/video.h>
#include <nds/arm9/cache.h>
#include <nds/arm9/trig_lut.h>
#include <nds/arm9/math.h>
#include <nds/arm9/dynamicArray.h>
#include "interrupts.h"
#include "common_shared.h"

#include <nds.h>
#include "sys/socket.h"
#include "netinet/in.h"
#include <netdb.h>
#include <ctype.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
//#include <dirent.h>
#include "ff.h"
#include <nds/memory.h>
#include <nds/ndstypes.h>
#include <nds/memory.h>
#include <nds/bios.h>
#include <nds/system.h>
#include <nds/arm9/math.h>
#include <nds/arm9/video.h>
#include <nds/arm9/videoGL.h>
#include <nds/arm9/trig_lut.h>
#include <nds/arm9/sassert.h>

#include "touch_ipc.h"


IN_DTCM
int _offsetY_tab[4] = { 16, 0, 32, 24 };

IN_DTCM
uint32 screen_mode;

IN_DTCM
int APU_MAX = 262;

IN_DTCM
u32 keys = 0;


void applyOptions()
{
	if (!CFG.Sound_output)
		APU_clear();

	if (CFG.LayersConf < 10)
		PPU_ChangeLayerConf(CFG.LayersConf);

	GFX.YScroll = _offsetY_tab[CFG.YScroll];
}

// FIXME: Move me...
IN_DTCM
uint8 LayersConf[10][4] =
{
{ 0, 1, 2, 3 },
{ 1, 2, 0, 3 },
{ 3, 3, 2, 3 },
{ 3, 3, 3, 3 },
{ 2, 2, 2, 2 },
{ 1, 1, 1, 1 },
{ 0, 0, 0, 0 },
{ 2, 3, 0, 1 },
{ 2, 0, 3, 1 },
{ 2, 1, 0, 3 }, 
};

void PPU_ChangeLayerConf(int i)
{
	CFG.LayersConf = i % 10;
	CFG.LayerPr[0] = LayersConf[CFG.LayersConf][0];
	CFG.LayerPr[1] = LayersConf[CFG.LayersConf][1];
	CFG.LayerPr[2] = LayersConf[CFG.LayersConf][2];
	CFG.LayerPr[3] = LayersConf[CFG.LayersConf][3];
}

void readOptionsFromConfig(char *section)
{
	CFG.BG3Squish = get_config_int(section, "BG3Squish", CFG.BG3Squish) & 3;
	// FIXME 
	GFX.YScroll = get_config_int(section, "YScroll", GFX.YScroll);
	if (GFX.YScroll == 16)
		CFG.YScroll = 0;
	if (GFX.YScroll == 0)
		CFG.YScroll = 1;
	if (GFX.YScroll == 32)
		CFG.YScroll = 2;
	if (GFX.YScroll == 24)
		CFG.YScroll = 3;	
	
	CFG.Scaled = get_config_int(section, "Scaled", CFG.Scaled);
	CFG.Sound_output = get_config_int(section, "Sound", CFG.Sound_output) & 1;
	CFG.BG_Layer = (get_config_int(section, "HDMA", 1)&1) << 7;

	int BG_Layer = get_config_oct(section, "BGLayers", 010111);
	if ((BG_Layer & 7) == 1)
		CFG.BG_Layer |= 1;
	if (((BG_Layer>>3) & 7) == 1)
		CFG.BG_Layer |= 2;
	if (((BG_Layer>>6) & 7) == 1)
		CFG.BG_Layer |= 4;
	if (((BG_Layer>>9) & 7) == 1)
		CFG.BG_Layer |= 8;
	if (((BG_Layer>>12) & 7) == 1)
		CFG.BG_Layer |= 0x10;

	CFG.LayersConf = get_config_int(section, "BGPriorities", CFG.LayersConf);
	if (CFG.LayersConf == 10)
	{
		int BGManualPriority = get_config_oct(section, "BGManualPriority",
				00123);
		CFG.LayerPr[0] = (BGManualPriority) & 3;
		CFG.LayerPr[1] = (BGManualPriority>>3) & 3;
		CFG.LayerPr[2] = (BGManualPriority>>6) & 3;
		CFG.LayerPr[3] = (BGManualPriority>>9) & 3;

		CFG.LayerPr[0] = get_config_int(section, "BG1Pr", CFG.LayerPr[0]) & 3;
		CFG.LayerPr[1] = get_config_int(section, "BG2Pr", CFG.LayerPr[1]) & 3;
		CFG.LayerPr[2] = get_config_int(section, "BG3Pr", CFG.LayerPr[2]) & 3;
		CFG.LayerPr[3] = get_config_int(section, "BG4Pr", CFG.LayerPr[3]) & 3;

	}
	else
		PPU_ChangeLayerConf(CFG.LayersConf);

	CFG.Transparency
			= get_config_int(section, "Transparency", CFG.Transparency);
	CFG.WaitVBlank = get_config_int(section, "Vblank", CFG.WaitVBlank);
	CFG.CPU_speedhack
			= get_config_int(section, "SpeedHacks", CFG.CPU_speedhack);
	CFG.FastDMA = get_config_int(section, "FastDMA", CFG.FastDMA);

	CFG.MouseXAddr = get_config_hex(section, "MouseXAddr", 0);
	CFG.MouseYAddr = get_config_hex(section, "MouseYAddr", 0);
	CFG.MouseMode = get_config_int(section, "MouseMode", 0);
	CFG.MouseXOffset = get_config_int(section, "MouseXOffset", 0);
	CFG.MouseYOffset = get_config_int(section, "MouseYOffset", 0);

	CFG.SoundPortSync = 0;

	int SoundPortSync = get_config_oct(section, "SoundPortSync",
			CFG.SoundPortSync);
	if ((SoundPortSync & 7) == 1)
		CFG.SoundPortSync |= 8;
	if (((SoundPortSync>>3) & 7) == 1)
		CFG.SoundPortSync |= 4;
	if (((SoundPortSync>>6) & 7) == 1)
		CFG.SoundPortSync |= 2;
	if (((SoundPortSync>>9) & 7) == 1)
		CFG.SoundPortSync |= 1;
	if (((SoundPortSync>>12) & 7) == 1)
		CFG.SoundPortSync |= 0x80;
	if (((SoundPortSync>>15) & 7) == 1)
		CFG.SoundPortSync |= 0x40;
	if (((SoundPortSync>>18) & 7) == 1)
		CFG.SoundPortSync |= 0x20;
	if (((SoundPortSync>>21) & 7) == 1)
		CFG.SoundPortSync |= 0x10;

	CFG.TilePriorityBG = get_config_int(section, "TilePriorityBG",
			CFG.TilePriorityBG);
	CFG.BG3TilePriority = get_config_int(section, "BG3TilePriority",
			CFG.BG3TilePriority);
	CFG.Debug2 = get_config_int(section, "BlankTileNumber", CFG.Debug2);
	int SpritePriority = get_config_oct(section, "SpritePriority", 01123);
	CFG.SpritePr[0] = (SpritePriority) & 3;
	CFG.SpritePr[1] = (SpritePriority>>3) & 3;
	CFG.SpritePr[2] = (SpritePriority>>6) & 3;
	CFG.SpritePr[3] = (SpritePriority>>9) & 3;
	
	CFG.MapExtMem = get_config_int(section, "MapExtMem", CFG.MapExtMem);
	
	CFG.AutoSRAM = get_config_int(section, "AutoSRAM", CFG.AutoSRAM);
}

void saveOptionsToConfig(char *section)
{
	set_config_int(section, "BG3Squish", CFG.BG3Squish);
	// FIXME 
	set_config_int(section, "YScroll", GFX.YScroll);
	set_config_int(section, "Sound", CFG.Sound_output);
	
	set_config_int(section, "Scaled", CFG.Scaled);
	//	set_config_int(section, "GFXEngine", CFG.TileMode);
	//	set_config_int(section, "HDMA", CFG.BG_Layer>>7);

	set_config_oct(section, "BGLayers", 5, (CFG.BG_Layer & 1)|((CFG.BG_Layer
			& 2)<<2)|((CFG.BG_Layer & 4)<<4)|((CFG.BG_Layer & 8)<<6)
			|((CFG.BG_Layer & 0x10)<<8));

	set_config_int(section, "BGPriorities", CFG.LayersConf);

	//	set_config_int(section, "Transparency", CFG.Transparency);
	set_config_int(section, "Vblank", CFG.WaitVBlank);
	set_config_int(section, "SpeedHacks", CFG.CPU_speedhack);
	//	set_config_int(section, "FastDMA", CFG.FastDMA);

	/*	set_config_hex(section, "MouseXAddr", 0);
	 set_config_hex(section, "MouseYAddr", 0);
	 set_config_int(section, "MouseMode", 0);
	 set_config_int(section, "MouseXOffset", 0);
	 set_config_int(section, "MouseYOffset", 0);*/

	//	set_config_oct(section, "SoundPortSync", CFG.SoundPortSync);

	/*	set_config_int(section, "TilePriorityBG", CFG.TilePriorityBG);
	 set_config_int(section, "BG3TilePriority", CFG.BG3TilePriority);
	 set_config_int(section, "BlankTileNumber", CFG.Debug2);
	 set_config_oct(section, "SpritePriority", 01123);*/
	
	set_config_int(section, "AutoSRAM", CFG.AutoSRAM);
	save_config_file();
}

// FIXME : fix layersconf

void packOptions(uint8 *ptr)
{
	t_Options *opt = (t_Options *)ptr;

	opt->BG3Squish = CFG.BG3Squish;
	opt->SoundOutput = CFG.Sound_output;
	if (CFG.LayersConf == 0)
		opt->LayersConf = 0x24; // 0/1/2
	else
		opt->LayersConf = CFG.LayerPr[0] | (CFG.LayerPr[1] << 2)
				| (CFG.LayerPr[2] << 4);
//	opt->TileMode = CFG.TileMode;
	opt->BG_Layer = CFG.BG_Layer;
	opt->YScroll = CFG.YScroll;
	opt->WaitVBlank = CFG.WaitVBlank;
	opt->SpeedHack = CFG.CPU_speedhack;
}

void unpackOptions(int version, uint8 *ptr)
{
	t_Options *opt = (t_Options *)ptr;

	if (version == 1)
		CFG.BG3Squish = 2-opt->BG3Squish;
	else
		CFG.BG3Squish = opt->BG3Squish;
	CFG.Sound_output = opt->SoundOutput;
	if (version == 1)
		CFG.LayersConf = opt->LayersConf;
	else
	{
		if (opt->LayersConf == 0x24) // 0/1/2 == automatic layer
		{
			CFG.LayersConf = 0;
		}
		else
		{
			CFG.LayersConf = 10;
			CFG.LayerPr[0] = opt->LayersConf&3;
			CFG.LayerPr[1] = (opt->LayersConf>>2)&3;
			CFG.LayerPr[2] = (opt->LayersConf>>4)&3;
			CFG.LayerPr[3] = 3;
		}
	}
/*	if (version == 1)
		CFG.TileMode = 0; // Force line by line mode
	else
		CFG.TileMode = opt->TileMode;*/
	CFG.BG_Layer = opt->BG_Layer;
	CFG.YScroll = opt->YScroll;
	CFG.WaitVBlank = opt->WaitVBlank;
	CFG.CPU_speedhack = opt->SpeedHack;

	applyOptions();
}

int checkConfiguration(char *name, int crc)
{
	// Check configuration file
	readOptionsFromConfig("Global");

	char *section= NULL;
	if (is_section_exists(SNES.ROM_info.title))
	{
		section = SNES.ROM_info.title;
	}
	else if (is_section_exists(FS_getFileName(name)))
	{
		section = FS_getFileName(name);
	}
	else if ((section = find_config_section_with_hex("crc", crc)))
	{
	}
	else if ((section = find_config_section_with_string("title2", SNES.ROM_info.title)))
	{
	}
	else if ((section = find_config_section_with_hex("crc2", crc)))
	{
	}
	else if ((section = find_config_section_with_string("title3", SNES.ROM_info.title)))
	{
	}
	else if ((section = find_config_section_with_hex("crc3", crc)))
	{
	}
	else if ((section = find_config_section_with_string("title4", SNES.ROM_info.title)))
	{
	}
	else if ((section = find_config_section_with_hex("crc4", crc)))
	{
	}

	if (section != NULL)
	{
		GUI_printf("Section : %s\n", section);
		readOptionsFromConfig(section);
	}
	
	return 0;
}

int loadROM(char *name, int confirm)
{
	int size;
	char romname[100];
	int ROMheader;
	char *ROM;
	int crc;

	// Save SRAM of previous game first
	saveSRAM();

	GUI_clear();
	/*	if (ROM && FS_shouldFreeROM())
	 free(ROM);*/
	CFG.LargeROM = 0;
	strcpy(romname, CFG.ROMPath);
	if (CFG.ROMPath[strlen(CFG.ROMPath)-1] != '/')
		strcat(romname, "/");
	strcat(romname, name);
	strcpy(CFG.ROMFile, romname);

	GUI_printf("Loading %s...\n", romname);

	mem_clear_paging(); // FIXME: move me...

	ROM = (char *)&rom_buffer[0];
	size = FS_getFileSize(romname);

	
	ROMheader = size & 8191;
	if (ROMheader != 0&& ROMheader != 512)
		ROMheader = 512;

#ifndef USE_GBFS	
	if (size-ROMheader > ROM_MAX_SIZE)
	{
	/*
		// Large ROM, memory pagging enabled
		if (size <= CFG.ExtRAMSize)
		{
			GUI_printf("Use External RAM\n");
			if (CFG.MapExtMem) // Use External RAM for mapping high addresses of Large ROM
				FS_loadROMInExtRAM(ROM-ROMheader, romname, ROM_MAX_SIZE+ROMheader, size);
			else // Use External RAM to load pages
				FS_loadROMInExtRAM(ROM-ROMheader, romname, ROM_STATIC_SIZE+ROMheader, size);
		}
		else
			FS_loadROMForPaging(ROM-ROMheader, romname, ROM_STATIC_SIZE+ROMheader);
	*/
		FS_loadROMForPaging(ROM-ROMheader, romname, ROM_STATIC_SIZE+ROMheader);
		CFG.LargeROM = 1;
		crc = crc32(0, ROM, ROM_STATIC_SIZE);
		GUI_printf("Large ROM detected. CRC(1Mb) = %08x\n", crc);
	}
	else
#endif	
	{
		FS_loadROM(ROM-ROMheader, romname);
		CFG.LargeROM = 0;
		crc = crc32(0, ROM, size-ROMheader);
		GUI_printf("CRC = %08x\n", crc);
	}

	changeROM(ROM-ROMheader, size);

	checkConfiguration(name, crc);
	
    if(SNES.HiROM == 0){
        printf("SNESROM is LoROM.Press A");
    }
    else if(SNES.HiROM == 1)
        printf("SNESROM is SNES2.HiROM.Press A");
    else{
        printf("An error as ocurred SNES2.HiROM: %d.Press A",SNES.HiROM);
    }
    while(1==1){
        scanKeys();
        
        if (keysDown()&KEY_A){
            break;
        }
    }
    
	return 0;
}


int selectSong(char *name)
{
	char spcname[100];

	strcpy(spcname, CFG.ROMPath);
	if (CFG.ROMPath[strlen(CFG.ROMPath)-1] != '/')
		strcat(spcname, "/");
	strcat(spcname, "/");
	strcat(spcname, name);
	strcpy(CFG.Playlist, spcname);
	CFG.Jukebox = 1;
	CFG.Sound_output = 0;
	APU_stop();
	if (FS_loadFile(spcname, (char *)APU_RAM_ADDRESS, 0x10200) < 0)
		return -1;
	APU_playSpc();
	// Wait APU init
	
	return 0;
}

/*
#ifdef ARM9
void exception_handler()
{
	
	u32 currentMode = getCPSR() & 0x1f;
	u32 thumbState = ((*(u32*)0x02CFFD90) & 0x20);
	u32 savedPC = *(u32*)0x02CFFD98;
	u32 exceptionAddress=0;

	exceptionRegisters[15] = savedPC;

	if (currentMode == 0x17)
	{
		// Data abort- actual faulting instruction was 8 bytes earlier 
		exceptionAddress = savedPC - 8;
	}
	else
	{
		
		// XXX: Assuming invalid instruction error?
		// Place the fault at the previous instruction.
		//
		exceptionAddress = savedPC - (thumbState ? 2 : 4);
	}

	iprintf(
		"r0=%08x r1=%08x r2=%08x r3=%08x\n",
		(unsigned int)AsmDebug[0],(unsigned int)AsmDebug[1],(unsigned int)AsmDebug[2],(unsigned int)AsmDebug[3] );

	iprintf("\nException %02x @ %08x (%s)\n",
		(unsigned int)currentMode, (unsigned int)exceptionAddress,
		(const char*)thumbState ? "Thumb" : "ARM");

    for (i = 0; i < 8; i++) {
		LOG(" %-03s %08x ", registerNames[i], exceptionRegisters[i]);
		LOG(" %-03s %08x \n", registerNames[i+8], exceptionRegisters[i+8]);
	}

}
#endif
*/

int argc;
char **argv;
int main(int _argc, char **_argv) {
	
	argc=_argc, argv=_argv;
	defaultExceptionHandler();
	MyIPC->APU_ADDR_CNT = 0;
	MyIPC->APU_ADDR_CMD = 0;
	
	resetMemory2_ARM9();
	
	powerOn(POWER_ALL_2D | POWER_SWAP_LCDS);
	
	//fifo setups
    irqInitExt(IntrMainExt);
	
	irqSet(IRQ_VBLANK, Vblank);
	irqSet(IRQ_HBLANK,Hblank);
	irqSet(IRQ_VCOUNT,vcounter);
	irqSet(IRQ_FIFO_NOT_EMPTY,HandleFifoNotEmpty);
    //irqSet(IRQ_FIFO_EMPTY,HandleFifoEmpty);
	
	interrupts_to_wait_arm9 = IRQ_HBLANK|IRQ_VBLANK|IRQ_VCOUNT| IRQ_FIFO_NOT_EMPTY; //| IRQ_FIFO_EMPTY;
	irqEnable(interrupts_to_wait_arm9);
	
	REG_IPC_SYNC = 0;
    REG_IPC_FIFO_CR = IPC_FIFO_RECV_IRQ | IPC_FIFO_SEND_IRQ | IPC_FIFO_ENABLE;
    
    //set up ppu: do irq on hblank/vblank/vcount/and vcount line is 159
    REG_DISPSTAT = REG_DISPSTAT | DISP_HBLANK_IRQ | DISP_VBLANK_IRQ | DISP_YTRIGGER_IRQ | (VCOUNT_LINE_INTERRUPT << 15);
	
#ifndef TIMER_Y	
	TIMER3_CR &= ~TIMER_ENABLE; // not strictly necessary if the timer hasn't been enabled before
	TIMER3_DATA = 0;
	TIMER3_CR = TIMER_ENABLE | TIMER_DIV_1;
#endif

	screen_mode = 0;
	videoSetMode(0); //not using the main screen
	videoSetModeSub(MODE_0_2D | DISPLAY_BG0_ACTIVE); //sub bg 0 will be used to print text
	REG_BG0CNT = REG_BG1CNT = REG_BG2CNT = REG_BG3CNT = 0;


	// 256Ko for Tiles (SNES: 32-64Ko) 
	vramSetBankA(VRAM_A_MAIN_BG_0x06020000);
	vramSetBankB(VRAM_B_MAIN_BG_0x06040000);

	// 128Ko (+48kb) for sub screen / GUI 
	vramSetBankC(VRAM_C_SUB_BG_0x06200000);

	// Some memory for ARM7 (128 Ko!)
	vramSetBankD(VRAM_D_ARM7_0x06000000);

	// 80Ko for Sprites (SNES : 32-64Ko) 
	vramSetBankE(VRAM_E_MAIN_SPRITE); // 0x6400000

	vramSetBankF(VRAM_F_MAIN_SPRITE);

	vramSetBankG(VRAM_G_BG_EXT_PALETTE);

	// 48ko For CPU 

	vramSetBankH(VRAM_H_LCD);
	vramSetBankI(VRAM_I_LCD);

	/* 32 first kilobytes for MAP */
	/* remaning memory for tiles */
	
	GUI_init();		
	
	GUI_setLanguage(PersonalData->language);
	
	GUI.printfy = 32;
	GUI_align_printf(GUI_TEXT_ALIGN_CENTER, SNEMULDS_TITLE);
	GUI_align_printf(GUI_TEXT_ALIGN_CENTER, SNEMULDS_SUBTITLE);
    GUI.printfy += 32; // FIXME
	GUI_align_printf(GUI_TEXT_ALIGN_CENTER, _STR(4));
	
	initSNESEmpty();
	update_ram_snes();
    
	
	int i=0;
	// Clear "HDMA"
	for (i = 0; i < 192; i++){
		GFX.lineInfo[i].mode = -1;
	}
	
    
	//later
	//nifi: 
	//switch_dswnifi_mode((u8)dswifi_nifimode);
	//wifi: 
	//switch_dswnifi_mode((u8)dswifi_wifimode);
	
	int ret=FS_init();
	if (ret == 0)
	{
		GUI_printf(_STR(IDS_FS_SUCCESS));
		//FRESULT res = FS_chdir("0:/");
	}
	else if(ret == -1)
	{
		GUI_printf(_STR(IDS_FS_FAILED));
	}
	
	CFG.ExtRAMSize = 0;
	
    /*
	{	char *p = malloc(10);
		iprintf("RAM = %p last malloc = %p", SNESC.RAM, p);
	}
	//TOUCH SCREEN TEST
	while (1)
	{
		int i;

		scanKeys();
		keys = keysHeld();

		//GUI_printf2(0, 2, "keys = %x %d\n", keys, SNES.h_blank);
		if (MyIPC->touched >0)
		{
			GUI_printf2(0, 3, "x = %d y = %d       ", MyIPC->touchX, MyIPC->touchY);  //touchXY.px replace with MyIPC
			//		waitReleaseTouch();	
		}

		if ((keys & KEY_START))
		break;
	}
    */

	for (i = 0; i < 100; i++)	GUI_clear();
	
	// Load SNEMUL.CFG
	set_config_file("snemul.cfg");
	
	GUI_printf("test!!\n");
	
	char *ROMfile;
	if(readFrontend(&ROMfile,&CFG.ROMPath)){
		readOptionsFromConfig("Global");
		GUI_getConfig();
	}else{
		CFG.ROMPath = get_config_string(NULL, "ROMPath", GAMES_DIR);
		readOptionsFromConfig("Global");
		GUI_getConfig();	
		ROMfile = GUI_getROM(CFG.ROMPath);
	}
	
	GUI_printf("2\n");

	loadROM(ROMfile, 0);
	
	GUI_printf("3\n");

	GUI_deleteROMSelector(); // Should also free ROMFile

	GUI_createMainMenu();
    
    //ok so far
    
	//wifi test
	/*
	if(Wifi_InitDefault(true) == true)
	{
		iprintf("WIFI OK");
	}
	else{
		iprintf("WIFI FAIL");
	}
	*/
	while (1)
	{
        if (REG_POWERCNT & POWER_SWAP_LCDS){
			GUI_update();
		}
        
        if (keys & KEY_LID)
		{
			saveSRAM();
			APU_pause();
			//			APU_stop();
			// hinge is closed 
			// power off everything not needed 
			powerOff(POWER_ALL) ;
			// set system into sleep 
			while (keys & KEY_LID)
			{
				scanKeys();
				keys = keysHeld();
			}
			// wait a bit until returning power 
			// power on again 
			powerOn(POWER_ALL_2D) ;
			// set up old irqs again 
			APU_pause();
		}
		
		
        if (!SNES.Stopped){
            go();   //boots here
        }
        
		//swiWaitForVBlank();
		swiIntrWait(1,IRQ_VBLANK | IRQ_HBLANK | IRQ_VCOUNT | IRQ_FIFO_NOT_EMPTY);
	}

}