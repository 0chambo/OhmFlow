# Módulos do código-fonte

← [Arquitetura](../04-arquitetura.md) · [Índice](../README.md)

Cada arquivo de `src/` tem um documento próprio. Todos seguem a mesma estrutura, para facilitar a leitura:

1. **Resumo** — o que o módulo faz, em uma frase.
2. **Explicação para leigos** — uma analogia do dia a dia.
3. **Estruturas** — os tipos de dados que o módulo define.
4. **Funções** — o que cada função recebe, faz e devolve.
5. **Algoritmos** — os passos das partes mais importantes, com diagramas.
6. **Por que foi feito assim** — as decisões e as alternativas.
7. **Testes** — onde o módulo é testado.

A Tabela 1 sugere a ordem de leitura, das bases para cima.

**Tabela 1 — Módulos, na ordem sugerida**

| Ordem | Módulo | Arquivos | Linhas aprox. | Depende de |
| --- | --- | --- | --- | --- |
| 1 | [Lista](lista.md) | `lista.h`, `lista.c` | 270 | — |
| 2 | [Formatação](formatacao.md) | `formatacao.h`, `formatacao.c` | 220 | lista (faixa) |
| 3 | [Resistor](resistor.md) | `resistor.h`, `resistor.c` | 225 | — |
| 4 | [Pilha](pilha.md) | `pilha.h`, `pilha.c` | 160 | lista (tipo `Resistor`) |
| 5 | [Checkpoint](checkpoint.md) | `checkpoint.h`, `checkpoint.c` | 360 | pilha, lista |
| 6 | [Log](log.md) | `log.h`, `log.c` | 400 | — |
| 7 | [UI](ui.md) | `ui/ui.h`, `ui/ui.c` | 780 | — |
| 8 | [Telas](telas.md) | `ui/telas.h`, `ui/telas.c` | 1440 | todos acima |
| 9 | [Main](main.md) | `main.c`, `assets/ohmflow.rc` | 100 | todos acima |
