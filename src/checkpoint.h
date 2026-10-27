/*
 * checkpoint.h — voltar o estoque a um ponto anterior da sessão (e avançar).
 *
 * Duas pilhas de operações (pilha.h):
 *   desfazer: ações aplicadas, a mais recente no topo;
 *   refazer : ações desfeitas, a próxima a refazer no topo.
 * Desfazer tira do topo de `desfazer`, aplica a inversa na lista e empilha em
 * `refazer`; refazer faz o caminho contrário. Uma ação nova esvazia `refazer`
 * (o "futuro" desfeito deixa de valer). Tudo vale só para a sessão atual: o
 * estoque não é salvo entre execuções.
 *
 * Posições: 0 é o início da sessão e i (1..total) é o estado logo depois da
 * i-ésima ação, em ordem cronológica. A posição atual é o tamanho de
 * `desfazer`; o total soma as duas pilhas.
 */
#ifndef CHECKPOINT_H
#define CHECKPOINT_H

#include "lista.h"
#include "pilha.h"

typedef struct checkpoint Checkpoint; /* definida em checkpoint.c */

/* Códigos de retorno */
enum {
    CK_OK       =  1,
    CK_ERRO     =  0, /* checkpoint/lista NULL, memória ou inversa falhou */
    CK_VAZIO    = -1, /* nada para desfazer/refazer                       */
    CK_INVALIDO = -2  /* posição fora de 0..total                         */
};

/* Sentido informado ao aviso de ck_ir_para */
enum { CK_DESFEITA = 0, CK_REFEITA };

Checkpoint *cria_checkpoint(void);
void        libera_checkpoint(Checkpoint *c);

/* ---- registro (chamar logo depois de a ação dar LISTA_OK) ---- */

/* Registra op (o checkpoint passa a ser dono de op.itens) e esvazia a pilha
 * de refazer. Se faltar memória, o histórico inteiro é descartado, para nunca
 * ficar uma inversa que não bate mais com o estoque */
int ck_registra(Checkpoint *c, Operacao op);

/* Atalhos que copiam os dados (o chamador continua dono dos seus) */
int ck_registra_novo(Checkpoint *c, const Resistor *itens, int n); /* 1 ou lote */
int ck_registra_remover(Checkpoint *c, Resistor removido);
int ck_registra_movimento(Checkpoint *c, int tipo, double ohms, int qtd); /* RETIRAR/REPOR */
/* Tira a foto do estoque ANTES de reinicializar; lista vazia não registra */
int ck_registra_reinicializar(Checkpoint *c, const Lista *li);

/* Descarta todo o histórico (as duas pilhas). Use quando uma mudança do
 * estoque não pôde ser registrada */
void ck_descarta(Checkpoint *c);

/* ---- navegação ---- */

int ck_desfazer(Checkpoint *c, Lista *li);
int ck_refazer(Checkpoint *c, Lista *li);

/* Desfaz/refaz até a posição `alvo` (0..total). aviso(sentido, op, ctx), se
 * não for NULL, é chamado a cada operação desfeita ou refeita */
int ck_ir_para(Checkpoint *c, Lista *li, int alvo,
               void (*aviso)(int sentido, const Operacao *op, void *ctx), void *ctx);

int ck_posicao(const Checkpoint *c); /* ações aplicadas agora      */
int ck_total(const Checkpoint *c);   /* aplicadas + desfeitas      */

/* i-ésima ação da sessão (1..total, cronológica), aplicada ou desfeita; NULL
 * fora da faixa */
const Operacao *ck_operacao(const Checkpoint *c, int i);

#endif /* CHECKPOINT_H */
