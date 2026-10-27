/*
 * telas.c — telas do OhmFlow.
 *
 * Roteiro: [1] boas-vindas -> [2] menu -> [3] tela da função -> [4] feedback
 * (inserir/remover/retirar/repor/reinicializar) -> volta ao [2].
 * Listar e consultar mostram o resultado na própria tela [3].
 * Enter vazio (ou fim da entrada) em qualquer pergunta cancela a operação.
 */
#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#  include <conio.h>
#else
#  include <termios.h>
#  include <unistd.h>
#endif
#include "telas.h"
#include "ui.h"
#include "formatacao.h"
#include "log.h"

#define TENTATIVAS  3
#define TAM_ENTRADA 128
#define TAM_RES     32

/* Altura aproximada de cada tela (linhas), para a centralização vertical:
 * título (3) + dicas + perguntas + caixa de feedback (5) + pausa (2) */
#define ALTURA_NOVO           17
#define ALTURA_REMOVER        14
#define ALTURA_LISTAR_BASE    10 /* + uma linha por tipo */
#define ALTURA_CONSULTAR      12
#define ALTURA_MOVIMENTO      15 /* retirar e repor */
#define ALTURA_REINICIALIZAR  17
#define ALTURA_DIFERENCIAL    10
#define ALTURA_ENCERRAMENTO   4
#define ALTURA_HISTORICO_BASE 10 /* + uma linha por ação */

/* Ações mostradas no Histórico e larguras das colunas: quando, ação,
 * resistor, detalhe, resultado */
#define HISTORICO_MAX 15
static const int HIST_LARG[] = { 11, 13, 10, 16, 14 };

/* Larguras das colunas da tabela de estoque: #, Resistência, Quantidade */
static const int TAB_LARG[] = { 2, 12, 10 };
static const int TAB_DIR[]  = { 1, 0, 1 };

/* ---- auxiliares ------------------------------------------------------- */

static const char *plural(long long n, const char *singular, const char *plur)
{
    return n == 1 ? singular : plur;
}

static void voltar_ao_menu(void)
{
    ui_pausar("Pressione Enter para voltar ao menu...");
}

/* Registra no log o desfecho de uma tela; detalhe em formato printf (NULL = "-") */
static void registra(int nivel, const char *acao, const char *alvo,
                     const char *resultado, const char *fmt, ...)
{
    char detalhe[96];
    detalhe[0] = '\0';
    if (fmt != NULL) {
        va_list ap;
        va_start(ap, fmt);
        vsnprintf(detalhe, sizeof detalhe, fmt, ap);
        va_end(ap);
    }
    log_acao(nivel, acao, alvo, detalhe, resultado);
}

static void feedback_cancelado(const char *acao, const char *alvo)
{
    registra(LOG_AVISO, acao, alvo, "CANCELADO", NULL);
    ui_caixa_feedback(UI_AVISO, "Operação cancelada", NULL);
    voltar_ao_menu();
}

/* Limpa a tela e desenha o título. `altura` é a estimativa de linhas da tela
 * inteira (título, perguntas, feedback e pausa), usada para centralizá-la na
 * vertical quando o modo centralizado está ligado */
static void abre_tela(const char *titulo, int altura)
{
    ui_limpar_tela();
    ui_espaco_vertical(altura);
    ui_caixa(UI_MOLDURA_ARREDONDADA, UI_COR_BORDA, titulo, NULL, NULL, 0, UI_LARGURA);
}

#define MSG_VALOR_INVALIDO "Valor inválido. Exemplos: 220, 4k7, 4.7k, 1M, 470m " \
                           "(faixa 1 m" SIMBOLO_OHM " a 1000 M" SIMBOLO_OHM ")"

/* Registra no log uma entrada que não é uma resistência válida */
static void registra_invalido(const char *acao, const char *entrada)
{
    char citado[40];
    snprintf(citado, sizeof citado, "\"%.24s\"", entrada);
    registra(LOG_ERRO, acao, citado, "VALOR INVÁLIDO", NULL);
}

/* Lê uma resistência; 0 se cancelado (vazio/EOF) ou após TENTATIVAS inválidas.
 * Cada entrada inválida vai para o log como ERRO da `acao` */
static int ler_resistencia(const char *acao, double *ohms)
{
    char buf[TAM_ENTRADA];
    int t;
    for (t = 0; t < TENTATIVAS; t++) {
        if (!ui_ler_linha("Resistência: ", buf, sizeof buf) || buf[0] == '\0')
            return 0;
        if (parse_resistencia(buf, ohms)) return 1;
        registra_invalido(acao, buf);
        ui_mensagem(UI_ERRO, MSG_VALOR_INVALIDO);
    }
    return 0;
}

/* 1 se s é um inteiro em [minimo, INT_MAX] (grava em *qtd), 0 caso contrário */
static int parse_quantidade(const char *s, int minimo, int *qtd)
{
    char *fim;
    long long v; /* long long: maior que int em qualquer plataforma */
    errno = 0;
    v = strtoll(s, &fim, 10);
    if (s[0] == '\0' || errno != 0 || *fim != '\0' || v < minimo || v > INT_MAX) return 0;
    *qtd = (int)v;
    return 1;
}

/* Lê um inteiro em [minimo, INT_MAX]; 0 se cancelado. Entradas inválidas vão
 * para o log como ERRO da `acao` sobre o resistor `alvo` */
static int ler_quantidade(const char *acao, const char *alvo, const char *prompt,
                          int minimo, int *qtd)
{
    char buf[TAM_ENTRADA];
    int t;
    for (t = 0; t < TENTATIVAS; t++) {
        if (!ui_ler_linha(prompt, buf, sizeof buf) || buf[0] == '\0') return 0;
        if (parse_quantidade(buf, minimo, qtd)) return 1;
        registra(LOG_ERRO, acao, alvo, "QUANTIDADE INVÁLIDA", "\"%.20s\"", buf);
        ui_mensagem(UI_ERRO, "Quantidade inválida: informe um inteiro de %d a %d.",
                    minimo, INT_MAX);
    }
    return 0;
}

int parse_item(const char *s, Resistor *r)
{
    /* larguras de %s = TAM_ENTRADA - 1; s mais curto que TAM_ENTRADA cabe inteiro */
    char res[TAM_ENTRADA], qtd[TAM_ENTRADA], resto[2];
    int campos;
    if (s == NULL || r == NULL || strlen(s) >= TAM_ENTRADA) return ITEM_INVALIDO;
    campos = sscanf(s, "%127s %127s %1s", res, qtd, resto);
    if ((campos != 1 && campos != 2) || !parse_resistencia(res, &r->resistencia))
        return ITEM_INVALIDO;
    r->quantidade = 0;
    if (campos == 1) return ITEM_SO_RESISTENCIA;
    return parse_quantidade(qtd, 0, &r->quantidade) ? ITEM_COMPLETO : ITEM_INVALIDO;
}

/* Lê a resistência e garante que ela está cadastrada; preenche texto e qtd */
static int ler_cadastrado(const char *acao, Lista *li, double *ohms, char *txt, int *qtd)
{
    if (!ler_resistencia(acao, ohms)) {
        feedback_cancelado(acao, NULL);
        return 0;
    }
    formata_resistencia(*ohms, txt, TAM_RES);
    if (consulta_lista(li, *ohms, qtd) != LISTA_OK) {
        registra(LOG_AVISO, acao, txt, "NÃO ENCONTRADO", NULL);
        ui_caixa_feedback(UI_ERRO, "Resistor não encontrado",
                          "%s não está no estoque", txt);
        voltar_ao_menu();
        return 0;
    }
    return 1;
}

/* ---- [1] boas-vindas -------------------------------------------------- */

void tela_boas_vindas(void)
{
    /* "big" figlet font — O mais largo, ~65 cols, contínuo */
    static const char *const LOGO[6] = {
        "  ____  _                 _____ _                    ",
        " / __ \\| |__  _ __ ___   |  ___| | _____      __    ",
        "| |  | | '_ \\| '_ ` _ \\  | |_  | |/ _ \\ \\ /\\ / /   ",
        "| |__| | | | | | | | | | |  _| | | (_) \\ V  V /    ",
        " \\____/|_| |_|_| |_| |_| |_|   |_|\\___/ \\_/\\_/     ",
        "                                                      ",
    };
    static const char SUBTITULO[] = "Estoque de Resistores \xe2\x80\x94 Estrutura de Dados";
    static const char MSG[]       = "Sistema pronto. Pressione Enter para continuar";

    /* Altura do conteúdo: 6 logo + 1 blank + subtítulo + 2 blank + msg = 11 */
    int cols = ui_colunas_terminal();
    int rows = ui_linhas_terminal();
    int i, esq, topo;
    const char *laranja = ui_cor(UI_COR_PRIMARIA);
    const char *dim     = ui_cor(UI_COR_DIM);
    const char *rs      = ui_cor(UI_COR_RESET);

    ui_limpar_tela();

    /* Padding vertical para centralizar */
    topo = (rows - 11) / 2;
    if (topo < 1) topo = 1;
    for (i = 0; i < topo; i++) putchar('\n');

    /* Logo laranja contínuo (sem quebras entre linhas) */
    esq = (cols - 54) / 2;
    if (esq < 0) esq = 0;
    for (i = 0; i < 6; i++) {
        printf("%*s%s%s%s\n", esq, "", laranja, LOGO[i], rs);
    }
    putchar('\n');

    /* Subtítulo centralizado */
    {
        int t_esq = (cols - (int)ui_largura(SUBTITULO)) / 2;
        if (t_esq < 0) t_esq = 0;
        printf("%*s%s%s%s\n\n", t_esq, "", dim, SUBTITULO, rs);
    }

    /* Mensagem estática centralizada */
    {
        int m_esq = (cols - (int)ui_largura(MSG) - 3) / 2;
        if (m_esq < 0) m_esq = 0;
        printf("%*s%s\xc2\xbb%s %s%s%s\n", m_esq, "", laranja, rs, dim, MSG, rs);
    }

    /* Aguarda Enter sem ecoar o que for digitado */
    if (ui_interativo()) {
        fflush(stdout);
        ui_esperar_enter();
    }
}

/* ---- [2] menu principal ----------------------------------------------- */

#define TECLA_ESC    27
#define TECLA_CTRL_D 4

int decodifica_tecla(int (*proximo)(void *ctx, int bloqueia), void *ctx)
{
    int c = proximo(ctx, 1);
    if (c == EOF || c == TECLA_CTRL_D) return KEY_FIM;
    if (c != TECLA_ESC) return c;

    /* ESC: só é seta se '[' ou 'O' chegar logo em seguida */
    c = proximo(ctx, 0);
    if (c == EOF) return KEY_FIM;
    if (c == TECLA_TIMEOUT) return KEY_ESC;        /* Esc sozinho        */
    if (c != '[' && c != 'O') return KEY_IGNORADA; /* Alt+tecla          */
    c = proximo(ctx, 0);
    if (c == EOF) return KEY_FIM;
    if (c == 'A') return KEY_UP;
    if (c == 'B') return KEY_DOWN;
    if (c == 'C') return KEY_RIGHT;
    if (c == 'D') return KEY_LEFT;
    return KEY_IGNORADA;
}

/* Cursor escondido pelo menu: restaurado também se o processo for interrompido */
static volatile sig_atomic_t cursor_oculto = 0;

#ifndef _WIN32
/* Modo do terminal antes de ler_tecla() entrar em modo raw */
static struct termios term_original;
static volatile sig_atomic_t term_raw = 0;

/* Lê um byte; sem `bloqueia`, espera no máximo 0,1 s (VTIME) */
// cppcheck-suppress constParameterCallback ; assinatura fixada por decodifica_tecla
static int le_byte_terminal(void *ctx, int bloqueia)
{
    struct termios t = *(const struct termios *)ctx;
    unsigned char b;
    t.c_cc[VMIN]  = bloqueia ? 1 : 0;
    t.c_cc[VTIME] = bloqueia ? 0 : 1;
    if (tcsetattr(STDIN_FILENO, TCSANOW, &t) != 0) return bloqueia ? EOF : TECLA_TIMEOUT;
    for (;;) {
        long n = (long)read(STDIN_FILENO, &b, 1);
        if (n == 1) return b;
        if (n < 0 && errno == EINTR) continue;
        if (n == 0 && !bloqueia) return TECLA_TIMEOUT;
        return EOF; /* terminal fechado ou erro de leitura */
    }
}
#endif

static void restaura_terminal_e_sai(int sinal)
{
#ifndef _WIN32
    if (term_raw) tcsetattr(STDIN_FILENO, TCSANOW, &term_original);
    if (cursor_oculto && write(STDOUT_FILENO, "\033[?25h", 6) < 0) {
        /* nada a fazer: o processo está encerrando */
    }
#endif
    signal(sinal, SIG_DFL);
    raise(sinal);
}

/* Instala (uma vez) os tratadores que devolvem terminal e cursor ao normal */
static void protege_terminal(void)
{
    static int instalado = 0;
    if (instalado) return;
    instalado = 1;
    signal(SIGINT, restaura_terminal_e_sai);
    signal(SIGTERM, restaura_terminal_e_sai);
#ifdef SIGHUP
    signal(SIGHUP, restaura_terminal_e_sai);
#endif
#ifdef SIGQUIT
    signal(SIGQUIT, restaura_terminal_e_sai);
#endif
}

static void mostra_cursor(int visivel)
{
    cursor_oculto = !visivel;
    ui_cursor(visivel);
}

static int ler_tecla(void)
{
#ifdef _WIN32
    int c = _getch();
    if (c == 0 || c == 224) {
        int c2 = _getch();
        if (c2 == 72) return KEY_UP;
        if (c2 == 80) return KEY_DOWN;
        if (c2 == 75) return KEY_LEFT;
        if (c2 == 77) return KEY_RIGHT;
        return KEY_IGNORADA;
    }
    if (c == TECLA_ESC) return KEY_ESC;
    return c;
#else
    struct termios raw_t;
    int c;
    if (tcgetattr(STDIN_FILENO, &term_original) != 0) {
        /* sem controle do terminal: leitura comum, EOF ainda encerra */
        c = getchar();
        return c == EOF ? KEY_FIM : c;
    }
    raw_t = term_original;
    raw_t.c_lflag &= ~((tcflag_t)(ICANON | ECHO));
    term_raw = 1;
    c = decodifica_tecla(le_byte_terminal, &raw_t);
    tcsetattr(STDIN_FILENO, TCSANOW, &term_original);
    term_raw = 0;
    return c;
#endif
}

int navegar_menu(int selecao, int tecla)
{
    int col, row;

    /* Normaliza WASD */
    if (tecla == 'w' || tecla == 'W') tecla = KEY_UP;
    else if (tecla == 's' || tecla == 'S') tecla = KEY_DOWN;
    else if (tecla == 'a' || tecla == 'A') tecla = KEY_LEFT;
    else if (tecla == 'd' || tecla == 'D') tecla = KEY_RIGHT;

    if (tecla != KEY_UP && tecla != KEY_DOWN &&
        tecla != KEY_LEFT && tecla != KEY_RIGHT)
        return selecao;

    /* Converte seleção para (col, row) */
    if (selecao == OPCAO_HISTORICO) {
        row = 4;
        col = 0; /* coluna padrão ao sair do Histórico */
    } else if (selecao >= 1 && selecao <= 4) {
        col = 0;
        row = selecao - 1;
    } else { /* 5-8 */
        col = 1;
        row = selecao - 5;
    }

    switch (tecla) {
    case KEY_UP:
        if (row == 4) { row = 3; } /* do Histórico vai para linha 3 */
        else if (row == 0) { row = 4; } /* do topo vai para o Histórico */
        else row--;
        break;
    case KEY_DOWN:
        if (row == 4) { row = 0; } /* do Histórico vai para linha 0 */
        else if (row == 3) { row = 4; } /* da última linha vai para o Histórico */
        else row++;
        break;
    case KEY_LEFT:
        if (row < 4) col = 0;
        break;
    case KEY_RIGHT:
        if (row < 4) col = 1;
        break;
    default:
        break;
    }

    /* Converte de volta para seleção */
    if (row == 4) return OPCAO_HISTORICO;
    return col == 0 ? row + 1 : row + 5;
}

int menu_tecla_opcao(int tecla, int selecao)
{
    /* Esc, tecla 0 ou fim da entrada (EOF, Ctrl+D, terminal fechado) saem */
    if (tecla == KEY_ESC || tecla == KEY_FIM || tecla == '0') return OPCAO_SAIR;

    /* Acesso rápido por dígito (1-9) */
    if (tecla >= '1' && tecla < '0' + OPCAO_TOTAL) return tecla - '0';

    /* Enter confirma seleção atual */
    if (tecla == '\r' || tecla == '\n') return selecao;

    return MENU_CONTINUA;
}

void desenha_menu(const Lista *li, int selecao)
{
    static const char *const ROTULOS[OPCAO_TOTAL] = {
        "", "Novo", "Remover", "Listar", "Consultar",
        "Retirar", "Repor", "Reinicializar", "(diferencial)", "Hist\xc3\xb3rico"
    };
    char linhas[6][160];
    const char *ptrs[6];
    const char *sec   = ui_cor(UI_COR_SECUNDARIA);
    const char *lrj   = ui_cor(UI_COR_PRIMARIA);
    const char *bold  = ui_cor(UI_COR_NEGRITO);
    const char *rs    = ui_cor(UI_COR_RESET);
    const char *dim   = ui_cor(UI_COR_DIM);
    const char *bullet = UI_BULLET;
    int i, cols = ui_colunas_terminal();

    for (i = 0; i < 4; i++) {
        int esq_num = i + 1;
        int dir_num = i + 5;
        const char *ce = (selecao == esq_num) ? lrj  : sec;
        const char *be = (selecao == esq_num) ? bold : "";
        const char *cd = (selecao == dir_num) ? lrj  : sec;
        const char *bd = (selecao == dir_num) ? bold : "";
        snprintf(linhas[i], sizeof linhas[i],
                 " %s%s%s%s %s[%d]%s %-11s  %s%s%s%s %s[%d]%s %s",
                 be, ce, bullet, rs,
                 ce, esq_num, rs, ROTULOS[esq_num],
                 bd, cd, bullet, rs,
                 cd, dir_num, rs, ROTULOS[dir_num]);
        ptrs[i] = linhas[i];
    }
    {
        /* [9] Histórico na linha de baixo */
        const char *cs = (selecao == OPCAO_HISTORICO) ? lrj : sec;
        const char *bs = (selecao == OPCAO_HISTORICO) ? bold : "";
        snprintf(linhas[4], sizeof linhas[4], " %s%s%s%s %s[%d]%s %s",
                 bs, cs, bullet, rs, cs, OPCAO_HISTORICO, rs, ROTULOS[OPCAO_HISTORICO]);
    }
    ptrs[4] = linhas[4];

    snprintf(linhas[5], sizeof linhas[5], " %sTipos:%s %-4d %s %sPeças:%s %lld",
             sec, rs, tamanho_lista(li),
             UI_BARRA,
             sec, rs, total_pecas(li));
    ptrs[5] = linhas[5];

    ui_caixa_centralizada(UI_MOLDURA_ARREDONDADA, UI_COR_PRIMARIA, " OhmFlow System ",
                          "Controle de Estoque 8-Bit", ptrs, 6, UI_LARGURA);

    /* Linha de dica centralizada */
    {
        const char *dica =
            "\xe2\x86\x91\xe2\x86\x93\xe2\x86\x90\xe2\x86\x92"
            "/WASD Navegar \xc2\xb7 [1-9] Acesso r\xc3\xa1pido \xc2\xb7 [Enter] Confirmar"
            " \xc2\xb7 [Esc/0] Sair";
        int esq = (cols - (int)ui_largura(dica)) / 2;
        if (esq < 0) esq = 0;
        printf("%*s%s%s%s\n", esq, "", dim, dica, rs);
    }
}

/* Linhas que o bloco do menu ocupa na tela (caixa + dica) */
#define MENU_LINHAS 11

int tela_menu(const Lista *li)
{
    int selecao = 1;

    /* Modo não interativo: fluxo de texto (e2e, CI, testes) */
    if (!ui_interativo()) {
        char buf[TAM_ENTRADA], erro[TAM_ENTRADA + 64];
        char *fim;
        erro[0] = '\0';
        for (;;) {
            long op;
            ui_limpar_tela();
            putchar('\n');
            desenha_menu(li, selecao);
            if (erro[0]) ui_mensagem(UI_ERRO, "%s", erro);
            if (!ui_ler_linha("Op\xc3\xa7\xc3\xa3o: ", buf, sizeof buf)) return OPCAO_SAIR;
            op = strtol(buf, &fim, 10);
            if (buf[0] != '\0' && *fim == '\0' && op == 0) return OPCAO_SAIR;
            if (buf[0] != '\0' && *fim == '\0' && op >= 1 && op < OPCAO_TOTAL)
                return (int)op;
            {
                char citado[40];
                snprintf(citado, sizeof citado, "\"%.24s\"", buf);
                registra(LOG_ERRO, "MENU", citado, "OPÇÃO INVÁLIDA", NULL);
            }
            snprintf(erro, sizeof erro,
                     "Op\xc3\xa7\xc3\xa3o inv\xc3\xa1lida \"%s\". Escolha de 0 a %d.",
                     buf, OPCAO_TOTAL - 1);
        }
    }

    /* Modo interativo: navegação por teclado */
    protege_terminal();
    mostra_cursor(0); /* esconde cursor piscante durante navegação */
    for (;;) {
        int c, op, pad, i;
        ui_limpar_tela();

        /* Centralização vertical: pad linhas em branco acima do menu */
        pad = (ui_linhas_terminal() - MENU_LINHAS) / 2;
        for (i = 0; i < pad; i++) putchar('\n');

        desenha_menu(li, selecao);

        c = ler_tecla();

        /* Esc, EOF, dígito ou Enter encerram o menu com uma opção */
        op = menu_tecla_opcao(c, selecao);
        if (op != MENU_CONTINUA) { mostra_cursor(1); return op; }

        selecao = navegar_menu(selecao, c);
    }
}

/* ---- [3]+[4] novo (unitário ou em massa) ------------------------------- */

/* Contadores da inserção em massa */
typedef struct {
    int inseridos, duplicados, invalidos;
} Lote;

/* Insere um item do lote, conta o resultado e mostra uma linha de feedback */
static void insere_do_lote(Lista *li, Resistor r, Lote *lote)
{
    char txt[TAM_RES];
    int res = insere_lista_ordenada(li, r);
    formata_resistencia(r.resistencia, txt, sizeof txt);
    if (res == LISTA_OK) {
        lote->inseridos++;
        registra(LOG_INFO, "NOVO", txt, "OK", "qtd %d (lote)", r.quantidade);
        ui_mensagem(UI_SUCESSO, "%s inserido (%d un.)", txt, r.quantidade);
    } else if (res == LISTA_DUPLICADO) {
        lote->duplicados++;
        registra(LOG_AVISO, "NOVO", txt, "JÁ CADASTRADO", "(lote)");
        ui_mensagem(UI_AVISO, "%s já cadastrado - ignorado", txt);
    } else {
        lote->invalidos++;
        registra(LOG_ERRO, "NOVO", txt, res == LISTA_ERRO ? "SEM MEMÓRIA" : "VALOR INVÁLIDO",
                 "(lote)");
        ui_mensagem(UI_ERRO, "%s: %s", txt,
                    res == LISTA_ERRO ? "memória insuficiente" : "valor inválido");
    }
}

/* Uma linha "<resistência> <quantidade>" por item até Enter vazio (ou fim da
 * entrada). Duplicados e inválidos não interrompem: entram no resumo final. */
static void inserir_em_massa(Lista *li, Resistor primeiro)
{
    Lote lote = { 0, 0, 0 };
    char buf[TAM_ENTRADA];

    insere_do_lote(li, primeiro, &lote);
    while (ui_ler_linha("Resistência [quantidade]: ", buf, sizeof buf) && buf[0] != '\0') {
        Resistor r;
        int tipo = parse_item(buf, &r);
        if (tipo == ITEM_INVALIDO) {
            lote.invalidos++;
            registra_invalido("NOVO", buf);
            ui_mensagem(UI_ERRO, "Linha inválida: \"%s\"", buf);
            continue;
        }
        /* só a resistência: pede a quantidade (duplicado nem chega a perguntar) */
        if (tipo == ITEM_SO_RESISTENCIA && consulta_lista(li, r.resistencia, NULL) != LISTA_OK) {
            char txt[TAM_RES];
            formata_resistencia(r.resistencia, txt, sizeof txt);
            if (!ler_quantidade("NOVO", txt, "Quantidade : ", 0, &r.quantidade)) {
                lote.invalidos++;
                registra(LOG_AVISO, "NOVO", txt, "SEM QUANTIDADE", "(lote)");
                ui_mensagem(UI_ERRO, "Item sem quantidade ignorado");
                continue;
            }
        }
        insere_do_lote(li, r, &lote);
    }

    registra(LOG_INFO, "NOVO", NULL, "LOTE CONCLUÍDO", "%d ok %s %d dup %s %d inv",
             lote.inseridos, UI_PONTO, lote.duplicados, UI_PONTO, lote.invalidos);
    ui_caixa_feedback(lote.inseridos > 0 ? UI_SUCESSO : UI_AVISO, "Inserção em massa concluída",
                      "Inseridos : %d\nDuplicados: %d\nInválidos : %d",
                      lote.inseridos, lote.duplicados, lote.invalidos);
    voltar_ao_menu();
}

void tela_inserir(Lista *li)
{
    Resistor r;
    char buf[TAM_ENTRADA], txt[TAM_RES];
    int t, tipo = ITEM_INVALIDO, res;

    abre_tela("Novo resistor", ALTURA_NOVO);
    ui_dica("Formatos aceitos: 220 %s 4k7 %s 4.7k %s 1M %s 470m", UI_PONTO, UI_PONTO,
            UI_PONTO, UI_PONTO);
    ui_dica("Em massa: <resistência> <quantidade> por linha (ex.: 4k7 50)");
    ui_dica("(Enter vazio cancela; em massa, encerra a lista)");
    putchar('\n');

    /* A primeira linha decide o modo: só a resistência -> um item, com a
     * quantidade perguntada em seguida; resistência e quantidade -> em massa */
    for (t = 0; t < TENTATIVAS && tipo == ITEM_INVALIDO; t++) {
        if (!ui_ler_linha("Resistência: ", buf, sizeof buf) || buf[0] == '\0') break;
        tipo = parse_item(buf, &r);
        if (tipo == ITEM_INVALIDO) {
            registra_invalido("NOVO", buf);
            ui_mensagem(UI_ERRO, MSG_VALOR_INVALIDO);
        }
    }
    if (tipo == ITEM_INVALIDO) {
        feedback_cancelado("NOVO", NULL);
        return;
    }
    if (tipo == ITEM_COMPLETO) {
        inserir_em_massa(li, r);
        return;
    }

    formata_resistencia(r.resistencia, txt, sizeof txt);
    if (consulta_lista(li, r.resistencia, NULL) == LISTA_OK) {
        registra(LOG_AVISO, "NOVO", txt, "JÁ CADASTRADO", NULL);
        ui_caixa_feedback(UI_ERRO, "Resistor já cadastrado",
                          "%s já cadastrado - use [6] Repor", txt);
        voltar_ao_menu();
        return;
    }
    if (!ler_quantidade("NOVO", txt, "Quantidade : ", 0, &r.quantidade)) {
        feedback_cancelado("NOVO", txt);
        return;
    }

    res = insere_lista_ordenada(li, r);
    if (res == LISTA_OK)
        registra(LOG_INFO, "NOVO", txt, "OK", "qtd %d", r.quantidade);
    else
        registra(LOG_ERRO, "NOVO", txt,
                 res == LISTA_ERRO ? "SEM MEMÓRIA" : "VALOR INVÁLIDO", "qtd %d", r.quantidade);
    if (res == LISTA_OK)
        ui_caixa_feedback(UI_SUCESSO, "Resistor inserido",
                          "Resistor %s inserido (%d un.)", txt, r.quantidade);
    else
        ui_caixa_feedback(UI_ERRO, "Falha ao inserir",
                          res == LISTA_ERRO ? "Memória insuficiente" : "Valor inválido");
    voltar_ao_menu();
}

/* ---- [3]+[4] remover --------------------------------------------------- */

void tela_remover(Lista *li)
{
    double ohms;
    char txt[TAM_RES];
    int qtd;

    abre_tela("Remover tipo de resistor", ALTURA_REMOVER);
    ui_dica("(Enter vazio cancela)");
    putchar('\n');
    if (!ler_cadastrado("REMOVER", li, &ohms, txt, &qtd)) return;

    if (!ui_confirmar("Remover %s (%d %s) do estoque?", txt, qtd,
                      plural(qtd, "unidade", "unidades"))) {
        feedback_cancelado("REMOVER", txt);
        return;
    }
    if (remove_lista(li, ohms) == LISTA_OK) {
        registra(LOG_INFO, "REMOVER", txt, "OK", "qtd %d", qtd);
        ui_caixa_feedback(UI_SUCESSO, "Resistor removido", "%s removido do estoque", txt);
    } else {
        registra(LOG_AVISO, "REMOVER", txt, "NÃO ENCONTRADO", NULL);
        ui_caixa_feedback(UI_ERRO, "Falha ao remover", "%s não está no estoque", txt);
    }
    voltar_ao_menu();
}

/* ---- [3] listar -------------------------------------------------------- */

static void linha_estoque(const Resistor *r, void *ctx)
{
    int *indice = ctx;
    char txt[TAM_RES], c_idx[16], c_res[96], c_qtd[48];
    const char *celulas[3];
    char *espaco;

    formata_resistencia(r->resistencia, txt, sizeof txt);
    espaco = strrchr(txt, ' '); /* separa número e unidade p/ alinhar */
    if (espaco != NULL) *espaco = '\0';

    snprintf(c_idx, sizeof c_idx, "%d", ++*indice);
    /* valor no texto padrão: só a moldura da tabela é colorida */
    snprintf(c_res, sizeof c_res, "%7s %s", txt, espaco ? espaco + 1 : "");
    snprintf(c_qtd, sizeof c_qtd, "%s%d%s", r->quantidade == 0 ? ui_cor(UI_COR_VERMELHO) : "",
             r->quantidade, ui_cor(UI_COR_RESET));

    celulas[0] = c_idx;
    celulas[1] = c_res;
    celulas[2] = c_qtd;
    ui_tabela_linha(celulas, TAB_LARG, TAB_DIR, 3);
}

void tela_listar(const Lista *li)
{
    char cab[3][48];
    const char *cabecalho[3];
    const char *ng = ui_cor(UI_COR_NEGRITO), *rs = ui_cor(UI_COR_RESET);
    int indice = 0, tipos = tamanho_lista(li);
    long long pecas = total_pecas(li);

    abre_tela("Estoque atual", ALTURA_LISTAR_BASE + tipos);
    registra(LOG_INFO, "LISTAR", NULL, "OK", "%d %s %s %lld %s", tipos,
             plural(tipos, "tipo", "tipos"), UI_PONTO, pecas, plural(pecas, "peça", "peças"));
    if (lista_vazia(li)) {
        ui_mensagem(UI_INFO, "Estoque vazio. Use [1] Novo.");
        voltar_ao_menu();
        return;
    }
    snprintf(cab[0], sizeof cab[0], "%s#%s", ng, rs);
    snprintf(cab[1], sizeof cab[1], "%sResistência%s", ng, rs);
    snprintf(cab[2], sizeof cab[2], "%sQuantidade%s", ng, rs);
    cabecalho[0] = cab[0];
    cabecalho[1] = cab[1];
    cabecalho[2] = cab[2];

    ui_tabela_borda(UI_TAB_TOPO, TAB_LARG, 3);
    ui_tabela_linha(cabecalho, TAB_LARG, TAB_DIR, 3);
    ui_tabela_borda(UI_TAB_MEIO, TAB_LARG, 3);
    percorre_lista(li, linha_estoque, &indice);
    ui_tabela_borda(UI_TAB_BASE, TAB_LARG, 3);
    ui_dica("%d %s %s %lld %s", tipos, plural(tipos, "tipo", "tipos"), UI_PONTO,
            pecas, plural(pecas, "peça", "peças"));
    voltar_ao_menu();
}

/* ---- [3] consultar ----------------------------------------------------- */

void tela_consultar(Lista *li)
{
    double ohms;
    char txt[TAM_RES];
    int qtd;

    abre_tela("Consultar estoque", ALTURA_CONSULTAR);
    ui_dica("(Enter vazio cancela)");
    putchar('\n');
    if (!ler_resistencia("CONSULTAR", &ohms)) {
        feedback_cancelado("CONSULTAR", NULL);
        return;
    }
    formata_resistencia(ohms, txt, sizeof txt);
    if (consulta_lista(li, ohms, &qtd) == LISTA_OK) {
        char linha[96];
        const char *linhas[1];
        registra(LOG_INFO, "CONSULTAR", txt, "OK", "qtd %d", qtd);
        snprintf(linha, sizeof linha, "%s%s%s %s %s%d%s %s", ui_cor(UI_COR_NEGRITO), txt,
                 ui_cor(UI_COR_RESET), UI_SETA,
                 qtd == 0 ? ui_cor(UI_COR_VERMELHO) : ui_cor(UI_COR_VERDE), qtd,
                 ui_cor(UI_COR_RESET), plural(qtd, "unidade", "unidades"));
        linhas[0] = linha;
        putchar('\n');
        ui_caixa(UI_MOLDURA_SIMPLES, UI_COR_BORDA, NULL, NULL, linhas, 1, 26);
    } else {
        registra(LOG_AVISO, "CONSULTAR", txt, "NÃO ENCONTRADO", NULL);
        ui_mensagem(UI_ERRO, "%s não está no estoque", txt);
    }
    voltar_ao_menu();
}

/* ---- [3]+[4] retirar --------------------------------------------------- */

void tela_retirar(Lista *li)
{
    double ohms;
    char txt[TAM_RES];
    int atual, qtd, res;

    abre_tela("Retirar do estoque", ALTURA_MOVIMENTO);
    ui_dica("(Enter vazio cancela)");
    putchar('\n');
    if (!ler_cadastrado("RETIRAR", li, &ohms, txt, &atual)) return;
    ui_dica("Em estoque: %d %s", atual, plural(atual, "unidade", "unidades"));
    if (!ler_quantidade("RETIRAR", txt, "Quantidade a retirar: ", 1, &qtd)) {
        feedback_cancelado("RETIRAR", txt);
        return;
    }

    res = retira_estoque(li, ohms, qtd);
    if (res == LISTA_OK && atual - qtd == 0)
        registra(LOG_AVISO, "RETIRAR", txt, "OK - ESTOQUE ZERADO", "-%d (%d%s0)", qtd, atual,
                 UI_SETA);
    else if (res == LISTA_OK)
        registra(LOG_INFO, "RETIRAR", txt, "OK", "-%d (%d%s%d)", qtd, atual, UI_SETA,
                 atual - qtd);
    else if (res == LISTA_SEM_ESTOQUE)
        registra(LOG_AVISO, "RETIRAR", txt, "SEM ESTOQUE", "-%d (tem %d)", qtd, atual);
    else
        registra(LOG_ERRO, "RETIRAR", txt, "QUANTIDADE INVÁLIDA", "-%d", qtd);
    if (res == LISTA_OK && atual - qtd == 0)
        ui_caixa_feedback(UI_AVISO, "Retirada concluída - estoque zerado",
                          "%s: %d %s 0 unidades", txt, atual, UI_SETA);
    else if (res == LISTA_OK)
        ui_caixa_feedback(UI_SUCESSO, "Retirada concluída", "%s: %d %s %d %s", txt,
                          atual, UI_SETA, atual - qtd, plural(atual - qtd, "unidade", "unidades"));
    else if (res == LISTA_SEM_ESTOQUE)
        ui_caixa_feedback(UI_ERRO, "Estoque insuficiente", "%s tem %d, pedido %d", txt, atual, qtd);
    else
        ui_caixa_feedback(UI_ERRO, "Falha ao retirar", "Quantidade inválida");
    voltar_ao_menu();
}

/* ---- [3]+[4] repor ----------------------------------------------------- */

void tela_repor(Lista *li)
{
    double ohms;
    char txt[TAM_RES];
    int atual, qtd, res;

    abre_tela("Repor estoque", ALTURA_MOVIMENTO);
    ui_dica("(Enter vazio cancela)");
    putchar('\n');
    if (!ler_cadastrado("REPOR", li, &ohms, txt, &atual)) return;
    ui_dica("Em estoque: %d %s", atual, plural(atual, "unidade", "unidades"));
    if (!ler_quantidade("REPOR", txt, "Quantidade a repor: ", 1, &qtd)) {
        feedback_cancelado("REPOR", txt);
        return;
    }

    res = repor_estoque(li, ohms, qtd);
    if (res == LISTA_OK)
        registra(LOG_INFO, "REPOR", txt, "OK", "+%d (%d%s%d)", qtd, atual, UI_SETA, atual + qtd);
    else
        registra(LOG_ERRO, "REPOR", txt, "LIMITE EXCEDIDO", "+%d (tem %d)", qtd, atual);
    if (res == LISTA_OK)
        ui_caixa_feedback(UI_SUCESSO, "Reposição concluída",
                          "%s: %d %s %d %s", txt, atual, UI_SETA, atual + qtd,
                          plural(atual + qtd, "unidade", "unidades"));
    else
        ui_caixa_feedback(UI_ERRO, "Quantidade inválida",
                          "O total excederia o limite de %d unidades", INT_MAX);
    voltar_ao_menu();
}

/* ---- [3]+[4] reinicializar --------------------------------------------- */

void tela_reinicializar(Lista *li)
{
    int tipos = tamanho_lista(li);
    long long pecas = total_pecas(li);

    abre_tela("Inicializar / reinicializar", ALTURA_REINICIALIZAR);
    putchar('\n');
    if (lista_vazia(li)) {
        reinicializa_lista(li);
        registra(LOG_INFO, "REINICIALIZAR", NULL, "OK", "0 tipos");
        ui_caixa_feedback(UI_INFO, "Estoque inicializado", "Lista vazia e pronta para uso");
        voltar_ao_menu();
        return;
    }
    ui_caixa_feedback(UI_AVISO, "Tem certeza disso, você está apagando sua base de dados.",
                      NULL);
    if (!ui_confirmar("Apagar TODOS os %d %s (%lld %s)?", tipos, plural(tipos, "tipo", "tipos"),
                      pecas, plural(pecas, "peça", "peças"))) {
        feedback_cancelado("REINICIALIZAR", NULL);
        return;
    }
    reinicializa_lista(li);
    registra(LOG_INFO, "REINICIALIZAR", NULL, "OK", "%d %s (%lld %s)", tipos,
             plural(tipos, "tipo", "tipos"), pecas, plural(pecas, "peça", "peças"));
    ui_caixa_feedback(UI_SUCESSO, "Estoque reinicializado", "%d %s removidos (%lld %s)", tipos,
                      plural(tipos, "tipo", "tipos"), pecas,
                      plural(pecas, "peça", "peças"));
    voltar_ao_menu();
}

/* ---- [8] diferencial (a definir) --------------------------------------- */

void tela_diferencial(void)
{
    abre_tela("Diferencial", ALTURA_DIFERENCIAL);
    registra(LOG_INFO, "DIFERENCIAL", NULL, "EM BREVE", NULL);
    ui_caixa_feedback(UI_INFO, "Em breve", "Funcionalidade diferencial em desenvolvimento");
    voltar_ao_menu();
}

/* ---- [3] histórico ---------------------------------------------------- */

/* Copia s em buf com no máximo `largura` colunas visíveis (UTF-8); se cortar,
 * termina com reticências */
static void corta(const char *s, char *buf, size_t n, int largura)
{
    size_t i = 0;
    int w = 0, cabe = (int)ui_largura(s) <= largura;
    for (; s[i] != '\0' && i + 4 < n; i++) {
        if (((unsigned char)s[i] & 0xC0) != 0x80) { /* início de caractere */
            if (w == (cabe ? largura : largura - 1)) break;
            w++;
        }
        buf[i] = s[i];
    }
    buf[i] = '\0';
    if (!cabe) strcat(buf, "\xE2\x80\xA6"); /* … */
}

static int cor_do_nivel(const char *nivel)
{
    if (strcmp(nivel, "ERRO") == 0) return UI_COR_VERMELHO;
    if (strcmp(nivel, "AVISO") == 0) return UI_COR_AMARELO;
    return UI_COR_VERDE;
}

void tela_historico(void)
{
    static const char *const TITULOS[5] = {
        "Quando", "Ação", "Resistor", "Detalhe", "Resultado"
    };
    LogEntrada v[HISTORICO_MAX];
    char cab[5][48], cel[5][128], quando[16];
    const char *ptrs[5];
    const char *rs = ui_cor(UI_COR_RESET);
    int n = log_ultimas(v, HISTORICO_MAX), i, c;

    abre_tela("Histórico", ALTURA_HISTORICO_BASE + n);
    if (n == 0) {
        ui_mensagem(UI_INFO, "Nenhuma ação registrada nos últimos %d dias.",
                    LOG_DIAS_HISTORICO);
        registra(LOG_INFO, "HISTÓRICO", NULL, "VAZIO", NULL);
        voltar_ao_menu();
        return;
    }

    for (c = 0; c < 5; c++) {
        snprintf(cab[c], sizeof cab[c], "%s%s%s", ui_cor(UI_COR_NEGRITO), TITULOS[c], rs);
        ptrs[c] = cab[c];
    }
    ui_tabela_borda(UI_TAB_TOPO, HIST_LARG, 5);
    ui_tabela_linha(ptrs, HIST_LARG, NULL, 5);
    ui_tabela_borda(UI_TAB_MEIO, HIST_LARG, 5);
    for (i = n - 1; i >= 0; i--) { /* mais recente primeiro */
        const LogEntrada *e = &v[i];
        char texto[96];
        /* "27/10/2026 14:03:20" -> "27/10 14:03" */
        snprintf(quando, sizeof quando, "%.5s %.5s", e->data, e->data + 11);
        corta(quando, cel[0], sizeof cel[0], HIST_LARG[0]);
        corta(e->acao, cel[1], sizeof cel[1], HIST_LARG[1]);
        corta(e->alvo, cel[2], sizeof cel[2], HIST_LARG[2]);
        corta(e->detalhe, cel[3], sizeof cel[3], HIST_LARG[3]);
        corta(e->resultado, texto, sizeof texto, HIST_LARG[4]);
        snprintf(cel[4], sizeof cel[4], "%s%s%s", ui_cor(cor_do_nivel(e->nivel)), texto, rs);
        for (c = 0; c < 5; c++) ptrs[c] = cel[c];
        ui_tabela_linha(ptrs, HIST_LARG, NULL, 5);
    }
    ui_tabela_borda(UI_TAB_BASE, HIST_LARG, 5);
    ui_dica("%d %s mais %s (últimos %d dias)", n,
            plural(n, "ação", "ações"),
            plural(n, "recente", "recentes"), LOG_DIAS_HISTORICO);
    /* registrado depois da leitura, para a tela não listar a si mesma */
    registra(LOG_INFO, "HISTÓRICO", NULL, "OK", "%d %s", n,
             plural(n, "ação", "ações"));
    voltar_ao_menu();
}

/* ---- [5] encerramento -------------------------------------------------- */

void tela_encerramento(const Lista *li)
{
    char resumo[96];
    const char *linhas[2];
    int tipos = tamanho_lista(li);
    long long pecas = total_pecas(li);

    snprintf(resumo, sizeof resumo, "Estoque final: %d %s %s %lld %s", tipos,
             plural(tipos, "tipo", "tipos"), UI_PONTO, pecas,
             plural(pecas, "peça", "peças"));
    linhas[0] = "Obrigado por usar o OhmFlow!";
    linhas[1] = resumo;
    ui_limpar_tela();
    putchar('\n');
    ui_espaco_vertical(ALTURA_ENCERRAMENTO);
    ui_caixa(UI_MOLDURA_DUPLA, UI_COR_BORDA, NULL, NULL, linhas, 2, UI_LARGURA);
}
