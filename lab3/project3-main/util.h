/***************************************************************/
/*                                                             */
/*   MIPS-32 Instruction Level Simulator                       */
/*                                                             */
/*   CS311 KAIST                                               */
/*   util.h                                                    */
/*                                                             */
/***************************************************************/

#ifndef _UTIL_H_
#define _UTIL_H_

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define FALSE 0
#define TRUE  1

/* Basic Information */
#define MEM_TEXT_START	0x00400000
#define MEM_TEXT_SIZE	0x00100000
#define MEM_DATA_START	0x10000000
#define MEM_DATA_SIZE	0x00100000
#define MIPS_REGS	32
#define BYTES_PER_WORD	4
#define PIPE_STAGE	5

#define IF_STAGE 	0
#define ID_STAGE	1
#define EX_STAGE	2
#define MEM_STAGE	3
#define WB_STAGE	4

/* You may add pipeline registers that you require */

typedef struct Control_Signal_Struct{
	unsigned char REG_DST;
	unsigned char I_type;  //if instruction is I_type then 1.
	unsigned char ALU_OP;
	unsigned char ALU_SRC;
	unsigned char BRCH;
	unsigned char MEM_READ;
	unsigned char MEM_WRITE;
	unsigned char REG_WRITE;
	unsigned char MEM_REG;
} Control;

/*
R-type
OP: 0x0 Add
	0x1 Sub
	0x2 And
	0x3 OR
	0x4 NOR
	0x5 SLT
	0x6 SLL
	0x7 SRL
	0x8 LUI
	and I_type = 1
	-> unsinged extension
	0x10 Add
	0x15 SLT
	and I_type = 1
	-> singed extension

BRCH 	0 no branch
		1 Beq
		2 Bne
*/

typedef struct CPU_State_Struct {
	uint32_t PC;			/* program counter for the IF stage*/
	//pc 넘어갔을 때 복구 pc
	uint32_t REGS[MIPS_REGS];	/* register file */
	uint32_t REGS_LOCK[MIPS_REGS];	/* register lock to support stalls 
					   Lock registers when data is not ready*/
	
	uint32_t PIPE[PIPE_STAGE];	/* PC being executed at each stage*/
	uint32_t PIPE_STALL[PIPE_STAGE];
	
	unsigned char FETCH_STALL;

	//IF_ID_latch
	uint32_t IF_ID_INST;
	uint32_t IF_ID_NPC;
	uint32_t IF_FINAL_PC;
	unsigned char IF_JUMP;
	unsigned char IF_BRCH;
	unsigned char IF_ID_STALL;
	unsigned char IF_ID_RS;
	unsigned char IF_ID_RT;
	unsigned char IF_ID_RD;
	

	//ID_EX_latch
	uint32_t ID_EX_NPC;
	uint32_t ID_EX_REG1;
	uint32_t ID_EX_REG2;
	short ID_EX_IMM;
	unsigned char ID_EX_DEST;
	unsigned char ID_JUMP;
	unsigned char ID_EX_STALL;
	unsigned char ID_EX_RS;
	unsigned char ID_EX_RT;
	unsigned char ID_EX_RD;

	//EX_MEM_latch
	uint32_t EX_MEM_NPC;
	uint32_t EX_MEM_ALU_OUT;
	uint32_t EX_MEM_W_VALUE;
	uint32_t EX_MEM_BR_TARGET;
	uint32_t EX_MEM_BR_TAKE;
	unsigned char EX_MEM_DEST;
	unsigned char EX_MEM_STALL;
	unsigned char EX_MEM_RS;
	unsigned char EX_MEM_RT;
	unsigned char EX_MEM_RD;
	unsigned char EX_LOAD_USE;

	//MEM_WB_latch
	uint32_t MEM_WB_NPC;
	uint32_t MEM_WB_ALU_OUT;
	uint32_t MEM_WB_MEM_OUT;
	uint32_t MEM_WB_BR_TAKE;
	unsigned char MEM_WB_DEST;
	unsigned char MEM_WB_STALL;
	unsigned char MEM_WB_RS;
	unsigned char MEM_WB_RT;
	unsigned char MEM_WB_RD;

	//Forwarding
	unsigned char EX_MEM_FORWARD_REG;
	unsigned char MEM_WB_FORWARD_REG;
	unsigned char LO_ST_FORWARD_REG;
	uint32_t EX_MEM_FORWARD_VALUE;
	uint32_t MEM_WB_FORWARD_VALUE;
	uint32_t LO_ST_FORWARD_VALUE;
	unsigned char EX_MEM_FORWARD_RS;
	unsigned char EX_MEM_FORWARD_RT;
	unsigned char MEM_WB_FORWARD_RS;
	unsigned char MEM_WB_FORWARD_RT;
	unsigned char LO_ST_FORWARD;

	//To choose right PC
	uint32_t IF_PC;
	uint32_t JUMP_PC;
	uint32_t BRANCH_PC;

	//Control Signal
	Control ID_EX_CONTROL;
	Control EX_MEM_CONTROL;
	Control MEM_WB_CONTROL;
	// make control signal at ID_STAGE.
	// set signal after execute. So this signal is for next cycle.

	unsigned char IF_FLUSH;
	unsigned char ID_FLUSH;
	unsigned char EX_FLUSH;
	unsigned char MEM_FLUSH;
	unsigned char WB_FLUSH;

	uint32_t inst_num;

} CPU_State;

typedef struct inst_s {
    short opcode;
    
    /*R-type*/
    short func_code;

    union {
        /* R-type or I-type: */
        struct {
	    unsigned char rs;
	    unsigned char rt;

	    union {
	        short imm;

	        struct {
		    unsigned char rd;
		    unsigned char shamt;
		} r;
	    } r_i;
	} r_i;
        /* J-type: */
        uint32_t target;
    } r_t;

    uint32_t value;
    
    //int32 encoding;
    //imm_expr *expr;
    //char *source_line;
} instruction;


typedef struct {
    uint32_t start, size;
    uint8_t *mem;
} mem_region_t;

/* For PC * Registers */
extern CPU_State CURRENT_STATE;

/* For Instructions */
extern instruction *INST_INFO;
extern int NUM_INST;

/* For Memory Regions */
extern mem_region_t MEM_REGIONS[2];

/* For Execution */
extern int RUN_BIT;	/* run bit */
extern int FETCH_BIT;	/* instruction fetch bit */
extern int INSTRUCTION_COUNT;

extern int BR_BIT;	/* Branch predictor enabled */
extern int FORWARDING_BIT;
extern uint64_t MAX_INSTRUCTION_NUM;
extern uint64_t CYCLE_COUNT;

/* Functions */
char**		str_split(char *a_str, const char a_delim);
int		fromBinary(const char *s);
uint32_t	mem_read_32(uint32_t address);
void		mem_write_32(uint32_t address, uint32_t value);
void		cycle();
void		run();
void		go();
void		mdump(int start, int stop);
void		rdump();
void		pdump();
void		init_memory();
void		init_inst_info();

/* YOU IMPLEMENT THIS FUNCTION in the run.c file */
void	process_instruction();

#endif
