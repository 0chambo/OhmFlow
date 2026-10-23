# CHANGES

## Janela própria, limpeza de tela robusta, título e ícone

### Problema
- Ao navegar entre telas, o conteúdo da tela anterior reaparecia (`\033[3J` não bastou no Windows Terminal).
- O ícone `ohmflow.ico` não aparecia na aba/janela.
- O programa rodava 100% dentro do Windows Terminal.

### Mudanças
- `Makefile`: novo `LDFLAGS_APP = -mwindows` (somente Windows), usado apenas no link de `dist/ohmflow.exe`. Testes unitários continuam como console.
- `src/main.c`: nova `ligar_console()` (somente `_WIN32`), chamada antes de `ui_init()`:
  - saída em pipe/arquivo (e2e/CI): não altera nada;
  - lançado de terminal: `AttachConsole(ATTACH_PARENT_PROCESS)`;
  - duplo clique: `AllocConsole()` abre janela de console própria;
  - religa `stdout`/`stderr`/`stdin` e os handles Win32 via `CONOUT$`/`CONIN$`.
- `src/ui/ui.c`:
  - `ui_limpar_tela`: no Windows preenche o buffer inteiro com espaços (`FillConsoleOutputCharacterA`/`Attribute`) e posiciona o cursor em (0,0), antes do `\033[3J\033[2J\033[H`.
  - `ui_init`: `SetConsoleTitleA("OhmFlow")` e ícone em runtime (`WM_SETICON`, recurso ID 1 de `ohmflow.rc`).

### Limitação conhecida
- O Windows Terminal usa o ícone do perfil e ignora o do processo; o ícone só aparece em janela de console nativa (Host de Console do Windows).

### Verificação
- `make`, `make test` e `make e2e` (31 verificações) passam.
- Teste manual por duplo clique: sem bleeding entre telas, título e ícone corretos.
