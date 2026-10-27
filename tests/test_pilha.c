/* test_pilha.c — testes do TAD Pilha Dinâmica de operações (checkpoint) */
#include <stdlib.h>
#include "test_framework.h"
#include "pilha.h"

/* ---- auxiliares ---------------------------------------------------- */

/* Operação RETIRAR/REPOR simples (sem itens) */
static Operacao op_mov(int tipo, double ohms, int qtd)
{
    Operacao op;
    op.tipo = tipo;
    op.quando = 0;
    op.resistencia = ohms;
    op.qtd = qtd;
    op.itens = NULL;
    op.n_itens = 0;
    return op;
}

/* Operação com `n` itens alocados (a pilha passa a ser dona deles) */
static Operacao op_itens(int tipo, int n)
{
    Operacao op = op_mov(tipo, 0, 0);
    int i;
    op.itens = malloc((size_t)n * sizeof *op.itens);
    if (op.itens == NULL) return op;
    for (i = 0; i < n; i++) {
        op.itens[i].resistencia = 100.0 * (i + 1);
        op.itens[i].quantidade = i + 1;
    }
    op.n_itens = n;
    return op;
}

#define MAX_COLETA 16

typedef struct {
    int n;
    int qtd[MAX_COLETA];
} Coleta;

static void coleta_cb(const Operacao *op, void *ctx)
{
    Coleta *c = ctx;
    if (c->n < MAX_COLETA) c->qtd[c->n++] = op->qtd;
}

/* ---- testes -------------------------------------------------------- */

static void teste_pilha_nova_vazia(void)
{
    Pilha *p = cria_pilha();
    const Operacao *topo;
    Operacao op;
    ASSERT_TRUE(p != NULL);
    ASSERT_EQ_INT(1, pilha_vazia(p));
    ASSERT_EQ_INT(0, tamanho_pilha(p));
    ASSERT_EQ_INT(PILHA_VAZIA, topo_pilha(p, &topo));
    ASSERT_EQ_INT(PILHA_VAZIA, desempilha(p, &op));
    libera_pilha(p);
}

static void teste_pilha_lifo(void)
{
    Pilha *p = cria_pilha();
    Operacao op;
    ASSERT_EQ_INT(PILHA_OK, empilha(p, op_mov(OP_RETIRAR, 220, 1)));
    ASSERT_EQ_INT(PILHA_OK, empilha(p, op_mov(OP_REPOR, 220, 2)));
    ASSERT_EQ_INT(PILHA_OK, empilha(p, op_mov(OP_RETIRAR, 220, 3)));
    ASSERT_EQ_INT(3, tamanho_pilha(p));
    ASSERT_EQ_INT(0, pilha_vazia(p));

    ASSERT_EQ_INT(PILHA_OK, desempilha(p, &op));
    ASSERT_EQ_INT(3, op.qtd);
    ASSERT_EQ_INT(OP_RETIRAR, op.tipo);
    ASSERT_EQ_INT(PILHA_OK, desempilha(p, &op));
    ASSERT_EQ_INT(2, op.qtd);
    ASSERT_EQ_INT(OP_REPOR, op.tipo);
    ASSERT_EQ_INT(PILHA_OK, desempilha(p, &op));
    ASSERT_EQ_INT(1, op.qtd);
    ASSERT_EQ_INT(1, pilha_vazia(p));
    libera_pilha(p);
}

static void teste_pilha_topo_nao_remove(void)
{
    Pilha *p = cria_pilha();
    const Operacao *topo = NULL;
    empilha(p, op_mov(OP_REPOR, 4700, 7));
    ASSERT_EQ_INT(PILHA_OK, topo_pilha(p, &topo));
    ASSERT_TRUE(topo != NULL);
    if (topo != NULL) {
        ASSERT_EQ_INT(7, topo->qtd);
        ASSERT_EQ_DBL(4700.0, topo->resistencia, 1e-12);
    }
    ASSERT_EQ_INT(1, tamanho_pilha(p));
    libera_pilha(p);
}

static void teste_pilha_posse_dos_itens(void)
{
    Pilha *p = cria_pilha();
    Operacao op;
    empilha(p, op_itens(OP_REINICIALIZAR, 3));
    ASSERT_EQ_INT(PILHA_OK, desempilha(p, &op));
    ASSERT_EQ_INT(3, op.n_itens);
    ASSERT_TRUE(op.itens != NULL);
    if (op.itens != NULL) {
        ASSERT_EQ_DBL(300.0, op.itens[2].resistencia, 1e-12);
        ASSERT_EQ_INT(3, op.itens[2].quantidade);
    }
    libera_operacao(&op); /* desempilhou: o chamador libera */
    ASSERT_TRUE(op.itens == NULL);
    ASSERT_EQ_INT(0, op.n_itens);
    libera_pilha(p);
}

static void teste_pilha_esvazia_libera_tudo(void)
{
    Pilha *p = cria_pilha();
    empilha(p, op_itens(OP_NOVO, 2));
    empilha(p, op_mov(OP_RETIRAR, 1, 1));
    empilha(p, op_itens(OP_REINICIALIZAR, 4));
    esvazia_pilha(p);
    ASSERT_EQ_INT(1, pilha_vazia(p));
    ASSERT_EQ_INT(0, tamanho_pilha(p));
    /* continua usável depois de esvaziada */
    ASSERT_EQ_INT(PILHA_OK, empilha(p, op_itens(OP_NOVO, 1)));
    ASSERT_EQ_INT(1, tamanho_pilha(p));
    libera_pilha(p); /* libera os itens que ficaram */
}

static void teste_pilha_percorre_do_topo(void)
{
    Pilha *p = cria_pilha();
    Coleta c;
    c.n = 0;
    empilha(p, op_mov(OP_REPOR, 1, 10));
    empilha(p, op_mov(OP_REPOR, 1, 20));
    empilha(p, op_mov(OP_REPOR, 1, 30));
    percorre_pilha(p, coleta_cb, &c);
    ASSERT_EQ_INT(3, c.n);
    ASSERT_EQ_INT(30, c.qtd[0]);
    ASSERT_EQ_INT(20, c.qtd[1]);
    ASSERT_EQ_INT(10, c.qtd[2]);
    libera_pilha(p);
}

static void teste_pilha_null_seguro(void)
{
    Operacao op = op_mov(OP_REPOR, 1, 1);
    const Operacao *topo;
    ASSERT_EQ_INT(PILHA_ERRO, empilha(NULL, op));
    ASSERT_EQ_INT(PILHA_ERRO, desempilha(NULL, &op));
    ASSERT_EQ_INT(PILHA_ERRO, topo_pilha(NULL, &topo));
    ASSERT_EQ_INT(0, tamanho_pilha(NULL));
    ASSERT_EQ_INT(1, pilha_vazia(NULL));
    esvazia_pilha(NULL);
    libera_pilha(NULL);
    percorre_pilha(NULL, coleta_cb, NULL);
    libera_operacao(NULL);
}

int main(void)
{
    printf("=== test_pilha ===\n");
    RUN_TEST(teste_pilha_nova_vazia);
    RUN_TEST(teste_pilha_lifo);
    RUN_TEST(teste_pilha_topo_nao_remove);
    RUN_TEST(teste_pilha_posse_dos_itens);
    RUN_TEST(teste_pilha_esvazia_libera_tudo);
    RUN_TEST(teste_pilha_percorre_do_topo);
    RUN_TEST(teste_pilha_null_seguro);
    return tf_resumo("pilha");
}
