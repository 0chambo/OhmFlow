# Funcionalidades

← [Índice](../README.md)

Um documento por funcionalidade do OhmFlow. Cada um conta a história **completa** daquela feature, de ponta a ponta:

1. **Para que serve** — em linguagem do dia a dia.
2. **Como usar** — passo a passo, com exemplos de tela.
3. **Regras e casos especiais** — tudo o que pode acontecer.
4. **Por dentro** — o caminho no código, das telas até a estrutura de dados.
5. **Registro no log** — as linhas que a feature grava.
6. **Desfazer** — se e como a ação pode ser revertida.
7. **Por que é assim** — as decisões tomadas.
8. **Testes** — onde a feature é verificada.

Para a explicação de cada **arquivo** de código, veja [Módulos do código-fonte](../modulos/README.md). Os dois conjuntos se completam: as funcionalidades olham "de fora para dentro", os módulos olham "de dentro para fora".

**Tabela 1 — Funcionalidades**

| # | Funcionalidade | Menu | Muda o estoque? | Desfazível? |
| --- | --- | --- | --- | --- |
| 00 | [Boas-vindas e menu](00-boas-vindas-e-menu.md) | — | Não | — |
| 01 | [Novo](01-novo.md) | `[1]` | Sim | Sim (lote = 1 passo) |
| 02 | [Remover](02-remover.md) | `[2]` | Sim | Sim |
| 03 | [Listar](03-listar.md) | `[3]` | Não | — |
| 04 | [Consultar](04-consultar.md) | `[4]` | Não | — |
| 05 | [Retirar](05-retirar.md) | `[5]` | Sim | Sim |
| 06 | [Repor](06-repor.md) | `[6]` | Sim | Sim |
| 07 | [Reinicializar](07-reinicializar.md) | `[7]` | Sim | Sim (foto do estoque) |
| 08 | [Diferencial](08-diferencial.md) | `[8]` | — (a definir) | — |
| 09 | [Histórico](09-historico.md) | `[9]` | Só via checkpoint | — |
| 10 | [Desfazer e refazer (checkpoint)](10-desfazer-refazer.md) | dentro do `[9]` | Sim | É o próprio desfazer |
| 11 | [Registro de ações (log)](11-registro-de-acoes.md) | automático | Não | — |
| 12 | [Entrada de valores](12-entrada-de-valores.md) | em todas as perguntas | Não | — |
| 13 | [Sair](13-sair.md) | `Esc` / `0` | Libera a memória | — |
| 14 | [Resistor colorido (código de cores)](14-resistor-colorido.md) | dentro do `[1]` | Não (só visual) | — |
