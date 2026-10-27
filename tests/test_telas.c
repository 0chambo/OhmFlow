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

/* ---- decodifica_tecla: sequências de bytes do terminal ------------------- */

/* Fonte falsa de bytes: devolve a sequência e conta leituras bloqueantes
 * feitas depois da primeira (não podem existir: travariam o menu). */
typedef struct {
    const int *bytes;
    int n, pos;
    int bloqueios_extras;
} Fonte;

static int fonte_proximo(void *ctx, int bloqueia)
{
    Fonte *f = ctx;
    if (bloqueia && f->pos > 0) f->bloqueios_extras++;
    if (f->pos >= f->n) return bloqueia ? EOF : TECLA_TIMEOUT;
    return f->bytes[f->pos++];
}

static int decodifica(const int *bytes, int n, Fonte *f)
{
    f->bytes = bytes;
    f->n = n;
    f->pos = 0;
    f->bloqueios_extras = 0;
    return decodifica_tecla(fonte_proximo, f);
}

static void teste_tecla_comum(void)
{
    static const int w[] = { 'w' }, enter[] = { '\n' }, dig[] = { '7' };
    Fonte f;
    ASSERT_EQ_INT('w', decodifica(w, 1, &f));
    ASSERT_EQ_INT('\n', decodifica(enter, 1, &f));
    ASSERT_EQ_INT('7', decodifica(dig, 1, &f));
}

static void teste_tecla_setas(void)
{
    static const int cima[] = { 27, '[', 'A' }, baixo[] = { 27, '[', 'B' };
    static const int dir[] = { 27, '[', 'C' }, esq[] = { 27, '[', 'D' };
    static const int ss3[] = { 27, 'O', 'A' }; /* modo "application cursor" */
    Fonte f;
    ASSERT_EQ_INT(KEY_UP, decodifica(cima, 3, &f));
    ASSERT_EQ_INT(KEY_DOWN, decodifica(baixo, 3, &f));
    ASSERT_EQ_INT(KEY_RIGHT, decodifica(dir, 3, &f));
    ASSERT_EQ_INT(KEY_LEFT, decodifica(esq, 3, &f));
    ASSERT_EQ_INT(KEY_UP, decodifica(ss3, 3, &f));
    ASSERT_EQ_INT(0, f.bloqueios_extras);
}

static void teste_tecla_fim_da_entrada(void)
{
    static const int ctrl_d[] = { 4 };
    Fonte f;
    ASSERT_EQ_INT(KEY_FIM, decodifica(NULL, 0, &f)); /* EOF / terminal fechado */
    ASSERT_EQ_INT(KEY_FIM, decodifica(ctrl_d, 1, &f));
}

static void teste_tecla_esc_isolado_nao_trava(void)
{
    static const int esc[] = { 27 };
    Fonte f;
    ASSERT_EQ_INT(KEY_ESC, decodifica(esc, 1, &f)); /* ESC sozinho: tecla Esc */
    ASSERT_EQ_INT(0, f.bloqueios_extras); /* não esperou outra tecla */
}

static void teste_menu_esc_sai(void)
{
    /* Esc no menu aciona Sair na hora; demais teclas seguem o fluxo normal */
    ASSERT_EQ_INT(OPCAO_SAIR, menu_tecla_opcao(KEY_ESC, 3));
    ASSERT_EQ_INT(OPCAO_SAIR, menu_tecla_opcao(KEY_ESC, 0));
    ASSERT_EQ_INT(OPCAO_SAIR, menu_tecla_opcao(KEY_FIM, 5));
    ASSERT_EQ_INT(OPCAO_LISTAR, menu_tecla_opcao('3', 7));
    ASSERT_EQ_INT(6, menu_tecla_opcao('\r', 6));
    ASSERT_EQ_INT(6, menu_tecla_opcao('\n', 6));
    ASSERT_EQ_INT(MENU_CONTINUA, menu_tecla_opcao(KEY_UP, 2));
    ASSERT_EQ_INT(MENU_CONTINUA, menu_tecla_opcao(KEY_IGNORADA, 2));
    ASSERT_EQ_INT(MENU_CONTINUA, menu_tecla_opcao('9', 2));
}

static void teste_tecla_esc_sequencia_desconhecida(void)
{
    static const int alt_x[] = { 27, 'x' }, csi_z[] = { 27, '[', 'Z' };
    static const int csi_cortado[] = { 27, '[' };
    Fonte f;
    ASSERT_EQ_INT(KEY_IGNORADA, decodifica(alt_x, 2, &f));
    ASSERT_EQ_INT(KEY_IGNORADA, decodifica(csi_z, 3, &f));
    ASSERT_EQ_INT(KEY_IGNORADA, decodifica(csi_cortado, 2, &f));
    ASSERT_EQ_INT(0, f.bloqueios_extras);
}

static void teste_navegar_ignora_teclas_especiais(void)
{
    ASSERT_EQ_INT(3, navegar_menu(3, KEY_IGNORADA));
    ASSERT_EQ_INT(6, navegar_menu(6, KEY_FIM));
    ASSERT_EQ_INT(4, navegar_menu(4, KEY_ESC));
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

static void teste_menu_so_laranja_e_branco(void)
{
    char buf[16384];
    FILE *f = abre_temp();
    Lista *li;
    if (f == NULL) return;

    li = cria_lista();
    ui_init();
    ui_definir_saida(f);
    ui_definir_cor(1);

    desenha_menu(li, 3);

    conteudo(f, buf, sizeof buf);
    ui_definir_cor(0);
    ui_definir_saida(stdout);
    fclose(f);
    libera_lista(li);

    /* Não selecionadas e contadores em branco (secundária), sem neon */
    ASSERT_TRUE(strstr(buf, "\033[97m") != NULL);
    ASSERT_TRUE(strstr(buf, "\033[38;5;46m") == NULL);  /* verde neon */
    ASSERT_TRUE(strstr(buf, "\033[38;5;141m") == NULL); /* roxo neon  */
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
    RUN_TEST(teste_tecla_comum);
    RUN_TEST(teste_tecla_setas);
    RUN_TEST(teste_tecla_fim_da_entrada);
    RUN_TEST(teste_tecla_esc_isolado_nao_trava);
    RUN_TEST(teste_tecla_esc_sequencia_desconhecida);
    RUN_TEST(teste_menu_esc_sai);
    RUN_TEST(teste_navegar_ignora_teclas_especiais);
    RUN_TEST(teste_menu_cor_laranja_na_borda);
    RUN_TEST(teste_menu_opcao_selecionada_laranja);
    RUN_TEST(teste_menu_so_laranja_e_branco);

    return tf_resumo("telas");
}
