# CHANGES

Correção das issues #29, #28 e #26 (interface do terminal).

## #29: dava para digitar nos prompts de "Pressione Enter"

**Causa.** `ui_pausar` (`src/ui/ui.c`) e `tela_boas_vindas` (`src/ui/telas.c`) liam a linha com `fgets`. No modo canônico do console, tudo o que se digitava era ecoado na tela até o Enter.

**Correção.**

- Nova `ui_esperar_enter()` (exposta em `ui.h`): espera Enter **sem eco**.
  - Windows: lê com `_getch()` (não ecoa) até `'\r'`.
  - POSIX: desliga `ICANON | ECHO` com `termios` (VMIN 1, VTIME 0), lê byte a byte até `'\n'` e restaura o modo original. Sem controle do terminal, descarta a linha com `getchar`.
- O laço foi extraído para `ui_espera_enter_de(proxima, ctx)` (também em `ui.h`), que recebe a fonte de teclas por callback e pode ser testado sem console:
  - termina em `'\r'` ou `'\n'` (devolve 1) ou em EOF (devolve 0, não trava);
  - prefixo `0`/`224` (tecla estendida do `_getch`: setas, F1...) descarta também o byte seguinte, para que o código da tecla não seja confundido com Enter.
- `ui_pausar` e a tela de boas-vindas passam a usar `ui_esperar_enter()`. `ui_pausar` imprime uma quebra de linha depois do Enter, já que o terminal não ecoa mais.
- Modo não interativo (e2e, CI, redirecionamento) inalterado: `ui_pausar` continua retornando na hora e a boas-vindas não espera.
- `ui.c` passa a incluir `<conio.h>` (Windows) e `<errno.h>`/`<termios.h>` (POSIX).

## #28: valores em kΩ ciano e MΩ azul na tela Listar

**Causa.** `linha_estoque` coloria a coluna Resistência com `cor_grandeza` (mΩ magenta, kΩ ciano = mesma cor da borda, MΩ azul).

**Correção.** A coluna Resistência sai no texto padrão (branco); só a moldura da tabela é colorida. `cor_grandeza` foi removida. A quantidade 0 continua em vermelho.

## #26: UX da tela Novo, Consultar e Reinicializar

### a) "Inserir" vira "Novo"

- Rótulo do menu: `[1] Novo` (`desenha_menu`).
- Título da tela: "Novo resistor".
- Listar com estoque vazio: "Estoque vazio. Use [1] Novo."
- Nomes internos (`OPCAO_INSERIR`, `tela_inserir`) mantidos, para não mexer em `main.c`.

### b) Consultar sem a linha de debug

Removida a linha "Busca: N nós visitados" de `tela_consultar`. `lista_passos` continua na API da lista (usada pelos testes de `test_lista.c`).

### c) Aviso antes de Reinicializar

Com o estoque não vazio, `tela_reinicializar` mostra antes da confirmação uma caixa de atenção com "Tem certeza disso, você está apagando sua base de dados." e depois pergunta "Apagar TODOS os N tipos (M peças)? (s/n)". Com o estoque vazio nada muda (sem aviso).

### d) Inserção em massa na tela Novo

- A **primeira linha** decide o modo:
  - só a resistência (`4k7`): fluxo unitário de antes (pergunta a quantidade, avisa "já cadastrado - use [6] Repor" se for duplicado, volta ao menu);
  - resistência e quantidade (`4k7 50`): modo em massa.
  - linha inválida: "Valor inválido..." e nova tentativa (até 3, como antes); Enter vazio ou fim da entrada cancelam.
- **Modo em massa** (`inserir_em_massa`): insere o primeiro item e lê uma linha por item (prompt "Resistência [quantidade]:") até Enter vazio ou fim da entrada. Cada linha recebe um feedback curto:
  - `✔ 10 kΩ inserido (5 un.)`;
  - `⚠ 10 kΩ já cadastrado - ignorado` (já no estoque ou repetido no próprio lote);
  - `✖ Linha inválida: "lixo 2"`.
  - Uma linha só com a resistência pede a quantidade em separado (`ler_quantidade`); se a resistência já estiver cadastrada, conta como duplicado sem perguntar. Quantidade cancelada conta como inválido.
  - Duplicados e inválidos não abortam. Ao final, caixa de resumo "Inserção em massa concluída" com `Inseridos : X`, `Duplicados: Y`, `Inválidos : Z` (SUCESSO se inseriu algo, ATENÇÃO caso contrário).
- **Parse puro e testável** em `telas.h`: `int parse_item(const char *s, Resistor *r)` devolve `ITEM_COMPLETO`, `ITEM_SO_RESISTENCIA` (quantidade 0) ou `ITEM_INVALIDO` (vazio, campo a mais, resistência inválida/fora da faixa, quantidade não inteira, negativa ou acima de `INT_MAX`). Reaproveita `parse_resistencia` e a mesma validação de `ler_quantidade`, agora extraída para `parse_quantidade` (static em `telas.c`).
- A mensagem de valor inválido virou a macro `MSG_VALOR_INVALIDO`, usada por `ler_resistencia` e pela tela Novo.

## Suporte a testes

- `ui_definir_interativo(int)` em `ui.h`: força o modo terminal ligado/desligado, para os testes chamarem telas sem pausa nem limpeza de tela mesmo quando rodam num console.

## Testes

- **`tests/test_novo.c`** (novo, adicionado a `TESTES` no `Makefile`), 15 testes. As telas são alimentadas por `stdin` redirecionado com `freopen` para um arquivo temporário em `build/`:
  - `parse_item`: item completo, só resistência, inválidos (vazio, campo a mais, quantidade ruim/negativa/acima de `INT_MAX`, fora da faixa, `NULL`) e limite `INT_MAX`;
  - tela Novo: fluxo unitário, duplicado no unitário, lote misto (3 inseridos, 2 duplicados incluindo repetido no lote, 2 inválidos, linha só com resistência), fim da entrada encerrando o lote, primeira linha inválida seguida de lote;
  - menu com `[1] Novo` e sem "Inserir"; Listar vazio sugerindo `[1] Novo`;
  - Consultar sem "Busca:"/"visitado";
  - Reinicializar: aviso aparece antes da pergunta e "n" não apaga; estoque vazio sem aviso;
  - Listar com cor ligada: valores de mΩ, kΩ e MΩ sem código de cor, sem azul/magenta, quantidade 0 ainda vermelha.
- **`tests/test_ui.c`**: 4 testes de `ui_espera_enter_de` (descarta texto e para no Enter, aceita `'\n'`, tecla estendida `0`/`224` com segundo byte 13 não conta como Enter, EOF não trava).
- **e2e** (`tests/e2e/`):
  - `esperado.txt`: confere `[1] Novo` no menu, sem "Busca: 3 nós visitados", com o aviso antes de "Apagar TODOS" e "Estoque vazio. Use [1] Novo.";
  - nova etapa de inserção em massa depois do Reinicializar: `10k 5`, `22k 3`, `10k 1` (duplicado), `lixo 2` (inválido), `33k` + `7`, Enter vazio; resumo 3/1/1, Listar com 10 kΩ, 22 kΩ e 33 kΩ e estoque final "3 tipos · 15 peças".

Validação: `make clean; make EXTRA_CFLAGS=-Werror all test e2e cppcheck` passa (E2E OK, 49 verificações; cppcheck sem avisos).

## Documentação

- `README.md`: tabela de funcionalidades com "Novo" (um por vez ou em massa) e aviso no Reinicializar; nova seção "Novo: um item ou vários de uma vez" com exemplo; removido o exemplo "Busca: 2 nós visitados" da tela Consultar (a contagem segue documentada como `lista_passos`); árvore de `tests/` com `test_telas.c` e `test_novo.c`.
- `AGENTS.md` é ignorado pelo git e não existe neste worktree: as edições (tabela com "Novo", `test_novo.c` na árvore, `lista_passos` fora da tela) ficam para aplicar no checkout principal.
