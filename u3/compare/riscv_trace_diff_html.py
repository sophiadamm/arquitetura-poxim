#!/usr/bin/env python3
"""
RISC-V Trace Comparator - HTML Report Generator
Gera um relatório HTML comparando arquivos de trace
"""

import sys
import re
import html
from difflib import SequenceMatcher
from typing import List, Tuple

def tokenize_line(line: str) -> List[str]:
    """Separa uma linha em tokens"""
    pattern = r'(0x[0-9a-fA-F?]+|\w+|[^\w\s]|\s+)'
    tokens = re.findall(pattern, line)
    return tokens

def compare_tokens_html(tokens1: List[str], tokens2: List[str]) -> Tuple[str, str]:
    """Compara tokens e retorna HTML com highlighting"""
    matcher = SequenceMatcher(None, tokens1, tokens2)
    
    result1 = []
    result2 = []
    
    for tag, i1, i2, j1, j2 in matcher.get_opcodes():
        if tag == 'equal':
            result1.extend([html.escape(t) for t in tokens1[i1:i2]])
            result2.extend([html.escape(t) for t in tokens2[j1:j2]])
        elif tag == 'replace':
            for token in tokens1[i1:i2]:
                if not token.isspace():
                    result1.append(f'<span class="diff-remove">{html.escape(token)}</span>')
                else:
                    result1.append(html.escape(token))
            for token in tokens2[j1:j2]:
                if not token.isspace():
                    result2.append(f'<span class="diff-add">{html.escape(token)}</span>')
                else:
                    result2.append(html.escape(token))
        elif tag == 'delete':
            for token in tokens1[i1:i2]:
                if not token.isspace():
                    result1.append(f'<span class="diff-remove">{html.escape(token)}</span>')
                else:
                    result1.append(html.escape(token))
        elif tag == 'insert':
            for token in tokens2[j1:j2]:
                if not token.isspace():
                    result2.append(f'<span class="diff-add">{html.escape(token)}</span>')
                else:
                    result2.append(html.escape(token))
    
    return ''.join(result1), ''.join(result2)

def generate_html_report(file1_path: str, file2_path: str, output_path: str, context_lines: int = 2):
    """Gera relatório HTML comparando dois arquivos"""
    
    try:
        with open(file1_path, 'r', encoding='utf-8') as f1:
            lines1 = f1.readlines()
        with open(file2_path, 'r', encoding='utf-8') as f2:
            lines2 = f2.readlines()
    except FileNotFoundError as e:
        print(f"Erro: Arquivo não encontrado - {e}")
        return
    
    total_lines = max(len(lines1), len(lines2))
    diff_count = 0
    
    # Início do HTML
    html_content = f"""<!DOCTYPE html>
<html lang="pt-BR">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>RISC-V Trace Diff - Comparação</title>
    <style>
        * {{
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }}
        
        body {{
            font-family: 'Consolas', 'Monaco', 'Courier New', monospace;
            font-size: 13px;
            background-color: #1e1e1e;
            color: #d4d4d4;
            padding: 20px;
        }}
        
        .header {{
            background-color: #2d2d30;
            padding: 20px;
            margin-bottom: 20px;
            border-radius: 5px;
            border-left: 4px solid #007acc;
        }}
        
        .header h1 {{
            color: #4ec9b0;
            margin-bottom: 10px;
        }}
        
        .file-info {{
            color: #ce9178;
            margin: 5px 0;
        }}
        
        .summary {{
            background-color: #2d2d30;
            padding: 20px;
            margin: 20px 0;
            border-radius: 5px;
            border-left: 4px solid #4ec9b0;
        }}
        
        .summary h2 {{
            color: #4ec9b0;
            margin-bottom: 10px;
        }}
        
        .stat {{
            margin: 5px 0;
            color: #dcdcaa;
        }}
        
        .stat.error {{
            color: #f48771;
            font-weight: bold;
        }}
        
        .stat.success {{
            color: #4ec9b0;
            font-weight: bold;
        }}
        
        .diff-section {{
            background-color: #252526;
            margin: 20px 0;
            border-radius: 5px;
            overflow: hidden;
            border: 1px solid #3e3e42;
        }}
        
        .diff-header {{
            background-color: #2d2d30;
            padding: 10px 15px;
            border-bottom: 1px solid #3e3e42;
            color: #dcdcaa;
            font-weight: bold;
        }}
        
        .diff-content {{
            padding: 10px 0;
        }}
        
        .line {{
            display: flex;
            padding: 2px 0;
            line-height: 1.4;
        }}
        
        .line-number {{
            min-width: 60px;
            text-align: right;
            padding: 0 10px;
            color: #858585;
            user-select: none;
            border-right: 1px solid #3e3e42;
        }}
        
        .line-content {{
            flex: 1;
            padding: 0 10px;
            white-space: pre;
            overflow-x: auto;
        }}
        
        .line.context {{
            background-color: #1e1e1e;
        }}
        
        .line.removed {{
            background-color: #4b1818;
        }}
        
        .line.removed .line-number {{
            color: #f48771;
            background-color: #3d1414;
        }}
        
        .line.added {{
            background-color: #1a4d1a;
        }}
        
        .line.added .line-number {{
            color: #4ec9b0;
            background-color: #143d14;
        }}
        
        .line.missing {{
            background-color: #2d2d30;
            opacity: 0.5;
        }}
        
        .diff-remove {{
            background-color: #6a1b1b;
            color: #ff5555;
            padding: 2px 0;
            font-weight: bold;
        }}
        
        .diff-add {{
            background-color: #1e5a1e;
            color: #5dff5d;
            padding: 2px 0;
            font-weight: bold;
        }}
        
        .separator {{
            height: 20px;
            background: linear-gradient(to right, #252526 0%, #3e3e42 50%, #252526 100%);
            margin: 10px 0;
        }}
        
        ::-webkit-scrollbar {{
            width: 10px;
            height: 10px;
        }}
        
        ::-webkit-scrollbar-track {{
            background: #1e1e1e;
        }}
        
        ::-webkit-scrollbar-thumb {{
            background: #3e3e42;
            border-radius: 5px;
        }}
        
        ::-webkit-scrollbar-thumb:hover {{
            background: #4e4e52;
        }}
        
        .controls {{
            position: sticky;
            top: 20px;
            background-color: #2d2d30;
            padding: 15px;
            margin-bottom: 20px;
            border-radius: 5px;
            z-index: 100;
            border: 1px solid #3e3e42;
        }}
        
        .controls button {{
            background-color: #007acc;
            color: white;
            border: none;
            padding: 8px 15px;
            margin-right: 10px;
            border-radius: 3px;
            cursor: pointer;
            font-family: inherit;
            font-size: 12px;
        }}
        
        .controls button:hover {{
            background-color: #005a9e;
        }}
        
        .controls label {{
            margin-left: 15px;
            color: #dcdcaa;
        }}
        
        .controls input[type="checkbox"] {{
            margin: 0 5px;
        }}
    </style>
</head>
<body>
    <div class="header">
        <h1>🔍 RISC-V Trace Comparator</h1>
        <div class="file-info">📄 Arquivo 1: {html.escape(file1_path)}</div>
        <div class="file-info">📄 Arquivo 2: {html.escape(file2_path)}</div>
    </div>
    
    <div class="controls">
        <button onclick="scrollToNextDiff()">⬇️ Próxima Diferença</button>
        <button onclick="scrollToPrevDiff()">⬆️ Diferença Anterior</button>
        <button onclick="toggleContext()">👁️ Toggle Contexto</button>
        <label>
            <input type="checkbox" id="autoScroll" checked> Auto-scroll
        </label>
    </div>
    
    <div id="content">
"""
    
    # Compara linha por linha
    diff_blocks = []
    
    for i in range(total_lines):
        line1 = lines1[i].rstrip('\n') if i < len(lines1) else None
        line2 = lines2[i].rstrip('\n') if i < len(lines2) else None
        
        if line1 != line2:
            diff_count += 1
            
            # Cria bloco de diferença com contexto
            block_html = f'<div class="diff-section" id="diff-{diff_count}">\n'
            block_html += f'<div class="diff-header">Diferença #{diff_count} - Linha {i+1}</div>\n'
            block_html += '<div class="diff-content">\n'
            
            # Contexto antes
            start = max(0, i - context_lines)
            for j in range(start, i):
                ctx_line = html.escape(lines1[j].rstrip('\n')) if j < len(lines1) else html.escape(lines2[j].rstrip('\n'))
                block_html += f'<div class="line context">'
                block_html += f'<div class="line-number">{j+1}</div>'
                block_html += f'<div class="line-content">{ctx_line}</div>'
                block_html += '</div>\n'
            
            # Linha com diferença
            if line1 is None:
                block_html += f'<div class="line missing">'
                block_html += f'<div class="line-number">-</div>'
                block_html += f'<div class="line-content">(vazio)</div>'
                block_html += '</div>\n'
                
                block_html += f'<div class="line added">'
                block_html += f'<div class="line-number">{i+1}</div>'
                block_html += f'<div class="line-content">{html.escape(line2)}</div>'
                block_html += '</div>\n'
            elif line2 is None:
                block_html += f'<div class="line removed">'
                block_html += f'<div class="line-number">{i+1}</div>'
                block_html += f'<div class="line-content">{html.escape(line1)}</div>'
                block_html += '</div>\n'
                
                block_html += f'<div class="line missing">'
                block_html += f'<div class="line-number">-</div>'
                block_html += f'<div class="line-content">(vazio)</div>'
                block_html += '</div>\n'
            else:
                tokens1 = tokenize_line(line1)
                tokens2 = tokenize_line(line2)
                colored1, colored2 = compare_tokens_html(tokens1, tokens2)
                
                block_html += f'<div class="line removed">'
                block_html += f'<div class="line-number">{i+1}</div>'
                block_html += f'<div class="line-content">{colored1}</div>'
                block_html += '</div>\n'
                
                block_html += f'<div class="line added">'
                block_html += f'<div class="line-number">{i+1}</div>'
                block_html += f'<div class="line-content">{colored2}</div>'
                block_html += '</div>\n'
            
            # Contexto depois
            end = min(total_lines, i + context_lines + 1)
            for j in range(i + 1, end):
                ctx_line = html.escape(lines1[j].rstrip('\n')) if j < len(lines1) else html.escape(lines2[j].rstrip('\n'))
                block_html += f'<div class="line context">'
                block_html += f'<div class="line-number">{j+1}</div>'
                block_html += f'<div class="line-content">{ctx_line}</div>'
                block_html += '</div>\n'
            
            block_html += '</div>\n</div>\n'
            diff_blocks.append(block_html)
    
    # Adiciona blocos de diferença
    html_content += '\n'.join(diff_blocks)
    
    # Resumo
    summary_class = "success" if diff_count == 0 else "error"
    summary_icon = "✓" if diff_count == 0 else "✗"
    
    html_content += f"""
    </div>
    
    <div class="summary">
        <h2>📊 Resumo da Comparação</h2>
        <div class="stat">Total de linhas (arquivo 1): {len(lines1)}</div>
        <div class="stat">Total de linhas (arquivo 2): {len(lines2)}</div>
        <div class="stat {summary_class}">{summary_icon} Diferenças encontradas: {diff_count}</div>
    </div>
    
    <script>
        let currentDiff = 0;
        const totalDiffs = {diff_count};
        
        function scrollToNextDiff() {{
            if (totalDiffs === 0) return;
            currentDiff = (currentDiff + 1) % totalDiffs;
            const target = document.getElementById('diff-' + (currentDiff === 0 ? totalDiffs : currentDiff));
            if (target) {{
                target.scrollIntoView({{ behavior: 'smooth', block: 'center' }});
                target.style.border = '2px solid #007acc';
                setTimeout(() => {{ target.style.border = '1px solid #3e3e42'; }}, 1000);
            }}
        }}
        
        function scrollToPrevDiff() {{
            if (totalDiffs === 0) return;
            currentDiff = currentDiff === 0 ? totalDiffs - 1 : currentDiff - 1;
            const target = document.getElementById('diff-' + (currentDiff + 1));
            if (target) {{
                target.scrollIntoView({{ behavior: 'smooth', block: 'center' }});
                target.style.border = '2px solid #007acc';
                setTimeout(() => {{ target.style.border = '1px solid #3e3e42'; }}, 1000);
            }}
        }}
        
        function toggleContext() {{
            const contextLines = document.querySelectorAll('.line.context');
            contextLines.forEach(line => {{
                line.style.display = line.style.display === 'none' ? 'flex' : 'none';
            }});
        }}
        
        // Auto-scroll para primeira diferença
        window.addEventListener('load', () => {{
            if (totalDiffs > 0 && document.getElementById('autoScroll').checked) {{
                setTimeout(() => {{
                    const firstDiff = document.getElementById('diff-1');
                    if (firstDiff) firstDiff.scrollIntoView({{ behavior: 'smooth', block: 'center' }});
                }}, 500);
            }}
        }});
        
        // Atalhos de teclado
        document.addEventListener('keydown', (e) => {{
            if (e.key === 'n' || e.key === 'ArrowDown') {{
                e.preventDefault();
                scrollToNextDiff();
            }} else if (e.key === 'p' || e.key === 'ArrowUp') {{
                e.preventDefault();
                scrollToPrevDiff();
            }} else if (e.key === 'c') {{
                toggleContext();
            }}
        }});
    </script>
</body>
</html>
"""
    
    # Salva HTML
    with open(output_path, 'w', encoding='utf-8') as f:
        f.write(html_content)
    
    print(f"✓ Relatório HTML gerado: {output_path}")
    print(f"  Total de diferenças: {diff_count}")
    print(f"  Abra o arquivo no navegador para visualizar")

def main():
    if len(sys.argv) < 3:
        print("Uso: python riscv_trace_diff_html.py <arquivo1> <arquivo2> [output.html]")
        print()
        print("Gera um relatório HTML comparando dois arquivos de trace")
        print()
        print("Exemplo:")
        print("  python riscv_trace_diff_html.py minha_saida.txt saida_esperada.txt relatorio.html")
        sys.exit(1)
    
    file1 = sys.argv[1]
    file2 = sys.argv[2]
    output = sys.argv[3] if len(sys.argv) > 3 else "trace_diff_report.html"
    
    generate_html_report(file1, file2, output)

if __name__ == "__main__":
    main()
