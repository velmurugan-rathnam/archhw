#ifndef ASSEMBLER
#define ASSEMBLER
#include <string>
using namespace std;



enum {
        //masks
        opcode_mask = 0x1f,
        reg_mask= 0xf,
        first8_reg_mask=0x7,


        //shifts
        opcode_shift = 11,
        first8_reg1_shift = 8,
        reg1_shift = 7,
        reg2_shift = 3,

        COND_BRANCH_PSUEDO = 0x17,
        MAX_LABEL_SIZE=200,
        MAX_ASCII=75
    };

enum FileTokenizeOptions {
    ignore_only_leading_spaces = (1 << 0),
    ignore_all_spaces = (1 << 1)
};


struct lkupTbl {
	string charCode;
	unsigned char hexCode;	
};


struct labelEntry {
    labelEntry(char *lbl,unsigned short int loc){
        if(lbl)
            strcpy(label,lbl);
        location = loc;
        next=NULL;
    }

    void addTail(labelEntry *new_entry){
        if(new_entry){
            next = new_entry;
        }
    }

    char label[MAX_LABEL_SIZE];
    unsigned short int location;
    struct labelEntry *next;
};

struct labelLst {

    labelLst(){
        head=NULL;
        tail=NULL;
    }

    void add(char *lbl,unsigned short int loc){
        if(!lbl){
            printf("Error creating label entry\n");
            return;
        }
            
        labelEntry *newEntry=new labelEntry(lbl,loc);

        if(!head){
            head=newEntry;
            tail=head;
        }
        else{
            tail->addTail(newEntry);
            tail=newEntry;
        }
    }

    bool lkup(char *lbl,unsigned short int* loc)
    {
        if(!lbl || !loc)
            return false;

        labelEntry *temp=head;
        while(temp){
            if(!strcmp(temp->label,lbl)){
                *loc = temp->location;
                return true;
            }else{
                temp=temp->next;
            }
        }

        return false;
    }

    labelEntry *head;
    labelEntry *tail;
};


class Assembler {


    
public:
	Assembler(){current_code_address=0;code_start_address=0;current_data_address=0;data_start_address=0;s1rec_count=0;}
	~Assembler(){}
	bool process(FILE *output_file,FILE *input_file);
    void produce_s1record(unsigned char hex_opcode,char *operands,FILE *op);
    void produce_data_s1record(int len,char *databytes,FILE *op);
    void produce_s5record(unsigned short int count,FILE *op);
    void produce_s9record(unsigned short int start,FILE *op);
    unsigned short int get_data(unsigned char opcode,char *operands);
    char *get_s1record(unsigned short int data);
    void writeOutput(char *str,FILE *op);    
    FILE* getInput(FILE *ip,char *str,int size);
    unsigned short int get_case1_data(unsigned char opcode,char *operands);
    unsigned short int get_case2_data(unsigned char opcode,char *operands);
    unsigned short int get_case3_data(unsigned char opcode,char *operands);
    unsigned short int get_case4_data(unsigned char opcode,char *operands);
    unsigned short int get_case5_data(unsigned char opcode,char *operands);
    unsigned short int get_case6_data(unsigned char opcode,char *operands);
    unsigned short int get_case7_data(unsigned char opcode,char *operands); 
    unsigned short int get_case8_data(unsigned char opcode,char *operands);
    unsigned short int get_cond_branch_data(unsigned char opcode,char *operands);

private: //data
    unsigned short int current_code_address;
    unsigned short int code_start_address;
    unsigned short int current_data_address;
    unsigned short int data_start_address;
    labelLst label_list;
    unsigned short int s1rec_count;
    

private://routines
    unsigned short int get_code_address(){return current_code_address;}
    void set_next_code_address(unsigned char curr_opcode);
    unsigned short int add_bytes(unsigned short int word);
    unsigned short int add_bytes(char* bytes,int len);
    unsigned char get_reg(char *regstr);
    int tokenize(char *dst,int len,char delimiter,char *src);
    FILE * file_tokenize(FILE *fp,char *dst,int len,char delimiter,FileTokenizeOptions options);
    unsigned char get_hex_opcode(char *opcode);
    void reset_code_address(){current_code_address=code_start_address;}
};





#endif //ASSEMBLER
