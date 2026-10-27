# Módulo Lista — TAD Lista Dinâmica Ordenada

← [Módulos](README.md) · Próximo: [Formatação](formatacao.md)

**Arquivos:** [src/lista.h](../../src/lista.h) (interface) · [src/lista.c](../../src/lista.c) (implementação)
**Testes:** [tests/test_lista.c](../../tests/test_lista.c)

> **Resumo:** guarda os resistores do estoque numa lista simplesmente encadeada, **sempre em ordem crescente de resistência**, com um nó descritor que mantém os totais de tipos e peças. É a estrutura central exigida pelo enunciado.

---

## 1. Explicação para leigos

Imagine um **fichário de cartões** onde cada cartão tem um valor de resistor e a quantidade na gaveta. A regra do fichário é: **os cartões ficam sempre em ordem de valor**. Para guardar um cartão novo, você percorre o fichário do começo e o encaixa logo antes do primeiro cartão de valor maior. Para achar um valor, você também percorre do começo e **para assim que passa dele**: se procura 5 kΩ e já chegou ao 10 kΩ, sabe que o 5 kΩ não existe.

Na frente do fichário fica uma **etiqueta** (o descritor) com "quantos cartões" e "quantas peças no total", atualizada a cada mudança. Assim ninguém precisa recontar tudo para responder a essas perguntas.

## 2. Estruturas

### 2.1 `Resistor` (público)

```c
typedef struct {
    double resistencia; /* valor em Ω (chave de ordenação)   */
    int    quantidade;  /* unidades disponíveis no estoque    */
} Resistor;
```

É a "carga" de cada nó. A resistência é sempre guardada em **ohms**; a conversão para mΩ/kΩ/MΩ só acontece na hora de imprimir ([Formatação](formatacao.md)).

### 2.2 `struct elemento` — o nó (privado)

```c
struct elemento {
    Resistor         dados;
    struct elemento *prox;   /* próximo nó, ou NULL no último */
};
typedef struct elemento Elem;
```

### 2.3 `struct descritor` — a lista (privado, exposto como `Lista`)

```c
struct descritor {
    Elem     *inicio; /* primeiro nó, ou NULL se vazia     */
    int       tipos;  /* quantidade de nós                 */
    long long pecas;  /* soma das quantidades              */
    int       passos; /* nós examinados pela última busca  */
};
```

No cabeçalho, só aparece `typedef struct descritor Lista;`. Esse é o **tipo opaco**: fora de `lista.c`, ninguém consegue ler `inicio` ou `prox`, apenas usar as funções públicas ([por quê](#7-por-que-foi-feito-assim)).

### 2.4 Constantes

| Constante | Valor | Uso |
| --- | --- | --- |
| `RESISTENCIA_MIN` | `1e-3` (1 mΩ) | Menor resistência aceita |
| `RESISTENCIA_MAX` | `1e9` (1000 MΩ) | Maior resistência aceita |
| `TOLERANCIA_RELATIVA` (privada) | `1e-9` | Margem para comparar dois `double` |

### 2.5 Códigos de retorno

| Código | Valor | Quando |
| --- | --- | --- |
| `LISTA_OK` | 1 | Sucesso |
| `LISTA_ERRO` | 0 | Lista `NULL` ou falta de memória |
| `LISTA_NAO_ENCONTRADO` | -1 | Resistência não cadastrada |
| `LISTA_DUPLICADO` | -2 | Resistência já cadastrada |
| `LISTA_SEM_ESTOQUE` | -3 | Retirada maior que o disponível |
| `LISTA_INVALIDO` | -4 | Resistência fora da faixa, quantidade ≤ 0 em movimentos, negativa no cadastro, ou soma que estouraria o `int` |

## 3. O coração: `localiza`

Todas as buscas passam por uma única função interna:

```c
static Elem **localiza(Lista *li, double ohms)
{
    Elem **pp = &li->inicio;
    li->passos = 0;
    while (*pp != NULL) {
        li->passos++;
        if (!((*pp)->dados.resistencia < ohms
              && !mesma_resistencia((*pp)->dados.resistencia, ohms)))
            break;
        pp = &(*pp)->prox;
    }
    return pp;
}
```

Em português: "comece pelo engate de entrada (`inicio`); enquanto houver vagão e ele for **menor** que o procurado, avance para o engate dele (`prox`). Pare no primeiro vagão maior ou igual, ou no fim."

O detalhe que faz a diferença é o retorno: **não o nó, mas o endereço do ponteiro que aponta para ele** (`Elem **`). Esse ponteiro é ou o campo `inicio` do descritor, ou o `prox` do nó anterior. Com ele em mãos:

- **Inserir** é `novo->prox = *pp; *pp = novo;`
- **Remover** é `*pp = no->prox; free(no);`

…e as duas linhas funcionam **igualzinho** no início, no meio e no fim da lista. Não há variável `ant` ("anterior") nem `if` especial para o primeiro nó, como na versão vista em aula.

A Tabela 1 mostra quantos nós `localiza` examina na lista `220 Ω → 4,7 kΩ → 10 kΩ`. O número fica em `lista_passos(li)`.

**Tabela 1 — Parada antecipada da busca ordenada**

| Procurando | Nós examinados | Por quê |
| --- | --- | --- |
| 100 Ω | 1 | o primeiro nó (220 Ω) já é maior: para ali |
| 220 Ω | 1 | achou no primeiro |
| 4,7 kΩ | 2 | achou no segundo |
| 5 kΩ | 3 | para em 10 kΩ, que é maior: não existe |
| 1 MΩ | 3 | chega ao fim da lista |

Funções auxiliares privadas:

| Função | O que faz |
| --- | --- |
| `e_o_no(no, ohms)` | 1 se `no` não é `NULL` e tem a resistência procurada |
| `busca(li, ohms)` | `*localiza(...)` + `e_o_no`: devolve o nó ou `NULL` |
| `resistencia_valida(ohms)` | Não é `NaN` e está entre 1 mΩ e 1000 MΩ (com a tolerância) |
| `libera_nos(li)` | Dá `free` em todos os nós e zera os totais |

## 4. Funções públicas

**Tabela 2 — API da lista**

| Função | Recebe | Faz | Devolve | Custo |
| --- | --- | --- | --- | --- |
| `cria_lista()` | — | Aloca o descritor vazio | `Lista*` ou `NULL` | O(1) |
| `libera_lista(li)` | lista (pode ser `NULL`) | Libera todos os nós e o descritor | — | O(n) |
| `reinicializa_lista(li)` | lista | Libera os nós, mantém o descritor | `LISTA_OK`/`ERRO` | O(n) |
| `tamanho_lista(li)` | lista | Lê `tipos` | número de tipos (0 se `NULL`) | O(1) |
| `total_pecas(li)` | lista | Lê `pecas` | soma das quantidades | O(1) |
| `lista_vazia(li)` | lista | `inicio == NULL` | 1/0 | O(1) |
| `insere_lista_ordenada(li, r)` | lista, resistor | Encaixa na posição certa | `OK`, `DUPLICADO`, `INVALIDO`, `ERRO` | O(n) |
| `remove_lista(li, ohms)` | lista, valor | Tira o nó e libera | `OK`, `NAO_ENCONTRADO`, `ERRO` | O(n) |
| `consulta_lista(li, ohms, &qtd)` | lista, valor, onde gravar | Lê a quantidade (`qtd` pode ser `NULL`) | `OK`, `NAO_ENCONTRADO`, `ERRO` | O(n) |
| `retira_estoque(li, ohms, qtd)` | lista, valor, qtd > 0 | Subtrai | `OK`, `SEM_ESTOQUE`, `NAO_ENCONTRADO`, `INVALIDO`, `ERRO` | O(n) |
| `repor_estoque(li, ohms, qtd)` | lista, valor, qtd > 0 | Soma (sem estourar `INT_MAX`) | `OK`, `NAO_ENCONTRADO`, `INVALIDO`, `ERRO` | O(n) |
| `percorre_lista(li, f, ctx)` | lista, função, contexto | Chama `f(&resistor, ctx)` para cada nó, em ordem | — | O(n) |
| `lista_passos(li)` | lista | Lê `passos` | nós examinados na última busca | O(1) |
| `mesma_resistencia(a, b)` | dois valores | Compara com tolerância relativa 1e-9 | 1/0 | O(1) |

Todas as funções aceitam `li == NULL` sem travar (devolvem `LISTA_ERRO`, 0 ou não fazem nada). Isso é testado em `teste_lista_nula`.

## 5. Algoritmos passo a passo

### 5.1 Inserir

A Figura 1 mostra o fluxo de `insere_lista_ordenada`.

```mermaid
flowchart TD
    S(["insere_lista_ordenada(li, r)"]) --> L{"li é NULL?"}
    L -- sim --> E0(["LISTA_ERRO"])
    L -- não --> V{"resistência na faixa<br/>e quantidade >= 0?"}
    V -- não --> E1(["LISTA_INVALIDO"])
    V -- sim --> I["pp = localiza(li, r.resistencia)"]
    I --> D{"*pp tem a mesma resistência?"}
    D -- sim --> E2(["LISTA_DUPLICADO"])
    D -- não --> M["novo = malloc(nó)"]
    M --> MF{"malloc falhou?"}
    MF -- sim --> E3(["LISTA_ERRO"])
    MF -- não --> P["novo->dados = r<br/>novo->prox = *pp<br/>*pp = novo"]
    P --> C["tipos++ · pecas += quantidade"]
    C --> OK(["LISTA_OK"])
```

**Figura 1 — Fluxo da inserção ordenada**

Exemplo: inserir **5 kΩ** em `220 Ω → 4,7 kΩ → 10 kΩ`. `localiza` para em 10 kΩ e devolve o endereço do `prox` do 4,7 kΩ. O novo nó aponta para 10 kΩ, e o `prox` do 4,7 kΩ passa a apontar para o novo, como na Figura 2.

```mermaid
flowchart LR
    subgraph antes["Antes"]
        direction LR
        a1["220 Ω"] --> a2["4,7 kΩ"] --> a3["10 kΩ"] --> a4["NULL"]
    end
    subgraph depois["Depois"]
        direction LR
        b1["220 Ω"] --> b2["4,7 kΩ"] --> b3["5 kΩ (novo)"] --> b4["10 kΩ"] --> b5["NULL"]
    end
    antes --> depois
```

**Figura 2 — Inserção no meio: só dois ponteiros mudam**

**Tabela 3 — Casos de borda da inserção**

| Caso | O que acontece |
| --- | --- |
| Lista vazia | `pp` é `&li->inicio` e `*pp` é `NULL`: o novo vira o único nó |
| Menor que todos | `pp` é `&li->inicio`: o novo vira o primeiro |
| Maior que todos | `pp` é o `prox` do último (que vale `NULL`): o novo vira o último |
| Já cadastrada (inclusive `470m` × `0.47`) | `LISTA_DUPLICADO`, nada muda |
| Fora de 1 mΩ..1000 MΩ, `NaN` ou quantidade negativa | `LISTA_INVALIDO` |
| Quantidade zero | Permitida: o tipo existe, sem peças |

### 5.2 Remover

```mermaid
flowchart TD
    S(["remove_lista(li, ohms)"]) --> L{"li é NULL?"}
    L -- sim --> E0(["LISTA_ERRO"])
    L -- não --> I["pp = localiza(li, ohms) · no = *pp"]
    I --> F{"no tem a resistência?"}
    F -- não --> E1(["LISTA_NAO_ENCONTRADO"])
    F -- sim --> T["*pp = no->prox"]
    T --> C["tipos-- · pecas -= no.quantidade"]
    C --> FR["free(no)"]
    FR --> OK(["LISTA_OK"])
```

**Figura 3 — Fluxo da remoção**

Ao remover 4,7 kΩ de `220 Ω → 4,7 kΩ → 10 kΩ`, o `prox` do 220 Ω passa a apontar direto para o 10 kΩ, e o nó do meio é liberado. Remover o primeiro troca `li->inicio`; remover o último faz o penúltimo apontar para `NULL`. Tudo com a mesma linha `*pp = no->prox`.

### 5.3 Retirar, repor e consultar

As três usam `busca` (ou seja, `localiza`) para achar o nó e **só mexem no campo `quantidade`** e no total `pecas`. A forma da lista e a ordem nunca mudam.

```mermaid
flowchart TD
    S(["retira_estoque / repor_estoque(li, ohms, qtd)"]) --> L{"li é NULL?"}
    L -- sim --> E0(["LISTA_ERRO"])
    L -- não --> Q{"qtd <= 0?"}
    Q -- sim --> E1(["LISTA_INVALIDO"])
    Q -- não --> B["no = busca(li, ohms)"]
    B --> F{"achou?"}
    F -- não --> E2(["LISTA_NAO_ENCONTRADO"])
    F -- sim --> T{"operação"}
    T -- retirar --> R1{"qtd > no.quantidade?"}
    R1 -- sim --> E3(["LISTA_SEM_ESTOQUE"])
    R1 -- não --> R2["quantidade -= qtd · pecas -= qtd"]
    T -- repor --> P1{"quantidade > INT_MAX - qtd?"}
    P1 -- sim --> E4(["LISTA_INVALIDO"])
    P1 -- não --> P2["quantidade += qtd · pecas += qtd"]
    R2 --> OK(["LISTA_OK"])
    P2 --> OK
```

**Figura 4 — Fluxo de retirar e repor**

Detalhes:

- **Retirar até zero é permitido.** O tipo continua cadastrado com quantidade 0; para apagá-lo, use `remove_lista`.
- **O teste de estouro** (`quantidade > INT_MAX - qtd`) é escrito assim de propósito: calcular `quantidade + qtd` primeiro poderia estourar antes da comparação, o que em C é comportamento indefinido.
- **Não existe "editar resistência".** A chave nunca é alterada no lugar, porque isso poderia quebrar a ordem. Para corrigir um valor, remove-se e insere-se de novo.

### 5.4 Percorrer

`percorre_lista(li, f, ctx)` visita os nós do início ao fim e chama `f` para cada um. É um **callback**: quem chama passa "o que fazer com cada resistor", e o `ctx` carrega dados extras (por exemplo, o contador de linhas da tabela). Assim, `telas.c` desenha a tabela e `checkpoint.c` tira a "foto" do estoque **sem conhecer os nós**.

## 6. Invariantes (o que é sempre verdade)

Depois de qualquer operação pública:

1. Os nós estão em **ordem estritamente crescente** de resistência (nunca dois iguais).
2. `tipos` é igual ao número de nós.
3. `pecas` é igual à soma das quantidades.
4. Toda quantidade é ≥ 0.
5. Toda resistência está entre 1 mΩ e 1000 MΩ.

O teste `teste_contadores_invariantes` faz dezenas de operações e confere 2 e 3 recontando a lista com `percorre_lista`.

## 7. Por que foi feito assim

| Decisão | Motivo | Alternativa descartada |
| --- | --- | --- |
| Lista encadeada em vez de vetor | Exigência do enunciado; inserção ordenada sem deslocar elementos | Vetor ordenado (busca binária, mas inserção desloca tudo) |
| **Nó descritor** | Totais em O(1) para o menu, que é redesenhado o tempo todo | Recontar a lista a cada desenho do menu |
| **Tipo opaco** | Ninguém fora de `lista.c` consegue quebrar a ordem ou os totais | Struct pública (mais simples, mas frágil) |
| **Ponteiro para ponteiro** em `localiza` | Um só código para início/meio/fim; menos casos especiais, menos bugs | Ponteiros `ant` e `atual` com `if (ant == NULL)` |
| Uma função de busca para tudo | A regra de comparação fica em um lugar só | Laços de busca repetidos em cada operação |
| `double` em Ω | Cobre de 0,001 a 1.000.000.000 com casas decimais | Inteiro em mΩ (1000 MΩ = 10¹² mΩ não cabe em `int`) |
| **Comparação com tolerância** | Rede de segurança contra arredondamento binário | `==` direto entre `double` |
| `long long` em `pecas` | No Windows `long` tem 32 bits; muitos tipos com quantidade alta estourariam | `long` |
| Códigos de retorno em vez de mensagens | A lista não sabe nada de tela; quem chama decide o texto | `printf` dentro da lista |
| `passos` no descritor | Mostra a parada antecipada nos testes e no relatório | Não medir |

## 8. Testes

[tests/test_lista.c](../../tests/test_lista.c) tem 20 testes, entre eles: lista nova vazia; inserção no início, meio e fim; duplicados; valores inválidos; remoção do primeiro, do meio, do último e do único; consulta; retirada parcial, total e além do estoque; reposição e estouro de `INT_MAX`; total de peças sem estouro; reinicialização; invariantes; contagem de passos; chamadas com `NULL`; e `mesma_resistencia`.

---

← [Módulos](README.md) · Próximo: [Formatação](formatacao.md)
