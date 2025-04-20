/***************************************************************/
/*                                                             */
/*   MIPS-32 Instruction Level Simulator                       */
/*                                                             */
/*   CS311 KAIST                                               */
/*   parse.c                                                   */
/*                                                             */
/***************************************************************/

#include <stdio.h>

#include "util.h"
#include "parse.h"

#define op_mask 0xFC000000
#define op_sft 26
#define func_mask 0x0000003F
#define rs_mask 0x03E00000
#define rs_sft 21
#define rt_mask 0x001F0000
#define rt_sft 16
#define rd_mask 0x0000F800
#define rd_sft 11
#define sh_mask 0x000007C0
#define sh_sft 6
#define imm_mask 0x0000FFFF
#define j_mask 0x03FFFFFF

int text_size;
int data_size;

instruction parsing_instr(const char *buffer, const int index)
{
    instruction instr;
	/** Implement this function */
	int l = strlen(buffer);
	char* inst = (char*)malloc(l+1);
	strncpy(inst, buffer, 33);
	unsigned inst_int = fromBinary(inst);
	instr.value = (uint32_t)inst_int;

	unsigned char opcode = (op_mask & inst_int)>>op_sft;
	instr.opcode = (short)opcode;
	if(opcode == 0){	//R format
		unsigned char rs = (rs_mask & inst_int)>>rs_sft;
		unsigned char rt = (rt_mask & inst_int)>>rt_sft;
		unsigned char rd = (rd_mask & inst_int)>>rd_sft;
		int  shamt = (sh_mask & inst_int)>>sh_sft;
		short func = (func_mask & inst_int);

			
		instr.r_t.r_i.rs = rs;
		instr.r_t.r_i.rt = rt;
		instr.r_t.r_i.r_i.r.rd = rd;
		instr.r_t.r_i.r_i.r.shamt = shamt;
		instr.func_code = (short)func;
	}
	else if(opcode == 2 || opcode == 3){		//J format
		int J = (j_mask & inst_int);

		instr.r_t.target = (uint32_t)J;
	}
	else{		//I format
		unsigned char rs = (rs_mask & inst_int)>>rs_sft;
		unsigned char rt = (rt_mask & inst_int)>>rt_sft;
		int imm = (0xFFFF & inst_int);

		instr.r_t.r_i.rs = rs;
		instr.r_t.r_i.rt = rt;
		instr.r_t.r_i.r_i.imm = (short)imm;
	}
    return instr;
}

void parsing_data(const char *buffer, const int index)
{
	/** Implement this function */
	int l = strlen(buffer);
	char* inst = (char*)malloc(l+1);
	strncpy(inst, buffer, 33);
	int inst_int = fromBinary(inst);

	mem_write_32((uint32_t)0x10000000+index, inst_int);
}

void print_parse_result()
{
    int i;
    printf("Instruction Information\n");

    for(i = 0; i < text_size/4; i++)
    {
	printf("INST_INFO[%d].value : %x\n",i, INST_INFO[i].value);
	printf("INST_INFO[%d].opcode : %d\n",i, INST_INFO[i].opcode);

	switch(INST_INFO[i].opcode)
	{
	    //I format
	    case 0x9:		//ADDIU
	    case 0xc:		//ANDI
	    case 0xf:		//LUI	
	    case 0xd:		//ORI
	    case 0xb:		//SLTIU
	    case 0x23:		//LW	
	    case 0x2b:		//SW
	    case 0x4:		//BEQ
	    case 0x5:		//BNE
		printf("INST_INFO[%d].rs : %d\n",i, INST_INFO[i].r_t.r_i.rs);
		printf("INST_INFO[%d].rt : %d\n",i, INST_INFO[i].r_t.r_i.rt);
		printf("INST_INFO[%d].imm : %d\n",i, INST_INFO[i].r_t.r_i.r_i.imm);
		break;

    	    //R format
	    case 0x0:		//ADDU, AND, NOR, OR, SLTU, SLL, SRL, SUBU if JR
		printf("INST_INFO[%d].func_code : %d\n",i, INST_INFO[i].func_code);
		printf("INST_INFO[%d].rs : %d\n",i, INST_INFO[i].r_t.r_i.rs);
		printf("INST_INFO[%d].rt : %d\n",i, INST_INFO[i].r_t.r_i.rt);
		printf("INST_INFO[%d].rd : %d\n",i, INST_INFO[i].r_t.r_i.r_i.r.rd);
		printf("INST_INFO[%d].shamt : %d\n",i, INST_INFO[i].r_t.r_i.r_i.r.shamt);
		break;

    	    //J format
	    case 0x2:		//J
	    case 0x3:		//JAL
		printf("INST_INFO[%d].target : %d\n",i, INST_INFO[i].r_t.target);
		break;

	    default:
		printf("Not available instruction\n");
		assert(0);
	}
    }

    printf("Memory Dump - Text Segment\n");
    for(i = 0; i < text_size; i+=4)
	printf("text_seg[%d] : %x\n", i, mem_read_32(MEM_TEXT_START + i));
    for(i = 0; i < data_size; i+=4)
	printf("data_seg[%d] : %x\n", i, mem_read_32(MEM_DATA_START + i));
    printf("Current PC: %x\n", CURRENT_STATE.PC);
}
