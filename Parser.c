#include "Parser.h"

// FIXME, implement this function.
// Last edit : Feb 05 17:05:00
// Author: Quoc Thinh Vo
// Here shows an example on how to translate "add x10, x10, x25"
void loadInstructions(Instruction_Memory *i_mem, const char *trace)
{
    printf("Loading trace file: %s\n", trace);

    FILE *fd = fopen(trace, "r");
    if (fd == NULL)
    {
        perror("Cannot open trace file. \n");
        exit(EXIT_FAILURE);
    }

    // Iterate all the assembly instructions
    char *line = NULL;
    size_t len = 0;
    ssize_t read;

    Addr PC = 0; // program counter points to the zeroth location initially.
    int IMEM_index = 0;
    while ((read = getline(&line, &len, fd)) != -1)
    {
        // Assign program counter
        i_mem->instructions[IMEM_index].addr = PC;

        // Extract operation
        char *raw_instr = strtok(line, " ");
        if (strcmp(raw_instr, "add") == 0 ||
            strcmp(raw_instr, "sub") == 0 ||
            strcmp(raw_instr, "sll") == 0 ||
            strcmp(raw_instr, "srl") == 0 ||
            strcmp(raw_instr, "xor") == 0 ||
            strcmp(raw_instr, "or")  == 0 ||
			strcmp(raw_instr, "and") == 0 ||        
			strcmp(raw_instr, "sd")  == 0  )  // R 
        {
            parseRType(raw_instr, &(i_mem->instructions[IMEM_index]));
            i_mem->last = &(i_mem->instructions[IMEM_index]);
		}  
        else if 
            (strcmp(raw_instr, "ld") == 0 ||
            strcmp(raw_instr, "addi") == 0 ||
            strcmp(raw_instr, "slli") == 0 )   // I 
        { 
            parseIType(raw_instr, &(i_mem->instructions[IMEM_index]));
            i_mem->last = &(i_mem->instructions[IMEM_index]);
        }  else if
         (strcmp(raw_instr, "bne") == 0)    // SB 
        { 
            parseSBType(raw_instr, &(i_mem->instructions[IMEM_index]));
            i_mem->last = &(i_mem->instructions[IMEM_index]);
        }

        IMEM_index++;
        PC += 4;
    }

    fclose(fd);
}

void parseRType(char *opr, Instruction *instr)
{
    instr->instruction = 0;
    unsigned opcode = 0;
    unsigned funct3 = 0;
    unsigned funct7 = 0;
	int pattern = 0;
	unsigned rd, rs_1, rs_2;
    if (strcmp(opr, "add") == 0)
    {
        opcode = 51;
        funct3 = 0;
        funct7 = 0;
    } else if (strcmp(opr, "sd") == 0) {
        opcode = 35;
        funct3 = 3;
        funct7 = 0;
        pattern = 1;
    } else if (strcmp(opr, "sll") == 0) {
        opcode = 51;
        funct3 = 1;
        funct7 = 0;
	} else if (strcmp(opr, "sub") == 0)
    {
        opcode = 51;
        funct3 = 0;
        funct7 = 40;
    } else if (strcmp(opr, "or") == 0)
    {
        opcode = 51;
        funct3 = 6;
        funct7 = 0;
    } else if (strcmp(opr, "and") == 0)
    {
        opcode = 51;
        funct3 = 7;
        funct7 = 0;
    }



	if (pattern == 0 ){
		char *reg = strtok(NULL, ", ");
		rd = regIndex(reg);

		reg = strtok(NULL, ", ");
		rs_1 = regIndex(reg);

		reg = strtok(NULL, ", ");
		reg[strlen(reg)-1] = '\0';
		rs_2 = regIndex(reg);
	} else {
		char *reg = strtok(NULL, ", ");
        rs_2 = regIndex(reg);

        reg = strtok(NULL,"(");
        int imm = atoi(reg);
        rd = imm & 0b000000011111; 
        funct7 = (imm & 0b111111100000) >> 5;   

        reg = strtok(NULL, ")");
        rs_1 = regIndex(reg);
	}
    // Contruct instruction
    instr->instruction |= opcode;
    instr->instruction |= (rd << 7);
    instr->instruction |= (funct3 << (7 + 5));
    instr->instruction |= (rs_1 << (7 + 5 + 3));
    instr->instruction |= (rs_2 << (7 + 5 + 3 + 5));
    instr->instruction |= (funct7 << (7 + 5 + 3 + 5 + 5));
}

void parseIType(char *opr, Instruction *instr ){
    
	instr->instruction = 0;
	int imm = 0;
    unsigned opcode = 0; 
    unsigned funct3 = 0;
    unsigned funct7 = 99999999; 
    unsigned rd, rs_1;
	int pattern; 

    if (strcmp(opr, "ld") == 0) {
        opcode = 3;
        funct3 = 0b011;      
        pattern = 1;
		
    } else if (strcmp(opr, "addi") == 0) {
        opcode = 19;
        funct3 = 0b000;       
        pattern = 0;
		
    } else if (strcmp(opr, "slli") == 0) {
        opcode = 19;
        funct3 = 0b001;
        funct7 = 0b0000000;
        pattern  = 0;
    } 
 
    if (pattern == 0) {  
        char *reg = strtok(NULL, ", ");
        rd = regIndex(reg);

        reg = strtok(NULL, ", ");
        rs_1 = regIndex(reg);

        reg = strtok(NULL, ", ");
        if (funct7 == 99999999){
            imm = atoi(reg);
        } else {
            funct7 = funct7 << 5;
            imm = atoi(reg)| funct7; 
        }
            
    } else if (pattern == 1){ 
        char *reg = strtok(NULL, ", ");
        rd = regIndex(reg);

        reg = strtok(NULL,"(");
        imm = atoi(reg)^0xFFFFF000; 

        reg = strtok(NULL, ")");
        rs_1 = regIndex(reg);
    }
    // Contruct instruction
    instr->instruction |= opcode;
    instr->instruction |= (rd << 7);
    instr->instruction |= (funct3 << (7 + 5));
    instr->instruction |= (rs_1 << (7 + 5 + 3));
    instr->instruction |= (imm  << (7 + 5 + 3 + 5));
}

void parseSBType(char *opr, Instruction *instr){
	
    instr->instruction = 0;
    unsigned opcode = 0b1100011;
    unsigned funct3=0; 
    unsigned allocate_OR_1, allocate_OR_2, imm_12, imm_11, imm_10_to_5, imm_4_to_1;
	unsigned rs_1, rs_2;
	int imm = 0;

    if (strcmp(opr, "bne") == 0) {
        funct3 = 0b001;
    }

    char *reg = strtok(NULL, ", ");
    rs_1 = regIndex(reg);

    reg = strtok(NULL, ", ");
    rs_2 = regIndex(reg);

    reg = strtok(NULL, ", ");
	
    imm = atoi(reg);
	
	imm_4_to_1 = (imm >> 1) & 0b01111;
	imm_11 = (imm >> 11);
    imm_11 = imm_11 & 0b01;
	allocate_OR_1 = (imm_4_to_1 << 1) | imm_11;
	
	imm_10_to_5 = (imm >> 5) & 0b0111111;
    imm_12 = (imm >> 12);
    imm_12 = imm_12 & 0b01; 
    allocate_OR_2 = (imm_12 << 6) | imm_10_to_5;

    instr->instruction |= opcode;
    instr->instruction |= (allocate_OR_1 << 7);
    instr->instruction |= (funct3 << (7 + 5));
    instr->instruction |= (rs_1 << (7 + 5 + 3));
    instr->instruction |= (rs_2  << (7 + 5 + 3 + 5));
    instr->instruction |= (allocate_OR_2  << (7 + 5 + 3 + 5 + 5));  
 
}

int regIndex(char *reg)
{
    unsigned i = 0;
    for (i; i < NUM_OF_REGS; i++)
    {
        if (strcmp(REGISTER_NAME[i], reg) == 0)
        {
            break;
        }
    }

    return i;
}


// *** end ***