/*
 * pilha.h — TAD Pilha Dinâmica de operações do estoque.
 *
 * Pilha encadeada (LIFO) vista em aula: cada nó guarda uma Operacao feita no
 * estoque, com o necessário para desfazê-la e refazê-la. É a base do
 * checkpoint (checkpoint.h), que usa duas pilhas: desfazer e refazer.
 * "Pilha" é um nó descritor (opaco): guarda o topo e o tamanho, para que
 * tamanho_pilha saia em O(1).
 */
#ifndef PILHA_H
#define PILHA_H

#include <time.h>
#include "lista.h"

/* Tipos de operação guardados na pilha (só as que alteram o estoque) */
enum { OP_NOVO = 0, OP_REMOVER, OP_RETIRAR, OP_REPOR, OP_REINICIALIZAR };

typedef struct {
    int       tipo;        /* OP_*                                          */
    time_t    quando;      /* instante em que a ação foi feita              */
    double    resistencia; /* RETIRAR/REPOR: resistor movimentado (Ω)       */
    int       qtd;         /* RETIRAR/REPOR: unidades movimentadas          */
    Resistor *itens;       /* NOVO (1 ou lote), REMOVER (1), REINICIALIZAR
                              (foto do estoque); NULL nas demais            */
    int       n_itens;
} Operacao;

typedef struct descritor_pilha Pilha; /* definida em pilha.c */

/* Códigos de retorno */
enum {
    PILHA_OK    =  1,
    PILHA_ERRO  =  0, /* pilha NULL ou falta de memória */
    PILHA_VAZIA = -1  /* nada para desempilhar          */
};

Pilha *cria_pilha(void);
void   libera_pilha(Pilha *p);   /* libera os nós e os itens de cada operação */
void   esvazia_pilha(Pilha *p);  /* idem, mas a pilha continua usável         */

int    tamanho_pilha(const Pilha *p);
int    pilha_vazia(const Pilha *p);

/* Empilha op; a pilha passa a ser dona de op.itens (mesmo se falhar, quando
 * os itens são liberados) */
int    empilha(Pilha *p, Operacao op);

/* Desempilha o topo em *op; a posse de op->itens volta para o chamador */
int    desempilha(Pilha *p, Operacao *op);

/* Aponta *op para o topo, sem remover */
int    topo_pilha(const Pilha *p, const Operacao **op);

/* Chama f(op, ctx) para cada operação, do topo para a base */
void   percorre_pilha(const Pilha *p, void (*f)(const Operacao *op, void *ctx), void *ctx);

/* Libera op->itens e zera o campo (para operações já desempilhadas) */
void   libera_operacao(Operacao *op);

#endif /* PILHA_H */
