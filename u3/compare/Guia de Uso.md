# 🔍 RISC-V Trace Comparator

Ferramenta para comparar arquivos de trace do simulador RISC-V, mostrando diferenças token por token de forma visual, similar ao comparador do VS Code mas otimizado para arquivos grandes.

## 📦 Arquivos

- **`riscv_trace_diff.py`** - Comparador para terminal (com cores)
- **`riscv_trace_diff_html.py`** - Gerador de relatório HTML interativo

## 🚀 Uso Rápido

### Comparador de Terminal

```bash
python riscv_trace_diff.py minha_saida.txt saida_esperada.txt
```

### Relatório HTML

```bash
python riscv_trace_diff_html.py minha_saida.txt saida_esperada.txt relatorio.html
```

Depois abra `relatorio.html` no seu navegador preferido.

## 📖 Comparador de Terminal - Detalhes

### Sintaxe Completa

```bash
python riscv_trace_diff.py <arquivo1> <arquivo2> [opções]
```

### Opções Disponíveis

- `-c N, --context N` - Número de linhas de contexto antes/depois das diferenças (padrão: 2)
- `-m N, --max-diffs N` - Máximo de diferenças a mostrar (padrão: todas)
- `-n, --no-line-numbers` - Não mostra números de linha

### Exemplos

```bash
# Comparação básica
python riscv_trace_diff.py output.log expected.log

# Com mais contexto (5 linhas antes/depois)
python riscv_trace_diff.py output.log expected.log -c 5

# Mostrando apenas as primeiras 20 diferenças
python riscv_trace_diff.py output.log expected.log -m 20

# Sem números de linha
python riscv_trace_diff.py output.log expected.log -n

# Combinando opções
python riscv_trace_diff.py output.log expected.log -c 5 -m 50
```

### Legenda de Cores no Terminal

- **Vermelho (fundo)** - Tokens/linhas do arquivo 1 (sua saída) que diferem
- **Verde (fundo)** - Tokens/linhas do arquivo 2 (saída esperada) que diferem
- **Ciano** - Números de linha e bordas
- **Amarelo** - Informações e avisos

## 🌐 Relatório HTML - Detalhes

### Sintaxe

```bash
python riscv_trace_diff_html.py <arquivo1> <arquivo2> [output.html]
```

### Recursos do HTML

- **Navegação interativa** entre diferenças
- **Botões de controle:**
  - ⬇️ Próxima Diferença
  - ⬆️ Diferença Anterior
  - 👁️ Toggle Contexto (mostra/esconde linhas de contexto)
- **Atalhos de teclado:**
  - `n` ou `↓` - Próxima diferença
  - `p` ou `↑` - Diferença anterior
  - `c` - Toggle contexto
- **Auto-scroll** para primeira diferença ao carregar
- **Visual estilo VS Code** (tema escuro)
- **Highlighting** de tokens diferentes

### Exemplo

```bash
python riscv_trace_diff_html.py minha_saida.txt esperado.txt resultado.html
firefox resultado.html  # ou chrome, edge, etc.
```

## 💡 Como Funciona

### Comparação Token por Token

A ferramenta não apenas compara linhas completas, mas também divide cada linha em **tokens** (palavras, números hexadecimais, símbolos) e compara token por token, destacando exatamente o que mudou dentro de cada linha.

Exemplo de linha RISC-V:
```
0x00000040:add    rd,rs1,rs2     rd=0x12345678+0xABCDEF00=0xBE024578
```

Tokens identificados:
- `0x00000040`
- `:`
- `add`
- `rd`
- `,`
- `rs1`
- ...

Se apenas um valor hexadecimal mudar, apenas aquele token específico será destacado.

### Tipos de Diferenças Detectadas

1. **Tokens diferentes** - Valores que mudaram (ex: `0x12345678` vs `0xABCDEF00`)
2. **Tokens faltando** - Existem em um arquivo mas não no outro
3. **Linhas extras** - Uma arquivo tem mais linhas que o outro
4. **Linhas faltando** - Um arquivo tem menos linhas que o outro

## 🎯 Vantagens sobre o VS Code Diff

1. **Suporta arquivos grandes** - Não trava com milhares de linhas
2. **Highlighting de tokens** - Destaca exatamente o que mudou dentro de cada linha
3. **Contexto configurável** - Escolha quantas linhas de contexto mostrar
4. **Limite de diferenças** - Veja apenas as N primeiras diferenças
5. **Saída HTML** - Relatório navegável que você pode salvar e compartilhar
6. **Formatado para RISC-V** - Otimizado para traces de simuladores

## 📊 Formato de Entrada

A ferramenta funciona com qualquer arquivo texto, mas foi otimizada para traces RISC-V no formato:

```
0x????????:instrução  operandos  valores=0x????????
```

Exemplos:
```
0x00000000:add    rd,rs1,rs2     rd=0x00000001+0x00000002=0x00000003
0x00000004:lw     rd,0x000(rs1)  rd=mem[0x00001000]=0xDEADBEEF
0x00000008:beq    rs1,rs2,0x010  (0x00000001==0x00000001)=u1->pc=0x00000018
>exception:illegal_instruction   cause=0x00000002,epc=0x0000000C,tval=0x00000000
```

## 🐛 Troubleshooting

### Problema: "Arquivo não encontrado"
**Solução:** Verifique o caminho dos arquivos. Use caminhos relativos ou absolutos.

### Problema: Arquivo muito grande, demora muito
**Solução:** Use a opção `-m` para limitar o número de diferenças mostradas:
```bash
python riscv_trace_diff.py output.log expected.log -m 100
```

### Problema: Terminal não mostra cores
**Solução:** Certifique-se de usar um terminal que suporte cores ANSI. Use a versão HTML como alternativa:
```bash
python riscv_trace_diff_html.py output.log expected.log report.html
```

### Problema: Encoding errado (caracteres estranhos)
**Solução:** Os scripts assumem UTF-8. Se seus arquivos usam outra codificação, converta-os primeiro:
```bash
iconv -f ISO-8859-1 -t UTF-8 arquivo.txt > arquivo_utf8.txt
```

## 🔧 Requisitos

- Python 3.6+
- Nenhuma biblioteca externa necessária (usa apenas bibliotecas padrão do Python)

## 📝 Exemplos de Saída

### Terminal
```
================================================================================
Comparando arquivos:
  Arquivo 1 (sua saída):     output.log
  Arquivo 2 (saída esperada): expected.log
================================================================================

  1      0x00000000:add    rd,rs1,rs2     rd=0x00000001+0x00000002=0x00000003
  2      0x00000004:sub    rd,rs1,rs2     rd=0x00000005-0x00000002=0x00000003

────────────────────────────────────────────────────────────────────────────────
>>> Linha 3: Diferença encontrada
-     3  0x00000008:lw     rd,0x000(rs1)  rd=mem[0x00001000]=0xDEADBEEF
+     3  0x00000008:lw     rd,0x000(rs1)  rd=mem[0x00001000]=0x12345678
────────────────────────────────────────────────────────────────────────────────

  4      0x0000000C:sw     rs2,0x004(rs1) mem[0x00001004]=0xCAFEBABE

================================================================================
RESUMO DA COMPARAÇÃO:
  Total de linhas (arquivo 1): 100
  Total de linhas (arquivo 2): 100
  Diferenças encontradas:      1

✗ Arquivos diferem em 1 linha(s)
================================================================================
```

## 🤝 Contribuições

Sugestões e melhorias são bem-vindas! Algumas ideias:

- [ ] Modo diff side-by-side no terminal
- [ ] Filtros para ignorar certos padrões
- [ ] Estatísticas detalhadas por tipo de instrução
- [ ] Export para outros formatos (JSON, CSV)
- [ ] Modo interativo de navegação no terminal

## 📄 Licença

Livre para uso pessoal e acadêmico.

---

**Feito para estudantes e desenvolvedores de simuladores RISC-V** 🚀
