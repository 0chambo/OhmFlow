# Documentação do OhmFlow

Bem-vindo à documentação completa do **OhmFlow**, o programa que controla o estoque de resistores do almoxarifado de um laboratório. Ela foi escrita em **módulos**: cada arquivo trata de um assunto e pode ser lido sozinho, mas a ordem numérica conta a história do projeto do começo ao fim.

A documentação foi pensada para **qualquer leitor**, inclusive quem nunca programou. Sempre que um termo técnico aparece pela primeira vez, ele é explicado em linguagem do dia a dia; se esquecer algum, consulte o [Glossário e FAQ](10-glossario-e-faq.md).

> **Fonte da verdade:** o código em `src/` e os testes em `tests/`. Se algo aqui divergir do código, o código vale, e esta documentação deve ser corrigida.

---

## Por onde começar

A Tabela 1 sugere trilhas de leitura conforme o seu objetivo.

**Tabela 1 — Trilhas de leitura**

| Quem é você | O que quer | Leia, nesta ordem |
| --- | --- | --- |
| Usuário do laboratório | Instalar e usar o programa | [01](01-visao-geral.md) → [02](02-guia-do-usuario.md) → [10](10-glossario-e-faq.md) |
| Quem quer entender uma feature específica | Tudo sobre Novo, Retirar, Histórico... | [Funcionalidades](funcionalidades/README.md) → o arquivo da feature |
| Leigo curioso | Entender como um programa assim funciona por dentro | [01](01-visao-geral.md) → [03](03-conceitos-fundamentais.md) → [04](04-arquitetura.md) → [módulo Lista](modulos/lista.md) |
| Estudante da disciplina | Estudar as estruturas de dados (lista e pilha) | [03](03-conceitos-fundamentais.md) → [Lista](modulos/lista.md) → [Pilha](modulos/pilha.md) → [Checkpoint](modulos/checkpoint.md) → [08](08-decisoes-de-projeto.md) |
| Desenvolvedor novo no time | Compilar, testar e alterar o código | [04](04-arquitetura.md) → [05](05-build-e-ambiente.md) → [06](06-testes.md) → [09](09-contribuindo.md) |
| Quem cuida de entregas | Gerar o instalador e publicar versões | [05](05-build-e-ambiente.md) → [07](07-ci-e-instalador.md) |

---

## Mapa dos módulos

### Parte I — Entender o produto

| # | Módulo | Do que trata |
| --- | --- | --- |
| 01 | [Visão geral](01-visao-geral.md) | O problema, o objetivo, os requisitos do enunciado e o que o programa faz |
| 02 | [Guia do usuário](02-guia-do-usuario.md) | Instalar, abrir e usar cada opção do menu, passo a passo |
| — | [Funcionalidades](funcionalidades/README.md) | **Um documento por feature**, de ponta a ponta: uso, regras, código, log, desfazer, porquês e testes |

As funcionalidades documentadas são:

| # | Funcionalidade | # | Funcionalidade |
| --- | --- | --- | --- |
| 00 | [Boas-vindas e menu](funcionalidades/00-boas-vindas-e-menu.md) | 07 | [Reinicializar](funcionalidades/07-reinicializar.md) |
| 01 | [Novo](funcionalidades/01-novo.md) | 08 | [Diferencial](funcionalidades/08-diferencial.md) |
| 02 | [Remover](funcionalidades/02-remover.md) | 09 | [Histórico](funcionalidades/09-historico.md) |
| 03 | [Listar](funcionalidades/03-listar.md) | 10 | [Desfazer e refazer](funcionalidades/10-desfazer-refazer.md) |
| 04 | [Consultar](funcionalidades/04-consultar.md) | 11 | [Registro de ações (log)](funcionalidades/11-registro-de-acoes.md) |
| 05 | [Retirar](funcionalidades/05-retirar.md) | 12 | [Entrada de valores](funcionalidades/12-entrada-de-valores.md) |
| 06 | [Repor](funcionalidades/06-repor.md) | 13 | [Sair](funcionalidades/13-sair.md) |
| | | 14 | [Resistor colorido (código de cores)](funcionalidades/14-resistor-colorido.md) |

### Parte II — Entender o código

| # | Módulo | Do que trata |
| --- | --- | --- |
| 03 | [Conceitos fundamentais](03-conceitos-fundamentais.md) | Memória, ponteiros, structs, lista encadeada, pilha, TAD, complexidade, explicados do zero |
| 04 | [Arquitetura](04-arquitetura.md) | Como o código está dividido, quem depende de quem e o caminho de uma ação do teclado até a memória |
| — | [Módulos do código-fonte](modulos/README.md) | Um documento por arquivo de `src/`: o que faz, como faz e por quê |

Os módulos do código-fonte são:

| Módulo | Arquivos | Papel |
| --- | --- | --- |
| [Lista](modulos/lista.md) | `src/lista.h`, `src/lista.c` | Estrutura central: lista dinâmica ordenada de resistores |
| [Resistor](modulos/resistor.md) | `src/resistor.h`, `src/resistor.c` | Código de cores de 4 faixas e desenho do resistor em pixel-art |
| [Pilha](modulos/pilha.md) | `src/pilha.h`, `src/pilha.c` | Pilha dinâmica de operações (base do desfazer/refazer) |
| [Checkpoint](modulos/checkpoint.md) | `src/checkpoint.h`, `src/checkpoint.c` | Desfazer e refazer com duas pilhas |
| [Formatação](modulos/formatacao.md) | `src/formatacao.h`, `src/formatacao.c` | Converte `4k7` em 4700 Ω e 4700 Ω em `4,7 kΩ` |
| [Log](modulos/log.md) | `src/log.h`, `src/log.c` | Registro de todas as ações em arquivo de texto |
| [UI](modulos/ui.md) | `src/ui/ui.h`, `src/ui/ui.c` | Peças de interface: cores, caixas, tabelas, leitura do teclado |
| [Telas](modulos/telas.md) | `src/ui/telas.h`, `src/ui/telas.c` | Cada tela do programa e o menu navegável |
| [Main](modulos/main.md) | `src/main.c`, `src/assets/ohmflow.rc` | Ponto de partida: monta tudo, roda o menu e libera a memória |

### Parte III — Construir, testar e entregar

| # | Módulo | Do que trata |
| --- | --- | --- |
| 05 | [Build e ambiente](05-build-e-ambiente.md) | Instalar as ferramentas, compilar e entender o `Makefile` alvo por alvo |
| 06 | [Testes](06-testes.md) | Mini framework próprio, testes unitários, ponta a ponta e do instalador |
| 07 | [CI e instalador](07-ci-e-instalador.md) | GitHub Actions, Inno Setup, versões e Releases |

### Parte IV — Contexto e manutenção

| # | Módulo | Do que trata |
| --- | --- | --- |
| 08 | [Decisões de projeto](08-decisoes-de-projeto.md) | O porquê de cada escolha importante e as alternativas descartadas |
| 09 | [Contribuindo](09-contribuindo.md) | Fluxo de trabalho, convenções, como adicionar uma funcionalidade nova |
| 10 | [Glossário e FAQ](10-glossario-e-faq.md) | Termos técnicos e de eletrônica, perguntas frequentes |

---

## Convenções desta documentação

- **Código e nomes de arquivo** aparecem em `fonte monoespaçada`, por exemplo `lista.c`.
- **Links para o código** apontam para o arquivo no repositório, por exemplo [src/lista.c](../src/lista.c).
- **Diagramas** usam [Mermaid](https://mermaid.js.org/), que o GitHub desenha automaticamente. Em outro visualizador, o texto do diagrama continua legível.
- **Tabelas e figuras** são numeradas dentro de cada módulo e sempre citadas no texto antes de aparecerem.
- Tudo em **português (pt-BR)**, como o código, os comentários e as mensagens do programa.

## Outros documentos do repositório

| Documento | Para quê |
| --- | --- |
| [README.md](../README.md) | Cartão de boas-vindas do projeto (prints, links e equipe) |
| [refs/Aula_07_Projeto01_22_10_2026.pdf](../refs/Aula_07_Projeto01_22_10_2026.pdf) | Enunciado oficial do trabalho |
| [refs/BRAINSTORM.md](../refs/BRAINSTORM.md) | Ideias para o item diferencial (opção 8 do menu) |
| [.github/CONTRIBUTING.md](../.github/CONTRIBUTING.md) | Guia de contribuição |
| [.github/COMMIT_CONVENTION.md](../.github/COMMIT_CONVENTION.md) | Padrão das mensagens de commit |
| [.github/SECURITY.md](../.github/SECURITY.md) | Como reportar problemas de segurança |
