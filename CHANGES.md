# CHANGES

## feat(menu): tecla Esc aciona Sair na hora

### Motivação

No menu principal, apertar `Esc` não fazia nada: o byte 27 sozinho era tratado como `KEY_IGNORADA`. O usuário pediu que `Esc` no menu encerre o programa imediatamente, como a opção `0 - Sair`.

### O que mudou

#### `src/ui/telas.h`

- Nova constante `KEY_ESC` (262): tecla Esc pressionada sozinha.
- `KEY_IGNORADA` agora cobre só Alt+tecla e sequências desconhecidas (o comentário foi atualizado).
- Nova constante `MENU_CONTINUA` (-1) e nova função pública `menu_tecla_opcao(int tecla, int selecao)`.

#### `src/ui/telas.c`

- `decodifica_tecla` (caminho POSIX): quando chega ESC e nenhum outro byte dentro do prazo curto (`TECLA_TIMEOUT`), devolve `KEY_ESC`. As setas (`ESC [ A`, `ESC O A`, ...) e Alt+tecla (`ESC x` → `KEY_IGNORADA`) continuam iguais, e ESC sozinho segue sem travar à espera de outra tecla.
- `ler_tecla` (caminho Windows, `_getch`): byte 27 vira `KEY_ESC`.
- Nova função `menu_tecla_opcao`: concentra a decisão "esta tecla encerra o menu com qual opção?":
  - `KEY_ESC` ou `KEY_FIM` → `OPCAO_SAIR`;
  - dígito `'0'`–`'8'` → a opção correspondente (limite agora derivado de `OPCAO_TOTAL`);
  - `Enter` (`'\r'` ou `'\n'`) → a seleção atual;
  - qualquer outra tecla → `MENU_CONTINUA` (o menu segue navegando).
- `tela_menu` (modo interativo) usa `menu_tecla_opcao` no lugar dos três `if` antigos (EOF, dígito, Enter). Comportamento dessas teclas não mudou; só o Esc passou a sair.
- Linha de dica do menu ganhou `[Esc] Sair`. Os separadores passaram de dois espaços para um, para a dica caber em 80 colunas:
  `↑↓←→/WASD Navegar · [0-8] Acesso rápido · [Enter] Confirmar · [Esc] Sair`

#### `tests/test_telas.c`

- `teste_tecla_esc_isolado_nao_trava`: agora espera `KEY_ESC` (antes `KEY_IGNORADA`) e continua garantindo que nenhuma leitura bloqueante extra foi feita.
- Novo `teste_menu_esc_sai`: `menu_tecla_opcao` devolve `OPCAO_SAIR` para `KEY_ESC` (com qualquer seleção) e `KEY_FIM`; dígito, `\r` e `\n` mantêm o comportamento; `KEY_UP`, `KEY_IGNORADA` e `'9'` devolvem `MENU_CONTINUA`.
- `teste_navegar_ignora_teclas_especiais`: `navegar_menu` não move a seleção com `KEY_ESC`.

#### Documentação

- `README.md`: nova seção "Teclas do menu" (setas/WASD, dígitos, Enter, Esc).
- `AGENTS.md`: descrição da UI menciona que `Esc` no menu aciona Sair.

### Fora do escopo

- O modo não interativo (e2e/CI, leitura por linha) não muda: lá o menu continua lendo o número da opção.
- `Esc` dentro das outras telas (Inserir, Remover, ...) não foi alterado.

### Validação

`make clean; make EXTRA_CFLAGS=-Werror all test e2e cppcheck`, tudo passando:

- test_lista: 20 testes, 170 asserts
- test_formatacao: 12 testes, 88 asserts
- test_ui: 14 testes, 48 asserts
- test_telas: 20 testes, 66 asserts (antes 19 testes / 56 asserts)
- E2E OK (36 verificações)
- cppcheck sem alertas

---

## docs(refs): brainstorm do item diferencial

### Motivação

O checkup do projeto contra o enunciado mostrou que a única lacuna da Parte 01 é o **item diferencial** do menu (opção 8, hoje "Em breve"). Antes de decidir, o grupo precisa de um documento com as opções comparadas.

### O que mudou

- Novo arquivo `refs/BRAINSTORM.md` (só documentação; nenhum código alterado), com:
  - citação do requisito do enunciado e o estado atual da opção 8;
  - **Tabela 1**, que compara 10 ideias por esforço, uso de estrutura de dados, impacto na demo e arquivos afetados;
  - recomendação: transformar a opção 8 num submenu "Ferramentas do laboratório" com dois pacotes sugeridos (A, foco em estrutura de dados: mais próximo + desfazer com pilha + faixa; B, foco em eletrônica: mais próximo + série/paralelo + código de cores). Para uma função só, a escolha é série/paralelo;
  - detalhe de cada ideia: o que faz, por que encaixa na lista ordenada, exemplo de saída, funções/módulos sugeridos e casos de teste:
    1. Resistor mais próximo / substituto (vizinhos via `localiza`)
    2. Associação série/paralelo (busca de pares: dois ponteiros sobre um vetor auxiliar ou laço duplo com parada antecipada)
    3. Divisor de tensão
    4. Desfazer última operação (pilha encadeada, segunda estrutura de dados)
    5. Busca por faixa de valores (começa no mínimo e para no máximo)
    6. Código de cores das faixas (ANSI, com fallback para `NO_COLOR`/`OHM_ASCII`)
    7. Cobertura da série E12/E24 (percurso conjunto de duas sequências ordenadas)
    8. Alerta de estoque baixo
    9. Salvar/carregar estoque em arquivo CSV
    10. Estatísticas do estoque (mín/máx, mediana, histograma por década)
  - checklist "Regras para quem implementar", alinhado ao workflow do `AGENTS.md` (testes primeiro, separação lógica/UI, `-Werror`, pt-BR, trocar o rótulo `(diferencial)`, README e e2e).
- Formatação seguindo a regra do enunciado: o nome da tabela fica acima dela e ela é chamada pelo número no texto antes de aparecer.
- **Observação de ideias favoritas** (bloco `[!NOTE]` logo após a recomendação): as ideias preferidas do autor são #4 (desfazer com pilha), #6 (código de cores), #8 (alerta de estoque baixo), #9 (salvar/carregar) e #10 (estatísticas), com link para a evolução da #9.
- **Ideia #9 ganhou escopo e evolução:**
  - CSV marcado como a solução para a **versão atual**;
  - nova subseção "Evolução futura: banco local com SQLite", para uma versão mais robusta: banco local, sem servidor nem migration, com `db/schema.sql` versionado (`CREATE TABLE IF NOT EXISTS` + `INSERT OR IGNORE`), `db/ohmflow.db` gerado e ignorado pelo Git e a amálgama `sqlite3.c/.h` em `src/vendor/`;
  - esboço do `schema.sql` com as tabelas `resistor` (PK na resistência, `CHECK` da faixa 1 mΩ–1000 MΩ e quantidade ≥ 0) e `movimentacao` (histórico que alimenta as ideias #4 e #10);
  - fluxo em 4 passos: rodar o schema ao abrir, carregar com `ORDER BY` para a lista ordenada (que continua sendo a estrutura do programa), gravar só após `LISTA_OK` com `sqlite3_prepare_v2` e parâmetros vinculados, e usar o histórico;
  - pontos de atenção: tamanho da amálgama, cppcheck ignorando `src/vendor/`, testes com `:memory:`, `sqlite3.c` compilado sem `-pedantic -Werror`.

### Mensagem de commit proposta

```text
feat(menu): tecla Esc aciona Sair na hora

Esc isolado vira KEY_ESC (POSIX: ESC sem byte seguinte; Windows: _getch 27).
Nova menu_tecla_opcao concentra Esc/EOF -> Sair, digito e Enter; tela_menu
passa a usa-la. Dica do menu mostra [Esc] Sair. Testes em test_telas.c;
README e AGENTS.md atualizados.

Adiciona refs/BRAINSTORM.md com 10 ideias comparadas para o item
diferencial do menu (opcao 8), recomendacao de submenu, ideias favoritas
(4, 6, 8, 9, 10) e evolucao da persistencia para SQLite local com
db/schema.sql.
```
