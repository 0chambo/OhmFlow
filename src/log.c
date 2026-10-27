/*
 * log.c — registro das ações do OhmFlow em arquivo (ver log.h).
 */
#if !defined(_WIN32) && !defined(_POSIX_C_SOURCE)
#define _POSIX_C_SOURCE 200809L /* mkdir com -std=c99 */
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "log.h"

#ifdef _WIN32
#include <direct.h>
#define CRIA_PASTA(p) _mkdir(p)
#define SEP "\\"
#else
#include <sys/stat.h>
#define CRIA_PASTA(p) mkdir((p), 0755)
#define SEP "/"
#endif

#define TAM_CAMINHO 512
#define TAM_LINHA   1024
#define TAM_CAMPO   256

/* Larguras das colunas: nível, ação, alvo, detalhe (o resultado é livre) */
#define LARG_NIVEL   5
#define LARG_ACAO    13
#define LARG_ALVO    10
#define LARG_DETALHE 20

static const char SEPARADOR[] =
    "================================================================";

/* ---- estado ---------------------------------------------------------- */

static FILE *arquivo = NULL;                 /* sessão aberta (NULL = sem log) */
static time_t inicio_sessao = 0;
static char pasta_forcada[TAM_CAMINHO] = ""; /* log_definir_pasta              */
static time_t (*relogio)(void) = NULL;       /* log_definir_relogio            */

static time_t agora(void)
{
    return relogio != NULL ? relogio() : time(NULL);
}

void log_definir_pasta(const char *pasta)
{
    if (pasta == NULL) pasta_forcada[0] = '\0';
    else snprintf(pasta_forcada, sizeof pasta_forcada, "%s", pasta);
}

void log_definir_relogio(time_t (*f)(void)) { relogio = f; }
int  log_ativo(void)                        { return arquivo != NULL; }

/* ---- pasta e arquivo ------------------------------------------------- */

/* Pasta dos logs, na ordem: forçada (testes), OHMFLOW_LOGS_DIR, APPDATA
 * (build do instalador) e, por fim, "logs" na pasta atual */
static int resolve_pasta(char *buf, size_t n)
{
    const char *env = getenv("OHMFLOW_LOGS_DIR");
    int r;
    if (pasta_forcada[0] != '\0')
        r = snprintf(buf, n, "%s", pasta_forcada);
    else if (env != NULL && env[0] != '\0')
        r = snprintf(buf, n, "%s", env);
    else {
#ifdef OHM_LOGS_APPDATA
        const char *appdata = getenv("APPDATA");
        if (appdata != NULL && appdata[0] != '\0')
            r = snprintf(buf, n, "%s" SEP "OhmFlow" SEP "logs", appdata);
        else
#endif
            r = snprintf(buf, n, "logs");
    }
    return r > 0 && (size_t)r < n;
}

/* Cria a pasta e as intermediárias que faltarem (erros são ignorados: quem
 * decide se deu certo é o fopen) */
static void cria_pastas(const char *caminho)
{
    char tmp[TAM_CAMINHO];
    char *p;
    snprintf(tmp, sizeof tmp, "%s", caminho);
    for (p = tmp + 1; *p; p++) {
        if (*p != '/' && *p != '\\') continue;
        if (p[-1] == ':') continue; /* "C:\" não é pasta a criar */
        *p = '\0';
        CRIA_PASTA(tmp);
        *p = SEP[0];
    }
    CRIA_PASTA(tmp);
}

/* Caminho do arquivo de log do dia que contém o instante t */
static int caminho_do_dia_de(time_t t, char *buf, size_t n)
{
    char pasta[TAM_CAMINHO], dia[16];
    const struct tm *tm = localtime(&t);
    int r;
    if (tm == NULL || !resolve_pasta(pasta, sizeof pasta)) return 0;
    strftime(dia, sizeof dia, "%Y-%m-%d", tm);
    r = snprintf(buf, n, "%s" SEP "ohmflow_%s.log", pasta, dia);
    return r > 0 && (size_t)r < n;
}

int log_caminho_do_dia(char *buf, size_t n)
{
    return caminho_do_dia_de(agora(), buf, n);
}

/* "27/10/2026 14:03:12" */
static void formata_data(time_t t, char *buf, size_t n)
{
    const struct tm *tm = localtime(&t);
    if (tm == NULL || strftime(buf, n, "%d/%m/%Y %H:%M:%S", tm) == 0)
        snprintf(buf, n, "--/--/---- --:--:--");
}

/* ---- linha de ação ---------------------------------------------------- */

/* Largura visível em UTF-8 (conta só bytes que iniciam caractere) */
static size_t largura(const char *s)
{
    size_t w = 0;
    for (; *s; s++)
        if (((unsigned char)*s & 0xC0) != 0x80) w++;
    return w;
}

/* Copia o campo trocando "|" por "/" e quebras/tabs por espaço; NULL ou
 * vazio vira "-" */
static void limpa_campo(const char *s, char *buf, size_t n)
{
    size_t i = 0;
    if (s == NULL || s[0] == '\0') s = "-";
    for (; *s && i + 1 < n; s++) {
        char c = *s;
        if (c == '|') c = '/';
        else if (c == '\n' || c == '\r' || c == '\t') c = ' ';
        buf[i++] = c;
    }
    buf[i] = '\0';
}

/* Acrescenta o campo em linha (capacidade cap) completando com espaços até
 * `larg` colunas visíveis (larg 0 = sem preenchimento) */
static void acrescenta(char *linha, size_t cap, const char *campo, size_t larg)
{
    size_t len = strlen(linha), w = largura(campo);
    snprintf(linha + len, cap - len, "%s", campo);
    len = strlen(linha);
    while (w < larg && len + 1 < cap) {
        linha[len++] = ' ';
        linha[len] = '\0';
        w++;
    }
}

size_t log_formata_linha(char *buf, size_t n, time_t t, int nivel, const char *acao,
                         const char *alvo, const char *detalhe, const char *resultado)
{
    static const char *const NIVEIS[] = { "INFO", "AVISO", "ERRO" };
    char linha[TAM_LINHA], campo[TAM_CAMPO];
    size_t len;

    if (buf == NULL || n == 0) return 0;
    formata_data(t, linha, sizeof linha);
    strcat(linha, " | ");
    acrescenta(linha, sizeof linha,
               (nivel >= LOG_INFO && nivel <= LOG_ERRO) ? NIVEIS[nivel] : "INFO", LARG_NIVEL);
    strcat(linha, " | ");
    limpa_campo(acao, campo, sizeof campo);
    acrescenta(linha, sizeof linha, campo, LARG_ACAO);
    strcat(linha, " | ");
    limpa_campo(alvo, campo, sizeof campo);
    acrescenta(linha, sizeof linha, campo, LARG_ALVO);
    strcat(linha, " | ");
    limpa_campo(detalhe, campo, sizeof campo);
    acrescenta(linha, sizeof linha, campo, LARG_DETALHE);
    strcat(linha, " | ");
    limpa_campo(resultado, campo, sizeof campo);
    acrescenta(linha, sizeof linha, campo, 0);

    len = strlen(linha); /* sem espaços sobrando no fim da linha */
    while (len > 0 && linha[len - 1] == ' ') linha[--len] = '\0';

    snprintf(buf, n, "%s", linha);
    return strlen(buf);
}

/* ---- leitura ----------------------------------------------------------- */

/* Copia s[0..len) para buf (capacidade n) sem os espaços do fim */
static void copia_campo(char *buf, size_t n, const char *s, size_t len)
{
    while (len > 0 && s[len - 1] == ' ') len--;
    if (len >= n) len = n - 1;
    memcpy(buf, s, len);
    buf[len] = '\0';
}

int log_parse_linha(const char *linha, LogEntrada *e)
{
    char *destinos[6];
    size_t tamanhos[6];
    const char *p, *sep;
    size_t len;
    int i;

    if (linha == NULL || e == NULL) return 0;
    /* começa com "dd/mm/aaaa hh:mm:ss" */
    if (strlen(linha) < 19 || linha[2] != '/' || linha[5] != '/' || linha[13] != ':')
        return 0;

    destinos[0] = e->data;    tamanhos[0] = sizeof e->data;
    destinos[1] = e->nivel;   tamanhos[1] = sizeof e->nivel;
    destinos[2] = e->acao;    tamanhos[2] = sizeof e->acao;
    destinos[3] = e->alvo;    tamanhos[3] = sizeof e->alvo;
    destinos[4] = e->detalhe; tamanhos[4] = sizeof e->detalhe;
    destinos[5] = e->resultado; tamanhos[5] = sizeof e->resultado;

    p = linha;
    for (i = 0; i < 5; i++) {
        sep = strstr(p, " | ");
        if (sep == NULL) return 0;
        copia_campo(destinos[i], tamanhos[i], p, (size_t)(sep - p));
        p = sep + 3;
    }
    if (strstr(p, " | ") != NULL) return 0; /* coluna a mais */
    len = strcspn(p, "\r\n");
    copia_campo(destinos[5], tamanhos[5], p, len);
    return 1;
}

/* Inverte v[ini..fim) (para girar o buffer circular) */
static void inverte(LogEntrada *v, int ini, int fim)
{
    while (ini < --fim) {
        LogEntrada t = v[ini];
        v[ini++] = v[fim];
        v[fim] = t;
    }
}

int log_ultimas(LogEntrada *v, int max)
{
    char caminho[TAM_CAMINHO], linha[TAM_LINHA];
    time_t hoje = agora();
    long total = 0;
    int d, ini;

    if (v == NULL || max <= 0) return 0;
    /* do dia mais antigo para hoje; buffer circular guarda as `max` últimas */
    for (d = LOG_DIAS_HISTORICO - 1; d >= 0; d--) {
        FILE *f;
        if (!caminho_do_dia_de(hoje - (time_t)d * 86400, caminho, sizeof caminho)) continue;
        f = fopen(caminho, "r");
        if (f == NULL) continue;
        while (fgets(linha, sizeof linha, f) != NULL) {
            LogEntrada e;
            if (!log_parse_linha(linha, &e)) continue;
            v[total % max] = e;
            total++;
        }
        fclose(f);
    }
    if (total <= max) return (int)total;
    /* o mais antigo está em total % max: gira para a ordem cronológica */
    ini = (int)(total % max);
    inverte(v, 0, ini);
    inverte(v, ini, max);
    inverte(v, 0, max);
    return max;
}

/* ---- sessão ------------------------------------------------------------ */

int log_iniciar(void)
{
    char caminho[TAM_CAMINHO], pasta[TAM_CAMINHO], data[32];

    if (arquivo != NULL) return 1; /* já aberta */
    if (!resolve_pasta(pasta, sizeof pasta)) return 0;
    cria_pastas(pasta);
    if (!log_caminho_do_dia(caminho, sizeof caminho)) return 0;
    arquivo = fopen(caminho, "a");
    if (arquivo == NULL) return 0;

    inicio_sessao = agora();
    formata_data(inicio_sessao, data, sizeof data);
    fprintf(arquivo, "%s\n OhmFlow \xE2\x80\x94 sess\xC3\xA3o iniciada em %s\n%s\n",
            SEPARADOR, data, SEPARADOR);
    fflush(arquivo);
    return 1;
}

void log_acao(int nivel, const char *acao, const char *alvo,
              const char *detalhe, const char *resultado)
{
    char linha[TAM_LINHA];
    if (arquivo == NULL) return;
    log_formata_linha(linha, sizeof linha, agora(), nivel, acao, alvo, detalhe, resultado);
    fprintf(arquivo, "%s\n", linha);
    fflush(arquivo); /* a linha sobrevive a janela fechada ou travamento */
}

void log_encerrar(void)
{
    char data[32], duracao[48];
    time_t fim;
    long seg;

    if (arquivo == NULL) return;
    fim = agora();
    seg = (long)difftime(fim, inicio_sessao);
    if (seg < 0) seg = 0;
    if (seg >= 3600)
        snprintf(duracao, sizeof duracao, "%ldh %ldm %lds", seg / 3600, seg % 3600 / 60, seg % 60);
    else
        snprintf(duracao, sizeof duracao, "%ldm %lds", seg / 60, seg % 60);
    formata_data(fim, data, sizeof data);
    fprintf(arquivo, "%s\n OhmFlow \xE2\x80\x94 sess\xC3\xA3o encerrada em %s (%s)\n%s\n\n",
            SEPARADOR, data, duracao, SEPARADOR);
    fclose(arquivo);
    arquivo = NULL;
}
