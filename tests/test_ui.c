/* test_ui.c — testes das primitivas de interface (largura, alinhamento, cores, molduras) */
#include <stdlib.h>
#include "test_framework.h"
#include "ui.h"

#define TAM_SAIDA 8192

/* Arquivo temporário para capturar a saída da ui */
static FILE *abre_temp(void)
{
    FILE *f = tmpfile();
    if (f == NULL) f = fopen("build/_tmp_test_ui.txt", "w+b");
    return f;
}

/* Lê todo o conteúdo de f (a partir do início) para buf */
static const char *conteudo(FILE *f, char *buf, size_t n)
{
    size_t lido;
    fflush(f);
    rewind(f);
    lido = fread(buf, 1, n - 1, f);
    buf[lido] = '\0';
    return buf;
}

/* Todas as linhas não vazias de s têm a mesma largura visível? */
static int linhas_mesma_largura(char *s, size_t *largura)
{
    char *linha = strtok(s, "\r\n");
    size_t w0 = 0;
    int primeira = 1;
    while (linha != NULL) {
        size_t w = ui_largura(linha);
        if (primeira) { w0 = w; primeira = 0; }
        else if (w != w0) return 0;
        linha = strtok(NULL, "\r\n");
    }
    if (largura) *largura = w0;
    return !primeira;
}

/* ---- largura visível ------------------------------------------------ */

static void teste_largura_ascii(void)
{
    ASSERT_EQ_INT(0, ui_largura(""));
    ASSERT_EQ_INT(3, ui_largura("abc"));
    ASSERT_EQ_INT(0, ui_largura(NULL));
}

static void teste_largura_utf8(void)
{
    ASSERT_EQ_INT(5, ui_largura("220 \xCE\xA9"));        /* 220 Ω   */
    ASSERT_EQ_INT(6, ui_largura("4,7 k\xCE\xA9"));       /* 4,7 kΩ  */
    ASSERT_EQ_INT(11, ui_largura("Resist\xC3\xAAncia")); /* ê       */
    ASSERT_EQ_INT(3, ui_largura("\xE2\x95\x94\xE2\x95\x90\xE2\x95\x97")); /* ╔═╗ */
}

static void teste_largura_ignora_ansi(void)
{
    ASSERT_EQ_INT(2, ui_largura("\033[32mOK\033[0m"));
    ASSERT_EQ_INT(2, ui_largura("\033[1;33mOK\033[0m"));
    ASSERT_EQ_INT(0, ui_largura("\033[0m"));
}

/* ---- padding -------------------------------------------------------- */

static void teste_pad(void)
{
    char buf[TAM_SAIDA];
    FILE *f = abre_temp();
    ASSERT_TRUE(f != NULL);
    if (f == NULL) return;
    ui_pad(f, "ab", 5, 0);
    fputc('|', f);
    ui_pad(f, "ab", 5, 1);
    fputc('|', f);
    ui_pad(f, "220 \xCE\xA9", 7, 1);
    fputc('|', f);
    ui_pad(f, "\033[31mX\033[0m", 3, 0);
    fputc('|', f);
    ui_pad(f, "longo demais", 3, 0); /* maior que largura: imprime inteiro */
    ASSERT_STR_EQ("ab   |   ab|  220 \xCE\xA9|\033[31mX\033[0m  |longo demais",
                  conteudo(f, buf, sizeof buf));
    fclose(f);
}

/* ---- cores ---------------------------------------------------------- */

static void teste_sem_cor_nao_emite_ansi(void)
{
    char buf[TAM_SAIDA];
    FILE *f = abre_temp();
    ASSERT_TRUE(f != NULL);
    if (f == NULL) return;
    ui_definir_saida(f);
    ui_definir_cor(0);
    ASSERT_EQ_INT(0, ui_cor_ativa());
    ASSERT_STR_EQ("", ui_cor(UI_COR_VERDE));
    ui_caixa_feedback(UI_SUCESSO, "Tudo certo", "linha %d", 2);
    ui_mensagem(UI_ERRO, "falhou %s", "aqui");
    ASSERT_TRUE(strchr(conteudo(f, buf, sizeof buf), '\033') == NULL);
    ASSERT_TRUE(strstr(buf, "Tudo certo") != NULL);
    ASSERT_TRUE(strstr(buf, "linha 2") != NULL);
    ASSERT_TRUE(strstr(buf, "falhou aqui") != NULL);
    ui_definir_saida(stdout);
    fclose(f);
}

static void teste_com_cor_emite_ansi(void)
{
    char buf[TAM_SAIDA];
    FILE *f = abre_temp();
    ASSERT_TRUE(f != NULL);
    if (f == NULL) return;
    ui_definir_saida(f);
    ui_definir_cor(1);
    ASSERT_EQ_INT(1, ui_cor_ativa());
    ui_caixa_feedback(UI_ERRO, "Falha", NULL);
    ASSERT_TRUE(strstr(conteudo(f, buf, sizeof buf), "\033[") != NULL);
    ASSERT_TRUE(strstr(buf, "Falha") != NULL);
    ui_definir_cor(0);
    ui_definir_saida(stdout);
    fclose(f);
}

/* ---- molduras alinhadas --------------------------------------------- */

static void teste_caixa_alinhada(void)
{
    char buf[TAM_SAIDA];
    size_t w = 0;
    const char *linhas[] = {
        "curta",
        "com acento: Resist\xC3\xAAncia 4,7 k\xCE\xA9",
        "\033[33mcolorida\033[0m"
    };
    FILE *f = abre_temp();
    ASSERT_TRUE(f != NULL);
    if (f == NULL) return;
    ui_definir_saida(f);
    ui_definir_cor(1);
    ui_caixa(UI_MOLDURA_DUPLA, UI_COR_BORDA, "Titulo", "Cabecalho", linhas, 3, UI_LARGURA);
    conteudo(f, buf, sizeof buf);
    ASSERT_TRUE(linhas_mesma_largura(buf, &w));
    ASSERT_EQ_INT(UI_LARGURA + 2, w); /* bordas laterais + largura interna */
    ui_definir_cor(0);
    ui_definir_saida(stdout);
    fclose(f);
}

static void teste_caixa_cresce_com_conteudo(void)
{
    char buf[TAM_SAIDA];
    size_t w = 0;
    const char *linhas[] = { "uma linha bem comprida que passa da largura padrao da caixa" };
    FILE *f = abre_temp();
    ASSERT_TRUE(f != NULL);
    if (f == NULL) return;
    ui_definir_saida(f);
    ui_caixa(UI_MOLDURA_SIMPLES, UI_COR_BORDA, NULL, NULL, linhas, 1, 10);
    conteudo(f, buf, sizeof buf);
    ASSERT_TRUE(linhas_mesma_largura(buf, &w));
    ASSERT_EQ_INT(ui_largura(linhas[0]) + 4, w); /* 2 bordas + 2 margens */
    ui_definir_saida(stdout);
    fclose(f);
}

static void teste_feedback_alinhado(void)
{
    char buf[TAM_SAIDA];
    FILE *f = abre_temp();
    ASSERT_TRUE(f != NULL);
    if (f == NULL) return;
    ui_definir_saida(f);
    ui_definir_cor(1);
    ui_caixa_feedback(UI_AVISO, "Retirada concluída", "4,7 k\xCE\xA9: 10 %s 7 unidades\nsegunda linha", UI_SETA);
    conteudo(f, buf, sizeof buf);
    ASSERT_TRUE(strstr(buf, "segunda linha") != NULL);
    ASSERT_TRUE(linhas_mesma_largura(buf, NULL));
    ui_definir_cor(0);
    ui_definir_saida(stdout);
    fclose(f);
}

static void teste_tabela_alinhada(void)
{
    char buf[TAM_SAIDA];
    const int larg[] = { 2, 12, 10 };
    const int dir[]  = { 1, 0, 1 };
    const char *cab[]  = { "#", "Resist\xC3\xAAncia", "Quantidade" };
    const char *l1[]   = { "1", "\033[35m 470 m\xCE\xA9\033[0m", "5" };
    const char *l2[]   = { "10", "  4,7 k\xCE\xA9", "\033[31m0\033[0m" };
    FILE *f = abre_temp();
    ASSERT_TRUE(f != NULL);
    if (f == NULL) return;
    ui_definir_saida(f);
    ui_definir_cor(1);
    ui_tabela_borda(UI_TAB_TOPO, larg, 3);
    ui_tabela_linha(cab, larg, dir, 3);
    ui_tabela_borda(UI_TAB_MEIO, larg, 3);
    ui_tabela_linha(l1, larg, dir, 3);
    ui_tabela_linha(l2, larg, dir, 3);
    ui_tabela_borda(UI_TAB_BASE, larg, 3);
    conteudo(f, buf, sizeof buf);
    ASSERT_TRUE(linhas_mesma_largura(buf, NULL));
    ui_definir_cor(0);
    ui_definir_saida(stdout);
    fclose(f);
}

static void teste_moldura_arredondada_alinhada(void)
{
    char buf[TAM_SAIDA];
    const char *linhas[] = { "Linha 1", "Linha 2 teste" };
    FILE *f = abre_temp();
    ASSERT_TRUE(f != NULL);
    if (f == NULL) return;
    ui_definir_saida(f);
    ui_caixa(UI_MOLDURA_ARREDONDADA, UI_COR_BORDA, "Retro Box", "Header", linhas, 2, 30);
    conteudo(f, buf, sizeof buf);
    ASSERT_TRUE(strstr(buf, "Retro Box") != NULL);
    ASSERT_TRUE(strstr(buf, "Linha 2 teste") != NULL);
    ASSERT_TRUE(linhas_mesma_largura(buf, NULL));
    ui_definir_saida(stdout);
    fclose(f);
}

static void teste_cores_retro_novas(void)
{
    ui_definir_cor(1);
    ASSERT_TRUE(strlen(ui_cor(UI_COR_LARANJA)) > 0);
    ASSERT_TRUE(strlen(ui_cor(UI_COR_VERDE_NEON)) > 0);
    ASSERT_TRUE(strlen(ui_cor(UI_COR_ROXO_NEON)) > 0);
    ui_definir_cor(0);
    ASSERT_STR_EQ("", ui_cor(UI_COR_LARANJA));
    ASSERT_STR_EQ("", ui_cor(UI_COR_VERDE_NEON));
    ASSERT_STR_EQ("", ui_cor(UI_COR_ROXO_NEON));
}

static void teste_largura_terminal(void)
{
    int cols = ui_colunas_terminal();
    /* Deve devolver ao menos a largura padrão se não for terminal ou em ambiente de teste */
    ASSERT_TRUE(cols >= 40);
}

static void teste_caixa_centralizada_alinhada(void)
{
    char buf[TAM_SAIDA];
    const char *linhas[] = { "Opcao 1", "Opcao 2" };
    FILE *f = abre_temp();
    ASSERT_TRUE(f != NULL);
    if (f == NULL) return;
    ui_definir_saida(f);
    ui_caixa_centralizada(UI_MOLDURA_ARREDONDADA, UI_COR_BORDA, "Menu", "Header", linhas, 2, 40);
    conteudo(f, buf, sizeof buf);
    ASSERT_TRUE(strstr(buf, "Menu") != NULL);
    ASSERT_TRUE(linhas_mesma_largura(buf, NULL));
    ui_definir_saida(stdout);
    fclose(f);
}

/* ---- espera por Enter (issue #29) ------------------------------------ */

/* Fonte falsa de teclas: devolve a sequência e depois EOF */
typedef struct {
    const int *teclas;
    int n, pos;
} Teclas;

static int teclas_proxima(void *ctx)
{
    Teclas *t = ctx;
    return t->pos < t->n ? t->teclas[t->pos++] : EOF;
}

static void teste_espera_enter_descarta_texto(void)
{
    static const int seq[] = { 'a', 'b', 'c', '\r', 'x' };
    Teclas t = { seq, 5, 0 };
    ASSERT_EQ_INT(1, ui_espera_enter_de(teclas_proxima, &t));
    ASSERT_EQ_INT(4, t.pos); /* parou no Enter: 'x' ficou para depois */
}

static void teste_espera_enter_aceita_lf(void)
{
    static const int seq[] = { 'z', '\n' };
    Teclas t = { seq, 2, 0 };
    ASSERT_EQ_INT(1, ui_espera_enter_de(teclas_proxima, &t));
    ASSERT_EQ_INT(2, t.pos);
}

static void teste_espera_enter_tecla_estendida(void)
{
    /* 0/224 + código: o segundo byte (13 aqui) não pode contar como Enter */
    static const int seq[] = { 224, 13, 0, 13, 'q', 13 };
    Teclas t = { seq, 6, 0 };
    ASSERT_EQ_INT(1, ui_espera_enter_de(teclas_proxima, &t));
    ASSERT_EQ_INT(6, t.pos);
}

static void teste_espera_enter_eof(void)
{
    static const int seq[] = { 'a', 'b' };
    Teclas t = { seq, 2, 0 };
    ASSERT_EQ_INT(0, ui_espera_enter_de(teclas_proxima, &t)); /* sem Enter: não trava */
    ASSERT_EQ_INT(2, t.pos);
}

/* ---- telas centralizadas ---------------------------------------------- */

/* Espaços no início de s */
static int espacos_iniciais(const char *s)
{
    int n = 0;
    while (s[n] == ' ') n++;
    return n;
}

/* Margem esperada para um bloco de `largura` colunas */
static int margem_esperada(int largura)
{
    int m = (ui_colunas_terminal() - largura) / 2;
    return m > 0 ? m : 0;
}

static void teste_centralizado_desligado_por_padrao(void)
{
    ASSERT_EQ_INT(0, ui_centralizado());
    ASSERT_EQ_INT(0, ui_margem());
}

static void teste_margem_do_bloco(void)
{
    ui_definir_centralizado(1);
    ASSERT_EQ_INT(1, ui_centralizado());
    ASSERT_EQ_INT(margem_esperada(UI_LARGURA + 2), ui_margem());
    ui_definir_centralizado(0);
    ASSERT_EQ_INT(0, ui_margem());
}

static void teste_caixa_centralizada_no_modo(void)
{
    char buf[TAM_SAIDA];
    const char *linhas[] = { "Linha" };
    FILE *f = abre_temp();
    ASSERT_TRUE(f != NULL);
    if (f == NULL) return;
    ui_definir_saida(f);
    ui_definir_centralizado(1);
    ui_caixa(UI_MOLDURA_SIMPLES, UI_COR_BORDA, "Titulo", NULL, linhas, 1, UI_LARGURA);
    ui_definir_centralizado(0);
    conteudo(f, buf, sizeof buf);
    ASSERT_EQ_INT(margem_esperada(UI_LARGURA + 2), espacos_iniciais(buf));
    ASSERT_TRUE(linhas_mesma_largura(buf, NULL));
    ui_definir_saida(stdout);
    fclose(f);
}

static void teste_caixa_larga_centralizada_pela_propria_largura(void)
{
    char buf[TAM_SAIDA], linha[80];
    const char *linhas[1];
    FILE *f = abre_temp();
    ASSERT_TRUE(f != NULL);
    if (f == NULL) return;
    memset(linha, 'x', 60);
    linha[60] = '\0';
    linhas[0] = linha;
    ui_definir_saida(f);
    ui_definir_centralizado(1);
    ui_caixa(UI_MOLDURA_SIMPLES, UI_COR_BORDA, NULL, NULL, linhas, 1, UI_LARGURA);
    ui_definir_centralizado(0);
    conteudo(f, buf, sizeof buf);
    ASSERT_EQ_INT(margem_esperada(60 + 2 + 2), espacos_iniciais(buf));
    ui_definir_saida(stdout);
    fclose(f);
}

static void teste_texto_alinhado_ao_bloco(void)
{
    char buf[TAM_SAIDA];
    FILE *f = abre_temp();
    ASSERT_TRUE(f != NULL);
    if (f == NULL) return;
    ui_definir_saida(f);
    ui_definir_centralizado(1);
    ui_dica("dica");
    ui_definir_centralizado(0);
    conteudo(f, buf, sizeof buf);
    ASSERT_EQ_INT(margem_esperada(UI_LARGURA + 2) + 2, espacos_iniciais(buf));
    ASSERT_TRUE(strstr(buf, "dica") != NULL);
    ui_definir_saida(stdout);
    fclose(f);
}

static void teste_dica_larga_centralizada_pela_propria_largura(void)
{
    char buf[TAM_SAIDA], texto[80];
    FILE *f = abre_temp();
    ASSERT_TRUE(f != NULL);
    if (f == NULL) return;
    memset(texto, 'y', 60);
    texto[60] = '\0';
    ui_definir_saida(f);
    ui_definir_centralizado(1);
    ui_dica("%s", texto);
    ui_definir_centralizado(0);
    conteudo(f, buf, sizeof buf);
    ASSERT_EQ_INT(margem_esperada(60), espacos_iniciais(buf));
    ui_definir_saida(stdout);
    fclose(f);
}

static void teste_mensagem_alinhada_ao_bloco(void)
{
    char buf[TAM_SAIDA];
    FILE *f = abre_temp();
    ASSERT_TRUE(f != NULL);
    if (f == NULL) return;
    ui_definir_saida(f);
    ui_definir_centralizado(1);
    ui_mensagem(UI_ERRO, "falhou");
    ui_definir_centralizado(0);
    conteudo(f, buf, sizeof buf);
    ASSERT_EQ_INT(margem_esperada(UI_LARGURA + 2) + 2, espacos_iniciais(buf));
    ui_definir_saida(stdout);
    fclose(f);
}

static void teste_tabela_centralizada(void)
{
    static const int larg[] = { 2, 12, 10 };
    const char *celulas[] = { "1", "220", "5" };
    char buf[TAM_SAIDA];
    /* largura total: borda esquerda + (largura + 2 de respiro + borda) por coluna */
    int total = 1 + (2 + 3) + (12 + 3) + (10 + 3);
    FILE *f = abre_temp();
    ASSERT_TRUE(f != NULL);
    if (f == NULL) return;
    ui_definir_saida(f);
    ui_definir_centralizado(1);
    ui_tabela_borda(UI_TAB_TOPO, larg, 3);
    ui_tabela_linha(celulas, larg, NULL, 3);
    ui_tabela_borda(UI_TAB_BASE, larg, 3);
    ui_definir_centralizado(0);
    conteudo(f, buf, sizeof buf);
    ASSERT_EQ_INT(margem_esperada(total), espacos_iniciais(buf));
    ASSERT_TRUE(linhas_mesma_largura(buf, NULL));
    ui_definir_saida(stdout);
    fclose(f);
}

static void teste_tabela_sem_centralizar_mantem_recuo(void)
{
    static const int larg[] = { 2, 12, 10 };
    char buf[TAM_SAIDA];
    FILE *f = abre_temp();
    ASSERT_TRUE(f != NULL);
    if (f == NULL) return;
    ui_definir_saida(f);
    ui_tabela_borda(UI_TAB_TOPO, larg, 3);
    conteudo(f, buf, sizeof buf);
    ASSERT_EQ_INT(2, espacos_iniciais(buf));
    ui_definir_saida(stdout);
    fclose(f);
}

static void teste_espaco_vertical(void)
{
    char buf[TAM_SAIDA];
    int esperado = (ui_linhas_terminal() - 6) / 2, n = 0;
    const char *p;
    FILE *f = abre_temp();
    ASSERT_TRUE(f != NULL);
    if (f == NULL) return;
    ui_definir_saida(f);
    ui_espaco_vertical(6);              /* desligado: nada */
    ui_definir_centralizado(1);
    ui_espaco_vertical(6);              /* ligado: metade da sobra */
    ui_espaco_vertical(100000);         /* maior que a tela: nada */
    ui_definir_centralizado(0);
    conteudo(f, buf, sizeof buf);
    for (p = buf; *p; p++) n += *p == '\n';
    ASSERT_EQ_INT(esperado > 0 ? esperado : 0, n);
    ui_definir_saida(stdout);
    fclose(f);
}

int main(void)
{
    ui_init();
    printf("test_ui\n");
    RUN_TEST(teste_largura_ascii);
    RUN_TEST(teste_largura_utf8);
    RUN_TEST(teste_largura_ignora_ansi);
    RUN_TEST(teste_pad);
    RUN_TEST(teste_sem_cor_nao_emite_ansi);
    RUN_TEST(teste_com_cor_emite_ansi);
    RUN_TEST(teste_caixa_alinhada);
    RUN_TEST(teste_caixa_cresce_com_conteudo);
    RUN_TEST(teste_feedback_alinhado);
    RUN_TEST(teste_tabela_alinhada);
    RUN_TEST(teste_moldura_arredondada_alinhada);
    RUN_TEST(teste_cores_retro_novas);
    RUN_TEST(teste_largura_terminal);
    RUN_TEST(teste_caixa_centralizada_alinhada);
    RUN_TEST(teste_espera_enter_descarta_texto);
    RUN_TEST(teste_espera_enter_aceita_lf);
    RUN_TEST(teste_espera_enter_tecla_estendida);
    RUN_TEST(teste_espera_enter_eof);
    RUN_TEST(teste_centralizado_desligado_por_padrao);
    RUN_TEST(teste_margem_do_bloco);
    RUN_TEST(teste_caixa_centralizada_no_modo);
    RUN_TEST(teste_caixa_larga_centralizada_pela_propria_largura);
    RUN_TEST(teste_texto_alinhado_ao_bloco);
    RUN_TEST(teste_dica_larga_centralizada_pela_propria_largura);
    RUN_TEST(teste_mensagem_alinhada_ao_bloco);
    RUN_TEST(teste_tabela_centralizada);
    RUN_TEST(teste_tabela_sem_centralizar_mantem_recuo);
    RUN_TEST(teste_espaco_vertical);
    return tf_resumo("test_ui");
}
