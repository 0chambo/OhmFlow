/*
 * resistor.c — código de cores de 4 faixas e desenho do resistor.
 */
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include "resistor.h"

/* Pixel-art 32x12 do resistor (src/assets/ressistor_preencher.svg):
 *   o = contorno, l = terminal, c = corpo, 1..4 = faixas, ' ' = vazio */
static const char *const MAPA[RESISTOR_LINHAS * 2] = {
    "       oooo          oooo       ",
    "      occ11oooooooooo44cco      ",
    "     occc11cc22cc33cc44ccco     ",
    "     occc11cc22cc33cc44ccco     ",
    "   oooccc11cc22cc33cc44cccooo   ",
    "ooolloccc11cc22cc33cc44cccolloo ",
    "olloooccc11cc22cc33cc44cccooollo",
    "ooo  occc11cc22cc33cc44ccco  ooo",
    "     occc11cc22cc33cc44ccco     ",
    "     occc11cc22cc33cc44ccco     ",
    "      occ11oooooooooo44cco      ",
    "       oooo          oooo       ",
};

/* Sombreamento do corpo por linha de pixel (%): topo claro, base escura */
static const int SOMBRA[RESISTOR_LINHAS * 2] = {
    100, 110, 115, 108, 100, 100, 100, 100, 85, 72, 60, 100
};

typedef struct { int r, g, b; } Rgb;

/* Cores da tabela do código de cores, depois corpo, contorno e terminal */
static const Rgb PALETA[] = {
    {   0,   0,   0 }, /* preto    */
    { 102,  51,   0 }, /* marrom   */
    { 255,   0,   0 }, /* vermelho */
    { 227, 108,  10 }, /* laranja  */
    { 255, 255,   0 }, /* amarelo  */
    { 146, 208,  80 }, /* verde    */
    {   0, 112, 192 }, /* azul     */
    { 204,  51, 153 }, /* violeta  */
    { 128, 128, 128 }, /* cinza    */
    { 255, 255, 255 }, /* branco   */
    { 204, 153,   0 }, /* dourado  */
    { 217, 217, 217 }, /* prata    */
    { 214, 170, 120 }, /* corpo    */
    {  90,  90,  90 }, /* contorno */
    { 170, 170, 170 }, /* terminal */
};

static const char *const NOMES[] = {
    "preto", "marrom", "vermelho", "laranja", "amarelo", "verde",
    "azul", "violeta", "cinza", "branco", "dourado", "prata"
};

/* ---- código de cores --------------------------------------------------- */

int codigo_cores(double ohms, int faixas[RESISTOR_FAIXAS])
{
    int e;
    long n;
    double m;

    faixas[0] = faixas[1] = faixas[2] = SEM_FAIXA;
    faixas[3] = COR_DOURADO;
    if (!(ohms > 0) || !isfinite(ohms)) return CORES_FORA;

    /* ohms = m x 10^e com m em [10, 100): dois algarismos significativos */
    e = (int)floor(log10(ohms)) - 1;
    m = ohms / pow(10.0, e);
    if (m < 9.5) { e--; m = ohms / pow(10.0, e); } /* log10 arredondou para cima */
    n = (long)floor(m + 0.5);
    if (n >= 100) { e++; m = ohms / pow(10.0, e); n = (long)floor(m + 0.5); }

    /* multiplicador: nº de zeros 0..9, dourado x0,1 ou prata x0,01 */
    if (e < -2 || e > 9) return CORES_FORA;
    faixas[0] = (int)(n / 10);
    faixas[1] = (int)(n % 10);
    faixas[2] = e >= 0 ? e : (e == -1 ? COR_DOURADO : COR_PRATA);
    return fabs(m - (double)n) <= 1e-9 * m ? CORES_EXATO : CORES_APROXIMADO;
}

const char *nome_cor_faixa(int cor)
{
    if (cor < COR_PRETO || cor > COR_PRATA) return "-";
    return NOMES[cor];
}

void resistor_rgb_faixa(int cor, char *buf, size_t n)
{
    if (buf == NULL || n == 0) return;
    if (cor < COR_PRETO || cor > RESISTOR_TERMINAL) cor = RESISTOR_CORPO;
    snprintf(buf, n, ";2;%d;%d;%dm", PALETA[cor].r, PALETA[cor].g, PALETA[cor].b);
}

/* ---- desenho ----------------------------------------------------------- */

static int limita(int v) { return v > 255 ? 255 : v; }

/* Cor do pixel (x, y) em *c; 0 se o pixel é vazio (fundo do terminal) */
static int pixel(const int *faixas, int x, int y, Rgb *c)
{
    char p = MAPA[y][x];
    int cor;
    switch (p) {
    case 'o': *c = PALETA[RESISTOR_CONTORNO]; return 1;
    case 'l': *c = PALETA[RESISTOR_TERMINAL]; return 1;
    case 'c': cor = RESISTOR_CORPO; break;
    case '1': case '2': case '3': case '4':
        cor = faixas != NULL ? faixas[p - '1'] : SEM_FAIXA;
        if (cor < COR_PRETO || cor > COR_PRATA) cor = RESISTOR_CORPO;
        break;
    default: return 0;
    }
    if (faixas == NULL) return 0; /* sem cor: corpo e faixas vazios */
    c->r = limita(PALETA[cor].r * SOMBRA[y] / 100);
    c->g = limita(PALETA[cor].g * SOMBRA[y] / 100);
    c->b = limita(PALETA[cor].b * SOMBRA[y] / 100);
    return 1;
}

/* Acrescenta texto formatado em buf a partir de *pos, sem estourar n */
static void acrescenta(char *buf, size_t n, size_t *pos, const char *fmt, ...)
{
    va_list ap;
    int w;
    if (*pos >= n) return;
    va_start(ap, fmt);
    w = vsnprintf(buf + *pos, n - *pos, fmt, ap);
    va_end(ap);
    if (w > 0) *pos += (size_t)w;
    if (*pos >= n) *pos = n - 1;
}

void resistor_arte(const int *faixas, char linhas[RESISTOR_LINHAS][RESISTOR_TAM_LINHA])
{
    int l, x;
    for (l = 0; l < RESISTOR_LINHAS; l++) {
        char *buf = linhas[l];
        size_t pos = 0;
        acrescenta(buf, RESISTOR_TAM_LINHA, &pos, "%*s", RESISTOR_RECUO, "");
        /* cada célula de texto junta dois pixels: o de cima e o de baixo */
        for (x = 0; x < RESISTOR_COLUNAS; x++) {
            Rgb cima, baixo;
            int tem_cima = pixel(faixas, x, 2 * l, &cima);
            int tem_baixo = pixel(faixas, x, 2 * l + 1, &baixo);
            acrescenta(buf, RESISTOR_TAM_LINHA, &pos, "\033[0m");
            if (tem_cima && tem_baixo)
                acrescenta(buf, RESISTOR_TAM_LINHA, &pos,
                           "\033[38;2;%d;%d;%dm\033[48;2;%d;%d;%dm\xE2\x96\x80", /* ▀ */
                           cima.r, cima.g, cima.b, baixo.r, baixo.g, baixo.b);
            else if (tem_cima)
                acrescenta(buf, RESISTOR_TAM_LINHA, &pos, "\033[38;2;%d;%d;%dm\xE2\x96\x80",
                           cima.r, cima.g, cima.b);
            else if (tem_baixo)
                acrescenta(buf, RESISTOR_TAM_LINHA, &pos, "\033[38;2;%d;%d;%dm\xE2\x96\x84", /* ▄ */
                           baixo.r, baixo.g, baixo.b);
            else
                acrescenta(buf, RESISTOR_TAM_LINHA, &pos, " ");
        }
        acrescenta(buf, RESISTOR_TAM_LINHA, &pos, "\033[0m");
    }
}
