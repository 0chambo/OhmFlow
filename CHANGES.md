# CHANGES

## fix(main): volta ao subsistema console no Windows (issue #11)

### Problema

Desde o commit `cef55c7`, o executável do Windows era linkado com `-mwindows` (subsistema GUI). Isso foi feito para o duplo clique abrir uma janela de console própria, e `ligar_console()` reaproveitava o console do terminal pai com `AttachConsole`.

O efeito colateral é que cmd e PowerShell não aguardam executáveis GUI:
- O prompt voltava na hora, enquanto o programa ainda lia e escrevia no mesmo console.
- A saída se misturava com o prompt, e o programa disputava as teclas com o shell.
- `make run` e scripts que dependem do código de saída também eram afetados.

Confirmado num terminal real: um script `.ps1` que executa `ohmflow.exe`, espera 2 s e imprime `===SHELL VOLTOU===` mostrou `===SHELL VOLTOU apos 2.0s===` e o prompt `PS>`. Enquanto isso, o programa ainda estava na tela de boas-vindas, esperando Enter.

### Decisão

O executável volta a usar o subsistema console (CUI). Foi a opção escolhida entre três alternativas:
- voltar ao console (escolhida);
- gerar dois executáveis, um console e um GUI;
- relançar o programa no `conhost.exe` quando aberto por duplo clique.

Consequência: no duplo clique, o Windows abre o programa no terminal padrão do sistema. No Windows 11 esse terminal costuma ser o Windows Terminal.

### Alterações

- [Makefile](Makefile):
  - Removido `LDFLAGS_APP = -mwindows` e o uso dessa variável no link de `dist/ohmflow.exe`.
  - Nova variável `CHECA_SUBSISTEMA`. No Windows ela roda `objdump -p dist/ohmflow.exe | grep -q '(Windows CUI)'`; fora do Windows é `true`.
  - `make e2e` agora começa por essa checagem e falha com "E2E FALHOU: dist/ohmflow.exe nao usa o subsistema console (CUI)" se o executável voltar a ser GUI.
- [src/main.c](src/main.c):
  - Removidos `ligar_console()` (`AttachConsole`, `AllocConsole`, reabertura de `CONOUT$`/`CONIN$`), a chamada em `main` e o `#include <windows.h>`.
  - O arquivo voltou a ser idêntico ao que era antes de `cef55c7`.
- Continuam valendo o ícone (`windres`), o título da janela (`SetConsoleTitleA`) e a limpeza de tela via Win32 em [src/ui/ui.c](src/ui/ui.c), que não dependem do subsistema GUI.

### Testes

- Primeiro adicionei a checagem de subsistema no `make e2e`. Antes da correção ela falhou como esperado (`Subsystem 00000002 (Windows GUI)`).
- Depois da correção, `objdump` mostra `Subsystem 00000003 (Windows CUI)` e a checagem passa.
- Terminal real (PowerShell, script `.ps1`): o shell passou a aguardar o programa. Após 4 s ainda não havia `===SHELL VOLTOU===`, e o programa continuava esperando Enter.

### Validação

- Windows 11, gcc 16.2.0 (MSYS2): `make clean; make EXTRA_CFLAGS=-Werror all test e2e` passou sem warnings, com `E2E OK (31 verificacoes)`.
- Ubuntu 24.04 (Docker): `make EXTRA_CFLAGS=-Werror all test e2e` passou. Fora do Windows, `CHECA_SUBSISTEMA` é `true`.
