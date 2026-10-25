# CHANGES

## fix(ui): tamanho real do terminal no POSIX e plural no Repor (issues #16, #17, #18)

### Problema

- **#16: `ui_colunas_terminal` sempre devolvia 80 fora do Windows.** A função só consultava `GetConsoleScreenBufferInfo` (`_WIN32`) e não tinha ramo POSIX. Em Linux/macOS, o menu, o logo e a dica eram centralizados como se o terminal tivesse 80 colunas. `ui_linhas_terminal`, ao lado, já usava `ioctl(TIOCGWINSZ)`.
- **#17: o feedback de Repor tinha "unidades" fixo.** Retirar tudo e repor 1 mostrava `470 mΩ: 0 → 1 unidades`. `tela_retirar` já usava `plural()`.
- **#18: a tela de boas-vindas reimplementava a altura do terminal.** Havia um bloco `#ifdef _WIN32` próprio, e fora do Windows `rows` ficava fixo em 24. O `cppcheck-suppress knownConditionTrueFalse` só existia por causa disso, já que no Linux `topo < 1` nunca era verdadeiro.

### Alterações

[src/ui/ui.c](src/ui/ui.c):
- Novo `static int tamanho_console(int *cols, int *rows)`, um único ponto para medir o terminal:
  - Windows: `GetConsoleScreenBufferInfo` + `srWindow`;
  - POSIX: `ioctl(UI_FILENO(saida()), TIOCGWINSZ, &ws)`, rejeitando `ws_col`/`ws_row` zerados.
  - Devolve 0 sem tocar nos ponteiros quando a saída não é terminal ou a medição falha.
- `ui_colunas_terminal` e `ui_linhas_terminal` passam a chamar o helper, com os mesmos padrões de antes (80×24) e os mesmos mínimos (40 colunas, 10 linhas).
- As 2 cópias de `GetConsoleScreenBufferInfo` viraram 1. Saiu também o `cppcheck-suppress knownConditionTrueFalse` de `ui_colunas_terminal`, que não é mais necessário.

[src/ui/telas.c](src/ui/telas.c):
- `tela_boas_vindas`: `int rows = ui_linhas_terminal();` substitui o `rows = 24` e o bloco `#ifdef _WIN32`. A supressão do cppcheck foi removida.
- `#include <windows.h>` saiu de telas.c, porque o arquivo não usa mais nada da API Win32 (o `<conio.h>` do `_getch` continua).
- `tela_repor`: a mensagem passa a ser `"%s: %d %s %d %s"` com `plural(atual + qtd, "unidade", "unidades")`.

[tests/e2e/entrada.txt](tests/e2e/entrada.txt) e [tests/e2e/esperado.txt](tests/e2e/esperado.txt):
- Novo roteiro antes da reinicialização: Retirar 5 de `470m` (zera o estoque) e depois Repor 1.
- Linhas esperadas:
  - `Retirada concluída - estoque zerado`
  - `470 mΩ: 5 → 0 unidades`
  - `Reposição concluída`
  - `470 mΩ: 0 → 1 unidade ` (com espaço no fim, para que `1 unidades` não satisfaça a busca por substring do e2e)
- A confirmação da reinicialização muda de `72 peças` para `68 peças`.
- O e2e passou de 31 para 35 verificações.

### Validação

- **Teste primeiro:** antes da correção, `make e2e` falhou em `nao encontrado (em ordem): 470 mΩ: 0 → 1 unidade`, e a saída mostrava `0 → 1 unidades`.
- **Windows 11 (MSYS2 UCRT64):** `make clean; make EXTRA_CFLAGS=-Werror all test e2e` passou sem warnings. Resultado: 18 + 11 + 14 + 19 testes, nenhuma falha, e `E2E OK (35 verificacoes)`.
- **Linux (Docker `gcc:14`):**
  - `make EXTRA_CFLAGS=-Werror all test e2e` e `make asan` passaram.
  - Medição real num TTY (`docker run -t`): com `stty cols 200 rows 50` saiu `cols=200 rows=50`; com `stty cols 30 rows 5` saíram os mínimos `cols=40 rows=10`. Antes, as colunas eram sempre 80.
- **cppcheck:** o cppcheck 2.17 da imagem `gcc:14` sai com 1 também no HEAD sem estas mudanças (é diferença de versão, só mensagens `information`). A versão do `ubuntu-latest` passa no CI, como no PR #19. A validação final fica com o job Ubuntu deste PR.
