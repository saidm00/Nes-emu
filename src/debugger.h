#ifndef DEBUGGER_H
#define DEBUGGER_H

#include <stdint.h>
#include <stdbool.h>

#define HEX_CHARS "0123456789ABCDEF"


typedef struct MOS6502_CodeLine
{
	char *assembly;
	uint16_t addr;
	uint8_t bytes[3];
	uint8_t instructionSize;
} MOS6502_CodeLine;

const MOS6502_CodeLine *debugger_get_line(uint16_t lineIndex);
bool debugger_step(uint16_t *lineIndex);
void init_debugger(uint16_t lowAddr, uint16_t highAddr);
void debugger_disassemble(void);
uint16_t debugger_line_count(void);

#endif // DEBUGGER_H