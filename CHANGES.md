# Detalhamento de Alterações — Navegação por Teclado no Menu + Borda Laranja (OhmFlow)

## 1. Objetivo

Tornar o menu principal navegável via teclado (WASD e setas), com cursor visual destacando a opção selecionada em laranja, mantendo acesso rápido por dígito. Também alinha a borda do menu com a paleta laranja já existente.

## 2. Arquivos Modificados

### `tests/test_telas.c` (novo)
- 13 testes cobrindo `navegar_menu()` (navegação vertical, horizontal, bordas, wrap-around e teclas ignoradas) e rendering do menu com destaque laranja e borda laranja.

### `src/ui/telas.h`
- Constantes `KEY_UP`, `KEY_DOWN`, `KEY_LEFT`, `KEY_RIGHT` (256–259) para navegação cross-plataforma.
- Protótipo público `navegar_menu(int selecao, int tecla) → int`.
- Protótipo público `desenha_menu(Lista *li, int selecao)`.

### `src/ui/telas.c`
- Incluído `<termios.h>` + `<unistd.h>` em plataformas não-Windows para raw terminal.
- **`ler_tecla()`** (static): lê uma tecla sem echo. Windows via `_getch()` + prefixo 224/0; POSIX via `termios` raw mode + sequência ESC para setas.
- **`navegar_menu(int selecao, int tecla)`**: lógica pura de navegação em grid 2×4+Sair. WASD normalizado para KEY_*. Fácil de testar sem I/O.
- **`desenha_menu(Lista *li, int selecao)`**: opção selecionada destacada com `UI_COR_LARANJA` + `UI_COR_NEGRITO`; borda trocada de `UI_COR_BORDA` para `UI_COR_LARANJA`; dica atualizada para "↑↓←→/WASD Navegar · [0-8] Acesso rápido · [Enter] Confirmar".
- **`tela_menu()`**: modo interativo usa loop `ler_tecla()` com seleção inicial em 1 (Inserir); dígito '0'-'8' retorna direto; Enter confirma seleção atual; WASD e setas movem cursor. Modo não interativo (`!ui_interativo()`) preserva fluxo original `fgets`+`strtol` para e2e e CI.

### `Makefile`
- Adicionado `test_telas` à variável `TESTES`; target `test` agora cobre os 4 binários.

## 3. Validação e Testes

- `test_lista.exe`: 17 testes, 0 falhas, 98 asserts.
- `test_formatacao.exe`: 11 testes, 0 falhas, 79 asserts.
- `test_ui.exe`: 15 testes, 0 falhas, 50 asserts.
- `test_telas.exe`: 13 testes, 0 falhas, 37 asserts.
- E2E: 31 verificações OK (modo não interativo preservado).
