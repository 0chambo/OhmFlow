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
| 8 | Alerta de estoque baixo | Baixo | Baixo | Médio | `lista.c`, `telas.c` |
| 9 | Salvar/carregar estoque em arquivo | Médio | Baixo | Alto | `arquivo.c` (novo), `main.c`, `telas.c` |

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
