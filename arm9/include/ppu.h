#ifndef ppu_snemulds
#define ppu_snemulds

#ifdef ARM9

//Map base for each VRAM allocated region
#define BG_MAP_RAM_0x06020000(base) (u16*)((u16*)(((base)*0x800) + 0x06020000))
#define BG_MAP_RAM_0x06040000(base) (u16*)((u16*)(((base)*0x800) + 0x06040000))
#define BG_MAP_RAM_0x06060000(base) (u16*)((u16*)(((base)*0x800) + 0x06060000))

#define BG_TILE_RAM_0x06020000(base)   (u16*)((u16*)(((base)*0x4000) + 0x06020000))    
#define BG_TILE_RAM_0x06040000(base)   (u16*)((u16*)(((base)*0x4000) + 0x06040000))    
#define BG_TILE_RAM_0x06060000(base)   (u16*)((u16*)(((base)*0x4000) + 0x06060000))

#endif

/* should be 64 bytes long */
typedef struct s_OAM_entry
{
	uint8 Y;
	uint8 rot_data:2;
	uint8 mode:2;
	uint8 mosaic:1;
	uint8 color_depth:1;
	uint8 shape:2;
	
	uint16 X:9;
	uint8 rot_data2:3;
	uint8 flip:2;
	uint8 size:2;
	
	uint16 tile_index:10;
	uint8 pr:2;
	uint8 palette:4;
	
	uint16 rot_data3;
} t_OAM_entry;


/* Testing stuff... */

typedef struct
{
	int 	base;	// SNES base address
	int		depth;  // Bpp depth: 2 4 8
	uint16	*DSVRAMAddress;
	int		used;	
} t_TileZone;

#define CONVERT_SPR_TILE(tn) (((tn)&0xF)|(((tn)>>4)<<5))
//#define CONVERT_SPR_TILE(tn) (tn)
#define SNES_VRAM_OFFSET ((SNES_Port[0x01]&0x03) << 14)
#define DRAW_TILE(I, J, TILENB, BG, P, F) PPU_setMap(I, J, (TILENB)&1023, BG, P, F) 
#define ADD_TILE(TILE_BASE, TILENB, BG_MODE) \
  switch (BG_MODE) { \
    case 2 : if (!(GFX.tiles2b_def[TILE_BASE/16+(TILENB)] & 2)) \
  			    add_tile_2(TILE_BASE, TILENB); break; \
    case 4 : if (!GFX.tiles4b_def[TILE_BASE/32+(TILENB)]) \
    			add_tile_4(TILE_BASE, TILENB); break; \
    case 8 : if (!GFX.tiles8b_def[TILE_BASE/64+(TILENB)]) \
    			add_tile_8(TILE_BASE, TILENB); break; \
  }

#define SPRITE_ADD_X(INDEX) -(((GFX.spr_info_ext[INDEX>>2]&(1<<((INDEX&0x3)<<1))) != 0)<<8)
#define SPRITE_POS_Y(INDEX) (GFX.spr_info[INDEX].pos_y > 239 ? (char)GFX.spr_info[INDEX].pos_y : GFX.spr_info[INDEX].pos_y)



#endif

#include <nds.h>

#ifdef __cplusplus
extern "C" {
#endif

int	map_duplicate(int snes_block);

extern void DRAW_PLANE(unsigned char BG,unsigned char  BG_MODE);
extern void draw_plane_32_30(unsigned char bg, unsigned char bg_mode);
extern void draw_plane_64_30(unsigned char bg, unsigned char bg_mode);
extern void draw_plane_32_60(unsigned char bg, unsigned char bg_mode);
extern void draw_plane_64_60(unsigned char bg, unsigned char bg_mode);

extern void renderMode1(int NB_BG, int MODE_1, int MODE_2, int MODE_3, int MODE_4);
extern void renderMode3(int MODE_1, int MODE_2);

#ifdef __cplusplus
}
#endif