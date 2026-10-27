# CHANGES — telas 100% centralizadas

## Resumo

Todas as telas passam a ficar no centro do terminal, na horizontal e na vertical: menu (já era), boas-vindas (já era), as telas de cada função (Novo, Remover, Listar, Consultar, Retirar, Repor, Reinicializar, Diferencial) e o encerramento. Antes, as telas das funções saíam coladas à esquerda e no topo.

## Como funciona

Novo **modo centralizado** em `src/ui/ui.c`, desligado por padrão e ligado em `main.c` só quando o programa roda num terminal (`ui_definir_centralizado(ui_interativo())`). Em modo não interativo (e2e, CI, testes) a saída é a mesma de antes.

Com o modo ligado:

- **Caixas** (`ui_caixa`, incluindo título das telas, `ui_caixa_feedback` e a caixa da Consulta): centralizadas pela própria largura (usam `ui_caixa_centralizada`).
- **Tabelas** (`ui_tabela_borda` / `ui_tabela_linha`): recuo calculado pela largura total da tabela (`recuo_tabela`), em vez dos 2 espaços fixos.
- **Dicas, mensagens, prompts e "Pressione Enter"** (`ui_dica`, `ui_mensagem`, `ui_ler_linha`, `ui_confirmar`, `ui_pausar`): alinhados ao texto de um bloco central de `UI_LARGURA + 2` colunas (mesma largura das caixas); uma linha mais larga que o bloco é centralizada pela própria largura (`recuo_texto`), para não vazar para a direita.
- **Vertical**: `ui_espaco_vertical(altura)` imprime metade da sobra de linhas antes do conteúdo. `abre_tela(titulo, altura)` em `telas.c` recebe uma altura estimada por tela (constantes `ALTURA_*`: título + dicas + perguntas + feedback + pausa); Listar soma uma linha por tipo. Se a tela não cabe, não há espaço extra.

## API nova (`src/ui/ui.h`)

- `void ui_definir_centralizado(int ativo)` / `int ui_centralizado(void)`
- `int ui_margem(void)`: recuo do bloco central (0 se desligado)
- `void ui_espaco_vertical(int altura)`

## Arquivos

- `src/ui/ui.h`, `src/ui/ui.c`: modo centralizado, `recuo_texto`, `recuo_tabela`.
- `src/ui/telas.c`: `abre_tela` com altura e espaço vertical; constantes `ALTURA_*`; encerramento centralizado na vertical.
- `src/main.c`: liga o modo quando interativo.
- `tests/test_ui.c`: 10 testes novos (desligado por padrão, margem do bloco, caixa normal e larga, dica normal e larga, mensagem, tabela centralizada e recuo antigo sem o modo, espaço vertical).

## Validação

- `make clean; make EXTRA_CFLAGS=-Werror all test e2e cppcheck`: tudo passa (test_ui: 28 testes; e2e 49 verificações + log).
- Não conferido visualmente em terminal interativo nesta sessão: rodar `make run` no Windows Terminal e passar por cada tela.
