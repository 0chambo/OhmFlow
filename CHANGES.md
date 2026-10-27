# CHANGES — Checkpoint no Histórico (desfazer/refazer com pilhas)

## Resumo

O **[9] Histórico** agora deixa voltar o estoque a qualquer ponto da **sessão atual** e, se voltou demais, avançar de novo. Cada ação que muda o estoque é guardada numa **pilha dinâmica**; voltar desempilha e aplica a inversa, avançar reaplica. Duas pilhas: *desfazer* (ações aplicadas) e *refazer* (ações desfeitas).

Vale só para a sessão: o estoque continua só em memória (`cria_lista()` vazio a cada execução). O desfazer não lê o log, porque o log guarda valores já formatados ("4,7 kΩ", 3 casas) e o Reinicializar não grava o conteúdo apagado; a pilha guarda os dados exatos em Ω.

## Novos módulos

### `src/pilha.h` / `src/pilha.c` — TAD Pilha Dinâmica

- Pilha simplesmente encadeada com nó descritor opaco (`topo`, `tamanho`), no estilo de `lista.c`. Empilhar e desempilhar em O(1).
- Elemento: `Operacao` com `tipo` (`OP_NOVO`, `OP_REMOVER`, `OP_RETIRAR`, `OP_REPOR`, `OP_REINICIALIZAR`), `quando` (`time_t`), `resistencia` e `qtd` (Retirar/Repor), `itens`/`n_itens` (Novo, um item ou lote; Remover; foto do estoque no Reinicializar).
- API: `cria_pilha`, `libera_pilha`, `esvazia_pilha`, `tamanho_pilha`, `pilha_vazia`, `empilha`, `desempilha`, `topo_pilha`, `percorre_pilha` (topo → base), `libera_operacao`.
- Posse da memória: `empilha` assume `op.itens` (e libera se faltar memória para o nó); `desempilha` devolve a posse ao chamador; `esvazia_pilha`/`libera_pilha` liberam os itens de cada nó.
- Códigos: `PILHA_OK`, `PILHA_ERRO`, `PILHA_VAZIA`. Todas as funções aceitam `NULL`.

### `src/checkpoint.h` / `src/checkpoint.c` — desfazer/refazer

- `Checkpoint` = duas pilhas: `desfazer` e `refazer`. Posição atual = tamanho de `desfazer` (0 = início da sessão); total = soma das duas.
- Registro (chamado pelas telas logo depois de `LISTA_OK`): `ck_registra`, `ck_registra_novo` (lote inteiro vira **uma** operação), `ck_registra_remover`, `ck_registra_movimento` (Retirar/Repor), `ck_registra_reinicializar` (tira a foto do estoque **antes** de apagar; lista vazia não registra). Os atalhos copiam os dados.
- Ação nova esvazia `refazer` (o "futuro" desfeito deixa de valer).
- Navegação: `ck_desfazer`, `ck_refazer`, `ck_ir_para(c, li, alvo, aviso, ctx)` (desfaz/refaz até o ponto, chamando `aviso` a cada passo), `ck_posicao`, `ck_total`, `ck_operacao(c, i)` (i-ésima ação em ordem cronológica, aplicada ou desfeita).
- Inversas, todas pela API pública de `lista.h`:

  | Ação | Desfazer | Refazer |
  | --- | --- | --- |
  | Novo (1 ou lote) | `remove_lista` de cada item | `insere_lista_ordenada` de cada |
  | Remover | `insere_lista_ordenada` com a quantidade que tinha | `remove_lista` |
  | Retirar *q* | `repor_estoque(q)` | `retira_estoque(q)` |
  | Repor *q* | `retira_estoque(q)` | `repor_estoque(q)` |
  | Reinicializar | reinsere a foto | `reinicializa_lista` |

- Robustez: inversas com vários itens fazem rollback se um passo falhar, e a operação volta para a pilha de origem. Se faltar memória para registrar uma ação, `ck_descarta` apaga o histórico inteiro, para nunca sobrar inversa que não bata com a lista.
- Códigos: `CK_OK`, `CK_ERRO`, `CK_VAZIO`, `CK_INVALIDO` (ponto fora de 0..total).

## Alterações

### `src/main.c`
- Cria o `Checkpoint` junto com a lista (falha de memória em qualquer um encerra com erro, liberando o outro), entrega às telas com `telas_definir_checkpoint` e libera no fim.
- `tela_historico(estoque)` recebe a lista.

### `src/ui/telas.h` / `src/ui/telas.c`
- `telas_definir_checkpoint(Checkpoint *)`: checkpoint usado pelas telas (NULL desliga o registro; padrão nos testes antigos).
- Registro no checkpoint em cada ramo de sucesso: Novo unitário, Novo em massa (a struct `Lote` agora guarda os itens inseridos em vetor dinâmico, `guarda_no_lote`; o lote é registrado no fim como uma operação), Remover, Retirar, Repor e Reinicializar (foto antes de `reinicializa_lista`).
- `checkpoint_navegar(pos, tecla, total)`: lógica pura das setas (↑/W = mais recente, ↓/S = mais antiga, trava nas bordas).
- `tela_historico(Lista *li)`:
  - tabela do log extraída para `tabela_log` (mesma saída de antes);
  - se a sessão tem ações: no terminal, dica `[C] Voltar a um checkpoint desta sessão`; `C` abre a tela **Checkpoint da sessão** (linha do tempo com `#`, hora, ação, resistor, detalhe e estado — `» atual`, `aplicada`, `desfeita` em cinza; ponto 0 = início), `↑↓`/`WS` movem o marcador, `Enter` vai até o ponto, `Esc` volta sem mudar;
  - sem terminal (e2e/testes): imprime a linha do tempo e pergunta `Ir para o ponto nº (Enter mantém)`; número fora da faixa mostra erro e registra `CHECKPOINT ... PONTO INVÁLIDO`.
  - Feedback: "Checkpoint aplicado — N ações desfeitas/refeitas — Agora no ponto X de Y"; mesmo ponto mostra "Nada a fazer".
- Log: cada passo vira `DESFAZER`/`REFAZER` (alvo = resistor, detalhe = ação original) e o salto vira `CHECKPOINT | ponto 11 → 9 | OK`. Desfazer/refazer **não** entram na pilha.

### `Makefile`
- `SRC_LIB` ganha `src/pilha.c` e `src/checkpoint.c`; `TESTES` ganha `test_pilha` e `test_checkpoint`.
- `make e2e` confere no log as linhas `DESFAZER`, `REFAZER` e `CHECKPOINT`.

## Testes

- `tests/test_pilha.c` (novo, 7 testes): vazia, LIFO, topo sem remover, posse dos itens, esvaziar e reusar, percorrer do topo, segurança com `NULL`.
- `tests/test_checkpoint.c` (novo, 12 testes): cada inversa (Novo, lote em um passo, Remover, Retirar/Repor, Reinicializar), Reinicializar de lista vazia não registra, ação nova limpa refazer, ordem cronológica de `ck_operacao`, `ck_ir_para` para trás e para frente, limites, `NULL`.
- `tests/test_telas.c` (+4 testes): `checkpoint_navegar`; Histórico sem terminal volta ao ponto 1 e avança ao 2; Enter mantém e ponto inválido não mexe; sem ações não pergunta. `tela_historico(NULL)` no teste antigo.
- e2e: depois do lote, `[9]` → ponto 9 (desfaz lote e Reinicializar → `3 tipos · 68 peças`), Listar, `[9]` → ponto 11 (refaz → `3 tipos · 15 peças`), Listar.

Resultado: `make clean; make EXTRA_CFLAGS=-Werror all test e2e cppcheck` — 140 testes unitários ok, e2e ok (70 verificações + log), cppcheck sem avisos.

## Documentação
- `README.md`: Funcionalidades ([9]), Estrutura (novos arquivos e testes) e seção "Checkpoint: voltar e avançar no histórico (pilhas)".
- `AGENTS.md` (local, ignorado pelo git): arquitetura, responsabilidades de Pilha e Checkpoint, UI e Log.
