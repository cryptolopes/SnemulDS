//these just extend libnds interrupt libraries
#ifndef nds_interrupt9_headers
#define nds_interrupt9_headers

#include <nds.h>
#include <nds/system.h>
#include <nds/interrupts.h>
#include <nds/touch.h>
#include <stdio.h>
#include <malloc.h>
#include <string.h>

#include "common_shared.h"
#include "interrupts.h"

#ifdef ARM7
#include <nds/arm7/i2c.h>
#endif


#endif
#ifdef __cplusplus
extern "C"{
#endif

//external (usermode)
extern void vcounter();
extern void Vblank();
extern void Hblank();
extern void initirqs();
extern void refreshNESjoypads();

extern volatile u32 interrupts_to_wait_arm9;

//internal code (kernel)
extern void IntrMainExt();

//libnds
extern void irqDummy(void);
extern struct IntTable irqTable[MAX_INTERRUPTS];

#ifdef INT_TABLE_SECTION
#else
extern struct IntTable irqTable[MAX_INTERRUPTS];
#endif

extern void irqInitExt(IntFn handler);

#ifdef __cplusplus
}
#endif
