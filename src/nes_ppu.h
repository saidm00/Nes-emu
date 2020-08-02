#pragma once

/*
    nes_ppu.h: Header-only implementation of the PPU found inside of the NES

    Technical references:
    http://www.thealmightyguru.com/Games/Hacking/Wiki/index.php/NES_Palette
    http://nesdev.com/2C02%20technical%20reference.TXT
    https://wiki.nesdev.com/w/index.php/PPU_programmer_reference
*/

/* 
    Standard color pallete of the NES, encoded as 32-bit hex values 0x00RRGGBB

    From NES Hacker wiki (http://www.thealmightyguru.com/Games/Hacking/Wiki/index.php/NES_Palette): 
    "The Nintendo wasn't sophisticated enough to use the new color systems of today. No alpha 
    channels or 24-bit color to work with, no 8-bit palette, not even RGB settings. On the NES you 
    can work with a total of 64 pre-set colors (56 of which are unique), and you can only show 25 
    on the screen at one time (under normal circumstances)."

*/
const uint32_t NES_palette[64] = {
    0x007C7C7C,
    0x000000FC,
    0x000000BC,
    0x004428BC,
    0x00940084,
    0x00A80020,
    0x00A81000,
    0x00881400,
    0x00503000,
    0x00007800,
    0x00006800,
    0x00005800,
    0x00004058,
    0x00000000,
    0x00000000,
    0x00000000,
    
    0x00BCBCBC,
    0x000078F8,
    0x000058F8,
    0x006844FC,
    0x00D800CC,
    0x00E40058,
    0x00F83800,
    0x00E45C10,
    0x00AC7C00,
    0x0000B800,
    0x0000A800,
    0x0000A844,
    0x00008888,
    0x00000000,
    0x00000000,
    0x00000000,

    0x00F8F8F8,
    0x003CBCFC,
    0x006888FC,
    0x009878F8,
    0x00F878F8,
    0x00F85898,
    0x00F87858,
    0x00FCA044,
    0x00F8B800,
    0x00B8F818,
    0x0058D854,
    0x0058F898,
    0x0000E8D8,
    0x00787878,
    0x00000000,
    0x00000000,
    
    0x00FCFCFC,
    0x00A4E4FC,
    0x00B8B8F8,
    0x00D8B8F8,
    0x00F8B8F8,
    0x00F8A4C0,
    0x00F0D0B0,
    0x00FCE0A8,
    0x00F8D878,
    0x00D8F878,
    0x00B8F8B8,
    0x00B8F8D8,
    0x0000FCFC,
    0x00F8D8F8,
    0x00000000,
    0x00000000
};

/*
Enum of PPU registers (as an index)

Common Name	| Address |	Bits      | Notes
------------+---------+-----------+------------
PPUCTRL	    | $2000	  | VPHB SINN | NMI enable (V), PPU master/slave (P), sprite height (H), background tile select (B), sprite tile select (S), increment mode (I), nametable select (NN)
PPUMASK	    | $2001	  | BGRs bMmG | color emphasis (BGR), sprite enable (s), background enable (b), sprite left column enable (M), background left column enable (m), greyscale (G)
PPUSTATUS	| $2002	  | VSO- ---- | vblank (V), sprite 0 hit (S), sprite overflow (O); read resets write pair for $2005/$2006
OAMADDR	    | $2003	  | aaaa aaaa | OAM read/write address
OAMDATA	    | $2004	  | dddd dddd | OAM data read/write
PPUSCROLL	| $2005	  | xxxx xxxx | fine scroll position (two writes: X scroll, Y scroll)
PPUADDR	    | $2006	  | aaaa aaaa | PPU read/write address (two writes: most significant byte, least significant byte)
PPUDATA	    | $2007	  | dddd dddd | PPU data read/write
OAMDMA	    | $4014	  | aaaa aaaa | OAM DMA high address
*/
typedef enum PPU_REGS
{
    PPUCTRL     = 0x0,
    PPUMASK     = 0x1,
    PPUSTATUS   = 0x2,
    OAMADDR     = 0x3,
    OAMDATA     = 0x4,
    PPUSCROLL   = 0x5,
    PPUADDR     = 0x6,
    PPUDATA     = 0x7,
    OAMDMA      = 0x8
}
PPU_REGS;

/* PPU implementation */
typedef struct _nes_ppu
{
    uint8_t * PPU_Nametable[4];             /* Pointers to the 4 nametables */
    uint8_t * PPU_Attribtable[4];           /* Pointers to the 4 attribute tables ($40 in size) */    
    uint8_t * PPU_Pallete_Data[2];          /* Pointer to PPU pallete data (0 -> BG, 1-> FG) */
    
    union 
    {
        uint8_t     * PPU_OAM_bytes[2];     /* OAM access via row data (4*8 = 32 bits) or byte stream */
        uint32_t    * PPU_OAM_row[2];
    }

    uint32_t    screen_buffer[340 * 260];   /* All of the on-screen buffer, only visible portion is drawn in SDL */
    uint16_t    v, h;                       /* Indices for the screen */

    uint8_t PPU_registers[9];               /* Registers of the PPU */
    bool    clear_vblank,                   /* Flag to set/clear vblank bit on next tick */
            set_scroll_addr_latch,          /* Flags to set address latch for PPUSCROLL and PPUADDR */
            set_PPU_addr_latch;

    uint16_t scroll_addr;                   /* Address for PPUSCROLL */
}
_nes_ppu;
_nes_ppu nes_ppu;

/* 
NES PPU bus (from https://wiki.nesdev.com/w/index.php/PPU_memory_map)

Address range   Size    Device

$0000-$0FFF 	$1000 	Pattern table 0
$1000-$1FFF 	$1000 	Pattern table 1
$2000-$23FF 	$0400 	Nametable 0
$2400-$27FF 	$0400 	Nametable 1
$2800-$2BFF 	$0400 	Nametable 2
$2C00-$2FFF 	$0400 	Nametable 3
$3000-$3EFF 	$0F00 	Mirrors of $2000-$2EFF
$3F00-$3F1F 	$0020 	Palette RAM indexes ($3F00-$3F0F is background pallete, 
                                             $3F10-$3F1F is foreground pallete)
$3F20-$3FFF 	$00E0 	Mirrors of $3F00-$3F1F
*/
typedef struct _nes_ppu_bus
{
    uint8_t mem[0x4000];
    /* 
    To save on pins, the lower 8 pins of AB were multiplexed with the DB, not so with this emulator.
    The lower 8 bits of the Address bus are stored somewhere before the data bus is written to, so
    the multiplexing ion this case is unnecessary
    */
    uint16_t    AB;         /* Address, data bus */ 
    uint8_t     DB;
    bool        RW;         /* Flags to indicate read or write */
}
_nes_ppu_bus;
_nes_ppu_bus nes_ppu_bus;

/* Read from PPU memory */
static inline uint8_t PPU_PEEK(uint16_t addr)
{
    /* Name table mirrors */
    if (addr >= 0x2000 && addr < 0x3F00)
        return nes_ppu_bus.mem[(addr & 0x0EFF) + 0x2000];
    /* Pallete mirrors */
    if (addr >= 0x3F00 && addr < 0x4000)
        return nes_ppu_bus.mem[(addr & 0x1F) + 0x3F00];
    return nes_ppu_bus.mem[addr];    
}

/* Write to PPU memory */
static inline void PPU_POKE(uint16_t addr, uint8_t data)
{
    /* Name table mirrors */
    if (addr >= 0x2000 && addr < 0x3F00)
        nes_ppu_bus.mem[(addr & 0x0EFF) + 0x2000] = data;
    /* Pallete mirrors */
    else if (addr >= 0x3F00 && addr < 0x4000)
        nes_ppu_bus.mem[(addr & 0x1F) + 0x3F00] = data;
    else
        nes_ppu_bus.mem[addr] = data;
}

/* All PPU reg operations */
static inline void EXEC_PPUCTRL     (void);
static inline void EXEC_PPUMASK     (void);
static inline void EXEC_PPUSTATUS   (void);
static inline void EXEC_OAMADDR     (void);
static inline void EXEC_OAMDATA     (void);
static inline void EXEC_PPUSCROLL   (void);
static inline void EXEC_PPUADDR     (void);
static inline void EXEC_PPUDATA     (void);
static inline void EXEC_OAMDMA      (void);

/* Function pointer to execute PPU reg operations */
void (*REG_EXEC[9])(void) = {
    EXEC_PPUCTRL,
    EXEC_PPUMASK,
    EXEC_PPUSTATUS,
    EXEC_OAMADDR,
    EXEC_OAMDATA,
    EXEC_PPUSCROLL,
    EXEC_PPUADDR,
    EXEC_PPUDATA,
    EXEC_OAMDMA
};

/* 
    Function to successfully use PPU registers within range $2000 to $2007 

    RW = 0 -> Read mode
    RW = 1 -> Write mode

    Function list is encoded as a string, meaning if the index of the string
    matches an 'x', the function pointer will execute the function 
    corresponding to the index, and if '_' then do nothing

    "__x_x__x" is all functions that read and "xx_xxxxx" is all functions 
    that write
*/
static inline void USE_REGS(PPU_REGS reg, bool RW, uint8_t data)
{
    nes_ppu_bus.DB == data;
    const char * function_list = (RW == 0) ? "__x_x__x" : "xx_xxxxx";

    if(function_list[reg] == 'x') 
        (*REG_EXEC[reg])();
}

/* Init the PPU */
static inline void ppu_init()
{
    /* Set pointers to each nametable */
    nes_ppu.PPU_Nametable[0] = &nes_ppu_bus.mem[0x2000];
    nes_ppu.PPU_Nametable[1] = &nes_ppu_bus.mem[0x2400];
    nes_ppu.PPU_Nametable[2] = &nes_ppu_bus.mem[0x2800];
    nes_ppu.PPU_Nametable[3] = &nes_ppu_bus.mem[0x2C00];

    /* Set pointers to each attribute tables */
    nes_ppu.PPU_Attribtable[0] = &nes_ppu_bus.mem[0x23C0];
    nes_ppu.PPU_Attribtable[1] = &nes_ppu_bus.mem[0x27C0];    
    nes_ppu.PPU_Attribtable[2] = &nes_ppu_bus.mem[0x2BC0];
    nes_ppu.PPU_Attribtable[3] = &nes_ppu_bus.mem[0x2FC0];

    /* Set pointers to pattern tables/OAM */
    nes_ppu.PPU_OAM_bytes[0] = &nes_ppu_bus.mem[0x0000];
    nes_ppu.PPU_OAM_bytes[1] = &nes_ppu_bus.mem[0x1000];

    /* Set pointers to BG/FG pallete indexes */
    nes_ppu.PPU_Pallete_Data[0] = &nes_ppu_bus.mem[0x3F00];
    nes_ppu.PPU_Pallete_Data[1] = &nes_ppu_bus.mem[0x3F10];

    /* Finally, set indices accordingly */
    nes_ppu.v = 0;
    nes_ppu.h = 0;
}

/*
PPUCTRL: PPU Control Register (write)

7  bit  0
---- ----
VPHB SINN
|||| ||||
|||| ||++- Base nametable address
|||| ||    (0 = $2000; 1 = $2400; 2 = $2800; 3 = $2C00)
|||| |+--- VRAM address increment per CPU read/write of PPUDATA
|||| |     (0: add 1, going across; 1: add 32, going down)
|||| +---- Sprite pattern table address for 8x8 sprites
||||       (0: $0000; 1: $1000; ignored in 8x16 mode)
|||+------ Background pattern table address (0: $0000; 1: $1000)
||+------- Sprite size (0: 8x8 pixels; 1: 8x16 pixels)
|+-------- PPU master/slave select
|          (0: read backdrop from EXT pins; 1: output color on EXT pins)
+--------- Generate an NMI at the start of the
           vertical blanking interval (0: off; 1: on)

*/
static inline void EXEC_PPUCTRL(uint8_t data)
{
    nes_ppu.PPU_registers[PPUCTRL] = data;
}

/*
PPUMASK: PPU mask register (write)

7  bit  0
---- ----
BGRs bMmG
|||| ||||
|||| |||+- Greyscale (0: normal color, 1: produce a greyscale display)
|||| ||+-- 1: Show background in leftmost 8 pixels of screen, 0: Hide
|||| |+--- 1: Show sprites in leftmost 8 pixels of screen, 0: Hide
|||| +---- 1: Show background
|||+------ 1: Show sprites
||+------- Emphasize red
|+-------- Emphasize green
+--------- Emphasize blue
*/
static inline void EXEC_PPUMASK()
{
    nes_ppu.PPU_registers[PPUMASK] = data;
}

/* */ 
static inline void EXEC_PPUSTATUS()
{
    nes_ppu.clear_vblank        = 1;   /* Clear vblank bit on next tick */
    nes_ppu.scroll_addr         = 0x0; /* Clear address latches and flags */
    nes_ppu_bus.AB              = 0x0;
    nes_ppu.set_addr_latch      = false;
    nes_ppu.set_PPU_addr_latch  = false;
}

static inline void EXEC_OAMADDR()
{

}

static inline void EXEC_OAMDATA()
{

}

static inline void EXEC_PPUSCROLL()
{
    if (nes_ppu.set_scroll_addr_latch == false)
    {
        nes_ppu.scroll_addr             = (uint16_t) nes_ppu_bus.DB << 8;
        nes_ppu.set_scroll_addr_latch   = true;
    }
    else
    {
        nes_ppu.scroll_addr |= nes_ppu_bus.DB;
    }
    nes_ppu.PPU_registers[PPUSCROLL] = nes_ppu_bus.DB;
}

static inline void EXEC_PPUADDR()
{
    if (nes_ppu.set_PPU_addr_latch == false)
    {
        nes_ppu_bus.AB                  = (uint16_t) nes_ppu_bus.DB << 8;
        nes_ppu.set_PPU_addr_latch      = true;
    }
    else
    {
        nes_ppu_bus.AB |= nes_ppu_bus.DB;
    }
    nes_ppu.PPU_registers[PPUSCROLL] = nes_ppu_bus.DB;
}

static inline void EXEC_PPUDATA()
{
    if (nes_ppu_bus.RW == 1)
        PPU_POKE(nes_ppu_bus.AB, nes_ppu_bus.DB)
    else
        nes_ppu_bus.DB = PPU_PEEK(nes_ppu_bus.AB);
}

static inline void EXEC_OAMDMA()
{

}

/* 
PPU tick

From https://wiki.nesdev.com/w/index.php/PPU_rendering:

"The PPU renders 262 scanlines per frame. Each scanline lasts for 341 PPU clock 
cycles (113.667 CPU clock cycles; 1 CPU cycle = 3 PPU cycles)""

From Bisqwit, Source: https://bisqwit.iki.fi/jutut/kuvat/programming_examples/nesemu1/nesemu1.cc

         x=0                 x=256      x=340
     ___|____________________|__________|
y=-1    | pre-render scanline| prepare  | >
     ___|____________________| sprites _| > Graphics
y=0     | visible area       | for the  | > processing
        | - this is rendered | next     | > scanlines
y=239   |   on the screen.   | scanline | >
     ___|____________________|______
y=240   | idle
     ___|_______________________________
y=241   | vertical blanking (idle)
        | 20 scanlines long
y=260___|____________________|__________|

Each cycle on the 2A02 (NES CPU) is about 3 PPU cycles
*/
static inline void PPU_tick()
{
    if (nes_ppu.clear_vblank == true)
        nes_ppu_bus.PPU_registers[PPUSTATUS] &= 0x7F;

    
}

