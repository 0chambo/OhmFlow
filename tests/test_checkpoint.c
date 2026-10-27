/* test_checkpoint.c — testes do checkpoint (desfazer/refazer com duas pilhas) */
#include <stdlib.h>
#include "test_framework.h"
#include "checkpoint.h"
#include "lista.h"

/* ---- auxiliares ---------------------------------------------------- */

static Resistor R(double ohms, int qtd)
{
    Resistor r;
    r.resistencia = ohms;
    r.quantidade = qtd;
    return r;
}

/* Quantidade de `ohms` na lista, ou -1 se não cadastrado */
static int qtd_de(Lista *li, double ohms)
{
    int q;
    return consulta_lista(li, ohms, &q) == LISTA_OK ? q : -1;
}

/* Insere na lista e registra no checkpoint (como a tela Novo faz) */
static void novo(Lista *li, Checkpoint *ck, double ohms, int qtd)
{
    Resistor r = R(ohms, qtd);
    insere_lista_ordenada(li, r);
    ck_registra_novo(ck, &r, 1);
}

typedef struct {
    int desfeitas, refeitas;
} Contagem;

static void conta_cb(int sentido, const Operacao *op, void *ctx)
{
    Contagem *c = ctx;
    (void)op;
    if (sentido == CK_DESFEITA) c->desfeitas++;
    else c->refeitas++;
}

/* ---- estado inicial ------------------------------------------------ */

static void teste_ck_novo_vazio(void)
{
    Checkpoint *ck = cria_checkpoint();
    Lista *li = cria_lista();
    ASSERT_TRUE(ck != NULL);
    ASSERT_EQ_INT(0, ck_posicao(ck));
    ASSERT_EQ_INT(0, ck_total(ck));
    ASSERT_EQ_INT(CK_VAZIO, ck_desfazer(ck, li));
    ASSERT_EQ_INT(CK_VAZIO, ck_refazer(ck, li));
    ASSERT_TRUE(ck_operacao(ck, 1) == NULL);
    libera_checkpoint(ck);
    libera_lista(li);
}

/* ---- inversas ------------------------------------------------------ */

static void teste_ck_desfaz_e_refaz_novo(void)
{
    Checkpoint *ck = cria_checkpoint();
    Lista *li = cria_lista();
    novo(li, ck, 4700, 10);
    ASSERT_EQ_INT(1, ck_posicao(ck));

    ASSERT_EQ_INT(CK_OK, ck_desfazer(ck, li));
    ASSERT_EQ_INT(-1, qtd_de(li, 4700));
    ASSERT_EQ_INT(0, ck_posicao(ck));
    ASSERT_EQ_INT(1, ck_total(ck));

    ASSERT_EQ_INT(CK_OK, ck_refazer(ck, li));
    ASSERT_EQ_INT(10, qtd_de(li, 4700));
    ASSERT_EQ_INT(1, ck_posicao(ck));
    libera_checkpoint(ck);
    libera_lista(li);
}

static void teste_ck_desfaz_lote_em_um_passo(void)
{
    Checkpoint *ck = cria_checkpoint();
    Lista *li = cria_lista();
    Resistor lote[3];
    int i;
    lote[0] = R(10e3, 5);
    lote[1] = R(22e3, 3);
    lote[2] = R(33e3, 7);
    for (i = 0; i < 3; i++) insere_lista_ordenada(li, lote[i]);
    ASSERT_EQ_INT(CK_OK, ck_registra_novo(ck, lote, 3));
    ASSERT_EQ_INT(1, ck_total(ck));

    ASSERT_EQ_INT(CK_OK, ck_desfazer(ck, li));
    ASSERT_EQ_INT(1, lista_vazia(li));
    ASSERT_EQ_INT(CK_OK, ck_refazer(ck, li));
    ASSERT_EQ_INT(3, tamanho_lista(li));
    ASSERT_EQ_INT(15, total_pecas(li));
    libera_checkpoint(ck);
    libera_lista(li);
}

static void teste_ck_desfaz_remover(void)
{
    Checkpoint *ck = cria_checkpoint();
    Lista *li = cria_lista();
    novo(li, ck, 1e6, 2);
    remove_lista(li, 1e6);
    ck_registra_remover(ck, R(1e6, 2));

    ASSERT_EQ_INT(CK_OK, ck_desfazer(ck, li));
    ASSERT_EQ_INT(2, qtd_de(li, 1e6));
    ASSERT_EQ_INT(CK_OK, ck_refazer(ck, li));
    ASSERT_EQ_INT(-1, qtd_de(li, 1e6));
    libera_checkpoint(ck);
    libera_lista(li);
}

static void teste_ck_desfaz_retirar_e_repor(void)
{
    Checkpoint *ck = cria_checkpoint();
    Lista *li = cria_lista();
    novo(li, ck, 220, 50);
    retira_estoque(li, 220, 20);
    ck_registra_movimento(ck, OP_RETIRAR, 220, 20);
    repor_estoque(li, 220, 5);
    ck_registra_movimento(ck, OP_REPOR, 220, 5);
    ASSERT_EQ_INT(35, qtd_de(li, 220));

    ASSERT_EQ_INT(CK_OK, ck_desfazer(ck, li)); /* desfaz repor */
    ASSERT_EQ_INT(30, qtd_de(li, 220));
    ASSERT_EQ_INT(CK_OK, ck_desfazer(ck, li)); /* desfaz retirar */
    ASSERT_EQ_INT(50, qtd_de(li, 220));
    ASSERT_EQ_INT(CK_OK, ck_refazer(ck, li));
    ASSERT_EQ_INT(30, qtd_de(li, 220));
    ASSERT_EQ_INT(CK_OK, ck_refazer(ck, li));
    ASSERT_EQ_INT(35, qtd_de(li, 220));
    libera_checkpoint(ck);
    libera_lista(li);
}

static void teste_ck_desfaz_reinicializar(void)
{
    Checkpoint *ck = cria_checkpoint();
    Lista *li = cria_lista();
    novo(li, ck, 470e-3, 5);
    novo(li, ck, 220, 50);
    novo(li, ck, 4700, 10);
    ASSERT_EQ_INT(CK_OK, ck_registra_reinicializar(ck, li)); /* foto antes */
    reinicializa_lista(li);
    ASSERT_EQ_INT(1, lista_vazia(li));

    ASSERT_EQ_INT(CK_OK, ck_desfazer(ck, li));
    ASSERT_EQ_INT(3, tamanho_lista(li));
    ASSERT_EQ_INT(65, total_pecas(li));
    ASSERT_EQ_INT(5, qtd_de(li, 470e-3));
    ASSERT_EQ_INT(CK_OK, ck_refazer(ck, li));
    ASSERT_EQ_INT(1, lista_vazia(li));
    libera_checkpoint(ck);
    libera_lista(li);
}

static void teste_ck_reinicializar_lista_vazia_nao_registra(void)
{
    Checkpoint *ck = cria_checkpoint();
    Lista *li = cria_lista();
    ASSERT_EQ_INT(CK_OK, ck_registra_reinicializar(ck, li));
    ASSERT_EQ_INT(0, ck_total(ck));
    libera_checkpoint(ck);
    libera_lista(li);
}

/* ---- pilha de refazer ---------------------------------------------- */

static void teste_ck_nova_acao_limpa_refazer(void)
{
    Checkpoint *ck = cria_checkpoint();
    Lista *li = cria_lista();
    novo(li, ck, 100, 1);
    novo(li, ck, 200, 2);
    ck_desfazer(ck, li);
    ASSERT_EQ_INT(2, ck_total(ck));
    novo(li, ck, 300, 3); /* novo ramo: o 200 desfeito some */
    ASSERT_EQ_INT(2, ck_total(ck));
    ASSERT_EQ_INT(2, ck_posicao(ck));
    ASSERT_EQ_INT(CK_VAZIO, ck_refazer(ck, li));
    ASSERT_EQ_INT(-1, qtd_de(li, 200));
    libera_checkpoint(ck);
    libera_lista(li);
}

static void teste_ck_operacao_em_ordem_cronologica(void)
{
    Checkpoint *ck = cria_checkpoint();
    Lista *li = cria_lista();
    const Operacao *op;
    novo(li, ck, 100, 1);
    retira_estoque(li, 100, 1);
    ck_registra_movimento(ck, OP_RETIRAR, 100, 1);
    repor_estoque(li, 100, 4);
    ck_registra_movimento(ck, OP_REPOR, 100, 4);
    ck_desfazer(ck, li); /* REPOR vai para a pilha de refazer */

    op = ck_operacao(ck, 1);
    ASSERT_TRUE(op != NULL && op->tipo == OP_NOVO);
    op = ck_operacao(ck, 2);
    ASSERT_TRUE(op != NULL && op->tipo == OP_RETIRAR);
    op = ck_operacao(ck, 3);
    ASSERT_TRUE(op != NULL && op->tipo == OP_REPOR && op->qtd == 4);
    ASSERT_TRUE(ck_operacao(ck, 0) == NULL);
    ASSERT_TRUE(ck_operacao(ck, 4) == NULL);
    libera_checkpoint(ck);
    libera_lista(li);
}

/* ---- ir para um ponto ---------------------------------------------- */

static void teste_ck_ir_para_tras_e_frente(void)
{
    Checkpoint *ck = cria_checkpoint();
    Lista *li = cria_lista();
    Contagem c = { 0, 0 };
    novo(li, ck, 100, 1);
    novo(li, ck, 200, 2);
    novo(li, ck, 300, 3);
    novo(li, ck, 400, 4);

    ASSERT_EQ_INT(CK_OK, ck_ir_para(ck, li, 1, conta_cb, &c));
    ASSERT_EQ_INT(1, ck_posicao(ck));
    ASSERT_EQ_INT(3, c.desfeitas);
    ASSERT_EQ_INT(1, tamanho_lista(li));

    ASSERT_EQ_INT(CK_OK, ck_ir_para(ck, li, 3, conta_cb, &c));
    ASSERT_EQ_INT(3, ck_posicao(ck));
    ASSERT_EQ_INT(2, c.refeitas);
    ASSERT_EQ_INT(3, tamanho_lista(li));
    ASSERT_EQ_INT(-1, qtd_de(li, 400));

    ASSERT_EQ_INT(CK_OK, ck_ir_para(ck, li, 0, NULL, NULL)); /* início da sessão */
    ASSERT_EQ_INT(1, lista_vazia(li));
    ASSERT_EQ_INT(4, ck_total(ck));
    libera_checkpoint(ck);
    libera_lista(li);
}

static void teste_ck_ir_para_fora_da_faixa(void)
{
    Checkpoint *ck = cria_checkpoint();
    Lista *li = cria_lista();
    novo(li, ck, 100, 1);
    ASSERT_EQ_INT(CK_INVALIDO, ck_ir_para(ck, li, -1, NULL, NULL));
    ASSERT_EQ_INT(CK_INVALIDO, ck_ir_para(ck, li, 2, NULL, NULL));
    ASSERT_EQ_INT(1, ck_posicao(ck));
    ASSERT_EQ_INT(CK_OK, ck_ir_para(ck, li, 1, NULL, NULL)); /* já está lá */
    libera_checkpoint(ck);
    libera_lista(li);
}

static void teste_ck_null_seguro(void)
{
    Resistor r = R(100, 1);
    ASSERT_EQ_INT(CK_ERRO, ck_registra_novo(NULL, &r, 1));
    ASSERT_EQ_INT(CK_ERRO, ck_registra_remover(NULL, r));
    ASSERT_EQ_INT(CK_ERRO, ck_registra_movimento(NULL, OP_REPOR, 100, 1));
    ASSERT_EQ_INT(CK_ERRO, ck_registra_reinicializar(NULL, NULL));
    ASSERT_EQ_INT(CK_ERRO, ck_desfazer(NULL, NULL));
    ASSERT_EQ_INT(CK_ERRO, ck_refazer(NULL, NULL));
    ASSERT_EQ_INT(0, ck_posicao(NULL));
    ASSERT_EQ_INT(0, ck_total(NULL));
    libera_checkpoint(NULL);
}

int main(void)
{
    printf("=== test_checkpoint ===\n");
    RUN_TEST(teste_ck_novo_vazio);
    RUN_TEST(teste_ck_desfaz_e_refaz_novo);
    RUN_TEST(teste_ck_desfaz_lote_em_um_passo);
    RUN_TEST(teste_ck_desfaz_remover);
    RUN_TEST(teste_ck_desfaz_retirar_e_repor);
    RUN_TEST(teste_ck_desfaz_reinicializar);
    RUN_TEST(teste_ck_reinicializar_lista_vazia_nao_registra);
    RUN_TEST(teste_ck_nova_acao_limpa_refazer);
    RUN_TEST(teste_ck_operacao_em_ordem_cronologica);
    RUN_TEST(teste_ck_ir_para_tras_e_frente);
    RUN_TEST(teste_ck_ir_para_fora_da_faixa);
    RUN_TEST(teste_ck_null_seguro);
    return tf_resumo("checkpoint");
}
