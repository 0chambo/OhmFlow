/* test_log.c — testes do sistema de logs (formato, sessão, falhas de I/O) */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "test_framework.h"
#include "log.h"

#define PASTA_TESTE "build/test_log_tmp/logs"

/* ---- relógio falso ---------------------------------------------------- */

static time_t agora_falso = 0;

static time_t relogio_falso(void)
{
    return agora_falso;
}

/* Data/hora no formato do log, calculada como o módulo calcula */
static void data_log(time_t t, char *buf, size_t n)
{
    strftime(buf, n, "%d/%m/%Y %H:%M:%S", localtime(&t));
}

/* Lê o arquivo inteiro (até cap-1 bytes); "" se não existir */
static void le_arquivo(const char *caminho, char *buf, size_t cap)
{
    FILE *f = fopen(caminho, "r");
    size_t n = 0;
    buf[0] = '\0';
    if (f == NULL) return;
    n = fread(buf, 1, cap - 1, f);
    buf[n] = '\0';
    fclose(f);
}

static int conta(const char *texto, const char *trecho)
{
    int n = 0;
    const char *p = texto;
    while ((p = strstr(p, trecho)) != NULL) {
        n++;
        p += strlen(trecho);
    }
    return n;
}

/* Começa cada teste de sessão com o arquivo do dia apagado */
static void prepara_sessao(void)
{
    char caminho[512];
    log_definir_relogio(relogio_falso);
    log_definir_pasta(PASTA_TESTE);
    agora_falso = (time_t)1800000000; /* data fixa */
    if (log_caminho_do_dia(caminho, sizeof caminho)) remove(caminho);
}

/* ---- formatação de linha --------------------------------------------- */

static void teste_linha_alinhada(void)
{
    char linha[256], esperado[256], data[32];
    time_t t = (time_t)1800000000;
    data_log(t, data, sizeof data);
    log_formata_linha(linha, sizeof linha, t, LOG_INFO, "INSERIR",
                      "4,7 k\xCE\xA9", "qtd 50", "OK");
    snprintf(esperado, sizeof esperado,
             "%s | INFO  | INSERIR       | 4,7 k\xCE\xA9     | qtd 50               | OK", data);
    ASSERT_STR_EQ(esperado, linha);
}

static void teste_linha_niveis(void)
{
    char linha[256];
    log_formata_linha(linha, sizeof linha, 0, LOG_AVISO, "RETIRAR", "1 \xCE\xA9", "-70",
                      "SEM ESTOQUE");
    ASSERT_TRUE(strstr(linha, " | AVISO | RETIRAR       | ") != NULL);
    log_formata_linha(linha, sizeof linha, 0, LOG_ERRO, "INSERIR", "\"abc\"", NULL,
                      "VALOR INV\xC3\x81LIDO");
    ASSERT_TRUE(strstr(linha, " | ERRO  | INSERIR       | \"abc\"      | -                    | "
                              "VALOR INV\xC3\x81LIDO") != NULL);
}

static void teste_linha_campos_nulos(void)
{
    char linha[256];
    log_formata_linha(linha, sizeof linha, 0, LOG_INFO, "REINICIALIZAR", NULL, "3 tipos", NULL);
    ASSERT_TRUE(strstr(linha, " | REINICIALIZAR | -          | 3 tipos              | -") != NULL);
}

static void teste_linha_sanitiza_barra_e_quebra(void)
{
    char linha[256];
    log_formata_linha(linha, sizeof linha, 0, LOG_ERRO, "MENU", "\"a|b\nc\"", NULL,
                      "OP|\r\n");
    ASSERT_TRUE(strstr(linha, "\"a/b c\"") != NULL);
    ASSERT_TRUE(strchr(linha, '\n') == NULL);
    ASSERT_TRUE(strchr(linha, '\r') == NULL);
    ASSERT_EQ_INT(5, conta(linha, " | ")); /* só os separadores das 6 colunas */
}

static void teste_linha_buffer_pequeno(void)
{
    char linha[16];
    log_formata_linha(linha, sizeof linha, 0, LOG_INFO, "INSERIR", "1 \xCE\xA9", "qtd 1", "OK");
    ASSERT_EQ_INT(15, (int)strlen(linha)); /* trunca sem estourar */
}

/* ---- sessão em arquivo ----------------------------------------------- */

static void teste_sessao_cria_pasta_e_cabecalho(void)
{
    char caminho[512], texto[4096], data[32];
    prepara_sessao();
    ASSERT_EQ_INT(1, log_iniciar());
    ASSERT_TRUE(log_ativo());
    ASSERT_TRUE(log_caminho_do_dia(caminho, sizeof caminho));
    ASSERT_TRUE(strstr(caminho, "ohmflow_") != NULL);
    ASSERT_TRUE(strstr(caminho, ".log") != NULL);
    log_encerrar();
    ASSERT_TRUE(!log_ativo());

    le_arquivo(caminho, texto, sizeof texto);
    data_log(agora_falso, data, sizeof data);
    ASSERT_TRUE(strstr(texto, "OhmFlow \xE2\x80\x94 sess\xC3\xA3o iniciada em ") != NULL);
    ASSERT_TRUE(strstr(texto, data) != NULL);
    ASSERT_TRUE(strstr(texto, "================================================================\n")
                != NULL);
}

static void teste_sessao_acao_e_rodape(void)
{
    char caminho[512], texto[4096];
    prepara_sessao();
    ASSERT_EQ_INT(1, log_iniciar());
    agora_falso += 20;
    log_acao(LOG_INFO, "INSERIR", "4,7 k\xCE\xA9", "qtd 50", "OK");
    agora_falso += 160;
    log_encerrar();

    log_caminho_do_dia(caminho, sizeof caminho);
    le_arquivo(caminho, texto, sizeof texto);
    ASSERT_TRUE(strstr(texto, "| INFO  | INSERIR       | 4,7 k\xCE\xA9     | qtd 50               | OK\n")
                != NULL);
    ASSERT_TRUE(strstr(texto, "sess\xC3\xA3o encerrada em ") != NULL);
    ASSERT_TRUE(strstr(texto, "(3m 0s)") != NULL);
}

static void teste_duracao_com_horas(void)
{
    char caminho[512], texto[4096];
    prepara_sessao();
    ASSERT_EQ_INT(1, log_iniciar());
    agora_falso += 3600 + 125;
    log_encerrar();
    log_caminho_do_dia(caminho, sizeof caminho);
    le_arquivo(caminho, texto, sizeof texto);
    ASSERT_TRUE(strstr(texto, "(1h 2m 5s)") != NULL);
}

static void teste_segunda_sessao_anexa(void)
{
    char caminho[512], texto[8192];
    prepara_sessao();
    ASSERT_EQ_INT(1, log_iniciar());
    log_acao(LOG_INFO, "LISTAR", NULL, "0 tipos", "OK");
    log_encerrar();
    ASSERT_EQ_INT(1, log_iniciar());
    log_acao(LOG_INFO, "LISTAR", NULL, "0 tipos", "OK");
    log_encerrar();

    log_caminho_do_dia(caminho, sizeof caminho);
    le_arquivo(caminho, texto, sizeof texto);
    ASSERT_EQ_INT(2, conta(texto, "sess\xC3\xA3o iniciada"));
    ASSERT_EQ_INT(2, conta(texto, "sess\xC3\xA3o encerrada"));
    ASSERT_EQ_INT(2, conta(texto, "| LISTAR "));
}

static void teste_pasta_invalida_nao_quebra(void)
{
    FILE *f;
    /* um ARQUIVO no lugar da pasta: mkdir e fopen falham */
    f = fopen("build/test_log_tmp/arquivo.txt", "w");
    ASSERT_TRUE(f != NULL);
    if (f) fclose(f);
    log_definir_relogio(relogio_falso);
    log_definir_pasta("build/test_log_tmp/arquivo.txt/logs");
    ASSERT_EQ_INT(0, log_iniciar());
    ASSERT_TRUE(!log_ativo());
    log_acao(LOG_INFO, "INSERIR", "1 \xCE\xA9", "qtd 1", "OK"); /* no-op */
    log_encerrar();                                          /* no-op */
    remove("build/test_log_tmp/arquivo.txt");
}

static void teste_sem_iniciar_e_noop(void)
{
    log_encerrar();
    ASSERT_TRUE(!log_ativo());
    log_acao(LOG_ERRO, "X", NULL, NULL, NULL);
    ASSERT_TRUE(!log_ativo());
}

/* ---- leitura (tela Histórico) ----------------------------------------- */

static void teste_parse_linha_de_acao(void)
{
    char linha[256], data[32];
    LogEntrada e;
    time_t t = (time_t)1800000000;
    log_formata_linha(linha, sizeof linha, t, LOG_AVISO, "RETIRAR", "4,7 k\xCE\xA9",
                      "-100 (tem 7)", "SEM ESTOQUE");
    ASSERT_EQ_INT(1, log_parse_linha(linha, &e));
    data_log(t, data, sizeof data);
    ASSERT_STR_EQ(data, e.data);
    ASSERT_STR_EQ("AVISO", e.nivel);
    ASSERT_STR_EQ("RETIRAR", e.acao);
    ASSERT_STR_EQ("4,7 k\xCE\xA9", e.alvo);
    ASSERT_STR_EQ("-100 (tem 7)", e.detalhe);
    ASSERT_STR_EQ("SEM ESTOQUE", e.resultado);
}

static void teste_parse_ignora_o_que_nao_e_acao(void)
{
    LogEntrada e;
    ASSERT_EQ_INT(0, log_parse_linha("================================================", &e));
    ASSERT_EQ_INT(0, log_parse_linha(" OhmFlow \xE2\x80\x94 sess\xC3\xA3o iniciada em 1", &e));
    ASSERT_EQ_INT(0, log_parse_linha("", &e));
    ASSERT_EQ_INT(0, log_parse_linha("abc | def", &e));
    ASSERT_EQ_INT(0, log_parse_linha(NULL, &e));
}

static void teste_ultimas_acoes_em_ordem(void)
{
    LogEntrada v[2];
    prepara_sessao();
    ASSERT_EQ_INT(1, log_iniciar());
    log_acao(LOG_INFO, "A1", NULL, NULL, "OK");
    agora_falso += 1;
    log_acao(LOG_INFO, "A2", NULL, NULL, "OK");
    agora_falso += 1;
    log_acao(LOG_ERRO, "A3", NULL, NULL, "FALHA");
    log_encerrar();

    ASSERT_EQ_INT(2, log_ultimas(v, 2)); /* só as 2 mais recentes */
    ASSERT_STR_EQ("A2", v[0].acao);       /* em ordem cronológica  */
    ASSERT_STR_EQ("A3", v[1].acao);
    ASSERT_STR_EQ("ERRO", v[1].nivel);
    ASSERT_EQ_INT(0, log_ultimas(v, 0));
}

static void teste_ultimas_junta_dias(void)
{
    char caminho[512];
    LogEntrada v[10];
    time_t hoje = (time_t)1800000000;

    log_definir_relogio(relogio_falso);
    log_definir_pasta(PASTA_TESTE);
    agora_falso = hoje - 86400; /* ontem */
    if (log_caminho_do_dia(caminho, sizeof caminho)) remove(caminho);
    ASSERT_EQ_INT(1, log_iniciar());
    log_acao(LOG_INFO, "ONTEM", NULL, NULL, "OK");
    log_encerrar();

    agora_falso = hoje;
    if (log_caminho_do_dia(caminho, sizeof caminho)) remove(caminho);
    ASSERT_EQ_INT(1, log_iniciar());
    log_acao(LOG_INFO, "HOJE", NULL, NULL, "OK");
    log_encerrar();

    ASSERT_EQ_INT(2, log_ultimas(v, 10));
    ASSERT_STR_EQ("ONTEM", v[0].acao);
    ASSERT_STR_EQ("HOJE", v[1].acao);
}

static void teste_ultimas_sem_arquivos(void)
{
    LogEntrada v[4];
    log_definir_relogio(relogio_falso);
    log_definir_pasta("build/test_log_tmp/nao_existe");
    ASSERT_EQ_INT(0, log_ultimas(v, 4));
}

int main(void)
{
    puts("test_log");
    RUN_TEST(teste_linha_alinhada);
    RUN_TEST(teste_linha_niveis);
    RUN_TEST(teste_linha_campos_nulos);
    RUN_TEST(teste_linha_sanitiza_barra_e_quebra);
    RUN_TEST(teste_linha_buffer_pequeno);
    RUN_TEST(teste_sessao_cria_pasta_e_cabecalho);
    RUN_TEST(teste_sessao_acao_e_rodape);
    RUN_TEST(teste_duracao_com_horas);
    RUN_TEST(teste_segunda_sessao_anexa);
    RUN_TEST(teste_pasta_invalida_nao_quebra);
    RUN_TEST(teste_sem_iniciar_e_noop);
    RUN_TEST(teste_parse_linha_de_acao);
    RUN_TEST(teste_parse_ignora_o_que_nao_e_acao);
    RUN_TEST(teste_ultimas_acoes_em_ordem);
    RUN_TEST(teste_ultimas_junta_dias);
    RUN_TEST(teste_ultimas_sem_arquivos);
    log_definir_pasta(NULL);
    log_definir_relogio(NULL);
    return tf_resumo("log");
}
