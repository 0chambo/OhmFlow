# CHANGES — Banner ANSI Shadow, menu sem flicker e barras de estoque

## Resumo

Três melhorias visuais na interface de terminal do OhmFlow:

1. **Boas-vindas com banner ANSI Shadow** — "OHMFLOW" em blocos cheios `█` com degradê vertical (creme → laranja, cor 24 bits) e sombra em traço duplo (`╔═╗║╚╝`) em laranja escuro, no estilo "Game Boy".
2. **Menu e checkpoint sem piscar** — as setas não limpam mais a tela a cada tecla; o quadro novo é desenhado por cima do anterior e sai numa escrita só.
3. **Barras de proporção** — Listar ganhou a coluna **Estoque** e Consultar uma linha com a barra e a porcentagem de cada resistor no total de peças.

Pixel-art do resistor (`resistor_arte`) não mudou: já usa meio-bloco `▀▄` com cor 24 bits.

## Detalhes

### Banner (`src/ui/telas.c`, `src/ui/telas.h`)

- Nova tabela `BANNER[6]` (63 colunas) com "OHMFLOW" na fonte figlet ANSI Shadow.
- `int banner_linha(int i, char *buf, size_t n)` (pública): devolve a linha `i` colorida por caractere. `█` recebe a cor do corpo, interpolada da linha 0 (`#FFE8B0`) até a penúltima (`#FF8700`); caracteres de caixa recebem a sombra (`#8A4A00`). Só emite `\033[38;2;R;G;Bm` quando a cor muda e termina com reset. Sem cor, devolve o texto puro. Índice inválido devolve 0 e buffer vazio.
- `int banner_disponivel(int colunas)`: 1 se cabe (≥ `BANNER_COLUNAS + 2`); sempre 0 com `-DOHM_ASCII`.
- `tela_boas_vindas` dividida em `boas_vindas_banner` (terminal interativo e largo: banner + `►►► ESTOQUE DE RESISTORES // ESTRUTURA DE DADOS ◄◄◄` + `[ LISTA DINÂMICA ORDENADA ]` + caixa dupla laranja `► [ PRESSIONE ENTER PARA COMEÇAR ] ◄`) e `boas_vindas_texto` (logo "big" antigo — usado sem terminal, em terminal estreito e com `OHM_ASCII`; o e2e continua igual).
- Antes de esperar o Enter, descarta teclas apertadas durante a abertura.

### Redesenho sem flicker (`src/ui/ui.c`, `src/ui/ui.h`, `src/ui/telas.c`, `src/main.c`)

- Novas funções `ui_quadro_inicio`, `ui_quadro_fim` e `ui_quadro_reset`:
  - `ui_quadro_inicio`: leva o cursor ao topo (`\033[H`) sem limpar. Limpa a tela inteira só no 1º quadro, depois de `ui_limpar_tela`/`ui_quadro_reset` ou quando o terminal muda de tamanho.
  - `ui_quadro_fim`: `\033[J` (apaga sobras abaixo) e um único `fflush`.
  - Dentro do quadro, os `fflush` internos de caixas, mensagens, linha respondida e espaço vertical são adiados (`descarrega`).
- `ui_bufferizar_saida()`: chamada por `main` logo após `ui_init`; em terminal interativo coloca stdout em buffer completo (64 KiB), para cada quadro sair numa escrita.
- Para não travar nada com o buffer: `ui_limpar_tela` dá `fflush` antes do `FillConsoleOutput*`, `ui_esperar_enter` e `ler_tecla` dão `fflush` antes de ler.
- `tela_menu` (modo interativo) e `checkpoint_interativo` passam a usar quadros em vez de `ui_limpar_tela` a cada tecla. Telas de função continuam limpando uma vez ao abrir.
- Decisão: não foi feito redesenho só das linhas alteradas (delta). Com o quadro inteiro sobrescrito numa escrita só o flicker já some; delta exigiria guardar estado anterior e quebraria no resize.

### Cursor e entrada (`src/ui/ui.c`, `src/ui/telas.c`)

- `ui_descartar_entrada()`: `FlushConsoleInputBuffer` no Windows, `tcflush(TCIFLUSH)` no POSIX; sem terminal não faz nada. Chamada ao entrar no menu e na boas-vindas (seta segurada ou Enter a mais não vazam para a tela seguinte).
- Ctrl+C com cursor escondido agora reexibe o cursor também no Windows (`WriteFile` direto no console dentro do tratador de sinal).
- `atexit(mostra_cursor_ao_sair)` registrado em `protege_terminal`.

### Barras de estoque (`src/ui/ui.c`, `src/ui/ui.h`, `src/ui/telas.c`)

- `ui_barra(fracao, largura, cor, buf, n)`: barra com largura visível fixa; parte cheia `█` + blocos de 1/8 (`▏▎▍▌▋▊▉`) na cor pedida, resto `░` apagado (DIM). Fração saturada em 0..1 (NaN = 0). Com `OHM_ASCII`: `#` e `.` arredondando para célula inteira. Nunca estoura o buffer.
- Macros `UI_BLOCO_CHEIO` / `UI_BLOCO_VAZIO` (Unicode ou ASCII).
- Listar: tabela com 4ª coluna **Estoque** (`BARRA_LISTAR` = 10 células + porcentagem). `linha_estoque` recebe `LinhaEstoque { indice, total }`. Porcentagem em vermelho com quantidade 0.
- Consultar: a caixa do resultado ganhou a linha `barra (16 células) NN% do estoque`.
- Auxiliares `fatia` e `celula_fatia` compartilhados pelas duas telas.

### Testes

- `tests/test_ui.c`: `teste_barra_largura_fixa`, `teste_barra_extremos`, `teste_barra_oitavos`, `teste_barra_cores`, `teste_barra_buffer_pequeno`, `teste_quadro_fora_do_terminal_nao_emite`.
- `tests/test_telas.c`: `teste_banner_sem_cor_literal`, `teste_banner_largura`, `teste_banner_cores`, `teste_banner_linha_invalida`, `teste_banner_fallback`, `teste_listar_mostra_porcentagem`.
- Validação: `make clean; make EXTRA_CFLAGS=-Werror all test e2e cppcheck` verde (test_ui 36, test_telas 34 testes; E2E 70 verificações). `test_ui` e `test_telas` também compilam e passam com `-DOHM_ASCII`.

### Documentação

- `AGENTS.md` (seção UI) descreve banner, quadros, buffer de saída, descarte de entrada, cursor e barras.

## Arquivos alterados

- `src/main.c`
- `src/ui/ui.h`, `src/ui/ui.c`
- `src/ui/telas.h`, `src/ui/telas.c`
- `tests/test_ui.c`, `tests/test_telas.c`
- `AGENTS.md` (local, ignorado pelo git)
