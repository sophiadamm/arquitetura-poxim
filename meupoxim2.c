#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>

int32_t x[32] = {0};
static int32_t saved_x[32] = {0};
const char* nomex[32] = { "zero", "ra", "sp", "gp", "tp", "t0", "t1", "t2", "s0", "s1", "a0", "a1", "a2", 
                            "a3", "a4", "a5", "a6", "a7", "s2", "s3", "s4", "s5", "s6", "s7", "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6"};
const uint32_t offset = 0x80000000;
uint8_t run = 1;

/*POXIM V2*/
//Registradores CSR
uint32_t mstatus = 0; // Registrador de status
uint32_t mie = 0;     // Habilitação de Interupção
uint32_t mtvec = 0;   // Endereço base do gerenciador
uint32_t mepc = 0;    // PC que gerou evento
uint32_t mcause = 0;  // Causa do Evento
uint32_t mtval = 0;   // Endereço/Instrução inválido
uint32_t mip = 0;     // Pendência de interrupção

void write_csr(uint32_t csr_addr, uint32_t value) {
    switch (csr_addr) {
        case 0x300: mstatus = value; break;
        case 0x304: mie = value; break;
        case 0x305: mtvec = value; break;
        case 0x341: mepc = value; break;
        case 0x342: mcause = value; break;
        case 0x343: mtval = value; break;
        case 0x344: mip = value; break;
        default: break; 
    }
}

const char* nome_csr(uint32_t csr_addr) {
    switch (csr_addr) {
        case 0x300: return "mstatus";
        case 0x304: return "mie";
        case 0x305: return "mtvec";
        case 0x341: return "mepc";
        case 0x342: return "mcause";
        case 0x343: return "mtval";
        case 0x344: return "mip";
        default: 
            return "unknown"; 
    }
}

uint32_t read_csr(uint32_t addr) {
    switch (addr) {
        case 0x300: return mstatus;
        case 0x304: return mie;
        case 0x305: return mtvec;
        case 0x341: return mepc;
        case 0x342: return mcause;
        case 0x343: return mtval;
        case 0x344: return mip;
        default:
            return 0;
    }
}
  
const uint32_t CLEAR_MASK = (3U << 11) | (1U << 7) | (1U << 3);


void trap_capture(uint32_t cause, uint32_t tval, uint32_t *pc_ptr, FILE *saida){
    mcause = cause;        
    mtval = tval;
    mepc = *pc_ptr;

    uint32_t mstatus_old = mstatus;
    uint32_t mie_bit = (mstatus_old >> 3) & 1;
    mstatus &= ~(1 << 3);       
    mstatus &= ~(1 << 7);       
    mstatus |= (mie_bit << 7);  
    mstatus |= (0b11 << 11);    

    fprintf(saida, ">exception:");
    switch (mcause) {
        case 1: fprintf(saida, "instruction_fault         "); break;
        case 2: fprintf(saida, "illegal_instruction       "); break;
        case 5: fprintf(saida, "load_fault                "); break;
        case 7: fprintf(saida, "store_fault               "); break;
        default: fprintf(saida, "unkown                    "); break;
    }

    if (mcause != 11) { 
        fprintf(saida,",epc=0x%08x,tval=0x%08x\n", mepc, mtval);
    }
    *pc_ptr = (mtvec & 0xFFFFFFFC) - 4;    
}

void trap_return(uint32_t *pc_ptr, FILE *saida){
    uint32_t mpie_bit = (mstatus >> 7) & 1;
    mstatus &= ~CLEAR_MASK; 
    mstatus |= (mpie_bit << 3); 
    mstatus |= (1 << 7);
    mstatus |= (1 << 7);
    *pc_ptr = mepc - 4; 
}

void CSR_Fluxo(uint32_t instrucao, uint32_t pc, uint8_t rd, uint8_t funct3, uint8_t rs1, int32_t immI, FILE* saida, uint32_t* pc_ptr) {
    
    uint32_t csr_addr = (uint32_t)immI & 0xFFF;

    if (funct3 == 0b000) {
        // Controle de Fluxo
        switch (csr_addr) {
            case 0b000000000000: { //ecall
                fprintf(saida, "ecall\n");
                trap_capture(11, 0, pc_ptr, saida);
                break;
            }
            case 0b001100000010: { //mret
                fprintf(saida, "mret                       pc=0x%08x\n", mepc);
                trap_return(pc_ptr, saida);
                break;
            }
            case 0b000000000001: { //ebreak
                fprintf(saida, "ebreak\n");
                run = 0;
                break;
            }
            default: {
                trap_capture(2, instrucao, pc_ptr, saida);
                break; 
            }
        }
    } else {
        // Instruções de CSR 
        uint32_t prev_csr = read_csr(csr_addr);
        uint32_t pos_csr = prev_csr;
        char nomeCsr[10];
        strcpy(nomeCsr, nome_csr(csr_addr));
        uint32_t ext_rs1 = (uint32_t)rs1;
        switch (funct3) {
            case 0b001: // csrrw: 
                pos_csr = x[rs1];
                fprintf(saida, "csrrw  %s,%s,%s     %s=%s=0x%08x,%s=%s=0x%08x\n",
                    nomex[rd], nomeCsr, nomex[rs1],
                    nomex[rd], nomeCsr, prev_csr, 
                    nomeCsr, nomex[rs1], pos_csr);
                break;
            case 0b010: // csrrs:
                pos_csr = prev_csr | x[rs1];
                fprintf(saida, "csrrs  %s,%s,%s     %s=%s=0x%08x,%s|=%s=0x%08x|0x%08x=0x%08x\n",
                        nomex[rd], nomeCsr, nomex[rs1],
                        nomex[rd], nomeCsr, prev_csr,
                        nomeCsr, nomex[rs1], prev_csr, x[rs1], pos_csr);
                break;
            case 0b011: // csrrc: 
                pos_csr = prev_csr & ~x[rs1];
                fprintf(saida, "csrrc  %s,%s,%s     %s=%s=0x%08x,%s&~=%s=0x%08x&~0x%08x=0x%08x\n",
                    nomex[rd], nomeCsr, nomex[rs1],
                    nomex[rd], nomeCsr, prev_csr, 
                    nomeCsr, nomex[rs1], prev_csr, x[rs1], pos_csr);
                break;
            case 0b101: // csrrwi: 
                pos_csr = ext_rs1;
                fprintf(saida, "csrrwi  %s,%s,%s     %s=%s=0x%08x,%s=%u=0x%08x\n",
                    nomex[rd], nomeCsr, nomex[rs1],
                    nomex[rd], nomeCsr, prev_csr, 
                    nomeCsr, ext_rs1, pos_csr);
                break;
            case 0b110: // csrrsi: 
                pos_csr = prev_csr | ext_rs1;
                fprintf(saida, "csrrsi  %s,%s,%s     %s=%s=0x%08x,%s|=%u=0x%08x|0x%08x=0x%08x\n",
                    nomex[rd], nomeCsr, nomex[rs1],
                    nomex[rd], nomeCsr, prev_csr, 
                    nomeCsr, ext_rs1, prev_csr, ext_rs1, pos_csr);
                break;
            case 0b111: // csrrci: 
                pos_csr = prev_csr & ~ext_rs1;
                fprintf(saida, "csrrci  %s,%s,%s     %s=%s=0x%08x,%s&~=%u=0x%08x&~0x%08x=0x%08x\n",
                    nomex[rd], nomeCsr, nomex[rs1],
                    nomex[rd], nomeCsr, prev_csr, 
                    nomeCsr, ext_rs1, prev_csr, ext_rs1, pos_csr);
                break;
            default:{
                trap_capture(2, instrucao, pc_ptr, saida);
                return;
            }
        }
        write_csr(csr_addr, pos_csr); 
    }
}

/* POXIM V1 */

void load_entrada(FILE *entrada, uint8_t* mem){
    char token[16];
    uint32_t curr = 0; //endereço atual 
    while (fscanf(entrada, "%s", token) == 1) {
        if (token[0] == '@') sscanf(token + 1, "%x", &curr); // leitura formatada
        else {
            uint8_t val;
            sscanf(token, "%hhx", &val); // hhx - hexadecimal de 8bits(1byte)
            mem[curr - offset] = val;
            curr++;
        }
    }
}

void S_type(uint32_t instrucao, int16_t imm, uint8_t rs1, uint8_t rs2, uint8_t funct3, FILE* saida, uint8_t* mem, uint32_t*pc_ptr){ //0100011
    uint32_t endereco = x[rs1] + (int32_t)imm; 
    uint32_t indx = endereco - offset;
    int32_t dado = x[rs2];

    if (endereco < offset || indx >= 32*1024) {
        trap_capture( 7, endereco,  pc_ptr, saida);
        return;
    }

    switch (funct3){
        case 0x0: /*Store Byte*/{
            mem[indx] = (uint8_t)(dado & 0xFF);
            fprintf(saida, "sb     %s,0x%03x(%s)  mem[0x%08x]=0x%02x\n",
                nomex[rs2], (imm & 0xFFF), nomex[rs1],
                endereco, (dado & 0xFF));
            break;
        }
        case 0x1: /*Store Half*/{
            *(uint16_t*)&mem[indx] = (uint16_t)(dado & 0xFFFF);
            fprintf(saida, "sh     %s,0x%03x(%s)  mem[0x%08x]=0x%04x\n",
                nomex[rs2], (imm & 0xFFF), nomex[rs1],
                endereco, (dado & 0xFFFF));
            break;
        }
        case 0x2: /*Store Word */{
            *(uint32_t*)&mem[indx] = (uint32_t)dado;
            fprintf(saida, "sw     %s,0x%03x(%s)  mem[0x%08x]=0x%08x\n",
                nomex[rs2], (imm & 0xFFF), nomex[rs1],
                endereco, dado);
            break;
        }
        default:
            trap_capture(2, instrucao, pc_ptr, saida);
    }
}

void I_type_imm(uint32_t instrucao, int32_t imm, uint8_t rs1, uint8_t funct3, uint8_t rd, FILE* saida, uint8_t* mem, uint32_t*pc_ptr){ //0010011
    uint8_t funct7 = (imm >> 5) & 0b1111111;
    uint8_t shamt = imm & 0b11111;
    int32_t prev_rs1 = x[rs1];

    switch (funct3){
        case 0x0:/*ADD Immediate*/{
            x[rd] = x[rs1] + imm;
            fprintf(saida,"addi   %s,%s,0x%03x   %s=0x%08x+0x%08x=0x%08x\n",
                nomex[rd], nomex[rs1], (imm & 0xFFF),
                nomex[rd], prev_rs1, imm, x[rd]);
            break;
        }
        case 0x1:/*Shift Left Logical Imm */{ 
            x[rd] = (uint32_t)x[rs1] << shamt;
            fprintf(saida,"slli   %s,%s,%u      %s=0x%08x<<%u=0x%08x\n",
                nomex[rd], nomex[rs1], shamt,
                nomex[rd], prev_rs1, shamt, x[rd]);
            break;
        }
        case 0x2:/*Set Less Than Imm*/{
            x[rd] = (x[rs1] < imm)?1:0; // com sinal
            fprintf(saida,"slti   %s,%s,0x%03x   %s=(0x%08x<0x%08x)=%u\n",
                nomex[rd], nomex[rs1], (imm & 0xFFF),
                nomex[rd], prev_rs1, imm, x[rd]);
            break;
        }
        case 0x3:/*Set Less Than Imm (U)*/{
            x[rd] = ((uint32_t)x[rs1] < (uint32_t)imm)?1:0;
            fprintf(saida,"sltiu  %s,%s,0x%03x   %s=(0x%08x<0x%08x)=%u\n",
                nomex[rd], nomex[rs1], (imm & 0xFFF),
                nomex[rd], prev_rs1, imm, x[rd]);
            break;
        }
        case 0x4: /*XOR Immediate */{
            x[rd] = x[rs1] ^ imm;
            fprintf(saida,"xori   %s,%s,0x%03x   %s=0x%08x^0x%08x=0x%08x\n",
                nomex[rd], nomex[rs1], (uint32_t)(imm & 0xFFF),
                nomex[rd], prev_rs1, imm, x[rd]);
            break;
        }
        case 0x5: /*Shift Right Imm*/{
            if(funct7 == 0x00){ //Shift Right Logical Imm(srli)
                x[rd] = (uint32_t)x[rs1] >> shamt;
                fprintf(saida,"srli   %s,%s,%u      %s=0x%08x>>%u=0x%08x\n",
                nomex[rd], nomex[rs1], shamt,
                nomex[rd], prev_rs1, shamt, x[rd]);

            }else if(funct7 == 0x20){ //Shift Right Arith Imm
                x[rd] = x[rs1] >> shamt; // com sinal
                fprintf(saida,"srai   %s,%s,%u      %s=0x%08x>>>%u=0x%08x\n",
                nomex[rd], nomex[rs1], shamt,
                nomex[rd], prev_rs1, shamt, x[rd]);
            }else{
                trap_capture(2, instrucao, pc_ptr, saida);
            }
            break;
        }
        case 0x6: /*OR Immediate*/{
            x[rd] = x[rs1] | imm;
            fprintf(saida, "ori    %s,%s,0x%03x   %s=0x%08x|0x%08x=0x%08x\n",
                nomex[rd], nomex[rs1], (imm & 0xFFF),
                nomex[rd], prev_rs1, imm, x[rd]);
            break;
        }
        case 0x7: /*AND Immediate*/ {
            x[rd] = x[rs1] & imm;
            fprintf(saida, "andi   %s,%s,0x%03x   %s=0x%08x&0x%08x=0x%08x\n",
                nomex[rd], nomex[rs1], (imm & 0xFFF),
                nomex[rd], prev_rs1, imm, x[rd]);
                break;
        }
        default:
            trap_capture(2, instrucao, pc_ptr, saida);

    }
}

void I_type_load(uint32_t instrucao, int16_t imm, uint8_t rs1, uint8_t funct3, uint8_t rd, FILE* saida, uint8_t* mem, uint32_t *pc_ptr) { //0000011

    uint32_t endereco = x[rs1] + (int32_t)imm; 
    uint32_t indx = endereco - offset;

    if (endereco < offset || indx >= 32*1024) {
        trap_capture( 5, endereco,  pc_ptr, saida);
        return;
    }

    switch (funct3){
        case 0x0: /*Load Byte (lb) - extensão de sinal*/ {
            x[rd] = (int32_t)(int8_t)mem[indx]; //mem é unsigned
            fprintf(saida, "lb     %s,0x%03x(%s)  %s=mem[0x%08x]=0x%08x\n",
                    nomex[rd], imm & 0xFFF, nomex[rs1],
                    nomex[rd], endereco, (uint32_t)x[rd]);
            break;
        }
        case 0x1: /*Load Half (lh) - extensão de sinal*/ {
            int16_t hword = *((int16_t*)&mem[indx]); // n tenho garantia q indx é alinhado
            x[rd] = (int32_t)hword;
            fprintf(saida, "lh     %s,0x%03x(%s)  %s=mem[0x%08x]=0x%08x\n",
                    nomex[rd], imm & 0xFFF, nomex[rs1],
                    nomex[rd], endereco, (uint32_t)x[rd]);
            break;
        }
        case 0x2: /*Load Word (lw) - sem extensão*/{
            x[rd] = *(int32_t*)&mem[indx];
            fprintf(saida,"lw     %s,0x%03x(%s)  %s=mem[0x%08x]=0x%08x\n",
                nomex[rd], imm & 0xFFF, nomex[rs1],
                nomex[rd], endereco, (uint32_t)x[rd]);
            break;
        }
        case 0x4: /*Load Byte (U) (lbu) - extensão por 0*/{
            x[rd] = (int32_t)mem[indx];
            fprintf(saida, "lbu    %s,0x%03x(%s)  %s=mem[0x%08x]=0x%08x\n",
                nomex[rd], imm & 0xFFF, nomex[rs1],
                nomex[rd], endereco, (uint32_t)x[rd]);
            break;
        }
        case 0x5: /*Load Half (U) (lhu) - extensão por 0*/{
            uint16_t hword = *(uint16_t*)&mem[indx];
            x[rd] = (int32_t)hword;
            fprintf(saida, "lhu    %s,0x%03x(%s)  %s=mem[0x%08x]=0x%08x\n",
                nomex[rd], imm & 0xFFF, nomex[rs1],
                nomex[rd], endereco, (uint32_t)x[rd]);
        }
        default:
            trap_capture(2, instrucao, pc_ptr, saida);
    }

}

void R_type(uint32_t instrucao, uint8_t funct7, uint8_t rs1, uint8_t rs2, uint8_t funct3,uint8_t rd, FILE* saida,  uint32_t *pc_ptr){ //0110011
    int32_t prev_rs1 = x[rs1], prev_rs2 = x[rs2];
    switch (funct3){
        case 0x0: /*add, sub, mul*/{
            if(funct7 == 0x00){ // add
                fprintf(saida,"add    %s,%s,%s     %s=0x%08x+0x%08x=0x%08x\n", 
                    nomex[rd], nomex[rs1], nomex[rs2], nomex[rd], 
                    x[rs1], x[rs2], x[rs1] + x[rs2]);                
                x[rd] = x[rs1] + x[rs2];

            }else if(funct7 == 0x20) { // sub
                fprintf(saida,"sub    %s,%s,%s     %s=0x%08x-0x%08x=0x%08x\n", 
                    nomex[rd], nomex[rs1], nomex[rs2], nomex[rd], 
                    x[rs1], x[rs2], x[rs1] - x[rs2]);                
                x[rd] = x[rs1] - x[rs2];

            }else if(funct7 == 0x01){ //mul
                x[rd] = (((int64_t)x[rs1] * (int64_t)x[rs2]) & 0xFFFFFFFF);
                fprintf(saida,"mul    %s,%s,%s     %s=0x%08x*0x%08x=0x%08x\n", 
                    nomex[rd], nomex[rs1], nomex[rs2], nomex[rd], 
                    prev_rs1, prev_rs2, x[rd]);    
            }else{
                trap_capture(2, instrucao, pc_ptr, saida);
            }
            break;
        }
        case 0x1: /*Shift Left Logical, (Multiplicação - Metade Superior com Sinal)*/ {
            if(funct7 == 0x00){ //sll
                uint32_t shamt = x[rs2] & 0b11111;
                x[rd] = (uint32_t)x[rs1] << shamt;
                fprintf(saida,"sll    %s,%s,%s     %s=0x%08x<<%u=0x%08x\n", 
                    nomex[rd], nomex[rs1], nomex[rs2], nomex[rd], 
                    prev_rs1, shamt, x[rd]);
            }else if(funct7 == 0x01){ //mulh
                int64_t res = (int64_t)x[rs1] * (int64_t)x[rs2];
                x[rd] = (res >> 32);
                fprintf(saida,"mulh   %s,%s,%s     %s=0x%08x*0x%08x=0x%08x\n",
                       nomex[rd], nomex[rs1], nomex[rs2],
                       nomex[rd], prev_rs1, prev_rs2, x[rd]);
            }else{

            }
            break;
        }
        case 0x2: /*Set Less Than, (Multiplicação - Metade Superior com Sinal/Sem Sinal)*/ {
            if(funct7 == 0x00){ // slt
                x[rd] = (x[rs1] < x[rs2])?1:0;
                fprintf(saida,"slt    %s,%s,%s     %s=(0x%08x<0x%08x)=%u\n", 
                        nomex[rd], nomex[rs1], nomex[rs2], nomex[rd], 
                        prev_rs1, prev_rs2, x[rd]);
            }else if(funct7 == 0x01){ //mulhsu
                int64_t res = (int64_t)(int32_t)prev_rs1* (uint64_t)(uint32_t)prev_rs2;
                x[rd] = (int32_t)(res >> 32);
                fprintf(saida,"mulhsu %s,%s,%s     %s=0x%08x*0x%08x=0x%08x\n",
                       nomex[rd], nomex[rs1], nomex[rs2],
                       nomex[rd], prev_rs1, prev_rs2, x[rd]);
            }else{
                trap_capture(2, instrucao, pc_ptr, saida);
            }
            break;
        }
        case 0x3: /* Set Less Than (U) - sltu, */{
            if(funct7 == 0x00){ //sltu
                x[rd] = ((uint32_t)x[rs1] < (uint32_t)x[rs2])?1:0;
                fprintf(saida,"sltu   %s,%s,%s     %s=(0x%08x<0x%08x)=%u\n",
                    nomex[rd], nomex[rs1], nomex[rs2],
                    nomex[rd], prev_rs1, prev_rs2, x[rd]);                
            }else if(funct7 == 0x01){ //mulhu
                uint64_t res = (uint64_t)(uint32_t)x[rs1] * (uint64_t)(uint32_t)x[rs2];
                uint32_t high = (uint32_t)(res >> 32);
                x[rd] = (int32_t)high;
                fprintf(saida,"mulhu  %s,%s,%s     %s=0x%08x*0x%08x=0x%08x\n",
                       nomex[rd], nomex[rs1], nomex[rs2],
                       nomex[rd], prev_rs1, prev_rs2, x[rd]);
            }else{
                trap_capture(2, instrucao, pc_ptr, saida);
            }

            break;
        }
        case 0x4: /*Xor, Divisão com Sinal*/{
            if(funct7 == 0x00){ //xor
                x[rd] = x[rs1]^x[rs2];
                fprintf(saida,"xor    %s,%s,%s     %s=0x%08x^0x%08x=0x%08x\n",
                        nomex[rd], nomex[rs1], nomex[rs2], nomex[rd], 
                        prev_rs1, prev_rs2, x[rd]);                
            }else if(funct7 == 0x01){ //div
                if (x[rs2] == 0) x[rd] = -1; 
                else x[rd] = (int32_t)x[rs1] / (int32_t)x[rs2];
                fprintf(saida,"div    %s,%s,%s     %s=0x%08x/0x%08x=0x%08x\n",
                       nomex[rd], nomex[rs1], nomex[rs2],
                       nomex[rd], prev_rs1, prev_rs2, x[rd]);
            }else{
                trap_capture(2, instrucao, pc_ptr, saida);
            }

            break;  
        }
        case 0x5: /*Shift Right, Divisão Sem Sinal*/{
            uint32_t shamt = x[rs2] & 0b11111;
            if(funct7 == 0x00){ // Shift Right Logical (srl)
                x[rd] = (uint32_t)x[rs1] >> shamt;
                fprintf(saida,"srl    %s,%s,%s     %s=0x%08x>>%u=0x%08x\n", 
                    nomex[rd], nomex[rs1], nomex[rs2],
                    nomex[rd], prev_rs1, shamt, x[rd]);
            }else if (funct7 == 0x20){ //Shift Right Arithmetic (sra) - considera o sinal
                x[rd] = (int32_t)x[rs1] >> shamt;
                fprintf(saida,"sra    %s,%s,%s     %s=0x%08x>>>%u=0x%08x\n", 
                    nomex[rd], nomex[rs1], nomex[rs2],nomex[rd], 
                    prev_rs1, shamt, x[rd]);
            }else if(funct7 == 0x01){ //divu
                if (x[rs2] == 0) x[rd] = -1;
                else x[rd] = (uint32_t)x[rs1] / (uint32_t)x[rs2];
                fprintf(saida,"divu   %s,%s,%s     %s=0x%08x/0x%08x=0x%08x\n",
                       nomex[rd], nomex[rs1], nomex[rs2],
                       nomex[rd], prev_rs1, prev_rs2, x[rd]);
            }else{
                trap_capture(2, instrucao, pc_ptr, saida);
            }
            break;
        }
        case 0x6: /*Or, Resto com Sinal*/{
            if(funct7 == 0x00){
                x[rd] = x[rs1] | x[rs2];
                fprintf(saida,"or     %s,%s,%s     %s=0x%08x|0x%08x=0x%08x\n",
                    nomex[rd], nomex[rs1], nomex[rs2],nomex[rd],
                    prev_rs1, prev_rs2, x[rd]);
            }else if(funct7){ //rem
                if (x[rs2] == 0) x[rd] = x[rs1]; 
                else x[rd] = (int32_t)x[rs1] % (int32_t)x[rs2];
                fprintf(saida,"rem    %s,%s,%s     %s=0x%08x%%0x%08x=0x%08x\n",
                       nomex[rd], nomex[rs1], nomex[rs2],
                       nomex[rd], prev_rs1, prev_rs2, x[rd]);
            }else{
                trap_capture(2, instrucao, pc_ptr, saida);
            }
            break;
        }
        case 0x7: /*And, Resto sem Sinal*/ {
            if(funct7 == 0x00){ //and
                x[rd] = x[rs1] & x[rs2];
                fprintf(saida,"and    %s,%s,%s     %s=0x%08x&0x%08x=0x%08x\n",
                    nomex[rd], nomex[rs1], nomex[rs2],nomex[rd],
                    prev_rs1, prev_rs2, x[rd]);
            }else if(funct7 == 0x01){ // remu
                if (x[rs2] == 0) x[rd] = x[rs1];
                else x[rd] = (uint32_t)x[rs1] % (uint32_t)x[rs2];
                fprintf(saida,"remu   %s,%s,%s     %s=0x%08x%%0x%08x=0x%08x\n",
                       nomex[rd], nomex[rs1], nomex[rs2],
                       nomex[rd], prev_rs1, prev_rs2, x[rd]);
            }else{
                trap_capture(2, instrucao, pc_ptr, saida);
            }
            break; 
        }
        default:
            trap_capture(2, instrucao, pc_ptr, saida);
    }
}

void B_type(uint32_t instrucao, int32_t imm, uint8_t rs1, uint8_t rs2, uint8_t funct3, FILE* saida, uint32_t *pc){ //0110011
    uint8_t flg = 0;

    switch (funct3){
        case 0x0: /*Branch ==(beq) */ {
            flg = (x[rs1] == x[rs2]);
            fprintf(saida,"beq    %s,%s,0x%03x  (0x%08x==0x%08x)=%u->pc=0x%08x\n",
                nomex[rs1], nomex[rs2], (imm >> 1)&0xFFF,
                x[rs1], x[rs2], flg, *pc + (flg ? imm : 4));

            break;
        }
        case 0x1: /*Branch != (bne)*/ {
            flg = (x[rs1] != x[rs2]);
            fprintf(saida, "bne    %s,%s,0x%03x  (0x%08x!=0x%08x)=%u->pc=0x%08x\n",
                nomex[rs1], nomex[rs2], (imm >> 1) & 0xFFF,
                x[rs1], x[rs2], flg, *pc + (flg ? imm : 4));

            break;
        }
        case 0x4: /*Branch < (blt)*/ {
            flg = (x[rs1] < x[rs2]);
            fprintf(saida, "blt    %s,%s,0x%03x  (0x%08x<0x%08x)=%u->pc=0x%08x\n",
                nomex[rs1], nomex[rs2], (imm >> 1) & 0xFFF,
                x[rs1], x[rs2], flg, *pc + (flg ? imm : 4));

            break;
        }
        case 0x5: /*Branch >= (bge)*/ {
            flg = (x[rs1] >= x[rs2]);
            fprintf(saida, "bge    %s,%s,0x%03x  (0x%08x>=0x%08x)=%u->pc=0x%08x\n",
                nomex[rs1], nomex[rs2], (imm >> 1) & 0xFFF,
                x[rs1], x[rs2], flg, *pc + (flg ? imm : 4));
            break;
        }
        case 0x6: /*Branch < (U) (bltu)*/ {
            flg = ((uint32_t)x[rs1] < (uint32_t)x[rs2]);
            fprintf(saida,"bltu   %s,%s,0x%03x  (0x%08x<0x%08x)=%u->pc=0x%08x\n",
                nomex[rs1], nomex[rs2],(imm >> 1) & 0xFFF,
                x[rs1], x[rs2], flg, *pc + (flg ? imm : 4));
            break;
        }
        case 0x7: /*Branch >= (U) (bgeu)*/ {
            flg = ((uint32_t)x[rs1] >= (uint32_t)x[rs2]);
            fprintf(saida, "bgeu   %s,%s,0x%03x  (0x%08x>=0x%08x)=%u->pc=0x%08x\n",
                nomex[rs1], nomex[rs2], (imm >> 1) & 0xFFF,
                x[rs1], x[rs2], flg, *pc + (flg ? imm : 4));
            break;
        }
        default:
            trap_capture(2, instrucao, pc, saida);
    }
    if(flg) *pc += imm - 4;
}

int main (int argc, char *argv[]){
    if(argc < 3){
        puts("Erro na linha de comando: \nDevem haver 3 argumentos (nome do programa, arquivo de entrada e arquivo de saída)");
        return 1;
    }

    FILE *entrada = fopen(argv[1], "rb");
    if (!entrada) {
        perror("Erro ao abrir arquivo de entrada");
        return 1;
    }

    FILE *saida = fopen(argv[2], "w");
    if (!saida) {
        perror("Erro ao abrir arquivo de saída");
        return 1;
    }

    uint8_t* mem = (uint8_t*)(malloc(32 * 1024)); //32 Kib
    load_entrada(entrada, mem);
    fclose(entrada);

    uint32_t pc = offset;

    while(run){
        if ((pc % 4 != 0) || (pc < offset) || (pc >= offset + 32*1024)) {
            trap_capture(1, pc, &pc, saida);
            pc += 4; 
            continue; 
        }
        uint32_t instrucao = ((uint32_t*)mem)[(pc - offset) >> 2];
        uint8_t opcode = instrucao & 0b1111111;              // bits 6:0
        uint8_t rd     = (instrucao >> 7) & 0b11111;         // bits 11:7
        uint8_t funct3 = (instrucao >> 12) & 0b111;          // bits 14:12
        uint8_t rs1    = (instrucao >> 15) & 0b11111;        // bits 19:15
        uint8_t rs2    = (instrucao >> 20) & 0b11111;        // bits 24:20
        uint8_t funct7 = (instrucao >> 25) & 0b1111111;      // bits 31:25
        int32_t immI = ((int32_t)instrucao >> 20);
        uint32_t immU = (instrucao >> 12);
        fprintf(saida, "0x%08x:", pc); 
        
        switch (opcode) {
            case 0b0110011:{ // R - type
                R_type(instrucao, funct7, rs1, rs2, funct3, rd, saida, &pc);
                break;
            }
            case 0b0000011:{ // I - type (load)
                I_type_load(instrucao, immI, rs1, funct3, rd, saida, mem, &pc);
                break;
            }
            case 0b0100011:{ // S - type (store)
                int16_t immS = (funct7 << 5) | rd;
                if (immS & 0x800) immS |= 0xF000;
                S_type(instrucao, immS, rs1, rs2, funct3, saida, mem, &pc);
                break;
            }
            case 0b0010011:{ // I - type (imm)
                I_type_imm(instrucao, immI, rs1, funct3, rd, saida, mem, &pc);
                break;
            }
            case 0b1100011:{ //B-type
                int32_t immB = 0; 
                immB |= ((rd >> 1) & 0xF) << 1;          // bits 1–4  
                immB |= (funct7 & 0x3F) << 5;            // bits 5–10 
                immB |= (rd & 0x1) << 11;                // bit 11    
                immB |= ((instrucao >> 31) & 0x1) << 12; // bit 12  
                if (immB & 0x1000) immB |= 0xFFFFE000;   // extende o sinal se o bit 12 tiver setado
                
                B_type(instrucao, immB, rs1, rs2, funct3, saida, &pc);
                break;
            }
            case 0b0110111:{ //lui U-type
                x[rd] = (immU << 12);
                fprintf(saida, "lui    %s,0x%05x     %s=0x%05x000\n",
                        nomex[rd], (immU & 0xFFFFF), nomex[rd], (immU & 0xFFFFF));
                break;
            }
            case 0b0010111:{ //auipc U-type
                x[rd] = pc + (immU << 12);
                fprintf(saida, "auipc  %s,0x%05x     %s=0x%08x+0x%05x000=0x%08x\n",
                        nomex[rd], (immU & 0xFFFFF), nomex[rd],pc, (immU & 0xFFFFF), x[rd]);
                break;
            }
            case 0b1100111:{ // jalr -> TIPO I
                uint32_t novo_pc = (x[rs1] + (int32_t)immI);
                fprintf(saida, "jalr   %s,%s,0x%03x   pc=0x%08x+0x%08x,%s=0x%08x\n",
                        nomex[rd], nomex[rs1], immI, x[rs1], (int32_t)immI, nomex[rd], pc + 4);
                x[rd] = pc + 4;
                pc = novo_pc - 4;
                break;
            }
            case 0b1101111:{ //jal
                int32_t immJ = 0;
                immJ |= ((instrucao >> 21) & 0x3FF) << 1; // bits 1 -> 10 (10 bits)
                immJ |= ((instrucao >> 20) & 0x1) << 11;  // bit 11
                immJ |= (instrucao & 0xFF000);            // bits 12 - 19 (8 bits)
                immJ |= (instrucao >> 31) << 20;          // bit 20
                if (immJ & 0x100000) immJ |= 0xFFE00000;

                x[rd] = pc + 4;
                uint32_t novo_pc = pc + immJ;

                fprintf(saida, "jal    %s,0x%05x     pc=0x%08x,%s=0x%08x\n", 
                        nomex[rd], ((immJ >> 1) & 0xFFFFF), novo_pc, nomex[rd], x[rd]);
                pc = novo_pc - 4;
                break;
            }
            case 0b1110011: { // instruções csr, mnret, ecall, ebreak;
                CSR_Fluxo(instrucao, pc, rd, funct3, rs1, immI, saida, &pc);
                break;
            }
            default:{
                trap_capture(2, instrucao, &pc, saida);
            }
        }
        x[0] = 0;
        pc += 4;
    }
    fclose(saida);
}