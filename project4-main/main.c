
/* main.c 

Page Table Entry (PTE) format:

Width: 32 bits

Bit 31 ~ 12     : 20-bit physical page number for the 2nd-level page table node or the actual physical page.
Bit 1           : Dirty bit
Bit 0           : Valid bit

*/



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "util.h"  /* DO NOT DELETE */


/* addr_t type is the 32-bit address type defined in util.h */

typedef struct WAY{
    int valid;
    uint32_t tag;
    uint32_t ppn;
    int dirty;
    int priority;
} Way;

typedef struct SET{
    int idx;
    Way* way_list;
} Set;

struct TLB_miss_sig{
    int rorw;
    uint32_t set;
    int j;
    int lru;
    uint32_t tag;
    int dirty;
};


void TLB_init();
int set_bit();
int assoc_bit();
int find_low_pri();
int do_evict(uint32_t PPN);
uint32_t set_mask(int set_bit);
void TMS_clear(struct TLB_miss_sig* TMS);
void LRU_change_pri(uint32_t set, int j, int target_priority);
void TLB_hit_action(int rorw, uint32_t set, int j, addr_t addr);
void PT_hit_action(uint32_t PPN, addr_t pg_addr, int origin_dirty);
int TLB_search(int rorw, addr_t addr, int s_bit);
int page_walker(addr_t addr);
void page_fault_handler(addr_t addr);
void reset_inst(FILE* file);

int ENTRY_NUM = 0;
int ASSOC = 0;
int SET_NUM = 0;

int tlb_hit = 0;
int page_valid = 0;

struct TLB_miss_sig TMS = {0};
Way *** tlb = NULL;

int sdump_arg[8] = {0};

void TLB_init(){
    for(int i = 0; i < SET_NUM; i++){
        for(int j = 0; j < ASSOC; j++){
            tlb[i][j]->priority = ASSOC - 1 - j;
        }
    }
}

int set_bit(){
    int i = 0;
    int cpy_set = SET_NUM;
    cpy_set = cpy_set / 2;
    while(cpy_set != 0){
        cpy_set = cpy_set / 2;
        i++;
    }
    return i;
}

int assoc_bit(){
    int i = 0;
    int cpy_ass = ASSOC;
    cpy_ass = cpy_ass / 2;
    while(cpy_ass != 0){
        cpy_ass = cpy_ass / 2;
        i++;
    }
    return i;
}

int find_low_pri(){
    for(int i = 0; i < ASSOC; i++){
        if(tlb[TMS.set][i]->priority == ASSOC - 1){ //수정 필요할수도
            return i;
        }
    }
    fprintf(stderr, "error: no_lowest_pri\n");
    return 0;
}

int do_evict(uint32_t PPN){
    int target_j = find_low_pri();
    int target_priority = tlb[TMS.set][target_j]->priority;
    LRU_change_pri(TMS.set, target_j, target_priority);
    tlb[TMS.set][target_j]->tag = TMS.tag;
    tlb[TMS.set][target_j]->ppn = PPN;
    tlb[TMS.set][target_j]->valid = 1;
    tlb[TMS.set][target_j]->dirty = 0;
    if(TMS.rorw == 1){
        tlb[TMS.set][target_j]->dirty = 1;
    }
    return target_j;
}

uint32_t set_mask(int set_bit){
    return (1<<set_bit) - 1;
}

void TMS_clear(struct TLB_miss_sig* TMS){
    TMS->rorw = 0;
    TMS->set = 0;
    TMS->j = 0;
    TMS->lru = 0;
    TMS->dirty = 0;
}


void LRU_change_pri(uint32_t set, int j, int target_priority){
    for(int i = 0; i < ASSOC; i++){
        if(tlb[set][i]->priority < target_priority){
            tlb[set][i]->priority = tlb[set][i]->priority + 1;
        }
        else if(tlb[set][i]->priority == target_priority){
            tlb[set][i]->priority = 0;
            if(i != j){
                fprintf(stderr, "error : two same priority in sets!!!!\n");
            }
        }
    }
}

void TLB_hit_action(int rorw, uint32_t set, int j, addr_t addr){
    if(rorw == 1){ //write
        tlb[set][j]->dirty = 1;
        TMS.dirty = 1;
        int reset = page_walker(addr);
        if(reset != 0){
            fprintf(stderr, "error: something wrong");
        }
    }
    int target_priority = tlb[set][j]->priority;
    LRU_change_pri(set, j, target_priority);
    tlb[set][j]->valid = 1;
}

void PT_hit_action(uint32_t PPN, addr_t pg_addr, int origin_dirty){
    uint32_t ppn = PPN >> 12;
    if(TMS.rorw == 0 && TMS.lru == 0){
        tlb[TMS.set][TMS.j]->ppn = ppn;
        tlb[TMS.set][TMS.j]->dirty = origin_dirty;
        tlb[TMS.set][TMS.j]->valid = 1;
        int target_priority = tlb[TMS.set][TMS.j]->priority;
        LRU_change_pri(TMS.set, TMS.j, target_priority);
    }
    else if(TMS.rorw == 0 && TMS.lru == 1){
        int target_j = do_evict(ppn); //<-여기 안에서 TLB 원소 바꾸는것까지.
        tlb[TMS.set][target_j]->dirty = origin_dirty;
    }
    else if(TMS.rorw == 1 && TMS.lru == 0){
        tlb[TMS.set][TMS.j]->ppn = ppn;
        tlb[TMS.set][TMS.j]->dirty = 1;
        tlb[TMS.set][TMS.j]->valid = 1;
        int target_priority = tlb[TMS.set][TMS.j]->priority;
        LRU_change_pri(TMS.set, TMS.j, target_priority);
        mem_write_word32(pg_addr, (PPN|3));
    }
    else if(TMS.rorw == 1 && TMS.lru == 1){
        do_evict(ppn);
        mem_write_word32(pg_addr, (PPN|3));
    }
}

int TLB_search(int rorw, addr_t addr, int s_bit){ // return 0 or 1
    sdump_arg[3]++;
    uint32_t tag = (addr>>12)>>s_bit;
    uint32_t set = (addr>>12)&set_mask(s_bit);
    for(int j = 0; j < ASSOC; j ++){
        if(tlb[set][j]->tag == tag && tlb[set][j]->valid){
            tlb_hit = 1;
            TLB_hit_action(rorw, set, j, addr);
            sdump_arg[4]++;
            return tlb_hit;
        }
        else if(tlb[set][j]->tag == tag){
            tlb_hit = 0;
            TMS.rorw = rorw;
            TMS.set = set;
            TMS.j = j;
            TMS.lru = 0;
            TMS.tag = tag;
            sdump_arg[5]++;
            return tlb_hit;
        }
    }
    tlb_hit = 0;
    TMS.rorw = rorw;
    TMS.set = set;
    TMS.j = 0;
    TMS.lru = 1;
    TMS.tag = tag;
    sdump_arg[5]++;
    return tlb_hit;
}

void page_fault_handler(addr_t pg_addr){
    if(TMS.rorw == 0 && TMS.lru == 0){
        tlb[TMS.set][TMS.j]->ppn = (pg_addr>>12);
        int target_priority = tlb[TMS.set][TMS.j]->priority;
        tlb[TMS.set][TMS.j]->dirty = 0;
        tlb[TMS.set][TMS.j]->valid = 1;
        LRU_change_pri(TMS.set, TMS.j, target_priority);
    }
    else if(TMS.rorw == 0 && TMS.lru == 1){
        do_evict((pg_addr>>12)); //<-여기 안에서 TLB 원소 바꾸는것까지.
    }
    else if(TMS.rorw == 1 && TMS.lru == 0){
        tlb[TMS.set][TMS.j]->ppn = (pg_addr>>12);
        int target_priority = tlb[TMS.set][TMS.j]->priority;
        LRU_change_pri(TMS.set, TMS.j, target_priority);
        tlb[TMS.set][TMS.j]->dirty = 1;
        tlb[TMS.set][TMS.j]->valid = 1;
    }
    else if(TMS.rorw == 1 && TMS.lru == 1){
        do_evict((pg_addr>>12));
    }
}

int page_walker(addr_t addr){
    addr_t entry1_addr = (addr >> 22) << 2;
    addr_t entry2_addr = ((addr << 10) >> 22) << 2;
    uint32_t data1 = mem_read_word32(page_table_base_addr() + entry1_addr);
    if(TMS.dirty == 1){
        TMS.dirty = 0;
        entry2_addr = entry2_addr + ((data1 >> 2) << 2);
        uint32_t data2 = mem_read_word32(entry2_addr);
        if((data2 & 0x2) == 0){
            sdump_arg[6]++;
            mem_write_word32(entry2_addr, (data2|3));
        }
        return TMS.dirty;
    }
    sdump_arg[6]++;
    if((data1 & 0x1) == 0){
        page_valid = 0;
        addr_t pt_node_addr = get_new_page_table_node();
        addr_t pg_addr = get_new_physical_page();
        mem_write_word32((page_table_base_addr() + entry1_addr), ((pt_node_addr)|0x1));
        if(TMS.rorw == 1){
            mem_write_word32(pt_node_addr+entry2_addr, (pg_addr|3));
        }
        else{
            mem_write_word32(pt_node_addr+entry2_addr, (pg_addr|1));
        }
    }
    else{
        entry2_addr = entry2_addr + ((data1 >> 2) << 2);
        uint32_t data2 = mem_read_word32(entry2_addr);
        if((data2 & 0x1) == 0){
            page_valid = 0;
            addr_t pg_addr = get_new_physical_page();
            if(TMS.rorw == 1){
                mem_write_word32(entry2_addr, (pg_addr|3));
            }
            else{
                mem_write_word32(entry2_addr, (pg_addr|1));
            }
        }
        else{
            page_valid = 1;
            PT_hit_action(data2, entry2_addr, (data2&0x2)>>1);
        }
    }
    return page_valid;
}


void tdump(){
    printf("TLB Content:\n");
    printf("-------------------------------------\n");
    printf("    ");
    for(int i = 0; i < ASSOC; i++){
        printf("      ");
        printf("WAY[%d]",i);
    }
    printf("\n");
    for(int i = 0; i < SET_NUM; i++){
        printf("SET[%d]:   ",i);
        for(int j = 0; j < ASSOC; j++){
            printf(" (v=%d tag=0x%05x ppn=0x%05x d=%d) |", tlb[i][j]->valid, tlb[i][j]->tag, tlb[i][j]->ppn, tlb[i][j]->dirty);
        }
        printf("\n");
    }
}

void reset_inst(FILE* file){
    int cur_pos = ftell(file);
    int new_pos = cur_pos - 2;
    char c;
    while(new_pos > 0){
        fseek(file, new_pos, SEEK_SET);
        c = fgetc(file);
        if(c == '\n'){
            new_pos++;
            break;
        }
        new_pos--;
    }
    fseek(file, new_pos, SEEK_SET);
}

int main(int argc, char *argv[]) {
    int flag = 0x00;
    int E_A = 0;
    int file_entry = 0;
    char* EA_value = NULL;
    char* file_name = NULL;
    char buffer[256];
    char* RORW;
    char* ADDR;
    int rorw = 0;
    addr_t addr = 0;
    int s_bit = 0;
    FILE* file;

	init(); /* DO NOT DELETE. */
    for(int i = 0; i < argc; i++){
        if(strcmp(argv[i], "-c") == 0){
            flag = flag | 0x01;
        }
        if(strcmp(argv[3], "-x") == 0){
            flag = flag | 0x10;
        }
    }
    switch(flag){
    case 0x00:
        E_A = 1;
        file_entry = 2;
        break;
    case 0x01:
        E_A = 2;
        file_entry = 3;
        break;
    case 0x10:
        E_A = 1;
        file_entry = 3;
        break;
    case 0x11:
        E_A = 2;
        file_entry = 4;
        break;
    default:
        fprintf(stderr, "error: Instruction fetch fault");
        break;
    }
    EA_value = argv[E_A];
    file_name = argv[file_entry];
    char* token = strtok(EA_value, ":");
    ENTRY_NUM = atoi(token);
    token = strtok(NULL, ":");
    ASSOC = atoi(token);
    SET_NUM = ENTRY_NUM / ASSOC;
    s_bit = set_bit();

    //tlb table making
    tlb = malloc(SET_NUM * sizeof(void**));
    for(int i = 0; i < SET_NUM; i ++){
        tlb[i] = malloc(ASSOC * sizeof(void*));
        for(int j = 0; j < ASSOC; j ++){
            tlb[i][j] = malloc(sizeof(Way));
        }
    }
    // tlb[setnum][waynum]
    TLB_init();
    file = fopen(file_name, "r");
    if(file != NULL){
        while(fgets(buffer, sizeof(buffer), file)){
            RORW = strtok(buffer, " \t\n");
            ADDR = strtok(NULL, " \t\n");
            addr = (addr_t)strtoul(ADDR, NULL, 16);
            sdump_arg[0]++;
            if(strcmp(RORW, "R") == 0){
                rorw = 0;
                sdump_arg[1]++;
            }
            else if(strcmp(RORW, "W") == 0){
                rorw = 1;
                sdump_arg[2]++;
            }
            TMS_clear(&TMS);
            page_valid = 0;
            while(page_valid == 0){
                tlb_hit = TLB_search(rorw, addr, s_bit);
                if(tlb_hit == 0){
                    page_valid = page_walker(addr);
                    if(page_valid == 0){
                        sdump_arg[7]++;
                    }
                }
                else{
                    page_valid = 1;
                }
            }
        }
    }
    if(flag & 0x01){
        cdump(ENTRY_NUM, ASSOC);
    }
    if(flag & 0x10){
        sdump(sdump_arg[0], sdump_arg[1], sdump_arg[2], sdump_arg[3],
            sdump_arg[4], sdump_arg[5], sdump_arg[6], sdump_arg[7]);
        tdump();
        dump_page_table_area();
    }
    flag = 0x00;
    fclose(file);
    return 0;
}
