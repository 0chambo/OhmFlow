/*
 * lista.c — implementação da Lista Dinâmica Ordenada de resistores.
 *
 * Lista simplesmente encadeada, sem nó cabeça: *li aponta para o primeiro
 * elemento (ou NULL se vazia). Os nós ficam em ordem crescente de resistência,
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

#define TOLERANCIA_RELATIVA 1e-9

int mesma_resistencia(double a, double b)
{
    double maior = fabs(a) > fabs(b) ? fabs(a) : fabs(b);
    return fabs(a - b) <= TOLERANCIA_RELATIVA * maior;
}

/* Nó com a resistência pedida, ou NULL. Para cedo graças à ordenação. */
static Elem *busca(Lista *li, double ohms)
{
    Elem *no = *li;
    while (no != NULL && no->dados.resistencia < ohms
           && !mesma_resistencia(no->dados.resistencia, ohms))
        no = no->prox;
    if (no != NULL && mesma_resistencia(no->dados.resistencia, ohms))
        return no;
    return NULL;
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
    while ((no = *li) != NULL) {
        *li = no->prox;
        free(no);
    }
}

/* ---- criação / destruição ------------------------------------------ */

Lista *cria_lista(void)
{
    Lista *li = malloc(sizeof(Lista));
    if (li != NULL) *li = NULL;
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

/* ---- informações --------------------------------------------------- */

int tamanho_lista(Lista *li)
{
    int n = 0;
    const Elem *no;
    if (li == NULL) return 0;
    for (no = *li; no != NULL; no = no->prox) n++;
    return n;
}

long long total_pecas(Lista *li)
{
    long long total = 0; /* int * nós não cabe em long de 32 bits */
    const Elem *no;
    if (li == NULL) return 0;
    for (no = *li; no != NULL; no = no->prox) total += no->dados.quantidade;
    return total;
}

int lista_vazia(Lista *li)
{
    return li == NULL || *li == NULL;
}

void percorre_lista(Lista *li, void (*f)(const Resistor *r, void *ctx), void *ctx)
{
    const Elem *no;
    if (li == NULL || f == NULL) return;
    for (no = *li; no != NULL; no = no->prox) f(&no->dados, ctx);
}

/* ---- inserção ordenada --------------------------------------------- */

int insere_lista_ordenada(Lista *li, Resistor r)
{
    Elem *ant = NULL, *atual, *novo;
    if (li == NULL) return LISTA_ERRO;
    if (!resistencia_valida(r.resistencia) || r.quantidade < 0) return LISTA_INVALIDO;

    /* avança até o primeiro nó com resistência >= r */
    atual = *li;
    while (atual != NULL && atual->dados.resistencia < r.resistencia
           && !mesma_resistencia(atual->dados.resistencia, r.resistencia)) {
        ant = atual;
        atual = atual->prox;
    }
    if (atual != NULL && mesma_resistencia(atual->dados.resistencia, r.resistencia))
        return LISTA_DUPLICADO;

    novo = malloc(sizeof(Elem));
    if (novo == NULL) return LISTA_ERRO;
    novo->dados = r;
    novo->prox = atual;
    if (ant == NULL) *li = novo; /* início (ou lista vazia) */
    else ant->prox = novo;       /* meio ou fim             */
    return LISTA_OK;
}

/* ---- remoção ------------------------------------------------------- */

int remove_lista(Lista *li, double ohms)
{
    Elem *ant = NULL, *no;
    if (li == NULL) return LISTA_ERRO;
    no = *li;
    while (no != NULL && no->dados.resistencia < ohms
           && !mesma_resistencia(no->dados.resistencia, ohms)) {
        ant = no;
        no = no->prox;
    }
    if (no == NULL || !mesma_resistencia(no->dados.resistencia, ohms))
        return LISTA_NAO_ENCONTRADO;
    if (ant == NULL) *li = no->prox;
    else ant->prox = no->prox;
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
    return LISTA_OK;
}
