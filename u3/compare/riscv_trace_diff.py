#!/usr/bin/env python3
"""
RISC-V Trace Comparator
Compara arquivos de trace do simulador RISC-V, mostrando diferenças token por token
"""

import sys
import re
from difflib import SequenceMatcher
from typing import List, Tuple, Optional

# Cores ANSI para terminal
class Colors:
    RED = '\033[91m'
    GREEN = '\033[92m'
    YELLOW = '\033[93m'
    BLUE = '\033[94m'
    MAGENTA = '\033[95m'
    CYAN = '\033[96m'
    BOLD = '\033[1m'
    UNDERLINE = '\033[4m'
    RESET = '\033[0m'
    BG_RED = '\033[101m'
    BG_GREEN = '\033[102m'
    BG_YELLOW = '\033[103m'

def colorize(text: str, color: str) -> str:
    """Adiciona cor ao texto"""
    return f"{color}{text}{Colors.RESET}"

def tokenize_line(line: str) -> List[str]:
    """
    Separa uma linha em tokens (palavras, números hexadecimais, símbolos)
    Preserva espaços entre tokens para reconstrução
    """
    # Regex para capturar tokens: hex numbers, words, symbols, spaces
    pattern = r'(0x[0-9a-fA-F?]+|\w+|[^\w\s]|\s+)'
    tokens = re.findall(pattern, line)
    return tokens

def compare_tokens(tokens1: List[str], tokens2: List[str]) -> Tuple[str, str]:
    """
    Compara duas listas de tokens e retorna strings coloridas mostrando diferenças
    """
    matcher = SequenceMatcher(None, tokens1, tokens2)
    
    result1 = []
    result2 = []
    
    for tag, i1, i2, j1, j2 in matcher.get_opcodes():
        if tag == 'equal':
            # Tokens iguais - sem cor
            result1.extend(tokens1[i1:i2])
            result2.extend(tokens2[j1:j2])
        elif tag == 'replace':
            # Tokens diferentes - vermelho para arquivo 1, verde para arquivo 2
            for token in tokens1[i1:i2]:
                if not token.isspace():
                    result1.append(colorize(token, Colors.BG_RED + Colors.BOLD))
                else:
                    result1.append(token)
            for token in tokens2[j1:j2]:
                if not token.isspace():
                    result2.append(colorize(token, Colors.BG_GREEN + Colors.BOLD))
                else:
                    result2.append(token)
        elif tag == 'delete':
            # Token existe apenas no arquivo 1 - vermelho
            for token in tokens1[i1:i2]:
                if not token.isspace():
                    result1.append(colorize(token, Colors.BG_RED + Colors.BOLD))
                else:
                    result1.append(token)
        elif tag == 'insert':
            # Token existe apenas no arquivo 2 - verde
            for token in tokens2[j1:j2]:
                if not token.isspace():
                    result2.append(colorize(token, Colors.BG_GREEN + Colors.BOLD))
                else:
                    result2.append(token)
    
    return ''.join(result1), ''.join(result2)

def compare_files(file1_path: str, file2_path: str, context_lines: int = 2, 
                  max_diffs: Optional[int] = None, show_line_numbers: bool = True):
    """
    Compara dois arquivos linha por linha, mostrando diferenças
    
    Args:
        file1_path: Caminho do primeiro arquivo (sua saída)
        file2_path: Caminho do segundo arquivo (saída esperada)
        context_lines: Número de linhas de contexto a mostrar antes/depois de diferenças
        max_diffs: Número máximo de diferenças a mostrar (None = todas)
        show_line_numbers: Se True, mostra números de linha
    """
    print(colorize("=" * 80, Colors.CYAN))
    print(colorize(f"Comparando arquivos:", Colors.BOLD + Colors.CYAN))
    print(colorize(f"  Arquivo 1 (sua saída):     {file1_path}", Colors.YELLOW))
    print(colorize(f"  Arquivo 2 (saída esperada): {file2_path}", Colors.YELLOW))
    print(colorize("=" * 80, Colors.CYAN))
    print()
    
    try:
        with open(file1_path, 'r', encoding='utf-8') as f1:
            lines1 = f1.readlines()
        with open(file2_path, 'r', encoding='utf-8') as f2:
            lines2 = f2.readlines()
    except FileNotFoundError as e:
        print(colorize(f"Erro: Arquivo não encontrado - {e}", Colors.RED + Colors.BOLD))
        return
    except Exception as e:
        print(colorize(f"Erro ao ler arquivos: {e}", Colors.RED + Colors.BOLD))
        return
    
    # Estatísticas
    total_lines = max(len(lines1), len(lines2))
    diff_count = 0
    shown_diffs = 0
    
    # Compara linha por linha
    for i in range(total_lines):
        line1 = lines1[i].rstrip('\n') if i < len(lines1) else None
        line2 = lines2[i].rstrip('\n') if i < len(lines2) else None
        
        # Verifica se as linhas são diferentes
        if line1 != line2:
            diff_count += 1
            
            # Limita número de diferenças mostradas
            if max_diffs and shown_diffs >= max_diffs:
                continue
            
            shown_diffs += 1
            
            # Mostra contexto antes
            if i > 0 and context_lines > 0:
                start = max(0, i - context_lines)
                for j in range(start, i):
                    ctx_line = lines1[j].rstrip('\n') if j < len(lines1) else lines2[j].rstrip('\n')
                    if show_line_numbers:
                        print(colorize(f"  {j+1:6d} ", Colors.CYAN) + ctx_line)
                    else:
                        print(f"  {ctx_line}")
            
            # Mostra a diferença
            line_num = i + 1
            print()
            print(colorize("─" * 80, Colors.MAGENTA))
            
            if line1 is None:
                # Linha existe apenas no arquivo 2
                print(colorize(f">>> Linha {line_num}: Faltando no arquivo 1", Colors.RED + Colors.BOLD))
                if show_line_numbers:
                    print(colorize(f"  {line_num:6d} ", Colors.CYAN) + colorize("(vazio)", Colors.RED))
                    print(colorize(f"+ {line_num:6d} ", Colors.GREEN) + colorize(line2, Colors.GREEN))
                else:
                    print(colorize("  (vazio)", Colors.RED))
                    print(colorize(f"+ {line2}", Colors.GREEN))
            elif line2 is None:
                # Linha existe apenas no arquivo 1
                print(colorize(f">>> Linha {line_num}: Extra no arquivo 1", Colors.RED + Colors.BOLD))
                if show_line_numbers:
                    print(colorize(f"- {line_num:6d} ", Colors.RED) + colorize(line1, Colors.RED))
                    print(colorize(f"  {line_num:6d} ", Colors.CYAN) + colorize("(vazio)", Colors.GREEN))
                else:
                    print(colorize(f"- {line1}", Colors.RED))
                    print(colorize("  (vazio)", Colors.GREEN))
            else:
                # Ambas as linhas existem mas são diferentes
                print(colorize(f">>> Linha {line_num}: Diferença encontrada", Colors.YELLOW + Colors.BOLD))
                
                # Compara token por token
                tokens1 = tokenize_line(line1)
                tokens2 = tokenize_line(line2)
                colored1, colored2 = compare_tokens(tokens1, tokens2)
                
                if show_line_numbers:
                    print(colorize(f"- {line_num:6d} ", Colors.RED) + colored1)
                    print(colorize(f"+ {line_num:6d} ", Colors.GREEN) + colored2)
                else:
                    print(colorize("- ", Colors.RED) + colored1)
                    print(colorize("+ ", Colors.GREEN) + colored2)
            
            print(colorize("─" * 80, Colors.MAGENTA))
            print()
            
            # Mostra contexto depois
            if i < total_lines - 1 and context_lines > 0:
                end = min(total_lines, i + context_lines + 1)
                for j in range(i + 1, end):
                    ctx_line = lines1[j].rstrip('\n') if j < len(lines1) else lines2[j].rstrip('\n')
                    if show_line_numbers:
                        print(colorize(f"  {j+1:6d} ", Colors.CYAN) + ctx_line)
                    else:
                        print(f"  {ctx_line}")
            print()
    
    # Resumo
    print(colorize("=" * 80, Colors.CYAN))
    print(colorize("RESUMO DA COMPARAÇÃO:", Colors.BOLD + Colors.CYAN))
    print(colorize(f"  Total de linhas (arquivo 1): {len(lines1)}", Colors.YELLOW))
    print(colorize(f"  Total de linhas (arquivo 2): {len(lines2)}", Colors.YELLOW))
    print(colorize(f"  Diferenças encontradas:      {diff_count}", 
                  Colors.RED + Colors.BOLD if diff_count > 0 else Colors.GREEN + Colors.BOLD))
    
    if max_diffs and diff_count > max_diffs:
        print(colorize(f"  (mostrando apenas as primeiras {max_diffs} diferenças)", Colors.YELLOW))
    
    if diff_count == 0:
        print(colorize("\n✓ Arquivos são idênticos!", Colors.GREEN + Colors.BOLD))
    else:
        print(colorize(f"\n✗ Arquivos diferem em {diff_count} linha(s)", Colors.RED + Colors.BOLD))
    
    print(colorize("=" * 80, Colors.CYAN))

def main():
    if len(sys.argv) < 3:
        print(colorize("Uso: python riscv_trace_diff.py <arquivo1> <arquivo2> [opções]", Colors.YELLOW))
        print()
        print("Opções:")
        print("  -c N, --context N       Número de linhas de contexto (padrão: 2)")
        print("  -m N, --max-diffs N     Máximo de diferenças a mostrar (padrão: todas)")
        print("  -n, --no-line-numbers   Não mostra números de linha")
        print()
        print("Exemplos:")
        print("  python riscv_trace_diff.py minha_saida.txt saida_esperada.txt")
        print("  python riscv_trace_diff.py output.log expected.log -c 5 -m 20")
        sys.exit(1)
    
    file1 = sys.argv[1]
    file2 = sys.argv[2]
    
    # Parse opções
    context_lines = 2
    max_diffs = None
    show_line_numbers = True
    
    i = 3
    while i < len(sys.argv):
        arg = sys.argv[i]
        if arg in ['-c', '--context']:
            if i + 1 < len(sys.argv):
                context_lines = int(sys.argv[i + 1])
                i += 2
            else:
                print(colorize(f"Erro: {arg} requer um valor", Colors.RED))
                sys.exit(1)
        elif arg in ['-m', '--max-diffs']:
            if i + 1 < len(sys.argv):
                max_diffs = int(sys.argv[i + 1])
                i += 2
            else:
                print(colorize(f"Erro: {arg} requer um valor", Colors.RED))
                sys.exit(1)
        elif arg in ['-n', '--no-line-numbers']:
            show_line_numbers = False
            i += 1
        else:
            print(colorize(f"Opção desconhecida: {arg}", Colors.RED))
            sys.exit(1)
    
    compare_files(file1, file2, context_lines, max_diffs, show_line_numbers)

if __name__ == "__main__":
    main()
