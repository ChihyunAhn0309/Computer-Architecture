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

void is_terminate();
void forward_check();

/***************************************************************/
/*                                                             */
/* Procedure: get_inst_info                                    */
/*                                                             */
/* Purpose: Read insturction information                       */
/*                                                             */
/***************************************************************/
instruction* get_inst_info(uint32_t pc) { 
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
	/** Your implementation here */
    if(CYCLE_COUNT == 0){
        CURRENT_STATE.inst_num = 0;
    }
    if(FORWARDING_BIT == TRUE){
        forward_check();
    }
    if(CYCLE_COUNT >= 4){
        WB_Stage();
    }
    if(CYCLE_COUNT >= 3){
        MEM_Stage();
    }
    if(CYCLE_COUNT >= 2){
        EX_Stage();
    }
    if(CYCLE_COUNT >= 1){
        ID_Stage();
    }
    IF_Stage();
    if(FETCH_BIT == FALSE){
        is_terminate();
    }
    if(CURRENT_STATE.inst_num >= MAX_INSTRUCTION_NUM){
        RUN_BIT = 0;
    }
    //FETCH_BIT가 False가 되면 하나씩 flush 시키기
    // pc증가 시키긴 해야 함함
    // 실행은 역순이 되어야 할 것 같다.
}


//-------------------------------------------------------------------------
void is_valid_PC(){
    if(CURRENT_STATE.PC >= MEM_TEXT_START + (NUM_INST)*4 || CURRENT_STATE.PC < MEM_TEXT_START){
        FETCH_BIT = FALSE; // 우선 run_bit를 false로 했는데 다음에는 FETCH_BIT로 해야한다. flush 먼저 구현.
    }
}

void is_terminate(){
    if(CURRENT_STATE.PIPE[0] == 0x0 && CURRENT_STATE.PIPE[1] == 0x0 
    && CURRENT_STATE.PIPE[2] == 0x0 && CURRENT_STATE.PIPE[3] == 0x0){
        RUN_BIT = FALSE;
    }
}

void forward_check(){
    if(CYCLE_COUNT >= 2){
        //forward 신호를 두개로 나눠야 할듯. rs용과 rt용. 왜냐하면 rs를 하고 rt도 겹치면 이게 다시 덮혀씌여짐.
        if(CURRENT_STATE.EX_MEM_CONTROL.MEM_READ && !CURRENT_STATE.ID_EX_CONTROL.MEM_WRITE &&
        (CURRENT_STATE.EX_MEM_RT == CURRENT_STATE.ID_EX_RS || CURRENT_STATE.EX_MEM_RT == CURRENT_STATE.ID_EX_RT)){
            CURRENT_STATE.FETCH_STALL = 1;
            CURRENT_STATE.IF_ID_STALL = 1;
            CURRENT_STATE.EX_FLUSH = 1;
        }
        else{
            if(CURRENT_STATE.EX_MEM_CONTROL.REG_WRITE &&
            CURRENT_STATE.EX_MEM_RD != 0 && CURRENT_STATE.EX_MEM_RD == CURRENT_STATE.ID_EX_RS &&
            !CURRENT_STATE.EX_MEM_CONTROL.I_type){
                CURRENT_STATE.EX_MEM_FORWARD_RS = 0x1;
                CURRENT_STATE.EX_MEM_FORWARD_VALUE = CURRENT_STATE.EX_MEM_ALU_OUT;
            }
            else if(CURRENT_STATE.EX_MEM_CONTROL.REG_WRITE &&
            CURRENT_STATE.EX_MEM_RT != 0 && CURRENT_STATE.EX_MEM_RT == CURRENT_STATE.ID_EX_RS &&
            CURRENT_STATE.EX_MEM_CONTROL.I_type){
                CURRENT_STATE.EX_MEM_FORWARD_RS = 0x1;
                CURRENT_STATE.EX_MEM_FORWARD_VALUE = CURRENT_STATE.EX_MEM_ALU_OUT;
            }
            if(CURRENT_STATE.EX_MEM_CONTROL.REG_WRITE &&
            CURRENT_STATE.EX_MEM_RD != 0 && CURRENT_STATE.EX_MEM_RD == CURRENT_STATE.ID_EX_RT &&
            !CURRENT_STATE.EX_MEM_CONTROL.I_type){
                CURRENT_STATE.EX_MEM_FORWARD_RT = 0x1;
                CURRENT_STATE.EX_MEM_FORWARD_VALUE = CURRENT_STATE.EX_MEM_ALU_OUT;
            }
            else if(CURRENT_STATE.EX_MEM_CONTROL.REG_WRITE &&
            CURRENT_STATE.EX_MEM_RT != 0 && CURRENT_STATE.EX_MEM_RT == CURRENT_STATE.ID_EX_RT &&
            CURRENT_STATE.EX_MEM_CONTROL.I_type){
                CURRENT_STATE.EX_MEM_FORWARD_RT = 0x1;
                CURRENT_STATE.EX_MEM_FORWARD_VALUE = CURRENT_STATE.EX_MEM_ALU_OUT;
            }
        }
        if(CYCLE_COUNT >= 3){
            if(CURRENT_STATE.MEM_WB_CONTROL.REG_WRITE && 
            CURRENT_STATE.MEM_WB_RD != 0 && CURRENT_STATE.EX_MEM_RD != CURRENT_STATE.ID_EX_RS &&
            CURRENT_STATE.MEM_WB_RD == CURRENT_STATE.ID_EX_RS &&
            !CURRENT_STATE.MEM_WB_CONTROL.I_type){
                CURRENT_STATE.MEM_WB_FORWARD_RS = 0x1;
                CURRENT_STATE.MEM_WB_FORWARD_VALUE = CURRENT_STATE.MEM_WB_ALU_OUT;
            }
            else if(CURRENT_STATE.MEM_WB_CONTROL.REG_WRITE && CURRENT_STATE.MEM_WB_CONTROL.MEM_READ &&
            CURRENT_STATE.MEM_WB_RT != 0 && CURRENT_STATE.EX_MEM_RT != CURRENT_STATE.ID_EX_RS &&
            CURRENT_STATE.MEM_WB_RT == CURRENT_STATE.ID_EX_RS &&
            CURRENT_STATE.MEM_WB_CONTROL.I_type){
                CURRENT_STATE.MEM_WB_FORWARD_RS = 0x1;
                CURRENT_STATE.MEM_WB_FORWARD_VALUE = CURRENT_STATE.MEM_WB_MEM_OUT;
            }
            else if(CURRENT_STATE.MEM_WB_CONTROL.REG_WRITE && 
            CURRENT_STATE.MEM_WB_RT != 0 && CURRENT_STATE.EX_MEM_RT != CURRENT_STATE.ID_EX_RS &&
            CURRENT_STATE.MEM_WB_RT == CURRENT_STATE.ID_EX_RS &&
            CURRENT_STATE.MEM_WB_CONTROL.I_type){
                CURRENT_STATE.MEM_WB_FORWARD_RS = 0x1;
                CURRENT_STATE.MEM_WB_FORWARD_VALUE = CURRENT_STATE.MEM_WB_ALU_OUT;
            }
            if(CURRENT_STATE.MEM_WB_CONTROL.REG_WRITE &&
            CURRENT_STATE.MEM_WB_RD != 0 && CURRENT_STATE.EX_MEM_RD != CURRENT_STATE.ID_EX_RT &&
            CURRENT_STATE.MEM_WB_RD == CURRENT_STATE.ID_EX_RT && 
            !CURRENT_STATE.MEM_WB_CONTROL.I_type){
                CURRENT_STATE.MEM_WB_FORWARD_RT = 0x1;
                CURRENT_STATE.MEM_WB_FORWARD_VALUE = CURRENT_STATE.MEM_WB_ALU_OUT;
            }
            else if(CURRENT_STATE.MEM_WB_CONTROL.REG_WRITE && CURRENT_STATE.MEM_WB_CONTROL.MEM_READ &&
            CURRENT_STATE.MEM_WB_RT != 0 && CURRENT_STATE.EX_MEM_RT != CURRENT_STATE.ID_EX_RT &&
            CURRENT_STATE.MEM_WB_RT == CURRENT_STATE.ID_EX_RT &&
            CURRENT_STATE.MEM_WB_CONTROL.I_type){
                CURRENT_STATE.MEM_WB_FORWARD_RT = 0x1;
                CURRENT_STATE.MEM_WB_FORWARD_VALUE = CURRENT_STATE.MEM_WB_MEM_OUT;
            }
            else if(CURRENT_STATE.MEM_WB_CONTROL.REG_WRITE && 
            CURRENT_STATE.MEM_WB_RT != 0 && CURRENT_STATE.EX_MEM_RT != CURRENT_STATE.ID_EX_RT &&
            CURRENT_STATE.MEM_WB_RT == CURRENT_STATE.ID_EX_RT &&
            CURRENT_STATE.MEM_WB_CONTROL.I_type){
                CURRENT_STATE.MEM_WB_FORWARD_RT = 0x1;
                CURRENT_STATE.MEM_WB_FORWARD_VALUE = CURRENT_STATE.MEM_WB_ALU_OUT;
            }
            if(CURRENT_STATE.MEM_WB_CONTROL.MEM_READ && 
            CURRENT_STATE.EX_MEM_CONTROL.MEM_WRITE &&
            CURRENT_STATE.MEM_WB_RT != 0 && CURRENT_STATE.MEM_WB_RT == CURRENT_STATE.EX_MEM_RT){
                CURRENT_STATE.LO_ST_FORWARD = 0x1;
                CURRENT_STATE.LO_ST_FORWARD_VALUE = CURRENT_STATE.MEM_WB_MEM_OUT;
            }
        }
    }
}

void sig_cpy_EX_MEM(){
    CURRENT_STATE.EX_MEM_CONTROL.REG_DST = 
    CURRENT_STATE.ID_EX_CONTROL.REG_DST;
    CURRENT_STATE.EX_MEM_CONTROL.BRCH = 
    CURRENT_STATE.ID_EX_CONTROL.BRCH;
    CURRENT_STATE.EX_MEM_CONTROL.MEM_READ = 
    CURRENT_STATE.ID_EX_CONTROL.MEM_READ;
    CURRENT_STATE.EX_MEM_CONTROL.MEM_WRITE = 
    CURRENT_STATE.ID_EX_CONTROL.MEM_WRITE;
    CURRENT_STATE.EX_MEM_CONTROL.REG_WRITE = 
    CURRENT_STATE.ID_EX_CONTROL.REG_WRITE;
    CURRENT_STATE.EX_MEM_CONTROL.MEM_REG = 
    CURRENT_STATE.ID_EX_CONTROL.MEM_REG;
    CURRENT_STATE.EX_MEM_CONTROL.I_type = 
    CURRENT_STATE.ID_EX_CONTROL.I_type;
}

void sig_cpy_MEM_WB(){
    CURRENT_STATE.MEM_WB_CONTROL.REG_DST = 
    CURRENT_STATE.EX_MEM_CONTROL.REG_DST;
    CURRENT_STATE.MEM_WB_CONTROL.MEM_READ = 
    CURRENT_STATE.EX_MEM_CONTROL.MEM_READ;
    CURRENT_STATE.MEM_WB_CONTROL.REG_WRITE = 
    CURRENT_STATE.EX_MEM_CONTROL.REG_WRITE;
    CURRENT_STATE.MEM_WB_CONTROL.MEM_REG = 
    CURRENT_STATE.EX_MEM_CONTROL.MEM_REG;
    CURRENT_STATE.MEM_WB_CONTROL.I_type = 
    CURRENT_STATE.EX_MEM_CONTROL.I_type;
}

void IF_Stage(){
    if(CURRENT_STATE.FETCH_STALL == 1){
        CURRENT_STATE.FETCH_STALL = 0;
    }
    else{
        if(CURRENT_STATE.IF_JUMP){
            CURRENT_STATE.IF_JUMP = 0;
            CURRENT_STATE.PC = CURRENT_STATE.IF_FINAL_PC;
            is_valid_PC();
        }
        if(CURRENT_STATE.ID_JUMP){
            CURRENT_STATE.ID_JUMP = 0;
            CURRENT_STATE.IF_JUMP = 1;
            CURRENT_STATE.ID_FLUSH = 1;
            CURRENT_STATE.IF_FINAL_PC = CURRENT_STATE.JUMP_PC;
        }
        if(CURRENT_STATE.IF_BRCH){
            CURRENT_STATE.IF_BRCH = 0;
            CURRENT_STATE.PC = CURRENT_STATE.IF_FINAL_PC;
            is_valid_PC();
        }
        if(CURRENT_STATE.MEM_WB_BR_TAKE){
            CURRENT_STATE.MEM_WB_BR_TAKE = 0;
            CURRENT_STATE.IF_BRCH = 1;
            CURRENT_STATE.IF_FINAL_PC = CURRENT_STATE.BRANCH_PC;
    
        }
        if(FETCH_BIT == TRUE){
            uint32_t if_pc = CURRENT_STATE.PC;
            CURRENT_STATE.PC = CURRENT_STATE.PC + 4;
            is_valid_PC();
            instruction* if_inst = get_inst_info(if_pc);
            CURRENT_STATE.IF_ID_RS = RS(if_inst);
            CURRENT_STATE.IF_ID_RT = RT(if_inst);
            CURRENT_STATE.IF_ID_RD = RD(if_inst);
            CURRENT_STATE.PIPE[0] = if_pc;
            CURRENT_STATE.IF_ID_NPC = if_pc;
            CURRENT_STATE.IF_ID_INST = if_inst->value;
        }
        else{
            CURRENT_STATE.PIPE[0] = 0x0;
            CURRENT_STATE.IF_ID_NPC = 0x0;
            CURRENT_STATE.IF_ID_INST = 0x0;
        }
        if(CURRENT_STATE.IF_FLUSH){
            CURRENT_STATE.IF_FLUSH = 0;
            CURRENT_STATE.ID_FLUSH = 1;
            CURRENT_STATE.PIPE[0] = 0x0;
        }
    }
}

void ID_Stage(){
    if(CURRENT_STATE.IF_ID_STALL){
        CURRENT_STATE.IF_ID_STALL = 0;
    }
    else if(CURRENT_STATE.ID_FLUSH){
        CURRENT_STATE.ID_FLUSH = 0;
        CURRENT_STATE.EX_FLUSH = 1;
        CURRENT_STATE.PIPE[1] = 0x0;
    }
    else{
        CURRENT_STATE.PIPE[1] = CURRENT_STATE.PIPE[0];
        uint32_t id_pc = CURRENT_STATE.PIPE[1];
        if(id_pc == 0x0){
        }
        else{
            instruction* id_inst = get_inst_info(id_pc);
            CURRENT_STATE.ID_EX_NPC = id_pc;
            CURRENT_STATE.ID_EX_RS = CURRENT_STATE.IF_ID_RS;
            CURRENT_STATE.ID_EX_RT = CURRENT_STATE.IF_ID_RT;
            CURRENT_STATE.ID_EX_RD = CURRENT_STATE.IF_ID_RD;
            if(OPCODE(id_inst) == 0x0){
                CURRENT_STATE.ID_EX_CONTROL.REG_DST = 1; //rd
                CURRENT_STATE.ID_EX_CONTROL.I_type = 0; //not I type.
                CURRENT_STATE.ID_EX_CONTROL.ALU_SRC = 0; //rt
                CURRENT_STATE.ID_EX_CONTROL.BRCH = 0;
                CURRENT_STATE.ID_EX_CONTROL.MEM_READ = 0;
                CURRENT_STATE.ID_EX_CONTROL.MEM_WRITE = 0;
                CURRENT_STATE.ID_EX_CONTROL.REG_WRITE = 1;
                CURRENT_STATE.ID_EX_CONTROL.MEM_REG = 0;
                CURRENT_STATE.ID_EX_REG1 = RS(id_inst);
                CURRENT_STATE.ID_EX_REG2 = RD(id_inst);
                CURRENT_STATE.ID_EX_DEST = 1;
                switch(FUNC(id_inst)){
                case 0x21:
                    CURRENT_STATE.ID_EX_CONTROL.ALU_OP = 0x00;
                    break;
                case 0x24:
                    CURRENT_STATE.ID_EX_CONTROL.ALU_OP = 0x02;
                    break;
                case 0x08:
                    CURRENT_STATE.JUMP_PC = CURRENT_STATE.REGS[RS(id_inst)];
                    CURRENT_STATE.ID_JUMP = 1;
                    break;
                case 0x27:
                    CURRENT_STATE.ID_EX_CONTROL.ALU_OP = 0x04;
                    break; 
                case 0x25:
                    CURRENT_STATE.ID_EX_CONTROL.ALU_OP = 0x03;
                    break;
                case 0x2b:
                    CURRENT_STATE.ID_EX_CONTROL.ALU_OP = 0x05;
                    break;
                case 0x00:
                    CURRENT_STATE.ID_EX_CONTROL.ALU_OP = 0x06;
                    break;
                case 0x02:
                    CURRENT_STATE.ID_EX_CONTROL.ALU_OP = 0x07;
                    break;
                case 0x23:
                    CURRENT_STATE.ID_EX_CONTROL.ALU_OP = 0x01;
                    break;
                default:
                    break;
                }
            }
            else if(OPCODE(id_inst) == 0x2){
                CURRENT_STATE.ID_EX_CONTROL.REG_DST = 0; //rt
                CURRENT_STATE.ID_EX_CONTROL.I_type = 0; //not I type.
                CURRENT_STATE.ID_EX_CONTROL.ALU_SRC = 0; //imm
                CURRENT_STATE.ID_EX_CONTROL.BRCH = 0;
                CURRENT_STATE.ID_EX_CONTROL.MEM_READ = 0;
                CURRENT_STATE.ID_EX_CONTROL.MEM_WRITE = 0;
                CURRENT_STATE.ID_EX_CONTROL.REG_WRITE = 0;
                CURRENT_STATE.ID_EX_CONTROL.MEM_REG = 0;
                CURRENT_STATE.ID_EX_REG1 = 0;
                CURRENT_STATE.ID_EX_REG2 = 0;
                CURRENT_STATE.ID_EX_DEST = 0;
                CURRENT_STATE.ID_EX_IMM = IMM(id_inst);
                CURRENT_STATE.JUMP_PC = JUMP_ADDR(id_pc, TARGET(id_inst));
                CURRENT_STATE.ID_JUMP = 1;
            }
            else if(OPCODE(id_inst) == 0x3){
                CURRENT_STATE.ID_EX_CONTROL.REG_DST = 0; //rt
                CURRENT_STATE.ID_EX_CONTROL.I_type = 0; //not I type.
                CURRENT_STATE.ID_EX_CONTROL.ALU_SRC = 0; //imm
                CURRENT_STATE.ID_EX_CONTROL.BRCH = 0;
                CURRENT_STATE.ID_EX_CONTROL.MEM_READ = 0;
                CURRENT_STATE.ID_EX_CONTROL.MEM_WRITE = 0;
                CURRENT_STATE.ID_EX_CONTROL.REG_WRITE = 0;
                CURRENT_STATE.ID_EX_CONTROL.MEM_REG = 0;
                CURRENT_STATE.ID_EX_REG1 = 0;
                CURRENT_STATE.ID_EX_REG2 = 0;
                CURRENT_STATE.ID_EX_DEST = 0;
                CURRENT_STATE.ID_EX_IMM = IMM(id_inst);
                CURRENT_STATE.REGS[31] = id_pc + 4;
                CURRENT_STATE.JUMP_PC = JUMP_ADDR(id_pc, TARGET(id_inst));
                CURRENT_STATE.ID_JUMP = 1;
            }
            else{
                CURRENT_STATE.ID_EX_CONTROL.REG_DST = 0; //rt
                CURRENT_STATE.ID_EX_CONTROL.I_type = 1; //not I type.
                CURRENT_STATE.ID_EX_CONTROL.ALU_SRC = 1; //imm
                CURRENT_STATE.ID_EX_CONTROL.BRCH = 0;
                CURRENT_STATE.ID_EX_CONTROL.MEM_READ = 0;
                CURRENT_STATE.ID_EX_CONTROL.MEM_WRITE = 0;
                CURRENT_STATE.ID_EX_CONTROL.REG_WRITE = 1;
                CURRENT_STATE.ID_EX_CONTROL.MEM_REG = 0;
                CURRENT_STATE.ID_EX_REG1 = RS(id_inst);
                CURRENT_STATE.ID_EX_REG2 = RT(id_inst);
                CURRENT_STATE.ID_EX_DEST = 0;
                CURRENT_STATE.ID_EX_IMM = IMM(id_inst);
                switch(OPCODE(id_inst)){
                case 0x9:
                    CURRENT_STATE.ID_EX_CONTROL.ALU_OP = 0x10;
                    break;
                case 0xc:
                    CURRENT_STATE.ID_EX_CONTROL.ALU_OP = 0x02;
                    break;
                case 0x4:   //beq
                    CURRENT_STATE.ID_EX_CONTROL.ALU_OP = 0x01;
                    CURRENT_STATE.ID_EX_CONTROL.REG_WRITE = 0;
                    CURRENT_STATE.ID_EX_CONTROL.BRCH = 1;
                    break;
                case 0x5:   //bne
                    CURRENT_STATE.ID_EX_CONTROL.ALU_OP = 0x01;
                    CURRENT_STATE.ID_EX_CONTROL.REG_WRITE = 0;
                    CURRENT_STATE.ID_EX_CONTROL.BRCH = 2;
                    break;
                case 0xf:
                    CURRENT_STATE.ID_EX_CONTROL.ALU_OP = 0x08;
                    break;
                case 0x23:
                    CURRENT_STATE.ID_EX_CONTROL.MEM_READ = 1;
                    CURRENT_STATE.ID_EX_CONTROL.MEM_REG = 1;
                    CURRENT_STATE.ID_EX_CONTROL.ALU_OP = 0x10;
                    break;
                case 0xd:
                    CURRENT_STATE.ID_EX_CONTROL.ALU_OP = 0x03;
                    break;
                case 0xb:
                    CURRENT_STATE.ID_EX_CONTROL.ALU_OP = 0x15;
                    break;
                case 0x2b:
                    CURRENT_STATE.ID_EX_CONTROL.MEM_WRITE = 1;
                    CURRENT_STATE.ID_EX_CONTROL.REG_WRITE = 0;
                    CURRENT_STATE.ID_EX_CONTROL.ALU_OP = 0x10;
                    break;
                default:
                    break;
                }
            }
        }
    }
}

void EX_Stage(){
    if(CURRENT_STATE.ID_EX_STALL){
        CURRENT_STATE.ID_EX_STALL = 0;
    }
    else if(CURRENT_STATE.EX_FLUSH){
        CURRENT_STATE.EX_FLUSH = 0;
        CURRENT_STATE.MEM_FLUSH = 1;
        CURRENT_STATE.PIPE[2] = 0x0;
        CURRENT_STATE.EX_MEM_RS = 0; 
        CURRENT_STATE.EX_MEM_RT = 0;
        CURRENT_STATE.EX_MEM_RD = 0;
    }
    else{
        CURRENT_STATE.PIPE[2] = CURRENT_STATE.PIPE[1];
        uint32_t ex_pc = CURRENT_STATE.PIPE[2];
        if(ex_pc == 0x0){
        }
        else{
            instruction* ex_inst = get_inst_info(ex_pc);
            CURRENT_STATE.EX_MEM_RS = CURRENT_STATE.ID_EX_RS;
            CURRENT_STATE.EX_MEM_RT = CURRENT_STATE.ID_EX_RT;
            CURRENT_STATE.EX_MEM_RD = CURRENT_STATE.ID_EX_RD;
            CURRENT_STATE.EX_MEM_BR_TAKE = 0;
            CURRENT_STATE.EX_MEM_DEST = CURRENT_STATE.EX_MEM_CONTROL.REG_DST;
            CURRENT_STATE.EX_MEM_NPC = ex_pc;
            uint32_t rs_selection = CURRENT_STATE.REGS[RS(ex_inst)];
            uint32_t rt_selection = CURRENT_STATE.REGS[RT(ex_inst)];
            if(FORWARDING_BIT == TRUE){
                if(CURRENT_STATE.EX_MEM_FORWARD_RS == 0x1){
                    rs_selection = CURRENT_STATE.EX_MEM_FORWARD_VALUE;
                }
                else if(CURRENT_STATE.MEM_WB_FORWARD_RS == 0x1){
                    rs_selection = CURRENT_STATE.MEM_WB_FORWARD_VALUE;
                }
                if(CURRENT_STATE.EX_MEM_FORWARD_RT == 0x1){
                    rt_selection = CURRENT_STATE.EX_MEM_FORWARD_VALUE;
                }
                else if(CURRENT_STATE.MEM_WB_FORWARD_RT == 0x1){
                    rt_selection = CURRENT_STATE.MEM_WB_FORWARD_VALUE;
                }
                CURRENT_STATE.EX_MEM_FORWARD_RS = 0x0;
                CURRENT_STATE.EX_MEM_FORWARD_RT = 0x0;
                CURRENT_STATE.MEM_WB_FORWARD_RS = 0x0;
                CURRENT_STATE.MEM_WB_FORWARD_RT = 0x0;
            }
            if(OPCODE(ex_inst) == 0x0){
                switch(FUNC(ex_inst)){
                case 0x21:
                    CURRENT_STATE.EX_MEM_ALU_OUT = rs_selection + rt_selection;
                    break;
                case 0x24:
                    CURRENT_STATE.EX_MEM_ALU_OUT = rs_selection & rt_selection;
                    break;
                case 0x27:
                    CURRENT_STATE.EX_MEM_ALU_OUT = ~(rs_selection | rt_selection);
                    break;
                case 0x25:
                    CURRENT_STATE.EX_MEM_ALU_OUT = rs_selection | rt_selection;
                    break;
                case 0x2b:
                    CURRENT_STATE.EX_MEM_ALU_OUT = (rs_selection < rt_selection) ? 1 : 0;
                    break;
                case 0x00:
                    CURRENT_STATE.EX_MEM_ALU_OUT = rt_selection << SHAMT(ex_inst);
                    break;
                case 0x02:
                    CURRENT_STATE.EX_MEM_ALU_OUT = rt_selection >> SHAMT(ex_inst);
                    break;
                case 0x23:
                    CURRENT_STATE.EX_MEM_ALU_OUT = rs_selection - rt_selection;
                    break;
                }
            }
            else{
                switch(OPCODE(ex_inst)){
                case 0x09:
                    CURRENT_STATE.EX_MEM_ALU_OUT = rs_selection + SIGN_EX(IMM(ex_inst));
                    break;
                case 0x0c:
                    CURRENT_STATE.EX_MEM_ALU_OUT = rs_selection & (uint32_t)IMM(ex_inst);
                    break;
                case 0x04:
                    CURRENT_STATE.EX_MEM_ALU_OUT = rs_selection - rt_selection;
                    CURRENT_STATE.EX_MEM_BR_TARGET = ex_pc + 4 + (SIGN_EX(IMM(ex_inst))<<2);
                    if(CURRENT_STATE.EX_MEM_ALU_OUT){
                        CURRENT_STATE.EX_MEM_BR_TAKE = 0;
                    }
                    else{
                        CURRENT_STATE.EX_MEM_BR_TAKE = 1;
                    }
                    break;
                case 0x05:
                    CURRENT_STATE.EX_MEM_ALU_OUT = rs_selection - rt_selection;
                    CURRENT_STATE.EX_MEM_BR_TARGET = ex_pc + 4 + (SIGN_EX(IMM(ex_inst))<<2);
                    if(CURRENT_STATE.EX_MEM_ALU_OUT){
                        CURRENT_STATE.EX_MEM_BR_TAKE = 1;
                    }
                    else{
                        CURRENT_STATE.EX_MEM_BR_TAKE = 0;
                    }
                    break;
                case 0x0f:
                    CURRENT_STATE.EX_MEM_ALU_OUT = ((uint32_t)(IMM(ex_inst))<<16);
                    break;
                case 0x23:
                    CURRENT_STATE.EX_MEM_ALU_OUT = rs_selection + SIGN_EX(IMM(ex_inst));
                    break;
                case 0x0d:
                    CURRENT_STATE.EX_MEM_ALU_OUT = rs_selection | IMM(ex_inst);
                    break;
                case 0x0b:
                    CURRENT_STATE.EX_MEM_ALU_OUT = rs_selection < SIGN_EX(IMM(ex_inst)) ? 1 : 0;
                    break;
                case 0x2b:
                    CURRENT_STATE.EX_MEM_ALU_OUT = rs_selection + SIGN_EX(IMM(ex_inst));
                    break;
                }
            }
        }
    }
    sig_cpy_EX_MEM();
}

void MEM_Stage(){
    if(CURRENT_STATE.EX_MEM_STALL){
        CURRENT_STATE.EX_MEM_STALL = 0;
    }
    else if(CURRENT_STATE.MEM_FLUSH){
        CURRENT_STATE.MEM_FLUSH = 0;
        CURRENT_STATE.WB_FLUSH = 1;
        CURRENT_STATE.PIPE[3] = 0x0;
    }
    else{
        CURRENT_STATE.PIPE[3] = CURRENT_STATE.PIPE[2];
        uint32_t mem_pc = CURRENT_STATE.PIPE[3];
        if(mem_pc == 0x0){
        }
        else{
            instruction* mem_inst = get_inst_info(mem_pc);
            CURRENT_STATE.MEM_WB_RS = CURRENT_STATE.EX_MEM_RS;
            CURRENT_STATE.MEM_WB_RT = CURRENT_STATE.EX_MEM_RT;
            CURRENT_STATE.MEM_WB_RD = CURRENT_STATE.EX_MEM_RD;
            CURRENT_STATE.MEM_WB_BR_TAKE = CURRENT_STATE.EX_MEM_BR_TAKE;
            CURRENT_STATE.MEM_WB_DEST = CURRENT_STATE.MEM_WB_CONTROL.REG_DST;
            CURRENT_STATE.MEM_WB_NPC = mem_pc;
            uint32_t rt_selection = CURRENT_STATE.REGS[RT(mem_inst)];
            if(CURRENT_STATE.MEM_WB_BR_TAKE){
                CURRENT_STATE.IF_FLUSH = 1;
                CURRENT_STATE.ID_FLUSH = 1;
                CURRENT_STATE.EX_FLUSH = 1;
                CURRENT_STATE.BRANCH_PC = CURRENT_STATE.EX_MEM_BR_TARGET;
            }
            if(FORWARDING_BIT == TRUE){
                if(CURRENT_STATE.LO_ST_FORWARD){
                    rt_selection = CURRENT_STATE.LO_ST_FORWARD_VALUE;
                }
                CURRENT_STATE.LO_ST_FORWARD = 0x0;
            }
            switch(OPCODE(mem_inst)){
            case 0x23:
                CURRENT_STATE.MEM_WB_ALU_OUT = CURRENT_STATE.EX_MEM_ALU_OUT;
                CURRENT_STATE.MEM_WB_MEM_OUT = mem_read_32(CURRENT_STATE.MEM_WB_ALU_OUT);
                break;
            case 0x2b:
                CURRENT_STATE.MEM_WB_ALU_OUT = CURRENT_STATE.EX_MEM_ALU_OUT;
                mem_write_32(CURRENT_STATE.MEM_WB_ALU_OUT, rt_selection);
                break;
            default:
                CURRENT_STATE.MEM_WB_ALU_OUT = CURRENT_STATE.EX_MEM_ALU_OUT;
                break;
            }
        }
    }
    sig_cpy_MEM_WB();
}

void WB_Stage(){
    if(CURRENT_STATE.MEM_WB_STALL){
        CURRENT_STATE.MEM_WB_STALL = 0;
    }
    else if(CURRENT_STATE.WB_FLUSH){
        CURRENT_STATE.WB_FLUSH = 0;
        CURRENT_STATE.PIPE[4] = 0x0;
    }
    else{
        CURRENT_STATE.PIPE[4] = CURRENT_STATE.PIPE[3];
        uint32_t wb_pc = CURRENT_STATE.PIPE[4];
        instruction* wb_inst = get_inst_info(wb_pc);
        if(OPCODE(wb_inst) == 0x0 && FUNC(wb_inst) != 0x8){
            CURRENT_STATE.REGS[RD(wb_inst)] = CURRENT_STATE.MEM_WB_ALU_OUT;
        }
        else{
            switch(OPCODE(wb_inst)){
            case 0x09:
            case 0x0c:
            case 0x0f:
            case 0x0d:
            case 0x0b:
                CURRENT_STATE.REGS[RT(wb_inst)] = CURRENT_STATE.MEM_WB_ALU_OUT;
                break;
            case 0x23:
                CURRENT_STATE.REGS[RT(wb_inst)] = CURRENT_STATE.MEM_WB_MEM_OUT;
                break;
            default:
                break;
            }
        }
        CURRENT_STATE.inst_num ++;
    }
}