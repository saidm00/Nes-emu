//
// Created by Said on 25/07/2020.
//

#include "debugger.h"
#include "nes_cpu.h"

typedef struct MOS6502_Debugger {
	MOS6502_CodeLine *code;
	uint16_t currAddr;
	uint16_t instructionLen;
	uint16_t lineCount;
	nes_cpu_addr_modes addrMode;
	char operand[32];
	uint16_t lowAddr, highAddr;
} MOS6502_Debugger;

MOS6502_Debugger debugger;


const MOS6502_CodeLine *debugger_get_line(uint16_t lineIndex)
{
	return &debugger.code[lineIndex];
}

void debugger_push_line(const MOS6502_CodeLine *codeLine)
{
	++debugger.lineCount;

	if (debugger.code == NULL)
		debugger.code = malloc(sizeof(MOS6502_CodeLine));
	else
		debugger.code = realloc(debugger.code, debugger.lineCount * sizeof(MOS6502_CodeLine));

	debugger.code[debugger.lineCount-1] = *codeLine;
}

void init_debugger(uint16_t lowAddr, uint16_t highAddr)
{
	debugger.lowAddr = lowAddr;
	debugger.highAddr = highAddr;
	debugger.currAddr = lowAddr;
	debugger.code = NULL;
	debugger.lineCount = 0;
}

#define DBG_CLEAR_OP_STRING() memset(debugger.operand, 0, 32)

bool disassemble_opcode(nes_cpu_opcodes opcode)
{
	char code_line[256];

	const char *opstr = nes_cpu_opcode_str[opcode];
	//if (opstr == NULL) return false;

	short oplen = debugger.instructionLen;


	char comment[64] = "";
	sprintf(code_line, "%s %s", opstr, debugger.operand);
	long len = strlen(code_line);

	MOS6502_CodeLine codeLine;// = &nes_disassembler.code[nes_disassembler.lineIndex];

	for (short i = 0; i < oplen; ++i)
		codeLine.bytes[i] = PEEK(debugger.currAddr + i);

	codeLine.addr = debugger.currAddr;
	codeLine.instructionSize = oplen;

	codeLine.assembly = malloc(len + 1);
	strcpy(codeLine.assembly, code_line);

	debugger_push_line(&codeLine);

	//++nes_disassembler.lineIndex;

	return true;
}


static inline void diassemble_addressing_mode(nes_cpu_addr_modes mode)
{
	DBG_CLEAR_OP_STRING();
	debugger.addrMode = mode;
	char *operand = debugger.operand;

	switch (mode)
	{

		case IMP:
			debugger.instructionLen = 1;
			break;
		case ACC:
			debugger.instructionLen = 1;
			operand[0] = 'A';
			operand[1] = '\0';
			break;
		case REL: {
			debugger.instructionLen = 2;
			static const uint16_t M1 = 0x000FU, M10 = 0x00F0U, M100 = 0x0F00U;

			operand[0] = '*';
			uint8_t op = PEEK(debugger.currAddr + 1);
			operand[1] = op & 0x80U ? '-' : '+';

			/* double dabble bcd converter */
			uint8_t sop = op & ~0x80U;
			uint16_t bcd = 0U; /* only uses first 3 bytes */

			for (int i = 0; i < 8; ++i) {
				if ((bcd & M1) > 4U) bcd += 3U;
				if (((uint16_t)(bcd & M10) >> 4U) > 4U) bcd += 48U; /* 48 = 3 << 4 */
				if (((uint16_t)(bcd & M100) >> 8U) > 4U) bcd += 768U; /* 768 = 3 << 8 */


				bcd = (uint16_t)(bcd << 1U) | ((sop & 0x80U) >> 7U);
				sop <<= 1U;
			}

			if (!((uint16_t)bcd & (uint16_t)(M10 | M100)))
			{
				operand[2] = '0' + (uint16_t)(bcd & M1);
				operand[3] = '\0';
				break;
			}

			if (!((uint16_t)bcd & M100))
			{
				operand[2] = '0' + ((uint16_t)(bcd & M10) >> 4U);
				operand[3] = '0' + (uint16_t)(bcd & M1);
				operand[4] = '\0';
				break;
			}


			operand[2] = '0' + ((uint16_t)(bcd & M100) >> 8U);
			operand[3] = '0' + ((uint16_t)(bcd & M10) >> 4U);
			operand[4] = '0' + (uint16_t)(bcd & M1);
			operand[5] = '\0';

			break;
		}
		case ZP: {
			uint8_t addr = PEEK(debugger.currAddr + 1);

			debugger.instructionLen = 2;

			operand[0] = '$';

			operand[1] = HEX_CHARS[(addr & 0xF0U) >> 4U];
			operand[2] = HEX_CHARS[(addr & 0x0FU)];
			operand[3] = '\0';

			break;
		}
		case ZPX: {
			uint8_t addr = PEEK(debugger.currAddr + 1);

			debugger.instructionLen = 2;

			operand[0] = '$';

			operand[1] = HEX_CHARS[(addr & 0xF0U) >> 4U];
			operand[2] = HEX_CHARS[(addr & 0x0FU)];
			operand[3] = ',';
			operand[4] = 'X';
			operand[5] = '\0';

			break;
		}
		case ZPY: {
			uint8_t addr = PEEK(debugger.currAddr + 1);

			debugger.instructionLen = 2;

			operand[0] = '$';

			operand[1] = HEX_CHARS[(addr & 0xF0U) >> 4U];
			operand[2] = HEX_CHARS[(addr & 0x0FU)];
			operand[3] = ',';
			operand[4] = 'Y';
			operand[5] = '\0';

			break;
		}
		case IMM: {
			uint8_t addr = PEEK(debugger.currAddr + 1);

			debugger.instructionLen = 2;

			operand[0] = '#';
 			operand[1] = '$';
			operand[2] = HEX_CHARS[(addr & 0xF0U) >> 4U];
			operand[3] = HEX_CHARS[(addr & 0x0FU)];
			operand[4] = '\0';

			break;
		}
		case ABS: {
			uint8_t lo = PEEK(debugger.currAddr + 1);
			uint8_t hi = PEEK(debugger.currAddr + 2);

			debugger.instructionLen = 3;

			operand[0] = '$';
			operand[1] = HEX_CHARS[(hi & 0xF0U) >> 4U];
			operand[2] = HEX_CHARS[(hi & 0x0FU)];
			operand[3] = HEX_CHARS[(lo & 0xF0U) >> 4U];
			operand[4] = HEX_CHARS[(lo & 0x0FU)];
			operand[5] = '\0';
			break;
		}
		case ABSX: {
			uint8_t lo = PEEK(debugger.currAddr + 1);
			uint8_t hi = PEEK(debugger.currAddr + 2);

			debugger.instructionLen = 3;

			operand[0] = '$';
			operand[1] = HEX_CHARS[(hi & 0xF0U) >> 4U];
			operand[2] = HEX_CHARS[(hi & 0x0FU)];
			operand[3] = HEX_CHARS[(lo & 0xF0U) >> 4U];
			operand[4] = HEX_CHARS[(lo & 0x0FU)];
			operand[5] = ',';
			operand[6] = 'X';
			operand[7] = '\0';
			break;
		}
		case ABSY: {
			uint8_t lo = PEEK(debugger.currAddr + 1);
			uint8_t hi = PEEK(debugger.currAddr + 2);

			debugger.instructionLen = 3;

			operand[0] = '$';
			operand[1] = HEX_CHARS[(hi & 0xF0U) >> 4U];
			operand[2] = HEX_CHARS[(hi & 0x0FU)];
			operand[3] = HEX_CHARS[(lo & 0xF0U) >> 4U];
			operand[4] = HEX_CHARS[(lo & 0x0FU)];
			operand[5] = ',';
			operand[6] = 'Y';
			operand[7] = '\0';
			break;
		}
		case IND: {
			uint8_t lo = PEEK(debugger.currAddr + 1);
			uint8_t hi = PEEK(debugger.currAddr + 2);

			debugger.instructionLen = 3;

			operand[0] = '(';
			operand[1] = '$';
			operand[2] = HEX_CHARS[(hi & 0xF0U) >> 4U];
			operand[3] = HEX_CHARS[(hi & 0x0FU)];
			operand[4] = HEX_CHARS[(lo & 0xF0U) >> 4U];
			operand[5] = HEX_CHARS[(lo & 0x0FU)];
			operand[6] = ')';
			operand[7] = '\0';
			break;
		}
		case INDX: {
			uint8_t lo = PEEK(debugger.currAddr + 1);
			uint8_t hi = PEEK(debugger.currAddr + 2);

			debugger.instructionLen = 3;

			operand[0] = '(';
			operand[1] = '$';
			operand[2] = HEX_CHARS[(lo & 0xF0U) >> 4U];
			operand[3] = HEX_CHARS[(lo & 0x0FU)];
			operand[4] = ',';
			operand[5] = 'X';
			operand[6] = ')';
			operand[7] = '\0';
			break;
		}
		case INDY: {
			uint8_t lo = PEEK(debugger.currAddr + 1);
			uint8_t hi = PEEK(debugger.currAddr + 2);

			debugger.instructionLen = 3;

			operand[0] = '(';
			operand[1] = '$';
			operand[2] = HEX_CHARS[(lo & 0xF0U) >> 4U];
			operand[3] = HEX_CHARS[(lo & 0x0FU)];
			operand[4] = ')';
			operand[5] = ',';
			operand[6] = 'Y';
			operand[7] = '\0';
			break;
		}
		case NONE: {
			break;
		}
	}

}

nes_cpu_addr_modes opcode_addr_mode[256] = {
	[BRK_IMP] = IMP,
	[ORA_INDX] = INDX,
	[ORA_ZP] = ZP,
	[ASL_ZP] = ZP,
	[PHP_IMP] = IMP,
	[ORA_IMM] = IMM,
	[ASL_ACC] = ACC,
	[ORA_ABS] = ABS,
	[ASL_ABS] = ABS,
	[BPL_REL] = REL,
	[ORA_INDY] = INDY,
	[ORA_ZPX] = ZPX,
	[ASL_ZPX] = ZPX,
	[CLC_IMP] = IMP,
	[ORA_ABSY] = ABSY,
	[ORA_ABSX] = ABSX,
	[ASL_ABSX] = ABSX,
	[JSR_ABS] = ABS,
	[AND_INDX] = INDX,
	[BIT_ZP] = ZP,
	[ROL_ZP] = ZP,
    [PLP_IMP] = IMP, 
    [AND_IMM] = IMM,
    [ROL_ACC] = ACC,
    [BIT_ABS] = ABS,
    [AND_ABS] = ABS,
    [ROL_ABS] = ABS,
    [BMI_REL] = REL,
    [AND_INDY] = INDY,
    [AND_ZPX] = ZPX,
    [ROL_ZPX] = ZPX,
    [SEC_IMP] = IMP, 
    [AND_ABSY] = ABSY,
    [AND_ABSX] = ABSX,
    [ROL_ABSX] = ABSX,
    [RTI_IMP] = IMP,
    [EOR_INDX] = INDX,
    [EOR_ZP] = ZP,
    [LSR_ZP] = ZP,
    [PHA_IMP] = IMP,
    [EOR_IMM] = IMM,
    [LSR_ACC] = ACC,
    [JMP_ABS] = ABS,
    [EOR_ABS] = ABS,
    [LSR_ABS] = ABS,
    [BVC_REL] = REL,
    [EOR_INDY] = INDY,
    [EOR_ZPX] = ZPX,
    [LSR_ZPX] = ZPX,
    [CLI_IMP] = IMP, 
    [EOR_ABSY] = ABSY,
    [EOR_ABSX] = ABSX,
    [LSR_ABSX] = ABSX,
    [RTS_IMP] = IMP,
    [ADC_INDX] = INDX,
    [ADC_ZP] = ZP,
    [ROR_ZP] = ZP,
    [PLA_IMP] = IMP,
    [ADC_IMM] = IMM,
    [ROR_ACC] = ACC,
    [JMP_IND] = IND,
    [ADC_ABS] = ABS,
    [ROR_ABS] = ABS,
    [BVS_REL] = REL,
    [ADC_INDY] = INDY,
    [ADC_ZPX] = ZPX,
    [ROR_ZPX] = ZPX,
    [SEI_IMP] = IMP,
    [ADC_ABSY] = ABSY,
    [ADC_ABSX] = ABSX, 
    [ROR_ABSX] = ABSX,
    [STA_INDX] = INDX,
    [STY_ZP] = ZP,
    [STA_ZP] = ZP,
    [STX_ZP] = ZP,
    [DEY_IMP] = IMP,
    [TXA_IMP] = IMP,
    [STY_ABS] = ABS,
    [STA_ABS] = ABS,
    [STX_ABS] = ABS,         
    [BCC_REL] = REL,  
    [STA_INDY] = INDY,        
    [STY_ZPX] = ZPX,  
    [STA_ZPX] = ZPX,            
    [STX_ZPY] = ZPY,
    [TYA_IMP] = IMP,
    [STA_ABSY] = ABSY,
    [TXS_IMP] = IMP,
    [STA_ABSX] = ABSX,
    [LDY_IMM] = IMM,
    [LDA_INDX] = INDX,
    [LDX_IMM] = IMM,
    [LDY_ZP] = ZP,
    [LDA_ZP] = ZP,
    [LDX_ZP] = ZP,
    [TAY_IMP] = IMP,
    [LDA_IMM] = IMM,
    [TAX_IMP] = IMP,
    [LDY_ABS] = ABS,
    [LDA_ABS] = ABS,
    [LDX_ABS] = ABS,
    [BCS_REL] = REL,
    [LDA_INDY] = INDY,
    [LDY_ZPX] = ZPX,
    [LDA_ZPX] = ZPX,
    [LDX_ZPY] = ZPY,
    [CLV_IMP] = IMP,
    [LDA_ABSY] = ABSY,
    [TSX_IMP] = IMP,
    [LDY_ABSX] = ABSX,
    [LDA_ABSX] = ABSX,
    [LDX_ABSY] = ABSY,
    [CPY_IMM] = IMM,
    [CMP_INDX] = INDX,
    [CPY_ZP] = ZP,
    [CMP_ZP] = ZP,
    [DEC_ZP] = ZP,
    [INY_IMP] = IMP,
    [CMP_IMM] = IMM,
    [DEX_IMP] = IMP,
    [CPY_ABS] = ABS,
    [CMP_ABS] = ABS,
    [DEC_ABS] = ABS,
    [BNE_REL] = REL,
    [CMP_INDY] = INDY,
    [CMP_ZPX] = ZPX,
    [DEC_ZPX] = ZPX,
    [CLD_IMP] = IMP,
    [CMP_ABSY] = ABSY,
    [CMP_ABSX] = ABSX,
    [DEC_ABSX] = ABSX,
    [CPX_IMM] = IMM,
    [SBC_INDX] = INDX,
    [CPX_ZP] = ZP,
    [SBC_ZP] = ZP,
    [INC_ZP] = ZP,
    [INX_IMP] = IMP,
    [SBC_IMM] = IMM,
    [NOP_IMP] = IMP,
    [CPX_ABS] = ABS, 
    [SBC_ABS] = ABS,
    [INC_ABS] = ABS,
    [BEQ_REL] = REL,
    [SBC_INDY] = INDY,
    [SBC_ZPX] = ZPX,
    [INC_ZPX] = ZPX,
    [SED_IMP] = IMP,
    [SBC_ABSY] = ABSY,
    [SBC_ABSX] = ABSX,
    [INC_ABSX] = ABSX
};

void debugger_disassemble(void)
{
	uint8_t opcode;

	while (debugger.currAddr >= debugger.lowAddr
		&& debugger.currAddr <= debugger.highAddr)
	{
		opcode = PEEK(debugger.currAddr);

		nes_cpu_addr_modes mode = opcode_addr_mode[opcode];
		diassemble_addressing_mode(mode);

		//print_opcode(opcode);
		disassemble_opcode(opcode);
		debugger.currAddr += debugger.instructionLen;
		//debugger.lineIndex++;
		//debugger.lineCount++;
	}
}

uint16_t debugger_line_count(void)
{
	return debugger.lineCount;
}

bool debugger_step(uint16_t *lineIndex)
{
	interpret_step();

	uint16_t addr = nes_cpu_registers.PC;

	for (uint16_t i = 0; i < debugger.lineCount; ++i)
	{
		if (debugger.code[i].addr == addr)
		{
			*lineIndex = i;
			return true;
		}
	}

	return false;
}