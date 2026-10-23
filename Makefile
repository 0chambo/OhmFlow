# OhmFlow — build, testes e verificações
#   make            compila dist/ohmflow
#   make run        executa o programa
#   make test       testes unitários
#   make e2e        teste ponta a ponta (menu alimentado por tests/e2e/entrada.txt)
#   make asan       testes + e2e com AddressSanitizer/UBSan (Linux/CI)
#   make cppcheck   análise estática
#   make clean      remove build/ e dist/

CC           = gcc
CFLAGS       = -std=c99 -Wall -Wextra -pedantic -g -Isrc -Isrc/ui
EXTRA_CFLAGS =
LDLIBS       = -lm
BUILD        = build
DIST         = dist

ifeq ($(OS),Windows_NT)
  EXE = .exe
else
  EXE =
endif

SRC_LIB   = src/lista.c src/formatacao.c src/ui/ui.c src/ui/telas.c
SRC_MAIN  = src/main.c
HEADERS   = $(wildcard src/*.h src/ui/*.h)
BIN       = $(DIST)/ohmflow$(EXE)
TESTES    = test_lista test_formatacao test_ui test_telas
TEST_BINS = $(addprefix $(BUILD)/,$(addsuffix $(EXE),$(TESTES)))

E2E_ENTRADA  = tests/e2e/entrada.txt
E2E_ESPERADO = tests/e2e/esperado.txt
E2E_SAIDA    = $(BUILD)/e2e_saida.txt

ASAN_FLAGS = -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer

.PHONY: all run test e2e asan cppcheck clean

all: $(BIN)

$(BUILD):
	mkdir -p $(BUILD)

$(BIN): $(SRC_MAIN) $(SRC_LIB) $(HEADERS)
	mkdir -p $(@D)
	$(CC) $(CFLAGS) $(EXTRA_CFLAGS) -o $@ $(SRC_MAIN) $(SRC_LIB) $(LDLIBS)

$(BUILD)/test_%$(EXE): tests/test_%.c tests/test_framework.h $(SRC_LIB) $(HEADERS) | $(BUILD)
	$(CC) $(CFLAGS) $(EXTRA_CFLAGS) -Itests -o $@ $< $(SRC_LIB) $(LDLIBS)

run: $(BIN)
	./$(BIN)

test: $(TEST_BINS)
	@falhou=0; for t in $(TEST_BINS); do ./$$t || falhou=1; done; exit $$falhou

# Roda o binário real com a entrada roteirizada e confere, EM ORDEM, cada linha
# de esperado.txt contra a saída (linhas vazias de esperado.txt são ignoradas).
e2e: $(BIN) | $(BUILD)
	@NO_COLOR=1 ./$(BIN) < $(E2E_ENTRADA) > $(E2E_SAIDA) 2>&1 || { echo "E2E FALHOU: programa saiu com erro"; exit 1; }
	@awk 'NR==FNR { sub(/\r$$/, ""); if (length($$0)) esp[++n] = $$0; next } \
	     { sub(/\r$$/, ""); if (i < n && index($$0, esp[i+1])) i++ } \
	     END { if (i < n) { print "E2E FALHOU: nao encontrado (em ordem): " esp[i+1]; \
	                        print "Saida completa em $(E2E_SAIDA)"; exit 1 } \
	           print "E2E OK (" n " verificacoes)" }' $(E2E_ESPERADO) $(E2E_SAIDA)

asan:
	$(MAKE) BUILD=$(BUILD)/asan DIST=$(BUILD)/asan EXTRA_CFLAGS="$(EXTRA_CFLAGS) $(ASAN_FLAGS)" test e2e

cppcheck:
	cppcheck --enable=warning,style,performance --error-exitcode=1 --inline-suppr --quiet -Isrc -Isrc/ui src/

clean:
	rm -rf $(BUILD) $(DIST)
