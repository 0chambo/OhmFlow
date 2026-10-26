# CHANGES

## feat(setup): instalador `ohm_flow_setup.exe` para o usuário final (Inno Setup 6)

### Motivação

Para usar o OhmFlow, era preciso seguir o README pelo PowerShell: winget → MSYS2 → pacman (gcc, make, git) → ajuste do `Path` → clone → `make`. Isso é ambiente de **desenvolvimento**. Para só **executar**, nada disso é necessário: `objdump -p dist/ohmflow.exe` mostra que o exe depende apenas de `KERNEL32.dll`, `USER32.dll` e `api-ms-win-crt-*` (UCRT). Todas já vêm no Windows 10/11.

Agora existe um instalador que copia o exe já compilado, cria atalhos e registra o desinstalador. Ele não precisa de MSYS2 nem de internet e, por padrão, não pede administrador.

### Novos arquivos

- **`installer/ohmflow.iss`**: script Inno Setup 6 (UTF-8 com BOM).
  - `AppId` fixo `{8F3C2A71-5D4B-4E9A-B6C1-2A7E9D0F4B53}`: uma versão nova instala por cima da anterior.
  - Versão por `#define Versao` (padrão `0.0.0-dev`), sobrescrita por `/DVersao=` no `make setup`.
  - `PrivilegesRequired=lowest` + `PrivilegesRequiredOverridesAllowed=dialog`: instala por usuário em `%LOCALAPPDATA%\Programs\OhmFlow`, ou para todos em `Program Files` (com admin).
  - `MinVersion=10.0` (UCRT nativa) e x64 (`x64compatible`).
  - Wizard moderno em pt-BR (`BrazilianPortuguese.isl`), ícone `src/assets/ohmflow.ico` e página de licença (`LICENSE`).
  - Arquivos instalados: `ohmflow.exe`, `README.md` e `LICENSE`.
  - Atalhos no Menu Iniciar (OhmFlow + Desinstalar). Atalho no desktop é opcional (desmarcado).
  - Tarefa opcional **"Adicionar o OhmFlow ao PATH"**. O `[Code]` (Pascal) acrescenta `{app}` ao `Path` do usuário (`HKCU\Environment`) ou da máquina, conforme o modo, e o remove na desinstalação sem mexer nas outras entradas. `ChangesEnvironment=yes` avisa o Windows da mudança.
  - Checkbox "Executar OhmFlow" ao final.
  - Saída: `dist/ohm_flow_setup.exe` (lzma2/max, sólido; cerca de 2 MB).
- **`tests/setup/test_setup.ps1`**: teste do instalador (13 verificações), escrito antes da implementação. Ele:
  1. confere que o setup existe;
  2. instala em silêncio (`/VERYSILENT /CURRENTUSER /DIR=<temp> /TASKS=""`);
  3. confere `ohmflow.exe`, `README.md`, `LICENSE` e `unins000.exe`;
  4. roda o exe **instalado** com `tests/e2e/entrada.txt` (`NO_COLOR=1`) e exige código 0 e `Obrigado por usar o OhmFlow`;
  5. confere a chave `HKCU\...\Uninstall\{AppId}_is1` e o `DisplayName`;
  6. desinstala em silêncio e exige que a pasta e a chave sumam.

  Mensagens sem acento, porque o PowerShell 5.1 lê `.ps1` sem BOM como ANSI.

### Arquivos alterados

- **`Makefile`**
  - `VERSAO ?= 1.0.0` (aceita o valor do ambiente ou da linha de comando) e `VERSAO_NUM` derivado (`1.2.3-ci` → `1,2,3,0`).
  - O `windres` recebe `-DVERSAO=$(VERSAO) -DVERSAO_NUM=$(VERSAO_NUM)`.
  - Novo alvo `setup: $(BIN)`: procura o `ISCC.exe` no `PATH`, em `C:\Program Files (x86)\Inno Setup 6` e em `%LOCALAPPDATA%\Programs\Inno Setup 6`. Se não achar, falha com a dica `winget install JRSoftware.InnoSetup`. Chama com `MSYS2_ARG_CONV_EXCL='*'` para o MSYS não converter `/Q` e `/DVersao=` em caminhos.
  - Novo alvo `teste-setup: setup`: `powershell -NoProfile -File tests/setup/test_setup.ps1`.
  - Comentário do topo e `.PHONY` atualizados.
- **`src/assets/ohmflow.rc`**: bloco `VERSIONINFO` (ProductName, FileVersion, ProductVersion, FileDescription, CompanyName, LegalCopyright, idioma pt-BR 0x416). A versão aparece em Propriedades → Detalhes do exe. Tem padrões `0.0.0-dev` / `0,0,0,0` para builds avulsos.
- **`.github/workflows/ci.yml`**
  - Também dispara em push de tags `v*`. O job ganha `permissions: contents: write`.
  - Passo "Versão": tag `v1.2.3` → `VERSAO=1.2.3`; senão `0.0.0-ci` (via `GITHUB_ENV`, então exe e setup ficam com a mesma versão).
  - Instala o Inno Setup (`choco install innosetup`).
  - Novo passo `make teste-setup` depois do cppcheck.
  - Publica `dist/ohm_flow_setup.exe` como artifact `ohm_flow_setup` em todo push/PR.
  - Em tag `v*`, anexa o setup ao Release (`softprops/action-gh-release@v2`).
- **`README.md`**
  - Nova seção no topo, **"Instalação rápida (Windows 10/11)"**: baixar em Releases, opções do assistente, desinstalação e aviso do SmartScreen (setup não assinado).
  - A seção de pré-requisitos virou "Compilar a partir do código: pré-requisitos", com Inno Setup 6 na tabela como opcional.
  - Nova subseção **"Gerar o instalador"** (`make setup`, `VERSAO`, `make teste-setup`, CI/Release).
  - Descrição do CI atualizada.
- **`AGENTS.md`** (local, ignorado pelo Git): arquitetura com `installer/` e `tests/setup/`, comandos `make setup` / `make teste-setup`, notas de encoding e CI.

### Validação

- `make clean; make EXTRA_CFLAGS=-Werror all test e2e cppcheck`: tudo verde (66 testes unitários, e2e com 36 verificações, cppcheck limpo).
- `make teste-setup`: `SETUP OK (13 verificacoes)`.
- Tarefa PATH testada à parte: com `/TASKS="path"`, `{app}` entra no `Path` do usuário. Após desinstalar, o `Path` volta **idêntico** ao original e a pasta é removida.
- `ohmflow.exe` → Propriedades: ProductName `OhmFlow`, versão `1.0.0`.
- `VERSAO` pelo ambiente chega ao `windres` e ao `ISCC` (`make -n`).
- **Falta verificar:** o CI no GitHub (vai rodar no push) e o wizard interativo por duplo clique.

### Fora do escopo

- Assinatura de código (o SmartScreen avisa).
- Instalador do ambiente de desenvolvimento (MSYS2/gcc), que continua pelo README.
- Publicação no winget.
