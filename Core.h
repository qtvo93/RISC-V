#ifndef __CORE_H__
#define __CORE_H__

#include "Instruction_Memory.h"

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#define BOOL bool

typedef uint8_t Byte;
typedef int64_t Signal;
typedef int64_t Register;

typedef struct Stage Stage;
struct Core;
typedef struct Core Core;
typedef struct Core
{
    Tick clk; // Keep track of core clock
    Addr PC; // Keep track of program counter

    // What else you need? Data memory? Register file?
    Instruction_Memory *instr_mem;
   
    int64_t data_mem[1024]; // data memory

    Register reg_file[32]; // register file.
	
	Stage *IF;
	Stage *ID;
	Stage *EX;
	Stage *MEM;
	Stage *WB;
	
    bool (*tick)(Core *core);
}Core;

Core *initCore(Instruction_Memory *i_mem);
bool tickFunc(Core *core);

// FIXME. Implement the following functions in Core.c
// FIXME (1). Control Unit.
typedef struct ControlSignals
{
    Signal Branch;
    Signal MemRead;
    Signal MemtoReg;
    Signal ALUOp;
    Signal MemWrite;
    Signal ALUSrc;
    Signal RegWrite;
}ControlSignals;
void ControlUnit(Signal input,
                 Stage *signals);

// FIXME (2). ALU Control Unit.
Signal ALUControlUnit(Signal ALUOp,
                      Signal Funct7,
                      Signal Funct3);

// FIXME (3). Imme. Generator
Signal ImmeGen(Signal input);

// FIXME (4). ALU
void ALU(Signal input_0,
         Signal input_1,
         Signal ALU_ctrl_signal,
         Signal *ALU_result,
         Signal *zero);

// (4). MUX
Signal MUX(Signal sel,
           Signal input_0,
           Signal input_1);

// (5). Add
Signal Add(Signal input_0,
           Signal input_1);

// (6). ShiftLeft1
Signal ShiftLeft1(Signal input);
typedef struct Stage {
	//int current;
	int none;
	Signal ALU_result;
	Signal zero;
	
	Addr PC;
	unsigned instruction;
	int64_t imm;
	
	Signal rs_1_data;
    Signal rs_2_data;
	Signal funct7;
	Signal funct3;
	
    Signal temp_memory;
	Signal ALU_opcode;
    Signal rs_1;
    Signal rs_2;
    Signal rd;
	
	Signal ALUSrc ;
	Signal MemtoReg ;
    Signal RegWrite ;
    Signal MemRead ;
    Signal MemWrite ;
    Signal Branch ;
    Signal ALUOp ;
	
} Stage;

#endif
