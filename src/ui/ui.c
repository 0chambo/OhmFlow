/*
 * ui.c — primitivas de interface de terminal (cores ANSI, molduras, entrada).
 */
#if !defined(_WIN32) && !defined(_POSIX_C_SOURCE)
#define _POSIX_C_SOURCE 200809L /* isatty / fileno com -std=c99 */
#endif

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ui.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <io.h>
#define UI_ISATTY _isatty
#define UI_FILENO _fileno
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif
#else
#include <unistd.h>
#include <sys/ioctl.h>
#define UI_ISATTY isatty
#define UI_FILENO fileno
#endif

#define TAM_MSG 1024
#define MAX_LINHAS 16

/* ---- estado ---------------------------------------------------------- */

static FILE *ui_out = NULL;    /* destino da saída (stdout ou arquivo de teste) */
static int cor_ativa = 0;      /* emitir códigos ANSI?                          */
static int entrada_tty = 0;    /* stdin é terminal?                             */
static int saida_tty = 0;      /* stdout é terminal?                            */
static int vt_ok = 1;          /* terminal entende sequências ANSI?             */

static FILE *saida(void)
{
    return ui_out != NULL ? ui_out : stdout;
}

static const char *const CODIGOS[UI_COR_TOTAL] = {
    "\033[0m",       /* RESET       */
    "\033[1m",       /* NEGRITO     */
    "\033[2m",       /* DIM         */
    "\033[31m",      /* VERMELHO    */
    "\033[32m",      /* VERDE       */
    "\033[33m",      /* AMARELO     */
    "\033[34m",      /* AZUL        */
    "\033[35m",      /* MAGENTA     */
    "\033[36m",      /* CIANO       */
    "\033[37m",      /* BRANCO      */
    "\033[36m",      /* BORDA       */
    "\033[1;37m",    /* TITULO      */
    "\033[1;33m",    /* DESTAQUE    */
    "\033[1;36m",    /* PROMPT      */
    "\033[38;5;208m",/* LARANJA retro (Claude Code style) */
    "\033[38;5;46m", /* VERDE NEON arcade                */
    "\033[38;5;141m",/* ROXO/LILAS NEON                   */
    "\033[38;5;130m" /* SOMBRA / PROFUNDIDADE (laranja queimado escuro) */
};

void ui_init(void)
{
    const char *sem_cor = getenv("NO_COLOR");
    ui_out = stdout;
    entrada_tty = UI_ISATTY(UI_FILENO(stdin)) != 0;
    saida_tty = UI_ISATTY(UI_FILENO(stdout)) != 0;
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    vt_ok = 0;
    if (saida_tty) {
        HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
        DWORD modo = 0;
        if (GetConsoleMode(h, &modo)
            && SetConsoleMode(h, modo | ENABLE_VIRTUAL_TERMINAL_PROCESSING))
            vt_ok = 1;
    }
#else
    {   /* terminais "dumb" não interpretam sequências ANSI */
        const char *term = getenv("TERM");
        vt_ok = term == NULL || strcmp(term, "dumb") != 0;
    }
#endif
    cor_ativa = saida_tty && vt_ok && !(sem_cor != NULL && sem_cor[0] != '\0');
}

void ui_definir_saida(FILE *f) { ui_out = f; }
void ui_definir_cor(int ativa) { cor_ativa = ativa != 0; }
int  ui_cor_ativa(void)        { return cor_ativa; }
int  ui_interativo(void)       { return entrada_tty && saida_tty; }

const char *ui_cor(int cor)
{
    if (!cor_ativa || cor < 0 || cor >= UI_COR_TOTAL) return "";
    return CODIGOS[cor];
}

/* ---- tela ------------------------------------------------------------ */

void ui_limpar_tela(void)
{
    if (ui_interativo() && vt_ok) {
        fputs("\033[2J\033[H", saida());
        fflush(saida());
    }
}

int ui_colunas_terminal(void)
{
    int colunas = 80;
#ifdef _WIN32
    if (saida_tty) {
        HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        if (GetConsoleScreenBufferInfo(h, &csbi)) {
            colunas = csbi.srWindow.Right - csbi.srWindow.Left + 1;
        }
    }
#endif
    return colunas > 40 ? colunas : 40;
}

int ui_linhas_terminal(void)
{
    int linhas = 24;
#ifdef _WIN32
    if (saida_tty) {
        HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        if (GetConsoleScreenBufferInfo(h, &csbi))
            linhas = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
    }
#else
    if (saida_tty) {
        struct winsize ws;
        if (ioctl(UI_FILENO(saida()), TIOCGWINSZ, &ws) == 0 && ws.ws_row > 0)
            linhas = (int)ws.ws_row;
    }
#endif
    return linhas > 10 ? linhas : 10;
}

void ui_cursor(int visivel)
{
    if (ui_interativo() && vt_ok) {
        fputs(visivel ? "\033[?25h" : "\033[?25l", saida());
        fflush(saida());
    }
}

void ui_pausar(const char *msg)
{
    char lixo[256];
    if (!ui_interativo()) return;
    fprintf(saida(), "\n  %s%s%s", ui_cor(UI_COR_DIM),
            msg ? msg : "Pressione Enter para continuar...", ui_cor(UI_COR_RESET));
    fflush(saida());
    while (fgets(lixo, sizeof lixo, stdin) != NULL && strchr(lixo, '\n') == NULL)
        ; /* descarta até o fim da linha */
}

/* ---- largura e alinhamento ------------------------------------------- */

size_t ui_largura(const char *s)
{
    size_t w = 0;
    const unsigned char *p = (const unsigned char *)s;
    if (s == NULL) return 0;
    while (*p) {
        if (*p == 0x1B && p[1] == '[') {          /* ESC [ ... letra final */
            p += 2;
            while (*p && !(*p >= 0x40 && *p <= 0x7E)) p++;
            if (*p) p++;
            continue;
        }
        if ((*p & 0xC0) != 0x80) w++;             /* não é byte de continuação */
        p++;
    }
    return w;
}

static void repete(FILE *out, const char *s, int n)
{
    while (n-- > 0) fputs(s, out);
}

void ui_pad(FILE *out, const char *s, int largura, int alinhar_dir)
{
    int falta = largura - (int)ui_largura(s);
    if (alinhar_dir) repete(out, " ", falta);
    fputs(s ? s : "", out);
    if (!alinhar_dir) repete(out, " ", falta);
}

/* ---- molduras --------------------------------------------------------- */

typedef struct {
    const char *tl, *tr, *bl, *br; /* cantos                 */
    const char *h, *v;             /* horizontal / vertical  */
    const char *ml, *mr;           /* junções laterais       */
    const char *tm, *bm, *cx;      /* junções de tabela      */
} Moldura;

#ifdef OHM_ASCII
static const Moldura MOLD_DUPLA       = { "+", "+", "+", "+", "=", "|", "+", "+", "+", "+", "+" };
static const Moldura MOLD_SIMPLES     = { "+", "+", "+", "+", "-", "|", "+", "+", "+", "+", "+" };
static const Moldura MOLD_ARREDONDADA = { "+", "+", "+", "+", "-", "|", "+", "+", "+", "+", "+" };
#else
static const Moldura MOLD_DUPLA = {
    "\xE2\x95\x94", "\xE2\x95\x97", "\xE2\x95\x9A", "\xE2\x95\x9D", /* ╔ ╗ ╚ ╝ */
    "\xE2\x95\x90", "\xE2\x95\x91",                                 /* ═ ║     */
    "\xE2\x95\xA0", "\xE2\x95\xA3",                                 /* ╠ ╣     */
    "\xE2\x95\xA6", "\xE2\x95\xA9", "\xE2\x95\xAC"                  /* ╦ ╩ ╬   */
};
static const Moldura MOLD_SIMPLES = {
    "\xE2\x94\x8C", "\xE2\x94\x90", "\xE2\x94\x94", "\xE2\x94\x98", /* ┌ ┐ └ ┘ */
    "\xE2\x94\x80", "\xE2\x94\x82",                                 /* ─ │     */
    "\xE2\x94\x9C", "\xE2\x94\xA4",                                 /* ├ ┤     */
    "\xE2\x94\xAC", "\xE2\x94\xB4", "\xE2\x94\xBC"                  /* ┬ ┴ ┼   */
};
static const Moldura MOLD_ARREDONDADA = {
    "\xE2\x95\xAD", "\xE2\x95\xAE", "\xE2\x95\xB0", "\xE2\x95\xAF", /* ╭ ╮ ╰ ╯ */
    "\xE2\x94\x80", "\xE2\x94\x82",                                 /* ─ │     */
    "\xE2\x94\x9C", "\xE2\x94\xA4",                                 /* ├ ┤     */
    "\xE2\x94\xAC", "\xE2\x94\xB4", "\xE2\x94\xBC"                  /* ┬ ┴ ┼   */
};
#endif

static const Moldura *moldura(int estilo)
{
    if (estilo == UI_MOLDURA_DUPLA) return &MOLD_DUPLA;
    if (estilo == UI_MOLDURA_ARREDONDADA) return &MOLD_ARREDONDADA;
    return &MOLD_SIMPLES;
}

/* Linha "║ conteúdo···· ║" com largura interna `inner` e margem esquerda `margem` */
static void linha_caixa(FILE *out, const Moldura *m, const char *cb, const char *s, int inner, int margem)
{
    const char *rs = ui_cor(UI_COR_RESET);
    repete(out, " ", margem);
    fprintf(out, "%s%s%s ", cb, m->v, rs);
    ui_pad(out, s, inner - 2, 0);
    fprintf(out, "%s %s%s%s\n", rs, cb, m->v, rs);
}

static void ui_caixa_com_margem(int estilo, int cor_borda, const char *titulo_borda,
                                const char *cabecalho, const char *const *linhas, int n,
                                int largura_min, int margem)
{
    FILE *out = saida();
    const Moldura *m = moldura(estilo);
    const char *cb = ui_cor(cor_borda), *rs = ui_cor(UI_COR_RESET);
    int inner = largura_min, w, i;

    for (i = 0; i < n; i++)
        if ((w = (int)ui_largura(linhas[i]) + 2) > inner) inner = w;
    if (cabecalho && (w = (int)ui_largura(cabecalho) + 2) > inner) inner = w;
    if (titulo_borda && (w = (int)ui_largura(titulo_borda) + 4) > inner) inner = w;

    /* topo: ╔═ Título ═════╗ */
    repete(out, " ", margem);
    fprintf(out, "%s%s", cb, m->tl);
    if (titulo_borda) {
        fprintf(out, "%s %s%s%s%s ", m->h, ui_cor(UI_COR_NEGRITO), titulo_borda, rs, cb);
        repete(out, m->h, inner - 3 - (int)ui_largura(titulo_borda));
    } else {
        repete(out, m->h, inner);
    }
    fprintf(out, "%s%s\n", m->tr, rs);

    /* cabeçalho centralizado + separador */
    if (cabecalho) {
        int esq;
        w = (int)ui_largura(cabecalho);
        esq = (inner - w) / 2;
        repete(out, " ", margem);
        fprintf(out, "%s%s%s", cb, m->v, rs);
        repete(out, " ", esq);
        fprintf(out, "%s%s%s", ui_cor(UI_COR_TITULO), cabecalho, rs);
        repete(out, " ", inner - w - esq);
        fprintf(out, "%s%s%s\n", cb, m->v, rs);
        repete(out, " ", margem);
        fprintf(out, "%s%s", cb, m->ml);
        repete(out, m->h, inner);
        fprintf(out, "%s%s\n", m->mr, rs);
    }

    for (i = 0; i < n; i++) linha_caixa(out, m, cb, linhas[i], inner, margem);

    repete(out, " ", margem);
    fprintf(out, "%s%s", cb, m->bl);
    repete(out, m->h, inner);
    fprintf(out, "%s%s\n", m->br, rs);
    fflush(out);
}

void ui_caixa(int estilo, int cor_borda, const char *titulo_borda,
              const char *cabecalho, const char *const *linhas, int n,
              int largura_min)
{
    ui_caixa_com_margem(estilo, cor_borda, titulo_borda, cabecalho, linhas, n, largura_min, 0);
}

void ui_caixa_centralizada(int estilo, int cor_borda, const char *titulo_borda,
                           const char *cabecalho, const char *const *linhas, int n,
                           int largura_min)
{
    int cols = ui_colunas_terminal();
    int inner = largura_min, w, i, margem;
    for (i = 0; i < n; i++)
        if ((w = (int)ui_largura(linhas[i]) + 2) > inner) inner = w;
    if (cabecalho && (w = (int)ui_largura(cabecalho) + 2) > inner) inner = w;
    if (titulo_borda && (w = (int)ui_largura(titulo_borda) + 4) > inner) inner = w;
    
    margem = (cols - (inner + 2)) / 2;
    if (margem < 0) margem = 0;
    ui_caixa_com_margem(estilo, cor_borda, titulo_borda, cabecalho, linhas, n, largura_min, margem);
}

void ui_caixa_titulo(const char *titulo)
{
    ui_caixa(UI_MOLDURA_DUPLA, UI_COR_BORDA, titulo, NULL, NULL, 0, UI_LARGURA);
}

static int cor_do_tipo(int tipo)
{
    switch (tipo) {
    case UI_SUCESSO: return UI_COR_VERDE;
    case UI_ERRO:    return UI_COR_VERMELHO;
    case UI_AVISO:   return UI_COR_AMARELO;
    default:         return UI_COR_AZUL;
    }
}

static const char *icone_do_tipo(int tipo)
{
    switch (tipo) {
    case UI_SUCESSO: return UI_ICONE_OK;
    case UI_ERRO:    return UI_ICONE_ERRO;
    case UI_AVISO:   return UI_ICONE_AVISO;
    default:         return UI_ICONE_INFO;
    }
}

static const char *rotulo_do_tipo(int tipo)
{
    switch (tipo) {
    case UI_SUCESSO: return "SUCESSO";
    case UI_ERRO:    return "ERRO";
    case UI_AVISO:   return "ATEN\xC3\x87\xC3\x83O"; /* ATENÇÃO */
    default:         return "INFO";
    }
}

void ui_caixa_feedback(int tipo, const char *titulo, const char *fmt, ...)
{
    char msg[TAM_MSG], rotulo[64], primeira[TAM_MSG];
    const char *linhas[MAX_LINHAS];
    int n = 0;
    char *p;

    snprintf(rotulo, sizeof rotulo, "%s %s", icone_do_tipo(tipo), rotulo_do_tipo(tipo));
    snprintf(primeira, sizeof primeira, "%s%s%s", ui_cor(UI_COR_NEGRITO),
             titulo ? titulo : "", ui_cor(UI_COR_RESET));
    linhas[n++] = primeira;

    msg[0] = '\0';
    if (fmt != NULL) {
        va_list ap;
        va_start(ap, fmt);
        vsnprintf(msg, sizeof msg, fmt, ap);
        va_end(ap);
    }
    /* quebra em linhas por '\n' */
    p = msg;
    while (*p && n < MAX_LINHAS) {
        char *nl = strchr(p, '\n');
        linhas[n++] = p;
        if (nl == NULL) break;
        *nl = '\0';
        p = nl + 1;
    }

    putc('\n', saida());
    ui_caixa(UI_MOLDURA_SIMPLES, cor_do_tipo(tipo), rotulo, NULL, linhas, n, UI_LARGURA);
}

/* ---- mensagens de uma linha ------------------------------------------- */

void ui_mensagem(int tipo, const char *fmt, ...)
{
    char msg[TAM_MSG];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(msg, sizeof msg, fmt, ap);
    va_end(ap);
    fprintf(saida(), "  %s%s %s%s\n", ui_cor(cor_do_tipo(tipo)), icone_do_tipo(tipo),
            msg, ui_cor(UI_COR_RESET));
    fflush(saida());
}

void ui_dica(const char *fmt, ...)
{
    char msg[TAM_MSG];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(msg, sizeof msg, fmt, ap);
    va_end(ap);
    fprintf(saida(), "  %s%s%s\n", ui_cor(UI_COR_DIM), msg, ui_cor(UI_COR_RESET));
}

/* ---- entrada ---------------------------------------------------------- */

int ui_ler_linha(const char *prompt, char *buf, size_t n)
{
    size_t len, ini = 0;
    FILE *out = saida();

    if (buf == NULL || n == 0) return 0;
    buf[0] = '\0';
    fprintf(out, "%s%s%s %s", ui_cor(UI_COR_PROMPT), UI_PROMPT, ui_cor(UI_COR_RESET),
            prompt ? prompt : "");
    fflush(out);

    if (fgets(buf, (int)n, stdin) == NULL) {
        buf[0] = '\0';
        if (!entrada_tty) putc('\n', out);
        return 0; /* EOF */
    }
    len = strlen(buf);
    if (len > 0 && buf[len - 1] != '\n') { /* linha longa: descarta o resto */
        int c;
        while ((c = getchar()) != '\n' && c != EOF)
            ;
    }
    /* apara espaços (inclui \r\n) nas duas pontas */
    while (len > 0 && isspace((unsigned char)buf[len - 1])) buf[--len] = '\0';
    while (buf[ini] && isspace((unsigned char)buf[ini])) ini++;
    if (ini > 0) memmove(buf, buf + ini, len - ini + 1);

    if (!entrada_tty) fprintf(out, "%s\n", buf); /* eco p/ logs e e2e */
    return 1;
}

int ui_confirmar(const char *fmt, ...)
{
    char msg[TAM_MSG], prompt[TAM_MSG + 64], resp[16];
    int tentativa;
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(msg, sizeof msg, fmt, ap);
    va_end(ap);
    snprintf(prompt, sizeof prompt, "%s%s %s (s/n):%s ", ui_cor(UI_COR_AMARELO),
             UI_ICONE_AVISO, msg, ui_cor(UI_COR_RESET));

    for (tentativa = 0; tentativa < 3; tentativa++) {
        if (!ui_ler_linha(prompt, resp, sizeof resp)) return 0;
        if (resp[0] == 's' || resp[0] == 'S') return 1;
        if (resp[0] == 'n' || resp[0] == 'N') return 0;
        ui_mensagem(UI_ERRO, "Responda s (sim) ou n (n\xC3\xA3o).");
    }
    return 0;
}

/* ---- tabelas ---------------------------------------------------------- */

void ui_tabela_borda(int pos, const int *larguras, int ncol)
{
    FILE *out = saida();
    const Moldura *m = &MOLD_SIMPLES;
    const char *esq, *meio, *dir;
    int i;
    switch (pos) {
    case UI_TAB_TOPO: esq = m->tl; meio = m->tm; dir = m->tr; break;
    case UI_TAB_MEIO: esq = m->ml; meio = m->cx; dir = m->mr; break;
    default:          esq = m->bl; meio = m->bm; dir = m->br; break;
    }
    fprintf(out, "  %s%s", ui_cor(UI_COR_BORDA), esq);
    for (i = 0; i < ncol; i++) {
        repete(out, m->h, larguras[i] + 2);
        fputs(i + 1 < ncol ? meio : dir, out);
    }
    fprintf(out, "%s\n", ui_cor(UI_COR_RESET));
}

void ui_tabela_linha(const char *const *celulas, const int *larguras,
                     const int *alinhar_dir, int ncol)
{
    FILE *out = saida();
    const char *cb = ui_cor(UI_COR_BORDA), *rs = ui_cor(UI_COR_RESET);
    int i;
    fprintf(out, "  %s%s%s", cb, MOLD_SIMPLES.v, rs);
    for (i = 0; i < ncol; i++) {
        putc(' ', out);
        ui_pad(out, celulas[i], larguras[i], alinhar_dir ? alinhar_dir[i] : 0);
        fprintf(out, "%s %s%s%s", rs, cb, MOLD_SIMPLES.v, rs);
    }
    putc('\n', out);
}
