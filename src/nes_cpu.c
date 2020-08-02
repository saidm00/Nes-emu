#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <time.h>

#include <string.h>
#include <errno.h>

#include "nes_cpu.h"

size_t file_size;

/* init NES cpu internals */
int nes_init_cpu(void)
{
    nes_cpu_registers.A = 0x00;
    nes_cpu_registers.X = 0x00;
    nes_cpu_registers.Y = 0x00;
    nes_cpu_registers.S = 0x34;
    
    nes_cpu_registers.SP = 0xFD;
    nes_cpu_registers.PC = 0xFFFC;

    nes_cpu_bus.AB = 0x0000;
    nes_cpu_bus.DB = 0x0000;

    return 0;
}

/*
Function to load ROM of NES game

iNES format (information from https://wiki.nesdev.com/w/index.php/INES):
-----
An iNES file consists of the following sections, in order:

Header (16 bytes)
Trainer, if present (0 or 512 bytes)
PRG ROM data (16384 * x bytes)
CHR ROM data, if present (8192 * y bytes)
PlayChoice INST-ROM, if present (0 or 8192 bytes)
PlayChoice PROM, if present (16 bytes Data, 16 bytes CounterOut) (this is often missing, see PC10 ROM-Images for details)
Some ROM-Images additionally contain a 128-byte (or sometimes 127-byte) title at the end of the file.

The format of the header is as follows:

0-3: Constant $4E $45 $53 $1A ("NES" followed by MS-DOS end-of-file)
4: Size of PRG ROM in 16 KB units
5: Size of CHR ROM in 8 KB units (Value 0 means the board uses CHR RAM)
6: Flags 6 - Mapper, mirroring, battery, trainer
7: Flags 7 - Mapper, VS/Playchoice, NES 2.0
8: Flags 8 - PRG-RAM size (rarely used extension)
9: Flags 9 - TV system (rarely used extension)
10: Flags 10 - TV system, PRG-RAM presence (unofficial, rarely used extension)
11-15: Unused padding (should be filled with zero, but some rippers put their name across bytes 7-15)

NES 2.0 Format (similar to iNES, information from https://wiki.nesdev.com/w/index.php/NES_2.0):
-----

*/
int nes_load_rom(const char * filename, _nes_cartridge * cart)
{
    /* Flags to check file format, string for telling which format */
    bool iNES = false, NES_20 = false;
    const char * format;

    cart->PRG_ROM_size = 0,
    cart->CHR_ROM_size = 0;

    /* Mapper ID */
    uint8_t mapper_ID = 0;

    /* Debug info to measure # of bytes copied */
    size_t bytes_copied = 0;

    /* If the file pointer is empty, something went wrong with opening it. */
    FILE * rom;
    rom = fopen(filename, "rb");
    if (rom == NULL)
    {
        fprintf(stderr, "error: failed to open %s for writing: %s\n", filename, strerror(errno));
        return -1;
    }
    else
    {
        printf("Successfully opened rom %s!\n", filename);
    }

    /* Get the size of the rom */
    fseek(rom, 0, SEEK_END);
    file_size = ftell(rom);
    rewind(rom);

    /* Buffer for the header of the ROM */
    uint8_t * header = malloc(16 * sizeof(uint8_t));
    if(!fread(header, sizeof(uint8_t), 16, rom))
    {
        fprintf(stderr, "error: Failed to copy ROM header! exiting");
        return -1;
    }

    /* Set cartridge address space to the NES address space */
    cart->nes_mem = nes_cpu_mem.mem;

    /* Check if iNES or NES 2.0 format, shameful copy/paste from https://wiki.nesdev.com/w/index.php/NES_2.0 */
    if ((uint32_t)(header[0] << 24 | header[1] << 16 | header[2] << 8 | header[3]) == 0x4E45531A)
    {
        if( (header[7] & 0xC) == 0x8 )
        {
            NES_20 = true;
            format = "NES 2.0";
        }
        else
        {
            iNES = true;
            format = "iNES";

            /* Check if last 4 bits of flags 10 are not equal to 0, see https://wiki.nesdev.com/w/index.php/INES#Flags_10*/
            if(header[10] & 0xF0 != 0)
            {
                fprintf(stderr, "error: Higher 4 bits of Flags 10 not equal to 0. Not loading ROM.\n");
                return -1;
            }

            /* Get sizes */
            cart->PRG_ROM_size = header[4] * 0x4000; /* Size in 16 KiB units */
            cart->CHR_ROM_size = header[5] * 0x2000; /* Size in  8 KiB units */

            //PRG_RAM_size = (header[8] > 0) ? (header[8] * 0x2000) : 0x2000; /* Size in 8 KiB units */

            /* Get mapper number */
            mapper_ID = (uint8_t)(header[7] & 0xF0) | ((header[6] & 0xF0) >> 4);

            /* Flags 6 and 7 combined into a single byte for your viewing pleasure B-) */
            uint8_t flags = (uint8_t)(header[7] & 0x0F) << 4 | ((header[6] & 0x0F));

            /* TO-DO: check other flags in 6 & 7 */

            /* Check for trainer, load it into address space $7000 */
            if (flags & 0x4)
                fread(&nes_cpu_mem.mem[0x7000], sizeof(uint8_t), 0x200, rom);

            /* Load rom according to which mapper is being used */
            if(mapper_ID > 0)
            {
                fprintf(stderr, "error: Unimplemented mapper number %d. exiting.\n", mapper_ID);
                return -1;
            }
            else
            {
                /* Call the mapper based on the ID */
                (*mapper[mapper_ID])(rom);
            }

            /* TO-DO: load CHR-ROM/RAM into memory and map memory accordingly. */
        }

        printf("Format:\t%s\n", format);
    }
    else
    {
        fprintf(stderr, "error: Unknown format! Exiting\n");
        return -1;
    }

    /* Finally, clear the file pointer and return 0 */
    fclose(rom);

    //nes_cpu_registers.PC = (uint16_t)PEEK(nes_cpu_registers.PC + 1) << 8 | PEEK(nes_cpu_registers.PC);
    nes_cpu_registers.PC = 0x8000;
    return 0;
}

/* literally copy and paste assembled 6502 code here */
void test_emu(uint8_t * program, size_t size)
{
    nes_cpu_registers.PC = 0x8000;
    memcpy(&nes_cpu_mem.mem[nes_cpu_registers.PC], program, size);
}

bool interpret_step(void)
{
    /* Fetch opcode from memory */
    uint8_t opcode = PEEK(nes_cpu_registers.PC);
    
    /* Decode and execute the opcode */
    switch (opcode)
    {
        case BRK_IMP:   
            get_operand_AM(IMP);
            BRK();
            nes_cpu_registers.Cycles = 7;
            break;
        case ORA_INDX:  
            get_operand_AM(INDX);
            ORA();
            nes_cpu_registers.Cycles = 6;
            break;
        case ORA_ZP:    
            get_operand_AM(ZP);
            ORA(); 
            nes_cpu_registers.Cycles = 3; 
            break;
        case ASL_ZP:    
            get_operand_AM(ZP);
            ASL(); 
            nes_cpu_registers.Cycles = 5; 
            break;
        case PHP_IMP:   
            get_operand_AM(IMP); 
            PHP();
            nes_cpu_registers.Cycles = 3; 
            break;
        case ORA_IMM:   
            get_operand_AM(IMM);
            ORA(); 
            nes_cpu_registers.Cycles = 2; 
            break;
        case ASL_ACC:   
            get_operand_AM(ACC); 
            ASL();
            nes_cpu_registers.A = nes_cpu_bus.DB;
             
            nes_cpu_registers.Cycles = 2; 
            break;
        case ORA_ABS:   
            get_operand_AM(ABS);
            ORA(); 
            nes_cpu_registers.Cycles = 4; 
            break;
        case ASL_ABS:   
            get_operand_AM(ABS);
            ASL();
            nes_cpu_registers.Cycles = 6; 
            break;
        case BPL_REL:   
            get_operand_AM(REL); 
            BPL();
            nes_cpu_registers.Cycles = 2; 
            break;
        case ORA_INDY:  
            get_operand_AM(INDY); 
            ORA();
            nes_cpu_registers.Cycles += 5; 
            break;
        case ORA_ZPX:   
            get_operand_AM(ZPX); 
            ORA();
            nes_cpu_registers.Cycles = 4; 
            break;
        case ASL_ZPX:   
            get_operand_AM(ZPX); 
            ASL();
            nes_cpu_registers.Cycles = 6; 
            break;
        case CLC_IMP:   
            get_operand_AM(IMP); 
            CLC();
            nes_cpu_registers.Cycles = 2; 
            break;
        case ORA_ABSY:  
            get_operand_AM(ABSY); 
            ORA();
            nes_cpu_registers.Cycles += 4; 
            break;
        case ORA_ABSX:  
            get_operand_AM(ABSX); 
            ORA();
            nes_cpu_registers.Cycles += 4; 
            break;
        case ASL_ABSX:  
            get_operand_AM(ABSX); 
            ASL();
            nes_cpu_registers.Cycles = 7; 
            break;
        case JSR_ABS:   
            get_operand_AM(ABS); 
            JSR();
            nes_cpu_registers.Cycles = 6; 
            break;
        case AND_INDX:  
            get_operand_AM(INDX); 
            AND();
            nes_cpu_registers.Cycles = 6; 
            break;
        case BIT_ZP:    
            get_operand_AM(ZP); 
            BIT();
            nes_cpu_registers.Cycles = 3; 
            break;
        case AND_ZP:    
            get_operand_AM(ZP); 
            AND();
            nes_cpu_registers.Cycles = 3; 
            break;
        case ROL_ZP:    
            get_operand_AM(ZP); 
            ROL();
            nes_cpu_registers.Cycles = 5; 
            break;
        case PLP_IMP:   
            get_operand_AM(IMP);
            PLP();
            nes_cpu_registers.Cycles = 4; 
            break;
        case AND_IMM:   
            get_operand_AM(IMM); 
            AND();
            nes_cpu_registers.Cycles = 2; 
            break;
        case ROL_ACC:   
            get_operand_AM(ACC);
            ROL(); 
            nes_cpu_registers.A = nes_cpu_bus.DB;
             
            nes_cpu_registers.Cycles = 2; 
            break;
        case BIT_ABS:   
            get_operand_AM(ABS); 
            BIT();
            nes_cpu_registers.Cycles = 4; 
            break;
        case AND_ABS:   
            get_operand_AM(ABS); 
            AND();
            nes_cpu_registers.Cycles = 4; 
            break;
        case ROL_ABS:   
            get_operand_AM(ABS); 
            ROL();
            nes_cpu_registers.Cycles = 6; 
            break;
        case BMI_REL:   
            get_operand_AM(REL); 
            BMI();
            nes_cpu_registers.Cycles = 2; 
            break;
        case AND_INDY:  
            get_operand_AM(INDY); 
            AND();
            nes_cpu_registers.Cycles += 5; 
            break;
        case AND_ZPX:   
            get_operand_AM(ZPX); 
            AND();
            nes_cpu_registers.Cycles = 4; 
            break;
        case ROL_ZPX:   
            get_operand_AM(ZPX); 
            ROL();
            nes_cpu_registers.Cycles = 6; 
            break;
        case SEC_IMP:   
            get_operand_AM(IMP);
            SEC();
            nes_cpu_registers.Cycles = 2; 
            break;
        case AND_ABSY:  
            get_operand_AM(ABSY); 
            AND();
            nes_cpu_registers.Cycles += 4; 
            break;
        case AND_ABSX:  
            get_operand_AM(ABSX); 
            AND();
            nes_cpu_registers.Cycles += 4; 
            break;
        case ROL_ABSX:  
            get_operand_AM(ABSX); 
            ROL();
            nes_cpu_registers.Cycles = 7; 
            break;
        case RTI_IMP:   
            get_operand_AM(IMP);
            RTI();
            nes_cpu_registers.Cycles = 6;
            break;
        case EOR_INDX:  
            get_operand_AM(INDX); 
            EOR();
            nes_cpu_registers.Cycles = 6; 
            break;
        case EOR_ZP:    
            get_operand_AM(ZP); 
            EOR();
            nes_cpu_registers.Cycles = 3; 
            break;
        case LSR_ZP:    
            get_operand_AM(ZP); 
            LSR();
            nes_cpu_registers.Cycles = 5; 
            break;
        case PHA_IMP:  
            get_operand_AM(IMP); 
            PHA();
            nes_cpu_registers.Cycles = 3;
            break;
        case EOR_IMM:   
            get_operand_AM(IMM); 
            EOR();
            nes_cpu_registers.Cycles = 2; 
            break;
        case LSR_ACC:   
            get_operand_AM(ACC);
            LSR(); 
            nes_cpu_registers.A = nes_cpu_bus.DB;
             
            nes_cpu_registers.Cycles = 2; 
            break;
        case JMP_ABS:   
            get_operand_AM(ABS); 
            JMP();
            nes_cpu_registers.Cycles = 3; 
            break;
        case EOR_ABS:   
            get_operand_AM(ABS); 
            EOR();
            nes_cpu_registers.Cycles = 4; 
            break;
        case LSR_ABS:   
            get_operand_AM(ABS); 
            LSR();
            nes_cpu_registers.Cycles = 6; 
            break;
        case BVC_REL:   
            get_operand_AM(REL); 
            BVC();
            nes_cpu_registers.Cycles += 2; 
            break;
        case EOR_INDY:  
            get_operand_AM(INDY); 
            EOR();
            nes_cpu_registers.Cycles += 5; 
            break;
        case EOR_ZPX:   
            get_operand_AM(ZPX); 
            EOR();
            nes_cpu_registers.Cycles = 4; 
            break;
        case LSR_ZPX:   
            get_operand_AM(ZPX); 
            LSR();
            nes_cpu_registers.Cycles = 6; 
            break;
        case CLI_IMP:   
            get_operand_AM(IMP);
            CLI();
            nes_cpu_registers.Cycles = 2;
            break;
        case EOR_ABSY:  
            get_operand_AM(ABSY); 
            EOR();
            nes_cpu_registers.Cycles += 4; 
            break;
        case EOR_ABSX:  
            get_operand_AM(ABSX); 
            EOR();
            nes_cpu_registers.Cycles += 4; 
            break;
        case LSR_ABSX:  
            get_operand_AM(ABSX); 
            LSR();
            nes_cpu_registers.Cycles = 7; 
            break;
        case RTS_IMP:   
            get_operand_AM(IMP);
            RTS();
            nes_cpu_registers.Cycles = 6;
            break;
        case ADC_INDX:  
            get_operand_AM(INDX); 
            ADC();
            nes_cpu_registers.Cycles = 6; 
            break;
        case ADC_ZP:    
            get_operand_AM(ZP); 
            ADC();
            nes_cpu_registers.Cycles = 3; 
            break;
        case ROR_ZP:    
            get_operand_AM(ZP); 
            ROR();
            nes_cpu_registers.Cycles = 5; 
            break;
        case PLA_IMP:   
            get_operand_AM(IMP);
            PLA();
            nes_cpu_registers.Cycles = 4;
            break;
        case ADC_IMM:   
            get_operand_AM(IMM); 
            ADC();
            nes_cpu_registers.Cycles = 2; 
            break;
        case ROR_ACC:   
            get_operand_AM(ACC);
            ROR(); 
            nes_cpu_registers.A = nes_cpu_bus.DB;
             
            nes_cpu_registers.Cycles = 2; 
            break;
        case JMP_IND: 
            get_operand_AM(IND); 
            JMP();
            nes_cpu_registers.Cycles = 5; 
            break;
        case ADC_ABS:   
            get_operand_AM(ABS); 
            ADC();
            nes_cpu_registers.Cycles = 4; 
            break;
        case ROR_ABS:   
            get_operand_AM(ABS); 
            ROR();
            nes_cpu_registers.Cycles = 6; 
            break;
        case BVS_REL:   
            get_operand_AM(REL); 
            BVS();
            nes_cpu_registers.Cycles += 2; 
            break;
        case ADC_INDY:  
            get_operand_AM(INDY); 
            ADC();
            nes_cpu_registers.Cycles += 5; 
            break;
        case ADC_ZPX:   
            get_operand_AM(ZPX); 
            ADC();
            nes_cpu_registers.Cycles = 4; 
            break;
        case ROR_ZPX:   
            get_operand_AM(ZPX); 
            ROR();
            nes_cpu_registers.Cycles = 6; 
            break;
        case SEI_IMP:   
            get_operand_AM(IMP);
            SEI();
            nes_cpu_registers.Cycles = 2;
            break;
        case ADC_ABSY:  
            get_operand_AM(ABSY); 
            ADC();
            nes_cpu_registers.Cycles += 4; 
            break;
        case ADC_ABSX:  
            get_operand_AM(ABSX); 
            ADC();
            nes_cpu_registers.Cycles += 4; 
            break;
        case ROR_ABSX:  
            get_operand_AM(ABSX); 
            ROR();
            nes_cpu_registers.Cycles = 7; 
            break;
        case STA_INDX:  
            get_operand_AM(INDX); 
            STA();
            nes_cpu_registers.Cycles = 6; 
            break;
        case STY_ZP:    
            get_operand_AM(ZP); 
            STY();
            nes_cpu_registers.Cycles = 3; 
            break;
        case STA_ZP:    
            get_operand_AM(ZP); 
            STA();
            nes_cpu_registers.Cycles = 3; 
            break;
        case STX_ZP:    
            get_operand_AM(ZP); 
            STX();
            nes_cpu_registers.Cycles = 3; 
            break;
        case DEY_IMP:   
            get_operand_AM(IMP);
            DEY();
            nes_cpu_registers.Cycles = 2;
            break;
        case TXA_IMP:   
            get_operand_AM(IMP);
            TXA();
            nes_cpu_registers.Cycles = 2;
            break;
        case STY_ABS:   
            get_operand_AM(ABS); 
            STY();
            nes_cpu_registers.Cycles = 4; 
            break;
        case STA_ABS:   
            get_operand_AM(ABS); 
            STA();
            nes_cpu_registers.Cycles = 4; 
            break;
        case STX_ABS:   
            get_operand_AM(ABS); 
            STX();
            nes_cpu_registers.Cycles = 4; 
            break;
        case BCC_REL:   
            get_operand_AM(REL); 
            BCC();
            nes_cpu_registers.Cycles = 2; 
            break;
        case STA_INDY:  
            get_operand_AM(INDY); 
            STA();
            nes_cpu_registers.Cycles = 6; 
            break;
        case STY_ZPX:   
            get_operand_AM(ZPX); 
            STY();
            nes_cpu_registers.Cycles = 4; 
            break;
        case STA_ZPX:   
            get_operand_AM(ZPX); 
            STA();
            nes_cpu_registers.Cycles = 4; 
            break;
        case STX_ZPY:   
            get_operand_AM(ZPY); 
            STX();
            nes_cpu_registers.Cycles = 4; 
            break;
        case TYA_IMP:   
            get_operand_AM(IMP);
            TYA();
            nes_cpu_registers.Cycles = 2;
            break;
        case STA_ABSY:  
            get_operand_AM(ABSY); 
            STA();
            nes_cpu_registers.Cycles = 5; 
            break;
        case TXS_IMP:   
            get_operand_AM(IMP);
            TXS();
            nes_cpu_registers.Cycles = 2;
            break;
        case STA_ABSX:  
            get_operand_AM(ABSX); 
            STA();
            nes_cpu_registers.Cycles = 5; 
            break;
        case LDY_IMM:   
            get_operand_AM(IMM); 
            LDY();
            nes_cpu_registers.Cycles = 2; 
            break;
        case LDA_INDX:  
            get_operand_AM(INDX); 
            LDA();
            nes_cpu_registers.Cycles = 6; 
            break;
        case LDX_IMM:   
            get_operand_AM(IMM); 
            LDX();
            nes_cpu_registers.Cycles = 2; 
            break;
        case LDY_ZP:    
            get_operand_AM(ZP); 
            LDY();
            nes_cpu_registers.Cycles = 3; 
            break;
        case LDA_ZP:    
            get_operand_AM(ZP); 
            LDA();
            nes_cpu_registers.Cycles = 3; 
            break;
        case LDX_ZP:    
            get_operand_AM(ZP); 
            LDX();
            nes_cpu_registers.Cycles = 3; 
            break;
        case TAY_IMP:   
            get_operand_AM(IMP);
            TAY();
            nes_cpu_registers.Cycles = 2;
            break;
        case LDA_IMM:   
            get_operand_AM(IMM); 
            LDA();
            nes_cpu_registers.Cycles = 2; 
            break;
        case TAX_IMP:   
            get_operand_AM(IMP);
            TAX();
            nes_cpu_registers.Cycles = 2; 
            break;
        case LDY_ABS:   
            get_operand_AM(ABS); 
            LDY();
            nes_cpu_registers.Cycles = 4; 
            break;
        case LDA_ABS:   
            get_operand_AM(ABS); 
            LDA();
            nes_cpu_registers.Cycles = 4; 
            break;
        case LDX_ABS:   
            get_operand_AM(ABS); 
            LDX();
            nes_cpu_registers.Cycles = 4; 
            break;
        case BCS_REL:   
            get_operand_AM(REL); 
            BCS();
            nes_cpu_registers.Cycles = 2; 
            break;
        case LDA_INDY:  
            get_operand_AM(INDY); 
            LDA();
            nes_cpu_registers.Cycles += 5; 
            break;
        case LDY_ZPX:   
            get_operand_AM(ZPX); 
            LDY();
            nes_cpu_registers.Cycles = 4; 
            break;
        case LDA_ZPX:   
            get_operand_AM(ZPX); 
            LDA();
            nes_cpu_registers.Cycles = 4; 
            break;
        case LDX_ZPY:   
            get_operand_AM(ZPY); 
            LDX();
            nes_cpu_registers.Cycles = 4; 
            break;
        case CLV_IMP:   
            get_operand_AM(IMP); 
            CLV();
            nes_cpu_registers.Cycles = 2; 
            break;
        case LDA_ABSY:  
            get_operand_AM(ABSY); 
            LDA();
            nes_cpu_registers.Cycles += 4; 
            break;
        case TSX_IMP:   
            TSX();
            nes_cpu_registers.Cycles = 2;
            nes_cpu_registers.PC += 1; 
            break;
        case LDY_ABSX:  
            get_operand_AM(ABSX); 
            LDY();
            nes_cpu_registers.Cycles += 4; 
            break;
        case LDA_ABSX:  
            get_operand_AM(ABSX); 
            LDA();
            nes_cpu_registers.Cycles += 4; 
            break;
        case LDX_ABSY:  
            get_operand_AM(ABSY); 
            LDX();
            nes_cpu_registers.Cycles += 4; 
            break;
        case CPY_IMM:   
            get_operand_AM(IMM); 
            CPY();
            nes_cpu_registers.Cycles = 2; 
            break;
        case CMP_INDX:  
            get_operand_AM(INDX); 
            CMP();
            nes_cpu_registers.Cycles = 6; 
            break;
        case CPY_ZP:    
            get_operand_AM(ZP); 
            CPY();
            nes_cpu_registers.Cycles = 3; 
            break;
        case CMP_ZP:    
            get_operand_AM(ZP); 
            CMP();
            nes_cpu_registers.Cycles = 3; 
            break;
        case DEC_ZP:    
            get_operand_AM(ZP); 
            DEC();
            nes_cpu_registers.Cycles = 5; 
            break;
        case INY_IMP:   
            get_operand_AM(IMP); 
            INY();
            nes_cpu_registers.Cycles = 2; 
            break;
        case CMP_IMM:   
            get_operand_AM(IMM); 
            CMP();
            nes_cpu_registers.Cycles = 2; 
            break;
        case DEX_IMP:   
            get_operand_AM(IMP); 
            DEX();
            nes_cpu_registers.Cycles = 2; 
            break;
        case CPY_ABS:   
            get_operand_AM(ABS); 
            CPY();
            nes_cpu_registers.Cycles = 4; 
            break;
        case CMP_ABS:   
            get_operand_AM(ABS); 
            CMP();
            nes_cpu_registers.Cycles = 4; 
            break;
        case DEC_ABS:   
            get_operand_AM(ABS); 
            DEC();
            nes_cpu_registers.Cycles = 6; 
            break;
        case BNE_REL:   
            get_operand_AM(REL); 
            BNE();
            nes_cpu_registers.Cycles = 2; 
            break;
        case CMP_INDY:  
            get_operand_AM(INDY); 
            CMP();
            nes_cpu_registers.Cycles += 5; 
            break;
        case CMP_ZPX:   
            get_operand_AM(ZPX); 
            CMP();
            nes_cpu_registers.Cycles = 4; 
            break;
        case DEC_ZPX:   
            get_operand_AM(ZPX); 
            DEC();
            nes_cpu_registers.Cycles = 6; 
            break;
        case CLD_IMP:   
            get_operand_AM(IMP); 
            CLD();
            nes_cpu_registers.Cycles = 2; 
            break;
        case CMP_ABSY:  
            get_operand_AM(ABSY); 
            CMP();
            nes_cpu_registers.Cycles += 4; 
            break;
        case CMP_ABSX:  
            get_operand_AM(ABSX); 
            CMP();
            nes_cpu_registers.Cycles += 4; 
            break;
        case DEC_ABSX:  
            get_operand_AM(ABSX); 
            DEC();
            nes_cpu_registers.Cycles = 7; 
            break;
        case CPX_IMM:   
            get_operand_AM(IMM); 
            CPX();
            nes_cpu_registers.Cycles = 2; 
            break;
        case SBC_INDX:  
            get_operand_AM(INDX); 
            SBC();
            nes_cpu_registers.Cycles = 6; 
            break;
        case CPX_ZP:    
            get_operand_AM(ZP); 
            CPX();
            nes_cpu_registers.Cycles = 3; 
            break;
        case SBC_ZP:    
            get_operand_AM(ZP); 
            SBC();
            nes_cpu_registers.Cycles = 3; 
            break;
        case INC_ZP:    
            get_operand_AM(ZP); 
            INC();
            nes_cpu_registers.Cycles = 5; 
            break;
        case INX_IMP:   
            get_operand_AM(IMP); 
            INX();
            nes_cpu_registers.Cycles = 2; 
            break;
        case SBC_IMM:   
            get_operand_AM(IMM); 
            SBC();
            nes_cpu_registers.Cycles = 2; 
            break;
        case NOP_IMP:   
            get_operand_AM(IMP); 
            NOP();
            nes_cpu_registers.Cycles = 2; 
            break;
        case CPX_ABS:   
            get_operand_AM(ABS); 
            CPX();
            nes_cpu_registers.Cycles = 4; 
            break;
        case SBC_ABS:   
            get_operand_AM(ABS); 
            SBC();
            nes_cpu_registers.Cycles = 4; 
            break;
        case INC_ABS:   
            get_operand_AM(ABS); 
            INC();
            nes_cpu_registers.Cycles = 6; 
            break;
        case BEQ_REL:   
            get_operand_AM(REL); 
            BEQ();
            nes_cpu_registers.Cycles = 2; 
            break;
        case SBC_INDY:  
            get_operand_AM(INDY); 
            SBC();
            nes_cpu_registers.Cycles += 5; 
            break;
        case SBC_ZPX:   
            get_operand_AM(ZPX); 
            SBC();
            nes_cpu_registers.Cycles = 4; 
            break;
        case INC_ZPX:   
            get_operand_AM(ZPX); 
            INC();
            nes_cpu_registers.Cycles = 6; 
            break;
        case SED_IMP:   
            get_operand_AM(IMP);
            SED();
            nes_cpu_registers.Cycles = 2;
            break;
        case SBC_ABSY:  
            get_operand_AM(ABSY); 
            SBC();
            nes_cpu_registers.Cycles += 4; 
            break;
        case SBC_ABSX:  
            get_operand_AM(ABSX); 
            SBC();
            nes_cpu_registers.Cycles += 4; 
            break;
        case INC_ABSX:  
            get_operand_AM(ABSX); 
            INC();
            nes_cpu_registers.Cycles = 7; 
            break;
        default:
            fprintf(stderr, "error: unknown opcode 0x%02X\n", opcode);
    }
    
    /* Increment the program counter accordingly */
    nes_cpu_registers.PC += PC_offset;

    return true; /* no breaks */
}

/* Finally, the "meat and potatoes" of the emulator, the interpreter! */
void interpret(void)
{
    uint8_t no_of_breaks = 0;

    while (interpret_step())
    {
        /* The clock of the emulator, for timing purposes */
        CPU_tick();
    }
}

#if 0
/* Driver code */
int main(int argc, char** argv)
{
    /* TO-DO: 
        1. add timing (akin to Wait() function)
        2. add checks for page boundary crossing
        3. add option to dump NES memory 
        4. properly replicate NES memory layout
    */

    /* Zero out registers, init CPU */
    nes_init_cpu();

    /* Check if only one argument after file name */    
    if (argc != 2) 
    {
        fprintf(stderr, "error: Invalid usage. USAGE:\n./nes_cpu [FILE]\n");
        return -1;
    }
    else
    {
        /* Load the rom into NES memory */  
        if (nes_load_rom(argv[1], &nes_cartridge) != 0) 
        {
            return -1;
        }
    }

    /* Begin interpreter */
    interpret();
    print_zp();
}

#endif