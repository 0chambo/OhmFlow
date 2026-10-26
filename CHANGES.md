# CHANGES

## ci/docs: suporte somente ao Windows (CI num job Windows, instalação só pelo PowerShell)

### Motivação

O projeto passa a ser suportado **somente no Windows**, por decisão do grupo. O código C continua portável (os ramos `#ifndef _WIN32` ficam), mas Linux e macOS deixam de ser testados e documentados. A instalação passa a ser descrita **só pelo PowerShell**, sem exigir o terminal do MSYS2.

### Alterações

[.github/workflows/ci.yml](.github/workflows/ci.yml):
- Removido o job `build-test` (Ubuntu), que rodava build, testes, e2e, `make asan` e `make cppcheck`.
- O job `build-test-windows` passa a se chamar **"Build, testes e análise (Windows, MinGW UCRT64)"** e assume a análise estática:
  - `pacboy: gcc:p cppcheck:p` instala o cppcheck do MSYS2 (2.21);
  - o passo "Versões" mostra `cppcheck --version`;
  - novo passo `make cppcheck` depois do e2e.
- Comentário no topo explica que existe um único job, no mesmo ambiente do desenvolvimento.
- Sai do CI o AddressSanitizer/UBSan, porque o gcc do MinGW não tem ASan.

[README.md](README.md):
- **Instalação dos pré-requisitos (Windows 10/11)**, só com PowerShell:
  1. `winget install --id MSYS2.MSYS2 -e`;
  2. atualização com `C:\msys64\usr\bin\bash.exe -lc "pacman -Syu --noconfirm"`, rodado duas vezes;
  3. instalação de git, make, gcc e cppcheck pelo mesmo `bash.exe -lc "pacman -S ..."`;
  4. as pastas `C:\msys64\ucrt64\bin` e `C:\msys64\usr\bin` entram no `Path` do usuário (com a variante só para a janela atual);
  5. conferência das versões.
- A seção do terminal MSYS2 UCRT64 já tinha sido removida pelo usuário antes desta mudança; ela fica fora.
- Removidas as seções **Linux** (apt/dnf) e **macOS** (xcode/brew), e as menções a `./dist/ohmflow` e a terminais Linux/macOS em "Baixar o projeto" e "Compilar e executar".
- "Opções": `NO_COLOR` aparece na sintaxe do PowerShell (`$env:NO_COLOR=1`).
- "Problemas comuns": saem as linhas do terminal MSYS2 e do `make asan`; entra "`winget` não reconhecido", com a alternativa do instalador em msys2.org.
- "Testes": sai o `make asan`; o `make test` passa a citar também as telas; a tabela de dois jobs dá lugar à descrição do job Windows único.

[Makefile](Makefile):
- Só o comentário do `make asan` mudou: "não roda no MinGW; fora do CI". O alvo foi mantido para quem compilar fora do Windows por conta própria.

`AGENTS.md` (ignorado pelo git, só cópia local):
- O projeto aparece como "suportado somente no Windows".
- O build cita o MSYS2 UCRT64 via PowerShell e diz que os ramos POSIX existem, mas não são suportados.
- A linha do `make asan`, a descrição do CI (um job Windows) e a nota de vazamento de memória (sem ASan no MinGW) foram ajustadas.

O código C não mudou.

### Validação

- cppcheck do MSYS2 instalado localmente (`pacman -S --needed mingw-w64-ucrt-x86_64-cppcheck`, versão 2.21.0, a mesma que o `setup-msys2` instala no CI). `make cppcheck` saiu com código 0 e nenhum apontamento.
- Windows 11 (MSYS2 UCRT64, pelo PowerShell): `make clean; make EXTRA_CFLAGS=-Werror all test e2e cppcheck` passou.
  - `test_lista`: 20 testes; `test_formatacao`: 12; `test_ui`: 14; `test_telas`: 19. Nenhum falhou.
  - `E2E OK (36 verificacoes)`.
- `grep -niE "linux|macos|asan|ubuntu|apt |brew" README.md`: nenhuma ocorrência.
- O fluxo de instalação do README (`bash.exe -lc "pacman ..."` pelo PowerShell) foi o mesmo usado para instalar o cppcheck nesta máquina.
- **Pendente:** a execução real do workflow novo no GitHub Actions, que acontece no push ou PR. Não havia PyYAML localmente para validar a sintaxe do `ci.yml`.
