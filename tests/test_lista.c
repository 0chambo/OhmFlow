/* test_lista.c — testes do TAD Lista Dinâmica Ordenada */
#include <limits.h>
#include <stdlib.h>
#include "test_framework.h"
#include "lista.h"

/* ---- auxiliares ---------------------------------------------------- */

#define MAX_COLETA 32

typedef struct {
    int      n;
    Resistor itens[MAX_COLETA];
} Coleta;

static void coleta_cb(const Resistor *r, void *ctx)
{
    Coleta *c = ctx;
    if (c->n < MAX_COLETA) c->itens[c->n++] = *r;
}

static Coleta coleta(Lista *li)
{
    Coleta c;
    c.n = 0;
    percorre_lista(li, coleta_cb, &c);
    return c;
}

static Resistor R(double ohms, int qtd)
{
    Resistor r;
    r.resistencia = ohms;
    r.quantidade = qtd;
    return r;
}

/* lista com 220 Ω(50), 4,7 kΩ(10), 1 MΩ(2) */
static Lista *lista_padrao(void)
{
    Lista *li = cria_lista();
    insere_lista_ordenada(li, R(4700, 10));
    insere_lista_ordenada(li, R(1e6, 2));
    insere_lista_ordenada(li, R(220, 50));
    return li;
}

static int ordenada(Lista *li)
{
    Coleta c = coleta(li);
    int i;
    for (i = 1; i < c.n; i++)
        if (!(c.itens[i - 1].resistencia < c.itens[i].resistencia)) return 0;
    return 1;
}

/* ---- criação ------------------------------------------------------- */

static void teste_lista_nova_vazia(void)
{
    Lista *li = cria_lista();
    ASSERT_TRUE(li != NULL);
    ASSERT_EQ_INT(0, tamanho_lista(li));
    ASSERT_EQ_INT(1, lista_vazia(li));
    ASSERT_EQ_INT(0, total_pecas(li));
    ASSERT_EQ_INT(0, coleta(li).n);
    libera_lista(li);
}

/* ---- inserção ordenada --------------------------------------------- */

static void teste_insere_em_lista_vazia(void)
{
    Lista *li = cria_lista();
    ASSERT_EQ_INT(LISTA_OK, insere_lista_ordenada(li, R(220, 5)));
    ASSERT_EQ_INT(1, tamanho_lista(li));
    ASSERT_EQ_INT(0, lista_vazia(li));
    ASSERT_EQ_INT(5, total_pecas(li));
    libera_lista(li);
}

static void teste_insere_inicio_meio_fim(void)
{
    Lista *li = cria_lista();
    Coleta c;
    ASSERT_EQ_INT(LISTA_OK, insere_lista_ordenada(li, R(1000, 1)));  /* vazia  */
    ASSERT_EQ_INT(LISTA_OK, insere_lista_ordenada(li, R(10, 2)));    /* início */
    ASSERT_EQ_INT(LISTA_OK, insere_lista_ordenada(li, R(1e6, 3)));   /* fim    */
    ASSERT_EQ_INT(LISTA_OK, insere_lista_ordenada(li, R(470, 4)));   /* meio   */
    ASSERT_EQ_INT(LISTA_OK, insere_lista_ordenada(li, R(0.47, 5)));  /* início */
    ASSERT_EQ_INT(5, tamanho_lista(li));
    ASSERT_TRUE(ordenada(li));
    c = coleta(li);
    ASSERT_EQ_DBL(0.47, c.itens[0].resistencia, 1e-12);
    ASSERT_EQ_DBL(10, c.itens[1].resistencia, 1e-12);
    ASSERT_EQ_DBL(470, c.itens[2].resistencia, 1e-12);
    ASSERT_EQ_DBL(1000, c.itens[3].resistencia, 1e-12);
    ASSERT_EQ_DBL(1e6, c.itens[4].resistencia, 1e-12);
    ASSERT_EQ_INT(4, c.itens[2].quantidade);
    ASSERT_EQ_INT(15, total_pecas(li));
    libera_lista(li);
}

static void teste_insere_duplicado(void)
{
    Lista *li = lista_padrao();
    int qtd = 0;
    ASSERT_EQ_INT(LISTA_DUPLICADO, insere_lista_ordenada(li, R(4700, 99)));
    ASSERT_EQ_INT(LISTA_DUPLICADO, insere_lista_ordenada(li, R(4.7e3, 1)));
    ASSERT_EQ_INT(LISTA_DUPLICADO, insere_lista_ordenada(li, R(4.7 * 1000.0, 1)));
    ASSERT_EQ_INT(3, tamanho_lista(li));
    ASSERT_EQ_INT(LISTA_OK, consulta_lista(li, 4700, &qtd));
    ASSERT_EQ_INT(10, qtd);
    libera_lista(li);
}

static void teste_insere_invalido(void)
{
    Lista *li = cria_lista();
    ASSERT_EQ_INT(LISTA_INVALIDO, insere_lista_ordenada(li, R(0, 1)));
    ASSERT_EQ_INT(LISTA_INVALIDO, insere_lista_ordenada(li, R(-10, 1)));
    ASSERT_EQ_INT(LISTA_INVALIDO, insere_lista_ordenada(li, R(220, -1)));
    ASSERT_EQ_INT(LISTA_INVALIDO, insere_lista_ordenada(li, R(RESISTENCIA_MIN / 10, 1)));
    ASSERT_EQ_INT(LISTA_INVALIDO, insere_lista_ordenada(li, R(RESISTENCIA_MAX * 10, 1)));
    ASSERT_EQ_INT(0, tamanho_lista(li));
    ASSERT_EQ_INT(LISTA_OK, insere_lista_ordenada(li, R(220, 0))); /* qtd 0 permitida */
    libera_lista(li);
}

/* ---- remoção ------------------------------------------------------- */

static void teste_remove_primeiro_meio_ultimo(void)
{
    Lista *li = cria_lista();
    Coleta c;
    insere_lista_ordenada(li, R(10, 1));
    insere_lista_ordenada(li, R(100, 1));
    insere_lista_ordenada(li, R(1000, 1));
    insere_lista_ordenada(li, R(10000, 1));
    insere_lista_ordenada(li, R(100000, 1));

    ASSERT_EQ_INT(LISTA_OK, remove_lista(li, 10));      /* primeiro */
    ASSERT_EQ_INT(LISTA_OK, remove_lista(li, 1000));    /* meio     */
    ASSERT_EQ_INT(LISTA_OK, remove_lista(li, 100000));  /* último   */
    ASSERT_EQ_INT(2, tamanho_lista(li));
    c = coleta(li);
    ASSERT_EQ_DBL(100, c.itens[0].resistencia, 1e-12);
    ASSERT_EQ_DBL(10000, c.itens[1].resistencia, 1e-12);
    libera_lista(li);
}

static void teste_remove_unico(void)
{
    Lista *li = cria_lista();
    insere_lista_ordenada(li, R(220, 3));
    ASSERT_EQ_INT(LISTA_OK, remove_lista(li, 220));
    ASSERT_EQ_INT(1, lista_vazia(li));
    ASSERT_EQ_INT(LISTA_OK, insere_lista_ordenada(li, R(330, 1))); /* reutilizável */
    ASSERT_EQ_INT(1, tamanho_lista(li));
    libera_lista(li);
}

static void teste_remove_inexistente_e_vazia(void)
{
    Lista *li = cria_lista();
    ASSERT_EQ_INT(LISTA_NAO_ENCONTRADO, remove_lista(li, 220));
    insere_lista_ordenada(li, R(220, 3));
    ASSERT_EQ_INT(LISTA_NAO_ENCONTRADO, remove_lista(li, 221));
    ASSERT_EQ_INT(LISTA_NAO_ENCONTRADO, remove_lista(li, 1e6));  /* depois do último */
    ASSERT_EQ_INT(LISTA_NAO_ENCONTRADO, remove_lista(li, 1));    /* antes do primeiro */
    ASSERT_EQ_INT(1, tamanho_lista(li));
    libera_lista(li);
}

/* ---- consulta ------------------------------------------------------ */

static void teste_consulta(void)
{
    Lista *li = lista_padrao();
    int qtd = -1;
    ASSERT_EQ_INT(LISTA_OK, consulta_lista(li, 220, &qtd));
    ASSERT_EQ_INT(50, qtd);
    ASSERT_EQ_INT(LISTA_OK, consulta_lista(li, 1e6, &qtd));
    ASSERT_EQ_INT(2, qtd);
    qtd = -1;
    ASSERT_EQ_INT(LISTA_NAO_ENCONTRADO, consulta_lista(li, 330, &qtd));
    ASSERT_EQ_INT(-1, qtd); /* não altera */
    ASSERT_EQ_INT(LISTA_OK, consulta_lista(li, 220, NULL)); /* qtd opcional */
    libera_lista(li);
}

/* ---- retirar ------------------------------------------------------- */

static void teste_retira_parcial_e_total(void)
{
    Lista *li = lista_padrao();
    int qtd = 0;
    ASSERT_EQ_INT(LISTA_OK, retira_estoque(li, 4700, 3));
    consulta_lista(li, 4700, &qtd);
    ASSERT_EQ_INT(7, qtd);
    ASSERT_EQ_INT(LISTA_OK, retira_estoque(li, 4700, 7));
    consulta_lista(li, 4700, &qtd);
    ASSERT_EQ_INT(0, qtd);
    ASSERT_EQ_INT(3, tamanho_lista(li)); /* nó permanece com qtd 0 */
    libera_lista(li);
}

static void teste_retira_alem_do_estoque(void)
{
    Lista *li = lista_padrao();
    int qtd = 0;
    ASSERT_EQ_INT(LISTA_SEM_ESTOQUE, retira_estoque(li, 1e6, 3));
    consulta_lista(li, 1e6, &qtd);
    ASSERT_EQ_INT(2, qtd); /* inalterado */
    libera_lista(li);
}

static void teste_retira_invalido(void)
{
    Lista *li = lista_padrao();
    ASSERT_EQ_INT(LISTA_INVALIDO, retira_estoque(li, 220, 0));
    ASSERT_EQ_INT(LISTA_INVALIDO, retira_estoque(li, 220, -5));
    ASSERT_EQ_INT(LISTA_NAO_ENCONTRADO, retira_estoque(li, 330, 1));
    ASSERT_EQ_INT(62, total_pecas(li)); /* 50 + 10 + 2: nada mudou */
    libera_lista(li);
}

/* ---- repor --------------------------------------------------------- */

static void teste_repor(void)
{
    Lista *li = lista_padrao();
    int qtd = 0;
    ASSERT_EQ_INT(LISTA_OK, repor_estoque(li, 220, 10));
    consulta_lista(li, 220, &qtd);
    ASSERT_EQ_INT(60, qtd);
    ASSERT_EQ_INT(LISTA_NAO_ENCONTRADO, repor_estoque(li, 330, 10));
    ASSERT_EQ_INT(LISTA_INVALIDO, repor_estoque(li, 220, 0));
    ASSERT_EQ_INT(LISTA_INVALIDO, repor_estoque(li, 220, -1));
    libera_lista(li);
}

static void teste_repor_overflow(void)
{
    Lista *li = cria_lista();
    int qtd = 0;
    insere_lista_ordenada(li, R(220, INT_MAX - 5));
    ASSERT_EQ_INT(LISTA_INVALIDO, repor_estoque(li, 220, 6));
    ASSERT_EQ_INT(LISTA_OK, repor_estoque(li, 220, 5));
    consulta_lista(li, 220, &qtd);
    ASSERT_EQ_INT(INT_MAX, qtd);
    libera_lista(li);
}

/* ---- reinicializar ------------------------------------------------- */

static void teste_reinicializa(void)
{
    Lista *li = lista_padrao();
    ASSERT_EQ_INT(LISTA_OK, reinicializa_lista(li));
    ASSERT_EQ_INT(0, tamanho_lista(li));
    ASSERT_EQ_INT(1, lista_vazia(li));
    ASSERT_EQ_INT(0, total_pecas(li));
    ASSERT_EQ_INT(LISTA_OK, reinicializa_lista(li)); /* já vazia */
    ASSERT_EQ_INT(LISTA_OK, insere_lista_ordenada(li, R(100, 1)));
    ASSERT_EQ_INT(1, tamanho_lista(li));
    libera_lista(li);
}

/* ---- robustez: lista NULL ------------------------------------------ */

static void conta_cb(const Resistor *r, void *ctx)
{
    (void)r;
    (*(int *)ctx)++;
}

static void teste_lista_nula(void)
{
    int qtd = 0, chamadas = 0;
    ASSERT_EQ_INT(LISTA_ERRO, insere_lista_ordenada(NULL, R(220, 1)));
    ASSERT_EQ_INT(LISTA_ERRO, remove_lista(NULL, 220));
    ASSERT_EQ_INT(LISTA_ERRO, consulta_lista(NULL, 220, &qtd));
    ASSERT_EQ_INT(LISTA_ERRO, retira_estoque(NULL, 220, 1));
    ASSERT_EQ_INT(LISTA_ERRO, repor_estoque(NULL, 220, 1));
    ASSERT_EQ_INT(LISTA_ERRO, reinicializa_lista(NULL));
    ASSERT_EQ_INT(0, tamanho_lista(NULL));
    ASSERT_EQ_INT(1, lista_vazia(NULL));
    ASSERT_EQ_INT(0, total_pecas(NULL));
    percorre_lista(NULL, conta_cb, &chamadas);
    ASSERT_EQ_INT(0, chamadas);
    libera_lista(NULL); /* não pode quebrar */
}

static void teste_mesma_resistencia(void)
{
    ASSERT_TRUE(mesma_resistencia(4700, 4.7e3));
    ASSERT_TRUE(mesma_resistencia(0.1 + 0.2, 0.3));
    ASSERT_TRUE(!mesma_resistencia(4700, 4701));
    ASSERT_TRUE(!mesma_resistencia(1e-3, 2e-3));
}

int main(void)
{
    printf("test_lista\n");
    RUN_TEST(teste_lista_nova_vazia);
    RUN_TEST(teste_insere_em_lista_vazia);
    RUN_TEST(teste_insere_inicio_meio_fim);
    RUN_TEST(teste_insere_duplicado);
    RUN_TEST(teste_insere_invalido);
    RUN_TEST(teste_remove_primeiro_meio_ultimo);
    RUN_TEST(teste_remove_unico);
    RUN_TEST(teste_remove_inexistente_e_vazia);
    RUN_TEST(teste_consulta);
    RUN_TEST(teste_retira_parcial_e_total);
    RUN_TEST(teste_retira_alem_do_estoque);
    RUN_TEST(teste_retira_invalido);
    RUN_TEST(teste_repor);
    RUN_TEST(teste_repor_overflow);
    RUN_TEST(teste_reinicializa);
    RUN_TEST(teste_lista_nula);
    RUN_TEST(teste_mesma_resistencia);
    return tf_resumo("test_lista");
}
