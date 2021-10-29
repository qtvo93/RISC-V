#include "Core.h"
#include <stdint.h>
/* 
	Author: Quoc Thinh Vo 
	Last edit: Feb 21th, 2021  
	
 */
void IF_S(Stage *Stage, Core *core);
void ID_S(Stage *Stage, Core *core);
void EX_S(Stage *Stage, Core *core, int forwardA, int forwardB, Signal result_1, Signal result_2);
void MEM_S(Stage *Stage, Core *core);
void WB_S(Stage *Stage, Core *core);
Core *initCore(Instruction_Memory *i_mem)
{
    Core *core = (Core *)malloc(sizeof(Core));
    core->clk = 0;
    core->PC = 0;
    core->instr_mem = i_mem;
    core->tick = tickFunc;
	core->IF = (Stage *) malloc(sizeof(Stage));
	core->ID = (Stage *) malloc(sizeof(Stage));
	core->EX= (Stage *) malloc(sizeof(Stage));
	core->MEM= (Stage *) malloc(sizeof(Stage));
	core->WB = (Stage *) malloc(sizeof(Stage));
	
	core->IF->none = 1;
	core->ID->none = 1;
	core->EX->none = 1;
	core->MEM->none = 1;
	core->WB->none = 1;
	core->IF->PC = 0;
	core->ID->PC = 0;
	core->EX->PC = 0;
	core->MEM->PC = 0;
	core->WB->PC = 0;

	
	core->reg_file[1] = 0;
	core->reg_file[5] = 26;
	core->reg_file[6] = -27;

	core->data_mem[5] = 100; 
	
    return core;

}

// FIXME, implement this function
bool tickFunc(Core *core)
{
	
    // => change to Stage functions	
	int forwardA = 0;
	int forwardB = 0;

	if (core->EX->rs_1 == core->MEM->rd && !core->MEM->MemWrite && !core->MEM->Branch) {
		forwardA = 2;
	} else if (core->EX->rs_1 == core->WB->rd && !core->WB->MemWrite && !core->WB->Branch) {
		forwardA = 1;
	}

	if (core->EX->rs_2 == core->MEM->rd && !core->MEM->MemWrite && !core->MEM->Branch) {
		forwardB = 2;
	} else if (core->EX->rs_2 == core->WB->rd && !core->WB->MemWrite && !core->WB->Branch) {
		forwardB = 1;
	}

	int wbResult;
	wbResult = core->WB->ALU_result;
	  
	if(core->WB->MemtoReg) {
		wbResult = core->WB->temp_memory;
	} else if(core->WB->Branch && core->WB->RegWrite) {
		wbResult = core->WB->PC + 4;
	}

	int memResult;
	memResult = core->MEM->ALU_result;
	
	if(core->MEM->MemRead) {
		core->MEM->temp_memory = core->data_mem[core->MEM->ALU_result/8];
	}
	if(core->MEM->MemtoReg) {
		memResult = core->MEM->temp_memory;
	} else if(core->MEM->Branch && core->MEM->RegWrite) {
		memResult = core->MEM->PC + 4;
	}

	WB_S(core->WB, core);
	 
	MEM_S(core->MEM, core);

	EX_S(core->EX, core, forwardA, forwardB, wbResult, memResult);
	 
	ID_S(core->ID, core);

	core->IF = (Stage *) malloc(sizeof(Stage));
	core->IF->PC = core->PC;
	if (core->PC > core->instr_mem->last->addr){
		core->IF->none = 1;
	} else {
		core->IF->none = 0;
	}
	 
	IF_S(core->IF, core);



	uint64_t next_PC = core->PC += 4;

	uint8_t funct3 = ((core->ID->instruction & (0b111 << 12)) >> 12);
	uint8_t zero = 0;
	uint64_t input_2 = 0;
	uint64_t input_1 = core->ID->rs_1_data;
	if(core->ID->ALUSrc) {
		input_2 = core->ID->imm;
	} else {
		input_2 = core->ID->rs_2_data;
	}
		forwardA = 0;
		forwardB = 0;

	if (core->ID->rs_1 == core->EX->rd && !core->EX->MemWrite && !core->EX->Branch) {
		forwardA = 2;
	} else if (core->ID->rs_1 == core->MEM->rd && !core->MEM->MemWrite && !core->MEM->Branch) {
		forwardA = 1;
	}

	if (core->ID->rs_2 == core->EX->rd && !core->EX->MemWrite && !core->EX->Branch) {
		forwardB = 2;
	} else if (core->ID->rs_2 == core->MEM->rd && !core->MEM->MemWrite && !core->MEM->Branch) {
		forwardB = 1;
	}
	if (forwardA == 1) {
		input_1 = core->MEM->ALU_result;
	} else if (forwardA == 2) {
		input_1 = core->EX->ALU_result;
	}

	if (forwardB == 1) {
		input_2 = core->MEM->ALU_result;
	} else if (forwardB == 2) {
		input_2 = core->EX->ALU_result;
	}

	if(core->ID->ALUOp == 1) { //Branch
		if (funct3 == 0b001)  //BNE
		zero = (input_1 != input_2);	
	}

	if (core->ID->Branch && zero && !core->ID->none) {
		next_PC = ShiftLeft1(core->ID->imm);
		core->IF->none = 1;
	}
	   
	
	// (Step 7) Increment PC. FIXME, is it correct to always increment PC by 4?!
    core->PC = next_PC;
	
    ++core->clk;
    // Are we reaching the final instruction?	
    if (core->ID->PC > core->instr_mem->last->addr && core->WB->PC  > core->instr_mem->last->addr &&
		core->IF->PC > core->instr_mem->last->addr && core->MEM->PC > core->instr_mem->last->addr &&
		core->EX->PC > core->instr_mem->last->addr ) {
		return false;			
	} else {
		core->WB = core->MEM;
		core->MEM= core->EX;
		core->EX = core->ID;
		core->ID = core->IF;
		return true;
	}
}

// FIXME (1). Control Unit. Refer to Figure 4.18.
void ControlUnit(Signal input,
                 Stage *signals)
{
	Signal opcode = input;
	
    signals->ALUSrc = 0;
	signals->MemtoReg = 0;
    signals->RegWrite = 0;
    signals->MemRead = 0;
    signals->MemWrite = 0;
    signals->Branch = 0;
    signals->ALUOp = 0;
	
	if (opcode == 3) { //ld
    signals->MemRead = 1;
    signals->MemtoReg = 1;
    signals->ALUSrc  = 1;
    signals->RegWrite = 1;
	
	} else if(opcode == 19) { //I instructions
    signals->ALUOp = 3;
    signals->ALUSrc  = 1;
    signals->RegWrite= 1;
	
	} else if (opcode == 0b0110011) { //R instructions
    signals->ALUOp = 2;
    signals->RegWrite = 1;
	
	} else if (opcode == 99) { //Branch instructions
    signals->ALUOp= 1;
    signals->Branch = 1;
	}
	else if (opcode == 35) { //Store instructions
    signals->MemWrite = 1;
    signals->ALUSrc = 1;
	}
	
}

// FIXME (2). ALU Control Unit. Refer to Figure 4.12.
Signal ALUControlUnit(Signal ALUOp,
                      Signal Funct7,
                      Signal Funct3)
{
    // For add
    if (ALUOp == 2 && Funct7 == 0 && Funct3 == 0)
    {
        return 2;
    }
    
	// For subtract
    if (ALUOp == 2 && Funct7 == 40 && Funct3 == 0)
    {
        return 6;
    }

	// For OR
    if (ALUOp == 2 && Funct7 == 0 && Funct3 == 6)
    {
        return 1;
    }

	// For AND
    if (ALUOp == 2 && Funct7 == 0 && Funct3 == 7)
    {
        return 0;
    }

	if (ALUOp == 2 && Funct3 == 1)
    {
        return 3;
    }	
	
	// For Load
	if (ALUOp == 0 )
	{
		return 2;
	}
	
	// For branch BNE
	if (ALUOp == 1 && Funct3 == 1)
	{
		return 11;
	}
	// For Addi
	if (ALUOp == 3 && Funct3 == 0)
	{
		return 2;
	}
	// for shift
	if (ALUOp == 3 && Funct3 == 1)
	{
		return 3;
	}
}

// FIXME (3). Imme. Generator

Signal ImmeGen(Signal input)
{		
	Signal imm = 0;
	Signal instruction = input;
	Signal opcode = (instruction & 0b1111111);
	
	
	if(opcode == 0b0010011 || opcode == 0b0000011 || opcode == 0b1100111) { //I instructions
		imm |= ((instruction & (0b111111111111 << 20)) >> 20);
		if(imm & 0x800) {
		  imm |= 0xFFFFF000;
		}
	} else if(opcode == 0b0100011) { //S instructions
		imm |= ((instruction & (0b11111 << 7)) >> 7);
		imm |= ((instruction & (0b1111111 << 25)) >> 20);
		if(imm & 0x800) {
		  imm |= 0xFFFFF000;
		}
	} else if(opcode == 0b1100011) { //SB instructions
		imm |= ((instruction & (0b1 << 7)) << 4);
		imm |= ((instruction & (0b1111 << 8)) >> 7);
		imm |= ((instruction & (0b111111 << 25)) >> 20);
		imm |= ((instruction & (0b1 << 31)) >> 19);
		if(imm & 0x1000) {
		//   imm = -4;
		  imm = 0xFFFFF000;
		}
	} 

  return imm;
}  

// FIXME (4). ALU
void ALU(Signal input_0,
         Signal input_1,
         Signal ALU_ctrl_signal,
         Signal *ALU_result,
         Signal *zero)
{
    // For addition
    if (ALU_ctrl_signal == 2)
    {
        *ALU_result = (input_0 + input_1);
        if (*ALU_result == 0) { *zero = 1; } else { *zero = 0; }
    }
	
	// For Subtract
	if (ALU_ctrl_signal == 6)
    {
        *ALU_result = (input_0 - input_1);
        if (*ALU_result == 0) { *zero = 1; } else { *zero = 0; }
    }
	
	// For AND
	if (ALU_ctrl_signal == 0)
    {
        *ALU_result = (input_0 & input_1);
        if (*ALU_result == 0) { *zero = 1; } else { *zero = 0; }
    }
	
	// For OR
	if (ALU_ctrl_signal == 1)
    {
        *ALU_result = (input_0 | input_1);
        if (*ALU_result == 0) { *zero = 1; } else { *zero = 0; }
    }
	
	// Shift left????? how about 3?
	if (ALU_ctrl_signal == 3)
    { 
		*ALU_result = input_0  << input_1;
        if (*ALU_result == 0) { *zero = 1; } else { *zero = 0; }
    }
	//BNE branch
	if (ALU_ctrl_signal == 11) { 
		if (input_0 != input_1) {*ALU_result = 1;} else {*ALU_result = 0;}			
		*zero = *ALU_result;
	}
}

// (4). MUX
Signal MUX(Signal sel,
           Signal input_0,
           Signal input_1)
{
    if (sel == 0) { return input_0; } else { return input_1; }
}

// (5). Add
Signal Add(Signal input_0,
           Signal input_1)
{
    return (input_0 + input_1);
}

// (6). ShiftLeft1
Signal ShiftLeft1(Signal input)
{
    return (input*2);
}

void IF_S(Stage *Stage, Core *core) {
	if (Stage->none ==1) {
		return;
	}
	  // (Step 1) Reading instruction from instruction memory
	Stage->instruction = core->instr_mem->instructions[Stage->PC / 4].instruction;
}

void ID_S(Stage *Stage, Core *core) {
	if (Stage->none == 1) {
		return;
	} else {
	  
	Stage->rs_1 = (Stage->instruction & (0b11111 << 15)) >> 15;
	Stage->rs_2 = (Stage->instruction & (0b11111 << 20)) >> 20;
	Stage->rs_1_data = core->reg_file[Stage->rs_1];
	Stage->rs_2_data = core->reg_file[Stage->rs_2];
	Stage->rd = (Stage->instruction & (0b11111 << 7)) >> 7;
	Stage->imm = ImmeGen(Stage->instruction);
	
	Stage->funct3 = ((0b111 << 12) & Stage->instruction) >> 12;
	Stage->funct7 = ((0b1111111 << 25) & Stage->instruction) >> 25;
	Stage->ALU_opcode = Stage->instruction & 0b1111111 ;
	ControlUnit(Stage->ALU_opcode,Stage);
	}
}

void EX_S(Stage *Stage, Core *core, int forwardA, int forwardB, Signal result_1, Signal result_2) {
    if (Stage->none ==1) {
       return;
    }
	 
	Signal input_2 = 0;
	Signal input_1 = Stage->rs_1_data;
	if(Stage->ALUSrc) {
		input_2 = Stage->imm;
	} else {
		input_2 = Stage->rs_2_data;
	}

	if (forwardA == 1) {
		input_1  = result_1;
	} else if (forwardA == 2) {
		input_1  = result_2;
	}

	if (forwardB == 1) {
		if (!Stage->ALUSrc) {
		   input_2 = result_1;
		}
		Stage->rs_2_data = result_1;
	} else if (forwardB == 2 && !Stage->ALUSrc) {
		if (!Stage->ALUSrc) {
		   input_2 = result_2; 
		}
		Stage->rs_2_data = result_2;
	}
	  	
	Signal ALU_control_signal = ALUControlUnit(Stage->ALUOp,Stage->funct7,Stage->funct3 );		
	ALU(input_1, input_2, ALU_control_signal, &Stage->ALU_result, &Stage->zero);
	
	}

void MEM_S(Stage *Stage, Core *core) {
	if (Stage->none == 1) {
		return;
	}
	Stage->temp_memory = 0;
  

	if(Stage->MemWrite) { 		
		core->data_mem[Stage->ALU_result/8] = core->reg_file[Stage->rs_2];	
	}
	if(Stage->MemRead) { 
		Stage->temp_memory  = core->data_mem[Stage->ALU_result/8];
	}
}

void WB_S(Stage *Stage, Core *core) {
	if (Stage->none == 1) {
		return;
	}
	int64_t temp_data;
	temp_data = Stage->ALU_result;
	
	if(Stage->MemtoReg == 1) {
		temp_data = Stage->temp_memory;
	} else if(Stage->Branch && Stage->RegWrite) {
		temp_data = Stage->PC + 4;
	}
	if (Stage->rd != 0 && Stage->RegWrite) { 
		core->reg_file[Stage->rd] = temp_data;
	}
}

