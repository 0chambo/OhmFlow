# Módulo Resistor — código de cores e desenho do resistor

← Anterior: [Formatação](formatacao.md) · [Módulos](README.md) · Próximo: [Pilha](pilha.md)

**Arquivos:** [src/resistor.h](../../src/resistor.h) · [src/resistor.c](../../src/resistor.c) · forma original: [src/assets/ressistor_preencher.svg](../../src/assets/ressistor_preencher.svg) (e `.png`)
**Testes:** [tests/test_resistor.c](../../tests/test_resistor.c)
**Funcionalidade:** [Resistor colorido na tela Novo](../funcionalidades/14-resistor-colorido.md)

> **Resumo:** calcula as **4 faixas de cor** que identificam um valor de resistência (padrão da indústria) e desenha um **resistor em pixel-art colorido** no terminal, usando meio-blocos (`▀`/`▄`) e cores de 24 bits.

---

## 1. Explicação para leigos

Resistores físicos não têm o valor escrito: eles têm **faixas coloridas** pintadas no corpo. Cada cor é um número (preto 0, marrom 1, vermelho 2... branco 9). As duas primeiras faixas são os dois primeiros algarismos, a terceira diz **quantos zeros** acrescentar, e a quarta é a **tolerância** (a margem de erro do componente). Este módulo faz essa "tradução" de número para cores e desenha o resistor na tela, para a pessoa conferir com o componente na mão.

Exemplo: **4,7 kΩ = 4700 Ω** → `4` (amarelo), `7` (violeta), mais **2 zeros** (vermelho), tolerância 5% (dourado).

## 2. Estruturas e constantes

**Tabela 1 — Cores das faixas (`enum` em `resistor.h`)**

| Constante | Algarismo / nº de zeros | Como multiplicador |
| --- | --- | --- |
| `COR_PRETO` | 0 | ×1 |
| `COR_MARROM` | 1 | ×10 |
| `COR_VERMELHO` | 2 | ×100 |
| `COR_LARANJA` | 3 | ×1.000 |
| `COR_AMARELO` | 4 | ×10.000 |
| `COR_VERDE` | 5 | ×100.000 |
| `COR_AZUL` | 6 | ×1.000.000 |
| `COR_VIOLETA` | 7 | ×10.000.000 |
| `COR_CINZA` | 8 | ×100.000.000 |
| `COR_BRANCO` | 9 | ×1.000.000.000 |
| `COR_DOURADO` | — | ×0,1 (e tolerância 5%) |
| `COR_PRATA` | — | ×0,01 |

Também há `RESISTOR_CORPO` (bege), `RESISTOR_CONTORNO` e `RESISTOR_TERMINAL` (cores do desenho) e `SEM_FAIXA` (−1, faixa sem cor).

| Constante | Valor | Significado |
| --- | --- | --- |
| `RESISTOR_FAIXAS` | 4 | Número de faixas |
| `RESISTOR_LINHAS` | 6 | Linhas de texto do desenho (12 pixels de altura, 2 por linha) |
| `RESISTOR_COLUNAS` | 32 | Largura em pixels/colunas |
| `RESISTOR_RECUO` | 2 | Espaços à esquerda (centra dentro da caixa) |
| `RESISTOR_TAM_LINHA` | 2048 | Bytes por linha (cada célula carrega códigos de cor) |

Retornos de `codigo_cores`: `CORES_EXATO`, `CORES_APROXIMADO`, `CORES_FORA`.

## 3. Funções

| Função | Faz |
| --- | --- |
| `codigo_cores(ohms, faixas)` | Preenche as 4 faixas e diz se o código é exato, aproximado ou impossível |
| `nome_cor_faixa(cor)` | Nome em pt-BR (`"violeta"`, `"dourado"`); `"-"` se inválida |
| `resistor_rgb_faixa(cor, buf, n)` | Trecho `;2;R;G;Bm` da sequência ANSI de 24 bits da cor |
| `resistor_arte(faixas, linhas)` | Desenha o resistor em 6 linhas de texto; `faixas == NULL` desenha só contorno e terminais |

## 4. Algoritmo do código de cores

Ideia: escrever o valor como **m × 10^e**, com **m entre 10 e 99** (dois algarismos significativos).

1. `e = floor(log10(ohms)) − 1` e `m = ohms / 10^e`.
2. Corrige o caso em que `log10` arredondou para cima (`m < 9,5`) e o caso em que arredondar `m` deu 100.
3. `n = m` arredondado → faixa 1 = `n / 10`, faixa 2 = `n % 10`.
4. Faixa 3 = `e` se `e` estiver entre 0 e 9; **dourado** se `e = −1`; **prata** se `e = −2`; fora disso, não há código (`CORES_FORA`).
5. Faixa 4 = sempre **dourado** (5%).
6. `CORES_EXATO` se `m` era exatamente `n`; senão `CORES_APROXIMADO` (valores com 3+ algarismos, como 4,75 kΩ, viram 4,8 kΩ nas cores).

**Tabela 2 — Exemplos**

| Valor | m × 10^e | Faixas | Resultado |
| --- | --- | --- | --- |
| 220 Ω | 22 × 10¹ | vermelho · vermelho · marrom · dourado | exato |
| 4,7 kΩ | 47 × 10² | amarelo · violeta · vermelho · dourado | exato |
| 10 kΩ | 10 × 10³ | marrom · preto · laranja · dourado | exato |
| 1 MΩ | 10 × 10⁵ | marrom · preto · verde · dourado | exato |
| 4,7 Ω | 47 × 10⁻¹ | amarelo · violeta · **dourado** · dourado | exato |
| 4,75 kΩ | ≈ 48 × 10² | amarelo · cinza · vermelho · dourado | aproximado |
| 0,05 Ω | — | — | fora (abaixo de 0,1 Ω não há código de 4 faixas) |

## 5. O desenho

O formato vem de [ressistor_preencher.svg](../../src/assets/ressistor_preencher.svg), reduzido a um **mapa de 32 × 12 pixels** em `resistor.c` (`o` contorno, `l` terminal, `c` corpo, `1`–`4` faixas, espaço vazio):

```text
       oooo          oooo
      occ11oooooooooo44cco
     occc11cc22cc33cc44ccco
   oooccc11cc22cc33cc44cccooo
ooolloccc11cc22cc33cc44cccolloo
olloooccc11cc22cc33cc44cccooollo
...
```

Um terminal mostra caracteres, não pixels. O truque é o **meio-bloco** `▀`: a parte de cima usa a cor do texto e a de baixo a cor de fundo, então **cada caractere mostra 2 pixels empilhados**. 12 pixels de altura viram 6 linhas.

Cada pixel recebe uma cor RGB de 24 bits (`ESC[38;2;R;G;Bm` para a frente, `ESC[48;2;R;G;Bm` para o fundo). O corpo e as faixas são escurecidos linha a linha por uma tabela de **sombreamento** (topo mais claro, base mais escura), dando volume.

Sem faixas (`NULL`), só o contorno e os fios aparecem: é o resistor "vazio" mostrado ao abrir a tela Novo.

## 6. Por que foi feito assim

| Decisão | Motivo |
| --- | --- |
| 4 faixas, tolerância fixa dourada | Padrão mais comum nos resistores de laboratório (5%) |
| Arredondar para 2 algarismos e avisar "(aprox.)" | O código de 4 faixas só tem 2 algarismos; melhor mostrar o mais próximo do que nada |
| Módulo separado da tela | Lógica pura testável (`codigo_cores`) sem terminal |
| Meio-blocos + 24 bits | Dobra a resolução vertical e reproduz as cores reais das faixas |
| Mapa de caracteres no código | Sem ler arquivos de imagem em tempo de execução; o `.svg`/`.png` ficam só como referência |

## 7. Testes

[tests/test_resistor.c](../../tests/test_resistor.c), 11 testes: valores comuns, multiplicador fracionário (dourado/prata), valores do roteiro de entrada, aproximação, fora da faixa, nomes das cores, dimensões do desenho, desenho sem cor só com contorno, desenho colorido, faixa ausente usa a cor do corpo e repetição da linha respondida (`ui_linha_respondida`).

---

← Anterior: [Formatação](formatacao.md) · [Módulos](README.md) · Próximo: [Pilha](pilha.md)
