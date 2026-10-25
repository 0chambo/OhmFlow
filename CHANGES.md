# CHANGES

## fix(lista): total_pecas sem overflow no Windows (issue #6)

### Problema

`total_pecas()` somava as quantidades (`int`) num `long`. No Windows (MinGW, modelo LLP64) `long` tem 32 bits, igual a `int`. Com quantidades grandes (ex.: 1 kΩ com 2147483647 peças + 2 kΩ com 1 peça) a soma estourava: overflow de inteiro com sinal, comportamento indefinido. O total aparecia negativo ou errado em "Peças:" no menu, em Listar, em Reinicializar e na tela de encerramento.

### Correção

- [src/lista.h](src/lista.h) / [src/lista.c](src/lista.c): `total_pecas()` passa a retornar `long long`, e o acumulador também é `long long`. `long long` tem pelo menos 64 bits em qualquer plataforma C99, então cabe `INT_MAX` vezes qualquer número de nós que a memória comporte.
- [src/ui/telas.c](src/ui/telas.c):
  - `plural()` recebe `long long`.
  - `tela_listar`, `tela_reinicializar` e `tela_encerramento` guardam o total em `long long pecas`.
  - Formatos `%ld` trocados por `%lld` (linha "Peças:" do menu, dica de Listar, confirmação e feedback de Reinicializar, resumo de encerramento).
  - O buffer da linha do menu (`linhas[6][160]`) comporta os até 19 dígitos a mais.

### Testes

- [tests/test_lista.c](tests/test_lista.c): novo `teste_total_pecas_sem_overflow`. Insere `INT_MAX` + 1 e confere `2147483648`; depois soma outro `INT_MAX` e confere `4294967295`.
- Antes da correção o teste falhava no Windows (`obtido -2147483648` e `obtido -1`). Depois da correção ele passa.

### Validação (Windows 11, gcc 16.2.0 MSYS2)

- `make clean; make EXTRA_CFLAGS=-Werror all test e2e`: sem warnings, todos os testes passaram, `E2E OK (31 verificacoes)`.
- `test_lista`: 18 testes, 0 falhas, 100 asserts.
- `make cppcheck` não rodou localmente porque o cppcheck não está instalado; o CI roda essa etapa.
