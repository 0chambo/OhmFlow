# OhmFlow — build, testes e verificações
#   make            compila dist/ohmflow
#   make run        executa o programa
#   make test       testes unitários
#   make e2e        teste ponta a ponta (menu alimentado por tests/e2e/entrada.txt)
#   make asan       testes + e2e com ASan/UBSan (nao roda no MinGW; fora do CI)
#   make cppcheck   análise estática
#   make clean      remove build/ e dist/
#   make icone      regenera src/assets/ohmflow.ico a partir do logo (Windows, Pillow)
#   make setup      gera dist/ohm_flow_setup.exe (Windows, Inno Setup 6) [VERSAO=x.y.z]
#   make teste-setup  instala o setup em silencio, roda o programa e desinstala

CC           = gcc
CFLAGS       = -std=c99 -Wall -Wextra -pedantic -g -Isrc -Isrc/ui
EXTRA_CFLAGS =
LDLIBS       = -lm
BUILD        = build
DIST         = dist
VERSAO      ?= 1.0.0

# "1.2.3-ci" -> "1,2,3,0" (FILEVERSION numérico do recurso VERSIONINFO)
virgula      = ,
VERSAO_NUM   = $(subst .,$(virgula),$(firstword $(subst -, ,$(VERSAO)))),0

ifeq ($(OS),Windows_NT)
  EXE     = .exe
  WINDRES = windres
  ICO     = src/assets/ohmflow.ico
  RES_OBJ = $(BUILD)/ohmflow_res.o
  # Subsistema console (CUI): cmd/PowerShell esperam o programa terminar (issue #11)
  CHECA_SUBSISTEMA = objdump -p $(BIN) | grep -q '(Windows CUI)' \
                     || { echo "E2E FALHOU: $(BIN) nao usa o subsistema console (CUI)"; exit 1; }
else
  EXE     =
  WINDRES =
  ICO     =
  RES_OBJ =
  CHECA_SUBSISTEMA = true
endif

SRC_LIB   = src/lista.c src/formatacao.c src/ui/ui.c src/ui/telas.c
SRC_MAIN  = src/main.c
HEADERS   = $(wildcard src/*.h src/ui/*.h)
BIN       = $(DIST)/ohmflow$(EXE)
TESTES    = test_lista test_formatacao test_ui test_telas test_novo
TEST_BINS = $(addprefix $(BUILD)/,$(addsuffix $(EXE),$(TESTES)))

E2E_ENTRADA  = tests/e2e/entrada.txt
E2E_ESPERADO = tests/e2e/esperado.txt
E2E_SAIDA    = $(BUILD)/e2e_saida.txt

ASAN_FLAGS = -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer

SETUP = $(DIST)/ohm_flow_setup.exe

.PHONY: all run test e2e asan cppcheck clean icone setup teste-setup

all: $(BIN)

$(BUILD):
	mkdir -p $(BUILD)

# O .ico é versionado: só é gerado (Python + Pillow) se estiver faltando, para
# um checkout limpo não depender da data de logo.png. Para regenerar após
# trocar o logo: make icone
GERA_ICO = python -c "from PIL import Image; img=Image.open('src/assets/logo.png').convert('RGBA'); img.save('$(ICO)', format='ICO', sizes=[(16,16),(32,32),(48,48),(256,256)])"

$(ICO): | src/assets/logo.png
	$(GERA_ICO)

icone: src/assets/logo.png
	$(GERA_ICO)

$(RES_OBJ): src/assets/ohmflow.rc $(ICO) | $(BUILD)
	$(WINDRES) -DVERSAO=$(VERSAO) -DVERSAO_NUM=$(VERSAO_NUM) $< -o $@

$(BIN): $(SRC_MAIN) $(SRC_LIB) $(RES_OBJ) $(HEADERS)
	mkdir -p $(@D)
	$(CC) $(CFLAGS) $(EXTRA_CFLAGS) -o $@ $(SRC_MAIN) $(SRC_LIB) $(RES_OBJ) $(LDLIBS)

$(BUILD)/test_%$(EXE): tests/test_%.c tests/test_framework.h $(SRC_LIB) $(HEADERS) | $(BUILD)
	$(CC) $(CFLAGS) $(EXTRA_CFLAGS) -Itests -o $@ $< $(SRC_LIB) $(LDLIBS)

run: $(BIN)
	./$(BIN)

test: $(TEST_BINS)
	@falhou=0; for t in $(TEST_BINS); do ./$$t || falhou=1; done; exit $$falhou

# Roda o binário real com a entrada roteirizada e confere, EM ORDEM, cada linha
# de esperado.txt contra a saída (linhas vazias de esperado.txt são ignoradas).
e2e: $(BIN) | $(BUILD)
	@$(CHECA_SUBSISTEMA)
	@NO_COLOR=1 ./$(BIN) < $(E2E_ENTRADA) > $(E2E_SAIDA) 2>&1 || { echo "E2E FALHOU: programa saiu com erro"; exit 1; }
	@awk 'NR==FNR { sub(/\r$$/, ""); if (length($$0)) esp[++n] = $$0; next } \
	     { sub(/\r$$/, ""); if (i < n && index($$0, esp[i+1])) i++ } \
	     END { if (i < n) { print "E2E FALHOU: nao encontrado (em ordem): " esp[i+1]; \
	                        print "Saida completa em $(E2E_SAIDA)"; exit 1 } \
	           print "E2E OK (" n " verificacoes)" }' $(E2E_ESPERADO) $(E2E_SAIDA)

asan:
	$(MAKE) BUILD=$(BUILD)/asan DIST=$(BUILD)/asan EXTRA_CFLAGS="$(EXTRA_CFLAGS) $(ASAN_FLAGS)" test e2e

# Instalador do usuário final. Procura o compilador do Inno Setup no PATH e nas
# pastas padrão (Arquivos de Programas ou %LOCALAPPDATA%\Programs, onde o winget
# instala por usuário). MSYS2_ARG_CONV_EXCL impede o MSYS de converter /Q e
# /DVersao=... em caminhos.
setup: $(BIN)
	@iscc="$$(command -v iscc 2>/dev/null)"; \
	for c in "/c/Program Files (x86)/Inno Setup 6/ISCC.exe" \
	         "$$(cygpath -u "$$LOCALAPPDATA" 2>/dev/null)/Programs/Inno Setup 6/ISCC.exe"; do \
	  if [ -z "$$iscc" ] && [ -f "$$c" ]; then iscc="$$c"; fi; \
	done; \
	[ -n "$$iscc" ] || { echo "SETUP FALHOU: ISCC.exe (Inno Setup 6) nao encontrado. Instale com: winget install JRSoftware.InnoSetup"; exit 1; }; \
	MSYS2_ARG_CONV_EXCL='*' "$$iscc" /Q /DVersao=$(VERSAO) installer/ohmflow.iss
	@echo "Instalador gerado: $(SETUP)"

teste-setup: setup
	powershell -NoProfile -File tests/setup/test_setup.ps1

cppcheck:
	cppcheck --enable=warning,style,performance --error-exitcode=1 --inline-suppr --quiet -Isrc -Isrc/ui src/

clean:
	rm -rf $(BUILD) $(DIST)
