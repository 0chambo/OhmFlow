/*
 * formatacao.c — parse de resistências digitadas e impressão por grandeza.
 *
 * Gramática aceita pelo parse (após remover espaços nas pontas e o sufixo
 * opcional "ohm"/"ohms"/"Ω"):
 *
 *   decimal :  A [ (.|,) B ] [espaços] [mult]      ex.: 4.7k, 0,47, 220R, 4.7 k
 *   RKM     :  A mult C                            ex.: 4k7, 1M5, 4R7, 2m2
 *
 * onde A, B, C são sequências de dígitos (A ou B não vazio) e mult ∈ {m,R,r,k,K,M}.
 * Na notação RKM a letra ocupa o lugar da vírgula decimal (4k7 = 4,7 kΩ).
 */
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "formatacao.h"
#include "lista.h"

#define TAM_MAX_ENTRADA 64

static const double ESCALAS[]  = { 1e-3, 1.0, 1e3, 1e6 };
static const char *const UNIDADES[] = { "m" SIMBOLO_OHM, SIMBOLO_OHM,
                                        "k" SIMBOLO_OHM, "M" SIMBOLO_OHM };

/* ---- parse ---------------------------------------------------------- */

/*
 * Expoente de 10 do multiplicador `c` em *exp; 0 se `c` não é multiplicador.
 * O valor é montado como texto "A.Be<exp>" e convertido por um único strtod
 * (arredondamento correto): grafias do mesmo número, como "470m" e "0.47",
 * geram exatamente o mesmo double. Multiplicar por 1e-3 não garantia isso,
 * porque 0,001 não é exato em binário.
 */
static int expoente(char c, int *exp)
{
    switch (c) {
    case 'm':           *exp = -3; return 1;
    case 'R': case 'r': *exp =  0; return 1;
    case 'k': case 'K': *exp =  3; return 1;
    case 'M':           *exp =  6; return 1;
    default:            return 0;
    }
}

static void apara_final(char *s, size_t *n)
{
    while (*n > 0 && isspace((unsigned char)s[*n - 1])) s[--*n] = '\0';
}

/* Remove sufixo "ohms", "ohm" (sem diferenciar maiúsculas) ou "Ω" do fim */
static void remove_sufixo_ohm(char *s, size_t *n)
{
    static const char *const SUFIXOS[] = { "ohms", "ohm" };
    size_t i, k;
    if (*n >= 2 && s[*n - 2] == '\xCE' && s[*n - 1] == '\xA9') { /* Ω */
        *n -= 2;
        s[*n] = '\0';
        apara_final(s, n);
        return;
    }
    for (i = 0; i < sizeof SUFIXOS / sizeof SUFIXOS[0]; i++) {
        size_t t = strlen(SUFIXOS[i]);
        if (*n < t) continue;
        for (k = 0; k < t; k++)
            if (tolower((unsigned char)s[*n - t + k]) != SUFIXOS[i][k]) break;
        if (k == t) {
            *n -= t;
            s[*n] = '\0';
            apara_final(s, n);
            return;
        }
    }
}

/* Copia dígitos de *p para dst (até cap-1); devolve quantos copiou */
static size_t le_digitos(const char **p, char *dst, size_t cap)
{
    size_t k = 0;
    while (isdigit((unsigned char)**p)) {
        if (k + 1 < cap) dst[k++] = **p;
        else return cap; /* longo demais */
        (*p)++;
    }
    dst[k] = '\0';
    return k;
}

int parse_resistencia(const char *s, double *ohms)
{
    char buf[TAM_MAX_ENTRADA];
    char inteira[TAM_MAX_ENTRADA], fracao[TAM_MAX_ENTRADA];
    char numero[2 * TAM_MAX_ENTRADA + 8]; /* "A.Be-3" */
    const char *p;
    size_t n, na, nf = 0;
    int exp = 0;
    double valor;

    if (s == NULL || ohms == NULL) return 0;

    while (isspace((unsigned char)*s)) s++;
    n = strlen(s);
    if (n == 0 || n >= sizeof buf) return 0;
    memcpy(buf, s, n + 1);
    apara_final(buf, &n);
    remove_sufixo_ohm(buf, &n);
    if (n == 0) return 0;

    p = buf;
    fracao[0] = '\0';
    na = le_digitos(&p, inteira, sizeof inteira);
    if (na >= sizeof inteira) return 0;

    if (*p == '.' || *p == ',') {                 /* notação decimal */
        p++;
        nf = le_digitos(&p, fracao, sizeof fracao);
        if (nf >= sizeof fracao) return 0;
        if (na == 0 && nf == 0) return 0;
        while (*p == ' ') p++;
        if (*p != '\0') {
            if (!expoente(*p, &exp)) return 0;
            p++;
        }
    } else {
        if (na == 0) return 0;
        while (*p == ' ') p++;
        if (*p != '\0') {
            if (!expoente(*p, &exp)) return 0;
            p++;
            nf = le_digitos(&p, fracao, sizeof fracao); /* RKM: 4k7 */
            if (nf >= sizeof fracao) return 0;
        }
    }
    if (*p != '\0') return 0; /* sobrou lixo: "4kk", "4k7k", "4 7" */

    snprintf(numero, sizeof numero, "%s.%se%d", na ? inteira : "0", nf ? fracao : "0", exp);
    valor = strtod(numero, NULL);

    /* mesma faixa aceita pela lista */
    if (!(valor >= RESISTENCIA_MIN * (1 - 1e-9) && valor <= RESISTENCIA_MAX * (1 + 1e-9)))
        return 0;
    *ohms = valor;
    return 1;
}

/* ---- formatação ----------------------------------------------------- */

static double arredonda3(double v)
{
    return floor(v * 1000.0 + 0.5) / 1000.0;
}

int grandeza_resistencia(double ohms)
{
    int g;
    if (ohms < 1.0) g = GRANDEZA_MILI;
    else if (ohms < 1e3) g = GRANDEZA_UNIDADE;
    else if (ohms < 1e6) g = GRANDEZA_KILO;
    else g = GRANDEZA_MEGA;
    /* 999,9996 Ω arredonda para 1000 Ω: imprimir como 1 kΩ */
    if (g < GRANDEZA_MEGA && arredonda3(ohms / ESCALAS[g]) >= 1000.0) g++;
    return g;
}

void formata_resistencia(double ohms, char *buf, size_t n)
{
    char num[48];
    char *c;
    size_t len;
    int g;

    if (buf == NULL || n == 0) return;
    g = grandeza_resistencia(ohms);
    snprintf(num, sizeof num, "%.3f", arredonda3(ohms / ESCALAS[g]));

    /* remove zeros à direita e o ponto, se sobrar inteiro */
    len = strlen(num);
    if (strchr(num, '.') != NULL) {
        while (len > 0 && num[len - 1] == '0') num[--len] = '\0';
        if (len > 0 && num[len - 1] == '.') num[--len] = '\0';
    }
    c = strchr(num, '.');
    if (c != NULL) *c = ',';

    snprintf(buf, n, "%s %s", num, UNIDADES[g]);
}
