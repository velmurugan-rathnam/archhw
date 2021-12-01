#include <iostream>
#include <stdio.h>
#include "assembler.h"
#include <stdlib.h>

using namespace std;

lkupTbl opcodeTable[] = {
                           {"lw",0x00},
						   {"sw",0x01},
                           {"lb",0x02},
                           {"sb",0x03},
                           {"add",0x04},
                           {"addi",0x05},
                           {"sub",0x06},
                           {"subi",0x07},
                           {"and",0x08},
                           {"andi",0x09},
                           {"or",0x0a},
                           {"ori",0x0b},
                           {"ls",0x0c},
                           {"rs",0x0d},
                           {"bneq",0x0e},
                           {"beq",0x0f},
                           {"j",0x10},
                           {"mov",0x11},
                           {"slt",0x12},
                           {"li",0x13}, 
                           {"bal",0x14},
                           {"comp",0x15},
                           {"ret",0x16},                           
                           {"b",0x17},
                           {"exit",0x18},
                           {"break",0x19},
                           {"",  0x00}
                          };

lkupTbl  regTable[] = {
                        {"$r0",0x0},
                        {"$r1",0x1},
                        {"$r2",0x2},
                        {"$r3",0x3},
                        {"$r4",0x4},
                        {"$r5",0x5},
                        {"$r6",0x6},
                        {"$r7",0x7},
                        {"$r8",0x8},
                        {"$r9",0x9},
                        {"$r10",0xa},
                        {"$r11",0xb},
                        {"$r12",0xc},
                        {"$lr",0xd},
                        {"$imm",0xe},
                        {"$sp",0xf},                        
                        {"",0x0}
                      };



bool Assembler::process(FILE *output_file,FILE *input_file)
{
    unsigned char hex_opcode=0;    
    char opcode[10];char operands[25];
    char ascii_str[MAX_ASCII];
    bool get_operands=false;

    if(!opcode || !operands || !output_file || !input_file){
        cout << "Assembler::process-invalid args\n";
        return false;
    }

    //first pass.locate labels
    while(!feof(input_file)){
        memset(opcode,0,sizeof(opcode));
        memset(operands,0,sizeof(operands)); 
        input_file=getInput(input_file,opcode,10); 

        //check for EOF
        if(opcode[0] == 0)
            continue;        

        if(opcode[strlen(opcode)-1] == ':'){
            opcode[strlen(opcode)-1] = 0;
            //encountered a label            
            label_list.add(opcode,get_code_address());
            printf("%s at %x\n",opcode,get_code_address());
        }else if(!strcmp(opcode,".data")){
            memset(opcode,0,sizeof(opcode));            
            input_file=getInput(input_file,opcode,10);
            data_start_address = (unsigned short int)strtoul(opcode,NULL,0);
            current_data_address=data_start_address;

        }else if(!strcmp(opcode,".text")){
            memset(opcode,0,sizeof(opcode));            
            input_file=getInput(input_file,opcode,10);
            code_start_address = (unsigned short int)strtoul(opcode,NULL,0);
            reset_code_address();

        }else if(!strcmp(opcode,".ascii")){
            //only upto 75 bytes supported            

            memset(opcode,0,sizeof(opcode)); 
            memset(ascii_str,0,sizeof(ascii_str));
            input_file=getInput(input_file,opcode,10);            
            label_list.add(opcode,current_data_address);

            //gobble up the string            
            input_file=file_tokenize(input_file,ascii_str,MAX_ASCII,0xa,ignore_only_leading_spaces);            

            //cleanup the double quotes
            if(strlen(ascii_str))
                ascii_str[strlen(ascii_str)-1] = '\0';
            ascii_str[0] = ' ';            

            //produce the s1record for data
            produce_data_s1record(strlen(ascii_str),ascii_str,output_file);
            //update the current_data_address
            current_data_address += strlen(ascii_str);

            //make it word(16 bit) aligned
            if((current_data_address & 1) == 1){
                current_data_address += 1;
            }


        }else{
            hex_opcode=get_hex_opcode(opcode);            
            set_next_code_address(hex_opcode);

            //if beq or bneq increment by one more since they produce psuedo instructions
            if(hex_opcode==0xe || hex_opcode==0xf)
                set_next_code_address(hex_opcode);

            switch(hex_opcode){
            case 0x16:
                get_operands=false;
                break;
            default:
                get_operands=true;
            }

            if(get_operands)
                input_file=file_tokenize(input_file,operands,sizeof(operands)-1,0xa,ignore_all_spaces);
        }        
    }   

    printf("First pass done\n");

    //reset file ptr and address
    reset_code_address();
    fseek(input_file,0,SEEK_SET);
    

    //2nd pass.
    while(!feof(input_file))
	{
        memset(opcode,0,sizeof(opcode));
        memset(operands,0,sizeof(operands)); 

        input_file=getInput(input_file,opcode,10);

        //check for EOF
        if(opcode[0] == 0)
            continue; 

        //check for directives and other stuff to be ignored
        if( !strcmp(opcode,".data") || !strcmp(opcode,".text")){
            input_file=getInput(input_file,opcode,10);
            continue;
        }else if(!strcmp(opcode,".ascii")){
            memset(opcode,0,sizeof(opcode)); 
            memset(ascii_str,0,sizeof(ascii_str));
            input_file=getInput(input_file,opcode,10);            

            //gobble up the string            
            input_file=file_tokenize(input_file,ascii_str,MAX_ASCII,0xa,ignore_all_spaces);
            continue;
        }
        
        //ignore labels and directives
        if( opcode[strlen(opcode)-1] != ':'){
            hex_opcode=get_hex_opcode(opcode);

            switch(hex_opcode){
            case 0x16:
                get_operands=false;
                break;
            default:
                get_operands=true;
            }
            
            if(get_operands)
                input_file=file_tokenize(input_file,operands,sizeof(operands)-1,0xa,ignore_all_spaces);

            printf("%x  :",get_code_address());
            cout << opcode << " " << operands << "\n";
            s1rec_count++;
            produce_s1record(hex_opcode,operands,output_file);
            set_next_code_address(hex_opcode);            
        }	
        
	}


    produce_s5record(s1rec_count ,output_file);
    produce_s9record(code_start_address,output_file);

    return true;
}



void Assembler::produce_s9record(unsigned short int start,FILE *op)
{
    char s9record[11];
    unsigned short int checksum=0;

    if( !op){
        printf("produce_s9record:Invalid args\n");
        return;
    }
    memset(s9record,0,sizeof(s9record));

    checksum = add_bytes(start);
    checksum += 3;  //count
    checksum  = ~checksum;
    
    sprintf(s9record,"%s","S903");
    sprintf(&s9record[4],"%04x",start);    
    sprintf(&s9record[8],"%02x",checksum & 0xff);        
    
    writeOutput(s9record,op);    

}

void Assembler::produce_s5record(unsigned short int count,FILE *op)
{
    char s5record[11];
    unsigned short int checksum=0;    

    if( !op){
        printf("produce_s5record:Invalid args\n");
        return;
    }
    memset(s5record,0,sizeof(s5record));

    checksum = add_bytes(count);
    checksum += 3;  //count
    checksum  = ~checksum;
    
    sprintf(s5record,"%s","S503");
    sprintf(&s5record[4],"%04x",count);
    sprintf(&s5record[8],"%02x",(checksum & 0xff));

    writeOutput(s5record,op);    

}


void Assembler::produce_s1record(unsigned char hex_opcode,char *operands,FILE *op)
{
    unsigned short int srecord_data=0;

    if(!operands || !op){
        printf("produce_s1record:Invalid args\n");
        return;
    }

    //memory leak here. Need to free the data returned from get_s1record
    switch(hex_opcode){
        case 0xe:
        case 0xf:
            //handle the psuedo
            srecord_data=get_data(hex_opcode,operands);
            writeOutput(get_s1record(srecord_data),op);

            set_next_code_address(hex_opcode);
            s1rec_count++;
            srecord_data=get_data(COND_BRANCH_PSUEDO,operands);
            writeOutput(get_s1record(srecord_data),op);
            
            break;
        default:
            srecord_data=get_data(hex_opcode,operands);
            writeOutput(get_s1record(srecord_data),op);
            
    }    
    
}


void Assembler::produce_data_s1record(int len,char *databytes,FILE *op)
{    
    int i=0;

    if(!databytes || !op){
        printf("produce_s1record:Invalid args\n");
        return;
    }    
    
    s1rec_count++;
    char *s1record = (char *)malloc(sizeof(char)*len*2+10);
    if(!s1record){
        cout << "Error in malloc\n";
        return ;
    }
    unsigned short int checksum=0;
    unsigned short int addr=current_data_address;
    
    memset(s1record,0,sizeof(char)*len*2+10);
    checksum = add_bytes(databytes,len);
    checksum += add_bytes(addr);
    //3 = address+checksum
    checksum += add_bytes((unsigned short int)(len+3));                  //count
    checksum  = ~checksum;

    sprintf(s1record,"%s","S1");
    sprintf(&s1record[2],"%02x",(len+3));
    sprintf(&s1record[4],"%04x",addr);

    for(i=0;i<len;i++){
        sprintf(&s1record[8+i*2],"%02x",databytes[i]);
    }
    
    sprintf(&s1record[8+i*2],"%02x",checksum & 0xff);    
    writeOutput(s1record,op);        
    
}

FILE *Assembler::getInput(FILE *ip,char *str,int size)
{
    char retstr[500];
    char temp=0;

    if(!ip || !str){
         printf("Invalid args\n");
         return ip;
    }   

    while(!feof(ip)){
        memset(retstr,0,sizeof(retstr));
        fscanf(ip,"%s",retstr);        
    
        if(retstr[0] == '/' && retstr[1] == '/'){
            //comment encountered            
            //read out the full comment line till line feed          
            temp=0;
            while(temp != 0xa){                
                fscanf(ip,"%c",&temp);
            }
                
        }else
            break;
    }

    strncpy(str,retstr,size);    
    return ip;
    
}

unsigned char Assembler::get_hex_opcode(char *opcode)
{
    int i=0;
    bool opcode_found=false;
    unsigned char hex_opcode=0;

    for(i=0;strcmp(opcodeTable[i].charCode.c_str(),"");i++){
        if(!strcmp(opcodeTable[i].charCode.c_str(),opcode)){                        
            hex_opcode=opcodeTable[i].hexCode;
            opcode_found=true;
            break;
        }        
    }
    if(!opcode_found){
        cout  << "Opcode not found.Invalid instruction\n";
        return false;
    }

    return hex_opcode;
}

unsigned char Assembler::get_reg(char *regstr)
{
    int i=0;
    bool reg_found=false;
    unsigned char hex_regcode=0;

    for(i=0;strcmp(regTable[i].charCode.c_str(),"");i++){
        if(!strcmp(regTable[i].charCode.c_str(),regstr)){
            hex_regcode=regTable[i].hexCode;
            reg_found=true;
            break;
        }        
    }
    if(!reg_found){
        printf("%s\n",regstr);
        cout  << "Regcode not found.Invalid instruction\n";
        return 0;
    }

    return hex_regcode;

}

int Assembler::tokenize(char *dst,int len,char delimiter,char *src)
{
    int i=0;
    if(!dst || !src){
        printf("Assembler::tokenize:Invalid args\n");
        return 0;
    }

    for(i=0;src[i] != delimiter && (i < len);i++){
        dst[i] = src[i];
    }

    return i;
}

FILE* Assembler::file_tokenize(FILE *fp,char *dst,int len,char delimiter,FileTokenizeOptions options)
{
    int i=0;int j=0;
    char temp=0;
    bool non_space_char_seen=false;
    bool char_goes_in=false;
    
    if(!fp || !dst){
        printf("Assembler::file_tokenize:Invalid args\n");
        return 0;
    }

    while(temp != delimiter && (i<len) && !feof(fp) ){
        fscanf(fp,"%c",&temp);          

        if(isspace(temp)){
            if(options & ignore_only_leading_spaces){
                if(non_space_char_seen)
                    char_goes_in=true;
            }else if(options & ignore_all_spaces){
                char_goes_in=false;               
            }
        }else{
            char_goes_in=true;
            if(non_space_char_seen == false){
                non_space_char_seen=true;
            }

        }

        if((temp != delimiter) && (char_goes_in == true)){
            dst[i] = temp;
            i++;              
        }
    }    
    
    return fp;
}

unsigned short int Assembler::get_data(unsigned char opcode,char *operands)
{    
    unsigned short int data=0;            

    if(!operands){
        printf("Assembler::get_data:Invalid args\n");
        return 0;
    }
    
    switch(opcode & opcode_mask){
        case 0x0:
        case 0x1:
        case 0x2:
        case 0x3:  
            data = get_case1_data(opcode,operands);           
            break;
        case 0x5:
        case 0x7:
        case 0x9:
        case 0xb:
        case 0x13:
            data = get_case8_data(opcode,operands);
            break;
        case 0xc:
        case 0xd:        
            data = get_case2_data(opcode,operands);
            break;
        case 0xe:
        case 0xf:
            //beq,bneq
            data = get_case3_data(opcode,operands);
            break;
        case 0x10:
        case 0x14:
            data = get_case4_data(opcode,operands);
            break;
        case 0x4:
        case 0x6:
        case 0x8:
        case 0xa:
        case 0x11:
            data = get_case5_data(opcode,operands);
            break;
        case 0x12:
            data = get_case6_data(opcode,operands);
            break;
        case 0x15:
            data = get_case7_data(opcode,operands);
            break;  
        case COND_BRANCH_PSUEDO:
            data = get_cond_branch_data(opcode,operands);
            break;  
        default:
            data = data | ((opcode & opcode_mask) << opcode_shift);
    }

    return data;
}

unsigned short int Assembler::get_case1_data(unsigned char opcode,char *operands)
{
    unsigned short int data=0;
    unsigned char reg_code=0;
    unsigned char offset = 0;
    char token[10]={0};
    int i=0;int nextToken=0;

    if(!operands){
        printf("Assembler::get_case1_data:Invalid args\n");
        return 0;
    }
    
    memset(token,0,sizeof(token));  
    char *temp=operands;

    data = data | ((opcode & opcode_mask) << opcode_shift);

    memset(token,0,sizeof(token));
    nextToken=tokenize(token,10,',',temp);
    reg_code = get_reg(token);
    data |= (reg_code & reg_mask) << reg1_shift;

    temp += nextToken+1;
    memset(token,0,sizeof(token));
    nextToken=tokenize(token,10,'(',temp);
    offset = (unsigned char)strtoul(token,NULL,0);
    
    temp += nextToken+1;
    memset(token,0,sizeof(token));
    nextToken=tokenize(token,10,')',temp);
    reg_code = get_reg(token);
    data |= (reg_code & reg_mask) << reg2_shift;
    
    data |= offset & 7;

    return data;
}

unsigned short int Assembler::get_case2_data(unsigned char opcode,char *operands)
{
    unsigned short int data=0;
    unsigned char reg_code=0;
    unsigned char imm_val = 0;
    char token[10]={0};
    int i=0;int nextToken=0;

    if(!operands){
        printf("Assembler::get_case2_data:Invalid args\n");
        return 0;
    }
    
    memset(token,0,sizeof(token));  
    char *temp=operands;

    data = data | ((opcode & opcode_mask) << opcode_shift);

    memset(token,0,sizeof(token));
    nextToken=tokenize(token,10,',',temp);
    reg_code = get_reg(token);
    data |= (reg_code & reg_mask) << reg1_shift;

    temp += nextToken+1;
    memset(token,0,sizeof(token));
    nextToken=tokenize(token,10,'\0',temp);
    imm_val = (unsigned char)strtoul(token,NULL,0);    
    
    data |= imm_val & 0x7F;

    return data;
}


unsigned short int Assembler::get_case8_data(unsigned char opcode,char *operands)
{
    unsigned short int data=0;
    unsigned char reg_code=0;
    unsigned char imm_val = 0;
    char token[10]={0};
    int i=0;int nextToken=0;

    if(!operands){
        printf("Assembler::get_case8_data:Invalid args\n");
        return 0;
    }
    
    memset(token,0,sizeof(token));  
    char *temp=operands;

    data = data | ((opcode & opcode_mask) << opcode_shift);

    memset(token,0,sizeof(token));
    nextToken=tokenize(token,10,',',temp);
    reg_code = get_reg(token);
    data |= (reg_code & first8_reg_mask) << first8_reg1_shift;

    temp += nextToken+1;
    memset(token,0,sizeof(token));
    nextToken=tokenize(token,10,'\0',temp);
    imm_val = (unsigned char)strtoul(token,NULL,0);
    
    data |= imm_val & 0xFF;

    return data;
}

//beq,bneq
unsigned short int Assembler::get_case3_data(unsigned char opcode,char *operands)
{
    unsigned short int data=0;
    unsigned char reg_code=0;
    unsigned char imm_val = 0;
    char token[10]={0};
    int i=0;int nextToken=0;

    if(!operands){
        printf("Assembler::get_case3_data:Invalid args\n");
        return 0;
    }
    
    memset(token,0,sizeof(token));  
    char *temp=operands;

    data = data | ((opcode & opcode_mask) << opcode_shift);

    memset(token,0,sizeof(token));
    nextToken=tokenize(token,10,',',temp);
    reg_code = get_reg(token);
    data |= (reg_code & reg_mask) << reg1_shift;

    memset(token,0,sizeof(token));
    temp += nextToken+1;
    nextToken=tokenize(token,10,',',temp);
    reg_code = get_reg(token);
    data |= (reg_code & reg_mask) << reg2_shift;
    
    //For beq and bneq since they are psduedo 
    //always get the next instruction on 
    // successful branch.
    imm_val = 2;    
    data |= imm_val & 0x7;

    return data;
}


unsigned short int Assembler::get_case4_data(unsigned char opcode,char *operands)
{
    unsigned short int data=0;
    unsigned char reg_code=0;
    char token[10]={0};
    int i=0;int nextToken=0;
    unsigned short int address=0;


    //TODO:Bug here. check for number or string.
    //if string then lkup. Else put number

    if(!operands){
        printf("Assembler::get_case4_data:Invalid args\n");
        return 0;
    }
    
    memset(token,0,sizeof(token));  
    char *temp=operands;

    data = data | ((opcode & opcode_mask) << opcode_shift);
    
    memset(token,0,sizeof(token));
    nextToken=tokenize(token,10,'\0',temp);
    //imm_val = (unsigned short int)strtoul(token,NULL,0);   

    if(label_list.lkup(token,&address) == false){
        printf("get_case4_data:Error finding label\n");
        return 0;
    }
    signed short int displacement = address-get_code_address();    
    
    data |= displacement & 0x7FF;//11 bits

    return data;
}

unsigned short int Assembler::get_case5_data(unsigned char opcode,char *operands)
{
    unsigned short int data=0;
    unsigned char reg_code=0;
    char token[10]={0};
    int i=0;int nextToken=0;

    if(!operands){
        printf("Assembler::get_case5_data:Invalid args\n");
        return 0;
    }
    
    memset(token,0,sizeof(token));  
    char *temp=operands;

    data = data | ((opcode & opcode_mask) << opcode_shift);

    memset(token,0,sizeof(token));    
    nextToken=tokenize(token,10,',',temp);
    reg_code = get_reg(token);
    data |= (reg_code & reg_mask) << reg1_shift;
    
    memset(token,0,sizeof(token));
    temp += nextToken+1;
    nextToken=tokenize(token,10,'\0',temp);
    reg_code = get_reg(token);
    data |= (reg_code & reg_mask) << reg2_shift;

    return data;
}

unsigned short int Assembler::get_case6_data(unsigned char opcode,char *operands)
{
    unsigned short int data=0;
    unsigned char reg_code=0;   
    char token[10]={0};
    int i=0;int nextToken=0;

    if(!operands){
        printf("Assembler::get_case6_data:Invalid args\n");
        return 0;
    }
    
    memset(token,0,sizeof(token));  
    char *temp=operands;

    data = data | ((opcode & opcode_mask) << opcode_shift);

    memset(token,0,sizeof(token));    
    nextToken=tokenize(token,10,',',temp);
    reg_code = get_reg(token);
    data |= (reg_code & first8_reg_mask) << first8_reg1_shift;
    
    memset(token,0,sizeof(token));
    temp += nextToken+1;
    nextToken=tokenize(token,10,',',temp);
    reg_code = get_reg(token);
    //shift modified since only $r0-$r7 must be used for destination regiter
    data |= (reg_code & reg_mask) << reg2_shift+1;

    memset(token,0,sizeof(token));
    temp += nextToken+1;
    nextToken=tokenize(token,10,'\0',temp);
    reg_code = get_reg(token);
    data |= (reg_code & 0xf) << 0;

    return data;
}
 
unsigned short int Assembler::get_case7_data(unsigned char opcode,char *operands)
{
    unsigned short int data=0;
    unsigned char reg_code=0;   
    char token[10]={0};
    int i=0;int nextToken=0;

    if(!operands){
        printf("Assembler::get_case7_data:Invalid args\n");
        return 0;
    }
    
    memset(token,0,sizeof(token));  
    char *temp=operands;

    data = data | ((opcode & opcode_mask) << opcode_shift);   

    memset(token,0,sizeof(token));    
    nextToken=tokenize(token,10,'\0',temp);
    reg_code = get_reg(token);
    data |= (reg_code & reg_mask) << reg1_shift;

    return data;
}

unsigned short int Assembler::get_cond_branch_data(unsigned char opcode,char *operands)
{
    unsigned short int data=0;
    char lbl[MAX_LABEL_SIZE];       
    int i=0;int nextToken=0;
    unsigned short int address=0;
    char token[10]={0};    

    if(!operands){
        printf("Assembler::get_cond_branch_data:Invalid args\n");
        return 0;
    }    
      
    char *temp=operands;
    data = data | ((opcode & opcode_mask) << opcode_shift);   

    memset(token,0,sizeof(token));
    nextToken=tokenize(token,10,',',temp);    

    memset(token,0,sizeof(token));
    temp += nextToken+1;
    nextToken=tokenize(token,10,',',temp);    

    memset(lbl,0,sizeof(lbl));    
    temp += nextToken+1;
    nextToken=tokenize(lbl,200,'\0',temp); 

    if(label_list.lkup(lbl,&address) == false){
        printf("get_cond_branch_data:Error finding label\n");
        return 0;
    }
    signed short int displacement = address-get_code_address();

    data |= (displacement & 0x7FF) << 0;

    return data;
}


char *Assembler::get_s1record(unsigned short int data)
{    
    char *s1record = (char *)malloc(sizeof(char)*15);
    if(!s1record){
        cout << "Error in malloc\n";
        return NULL;
    }
    unsigned short int checksum=0;
    unsigned short int addr=get_code_address();
    
    memset(s1record,0,15);
    checksum = add_bytes(data);
    checksum += add_bytes(addr);
    checksum += 5;                  //count
    checksum  = ~checksum;

    sprintf(s1record,"%s","S105");
    sprintf(&s1record[4],"%04x",addr);
    sprintf(&s1record[8],"%04x",data);
    sprintf(&s1record[12],"%02x",checksum & 0xff);

    return s1record;
}


unsigned short int Assembler::add_bytes(unsigned short int word)
{
    unsigned short int sum=0;
    for(int i=0;i<sizeof(word);i++){
        sum += (word >> (i*8)) & 0xff;
    }
    return sum;
}

unsigned short int Assembler::add_bytes(char* bytes,int len)
{
    unsigned short int sum=0;
    for(int i=0;i<len;i++){
        sum += bytes[i] & 0xff;
    }
    return sum;
}

//set the next instruction address based on the current instruction being processed
void Assembler::set_next_code_address(unsigned char curr_opcode)
{
    switch(curr_opcode){
        case 0xe:
        case 0xf:            
            current_code_address += 2;
            break;
        default:
            current_code_address += 2;
    }
}

void Assembler::writeOutput(char *str,FILE *op)
{
    if(!str || !op){
        printf("Invalid args\n");
        return;
    }

    fprintf(op,"%s\n",str);
    
}


int main(int argc,char *argv[])
{
	if(argc < 3){	
		cout << "Usage:assembler <input filename> <output filename> \n";
		return 0;
	}	
    FILE *input_file = fopen(argv[1],"r");
    FILE *output_file = fopen(argv[2],"w");    

	if(!input_file || !output_file){
		cout << "Error opening the file\n";
		return 0;
	}

    Assembler assembler;
	assembler.process (output_file,input_file);
    
    fclose(input_file);
    fclose(output_file);

	return 0;
}



