# CHANGES

## feat(novo): resistor desenhado e colorido pelo código de cores na tela Novo

### Resumo

A caixa laranja "Novo resistor", antes vazia, agora mostra o desenho do resistor (mesma forma de `src/assets/ressistor_preencher.svg`). Ao abrir a tela, aparece só o contorno e os terminais (sem cor). Depois que o usuário digita a resistência e tecla Enter, a tela é redesenhada com o corpo bege e as **4 faixas** pintadas pelo código de cores, sempre com a **4ª faixa dourada** (tolerância 5%), e uma linha de apoio com o nome das cores (ex.: `Faixas: amarelo · violeta · vermelho · dourado`).

O terminal não renderiza SVG; o desenho é uma pixel-art de 32x12 pixels tirada do SVG/PNG, impressa em 6 linhas com meio-bloco (`▀`/`▄`) e cor ANSI de 24 bits.

### Regra do código de cores

Tabela usada (cor = algarismo = nº de zeros):

| Cor | 1ª/2ª faixa | Multiplicador |
| --- | --- | --- |
| preto | 0 | x1 |
| marrom | 1 | x10 |
| vermelho | 2 | x100 |
| laranja | 3 | x1k |
| amarelo | 4 | x10k |
| verde | 5 | x100k |
| azul | 6 | x1M |
| violeta | 7 | x10M |
| cinza | 8 | x100M |
| branco | 9 | x1G |
| dourado | - | x0,1 |
| prata | - | x0,01 |

- 4ª faixa: sempre dourada.
- Valor com 3 ou mais algarismos significativos (ex.: 4,75 kΩ): arredondado para 2 (`amarelo · cinza · vermelho`) e marcado `(aprox.)`.
- Abaixo de 0,1 Ω (ex.: 47 mΩ, 1 mΩ): não há código de 4 faixas; o corpo é pintado sem as 3 primeiras faixas (só a dourada) e aparece o aviso "Abaixo de 0,1 Ω: sem código de 4 faixas".
- Exemplos: `4k7` → amarelo · violeta · vermelho · dourado; `470m` → amarelo · violeta · prata · dourado; `1000M` → marrom · preto · cinza · dourado.

### Arquivos

- **`src/resistor.h` / `src/resistor.c`** (novo):
  - `codigo_cores(ohms, faixas)`: calcula as 4 faixas (`m × 10^e`, `m` em [10, 100) arredondado; multiplicador de -2 a 9). Retorna `CORES_EXATO`, `CORES_APROXIMADO` ou `CORES_FORA` (faixas 0..2 = `SEM_FAIXA`).
  - `nome_cor_faixa(cor)`: nome em pt-BR ("-" para inválida/`SEM_FAIXA`).
  - `resistor_rgb_faixa(cor, buf, n)`: trecho `;2;R;G;Bm` da cor (usado nos testes).
  - `resistor_arte(faixas, linhas)`: gera as 6 linhas da arte (recuo de 2 colunas, 32 colunas de desenho, reset no fim). `faixas == NULL` = sem cor (só contorno e terminais). Sombreamento por linha (topo mais claro, base mais escura). Cores RGB da tabela de cores; corpo bege, contorno cinza-escuro (visível no fundo preto do terminal) e terminais cinza-claro.
  - Sem I/O: só monta strings, como `formatacao`.
- **`src/ui/ui.h` / `src/ui/ui.c`**: `ui_linha_respondida(prompt, resposta)` reescreve uma pergunta já respondida com o mesmo visual do eco de `ui_ler_linha` (usada após redesenhar a tela).
- **`src/ui/telas.c`**:
  - `mostra_resistor()`: desenho só no terminal com cores (`ui_interativo() && ui_cor_ativa()`), nunca com `-DOHM_ASCII`. Sem terminal (e2e, testes) ou com `NO_COLOR` a tela fica exatamente como antes.
  - `abre_tela_novo(faixas)`: limpa a tela, centraliza (`ALTURA_NOVO + RESISTOR_LINHAS`), desenha a caixa "Novo resistor" com o resistor dentro e as dicas de formato.
  - `colore_resistor(ohms, digitado)`: após a primeira linha válida, redesenha a tela com o resistor colorido, repete `» Resistência: <digitado>` e mostra os nomes das faixas (ou o aviso de fora da faixa).
  - `tela_inserir`: abre com `abre_tela_novo(NULL)` e chama `colore_resistor` assim que a primeira linha é válida, antes de seguir para a quantidade (unitário) ou para a lista (em massa). No modo em massa o desenho fica com o primeiro item digitado.
  - Supressão `cppcheck-suppress knownConditionTrueFalse` nas duas checagens de `mostra_resistor()` (o cppcheck também analisa a configuração `OHM_ASCII`, em que ela é sempre 0).
- **`Makefile`**: `src/resistor.c` em `SRC_LIB`; `test_resistor` em `TESTES`.
- **`tests/test_resistor.c`** (novo, 11 testes):
  - código de cores para valores comuns, multiplicadores dourado/prata, valores vindos de `parse_resistencia` (`470m`, `1M5`, `1000M`, `4R7`), arredondamento (`4750`, `1234`, `996`) e fora da faixa (`0,047`, `1 mΩ`, `0`, negativo);
  - nomes das cores;
  - arte: largura visível de cada linha (34 colunas), reset no fim, sem cor = só contorno, colorida = cores das faixas e do corpo, `SEM_FAIXA` = cor do corpo;
  - `ui_linha_respondida` sem cor e sem centralização.
- **`AGENTS.md`**: módulo `resistor` e `test_resistor.c` na arquitetura.

### Validação

`make EXTRA_CFLAGS=-Werror all test e2e cppcheck`: todos os testes passam (test_resistor: 11 testes, 150 asserts), `E2E OK (70 verificacoes)`, `E2E LOG OK`, cppcheck sem achados. `test_resistor` também compila e passa com `-DOHM_ASCII`. Arte conferida renderizando a saída real de `resistor_arte` em imagem (sem cor e 4k7).

Não verificado: aparência no terminal real do usuário (depende de suporte a cor de 24 bits; o Windows Terminal suporta).
