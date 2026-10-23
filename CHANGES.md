# Detalhamento de Alterações — Tela de Boas-Vindas (OhmFlow)

## 1. Objetivo

Corrigir e polir a tela de boas-vindas: remover valores `(null)`, eliminar caixa ao redor do subtítulo, restaurar logo laranja sólido, remover animação, centralizar vertical e horizontalmente, e aumentar o logo.

## 2. Arquivos Modificados

### `src/ui/telas.c`

- **Corrigido `(null)` × 2**: `CLAUDE_LOGO_FRONT[6]` declarado com 6 slots mas só 5 strings inicializadas — índice `[5]` era NULL. Renomeado para `LOGO[5]` com tamanho correto.
- **Corrigido `(null)` × 2 nos subtítulos**: `TEXTOS[3]` tinha 1 string, loop `for (i = 1; i < 3)` acessava índices NULL. Substituído por variável estática simples.
- **Removida caixa ao redor do subtítulo**: `╭─╮│╰─╯` removidos; "Estoque de Resistores" vira subtítulo plain text com cor `DIM`, centralizado.
- **Removida animação**: código `Sleep/_kbhit/_getch` eliminado. Espera Enter via `fgets` silencioso sem imprimir texto extra.
- **Includes limpos**: `<conio.h>` removido (sem uso); `<windows.h>` mantido para detecção de altura de terminal.
- **Centralização vertical**: lê altura real do terminal via `GetConsoleScreenBufferInfo` no Windows (fallback 24); calcula padding de linhas no topo.
- **Logo maior**: substituído para "big" figlet variant — O usa `/ __ \` e `\____/` (mais largo que `/ _ \` e `\___/`); array agora tem 6 linhas (~54 colunas vs 49 antes).
- **Logo contínuo**: removidas linhas em branco entre cada row do art que quebravam visualmente as letras.
- **Restaurado laranja sólido**: revertido gradiente amarelo→vermelho→sombra de tentativa anterior; todas as linhas voltam para `UI_COR_LARANJA`.

## 3. Validação

- Build limpo: zero erros, zero warnings com `-Wall -Wextra -pedantic`.
- Smoke test via pipe confirma: sem `(null)`, sem caixa, logo contínuo, subtítulo e mensagem centralizados.
