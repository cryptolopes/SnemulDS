#ifndef MAIN_SNEMULDS
#define MAIN_SNEMULDS
typedef struct s_Options
{
	uint8 BG3Squish :2;
	uint8 SoundOutput :1;
	uint8 LayersConf :6;
	uint8 TileMode :1;
	uint8 BG_Layer :8;
	uint8 YScroll :2;
	uint8 WaitVBlank :1;
	uint8 SpeedHack :3;
} t_Options;

typedef int (*intfuncptr)();
typedef u32 (*u32funcptr)();
typedef void (*voidfuncptr)();

#endif



#ifdef __cplusplus
extern "C" {
#endif

extern uint32 screen_mode;
extern int APU_MAX;

void	PPU_updateGFX(int line);
void	APU_clear();
void 	PPU_ChangeLayerConf(int i);
int 	saveSRAM();
void	GUI_clear();
void 	mem_clear_paging();
u32	    crc32 (unsigned int crc, const void *buffer, unsigned int size);
int		changeROM(char *ROM, int size);
void 	GUI_setLanguage(int lang);
void	GUI_align_printf(int flags, char *fmt, ...);
int 	initSNESEmpty();
void 	GUI_getConfig();
char 	*GUI_getROM(char *rompath);
void 	GUI_deleteROMSelector();
void 	GUI_createMainMenu();
void	APU_pause();
int 	go();

extern char logbuf[];
extern uint32 AsmDebug[16];
extern void exception_handler();

#ifdef LIBNDS
extern unsigned int __ewram_start;
extern unsigned int __ewram_end;
#endif

//snapshot.c
extern void	FS_lock();
extern void	FS_unlock();
extern void	APU_stop();
extern void	APU_loadSpc();
extern void	GUI_console_printf(int x, int y, char *fmt, ...);
extern void	APU_saveSpc();

extern u32 keys;

#ifdef __cplusplus
}
#endif