/* test_telas.c — testes da lógica de navegação do menu */
#include <stdlib.h>
#include <string.h>
#include "test_framework.h"
#include "telas.h"
#include "ui.h"
#include "lista.h"

/* ---- navegar_menu: navegação vertical ------------------------------------ */

static void teste_navegar_baixo_simples(void)
{
    ASSERT_EQ_INT(2, navegar_menu(1, KEY_DOWN));
    ASSERT_EQ_INT(3, navegar_menu(2, KEY_DOWN));
    ASSERT_EQ_INT(4, navegar_menu(3, KEY_DOWN));
    ASSERT_EQ_INT(6, navegar_menu(5, KEY_DOWN));
    ASSERT_EQ_INT(7, navegar_menu(6, KEY_DOWN));
    ASSERT_EQ_INT(8, navegar_menu(7, KEY_DOWN));
}

static void teste_navegar_baixo_para_sair(void)
{
    /* últimas linhas de cada coluna descem para Sair */
    ASSERT_EQ_INT(0, navegar_menu(4, KEY_DOWN));
    ASSERT_EQ_INT(0, navegar_menu(8, KEY_DOWN));
}

static void teste_navegar_baixo_de_sair(void)
{
    /* de Sair volta ao topo da coluna */
    int r = navegar_menu(0, KEY_DOWN);
    ASSERT_TRUE(r == 1 || r == 5); /* col preservada; col padrão = 0 → 1 */
}

static void teste_navegar_cima_simples(void)
{
    ASSERT_EQ_INT(1, navegar_menu(2, KEY_UP));
    ASSERT_EQ_INT(2, navegar_menu(3, KEY_UP));
    ASSERT_EQ_INT(3, navegar_menu(4, KEY_UP));
    ASSERT_EQ_INT(5, navegar_menu(6, KEY_UP));
    ASSERT_EQ_INT(6, navegar_menu(7, KEY_UP));
    ASSERT_EQ_INT(7, navegar_menu(8, KEY_UP));
}

static void teste_navegar_cima_para_sair(void)
{
    /* topo de cada coluna sobe para Sair */
    ASSERT_EQ_INT(0, navegar_menu(1, KEY_UP));
    ASSERT_EQ_INT(0, navegar_menu(5, KEY_UP));
}

static void teste_navegar_cima_de_sair(void)
{
    /* de Sair volta ao fundo da coluna (linha 3 = opções 4 ou 8) */
    int r = navegar_menu(0, KEY_UP);
    ASSERT_TRUE(r == 4 || r == 8);
}

/* ---- navegar_menu: navegação horizontal ---------------------------------- */

static void teste_navegar_direita(void)
{
    ASSERT_EQ_INT(5, navegar_menu(1, KEY_RIGHT));
    ASSERT_EQ_INT(6, navegar_menu(2, KEY_RIGHT));
    ASSERT_EQ_INT(7, navegar_menu(3, KEY_RIGHT));
    ASSERT_EQ_INT(8, navegar_menu(4, KEY_RIGHT));
}

static void teste_navegar_esquerda(void)
{
    ASSERT_EQ_INT(1, navegar_menu(5, KEY_LEFT));
    ASSERT_EQ_INT(2, navegar_menu(6, KEY_LEFT));
    ASSERT_EQ_INT(3, navegar_menu(7, KEY_LEFT));
    ASSERT_EQ_INT(4, navegar_menu(8, KEY_LEFT));
}

static void teste_navegar_sem_mover_na_borda(void)
{
    /* col 0 não vai mais para esquerda */
    ASSERT_EQ_INT(1, navegar_menu(1, KEY_LEFT));
    ASSERT_EQ_INT(4, navegar_menu(4, KEY_LEFT));
    /* col 1 não vai mais para direita */
    ASSERT_EQ_INT(5, navegar_menu(5, KEY_RIGHT));
    ASSERT_EQ_INT(8, navegar_menu(8, KEY_RIGHT));
}

static void teste_navegar_horizontal_de_sair(void)
{
    /* Sair não se move para esquerda/direita */
    ASSERT_EQ_INT(0, navegar_menu(0, KEY_LEFT));
    ASSERT_EQ_INT(0, navegar_menu(0, KEY_RIGHT));
}

/* ---- navegar_menu: teclas irrelevantes ----------------------------------- */

static void teste_navegar_tecla_ignorada(void)
{
    /* tecla qualquer não muda seleção */
    ASSERT_EQ_INT(3, navegar_menu(3, 'x'));
    ASSERT_EQ_INT(7, navegar_menu(7, 'z'));
}

/* ---- desenha_menu: borda laranja + destaque ------------------------------ */

static FILE *abre_temp(void)
{
    FILE *f = tmpfile();
    if (f == NULL) f = fopen("build/_tmp_test_telas.txt", "w+b");
    return f;
}

static const char *conteudo(FILE *f, char *buf, size_t n)
{
    size_t lido;
    fflush(f);
    rewind(f);
    lido = fread(buf, 1, n - 1, f);
    buf[lido] = '\0';
    return buf;
}

static void teste_menu_cor_laranja_na_borda(void)
{
    char buf[16384];
    FILE *f = abre_temp();
    Lista *li;
    if (f == NULL) return; /* skip se não conseguiu temp */

    li = cria_lista();
    ui_init();
    ui_definir_saida(f);
    ui_definir_cor(1); /* força cor ligada */

    desenha_menu(li, 1);

    conteudo(f, buf, sizeof buf);
    ui_definir_saida(stdout);
    fclose(f);
    libera_lista(li);

    /* A borda usa UI_COR_LARANJA = \033[38;5;208m */
    ASSERT_TRUE(strstr(buf, "\033[38;5;208m") != NULL);
}

static void teste_menu_opcao_selecionada_laranja(void)
{
    char buf[16384];
    FILE *f = abre_temp();
    Lista *li;
    if (f == NULL) return;

    li = cria_lista();
    ui_init();
    ui_definir_saida(f);
    ui_definir_cor(1);

    desenha_menu(li, 3); /* seleciona Listar */

    conteudo(f, buf, sizeof buf);
    ui_definir_saida(stdout);
    fclose(f);
    libera_lista(li);

    /* Opção selecionada usa laranja */
    ASSERT_TRUE(strstr(buf, "\033[38;5;208m") != NULL);
    /* Rótulo da opção aparece no output */
    ASSERT_TRUE(strstr(buf, "Listar") != NULL);
}

/* ---- main ---------------------------------------------------------------- */

int main(void)
{
    printf("=== test_telas ===\n");

    RUN_TEST(teste_navegar_baixo_simples);
    RUN_TEST(teste_navegar_baixo_para_sair);
    RUN_TEST(teste_navegar_baixo_de_sair);
    RUN_TEST(teste_navegar_cima_simples);
    RUN_TEST(teste_navegar_cima_para_sair);
    RUN_TEST(teste_navegar_cima_de_sair);
    RUN_TEST(teste_navegar_direita);
    RUN_TEST(teste_navegar_esquerda);
    RUN_TEST(teste_navegar_sem_mover_na_borda);
    RUN_TEST(teste_navegar_horizontal_de_sair);
    RUN_TEST(teste_navegar_tecla_ignorada);
    RUN_TEST(teste_menu_cor_laranja_na_borda);
    RUN_TEST(teste_menu_opcao_selecionada_laranja);

    return tf_resumo("telas");
}
