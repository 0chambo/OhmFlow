# Brainstorm — item diferencial do menu (opção 8)

Ideias para o **item diferencial** exigido pelo enunciado ([Aula_07_Projeto01_22_10_2026.pdf](Aula_07_Projeto01_22_10_2026.pdf)):

> "Inserir um item diferencial em seu Menu, cada grupo deverá ter um diferencial de especificidade dentro deste Menu em seu algoritmo."

Hoje a opção 8 existe no menu com o rótulo `(diferencial)` e [`tela_diferencial`](../src/ui/telas.c) só mostra "Em breve". Este documento lista candidatos, compara e recomenda. **Nada aqui está decidido.**

---

## Resumo e recomendação

A Tabela 1 compara as ideias por esforço, quanto mostram de estrutura de dados (o foco da disciplina) e impacto na apresentação.

**Tabela 1 — Comparativo das ideias de diferencial**

| # | Ideia | Esforço | Estrutura de dados | Impacto na demo | Arquivos novos/alterados |
| --- | --- | :---: | :---: | :---: | --- |
| 1 | Resistor mais próximo / substituto | Baixo | Alto | Alto | `lista.c`, `telas.c` |
| 4 | Desfazer última operação (pilha) | Médio | **Muito alto** | Alto | `historico.c` (novo), `main.c`, `telas.c` |
| 8 | Alerta de estoque baixo | Baixo | Baixo | Médio | `lista.c`, `telas.c` |
| 9 | Salvar/carregar estoque em arquivo | Médio | Baixo | Alto | `arquivo.c` (novo), `main.c`, `telas.c` |

**Recomendação:** a opção 8 vira um **submenu "Ferramentas do laboratório"** com 2 ou 3 itens desta lista. Isso dá mais especificidade do que uma função solta. Combinações que funcionam bem juntas:

- **A — foco em estrutura de dados:** #1 Mais próximo + #4 Desfazer (pilha) + #5 Faixa. Acrescenta uma segunda estrutura (pilha) ao relatório e explora a ordenação da lista.
- **B — foco em eletrônica:** #1 Mais próximo + #2 Série/paralelo + #6 Código de cores. É o mais "específico de laboratório" e o mais visual na demo.

Se for uma função só: **#2 Série/paralelo**. Usa a lista ordenada de verdade (busca de pares), resolve um problema real do almoxarifado ("não tenho 5 kΩ, que dois resistores do estoque dão isso?") e rende uma boa explicação no relatório.

> [!NOTE]
> **Observação — ideias favoritas (Grolla):** as preferidas são a **#4 Desfazer última operação (pilha)**, a **#8 Alerta de estoque baixo**, a **#9 Salvar/carregar estoque em arquivo**. Para a #9, ver também a [evolução com SQLite](#evolução-futura-banco-local-com-sqlite) prevista para uma versão mais robusta.

---

## Ideias em detalhe

### 1. Resistor mais próximo / substituto

**O que faz:** o usuário digita um valor alvo (ex.: `5k`). Se ele existe no estoque, mostra a quantidade. Se não existe ou está zerado, mostra o **vizinho menor e o vizinho maior** com estoque e o erro percentual de cada um.

**Por que encaixa:** a lista é ordenada, então os vizinhos ficam lado a lado. `localiza` já para no primeiro nó ≥ alvo; o anterior é o vizinho menor. A busca para cedo, como em Consultar (`lista_passos` pode ser mostrado de novo).

**Exemplo de saída:**

```text
Alvo: 5 kΩ (não cadastrado)
  Menor: 4,7 kΩ  (12 un.)   erro -6,0 %
  Maior: 5,6 kΩ  ( 3 un.)   erro +12,0 %
Sugestão: 4,7 kΩ
```

**Implementação:** nova função `vizinhos_lista(const Lista*, double alvo, Resistor *menor, Resistor *maior)` em `lista.c`. Os testes cobrem: lista vazia, alvo menor que todos, maior que todos, exato e vizinho com quantidade 0 (pulado).

---

### 4. Desfazer última operação (pilha)

**O que faz:** cada Inserir, Remover, Retirar, Repor e Reinicializar empilha o que foi feito. A opção "Desfazer" desempilha e reverte a última operação, mostrando o que foi revertido. Pode haver também "Ver histórico", que lista a pilha do topo para a base.

**Por que encaixa:** traz uma **segunda estrutura de dados dinâmica** (pilha encadeada) para o projeto. O relatório tem que "listar todas as estruturas de dados", e esta é a ideia que mais acrescenta ali. Também mostra a diferença LIFO (pilha) × lista ordenada.

**Detalhes de projeto:**

- Nó da pilha: `{ tipo_operacao, Resistor antes, Resistor depois }`.
- Reinicializar precisa guardar a lista inteira (cópia) ou ficar fora do desfazer. Mais simples: limpar a pilha ao reinicializar, avisando o usuário.
- A pilha é liberada ao sair (cada `malloc` com o seu `free`, regra do AGENTS.md).
- Para manter lógica e interface separadas, as telas registram a operação **depois** de a lista confirmar `LISTA_OK`.

**Esforço:** médio. Mexe em todas as telas de escrita, mas cada mudança é pequena.

---

### 8. Alerta de estoque baixo

**O que faz:** o usuário define um limite (ex.: 10 unidades) e o programa lista os tipos com quantidade ≤ limite, destacando os zerados em vermelho.

**Por que encaixa:** simples e útil, mas é só um filtro na listagem e pouco "específico". Melhor como item de um submenu do que como diferencial sozinho.

---

### 9. Salvar/carregar estoque em arquivo

**O que faz:** grava o estoque em `estoque.csv` (`resistencia_ohms;quantidade`) e carrega na abertura ou sob demanda.

**Por que encaixa:** resolve a maior limitação atual (fechar o programa apaga tudo) e facilita a demo, porque dá para abrir com um estoque pronto. Usa `insere_lista_ordenada` ao carregar, então a ordenação vem de graça.

**Cuidados:** validar cada linha (reaproveitar a faixa `RESISTENCIA_MIN`–`RESISTENCIA_MAX`), tratar duplicados, `fopen`/`fclose` sempre pareados. Os testes usam arquivo temporário, como o `test_telas.c` já faz.

**Escopo:** o arquivo CSV é a solução para a **versão atual** do projeto.

#### Evolução futura: banco local com SQLite

Numa versão mais robusta e concisa, a persistência passa a usar um **banco de dados local SQLite**, sem servidor, sem ferramenta de migration, tudo dentro do repositório:

| Arquivo | Papel |
| --- | --- |
| `db/schema.sql` | script `.sql` versionado que cria e junta os dados: `CREATE TABLE IF NOT EXISTS`, carga inicial opcional (`INSERT OR IGNORE`) |
| `db/ohmflow.db` | arquivo do banco gerado localmente a partir do `schema.sql` (fica no `.gitignore`) |
| `src/vendor/sqlite3.c` / `sqlite3.h` | amálgama oficial do SQLite (dois arquivos C, domínio público), compilada junto com o projeto, sem DLL nem instalação extra |

Esboço do `schema.sql`:

```sql
CREATE TABLE IF NOT EXISTS resistor (
    resistencia REAL    PRIMARY KEY CHECK (resistencia BETWEEN 1e-3 AND 1e9),
    quantidade  INTEGER NOT NULL    CHECK (quantidade >= 0)
);

-- histórico de movimentações (combina com a ideia #4, desfazer)
CREATE TABLE IF NOT EXISTS movimentacao (
    id          INTEGER PRIMARY KEY AUTOINCREMENT,
    operacao    TEXT    NOT NULL, -- inserir, remover, retirar, repor, reinicializar
    resistencia REAL,
    quantidade  INTEGER,
    momento     TEXT    NOT NULL DEFAULT (datetime('now', 'localtime'))
);
```

Funcionamento:

1. Ao abrir, o programa executa o `schema.sql` (`sqlite3_exec`). Como tudo é `IF NOT EXISTS`, rodar de novo não faz mal, e por isso não precisa de migration.
2. Carrega `SELECT resistencia, quantidade FROM resistor ORDER BY resistencia` para a lista com `insere_lista_ordenada`. A **lista dinâmica ordenada continua sendo a estrutura do programa**; o banco é só o armazenamento.
3. Cada operação que der `LISTA_OK` grava no banco (`INSERT`/`UPDATE`/`DELETE` com `sqlite3_prepare_v2` e parâmetros vinculados, nunca SQL montado com `sprintf`).
4. A tabela `movimentacao` alimenta o histórico (#4) e as estatísticas (#10) sem mudar a lista.

Pontos de atenção: a amálgama é grande (~9 MB de código-fonte) e o cppcheck deve ignorar `src/vendor/`. Os testes usam banco em memória (`:memory:`) para não deixar arquivo para trás. O `Makefile` precisa de `-DSQLITE_OMIT_LOAD_EXTENSION` e de compilar `sqlite3.c` sem `-pedantic -Werror`, porque o código de terceiros tem warnings.

---

## Regras para quem implementar

Seguir o workflow do [AGENTS.md](../AGENTS.md):

1. **Testes primeiro** em `tests/` (novo `test_<modulo>.c` registrado no `Makefile`).
2. Regra de negócio em `lista`/módulo novo; `telas.c` só conversa com o usuário.
3. C99 com `-Wall -Wextra -pedantic -Werror`, sem vazamento de memória.
4. Textos e nomes em pt-BR.
5. Trocar o rótulo `(diferencial)` em `desenha_menu` pelo nome real, atualizar a linha 8 da tabela do [README.md](../README.md) e acrescentar o fluxo da opção 8 ao e2e ([tests/e2e/](../tests/e2e/)).
6. Documentar no `CHANGES.md` e pedir validação antes do commit.
