#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef enum {
	ADDI, ADD, AND, ANDI, BEQ, BNE, J, JAL, JR, LUI, LW, LA, NOR, OR, ORI, SLTI, SLT, SLL, SRL, SW, SUB
} inst_type;

typedef struct{
	char * inst_str;
} inst_str;

inst_str inst_list[] = {
	{"addi"},
	{"add"},
	{"and"},
	{"andi"},
	{"beq"},
	{"bne"},
	{"j"},
	{"jal"},
	{"jr"},
	{"lui"},
	{"lw"},
	{"la"},
	{"nor"},
	{"or"},
	{"ori"},
	{"slti"},
	{"slt"},
	{"sll"},
	{"srl"},
	{"sw"},
	{"sub"},
};

int inst_str2num(char *inst){
	for(int i = 0; i < 21; i++){
		if(strcmp(inst_list[i].inst_str, inst) == 0){
			return i;
		}
	}
	return -1;
}

struct instruction{
	char* 	inst;
	char*	format;
	int 	reg[3];
	int		imm;
	char* 	label_name;
	char* 	la_data;
	int		cur_pc;
	struct instruction* next;
};

struct reference{
	char* 	name;
	int		data;
	int		label;
	int 	mode;
	// 0 for data, 1 for label.
	struct reference* next;
};

char* copy_string(char* target){
	int l = strlen(target);
	char* copy = (char*)malloc(l+1);
	strcpy(copy, target);
	return copy;
}

void print_binary(int a){
	for(int i = 31; i >= 0 ; i--){
		printf("%d",(a>>i) & 1);
	}
}


void print_reg_binary(int reg_num){
	for(int i = 4; i >= 0 ; i--){
		printf("%d",(reg_num>>i) & 1);
	}
}

void print_imm_binary(int imm_num){
	for(int i = 15; i >= 0 ; i--){
		printf("%d",(imm_num>>i) & 1);
	}	
}

void print_add_binary(int add_num){
	for(int i = 25; i >= 0 ; i--){
		printf("%d",(add_num>>i) & 1);
	}	
}

int get_data(struct reference* ref_rftable, char* data_name){
	struct reference* cur_rftable = ref_rftable;
	while(cur_rftable != NULL){
		int l = strlen(cur_rftable->name);
		char *b = (char*)malloc(l);
		strncpy(b,cur_rftable->name,l-1);
		if(strcmp(data_name, b) == 0){
			return cur_rftable->data;
		}
		cur_rftable = cur_rftable->next;
	}
}

int get_label(struct reference* ref_rftable, char* lable_name){
	struct reference* cur_rftable = ref_rftable;
	while(cur_rftable != NULL){
		int l = strlen(cur_rftable->name);
		char *b = (char*)malloc(l);
		strncpy(b,cur_rftable->name,l-1);
		if(strcmp(lable_name, b) == 0){
			return cur_rftable->label;
		}
		cur_rftable = cur_rftable->next;
	}
}

void print_all_data(struct reference* ref_rftable){
	struct reference* cur_rftable = ref_rftable;
	while(cur_rftable != NULL && cur_rftable->mode == 0){
		print_binary(cur_rftable->data);
		cur_rftable = cur_rftable->next;
	}
}


int main(int argc, char* argv[]){

	if(argc != 2) {
		printf("Usage: ./runfile <assembly file>\n"); //Example) ./runfile /sample_input/example1.s
		printf("Example) ./runfile ./sample_input/example1.s\n");
		exit(0);
	} else {

		// To help you handle the file IO, the deafult code is provided.
		// If we use freopen, we don't need to use fscanf, fprint,..etc. 
		// You can just use scanf or printf function 
		// ** You don't need to modify this part **
		// If you are not famailiar with freopen,  you can see the following reference
		// http://www.cplusplus.com/reference/cstdio/freopen/

		//For input file read (sample_input/example*.s)

		char *file = (char *)malloc(strlen(argv[1]) + 3);
		strncpy(file, argv[1], strlen(argv[1]));

		if (freopen(file, "r",stdin) == 0) { //std input을 file로 변경
			printf("File open Error!\n");
			exit(1);
		}

		//From now on, if you want to read string from input file, you can just use scanf function.


		// For output file write 
		// You can see your code's output in the sample_input/example#.o 
		// So you can check what is the difference between your output and the answer directly if you see that file
		// make test command will compare your output with the answer
		file[strlen(file) - 1] = 'o';
		freopen(file, "w", stdout); //stdout을 file로 변경
		//If you use printf from now on, the result will be written to the output file.
		int section = 0; // 0 for .data section, 
		// 1 for 32bit data recog,
		// 3 for .text section
		// 9 for data name or .word recog
		int data_size = 0;
		int data_add = 0;
		int text_size = 0;
		int inst_num = 0;
		int cur_add = 0;
		int la_upp_data;
		int la_num = 0;
		int k;
		char*a = NULL;
		int array = 0;
		char* prev_data_name = NULL;
		struct instruction* inst_start = (struct instruction*)malloc(sizeof(struct instruction));
		struct reference* rftable = (struct reference*)malloc(sizeof(struct reference));
		struct instruction* cur_inst = inst_start;
		struct reference* cur_tab = rftable;
		struct reference* tab_head = rftable;
		while(scanf("%s", file) != EOF){
			if(strcmp(file,".data") == 0){
				section = 9;
			}
			else if(section == 9){
				if(strcmp(file,".word") == 0 && array == 1){
					cur_tab->name = prev_data_name;
					section = 1;
				}
				else if(strcmp(file,".word") == 0 && array == 0){
					section = 1;
				}
				else if(strcmp(file,".text") == 0){
					section = 3;
				}
				else{
					prev_data_name = copy_string(file);
					cur_tab->name = prev_data_name;
					array = 0;
					section = 9;
				}
			}
			else if(section == 1){
				data_size++;
				data_add++;
				cur_tab->data = strtol(file,NULL,0);
				cur_tab->mode = 0;
				cur_tab->label = 0x10000000 + (data_add-1)*4;
				cur_tab->next = (struct reference*)malloc(sizeof(struct reference));
				cur_tab = cur_tab->next;
				array = 1;
				section = 9;
			}
			else if(section == 3){
				inst_num = inst_str2num(file);
				if(inst_num == -1){
					cur_tab->name = copy_string(file);
					cur_tab->label = text_size;
					cur_tab->mode = 1;
					cur_tab->next = (struct reference*)malloc(sizeof(struct reference));
					cur_tab = cur_tab->next;
				}
				else{
					cur_add ++;
					text_size ++;
					int i = 0;
					switch(inst_num){
						case ADDI:
							for(i = 0; i < 3; i++){
								scanf("%s",file);
							}
							break;
						case ADD:
							for(i = 0; i < 3; i++){
								scanf("%s",file);
							}
							break;
						case AND:
							for(i = 0; i < 3; i++){
								scanf("%s",file);
							}
							break;
						case ANDI:
							for(i = 0; i < 3; i++){
								scanf("%s",file);
							}
							break;
						case BEQ:
							for(i = 0; i < 3; i++){
								scanf("%s",file);
							}
							break;
						case BNE:
							for(i = 0; i < 3; i++){
								scanf("%s",file);
							}
							break;
						case J:
							for(i = 0; i < 1; i++){
								scanf("%s",file);
							}
							break;
						case JAL:
							for(i = 0; i < 1; i++){
								scanf("%s",file);
							}
							break;
						case JR:
							for(i = 0; i < 1; i++){
								scanf("%s",file);
							}
							break;
						case LUI:
							for(i = 0; i < 2; i++){
								scanf("%s",file);
							}
							break;
						case LW:
							for(i = 0; i < 2; i++){
								scanf("%s",file);
							}
							break;
						case LA:
							text_size ++;
							for(i = 0; i < 2; i++){
								scanf("%s",file);
								if(i == 1){
									a = copy_string(file);
								}
								else{
									cur_inst->reg[i] = atoi(file+1);
								}
							}
							k = get_label(tab_head, a);
							if((k>>16)<<16 == k){
								text_size --;
							}
							break;
						case NOR:
							for(i = 0; i < 3; i++){
								scanf("%s",file);
							}
							break;
						case OR:
							for(i = 0; i < 3; i++){
								scanf("%s",file);
							}
							break;
						case ORI:
							for(i = 0; i < 3; i++){
								scanf("%s",file);
							}
							break;
						case SLTI:
							for(i = 0; i < 3; i++){
								scanf("%s",file);
							}
							break;
						case SLT:
							for(i = 0; i < 3; i++){
								scanf("%s",file);
							}
							break;
						case SLL:
							for(i = 0; i < 3; i++){
								scanf("%s",file);
							}
							break;
						case SRL:
							for(i = 0; i < 3; i++){
								scanf("%s",file);
							}
							break;
						case SW:
							for(i = 0; i < 2; i++){
								scanf("%s",file);
							}
							break;
						case SUB:
							for(i = 0; i < 3; i++){
								scanf("%s",file);
							}
							break;
						default:
							printf("switch error");
							break;
					}
				}
			}
		}
		print_binary(text_size * 4);
		print_binary(data_size * 4);
		fseek(stdin, 0, SEEK_SET);
		int line_num = -1;
		int add_diff;
		la_num = 0;
		char lw_sign;
		
		while(scanf("%s", file) != EOF){
			if(strcmp(file,".text") == 0){
				section = 3;
			}
			else if(section == 3){
				inst_num = inst_str2num(file);
				if(inst_num == -1){
					cur_tab->name = file;
					cur_tab->label = line_num + 1;
					cur_tab->next = (struct reference*)malloc(sizeof(struct reference));
					cur_tab = cur_tab->next;
				}
				else{
					line_num = line_num + 1;
					cur_add ++;
					int i = 0;
					switch(inst_num){
						case ADDI:
							printf("001000");
							cur_inst->format = "I";
							for(i = 0; i < 3; i++){
								scanf("%s",file);
								if(i == 2){
									cur_inst->imm = strtol(file,NULL,0);
								}
								else{
									cur_inst->reg[i] = atoi(file+1);
								}
							}
							print_reg_binary(cur_inst->reg[1]);
							print_reg_binary(cur_inst->reg[0]);
							print_imm_binary(cur_inst->imm);
							cur_inst->next = (struct instruction*)malloc(sizeof(struct instruction));
							cur_inst = cur_inst->next;
							break;
						case ADD:
							printf("000000");
							cur_inst->format = "R";
							for(i = 0; i < 3; i++){
								scanf("%s",file);
								if(i == 0){
									cur_inst->reg[i+2] = atoi(file+1);	
								}
								else{
									cur_inst->reg[i-1] = atoi(file+1);	
								}
							}
							for(i = 0; i < 3; i++){
								print_reg_binary(cur_inst->reg[i]);
							}
							printf("00000100000");
							cur_inst->next = (struct instruction*)malloc(sizeof(struct instruction));
							cur_inst = cur_inst->next;
							//출력떄는 뒤에 funct까지 잘 출력
							break;
						case AND:
							printf("000000");
							cur_inst->format = "R";
							for(i = 0; i < 3; i++){
								scanf("%s",file);
								if(i == 0){
									cur_inst->reg[i+2] = atoi(file+1);	
								}
								else{
									cur_inst->reg[i-1] = atoi(file+1);	
								}
							}
							for(i = 0; i < 3; i++){
								print_reg_binary(cur_inst->reg[i]);
							}
							printf("00000100100");
							cur_inst->next = (struct instruction*)malloc(sizeof(struct instruction));
							cur_inst = cur_inst->next;
							break;
						case ANDI:
							printf("001100");
							cur_inst->format = "I";
							for(i = 0; i < 3; i++){
								scanf("%s",file);
								if(i == 2){
									cur_inst->imm = strtol(file,NULL,0);
								}
								else{
									cur_inst->reg[i] = atoi(file+1);
								}
							}
							print_reg_binary(cur_inst->reg[1]);
							print_reg_binary(cur_inst->reg[0]);
							print_imm_binary(cur_inst->imm);
							cur_inst->next = (struct instruction*)malloc(sizeof(struct instruction));
							cur_inst = cur_inst->next;
							break;
						case BEQ:
							printf("000100");
							cur_inst->format = "I";
							
							for(i = 0; i < 3; i++){
								scanf("%s",file);
								if(i == 2){
									a = copy_string(file);
									cur_inst->cur_pc = line_num + 1;
								}
								else{
									cur_inst->reg[i] = atoi(file+1);
								}
							}
							print_reg_binary(cur_inst->reg[0]);
							print_reg_binary(cur_inst->reg[1]);
							k = get_label(tab_head, a);
							add_diff = k - cur_inst->cur_pc;
							print_imm_binary(add_diff);
							cur_inst->next = (struct instruction*)malloc(sizeof(struct instruction));
							cur_inst = cur_inst->next;
							break;
						case BNE:
							printf("000101");
							cur_inst->format = "I";
							for(i = 0; i < 3; i++){
								scanf("%s",file);
								if(i == 2){
									a = copy_string(file);
									cur_inst->cur_pc = line_num + 1;
								}
								else{
									cur_inst->reg[i] = atoi(file+1);
								}
							}
							print_reg_binary(cur_inst->reg[0]);
							print_reg_binary(cur_inst->reg[1]);
							k = get_label(tab_head, a);
							add_diff = k - cur_inst->cur_pc;
							print_imm_binary(add_diff);
							cur_inst->next = (struct instruction*)malloc(sizeof(struct instruction));
							cur_inst = cur_inst->next;
							break;
						case J:
							printf("000010");
							cur_inst->format = "J";
							for(i = 0; i < 1; i++){
								scanf("%s",file);
								a = copy_string(file);
								cur_inst->cur_pc = line_num + 1;
							}
							k = get_label(tab_head, a);
							add_diff = k + 0x100000;
							print_add_binary(add_diff);
							cur_inst->next = (struct instruction*)malloc(sizeof(struct instruction));
							cur_inst = cur_inst->next;
							break;
						case JAL:
							printf("000011");
							cur_inst->format = "J";
							for(i = 0; i < 1; i++){
								scanf("%s",file);
								a = copy_string(file);
								cur_inst->cur_pc = line_num + 1;
							}
							k = get_label(tab_head, a);
							add_diff = k + 0x100000;
							print_add_binary(add_diff);
							cur_inst->next = (struct instruction*)malloc(sizeof(struct instruction));
							cur_inst = cur_inst->next;
							break;
						case JR:
							printf("000000");
							cur_inst->format = "R";
							for(i = 0; i < 1; i++){
								scanf("%s",file);
								if(i == 0){
									cur_inst->reg[i] = atoi(file+1);	
								}
							}
							for(i = 0; i < 1; i++){
								print_reg_binary(cur_inst->reg[i]);
							}
							printf("000000000000000001000");
							cur_inst->next = (struct instruction*)malloc(sizeof(struct instruction));
							cur_inst = cur_inst->next;
							break;
						case LUI:
							printf("001111");
							cur_inst->format = "I";
							for(i = 0; i < 2; i++){
								scanf("%s",file);
								if(i == 1){
									cur_inst->imm = strtol(file,NULL,0);
								}
								else{
									cur_inst->reg[i] = atoi(file+1);
								}
							}
							printf("00000");
							print_reg_binary(cur_inst->reg[0]);
							print_imm_binary(cur_inst->imm);
							cur_inst->next = (struct instruction*)malloc(sizeof(struct instruction));
							cur_inst = cur_inst->next;
							break;
						case LW:
							printf("100011");
							cur_inst->format = "I";
							for(i = 0; i < 2; i++){
								scanf("%s",file);
								if(i == 1){
									int l = strlen(file);
									if(*(file+l-3) == '$'){
										cur_inst->reg[1] = atoi(file+l-2);
									}
									else{
										cur_inst->reg[1] = atoi(file+l-3);
									}
									a = copy_string(file);
									if(*a == '-'){
										cur_inst->imm = 0xFFFF - strtol(file+1,NULL,0) + 1;
									}
									else{
										cur_inst->imm = strtol(file,NULL,0);
									}
								}
								else{
									cur_inst->reg[i] = atoi(file+1);
								}
							}
							print_reg_binary(cur_inst->reg[1]);
							print_reg_binary(cur_inst->reg[0]);
							print_imm_binary(cur_inst->imm);
							cur_inst->next = (struct instruction*)malloc(sizeof(struct instruction));
							cur_inst = cur_inst->next;
							break;
						case LA:
							line_num ++;
							for(i = 0; i < 2; i++){
								scanf("%s",file);
								if(i == 1){
									a = copy_string(file);
								}
								else{
									cur_inst->reg[i] = atoi(file+1);
								}
							}
							k = get_label(tab_head, a);
							if((k>>16)<<16 == k){
								printf("00111100000");
								print_reg_binary(cur_inst->reg[0]);
								print_imm_binary(k>>16);
								line_num --;
							}
							else{
								printf("00111100000");
								print_reg_binary(cur_inst->reg[0]);
								print_imm_binary(k>>16);
								printf("001101");
								print_reg_binary(cur_inst->reg[0]);
								print_reg_binary(cur_inst->reg[0]);
								print_imm_binary(k);
							}
							cur_inst->next = (struct instruction*)malloc(sizeof(struct instruction));
							cur_inst = cur_inst->next;
							break;
						case NOR:
							printf("000000");
							cur_inst->format = "R";
							for(i = 0; i < 3; i++){
								scanf("%s",file);
								if(i == 0){
									cur_inst->reg[i+2] = atoi(file+1);	
								}
								else{
									cur_inst->reg[i-1] = atoi(file+1);	
								}
							}
							for(i = 0; i < 3; i++){
								print_reg_binary(cur_inst->reg[i]);
							}
							printf("00000100111");
							cur_inst->next = (struct instruction*)malloc(sizeof(struct instruction));
							cur_inst = cur_inst->next;
							break;
						case OR:
							printf("000000");
							cur_inst->format = "R";
							for(i = 0; i < 3; i++){
								scanf("%s",file);
								if(i == 0){
									cur_inst->reg[i+2] = atoi(file+1);	
								}
								else{
									cur_inst->reg[i-1] = atoi(file+1);	
								}
							}
							for(i = 0; i < 3; i++){
								print_reg_binary(cur_inst->reg[i]);
							}
							printf("00000100101");
							cur_inst->next = (struct instruction*)malloc(sizeof(struct instruction));
							cur_inst = cur_inst->next;
							break;
						case ORI:
							printf("001101");
							cur_inst->format = "I";
							for(i = 0; i < 3; i++){
								scanf("%s",file);
								if(i == 2){
									cur_inst->imm = strtol(file,NULL,0);
								}
								else{
									cur_inst->reg[i] = atoi(file+1);
								}
							}
							print_reg_binary(cur_inst->reg[1]);
							print_reg_binary(cur_inst->reg[0]);
							print_imm_binary(cur_inst->imm);
							cur_inst->next = (struct instruction*)malloc(sizeof(struct instruction));
							cur_inst = cur_inst->next;
							break;
						case SLTI:
							printf("001010");
							cur_inst->format = "I";
							for(i = 0; i < 3; i++){
								scanf("%s",file);
								if(i == 2){
									cur_inst->imm = strtol(file,NULL,0);
								}
								else{
									cur_inst->reg[i] = atoi(file+1);
								}
							}
							print_reg_binary(cur_inst->reg[1]);
							print_reg_binary(cur_inst->reg[0]);
							print_imm_binary(cur_inst->imm);
							cur_inst->next = (struct instruction*)malloc(sizeof(struct instruction));
							cur_inst = cur_inst->next;
							break;
						case SLT:
							printf("000000");
							cur_inst->format = "R";
							for(i = 0; i < 3; i++){
								scanf("%s",file);
								if(i == 0){
									cur_inst->reg[i+2] = atoi(file+1);	
								}
								else{
									cur_inst->reg[i-1] = atoi(file+1);	
								}
							}
							for(i = 0; i < 3; i++){
								print_reg_binary(cur_inst->reg[i]);
							}
							printf("00000101010");
							cur_inst->next = (struct instruction*)malloc(sizeof(struct instruction));
							cur_inst = cur_inst->next;
							break;
						case SLL:
							printf("000000");
							cur_inst->format = "R";
							for(i = 0; i < 3; i++){
								scanf("%s",file);
								cur_inst->reg[i] = atoi(file+1);
								if(i == 2){
									cur_inst->imm = strtol(file,NULL,0);
								}
							}
							printf("00000");
							print_reg_binary(cur_inst->reg[1]);
							print_reg_binary(cur_inst->reg[0]);
							print_reg_binary(cur_inst->imm);
							printf("000000");
							cur_inst->next = (struct instruction*)malloc(sizeof(struct instruction));
							cur_inst = cur_inst->next;
							break;
						case SRL:
							printf("000000");
							cur_inst->format = "R";
							for(i = 0; i < 3; i++){
								scanf("%s",file);
								cur_inst->reg[i] = atoi(file+1);
								if(i == 2){
									cur_inst->imm = strtol(file,NULL,0);
								}
							}
							printf("00000");
							print_reg_binary(cur_inst->reg[1]);
							print_reg_binary(cur_inst->reg[0]);
							print_reg_binary(cur_inst->imm);
							printf("000010");
							cur_inst->next = (struct instruction*)malloc(sizeof(struct instruction));
							cur_inst = cur_inst->next;
							break;
						case SW:
							printf("101011");
							cur_inst->format = "I";
							for(i = 0; i < 2; i++){
								scanf("%s",file);
								if(i == 1){
									int l = strlen(file);
									if(*(file+l-3) == '$'){
										cur_inst->reg[1] = atoi(file+l-2);
									}
									else{
										cur_inst->reg[1] = atoi(file+l-3);
									}
									a = copy_string(file);
									if(*a == '-'){
										cur_inst->imm = 0xFFFF - strtol(file+1,NULL,0) + 1;
									}
									else{
										cur_inst->imm = strtol(file,NULL,0);
									}
								}
								else{
									cur_inst->reg[i] = atoi(file+1);
								}
							}
							print_reg_binary(cur_inst->reg[1]);
							print_reg_binary(cur_inst->reg[0]);
							print_imm_binary(cur_inst->imm);
							cur_inst->next = (struct instruction*)malloc(sizeof(struct instruction));
							cur_inst = cur_inst->next;
							break;
						case SUB:
							printf("000000");
							cur_inst->format = "R";
							for(i = 0; i < 3; i++){
								scanf("%s",file);
								if(i == 0){
									cur_inst->reg[i+2] = atoi(file+1);	
								}
								else{
									cur_inst->reg[i-1] = atoi(file+1);	
								}
							}
							for(i = 0; i < 3; i++){
								print_reg_binary(cur_inst->reg[i]);
							}
							printf("00000100010");
							cur_inst->next = (struct instruction*)malloc(sizeof(struct instruction));
							cur_inst = cur_inst->next;
							break;
						default:
							printf("switch error");
							break;
					}
				}
			}
		}
		print_all_data(tab_head);
	}
	return 0;
}

