oid CSR_Enviroment(uint32_t instrucao, uint32_t pc, uint8_t rd, uint8_t funct3, uint8_t rs1, int32_t immI, FILE* saida, uint32_t* pc_ptr, uint8_t* flg) {
    
    // O endereço do CSR ou o código de instrução especial está nos 12 bits do imediato (immI)
    uint32_t csr_addr = (uint32_t)immI & 0xFFF;

    if (funct3 == 0b000) {
        // Instruções de controle de fluxo (ecall, ebreak, mret)
        if (csr_addr == 0b000000000000) { // ecall
            fprintf(saida, "ecall\n");
            // Dispara exceção 11 (Environment Call from M-mode)
            trigger_trap(11, pc, 0, saida, pc_ptr, flg); 

        } else if (csr_addr == 0b001100000010) { // mret
            fprintf(saida, "mret\n");

            // 1. Manipula mstatus
            uint32_t mstatus_old = mstatus;
            uint32_t mpie = (mstatus_old >> 7) & 0b1; // Captura MPIE[7]

            // MIE (bit 3) = MPIE (bit 7)
            mstatus = (mstatus & ~(1 << 3)) | (mpie << 3); 
            // MPIE (bit 7) = 1
            mstatus |= (1 << 7);
            // MPP (bits 12:11) = 00 (Modo User, se estiver em M-mode)
            mstatus &= ~(0b11 << 11);

            // 2. Atualiza o PC para MEPC.
            // O loop principal vai somar +4 no final, então setamos pc para mepc - 4
            *pc_ptr = mepc - 4; 

        } else if (csr_addr == 0b000000000001) { // ebreak
            // ebreak (Breakpoint) é Causa 3. 
            // A instrução no 2_exception.out.txt é tratada como parada final, mas vamos tratar como trap 3.
            trigger_trap(3, pc, 0, saida, pc_ptr, flg);
            *flg = 0; // Termina a simulação após o trap, conforme o final do 2_exception.out.txt

        } else {
             // Instrução de sistema desconhecida
             trigger_trap(2, pc, instrucao, saida, pc_ptr, flg);
        }
    } else {
        // Instruções de CSR (csrrw, csrrs, csrrc, csrrwi, csrrsi, csrrci)

        uint32_t csr_old = map_csr(csr_addr);
        
        // Determina o valor a ser escrito/modificado
        // Se for Imediato (funct3 >= 0b101), rs1_val é o uimm (rs1)
        // Se for Registrador (funct3 < 0b101), rs1_val é o valor de x[rs1]
        uint32_t rs1_val = (funct3 < 0b101) ? (uint32_t)x[rs1] : (uint32_t)rs1;
        
        uint32_t csr_new = csr_old;
        char csr_mnemonic[10];

        switch (funct3) {
            case 0b001: // csrrw: rd=csr, csr=rs1
                strcpy(csr_mnemonic, "csrrw");
                csr_new = rs1_val;
                break;
            case 0b010: // csrrs: rd=csr, csr=csr | rs1
                strcpy(csr_mnemonic, "csrrs");
                csr_new = csr_old | rs1_val;
                break;
            case 0b011: // csrrc: rd=csr, csr=csr & ~rs1
                strcpy(csr_mnemonic, "csrrc");
                csr_new = csr_old & ~rs1_val;
                break;
            case 0b101: // csrrwi: rd=csr, csr=uimm
                strcpy(csr_mnemonic, "csrrwi");
                csr_new = rs1_val;
                break;
            case 0b110: // csrrsi: rd=csr, csr=csr | uimm
                strcpy(csr_mnemonic, "csrrsi");
                csr_new = csr_old | rs1_val;
                break;
            case 0b111: // csrrci: rd=csr, csr=csr & ~uimm
                strcpy(csr_mnemonic, "csrrci");
                csr_new = csr_old & ~rs1_val;
                break;
            default:
                // Trigger illegal instruction trap
                trigger_trap(2, pc, instrucao, saida, pc_ptr, flg); 
                return;
        }
        
        // Output format (adequado para simulação)
        if (funct3 < 0b101) { // Variantes de Registrador
            fprintf(saida, "%s  %s,0x%03x,%s   %s=%s=0x%08x,%s|=zero=0x%08x\n",
                csr_mnemonic, nomex[rd], csr_addr, nomex[rs1],
                nomex[rd], csr_mnemonic, csr_old, nomex[rs1], csr_new);
        } else { // Variantes Imediatas
             fprintf(saida, "%s  %s,0x%03x,0x%x   %s=%s=0x%08x,%s|=zero=0x%08x\n",
                csr_mnemonic, nomex[rd], csr_addr, rs1_val,
                nomex[rd], csr_mnemonic, csr_old, nomex[rs1], csr_new);
        }


        if (rd != 0) x[rd] = csr_old;
        write_csr(csr_addr, csr_new);
    }
}