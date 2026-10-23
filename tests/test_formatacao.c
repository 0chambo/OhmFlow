/* test_formatacao.c — testes de parse e formatação de resistências */
#include "test_framework.h"
#include "formatacao.h"
#include "lista.h"

#define OHM SIMBOLO_OHM

/* ---- parse: entradas válidas ---------------------------------------- */

static double parse_ok(const char *s)
{
    double v = -1;
    if (!parse_resistencia(s, &v)) {
        TF_FALHA("parse_resistencia(\"%s\") deveria aceitar", s);
        return -1;
    }
    return v;
}

static void teste_parse_numero_puro(void)
{
    ASSERT_EQ_DBL(4700, parse_ok("4700"), 1e-12);
    ASSERT_EQ_DBL(220, parse_ok("220"), 1e-12);
    ASSERT_EQ_DBL(0.47, parse_ok("0.47"), 1e-12);
    ASSERT_EQ_DBL(0.47, parse_ok("0,47"), 1e-12);
    ASSERT_EQ_DBL(1.5, parse_ok("1.5"), 1e-12);
}

static void teste_parse_multiplicador(void)
{
    ASSERT_EQ_DBL(4700, parse_ok("4.7k"), 1e-12);
    ASSERT_EQ_DBL(4700, parse_ok("4,7k"), 1e-12);
    ASSERT_EQ_DBL(4700, parse_ok("4.7K"), 1e-12);
    ASSERT_EQ_DBL(10e6, parse_ok("10M"), 1e-12);
    ASSERT_EQ_DBL(0.47, parse_ok("470m"), 1e-12);
    ASSERT_EQ_DBL(220, parse_ok("220R"), 1e-12);
    ASSERT_EQ_DBL(4700, parse_ok("4.7 k"), 1e-12);
}

static void teste_parse_notacao_rkm(void)
{
    ASSERT_EQ_DBL(4700, parse_ok("4k7"), 1e-12);
    ASSERT_EQ_DBL(4700, parse_ok("4K7"), 1e-12);
    ASSERT_EQ_DBL(1.5e6, parse_ok("1M5"), 1e-12);
    ASSERT_EQ_DBL(4.7, parse_ok("4R7"), 1e-12);
    ASSERT_EQ_DBL(2.2e-3, parse_ok("2m2"), 1e-12); /* 2,2 mΩ */
}

static void teste_parse_espacos_e_sufixo(void)
{
    ASSERT_EQ_DBL(220, parse_ok(" 220 "), 1e-12);
    ASSERT_EQ_DBL(220, parse_ok("\t220\n"), 1e-12);
    ASSERT_EQ_DBL(220, parse_ok("220ohm"), 1e-12);
    ASSERT_EQ_DBL(220, parse_ok("220 ohms"), 1e-12);
    ASSERT_EQ_DBL(220, parse_ok("220 OHM"), 1e-12);
    ASSERT_EQ_DBL(4700, parse_ok("4k7 " OHM), 1e-12);
    ASSERT_EQ_DBL(4700, parse_ok("4.7k\xCE\xA9"), 1e-12);
    ASSERT_EQ_DBL(0.47, parse_ok("470mohm"), 1e-12);
}

static void teste_parse_limites(void)
{
    ASSERT_EQ_DBL(RESISTENCIA_MIN, parse_ok("1m"), 1e-12);
    ASSERT_EQ_DBL(RESISTENCIA_MAX, parse_ok("1000M"), 1e-12);
}

/* ---- parse: entradas inválidas -------------------------------------- */

static void parse_falha(const char *s)
{
    double v = 123.0;
    tf_asserts++;
    if (parse_resistencia(s, &v)) TF_FALHA("parse_resistencia(\"%s\") deveria rejeitar", s);
    if (v != 123.0) TF_FALHA("parse_resistencia(\"%s\") alterou *ohms ao falhar", s);
}

static void teste_parse_invalidos(void)
{
    parse_falha("");
    parse_falha("   ");
    parse_falha("abc");
    parse_falha("-10");
    parse_falha("+10");
    parse_falha("0");
    parse_falha("0k");
    parse_falha("4kk");
    parse_falha("4k7k");
    parse_falha("k");
    parse_falha("k7");
    parse_falha(".");
    parse_falha("4.7.1");
    parse_falha("4.7k5");
    parse_falha("1e3");
    parse_falha("4 7");
    parse_falha("ohm");
    parse_falha("0.5m");    /* abaixo de 1 mΩ   */
    parse_falha("2000M");   /* acima de 1000 MΩ */
    parse_falha("99999999999999999999999");
    ASSERT_EQ_INT(0, parse_resistencia(NULL, NULL));
}

/* ---- formatação ----------------------------------------------------- */

static void fmt_igual(double ohms, const char *esperado)
{
    char buf[32];
    formata_resistencia(ohms, buf, sizeof buf);
    ASSERT_STR_EQ(esperado, buf);
}

static void teste_formata_grandezas(void)
{
    fmt_igual(0.47, "470 m" OHM);
    fmt_igual(0.001, "1 m" OHM);
    fmt_igual(1, "1 " OHM);
    fmt_igual(220, "220 " OHM);
    fmt_igual(4.7, "4,7 " OHM);
    fmt_igual(4700, "4,7 k" OHM);
    fmt_igual(1000, "1 k" OHM);
    fmt_igual(1e6, "1 M" OHM);
    fmt_igual(2.2e6, "2,2 M" OHM);
    fmt_igual(1e9, "1000 M" OHM);
}

static void teste_formata_casas_decimais(void)
{
    fmt_igual(1234.5, "1,235 k" OHM);  /* 3 casas, arredondado */
    fmt_igual(999.5, "999,5 " OHM);
    fmt_igual(12.345, "12,345 " OHM);
    fmt_igual(100, "100 " OHM);        /* sem zeros à direita */
}

static void teste_formata_fronteira_arredondamento(void)
{
    fmt_igual(999.9996, "1 k" OHM);       /* arredonda p/ 1000 Ω -> 1 kΩ */
    fmt_igual(0.9999996, "1 " OHM);
    fmt_igual(999999.6, "1 M" OHM);
    ASSERT_EQ_INT(GRANDEZA_KILO, grandeza_resistencia(999.9996));
    ASSERT_EQ_INT(GRANDEZA_MILI, grandeza_resistencia(0.47));
    ASSERT_EQ_INT(GRANDEZA_UNIDADE, grandeza_resistencia(220));
    ASSERT_EQ_INT(GRANDEZA_KILO, grandeza_resistencia(4700));
    ASSERT_EQ_INT(GRANDEZA_MEGA, grandeza_resistencia(2.2e6));
}

static void teste_formata_buffer_pequeno(void)
{
    char buf[4] = "xxx";
    formata_resistencia(4700, buf, sizeof buf);
    ASSERT_EQ_INT(3, (long long)strlen(buf)); /* truncado, mas terminado em '\0' */
    formata_resistencia(4700, NULL, 0);       /* não pode quebrar */
}

static void teste_ida_e_volta(void)
{
    const char *entradas[] = { "470m", "1", "4R7", "220", "4k7", "10k", "1M5", "1000M" };
    char buf[32];
    double v, w;
    size_t i;
    for (i = 0; i < sizeof entradas / sizeof entradas[0]; i++) {
        v = parse_ok(entradas[i]);
        formata_resistencia(v, buf, sizeof buf);
        w = parse_ok(buf);
        ASSERT_TRUE(mesma_resistencia(v, w));
    }
}

int main(void)
{
    printf("test_formatacao\n");
    RUN_TEST(teste_parse_numero_puro);
    RUN_TEST(teste_parse_multiplicador);
    RUN_TEST(teste_parse_notacao_rkm);
    RUN_TEST(teste_parse_espacos_e_sufixo);
    RUN_TEST(teste_parse_limites);
    RUN_TEST(teste_parse_invalidos);
    RUN_TEST(teste_formata_grandezas);
    RUN_TEST(teste_formata_casas_decimais);
    RUN_TEST(teste_formata_fronteira_arredondamento);
    RUN_TEST(teste_formata_buffer_pequeno);
    RUN_TEST(teste_ida_e_volta);
    return tf_resumo("test_formatacao");
}
