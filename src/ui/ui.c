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
#include <conio.h>
#define UI_ISATTY _isatty
#define UI_FILENO _fileno
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif
#else
#include <errno.h>
#include <termios.h>
#include <time.h>
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
static int centralizar = 0;    /* telas centralizadas (ui_definir_centralizado) */
static int quadro_aberto = 0;  /* entre ui_quadro_inicio e ui_quadro_fim        */
static int quadro_limpar = 1;  /* próximo quadro limpa a tela inteira?          */
static int quadro_cols = 0, quadro_rows = 0; /* tamanho do terminal no último quadro */

static void repete(FILE *out, const char *s, int n);
static void recuo_texto(FILE *out, size_t largura);

static FILE *saida(void)
{
    return ui_out != NULL ? ui_out : stdout;
}

/* fflush, exceto no meio de um quadro: ele sai inteiro em ui_quadro_fim */
static void descarrega(FILE *out)
{
    if (!quadro_aberto) fflush(out);
}

static const char *const CODIGOS[UI_COR_TOTAL] = {
    "\033[0m",       /* RESET       */
    "\033[1m",       /* NEGRITO     */
    "\033[38;5;244m",/* DIM         = cinza das dicas    */
    "\033[31m",      /* VERMELHO    */
    "\033[32m",      /* VERDE       */
    "\033[33m",      /* AMARELO     */
    "\033[34m",      /* AZUL        */
    "\033[35m",      /* MAGENTA     */
    "\033[36m",      /* CIANO       */
    "\033[37m",      /* BRANCO      */
    "\033[38;5;240m",  /* BORDA       = cinza fino         */
    "\033[1;38;5;255m",/* TITULO      = secundária negrito */
    "\033[1;38;5;208m",/* PROMPT      = primária negrito   */
    "\033[38;5;208m",  /* LARANJA retro (Claude Code style) */
    "\033[38;5;46m",   /* VERDE NEON arcade                */
    "\033[38;5;141m",  /* ROXO/LILAS NEON                   */
    "\033[38;5;208m",  /* PRIMARIA    = laranja (destaque) */
    "\033[38;5;255m",  /* SECUNDARIA  = branco 255         */
    "\033[38;5;245m",  /* CINZA       = rótulos/metadados  */
    "\033[48;5;236m"   /* FUNDO_FOCO  = fundo da linha ativa */
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
    SetConsoleTitleA("OhmFlow");
    {
        HWND hwnd = GetConsoleWindow();
        if (hwnd) {
            HICON icon = LoadIconA(GetModuleHandleA(NULL),
                                   MAKEINTRESOURCEA(1));
            if (icon) {
                SendMessageA(hwnd, WM_SETICON, ICON_BIG,   (LPARAM)icon);
                SendMessageA(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)icon);
            }
        }
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
void ui_definir_interativo(int ativo) { entrada_tty = saida_tty = ativo != 0; }

const char *ui_cor(int cor)
{
    if (!cor_ativa || cor < 0 || cor >= UI_COR_TOTAL) return "";
    return CODIGOS[cor];
}

/* ---- tela ------------------------------------------------------------ */

void ui_limpar_tela(void)
{
    quadro_limpar = 1; /* a tela mudou: o próximo quadro começa do zero */
    if (ui_interativo() && vt_ok) {
        fflush(saida()); /* o que está no buffer sai antes do Fill do console */
#ifdef _WIN32
        /* Preenche o buffer inteiro: nada de tela anterior para reaparecer */
        HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        if (GetConsoleScreenBufferInfo(h, &csbi)) {
            COORD origem = { 0, 0 };
            DWORD n = (DWORD)csbi.dwSize.X * (DWORD)csbi.dwSize.Y, escritos;
            FillConsoleOutputCharacterA(h, ' ', n, origem, &escritos);
            FillConsoleOutputAttribute(h, csbi.wAttributes, n, origem, &escritos);
            SetConsoleCursorPosition(h, origem);
        }
#endif
        fputs("\033[3J\033[2J\033[H", saida());
        fflush(saida());
    }
}

/* Tamanho visível do terminal; devolve 0 (sem tocar em cols/rows) se não for
 * terminal ou não der para medir */
static int tamanho_console(int *cols, int *rows)
{
    if (!saida_tty) return 0;
#ifdef _WIN32
    {
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        if (!GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi)) return 0;
        *cols = csbi.srWindow.Right - csbi.srWindow.Left + 1;
        *rows = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
    }
#else
    {
        struct winsize ws;
        if (ioctl(UI_FILENO(saida()), TIOCGWINSZ, &ws) != 0
            || ws.ws_col == 0 || ws.ws_row == 0) return 0;
        *cols = (int)ws.ws_col;
        *rows = (int)ws.ws_row;
    }
#endif
    return 1;
}

int ui_colunas_terminal(void)
{
    int colunas = 80, linhas = 24;
    tamanho_console(&colunas, &linhas);
    return colunas > 40 ? colunas : 40;
}

int ui_linhas_terminal(void)
{
    int colunas = 80, linhas = 24;
    tamanho_console(&colunas, &linhas);
    return linhas > 10 ? linhas : 10;
}

/* ---- centralização ---------------------------------------------------- */

void ui_definir_centralizado(int ativo) { centralizar = ativo != 0; }
int  ui_centralizado(void)              { return centralizar; }

int ui_margem(void)
{
    int m;
    if (!centralizar) return 0;
    m = (ui_colunas_terminal() - (UI_LARGURA + 2)) / 2;
    return m > 0 ? m : 0;
}

void ui_espaco_vertical(int altura)
{
    int pad;
    if (!centralizar) return;
    pad = (ui_linhas_terminal() - altura) / 2;
    repete(saida(), "\n", pad);
    descarrega(saida());
}

void ui_cursor(int visivel)
{
    if (ui_interativo() && vt_ok) {
        fputs(visivel ? "\033[?25h" : "\033[?25l", saida());
        fflush(saida());
    }
}

void ui_bufferizar_saida(void)
{
    static int feito = 0;
    if (feito || !ui_interativo()) return;
    feito = 1;
    setvbuf(stdout, NULL, _IOFBF, 1 << 16);
}

void ui_descartar_entrada(void)
{
    if (!entrada_tty) return;
#ifdef _WIN32
    FlushConsoleInputBuffer(GetStdHandle(STD_INPUT_HANDLE));
#else
    tcflush(STDIN_FILENO, TCIFLUSH);
#endif
}

/* ---- quadros (redesenho sem limpar a tela) ---------------------------- */

void ui_quadro_reset(void) { quadro_limpar = 1; }

void ui_quadro_inicio(void)
{
    int cols, rows;
    if (!(ui_interativo() && vt_ok)) return;
    cols = ui_colunas_terminal();
    rows = ui_linhas_terminal();
    if (quadro_limpar || cols != quadro_cols || rows != quadro_rows) {
        ui_limpar_tela();
    } else {
        fputs("\033[H", saida()); /* volta ao topo; o quadro novo sobrescreve */
    }
    quadro_limpar = 0;
    quadro_cols = cols;
    quadro_rows = rows;
    quadro_aberto = 1;
}

void ui_quadro_fim(void)
{
    if (!(ui_interativo() && vt_ok)) return;
    fputs("\033[J", saida()); /* apaga sobras do quadro anterior abaixo */
    quadro_aberto = 0;
    fflush(saida());
}

#define TECLA_ESTENDIDA_0   0
#define TECLA_ESTENDIDA_224 224

int ui_espera_enter_de(int (*proxima)(void *ctx), void *ctx)
{
    for (;;) {
        int c = proxima(ctx);
        if (c == EOF) return 0;
        if (c == '\r' || c == '\n') return 1;
        if ((c == TECLA_ESTENDIDA_0 || c == TECLA_ESTENDIDA_224) && proxima(ctx) == EOF)
            return 0; /* setas, F1...: o código da tecla vem no 2º byte */
    }
}

#ifdef _WIN32
static int le_getch(void *ctx)
{
    (void)ctx;
    return _getch(); /* lê do console sem eco */
}
#else
static int le_byte_cru(void *ctx)
{
    unsigned char b;
    (void)ctx;
    for (;;) {
        long n = (long)read(STDIN_FILENO, &b, 1);
        if (n == 1) return b;
        if (n < 0 && errno == EINTR) continue;
        return EOF;
    }
}
#endif

void ui_esperar_enter(void)
{
    fflush(saida());
#ifdef _WIN32
    ui_espera_enter_de(le_getch, NULL);
#else
    struct termios original, cru;
    if (tcgetattr(STDIN_FILENO, &original) != 0) {
        int c; /* sem controle do terminal: descarta a linha */
        while ((c = getchar()) != '\n' && c != EOF)
            ;
        return;
    }
    cru = original;
    cru.c_lflag &= ~((tcflag_t)(ICANON | ECHO));
    cru.c_cc[VMIN] = 1;
    cru.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &cru);
    ui_espera_enter_de(le_byte_cru, NULL);
    tcsetattr(STDIN_FILENO, TCSANOW, &original);
#endif
}

void ui_pausar(const char *msg)
{
    if (!ui_interativo()) return;
    if (msg == NULL) msg = "Pressione Enter para continuar...";
    putc('\n', saida());
    recuo_texto(saida(), ui_largura(msg));
    fprintf(saida(), "%s%s%s", ui_cor(UI_COR_DIM), msg, ui_cor(UI_COR_RESET));
    fflush(saida());
    ui_esperar_enter();
    putc('\n', saida());
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
/* Margem interna (colunas de cada lado do texto): o card arredondado
 * respira mais (2) que as molduras simples e duplas (1) */
static int respiro(int estilo)
{
    return estilo == UI_MOLDURA_ARREDONDADA ? 2 : 1;
}

/* Largura interna da caixa: cabe o maior texto, com respiro, e o título
 * embutido na borda ("── Título ─"), nunca abaixo de `largura_min` */
static int largura_interna(int estilo, const char *titulo_borda, const char *cabecalho,
                           const char *const *linhas, int n, int largura_min)
{
    int inner = largura_min, w, i, pad = respiro(estilo);
    for (i = 0; i < n; i++)
        if ((w = (int)ui_largura(linhas[i]) + 2 * pad) > inner) inner = w;
    if (cabecalho && (w = (int)ui_largura(cabecalho) + 2) > inner) inner = w;
    if (titulo_borda && (w = (int)ui_largura(titulo_borda) + 5) > inner) inner = w;
    return inner;
}

static void linha_caixa(FILE *out, const Moldura *m, const char *cb, const char *s,
                        int inner, int margem, int pad)
{
    const char *rs = ui_cor(UI_COR_RESET);
    repete(out, " ", margem);
    fprintf(out, "%s%s%s", cb, m->v, rs);
    repete(out, " ", pad);
    ui_pad(out, s, inner - 2 * pad, 0);
    fputs(rs, out);
    repete(out, " ", pad);
    fprintf(out, "%s%s%s\n", cb, m->v, rs);
}

static void ui_caixa_com_margem(int estilo, int cor_borda, const char *titulo_borda,
                                const char *cabecalho, const char *const *linhas, int n,
                                int largura_min, int margem)
{
    FILE *out = saida();
    const Moldura *m = moldura(estilo);
    const char *cb = ui_cor(cor_borda), *rs = ui_cor(UI_COR_RESET);
    int pad = respiro(estilo);
    int inner = largura_interna(estilo, titulo_borda, cabecalho, linhas, n, largura_min);
    int i;

    /* topo: ╭── Título ─────╮ */
    repete(out, " ", margem);
    fprintf(out, "%s%s", cb, m->tl);
    if (titulo_borda) {
        fprintf(out, "%s%s %s%s%s%s ", m->h, m->h, ui_cor(UI_COR_TITULO), titulo_borda, rs, cb);
        repete(out, m->h, inner - 4 - (int)ui_largura(titulo_borda));
    } else {
        repete(out, m->h, inner);
    }
    fprintf(out, "%s%s\n", m->tr, rs);

    /* cabeçalho centralizado + separador */
    if (cabecalho) {
        int esq, w;
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

    /* card: uma linha em branco acima e abaixo do conteúdo */
    if (pad > 1 && n > 0) linha_caixa(out, m, cb, "", inner, margem, pad);
    for (i = 0; i < n; i++) linha_caixa(out, m, cb, linhas[i], inner, margem, pad);
    if (pad > 1 && n > 0) linha_caixa(out, m, cb, "", inner, margem, pad);

    repete(out, " ", margem);
    fprintf(out, "%s%s", cb, m->bl);
    repete(out, m->h, inner);
    fprintf(out, "%s%s\n", m->br, rs);
    descarrega(out);
}

void ui_caixa(int estilo, int cor_borda, const char *titulo_borda,
              const char *cabecalho, const char *const *linhas, int n,
              int largura_min)
{
    if (centralizar) {
        ui_caixa_centralizada(estilo, cor_borda, titulo_borda, cabecalho, linhas, n, largura_min);
        return;
    }
    ui_caixa_com_margem(estilo, cor_borda, titulo_borda, cabecalho, linhas, n, largura_min, 0);
}

void ui_caixa_centralizada(int estilo, int cor_borda, const char *titulo_borda,
                           const char *cabecalho, const char *const *linhas, int n,
                           int largura_min)
{
    int cols = ui_colunas_terminal();
    int inner = largura_interna(estilo, titulo_borda, cabecalho, linhas, n, largura_min);
    int margem = (cols - (inner + 2)) / 2;
    if (margem < 0) margem = 0;
    ui_caixa_com_margem(estilo, cor_borda, titulo_borda, cabecalho, linhas, n, largura_min, margem);
}

static int cor_do_tipo(int tipo)
{
    switch (tipo) {
    case UI_SUCESSO: return UI_COR_VERDE;
    case UI_ERRO:    return UI_COR_VERMELHO;
    case UI_AVISO:   return UI_COR_AMARELO;
    default:         return UI_COR_SECUNDARIA;
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
    case UI_SUCESSO: return "OK";
    case UI_ERRO:    return "ERRO";
    case UI_AVISO:   return "ATEN\xC3\x87\xC3\x83O"; /* ATENÇÃO */
    case UI_NEUTRO:  return "VAZIO";
    default:         return "INFO";
    }
}

void ui_caixa_feedback(int tipo, const char *titulo, const char *fmt, ...)
{
    char msg[TAM_MSG], rotulo[96], primeira[TAM_MSG];
    const char *linhas[MAX_LINHAS];
    int n = 0;
    char *p;

    /* badge na borda; a cor semântica fica só nele, a moldura é cinza */
    ui_badge(tipo, NULL, rotulo, sizeof rotulo);
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
    ui_caixa(UI_MOLDURA_SIMPLES, UI_COR_BORDA, rotulo, NULL, linhas, n, UI_LARGURA);
}

/* ---- badges, tracking e micro-animações ------------------------------ */

void ui_badge(int tipo, const char *rotulo, char *buf, size_t n)
{
    const char *fundo;
    if (buf == NULL || n == 0) return;
    if (rotulo == NULL) rotulo = rotulo_do_tipo(tipo);
    if (!cor_ativa) {
        snprintf(buf, n, "[ %s ]", rotulo);
        return;
    }
    switch (tipo) { /* texto escuro sobre o fundo semântico */
    case UI_SUCESSO: fundo = "\033[1;38;5;232;48;5;71m";  break; /* verde  */
    case UI_AVISO:   fundo = "\033[1;38;5;232;48;5;214m"; break; /* âmbar  */
    case UI_ERRO:    fundo = "\033[1;38;5;232;48;5;167m"; break; /* vermelho */
    case UI_NEUTRO:  fundo = "\033[1;38;5;255;48;5;240m"; break; /* cinza  */
    default:         fundo = "\033[1;38;5;232;48;5;208m"; break; /* laranja */
    }
    snprintf(buf, n, "%s  %s  %s", fundo, rotulo, CODIGOS[UI_COR_RESET]);
}

/* Acrescenta `k` bytes de s em buf[*pos], sem passar de n */
static void anexa_n(char *buf, size_t n, size_t *pos, const char *s, size_t k)
{
    if (*pos + 1 >= n) return;
    if (k > n - 1 - *pos) k = n - 1 - *pos;
    memcpy(buf + *pos, s, k);
    *pos += k;
    buf[*pos] = '\0';
}

int ui_espacar(const char *s, char *buf, size_t n, int max_colunas)
{
    const unsigned char *p;
    size_t pos = 0;
    int colunas = 0, primeiro = 1, entre_palavras = 0;

    if (buf == NULL || n == 0) return 0;
    buf[0] = '\0';
    if (s == NULL) return 0;
    for (p = (const unsigned char *)s; *p; ) {
        char letra[4];
        size_t tam = *p < 0x80 ? 1 : (*p >= 0xF0 ? 4 : (*p >= 0xE0 ? 3 : 2));
        if (*p == ' ') { entre_palavras = 1; p++; continue; }
        memcpy(letra, p, tam);
        if (tam == 1) {
            letra[0] = (char)toupper(*p);
        } else if (tam == 2 && p[0] == 0xC3 && p[1] >= 0xA0 && p[1] <= 0xBE && p[1] != 0xB7) {
            letra[1] = (char)(p[1] - 0x20); /* à..þ -> À..Þ (Latin-1) */
        }
        if (!primeiro) {
            anexa_n(buf, n, &pos, "   ", entre_palavras ? 3 : 1);
            colunas += entre_palavras ? 3 : 1;
        }
        anexa_n(buf, n, &pos, letra, tam);
        colunas++;
        primeiro = 0;
        entre_palavras = 0;
        p += tam;
    }
    if (colunas > max_colunas || pos + 1 >= n) {
        snprintf(buf, n, "%s", s);
        return 0;
    }
    return 1;
}

void ui_pausa_ms(int ms)
{
    if (!ui_interativo() || ms <= 0) return;
    fflush(saida());
#ifdef _WIN32
    Sleep((DWORD)ms);
#else
    {
        struct timespec t;
        t.tv_sec = ms / 1000;
        t.tv_nsec = (long)(ms % 1000) * 1000000L;
        nanosleep(&t, NULL);
    }
#endif
}

int ui_tecla_pendente(void)
{
    if (!entrada_tty) return 0;
#ifdef _WIN32
    return _kbhit() != 0;
#else
    return 0; /* POSIX: a tecla só é lida em modo cru dentro de ler_tecla */
#endif
}

/* ---- mensagens de uma linha ------------------------------------------- */

/* Recuo de uma linha de texto com `largura` colunas visíveis: "  " normalmente;
 * no modo centralizado, alinhada ao texto do bloco central ou, se não couber
 * nele, centralizada pela própria largura */
static void recuo_texto(FILE *out, size_t largura)
{
    int m;
    if (!centralizar) {
        fputs("  ", out);
        return;
    }
    if ((int)largura + 4 <= UI_LARGURA + 2) {
        repete(out, " ", ui_margem() + 2);
        return;
    }
    m = (ui_colunas_terminal() - (int)largura) / 2;
    repete(out, " ", m > 0 ? m : 0);
}

void ui_mensagem(int tipo, const char *fmt, ...)
{
    char msg[TAM_MSG];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(msg, sizeof msg, fmt, ap);
    va_end(ap);
    recuo_texto(saida(), ui_largura(icone_do_tipo(tipo)) + 1 + ui_largura(msg));
    fprintf(saida(), "%s%s %s%s\n", ui_cor(cor_do_tipo(tipo)), icone_do_tipo(tipo),
            msg, ui_cor(UI_COR_RESET));
    descarrega(saida());
}

void ui_dica(const char *fmt, ...)
{
    char msg[TAM_MSG];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(msg, sizeof msg, fmt, ap);
    va_end(ap);
    recuo_texto(saida(), ui_largura(msg));
    fprintf(saida(), "%s%s%s\n", ui_cor(UI_COR_DIM), msg, ui_cor(UI_COR_RESET));
}

/* ---- entrada ---------------------------------------------------------- */

void ui_linha_respondida(const char *prompt, const char *resposta)
{
    FILE *out = saida();
    if (centralizar)
        recuo_texto(out, ui_largura(UI_PROMPT) + 1 + ui_largura(prompt));
    fprintf(out, "%s%s%s %s%s\n", ui_cor(UI_COR_PROMPT), UI_PROMPT, ui_cor(UI_COR_RESET),
            prompt ? prompt : "", resposta ? resposta : "");
    descarrega(out);
}

int ui_ler_linha(const char *prompt, char *buf, size_t n)
{
    size_t len, ini = 0;
    FILE *out = saida();

    if (buf == NULL || n == 0) return 0;
    buf[0] = '\0';
    if (centralizar) /* alinha com o texto do bloco (sem centralizado: coluna 0) */
        recuo_texto(out, ui_largura(UI_PROMPT) + 1 + ui_largura(prompt));
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

/* ---- barra de proporção ---------------------------------------------- */

/* Acrescenta s em buf[*pos], sem passar de n (corta em silêncio) */
static void anexa(char *buf, size_t n, size_t *pos, const char *s)
{
    size_t k = strlen(s);
    if (*pos + 1 >= n) return;
    if (k > n - 1 - *pos) k = n - 1 - *pos;
    memcpy(buf + *pos, s, k);
    *pos += k;
    buf[*pos] = '\0';
}

void ui_barra(double fracao, int largura, int cor, char *buf, size_t n)
{
#ifndef OHM_ASCII
    static const char *const OITAVOS[8] = {
        "", "\xE2\x96\x8F", "\xE2\x96\x8E", "\xE2\x96\x8D",  /*   ▏ ▎ ▍ */
        "\xE2\x96\x8C", "\xE2\x96\x8B", "\xE2\x96\x8A", "\xE2\x96\x89" /* ▌ ▋ ▊ ▉ */
    };
#endif
    size_t pos = 0;
    int cheias, resto, vazias, i;

    if (buf == NULL || n == 0) return;
    buf[0] = '\0';
    if (largura <= 0) return;
    if (!(fracao > 0.0)) fracao = 0.0; /* também NaN */
    if (fracao > 1.0) fracao = 1.0;
    {   /* arredonda para o oitavo de célula mais próximo */
        int oitavos = (int)(fracao * largura * 8 + 0.5);
#ifdef OHM_ASCII
        oitavos = (oitavos + 4) / 8 * 8; /* sem frações: célula inteira */
#endif
        cheias = oitavos / 8;
        resto = oitavos % 8;
    }
    vazias = largura - cheias - (resto > 0);

    if (cheias > 0 || resto > 0) {
        anexa(buf, n, &pos, ui_cor(cor));
        for (i = 0; i < cheias; i++) anexa(buf, n, &pos, UI_BLOCO_CHEIO);
#ifndef OHM_ASCII
        anexa(buf, n, &pos, OITAVOS[resto]);
#endif
        anexa(buf, n, &pos, ui_cor(UI_COR_RESET));
    }
    if (vazias > 0) {
        anexa(buf, n, &pos, ui_cor(UI_COR_DIM));
        for (i = 0; i < vazias; i++) anexa(buf, n, &pos, UI_BLOCO_VAZIO);
        anexa(buf, n, &pos, ui_cor(UI_COR_RESET));
    }
}

/* ---- tabelas ---------------------------------------------------------- */

/* Recuo da tabela: "  " normalmente; centralizada pela largura total no modo
 * centralizado (borda + largura + 2 de respiro + borda, por coluna) */
static void recuo_tabela(FILE *out, const int *larguras, int ncol)
{
    int total = 1, i, m;
    if (!centralizar) {
        fputs("  ", out);
        return;
    }
    for (i = 0; i < ncol; i++) total += larguras[i] + 3;
    m = (ui_colunas_terminal() - total) / 2;
    repete(out, " ", m > 0 ? m : 0);
}

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
    recuo_tabela(out, larguras, ncol);
    fprintf(out, "%s%s", ui_cor(UI_COR_BORDA), esq);
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
    recuo_tabela(out, larguras, ncol);
    fprintf(out, "%s%s%s", cb, MOLD_SIMPLES.v, rs);
    for (i = 0; i < ncol; i++) {
        putc(' ', out);
        ui_pad(out, celulas[i], larguras[i], alinhar_dir ? alinhar_dir[i] : 0);
        fprintf(out, "%s %s%s%s", rs, cb, MOLD_SIMPLES.v, rs);
    }
    putc('\n', out);
}
