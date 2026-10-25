# CHANGES

## fix(telas): menu interativo trata EOF, ESC isolado e interrupção (issue #9)

### Problema

No menu interativo em POSIX (Linux, WSL, macOS), `ler_tecla()` e `tela_menu()` tinham três falhas:

1. **EOF e Ctrl+D.** Quando o terminal fechava, `getchar()` devolvia `EOF` e o menu era redesenhado em loop infinito, com 100% de CPU. Em modo não canônico o Ctrl+D chega como o byte 4, que também era ignorado.
2. **ESC sozinho.** Depois do byte 27 havia um `getchar()` bloqueante esperando `[`. O programa ficava parado e ainda engolia a tecla seguinte: ESC seguido de `0` não saía do programa.
3. **Terminal desprotegido.** O retorno de `tcgetattr` não era checado. Um Ctrl+C (SIGINT) no menu deixava o terminal sem eco e sem modo canônico, e o cursor escondido.

### Correção

[src/ui/telas.h](src/ui/telas.h):
- Novas teclas: `KEY_FIM` (fim da entrada) e `KEY_IGNORADA` (ESC isolado, Alt+tecla ou sequência desconhecida).
- Nova constante `TECLA_TIMEOUT` (-2): retorno da fonte de bytes quando nada chega dentro do prazo.
- Nova função `decodifica_tecla(proximo, ctx)`, que converte os bytes de uma tecla em caractere ou `KEY_*`. É uma função pura, testável sem terminal.

[src/ui/telas.c](src/ui/telas.c):
- `decodifica_tecla`:
  - `EOF` e Ctrl+D viram `KEY_FIM`.
  - Depois de ESC, os bytes seguintes são lidos com prazo curto. Sem `[` ou `O` logo em seguida, o resultado é `KEY_IGNORADA`.
  - Reconhece `ESC [ A-D` (CSI) e `ESC O A-D` (SS3, modo "application cursor").
- `le_byte_terminal` (POSIX):
  - Lê com `read()` direto no descritor, sem o buffer do stdio.
  - O primeiro byte usa `VMIN=1`, que é bloqueante. Os bytes seguintes usam `VMIN=0, VTIME=1`, ou seja, esperam no máximo 0,1 s.
  - Repete a leitura em `EINTR`. `read` igual a 0 em modo bloqueante, ou erro, vira `EOF`: terminal fechado.
- `ler_tecla` (POSIX):
  - Checa o retorno de `tcgetattr`. Se falhar, usa `getchar()` comum, e `EOF` ainda encerra.
  - Guarda o modo original em `term_original` e marca `term_raw` enquanto está em modo raw.
- `ler_tecla` (Windows): um código estendido desconhecido (prefixo 0 ou 224) agora vira `KEY_IGNORADA`, em vez de o segundo byte ser devolvido como tecla comum.
- `protege_terminal` e `restaura_terminal_e_sai`:
  - Tratadores de SIGINT, SIGTERM, SIGHUP e SIGQUIT, instalados uma única vez ao entrar no menu interativo.
  - Restauram o modo do terminal e mostram o cursor, usando só chamadas seguras em sinal (`tcsetattr`, `write`). Depois re-emitem o sinal com o tratador padrão.
- `mostra_cursor`: envolve `ui_cursor` e registra em `cursor_oculto` se o cursor está escondido.
- `tela_menu`: `KEY_FIM` encerra como "Sair" (`OPCAO_SAIR`), e o encerramento e a liberação de memória seguem normalmente.
- `le_byte_terminal` ganhou `cppcheck-suppress constParameterCallback`, porque a assinatura é fixada pelo callback de `decodifica_tecla`.

### Testes

Novos testes em [tests/test_telas.c](tests/test_telas.c), usando uma fonte de bytes falsa que também conta leituras bloqueantes indevidas:

| Teste | O que verifica |
| --- | --- |
| `teste_tecla_comum` | `w`, Enter e dígito passam direto. |
| `teste_tecla_setas` | CSI e SS3 viram `KEY_UP`, `KEY_DOWN`, `KEY_RIGHT` e `KEY_LEFT`. |
| `teste_tecla_fim_da_entrada` | `EOF` e Ctrl+D viram `KEY_FIM`. |
| `teste_tecla_esc_isolado_nao_trava` | ESC sozinho vira `KEY_IGNORADA` sem nenhuma leitura bloqueante extra. |
| `teste_tecla_esc_sequencia_desconhecida` | Alt+x, `ESC [ Z` e `ESC [` cortado viram `KEY_IGNORADA`. |
| `teste_navegar_ignora_teclas_especiais` | `navegar_menu` mantém a seleção com `KEY_IGNORADA` e `KEY_FIM`. |

### Validação

- **Windows 11, gcc 16.2.0 (MSYS2):** `make clean; make EXTRA_CFLAGS=-Werror all test e2e` passou sem warnings. São 19 testes em `telas` e `E2E OK (31 verificacoes)`.
- **Ubuntu 24.04 (container Docker):** passaram `make EXTRA_CFLAGS=-Werror all test e2e`, `make asan` (ASan/UBSan) e `make cppcheck`.
- **Pseudo-terminal real (Ubuntu, script Python com `pty`):** os 8 cenários abaixo passaram.
  - Ctrl+D encerra com código 0.
  - ESC sozinho seguido de `0` encerra.
  - Setas mais Enter chegam em "Sair".
  - Fechar o terminal encerra o programa.
  - O menu fica em modo raw enquanto espera tecla.
  - SIGINT encerra pelo sinal.
  - Depois do SIGINT, `ICANON` e `ECHO` voltam ao normal.
  - Depois do SIGINT, o cursor é mostrado de novo (`\033[?25h`).
