/*
 * pilha.c — implementação da Pilha Dinâmica de operações.
 *
 * Pilha simplesmente encadeada com nó descritor: o descritor guarda o ponteiro
 * para o nó do topo (NULL se vazia) e a quantidade de nós. Empilhar e
 * desempilhar mexem só no topo, então ambos são O(1).
 */
#include <stdlib.h>
#include "pilha.h"

struct elemento_pilha {
    Operacao               dados;
    struct elemento_pilha *prox; /* nó de baixo */
};
typedef struct elemento_pilha ElemP;

struct descritor_pilha {
    ElemP *topo;    /* nó do topo, ou NULL se vazia */
    int    tamanho; /* quantidade de nós            */
};

Pilha *cria_pilha(void)
{
    Pilha *p = malloc(sizeof *p);
    if (p != NULL) {
        p->topo = NULL;
        p->tamanho = 0;
    }
    return p;
}

void libera_operacao(Operacao *op)
{
    if (op == NULL) return;
    free(op->itens);
    op->itens = NULL;
    op->n_itens = 0;
}

void esvazia_pilha(Pilha *p)
{
    ElemP *no;
    if (p == NULL) return;
    while ((no = p->topo) != NULL) {
        p->topo = no->prox;
        libera_operacao(&no->dados);
        free(no);
    }
    p->tamanho = 0;
}

void libera_pilha(Pilha *p)
{
    if (p == NULL) return;
    esvazia_pilha(p);
    free(p);
}

int tamanho_pilha(const Pilha *p) { return p != NULL ? p->tamanho : 0; }
int pilha_vazia(const Pilha *p)   { return p == NULL || p->topo == NULL; }

int empilha(Pilha *p, Operacao op)
{
    ElemP *no;
    if (p == NULL || (no = malloc(sizeof *no)) == NULL) {
        libera_operacao(&op); /* a posse dos itens já era da pilha */
        return PILHA_ERRO;
    }
    no->dados = op;
    no->prox = p->topo;
    p->topo = no;
    p->tamanho++;
    return PILHA_OK;
}

int desempilha(Pilha *p, Operacao *op)
{
    ElemP *no;
    if (p == NULL || op == NULL) return PILHA_ERRO;
    if ((no = p->topo) == NULL) return PILHA_VAZIA;
    *op = no->dados;
    p->topo = no->prox;
    p->tamanho--;
    free(no);
    return PILHA_OK;
}

int topo_pilha(const Pilha *p, const Operacao **op)
{
    if (p == NULL || op == NULL) return PILHA_ERRO;
    if (p->topo == NULL) return PILHA_VAZIA;
    *op = &p->topo->dados;
    return PILHA_OK;
}

void percorre_pilha(const Pilha *p, void (*f)(const Operacao *op, void *ctx), void *ctx)
{
    const ElemP *no;
    if (p == NULL || f == NULL) return;
    for (no = p->topo; no != NULL; no = no->prox) f(&no->dados, ctx);
}
