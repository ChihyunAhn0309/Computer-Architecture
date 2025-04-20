/***************************************************************/
/*                                                             */
/*   MIPS-32 Instruction Level Simulator                       */
/*                                                             */
/*   CS311 KAIST                                               */
/*   run.c                                                     */
/*                                                             */
/***************************************************************/

#include <stdio.h>

#include "util.h"
#include "run.h"

#define uppc_mask 0xF0000000

uint32_t SEI(short imm){
    return ((int)((imm)<<16))>>16;
}
uint32_t ZEI(short imm){
    return (((unsigned)imm)<<16)>>16;
}
int BA(short imm){
    return ((int)((imm)<<16))>>14;
}
uint32_t JA(uint32_t target){
    return ((CURRENT_STATE.PC + 4) & uppc_mask) + (target<<2);
}
uint32_t OCU(int i){
    return (unsigned)i;
}
void inc_pc(){
    CURRENT_STATE.PC = CURRENT_STATE.PC + 4;
}

int find_end = 1;

uint32_t bf_jump_pc = MEM_TEXT_START;

/***************************************************************/
/*                                                             */
/* Procedure: get_inst_info                                    */
/*                                                             */
/* Purpose: Read insturction information                       */
/*                                                             */
/***************************************************************/
instruction* get_inst_info(uint32_t pc) 
{ 
    return &INST_INFO[(pc - MEM_TEXT_START) >> 2];
}

/***************************************************************/
/*                                                             */
/* Procedure: process_instruction                              */
/*                                                             */
/* Purpose: Process one instrction                             */
/*                                                             */
/***************************************************************/
void process_instruction(){
	/** Implement this function */
    instruction* ci = get_inst_info(CURRENT_STATE.PC);
    if(CURRENT_STATE.PC <= MEM_TEXT_START + (NUM_INST-1)*4){
        if(ci->opcode == 0x0){
            uint32_t rs = CURRENT_STATE.REGS[ci->r_t.r_i.rs];
            uint32_t rt = CURRENT_STATE.REGS[ci->r_t.r_i.rt];
            uint32_t* rd = &CURRENT_STATE.REGS[ci->r_t.r_i.r_i.r.rd];
            uint32_t shamt = ci->r_t.r_i.r_i.r.shamt;
            switch(ci->func_code){
            case 0x21:
                *rd = rs + rt;
                inc_pc();
                break;
            case 0x24:
                *rd = rs & rt;
                inc_pc();
                break;
            case 0x8:
                bf_jump_pc = CURRENT_STATE.PC;
                CURRENT_STATE.PC = rs;
                if(CURRENT_STATE.PC > MEM_TEXT_START + (NUM_INST-1)*4){
                    CURRENT_STATE.PC = bf_jump_pc;
                    RUN_BIT = 0;
                }
                break;
            case 0x27:
                *rd = ~(rs | rt);
                inc_pc();
                break;
            case 0x25:
                *rd = (rs | rt);
                inc_pc();
                break;
            case 0x2b:
                *rd = (rs < rt) ? 1 : 0;
                inc_pc();
                break;
            case 0x00:
                *rd = rt << shamt;
                inc_pc();
                break;
            case 0x02:
                *rd = rt >> shamt;
                inc_pc();
                break;
            case 0x23:
                *rd = rs - rt;
                inc_pc();
                break;
            default:
                break;
            }
        }
        else if(ci->opcode == 0x2){
            bf_jump_pc = CURRENT_STATE.PC;
            CURRENT_STATE.PC = JA(ci->r_t.target);
            if(CURRENT_STATE.PC > MEM_TEXT_START + (NUM_INST-1)*4){
                CURRENT_STATE.PC = bf_jump_pc;
                RUN_BIT = 0;
            }
        }
        else if(ci->opcode == 0x3){
            bf_jump_pc = CURRENT_STATE.PC;
            CURRENT_STATE.REGS[31] = (CURRENT_STATE.PC + 4);
            CURRENT_STATE.PC = JA(ci->r_t.target);
            if(CURRENT_STATE.PC > MEM_TEXT_START + (NUM_INST-1)*4){
                CURRENT_STATE.PC = bf_jump_pc;
                RUN_BIT = 0;
            }
        }
        else{
            uint32_t rs = CURRENT_STATE.REGS[ci->r_t.r_i.rs];
            uint32_t* rt = &CURRENT_STATE.REGS[ci->r_t.r_i.rt];
            short imm = ci->r_t.r_i.r_i.imm;
            switch(ci->opcode){
            case 0x9:
                *rt = rs + (int)SEI(imm);
                inc_pc();
                break;
            case 0xc:
                *rt = rs & ZEI(imm);
                inc_pc();
                break;
            case 0x4:
                if(*rt == rs){
                    bf_jump_pc = CURRENT_STATE.PC;
                    CURRENT_STATE.PC = CURRENT_STATE.PC + 4 + BA(imm);
                    if(CURRENT_STATE.PC > MEM_TEXT_START + (NUM_INST-1)*4){
                        CURRENT_STATE.PC = bf_jump_pc;
                        RUN_BIT = 0;
                    }
                }
                else{
                    inc_pc();
                }
                break;
            case 0x5:
                if(*rt != rs){
                    bf_jump_pc = CURRENT_STATE.PC;
                    CURRENT_STATE.PC = CURRENT_STATE.PC + 4 + BA(imm);
                    if(CURRENT_STATE.PC > MEM_TEXT_START + (NUM_INST-1)*4){
                        CURRENT_STATE.PC = bf_jump_pc;
                        RUN_BIT = 0;
                    }
                }
                else{
                    inc_pc();
                }
                break;
            case 0xf:
                *rt = ((unsigned)imm)<<16;
                inc_pc();
                break;
            case 0x23:
                *rt = mem_read_32(rs + SEI(imm));
                inc_pc();
                break;
            case 0xd:
                *rt = (rs|ZEI(imm));
                inc_pc();
                break;
            case 0xb:
                *rt = (rs < SEI(imm)) ? 1 : 0;
                inc_pc();
                break;
            case 0x2b:
                mem_write_32(rs + SEI(imm), *rt);
                inc_pc();
                break;
            default:
                break;
            }
        }
    }
    else{
        RUN_BIT = 0;
    }
}
