# CHANGES

## refactor(lista): nó descritor, busca por ponteiro para ponteiro e chave canônica

### Problema

A revisão de código encontrou três fraquezas no núcleo da lista:

1. **Laço duplicado 3 vezes.** O mesmo "avança enquanto menor e diferente" estava em `busca`, `insere_lista_ordenada` e `remove_lista`. Inserir e remover ainda carregavam um ponteiro `ant` e um caso especial para o início da lista.
2. **Contagens em O(n).** `tamanho_lista` e `total_pecas` percorriam a lista inteira, e são chamadas a cada redesenho do menu, em Listar, Reinicializar e no encerramento.
3. **Chave não canônica.** `parse_resistencia` fazia `strtod("470.0") * 1e-3`. Como 0,001 não é exato em binário, `"470m"` virava `0.47000000000000003` e `"0.47"` virava `0.46999999999999997`. Só a tolerância de `mesma_resistencia` impedia duplicatas.

### Alterações

[src/lista.h](src/lista.h):
- `typedef struct descritor Lista;` (opaco) substitui `typedef struct elemento *Lista;`. As assinaturas públicas (`Lista *`) e os códigos de retorno não mudaram: `main.c`, `telas.c` e os 18 testes antigos compilam e passam sem alteração.
- Nova função `int lista_passos(Lista *li);`, que devolve os nós examinados pela última busca (0 para NULL).
- O comentário do topo passa a descrever o nó descritor.

[src/lista.c](src/lista.c):
- **Descritor:** `struct descritor { Elem *inicio; int tipos; long long pecas; int passos; }`.
  - `cria_lista` zera todos os campos.
  - `libera_nos` também zera `tipos` e `pecas`, então `reinicializa_lista` deixa os totais corretos.
- **O(1):** `tamanho_lista`, `total_pecas` e `lista_vazia` só leem o descritor (com os mesmos retornos para NULL).
- **`static Elem **localiza(Lista *li, double ohms)`**, com ponteiro para ponteiro: devolve o endereço do ponteiro (`inicio` ou `prox` do anterior) que aponta para o primeiro nó com resistência >= `ohms`, e conta em `passos` os nós examinados. Para cedo, graças à ordenação.
  - Novo helper `e_o_no()`: o nó existe e tem a chave procurada.
  - `busca()` vira um wrapper de duas linhas sobre `localiza`.
- **insere:** `novo->prox = *pp; *pp = novo;` cobre início, meio e fim sem `ant`. Atualiza `tipos++` e `pecas += quantidade`.
- **remove:** `*pp = no->prox;` + `free`. Atualiza `tipos--` e `pecas -= quantidade`.
- **retira / repor:** atualizam `pecas -= qtd` e `pecas += qtd`. A ordem das validações é a mesma de antes.

[src/formatacao.c](src/formatacao.c):
- `multiplicador(char) → double` vira `expoente(char, int *exp)`: m → -3, R/r → 0, k/K → 3, M → 6.
- O número é montado como `"A.Be<exp>"` e convertido por um único `strtod`, que arredonda corretamente. Agora `"470m"`, `"0.47"` e `"0,47"` geram o mesmo `double`, sem perder precisão (`"470.5m"` continua sendo 470,5 mΩ).
- O buffer `numero` cresceu 8 bytes para caber o expoente.
- A gramática aceita e a checagem de faixa não mudaram (`"1e3"` continua sendo rejeitado).

[src/ui/telas.c](src/ui/telas.c):
- `tela_consultar` mostra `Busca: N nós visitados` (ou `1 nó visitado`) depois do resultado, tanto quando encontra quanto quando não encontra. Reaproveita `plural()` e `ui_dica()`.

Testes:
- [tests/test_lista.c](tests/test_lista.c):
  - `teste_contadores_invariantes`: depois de cada operação (insere no início, no meio, no fim, duplicado e inválido; retira normal e sem estoque; repor; remove do primeiro, do meio, do último e de um inexistente; reinicializa), confere `tamanho_lista` e `total_pecas` contra uma recontagem independente via `percorre_lista`, além de `lista_vazia` e da ordenação.
  - `teste_passos_busca`: lista vazia → 0; com 220 → 4,7k → 1M, buscar 100 → 1, 220 → 1, 330 → 2, 4700 → 2, 2M → 3; NULL → 0.
- [tests/test_formatacao.c](tests/test_formatacao.c), `teste_parse_chave_canonica`: igualdade **exata** (`==`) entre `470m`/`0.47`/`0,47`, `4k7`/`4.7k`/`4700`, `1M5`/`1500000`, `2m2`/`0.0022`, `4R7`/`4.7` e `1m`/`0.001`. Também confere que `470.5m` é formatado como `470,5 mΩ`.
- [tests/e2e/esperado.txt](tests/e2e/esperado.txt): espera `Busca: 3 nós visitados` ao consultar `4k7` (lista `470m, 220, 4k7, 1M`). O e2e passou de 35 para 36 verificações.

Documentação:
- [README.md](README.md), seção "Como os dados são guardados":
  - diagrama e struct do descritor;
  - nova subseção sobre `localiza`, com a tabela de nós visitados e o exemplo da tela Consultar;
  - fluxogramas de Inserir e Remover redesenhados com `pp`/`*pp`, sem `ant`;
  - casos de borda reescritos;
  - Retirar/Repor citam `localiza` e o total de peças;
  - tabela "Outras operações" com as contagens em O(1);
  - nova subseção "Chave de ordenação exata".
- `AGENTS.md` (ignorado pelo git, só cópia local): seções Lista e Formatação atualizadas.

### Validação

- **Testes primeiro:** antes da implementação, os testes novos falharam como previsto.
  - `test_formatacao` falhou em `"470m" (0.47000000000000003) != "0.47" (0.46999999999999997)`.
  - `test_lista` não compilou (`implicit declaration of function 'lista_passos'`).
  - O e2e falhou em `nao encontrado (em ordem): Busca: 3 nós visitados`.
- **Windows 11 (MSYS2 UCRT64):** `make clean; make EXTRA_CFLAGS=-Werror all test e2e` passou sem warnings.
  - `test_lista`: 20 testes (170 asserts); `test_formatacao`: 12 (88); `test_ui`: 14 (48); `test_telas`: 19 (56). Nenhum falhou.
  - `E2E OK (36 verificacoes)`.
- **Linux (Docker `gcc:14`):** `make EXTRA_CFLAGS=-Werror all test e2e` e `make asan` (AddressSanitizer/UBSan) passaram, sem vazamento nem acesso inválido.
- **Manual (Linux, `NO_COLOR=1`):** com 220, 4k7 e 1M cadastrados, Consultar `4k7` mostrou `Busca: 2 nós visitados`, e Consultar `100` mostrou `100 Ω não está no estoque` + `Busca: 1 nó visitado`.
- **Pendente:** `make cppcheck` fica a cargo do job Ubuntu do CI (o cppcheck 2.17 local falha também no código antigo, por diferença de versão).
