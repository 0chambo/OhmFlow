# OhmFlow

[![CI](https://github.com/Grolla05/OhmFlow/actions/workflows/ci.yml/badge.svg)](https://github.com/Grolla05/OhmFlow/actions/workflows/ci.yml)

Programa em C que controla o estoque de resistores do almoxarifado de um laboratório usando uma **lista dinâmica ordenada**. Desenvolvido para a disciplina de Estrutura de Dados e Recuperação da Informação (Projeto 01).

## Funcionalidades

| Opção | Função | Descrição |
| --- | --- | --- |
| 1 | Inserir | cadastra um novo tipo de resistor (resistência + quantidade) |
| 2 | Remover | apaga um tipo de resistor da lista, com confirmação |
| 3 | Listar | tabela de todos os tipos em ordem crescente, impressos em mΩ / Ω / kΩ / MΩ |
| 4 | Consultar | mostra a quantidade disponível de uma resistência |
| 5 | Retirar | dá baixa de uma quantidade no estoque |
| 6 | Repor | soma uma quantidade ao estoque |
| 7 | Inicializar/Reinicializar | esvazia a lista, com confirmação |
| 8 | Diferencial | a definir |
| 0 | Sair | encerra o programa e libera a memória |

### Como digitar resistências

A lista guarda os valores em Ω, mas a entrada aceita os formatos usados no laboratório:

| Digitado | Valor |
| --- | --- |
| `220`, `220R`, `220 ohm` | 220 Ω |
| `4700`, `4.7k`, `4,7k`, `4k7`, `4K7` | 4,7 kΩ |
| `1M`, `1M5` | 1 MΩ, 1,5 MΩ |
| `470m`, `0.47`, `0,47` | 470 mΩ |
| `4R7` | 4,7 Ω |

Atenção: `m` minúsculo é mili e `M` maiúsculo é mega. A faixa aceita vai de 1 mΩ a 1000 MΩ. Deixar a resposta em branco (só Enter) cancela a operação.

## Estrutura

```text
src/
├── lista.h / lista.c             TAD lista dinâmica ordenada (chave: resistência)
├── formatacao.h / formatacao.c   leitura "4k7" -> Ω e impressão por grandeza
├── main.c                        cria a lista e conduz as telas
└── ui/
    ├── ui.h / ui.c               cores ANSI, molduras, tabelas, leitura de entrada
    └── telas.h / telas.c         boas-vindas, menu, telas das funções, feedback, encerramento
tests/
├── test_framework.h              mini framework de testes (sem dependências)
├── test_lista.c                  testes da lista
├── test_formatacao.c             testes de parse e formatação
├── test_ui.c                     testes de alinhamento e cores
└── e2e/                          teste ponta a ponta: entrada simulada e saída esperada
```

## Compilar e executar

Requisitos: `gcc` (C99) e `make`. No Linux use os pacotes do sistema. No Windows use o [MSYS2](https://www.msys2.org/) no ambiente **UCRT64**:

```sh
pacman -S --needed mingw-w64-ucrt-x86_64-gcc make mingw-w64-ucrt-x86_64-cppcheck
```

```sh
make            # compila build/ohmflow
make run        # executa
make clean      # remove build/
```

> No Windows, rode o `build/ohmflow.exe` no **Windows Terminal**, PowerShell ou cmd. O terminal do MSYS2 (mintty) não é reconhecido como console interativo, então cores, limpeza de tela e pausas ficam desligadas.

Variáveis e opções:

- `NO_COLOR=1`: desliga as cores.
- `make EXTRA_CFLAGS=-DOHM_ASCII`: troca molduras e símbolos Unicode por ASCII (`+-|`, `ohm`), para terminais sem UTF-8.

## Testes

```sh
make test       # testes unitários (lista, formatação, ui)
make e2e        # roda o programa com tests/e2e/entrada.txt e confere tests/e2e/esperado.txt
make asan       # testes + e2e com AddressSanitizer/UBSan (Linux; o gcc do MinGW não suporta)
make cppcheck   # análise estática
```

O GitHub Actions ([.github/workflows/ci.yml](.github/workflows/ci.yml)) roda tudo isso, com `-Werror`, a cada push ou pull request em `main` e `Grolla`.
