#pragma once
//#include "nes_ppu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* NES Cartridge data */
typedef struct _nes_cartridge
{
    size_t  CHR_ROM_size,
            PRG_ROM_size;
    
    /* Pointer to NES address space */
    uint8_t * nes_mem;
}
_nes_cartridge;
_nes_cartridge nes_cartridge;

/* Function pointers to select method of memory access */
uint8_t (*PEEK_MAPPER)(uint16_t);
void    (*POKE_MAPPER)(uint16_t, uint8_t);


/* Mapper 000 PEEK */
static uint8_t PEEK_000(uint16_t addr)
{
    /* Internal NES memory */
    if (addr >= 0x0 && addr < 0x2000)
        return nes_cartridge.nes_mem[(addr & 0x07FF)];
    /* PPU Registers */
    if (addr >= 0x2000 && addr < 0x4000)
    {
        //USE_REGS((addr & 0x7), 0, 0x0);
        //return nes_ppu.PPU_registers[(addr & 0x7)];
    }
    
    /* Mirror if PRG_ROM is only 16 KiB */
    if (addr >= 0x8000)
    {
        if (nes_cartridge.PRG_ROM_size == 0x4000)
            return nes_cartridge.nes_mem[(addr & 0x3FFF) + 0x8000];
        else
            return nes_cartridge.nes_mem[addr];
    }
}

/* Mapper 000 POKE */
static void POKE_000(uint16_t addr, uint8_t data)
{
    /* Internal NES memory */
    if (addr >= 0x0 && addr < 0x2000)
        nes_cartridge.nes_mem[(addr & 0x07FF)] = data;
    /* PPU Registers */
    if (addr >= 0x2000 && addr < 0x4000)
        //USE_REGS((addr & 0x7), 1, data);
    /* Mirror if PRG_ROM is only 16 KiB */
    if (addr >= 0x8000)
    {
        if (nes_cartridge.PRG_ROM_size == 0x4000)
            nes_cartridge.nes_mem[(addr & 0x3FFF) + 0x8000] = data;
        else
            nes_cartridge.nes_mem[addr] = data;
    }
}

/* Mapper 000 */
static void mapper_000(FILE * rom)
{
    /* Set PEEK and POKE functions respectively */
    PEEK_MAPPER = PEEK_000;
    POKE_MAPPER = POKE_000;

    /* Program ROM, loaded in the range $8000-$FFFF */
    if (fread(&nes_cartridge.nes_mem[0x8000], sizeof(uint8_t), nes_cartridge.PRG_ROM_size, rom) != nes_cartridge.PRG_ROM_size)
    {
        fprintf(stderr, "error: Failed to copy PRG-ROM: %s. exiting\n", strerror(errno));
        return;
    }

    printf("Successfully mapped memory (mapper_000)!\n");
}

/* Default NULL mapper */
static void mapper_NULL(FILE * rom)
{
    return;
}

/* Function pointer to select correct mapper */
static void (*mapper[256])(FILE*) = {
    mapper_000,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL,
    mapper_NULL
};
