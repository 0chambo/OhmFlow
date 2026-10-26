/*
 * lista.c — implementação da Lista Dinâmica Ordenada de resistores.
 *
 * Lista simplesmente encadeada com nó descritor: o descritor guarda o ponteiro
 * para o primeiro elemento (NULL se vazia) e os totais de tipos e peças,
 * atualizados a cada operação. Os nós ficam em ordem crescente de resistência,
 * o que permite encerrar as buscas assim que passamos do valor procurado.
 */
#include <stdlib.h>
#include <limits.h>
#include <math.h>
#include "lista.h"

struct elemento {
    Resistor         dados;
    struct elemento *prox;
};
typedef struct elemento Elem;

struct descritor {
    Elem     *inicio; /* primeiro nó, ou NULL se vazia     */
    int       tipos;  /* quantidade de nós                 */
    long long pecas;  /* soma das quantidades (long long: int * nós não cabe em long de 32 bits) */
    int       passos; /* nós examinados pela última busca  */
};

#define TOLERANCIA_RELATIVA 1e-9

int mesma_resistencia(double a, double b)
{
    double maior = fabs(a) > fabs(b) ? fabs(a) : fabs(b);
    return fabs(a - b) <= TOLERANCIA_RELATIVA * maior;
}

/*
 * Endereço do ponteiro que aponta (ou apontaria) para o nó `ohms`: o campo
 * `inicio` do descritor ou o `prox` do nó anterior. Para no primeiro nó com
 * resistência >= ohms, graças à ordenação. Com o ponteiro para ponteiro,
 * inserir e remover no início, no meio ou no fim é o mesmo código.
 */
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

/* 1 se o nó existe e tem a resistência procurada */
static int e_o_no(const Elem *no, double ohms)
{
    return no != NULL && mesma_resistencia(no->dados.resistencia, ohms);
}

/* Nó com a resistência pedida, ou NULL */
static Elem *busca(Lista *li, double ohms)
{
    Elem *no = *localiza(li, ohms);
    return e_o_no(no, ohms) ? no : NULL;
}

static int resistencia_valida(double ohms)
{
    return !isnan(ohms)
        && ohms >= RESISTENCIA_MIN * (1 - TOLERANCIA_RELATIVA)
        && ohms <= RESISTENCIA_MAX * (1 + TOLERANCIA_RELATIVA);
}

static void libera_nos(Lista *li)
{
    Elem *no;
    while ((no = li->inicio) != NULL) {
        li->inicio = no->prox;
        free(no);
    }
    li->tipos = 0;
    li->pecas = 0;
}

/* ---- criação / destruição ------------------------------------------ */

Lista *cria_lista(void)
{
    Lista *li = malloc(sizeof(Lista));
    if (li != NULL) {
        li->inicio = NULL;
        li->tipos = 0;
        li->pecas = 0;
        li->passos = 0;
    }
    return li;
}

void libera_lista(Lista *li)
{
    if (li == NULL) return;
    libera_nos(li);
    free(li);
}

int reinicializa_lista(Lista *li)
{
    if (li == NULL) return LISTA_ERRO;
    libera_nos(li);
    return LISTA_OK;
}

/* ---- informações (O(1): lidas do descritor) ------------------------- */

int tamanho_lista(Lista *li)
{
    return li == NULL ? 0 : li->tipos;
}

long long total_pecas(Lista *li)
{
    return li == NULL ? 0 : li->pecas;
}

int lista_vazia(Lista *li)
{
    return li == NULL || li->inicio == NULL;
}

int lista_passos(Lista *li)
{
    return li == NULL ? 0 : li->passos;
}

void percorre_lista(Lista *li, void (*f)(const Resistor *r, void *ctx), void *ctx)
{
    const Elem *no;
    if (li == NULL || f == NULL) return;
    for (no = li->inicio; no != NULL; no = no->prox) f(&no->dados, ctx);
}

/* ---- inserção ordenada --------------------------------------------- */

int insere_lista_ordenada(Lista *li, Resistor r)
{
    Elem **pp, *novo;
    if (li == NULL) return LISTA_ERRO;
    if (!resistencia_valida(r.resistencia) || r.quantidade < 0) return LISTA_INVALIDO;

    pp = localiza(li, r.resistencia); /* primeiro nó com resistência >= r */
    if (e_o_no(*pp, r.resistencia)) return LISTA_DUPLICADO;

    novo = malloc(sizeof(Elem));
    if (novo == NULL) return LISTA_ERRO;
    novo->dados = r;
    novo->prox = *pp; /* início, meio ou fim: o mesmo encadeamento */
    *pp = novo;
    li->tipos++;
    li->pecas += r.quantidade;
    return LISTA_OK;
}

/* ---- remoção ------------------------------------------------------- */

int remove_lista(Lista *li, double ohms)
{
    Elem **pp, *no;
    if (li == NULL) return LISTA_ERRO;
    pp = localiza(li, ohms);
    no = *pp;
    if (!e_o_no(no, ohms)) return LISTA_NAO_ENCONTRADO;
    *pp = no->prox; /* o anterior (ou o início) passa a apontar para o seguinte */
    li->tipos--;
    li->pecas -= no->dados.quantidade;
    free(no);
    return LISTA_OK;
}

/* ---- consulta e movimentação de estoque ---------------------------- */

int consulta_lista(Lista *li, double ohms, int *qtd)
{
    Elem *no;
    if (li == NULL) return LISTA_ERRO;
    no = busca(li, ohms);
    if (no == NULL) return LISTA_NAO_ENCONTRADO;
    if (qtd != NULL) *qtd = no->dados.quantidade;
    return LISTA_OK;
}

int retira_estoque(Lista *li, double ohms, int qtd)
{
    Elem *no;
    if (li == NULL) return LISTA_ERRO;
    if (qtd <= 0) return LISTA_INVALIDO;
    no = busca(li, ohms);
    if (no == NULL) return LISTA_NAO_ENCONTRADO;
    if (qtd > no->dados.quantidade) return LISTA_SEM_ESTOQUE;
    no->dados.quantidade -= qtd; /* pode zerar: o tipo continua cadastrado */
    li->pecas -= qtd;
    return LISTA_OK;
}

int repor_estoque(Lista *li, double ohms, int qtd)
{
    Elem *no;
    if (li == NULL) return LISTA_ERRO;
    if (qtd <= 0) return LISTA_INVALIDO;
    no = busca(li, ohms);
    if (no == NULL) return LISTA_NAO_ENCONTRADO;
    if (no->dados.quantidade > INT_MAX - qtd) return LISTA_INVALIDO; /* overflow */
    no->dados.quantidade += qtd;
    li->pecas += qtd;
    return LISTA_OK;
}
