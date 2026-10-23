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

## Instalação dos pré-requisitos

Este guia parte de um computador sem nenhuma ferramenta de programação instalada. Você vai precisar de:

| Ferramenta | Para que serve | Obrigatória? |
| --- | --- | --- |
| **Git** | baixar (clonar) o código do GitHub | sim (ou baixe o ZIP pelo site) |
| **gcc** | compilador C que gera o executável | sim |
| **make** | roda os comandos de build e de teste do `Makefile` | sim |
| **cppcheck** | análise estática do código (`make cppcheck`) | não |

### Windows 10/11 — MSYS2

O [MSYS2](https://www.msys2.org/) instala no Windows um terminal estilo Linux com o gerenciador de pacotes `pacman`, de onde vêm o gcc, o make e o git.

**1. Instale o MSYS2.** Escolha uma das opções:

- Pelo **PowerShell** (já vem com o Windows):

  ```powershell
  winget install --id MSYS2.MSYS2 -e
  ```

- Ou baixe o instalador em [msys2.org](https://www.msys2.org/), execute e aceite a pasta padrão `C:\msys64`.

**2. Abra o terminal certo.** No menu Iniciar, procure e abra **MSYS2 UCRT64**. Há vários atalhos do MSYS2 (MSYS, MINGW64, CLANG64...), e os comandos abaixo só funcionam no **UCRT64**. Para conferir, rode:

```sh
echo $MSYSTEM     # deve mostrar: UCRT64
```

**3. Atualize o MSYS2.** No terminal UCRT64:

```sh
pacman -Syu
```

Responda `Y` quando perguntar. Se o terminal pedir para fechar, feche, abra o **MSYS2 UCRT64** de novo e termine a atualização com:

```sh
pacman -Su
```

**4. Instale as ferramentas:**

```sh
pacman -S --needed git make mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cppcheck
```

**5. Confira a instalação.** Cada comando deve imprimir uma versão:

```sh
gcc --version
make --version
git --version
cppcheck --version
```

> **Caminhos no MSYS2:** os discos do Windows aparecem como `/c/`, `/d/`... Por exemplo, `D:\GitHub` vira `/d/GitHub`. O terminal começa na sua pasta pessoal do MSYS2 (`~`).

#### Usar pelo PowerShell, sem abrir o MSYS2

Depois da instalação acima, o `gcc`, o `make` e o `git` também rodam direto no PowerShell. Basta o PowerShell enxergar duas pastas do MSYS2: `C:\msys64\ucrt64\bin` (gcc) e `C:\msys64\usr\bin` (make, git e utilitários que o `Makefile` usa).

Só na janela atual (vale até fechar o PowerShell):

```powershell
$env:Path += ';C:\msys64\ucrt64\bin;C:\msys64\usr\bin'
```

Ou de forma permanente, para todo PowerShell novo (rode uma vez e abra outro PowerShell):

```powershell
$atual = [Environment]::GetEnvironmentVariable('Path', 'User')
[Environment]::SetEnvironmentVariable('Path', "$atual;C:\msys64\ucrt64\bin;C:\msys64\usr\bin", 'User')
```

As pastas entram no **fim** do `Path`, assim os comandos do Windows com o mesmo nome (`find`, `sort`...) continuam valendo. Confira:

```powershell
gcc --version
make --version
```

### Linux

Debian/Ubuntu:

```sh
sudo apt update
sudo apt install -y build-essential git cppcheck
```

Fedora:

```sh
sudo dnf install -y gcc make git cppcheck
```

### macOS

```sh
xcode-select --install     # instala clang (atende pelo nome gcc), make e git
brew install cppcheck      # opcional; requer o Homebrew (https://brew.sh)
```

## Baixar o projeto

No **PowerShell** (Windows, após a seção acima) ou em qualquer terminal Linux/macOS, vá até a pasta onde quer guardar o projeto e clone:

```powershell
cd D:\GitHub        # exemplo no Windows; no Linux/macOS use qualquer pasta
git clone https://github.com/Grolla05/OhmFlow.git
cd OhmFlow
```

Sem Git, também dá para baixar o ZIP no GitHub (**Code → Download ZIP**), extrair e abrir a pasta no terminal com `cd`. Quem preferir o terminal **MSYS2 UCRT64** pode usá-lo: lá o caminho fica `/d/GitHub`.

## Compilar e executar

O executável final sai na pasta **`dist/`**. Os arquivos intermediários e os testes ficam em `build/`. As duas pastas são ignoradas pelo Git.

Na pasta do projeto, no **PowerShell** (Windows) ou no terminal (Linux/macOS):

```powershell
make            # compila dist/ohmflow (no Windows: dist\ohmflow.exe)
make clean      # remove build/ e dist/
```

Para executar:

```powershell
.\dist\ohmflow.exe      # Windows (PowerShell, cmd ou Windows Terminal)
./dist/ohmflow          # Linux/macOS
```

### Executando no Windows

Rode o `dist\ohmflow.exe` pelo **Windows Terminal, PowerShell ou cmd**, como acima. O terminal do MSYS2 (mintty) não é reconhecido como console interativo: nele, cores, limpeza de tela e pausas ficam desligadas.

O `ohmflow.exe` depende só de DLLs do próprio Windows. Ele roda fora do MSYS2, e a pasta `dist/` pode ser copiada para outro computador.

### Opções

- `NO_COLOR=1`: desliga as cores.
- `make EXTRA_CFLAGS=-DOHM_ASCII`: troca molduras e símbolos Unicode por ASCII (`+-|`, `ohm`), para terminais sem UTF-8.

### Problemas comuns

| Sintoma | Causa / solução |
| --- | --- |
| `gcc` ou `make` não reconhecido no PowerShell | As pastas do MSYS2 não estão no `Path`. Rode o comando de [Usar pelo PowerShell](#usar-pelo-powershell-sem-abrir-o-msys2) e abra um PowerShell novo se foi a versão permanente. |
| `gcc: command not found` no MSYS2 | O terminal aberto não é o **MSYS2 UCRT64** (`echo $MSYSTEM`), ou o passo 4 não foi feito. |
| `make: command not found` no MSYS2 | Rode `pacman -S make` no UCRT64. |
| `Ω` e bordas aparecem como `Î©` ou `â•”` | O terminal não está em UTF-8. Use o Windows Terminal ou compile com `-DOHM_ASCII`. |
| Sem cores, sem limpar tela, sem pausas | O programa está rodando no mintty ou com entrada/saída redirecionada. Execute no Windows Terminal. |
| `make asan` falha no Windows | É esperado: o gcc do MinGW não tem AddressSanitizer. Esse alvo roda no Linux e no CI. |

## Testes

Os mesmos comandos valem no PowerShell (Windows), no Linux e no macOS:

```powershell
make test       # testes unitários (lista, formatação, ui)
make e2e        # roda o programa com tests/e2e/entrada.txt e confere tests/e2e/esperado.txt
make asan       # testes + e2e com AddressSanitizer/UBSan (Linux; o gcc do MinGW não suporta)
make cppcheck   # análise estática
```

Para validar tudo de uma vez:

```powershell
make clean; make EXTRA_CFLAGS=-Werror all test e2e cppcheck
```

O GitHub Actions ([.github/workflows/ci.yml](.github/workflows/ci.yml)) roda tudo isso, com `-Werror` e também o `make asan`, a cada push ou pull request em `main` e `Grolla`.

## Referência

O enunciado do projeto está em [refs/Aula_07_Projeto01_22_10_2026.pdf](refs/Aula_07_Projeto01_22_10_2026.pdf).
