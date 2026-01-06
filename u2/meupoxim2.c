#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>


#define RAM_INF 0x80000000
#define RAM_SUP 0x80008000

#define CLINT_INF  0x02000000
#define CLINT_SUP  0x020c0000 

#define PLIC_INF  0x0c000000
#define PLIC_SUP  0x0c200004

#define UART_INF   0x10000000 
#define UART_SUP   0x10000005  

#define RST_ISR  0b00000001
#define ADDRS_ISR  0x10000002
#define RST_LSR  0b01100000
#define ADDRS_LSR  0x10000005

#define MTIME_HIGH   0x0200BFFC 
#define MTIME_LOW   0x0200BFF8  
#define MTIMECMP_HIGH   0x02004004  
#define MTIMECMP_LOW   0x02004000  

int32_t x[32] = {0};
const char* nomex[32] = { "zero", "ra", "sp", "gp", "tp", "t0", "t1", "t2", "s0", "s1", "a0", "a1", "a2", 
                            "a3", "a4", "a5", "a6", "a7", "s2", "s3", "s4", "s5", "s6", "s7", "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6"};
const uint32_t offset[4] = {
    RAM_INF,
    CLINT_INF - (RAM_SUP - RAM_INF + 1),
    PLIC_INF - (RAM_SUP - RAM_INF + CLINT_SUP - CLINT_INF + 2),
    UART_INF - (RAM_SUP - RAM_INF + CLINT_SUP - CLINT_INF + PLIC_SUP - PLIC_INF + 3)
};

uint8_t run = 1;
uint32_t pc = 0;

size_t tam_end =
    (RAM_SUP   - RAM_INF   + 1) +
    (CLINT_SUP - CLINT_INF + 1) +
    (PLIC_SUP  - PLIC_INF  + 1) +
    (UART_SUP  - UART_INF  + 1);

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

const uint32_t CLEAR_MASK = ((1 << 3) | (1 << 7));
const uint32_t mode = ((1 << 11) | (1 << 12));

void trap_capture(uint32_t cause, uint32_t tval, FILE *saida){
    mcause = cause;        
    mtval = tval;
    mepc = pc;

    uint32_t mstatus_old = mstatus;
    uint32_t mie_bit = (mstatus_old >> 3) & 1;
    mstatus &= ~CLEAR_MASK;   
    mstatus |= (mie_bit << 7); // MPIE recebe MIE
    mstatus |= mode;

     if ((mcause & 0x80000000U) != 0) {
        uint32_t code = mcause & 0x7FFFFFFF;

        fprintf(saida, ">interrupt:");
        switch (code) {
            case 3:  fprintf(saida, "software        "); break;
            case 7:  fprintf(saida, "timer           "); break;
            case 11: fprintf(saida, "external        "); break;
            default: fprintf(saida, "unknown         "); break;
        }
    }else{
        fprintf(saida, ">exception:");
        switch (mcause) {
            case 1: fprintf(saida, "instruction_fault         "); break;
            case 2: fprintf(saida, "illegal_instruction       "); break;
            case 5: fprintf(saida, "load_fault                "); break;
            case 7: fprintf(saida, "store_fault               "); break;
            case 11: fprintf(saida, "environment_call         "); break;
            default: fprintf(saida, "unkown                   "); break;
        }
    }
    fprintf(saida,"cause=0x%08x,epc=0x%08x,tval=0x%08x\n",mcause, mepc, mtval);


    uint32_t base = mtvec & 0xFFFFFFFC;
    uint32_t mtvec_mode = mtvec & 0x00000003;

    if (mtvec_mode == 1 && (mcause & 0x80000000U)) { 
        // MODO VETORIZADO: Ativado se mode for 1 E for uma interrupção (bit 31 setado)
        uint32_t code = mcause & 0x7FFFFFFF;
        pc = (base + (code * 4)) - 4; 
    } else pc = base - 4;
}

void trap_return(){
    uint32_t mpie = (mstatus >> 7) & 1;
    mstatus &= ~CLEAR_MASK;
    mstatus |= (mpie << 3);
    mstatus |= (1 << 7);
    mstatus &= ~mode; 

    pc = mepc - 4;
}

void CSR_Fluxo(uint32_t instrucao, uint8_t rd, uint8_t funct3, uint8_t rs1, int32_t immI, FILE* saida) {
    
    uint32_t csr_addr = (uint32_t)immI & 0xFFF;

    if (funct3 == 0b000) {
        // Controle de Fluxo
        switch (csr_addr) {
            case 0b000000000000: { //ecall
                fprintf(saida, "0x%08x:ecall\n", pc);
                trap_capture(11, 0, saida);
                break;
            }
            case 0b001100000010: { //mret
                fprintf(saida, "0x%08x:mret                       pc=0x%08x\n", pc, mepc);
                trap_return();
                break;
            }
            case 0b000000000001: { //ebreak
                fprintf(saida, "0x%08x:ebreak\n", pc);
                run = 0;
                break;
            }
            default: {
                trap_capture(2, instrucao, saida);
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

        x[rd] = prev_csr;

        switch (funct3) {
            case 0b001: // csrrw: 
                pos_csr = x[rs1];
                fprintf(saida, "0x%08x:csrrw  %s,%s,%s     %s=%s=0x%08x,%s=%s=0x%08x\n",
                    pc, nomex[rd], nomeCsr, nomex[rs1],
                    nomex[rd], nomeCsr, prev_csr, 
                    nomeCsr, nomex[rs1], pos_csr);
                break;
            case 0b010: // csrrs:
                pos_csr = prev_csr | x[rs1];
                fprintf(saida, "0x%08x:csrrs  %s,%s,%s      %s=%s=0x%08x,%s|=%s=0x%08x|0x%08x=0x%08x\n",
                        pc, nomex[rd], nomeCsr, nomex[rs1],
                        nomex[rd], nomeCsr, prev_csr,
                        nomeCsr, nomex[rs1], prev_csr, x[rs1], pos_csr);
                break;
            case 0b011: // csrrc: 
                pos_csr = prev_csr & ~x[rs1];
                fprintf(saida, "0x%08x:csrrc  %s,%s,%s      %s=%s=0x%08x,%s&~=%s=0x%08x&~0x%08x=0x%08x\n",
                    pc, nomex[rd], nomeCsr, nomex[rs1],
                    nomex[rd], nomeCsr, prev_csr, 
                    nomeCsr, nomex[rs1], prev_csr, x[rs1], pos_csr);
                break;
            case 0b101: // csrrwi: 
                pos_csr = ext_rs1;
                fprintf(saida, "0x%08x:csrrwi  %s,%s,%s     %s=%s=0x%08x,%s=%u=0x%08x\n",
                    pc, nomex[rd], nomeCsr, nomex[rs1],
                    nomex[rd], nomeCsr, prev_csr, 
                    nomeCsr, ext_rs1, pos_csr);
                break;
            case 0b110: // csrrsi: 
                pos_csr = prev_csr | ext_rs1;
                fprintf(saida, "0x%08x:csrrsi  %s,%s,%s     %s=%s=0x%08x,%s|=%u=0x%08x|0x%08x=0x%08x\n",
                    pc, nomex[rd], nomeCsr, nomex[rs1],
                    nomex[rd], nomeCsr, prev_csr, 
                    nomeCsr, ext_rs1, prev_csr, ext_rs1, pos_csr);
                break;
            case 0b111: // csrrci: 
                pos_csr = prev_csr & ~ext_rs1;
                fprintf(saida, "0x%08x:csrrci  %s,%s,%s     %s=%s=0x%08x,%s&~=%u=0x%08x&~0x%08x=0x%08x\n",
                    pc, nomex[rd], nomeCsr, nomex[rs1],
                    nomex[rd], nomeCsr, prev_csr, 
                    nomeCsr, ext_rs1, prev_csr, ext_rs1, pos_csr);
                break;
            default:{
                trap_capture(2, instrucao, saida);
                return;
            }
        }
        
        if (rd != 0) {
            x[rd] = prev_csr;
        }
        write_csr(csr_addr, pos_csr);
    }
}

int check_int(uint32_t *mask, uint8_t ISR, uint8_t IER) {

    if ((mstatus & (1 << 3)) == 0) return 0;

    if((ISR & 0b1111) == (1 << 2) && (IER & 1)){ // Received Data Ready
        mip |= (1 << 11);
    }else if((ISR & 0b1111) == (1 << 1) && (IER & 2)){ // Transmitter Holding Register Empty
        mip |= (1 << 11);
    }else{
        mip &= ~(1 << 11);
    }
    
    uint32_t sts = mip & mie;
    if (sts == 0) return 0; // n tem nem pendente, nem habilitada
    
    if (sts & (1 << 11)) {
        *mask = 0x80000000 | 11;  // external
        return 1;
    }
    if (sts & (1 << 3)) {
        *mask = 0x80000000 | 3;   // software
        return 1;
    }
    if (sts & (1 << 7)) {
        *mask = 0x80000000 | 7;   // timer
        return 1;
    }

    return 0;
}

int addrs_range(uint32_t addr){
    if(addr >= RAM_INF && addr <= RAM_SUP) return 0;
    if(addr >= CLINT_INF && addr <= CLINT_SUP) return 1;
    if(addr >= PLIC_INF && addr <= PLIC_SUP) return 2;
    if(addr >= UART_INF && addr <= UART_SUP) return 3;
    return -1;
}

void timer(uint8_t* mem){
    uint32_t addr_mtime_low    = MTIME_LOW - offset[1];
    uint32_t addr_mtime_high   = MTIME_HIGH - offset[1];
    uint32_t addr_mtimecmp_low = MTIMECMP_LOW - offset[1];
    uint32_t addr_mtimecmp_high= MTIMECMP_HIGH - offset[1];

    uint32_t mtime_l = *(uint32_t*)&mem[addr_mtime_low];
    uint32_t mtime_h = *(uint32_t*)&mem[addr_mtime_high];
    uint64_t mtime   = ((uint64_t)mtime_h << 32) | mtime_l;

    uint32_t mtimecmp_l = *(uint32_t*)&mem[addr_mtimecmp_low];
    uint32_t mtimecmp_h = *(uint32_t*)&mem[addr_mtimecmp_high];
    uint64_t mtimecmp   = ((uint64_t)mtimecmp_h << 32) | mtimecmp_l;

    mtime++;

    *(uint32_t*)&mem[addr_mtime_low]  = (uint32_t)(mtime & 0xFFFFFFFF);
    *(uint32_t*)&mem[addr_mtime_high] = (uint32_t)(mtime >> 32);

    if(mtime >= mtimecmp){
        mip |= (1 << 7);  
    } else {
        mip &= ~(1 << 7); 
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
            sscanf(token, "%hhx", &val);
            int cd = addrs_range(curr);
            if (cd < 0) {
                fprintf(stderr, "warning: endereço %08x fora do espaço mapeado\n", curr);
            } else {
                uint32_t idx = curr - offset[cd]; 
                mem[idx] = val;
            }
            curr++;
        }
    }
}

void S_type(uint32_t instrucao, int16_t imm, uint8_t rs1, uint8_t rs2, uint8_t funct3, FILE* saida, uint8_t* mem){ //0100011
    uint32_t endereco = x[rs1] + (int32_t)imm; 
    int32_t dado = x[rs2];


    int cd = addrs_range(endereco);

    uint32_t indx = endereco - offset[cd];

    switch (funct3){
        case 0x0: /*Store Byte*/{
            mem[indx] = (uint8_t)(dado & 0xFF);
            fprintf(saida, "0x%08x:sb     %s,0x%03x(%s)  mem[0x%08x]=0x%02x\n",
                pc, nomex[rs2], (imm & 0xFFF), nomex[rs1],
                endereco, (dado & 0xFF));
            break;
        }
        case 0x1: /*Store Half*/{
            *(uint16_t*)&mem[indx] = (uint16_t)(dado & 0xFFFF);
            fprintf(saida, "0x%08x:sh     %s,0x%03x(%s)  mem[0x%08x]=0x%04x\n",
                pc, nomex[rs2], (imm & 0xFFF), nomex[rs1],
                endereco, (dado & 0xFFFF));
            break;
        }
        case 0x2: /*Store Word */{
            *(uint32_t*)&mem[indx] = (uint32_t)dado;
            fprintf(saida, "0x%08x:sw     %s,0x%03x(%s)  mem[0x%08x]=0x%08x\n",
                pc, nomex[rs2], (imm & 0xFFF), nomex[rs1],
                endereco, dado);
            break;
        }
        default:
            trap_capture(2, instrucao, saida);
    }


    if(endereco == 0x02000000){
        if (dado != 0) {
            mip |= (1 << 3);  // Ativa interrupção de software
        } else {
            mip &= ~(1 << 3); // Limpa interrupção de software
        }
    }else if(endereco == 0x10000000){
        if(dado != 0){ // Received Data Ready
            mem[ADDRS_ISR] &= ~RST_ISR; 
            mem[ADDRS_ISR] |= (1 << 2); 

            mem[ADDRS_LSR] |= 1; // Data Ready Bit
            mem[ADDRS_LSR] &= ~RST_LSR; // Realizando uma transmissão

        }else{ // Transmitter Holding Register Empty
            mem[ADDRS_ISR] &= ~RST_ISR;
            mem[ADDRS_ISR] |= (1 << 1); 

            mem[ADDRS_LSR] &= ~RST_ISR; // Data Ready Bit 
            mem[ADDRS_LSR] = RST_LSR; // Sem dados para transmitir
        }
    }

}

void I_type_imm(uint32_t instrucao, int32_t imm, uint8_t rs1, uint8_t funct3, uint8_t rd, FILE* saida, uint8_t* mem){ //0010011
    uint8_t funct7 = (imm >> 5) & 0b1111111;
    uint8_t shamt = imm & 0b11111;
    int32_t prev_rs1 = x[rs1];

    switch (funct3){
        case 0x0:/*ADD Immediate*/{
            x[rd] = x[rs1] + imm;
            fprintf(saida,"0x%08x:addi   %s,%s,0x%03x   %s=0x%08x+0x%08x=0x%08x\n",
                    pc, nomex[rd], nomex[rs1], (imm & 0xFFF),
                    nomex[rd], prev_rs1, imm, x[rd]);
            break;
        }
        case 0x1:/*Shift Left Logical Imm */{ 
            x[rd] = (uint32_t)x[rs1] << shamt;
            fprintf(saida,"0x%08x:slli   %s,%s,%u      %s=0x%08x<<%u=0x%08x\n",
                    pc, nomex[rd], nomex[rs1], shamt,
                    nomex[rd], prev_rs1, shamt, x[rd]);
            break;
        }
        case 0x2:/*Set Less Than Imm*/{
            x[rd] = (x[rs1] < imm)?1:0; // com sinal
            fprintf(saida,"0x%08x:slti   %s,%s,0x%03x   %s=(0x%08x<0x%08x)=%u\n",
                    pc, nomex[rd], nomex[rs1], (imm & 0xFFF),
                    nomex[rd], prev_rs1, imm, x[rd]);
            break;
        }
        case 0x3:/*Set Less Than Imm (U)*/{
            x[rd] = ((uint32_t)x[rs1] < (uint32_t)imm)?1:0;
            fprintf(saida,"0x%08x:sltiu  %s,%s,0x%03x   %s=(0x%08x<0x%08x)=%u\n",
                    pc, nomex[rd], nomex[rs1], (imm & 0xFFF),
                    nomex[rd], prev_rs1, imm, x[rd]);
            break;
        }
        case 0x4: /*XOR Immediate */{
            x[rd] = x[rs1] ^ imm;
            fprintf(saida,"0x%08x:xori   %s,%s,0x%03x   %s=0x%08x^0x%08x=0x%08x\n",
                    pc, nomex[rd], nomex[rs1], (uint32_t)(imm & 0xFFF),
                    nomex[rd], prev_rs1, imm, x[rd]);
            break;
        }
        case 0x5: /*Shift Right Imm*/{
            if(funct7 == 0x00){ //Shift Right Logical Imm(srli)
                x[rd] = (uint32_t)x[rs1] >> shamt;
                fprintf(saida,"0x%08x:srli   %s,%s,%u      %s=0x%08x>>%u=0x%08x\n",
                        pc, nomex[rd], nomex[rs1], shamt,
                        nomex[rd], prev_rs1, shamt, x[rd]);

            }else if(funct7 == 0x20){ //Shift Right Arith Imm
                x[rd] = x[rs1] >> shamt; // com sinal
                fprintf(saida,"0x%08x:srai   %s,%s,%u      %s=0x%08x>>>%u=0x%08x\n",
                        pc, nomex[rd], nomex[rs1], shamt,
                        nomex[rd], prev_rs1, shamt, x[rd]);
            }else{
                trap_capture(2, instrucao, saida);
            }
            break;
        }
        case 0x6: /*OR Immediate*/{
            x[rd] = x[rs1] | imm;
            fprintf(saida, "0x%08x:ori    %s,%s,0x%03x   %s=0x%08x|0x%08x=0x%08x\n",
                    pc, nomex[rd], nomex[rs1], (imm & 0xFFF),
                    nomex[rd], prev_rs1, imm, x[rd]);
            break;
        }
        case 0x7: /*AND Immediate*/ {
            x[rd] = x[rs1] & imm;
            fprintf(saida, "0x%08x:andi   %s,%s,0x%03x   %s=0x%08x&0x%08x=0x%08x\n",
                    pc, nomex[rd], nomex[rs1], (imm & 0xFFF),
                    nomex[rd], prev_rs1, imm, x[rd]);
            break;
        }
        default:
            trap_capture(2, instrucao, saida);

    }
}

void I_type_load(uint32_t instrucao, int16_t imm, uint8_t rs1, uint8_t funct3, uint8_t rd, FILE* saida, uint8_t* mem) { //0000011

    uint32_t endereco = x[rs1] + (int32_t)imm; 
    int cd = addrs_range(endereco);
    if(cd < 0){
        trap_capture(5, endereco, saida);
        return;
    }
    uint32_t indx = endereco - offset[cd];

    switch (funct3){
        case 0x0: /*Load Byte (lb) - extensão de sinal*/ {
            x[rd] = (int32_t)(int8_t)mem[indx]; //mem é unsigned
            fprintf(saida, "0x%08x:lb     %s,0x%03x(%s)  %s=mem[0x%08x]=0x%08x\n",
                    pc, nomex[rd], imm & 0xFFF, nomex[rs1],
                    nomex[rd], endereco, (uint32_t)x[rd]);
            break;
        }
        case 0x1: /*Load Half (lh) - extensão de sinal*/ {
            int16_t hword = *((int16_t*)&mem[indx]); // n tenho garantia q indx é alinhado
            x[rd] = (int32_t)hword;
            fprintf(saida, "0x%08x:lh     %s,0x%03x(%s)  %s=mem[0x%08x]=0x%08x\n",
                    pc, nomex[rd], imm & 0xFFF, nomex[rs1],
                    nomex[rd], endereco, (uint32_t)x[rd]);
            break;
        }
        case 0x2: /*Load Word (lw) - sem extensão*/{
            x[rd] = *(int32_t*)&mem[indx];
            fprintf(saida,"0x%08x:lw     %s,0x%03x(%s)  %s=mem[0x%08x]=0x%08x\n",
                pc, nomex[rd], imm & 0xFFF, nomex[rs1],
                nomex[rd], endereco, (uint32_t)x[rd]);
            break;
        }
        case 0x4: /*Load Byte (U) (lbu) - extensão por 0*/{
            x[rd] = (int32_t)mem[indx];
            fprintf(saida, "0x%08x:lbu    %s,0x%03x(%s)  %s=mem[0x%08x]=0x%08x\n",
                pc, nomex[rd], imm & 0xFFF, nomex[rs1],
                nomex[rd], endereco, (uint32_t)x[rd]);
            break;
        }
        case 0x5: /*Load Half (U) (lhu) - extensão por 0*/{
            uint16_t hword = *(uint16_t*)&mem[indx];
            x[rd] = (int32_t)hword;
            fprintf(saida, "0x%08x:lhu    %s,0x%03x(%s)  %s=mem[0x%08x]=0x%08x\n",
                pc, nomex[rd], imm & 0xFFF, nomex[rs1],
                nomex[rd], endereco, (uint32_t)x[rd]);
            break;
        }
        default:
            trap_capture(2, instrucao, saida);
    }

}

void R_type(uint32_t instrucao, uint8_t funct7, uint8_t rs1, uint8_t rs2, uint8_t funct3,uint8_t rd, FILE* saida){ //0110011
    int32_t prev_rs1 = x[rs1], prev_rs2 = x[rs2];
    switch (funct3){
        case 0x0: /*add, sub, mul*/{
            if(funct7 == 0x00){ // add
                fprintf(saida,"0x%08x:add    %s,%s,%s     %s=0x%08x+0x%08x=0x%08x\n", 
                    pc, nomex[rd], nomex[rs1], nomex[rs2], nomex[rd], 
                    x[rs1], x[rs2], x[rs1] + x[rs2]);                
                x[rd] = x[rs1] + x[rs2];

            }else if(funct7 == 0x20) { // sub
                fprintf(saida,"0x%08x:sub    %s,%s,%s     %s=0x%08x-0x%08x=0x%08x\n", 
                    pc, nomex[rd], nomex[rs1], nomex[rs2], nomex[rd], 
                    x[rs1], x[rs2], x[rs1] - x[rs2]);                
                x[rd] = x[rs1] - x[rs2];

            }else if(funct7 == 0x01){ //mul
                x[rd] = (((int64_t)x[rs1] * (int64_t)x[rs2]) & 0xFFFFFFFF);
                fprintf(saida,"0x%08x:mul    %s,%s,%s     %s=0x%08x*0x%08x=0x%08x\n", 
                    pc, nomex[rd], nomex[rs1], nomex[rs2], nomex[rd], 
                    prev_rs1, prev_rs2, x[rd]);    
            }else{
                trap_capture(2, instrucao, saida);
            }
            break;
        }
        case 0x1: /*Shift Left Logical, (Multiplicação - Metade Superior com Sinal)*/ {
            if(funct7 == 0x00){ //sll
                uint32_t shamt = x[rs2] & 0b11111;
                x[rd] = (uint32_t)x[rs1] << shamt;
                fprintf(saida,"0x%08x:sll    %s,%s,%s     %s=0x%08x<<%u=0x%08x\n", 
                    pc, nomex[rd], nomex[rs1], nomex[rs2], nomex[rd], 
                    prev_rs1, shamt, x[rd]);
            }else if(funct7 == 0x01){ //mulh
                int64_t res = (int64_t)x[rs1] * (int64_t)x[rs2];
                x[rd] = (res >> 32);
                fprintf(saida,"0x%08x:mulh   %s,%s,%s     %s=0x%08x*0x%08x=0x%08x\n",
                       pc, nomex[rd], nomex[rs1], nomex[rs2],
                       nomex[rd], prev_rs1, prev_rs2, x[rd]);
            }else{

            }
            break;
        }
        case 0x2: /*Set Less Than, (Multiplicação - Metade Superior com Sinal/Sem Sinal)*/ {
            if(funct7 == 0x00){ // slt
                x[rd] = (x[rs1] < x[rs2])?1:0;
                fprintf(saida,"0x%08x:slt    %s,%s,%s     %s=(0x%08x<0x%08x)=%u\n", 
                        pc, nomex[rd], nomex[rs1], nomex[rs2], nomex[rd], 
                        prev_rs1, prev_rs2, x[rd]);
            }else if(funct7 == 0x01){ //mulhsu
                int64_t res = (int64_t)(int32_t)prev_rs1* (uint64_t)(uint32_t)prev_rs2;
                x[rd] = (int32_t)(res >> 32);
                fprintf(saida,"0x%08x:mulhsu %s,%s,%s     %s=0x%08x*0x%08x=0x%08x\n",
                       pc, nomex[rd], nomex[rs1], nomex[rs2],
                       nomex[rd], prev_rs1, prev_rs2, x[rd]);
            }else{
                trap_capture(2, instrucao, saida);
            }
            break;
        }
        case 0x3: /* Set Less Than (U) - sltu, */{
            if(funct7 == 0x00){ //sltu
                x[rd] = ((uint32_t)x[rs1] < (uint32_t)x[rs2])?1:0;
                fprintf(saida,"0x%08x:sltu   %s,%s,%s     %s=(0x%08x<0x%08x)=%u\n",
                    pc, nomex[rd], nomex[rs1], nomex[rs2],
                    nomex[rd], prev_rs1, prev_rs2, x[rd]);                
            }else if(funct7 == 0x01){ //mulhu
                uint64_t res = (uint64_t)(uint32_t)x[rs1] * (uint64_t)(uint32_t)x[rs2];
                uint32_t high = (uint32_t)(res >> 32);
                x[rd] = (int32_t)high;
                fprintf(saida,"0x%08x:mulhu  %s,%s,%s     %s=0x%08x*0x%08x=0x%08x\n",
                       pc, nomex[rd], nomex[rs1], nomex[rs2],
                       nomex[rd], prev_rs1, prev_rs2, x[rd]);
            }else{
                trap_capture(2, instrucao, saida);
            }

            break;
        }
        case 0x4: /*Xor, Divisão com Sinal*/{
            if(funct7 == 0x00){ //xor
                x[rd] = x[rs1]^x[rs2];
                fprintf(saida,"0x%08x:xor    %s,%s,%s     %s=0x%08x^0x%08x=0x%08x\n",
                        pc, nomex[rd], nomex[rs1], nomex[rs2], nomex[rd], 
                        prev_rs1, prev_rs2, x[rd]);                
            }else if(funct7 == 0x01){ //div
                if (x[rs2] == 0) x[rd] = -1; 
                else x[rd] = (int32_t)x[rs1] / (int32_t)x[rs2];
                fprintf(saida,"0x%08x:div    %s,%s,%s     %s=0x%08x/0x%08x=0x%08x\n",
                       pc, nomex[rd], nomex[rs1], nomex[rs2],
                       nomex[rd], prev_rs1, prev_rs2, x[rd]);
            }else{
                trap_capture(2, instrucao, saida);
            }

            break;  
        }
        case 0x5: /*Shift Right, Divisão Sem Sinal*/{
            uint32_t shamt = x[rs2] & 0b11111;
            if(funct7 == 0x00){ // Shift Right Logical (srl)
                x[rd] = (uint32_t)x[rs1] >> shamt;
                fprintf(saida,"0x%08x:srl    %s,%s,%s     %s=0x%08x>>%u=0x%08x\n", 
                    pc, nomex[rd], nomex[rs1], nomex[rs2],
                    nomex[rd], prev_rs1, shamt, x[rd]);
            }else if (funct7 == 0x20){ //Shift Right Arithmetic (sra) - considera o sinal
                x[rd] = (int32_t)x[rs1] >> shamt;
                fprintf(saida,"0x%08x:sra    %s,%s,%s     %s=0x%08x>>>%u=0x%08x\n", 
                    pc, nomex[rd], nomex[rs1], nomex[rs2],nomex[rd], 
                    prev_rs1, shamt, x[rd]);
            }else if(funct7 == 0x01){ //divu
                if (x[rs2] == 0) x[rd] = -1;
                else x[rd] = (uint32_t)x[rs1] / (uint32_t)x[rs2];
                fprintf(saida,"0x%08x:divu   %s,%s,%s     %s=0x%08x/0x%08x=0x%08x\n",
                       pc, nomex[rd], nomex[rs1], nomex[rs2],
                       nomex[rd], prev_rs1, prev_rs2, x[rd]);
            }else{
                trap_capture(2, instrucao, saida);
            }
            break;
        }
        case 0x6: /*Or, Resto com Sinal*/{
            if(funct7 == 0x00){
                x[rd] = x[rs1] | x[rs2];
                fprintf(saida,"0x%08x:or     %s,%s,%s     %s=0x%08x|0x%08x=0x%08x\n",
                    pc, nomex[rd], nomex[rs1], nomex[rs2],nomex[rd],
                    prev_rs1, prev_rs2, x[rd]);
            }else if(funct7){ //rem
                if (x[rs2] == 0) x[rd] = x[rs1]; 
                else x[rd] = (int32_t)x[rs1] % (int32_t)x[rs2];
                fprintf(saida,"0x%08x:rem    %s,%s,%s     %s=0x%08x%%0x%08x=0x%08x\n",
                       pc, nomex[rd], nomex[rs1], nomex[rs2],
                       nomex[rd], prev_rs1, prev_rs2, x[rd]);
            }else{
                trap_capture(2, instrucao, saida);
            }
            break;
        }
        case 0x7: /*And, Resto sem Sinal*/ {
            if(funct7 == 0x00){ //and
                x[rd] = x[rs1] & x[rs2];
                fprintf(saida,"0x%08x:and    %s,%s,%s     %s=0x%08x&0x%08x=0x%08x\n",
                    pc, nomex[rd], nomex[rs1], nomex[rs2],nomex[rd],
                    prev_rs1, prev_rs2, x[rd]);
            }else if(funct7 == 0x01){ // remu
                if (x[rs2] == 0) x[rd] = x[rs1];
                else x[rd] = (uint32_t)x[rs1] % (uint32_t)x[rs2];
                fprintf(saida,"0x%08x:remu   %s,%s,%s     %s=0x%08x%%0x%08x=0x%08x\n",
                       pc, nomex[rd], nomex[rs1], nomex[rs2],
                       nomex[rd], prev_rs1, prev_rs2, x[rd]);
            }else{
                trap_capture(2, instrucao, saida);
            }
            break; 
        }
        default:
            trap_capture(2, instrucao, saida);
    }
}

void B_type(uint32_t instrucao, int32_t imm, uint8_t rs1, uint8_t rs2, uint8_t funct3, FILE* saida){ //0110011
    uint8_t flg = 0;

    switch (funct3){
        case 0x0: /*Branch ==(beq) */ {
            flg = (x[rs1] == x[rs2]);
            fprintf(saida,"0x%08x:beq    %s,%s,0x%03x  (0x%08x==0x%08x)=%u->pc=0x%08x\n",
                pc, nomex[rs1], nomex[rs2], (imm >> 1)&0xFFF,
                x[rs1], x[rs2], flg, pc + (flg ? imm : 4));

            break;
        }
        case 0x1: /*Branch != (bne)*/ {
            flg = (x[rs1] != x[rs2]);
            fprintf(saida, "0x%08x:bne    %s,%s,0x%03x  (0x%08x!=0x%08x)=%u->pc=0x%08x\n",
                pc, nomex[rs1], nomex[rs2], (imm >> 1) & 0xFFF,
                x[rs1], x[rs2], flg, pc + (flg ? imm : 4));

            break;
        }
        case 0x4: /*Branch < (blt)*/ {
            flg = (x[rs1] < x[rs2]);
            fprintf(saida, "0x%08x:blt    %s,%s,0x%03x  (0x%08x<0x%08x)=%u->pc=0x%08x\n",
                pc, nomex[rs1], nomex[rs2], (imm >> 1) & 0xFFF,
                x[rs1], x[rs2], flg, pc + (flg ? imm : 4));

            break;
        }
        case 0x5: /*Branch >= (bge)*/ {
            flg = (x[rs1] >= x[rs2]);
            fprintf(saida, "0x%08x:bge    %s,%s,0x%03x  (0x%08x>=0x%08x)=%u->pc=0x%08x\n",
                pc, nomex[rs1], nomex[rs2], (imm >> 1) & 0xFFF,
                x[rs1], x[rs2], flg, pc + (flg ? imm : 4));
            break;
        }
        case 0x6: /*Branch < (U) (bltu)*/ {
            flg = ((uint32_t)x[rs1] < (uint32_t)x[rs2]);
            fprintf(saida,"0x%08x:bltu   %s,%s,0x%03x  (0x%08x<0x%08x)=%u->pc=0x%08x\n",
                pc, nomex[rs1], nomex[rs2],(imm >> 1) & 0xFFF,
                x[rs1], x[rs2], flg, pc + (flg ? imm : 4));
            break;
        }
        case 0x7: /*Branch >= (U) (bgeu)*/ {
            flg = ((uint32_t)x[rs1] >= (uint32_t)x[rs2]);
            fprintf(saida, "0x%08x:bgeu   %s,%s,0x%03x  (0x%08x>=0x%08x)=%u->pc=0x%08x\n",
                pc, nomex[rs1], nomex[rs2], (imm >> 1) & 0xFFF,
                x[rs1], x[rs2], flg, pc + (flg ? imm : 4));
            break;
        }
        default:
            trap_capture(2, instrucao, saida);
    }
    if(flg) pc += imm - 4;
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

    uint8_t* mem = (uint8_t*)(malloc(tam_end)); 
    load_entrada(entrada, mem);
    fclose(entrada);

    pc = RAM_INF;
    mem[ADDRS_ISR] = RST_ISR;
    mem[ADDRS_LSR] = RST_LSR;

    while(run){
        if ((pc % 4 != 0) || addrs_range(pc) != 0) {
            trap_capture(1, 0, saida);
            pc += 4; 
            continue; 
        }

        uint32_t icause;
        if (check_int(&icause)) {
            trap_capture(icause, 0, saida);
            pc += 4;
            continue;
        }

        uint32_t instrucao = ((uint32_t*)mem)[(pc - RAM_INF) >> 2];
        uint8_t opcode = instrucao & 0b1111111;              // bits 6:0
        uint8_t rd     = (instrucao >> 7) & 0b11111;         // bits 11:7
        uint8_t funct3 = (instrucao >> 12) & 0b111;          // bits 14:12
        uint8_t rs1    = (instrucao >> 15) & 0b11111;        // bits 19:15
        uint8_t rs2    = (instrucao >> 20) & 0b11111;        // bits 24:20
        uint8_t funct7 = (instrucao >> 25) & 0b1111111;      // bits 31:25
        int32_t immI = ((int32_t)instrucao >> 20);
        uint32_t immU = (instrucao >> 12);
        
        switch (opcode) {
            case 0b0110011:{ // R - type
                R_type(instrucao, funct7, rs1, rs2, funct3, rd, saida);
                break;
            }
            case 0b0000011:{ // I - type (load)
                I_type_load(instrucao, immI, rs1, funct3, rd, saida, mem);
                break;
            }
            case 0b0100011:{ // S - type (store)
                int16_t immS = (funct7 << 5) | rd;
                if (immS & 0x800) immS |= 0xF000;
                S_type(instrucao, immS, rs1, rs2, funct3, saida, mem);
                break;
            }
            case 0b0010011:{ // I - type (imm)
                I_type_imm(instrucao, immI, rs1, funct3, rd, saida, mem);
                break;
            }
            case 0b1100011:{ //B-type
                int32_t immB = 0; 
                immB |= ((rd >> 1) & 0xF) << 1;          // bits 1–4  
                immB |= (funct7 & 0x3F) << 5;            // bits 5–10 
                immB |= (rd & 0x1) << 11;                // bit 11    
                immB |= ((instrucao >> 31) & 0x1) << 12; // bit 12  
                if (immB & 0x1000) immB |= 0xFFFFE000;   // extende o sinal se o bit 12 tiver setado
                
                B_type(instrucao, immB, rs1, rs2, funct3, saida);
                break;
            }
            case 0b0110111:{ //lui U-type
                x[rd] = (immU << 12);
                fprintf(saida, "0x%08x:lui    %s,0x%05x     %s=0x%05x000\n",
                        pc, nomex[rd], (immU & 0xFFFFF), nomex[rd], (immU & 0xFFFFF));
                break;
            }
            case 0b0010111:{ //auipc U-type
                x[rd] = pc + (immU << 12);
                fprintf(saida, "0x%08x:auipc  %s,0x%05x     %s=0x%08x+0x%05x000=0x%08x\n",
                        pc, nomex[rd], (immU & 0xFFFFF), nomex[rd],pc, (immU & 0xFFFFF), x[rd]);
                break;
            }
            case 0b1100111:{ // jalr -> TIPO I
                uint32_t novo_pc = (x[rs1] + (int32_t)immI);
                fprintf(saida, "0x%08x:jalr   %s,%s,0x%03x   pc=0x%08x+0x%08x,%s=0x%08x\n",
                        pc, nomex[rd], nomex[rs1], immI, x[rs1], (int32_t)immI, nomex[rd], pc + 4);
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

                fprintf(saida, "0x%08x:jal    %s,0x%05x     pc=0x%08x,%s=0x%08x\n", 
                        pc, nomex[rd], ((immJ >> 1) & 0xFFFFF), novo_pc, nomex[rd], x[rd]);
                pc = novo_pc - 4;
                break;
            }
            case 0b1110011: { // instruções csr, mnret, ecall, ebreak;
                CSR_Fluxo(instrucao, rd, funct3, rs1, immI, saida);
                break;
            }
            default:{
                trap_capture(2, instrucao, saida);
            }
        }
        x[0] = 0;
        pc += 4;
        timer(mem);
    }
    fclose(saida);
}