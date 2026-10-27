/*
 * test_resistor.c — código de cores de 4 faixas (codigo_cores) e desenho do
 * resistor na tela Novo (resistor_arte, ui_linha_respondida).
 */
#include <string.h>
#include "test_framework.h"
#include "formatacao.h"
#include "resistor.h"
#include "telas.h"
#include "ui.h"

/* Confere as 4 faixas e o retorno de codigo_cores para `ohms` */
static void confere(double ohms, int ret, int f1, int f2, int f3)
{
    int f[RESISTOR_FAIXAS];
    ASSERT_EQ_INT(ret, codigo_cores(ohms, f));
    ASSERT_EQ_INT(f1, f[0]);
    ASSERT_EQ_INT(f2, f[1]);
    ASSERT_EQ_INT(f3, f[2]);
    ASSERT_EQ_INT(COR_DOURADO, f[3]); /* tolerância sempre dourada (5%) */
}

/* ---- codigo_cores ----------------------------------------------------- */

static void teste_cores_valores_comuns(void)
{
    confere(4700, CORES_EXATO, COR_AMARELO, COR_VIOLETA, COR_VERMELHO);
    confere(220, CORES_EXATO, COR_VERMELHO, COR_VERMELHO, COR_MARROM);
    confere(10e3, CORES_EXATO, COR_MARROM, COR_PRETO, COR_LARANJA);
    confere(1e6, CORES_EXATO, COR_MARROM, COR_PRETO, COR_VERDE);
    confere(10, CORES_EXATO, COR_MARROM, COR_PRETO, COR_PRETO);
    confere(1000, CORES_EXATO, COR_MARROM, COR_PRETO, COR_VERMELHO);
    confere(68e6, CORES_EXATO, COR_AZUL, COR_CINZA, COR_AZUL);
    confere(99e7, CORES_EXATO, COR_BRANCO, COR_BRANCO, COR_VIOLETA);
}

static void teste_cores_multiplicador_fracionario(void)
{
    confere(4.7, CORES_EXATO, COR_AMARELO, COR_VIOLETA, COR_DOURADO);  /* x0,1  */
    confere(0.47, CORES_EXATO, COR_AMARELO, COR_VIOLETA, COR_PRATA);   /* x0,01 */
    confere(1, CORES_EXATO, COR_MARROM, COR_PRETO, COR_DOURADO);
    confere(0.1, CORES_EXATO, COR_MARROM, COR_PRETO, COR_PRATA);
}

static void teste_cores_valores_da_entrada(void)
{
    double v;
    ASSERT_TRUE(parse_resistencia("470m", &v));
    confere(v, CORES_EXATO, COR_AMARELO, COR_VIOLETA, COR_PRATA);
    ASSERT_TRUE(parse_resistencia("1M5", &v));
    confere(v, CORES_EXATO, COR_MARROM, COR_VERDE, COR_VERDE);
    ASSERT_TRUE(parse_resistencia("1000M", &v));
    confere(v, CORES_EXATO, COR_MARROM, COR_PRETO, COR_CINZA);
    ASSERT_TRUE(parse_resistencia("4R7", &v));
    confere(v, CORES_EXATO, COR_AMARELO, COR_VIOLETA, COR_DOURADO);
}

static void teste_cores_aproximado(void)
{
    /* 3 algarismos significativos: arredonda para 2 */
    confere(4750, CORES_APROXIMADO, COR_AMARELO, COR_CINZA, COR_VERMELHO);  /* 4,8k */
    confere(1234, CORES_APROXIMADO, COR_MARROM, COR_VERMELHO, COR_VERMELHO); /* 1,2k */
    confere(996, CORES_APROXIMADO, COR_MARROM, COR_PRETO, COR_VERMELHO);    /* 1,0k */
    confere(5, CORES_EXATO, COR_VERDE, COR_PRETO, COR_DOURADO);
}

static void teste_cores_fora_da_faixa(void)
{
    int f[RESISTOR_FAIXAS];
    /* abaixo de 0,1 Ω não cabe em 4 faixas (multiplicador mínimo x0,01) */
    ASSERT_EQ_INT(CORES_FORA, codigo_cores(0.047, f));
    ASSERT_EQ_INT(SEM_FAIXA, f[0]);
    ASSERT_EQ_INT(SEM_FAIXA, f[1]);
    ASSERT_EQ_INT(SEM_FAIXA, f[2]);
    ASSERT_EQ_INT(COR_DOURADO, f[3]);
    ASSERT_EQ_INT(CORES_FORA, codigo_cores(1e-3, f));
    ASSERT_EQ_INT(CORES_FORA, codigo_cores(0, f));
    ASSERT_EQ_INT(CORES_FORA, codigo_cores(-5, f));
}

static void teste_nome_cor(void)
{
    ASSERT_STR_EQ("preto", nome_cor_faixa(COR_PRETO));
    ASSERT_STR_EQ("violeta", nome_cor_faixa(COR_VIOLETA));
    ASSERT_STR_EQ("dourado", nome_cor_faixa(COR_DOURADO));
    ASSERT_STR_EQ("prata", nome_cor_faixa(COR_PRATA));
    ASSERT_STR_EQ("-", nome_cor_faixa(SEM_FAIXA));
    ASSERT_STR_EQ("-", nome_cor_faixa(99));
}

/* ---- resistor_arte ------------------------------------------------------ */

static char arte[RESISTOR_LINHAS][RESISTOR_TAM_LINHA];

static void teste_arte_dimensoes(void)
{
    int i, faixas[RESISTOR_FAIXAS] = { COR_AMARELO, COR_VIOLETA, COR_VERMELHO, COR_DOURADO };
    resistor_arte(NULL, arte);
    for (i = 0; i < RESISTOR_LINHAS; i++)
        ASSERT_EQ_INT(RESISTOR_COLUNAS + RESISTOR_RECUO, (long long)ui_largura(arte[i]));
    resistor_arte(faixas, arte);
    for (i = 0; i < RESISTOR_LINHAS; i++) {
        size_t n = strlen(arte[i]);
        ASSERT_EQ_INT(RESISTOR_COLUNAS + RESISTOR_RECUO, (long long)ui_largura(arte[i]));
        /* toda linha termina com reset, para não vazar cor na moldura */
        ASSERT_TRUE(n >= 4 && strcmp(arte[i] + n - 4, "\033[0m") == 0);
    }
}

/* Cor truecolor "R;G;Bm" de uma faixa aparece na arte? */
static int tem_cor(int cor)
{
    char rgb[24];
    int i;
    resistor_rgb_faixa(cor, rgb, sizeof rgb);
    for (i = 0; i < RESISTOR_LINHAS; i++)
        if (strstr(arte[i], rgb) != NULL) return 1;
    return 0;
}

static void teste_arte_sem_cor_so_contorno(void)
{
    resistor_arte(NULL, arte);
    ASSERT_TRUE(!tem_cor(COR_AMARELO));
    ASSERT_TRUE(!tem_cor(COR_DOURADO));
    ASSERT_TRUE(!tem_cor(RESISTOR_CORPO));
    ASSERT_TRUE(tem_cor(RESISTOR_CONTORNO));
}

static void teste_arte_colorida(void)
{
    int faixas[RESISTOR_FAIXAS] = { COR_AMARELO, COR_VIOLETA, COR_VERMELHO, COR_DOURADO };
    resistor_arte(faixas, arte);
    ASSERT_TRUE(tem_cor(COR_AMARELO));
    ASSERT_TRUE(tem_cor(COR_VIOLETA));
    ASSERT_TRUE(tem_cor(COR_VERMELHO));
    ASSERT_TRUE(tem_cor(COR_DOURADO));
    ASSERT_TRUE(tem_cor(RESISTOR_CORPO));
    ASSERT_TRUE(!tem_cor(COR_AZUL));
}

static void teste_arte_sem_faixa_usa_corpo(void)
{
    int faixas[RESISTOR_FAIXAS] = { SEM_FAIXA, SEM_FAIXA, SEM_FAIXA, COR_DOURADO };
    resistor_arte(faixas, arte);
    ASSERT_TRUE(tem_cor(RESISTOR_CORPO));
    ASSERT_TRUE(tem_cor(COR_DOURADO));
    ASSERT_TRUE(!tem_cor(COR_PRETO));
}

/* ---- tela Novo sem terminal: nada muda (e2e) ---------------------------- */

static void teste_linha_respondida(void)
{
    char buf[256];
    FILE *f = tmpfile();
    size_t n;
    if (f == NULL) return;
    ui_init();
    ui_definir_interativo(0);
    ui_definir_cor(0);
    ui_definir_centralizado(0);
    ui_definir_saida(f);
    ui_linha_respondida("Resistência: ", "4k7");
    fflush(f);
    rewind(f);
    n = fread(buf, 1, sizeof buf - 1, f);
    buf[n] = '\0';
    ui_definir_saida(stdout);
    fclose(f);
    ASSERT_STR_EQ(UI_PROMPT " Resistência: 4k7\n", buf);
}

int main(void)
{
    printf("test_resistor\n");
    RUN_TEST(teste_cores_valores_comuns);
    RUN_TEST(teste_cores_multiplicador_fracionario);
    RUN_TEST(teste_cores_valores_da_entrada);
    RUN_TEST(teste_cores_aproximado);
    RUN_TEST(teste_cores_fora_da_faixa);
    RUN_TEST(teste_nome_cor);
    RUN_TEST(teste_arte_dimensoes);
    RUN_TEST(teste_arte_sem_cor_so_contorno);
    RUN_TEST(teste_arte_colorida);
    RUN_TEST(teste_arte_sem_faixa_usa_corpo);
    RUN_TEST(teste_linha_respondida);
    return tf_resumo("test_resistor");
}
