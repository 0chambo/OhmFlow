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

#define TENTATIVAS  3
#define TAM_ENTRADA 128
#define TAM_RES     32

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

static void feedback_cancelado(void)
{
    ui_caixa_feedback(UI_AVISO, "Operação cancelada", NULL);
    voltar_ao_menu();
}

static void abre_tela(const char *titulo)
{
    ui_limpar_tela();
    ui_caixa(UI_MOLDURA_ARREDONDADA, UI_COR_BORDA, titulo, NULL, NULL, 0, UI_LARGURA);
}

/* Lê uma resistência; 0 se cancelado (vazio/EOF) ou após TENTATIVAS inválidas */
static int ler_resistencia(double *ohms)
{
    char buf[TAM_ENTRADA];
    int t;
    for (t = 0; t < TENTATIVAS; t++) {
        if (!ui_ler_linha("Resistência: ", buf, sizeof buf) || buf[0] == '\0')
            return 0;
        if (parse_resistencia(buf, ohms)) return 1;
        ui_mensagem(UI_ERRO, "Valor inválido. Exemplos: 220, 4k7, 4.7k, 1M, 470m "
                    "(faixa 1 m" SIMBOLO_OHM " a 1000 M" SIMBOLO_OHM ")");
    }
    return 0;
}

/* Lê um inteiro em [minimo, INT_MAX]; 0 se cancelado */
static int ler_quantidade(const char *prompt, int minimo, int *qtd)
{
    char buf[TAM_ENTRADA];
    char *fim;
    int t;
    for (t = 0; t < TENTATIVAS; t++) {
        long long v; /* long long: maior que int em qualquer plataforma */
        if (!ui_ler_linha(prompt, buf, sizeof buf) || buf[0] == '\0') return 0;
        errno = 0;
        v = strtoll(buf, &fim, 10);
        if (errno == 0 && *fim == '\0' && v >= minimo && v <= INT_MAX) {
            *qtd = (int)v;
            return 1;
        }
        ui_mensagem(UI_ERRO, "Quantidade inválida: informe um inteiro de %d a %d.",
                    minimo, INT_MAX);
    }
    return 0;
}

/* Lê a resistência e garante que ela está cadastrada; preenche texto e qtd */
static int ler_cadastrado(Lista *li, double *ohms, char *txt, int *qtd)
{
    if (!ler_resistencia(ohms)) {
        feedback_cancelado();
        return 0;
    }
    formata_resistencia(*ohms, txt, TAM_RES);
    if (consulta_lista(li, *ohms, qtd) != LISTA_OK) {
        ui_caixa_feedback(UI_ERRO, "Resistor não encontrado",
                          "%s não está no estoque", txt);
        voltar_ao_menu();
        return 0;
    }
    return 1;
}

static int cor_grandeza(int g)
{
    switch (g) {
    case GRANDEZA_MILI:    return UI_COR_MAGENTA;
    case GRANDEZA_UNIDADE: return UI_COR_BRANCO;
    case GRANDEZA_KILO:    return UI_COR_CIANO;
    default:               return UI_COR_AZUL;
    }
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
    const char *laranja = ui_cor(UI_COR_LARANJA);
    const char *neon    = ui_cor(UI_COR_VERDE_NEON);
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
        printf("%*s%s\xc2\xbb%s %s%s%s\n", m_esq, "", neon, rs, dim, MSG, rs);
    }

    /* Aguarda Enter sem imprimir texto extra */
    if (ui_interativo()) {
        char lixo[8];
        fflush(stdout);
        while (fgets(lixo, sizeof lixo, stdin) && !strchr(lixo, '\n')) ;
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
    if (c != '[' && c != 'O') return KEY_IGNORADA; /* ESC isolado ou Alt+tecla */
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
    if (selecao == 0) {
        row = 4;
        col = 0; /* coluna padrão ao sair de Sair */
    } else if (selecao >= 1 && selecao <= 4) {
        col = 0;
        row = selecao - 1;
    } else { /* 5-8 */
        col = 1;
        row = selecao - 5;
    }

    switch (tecla) {
    case KEY_UP:
        if (row == 4) { row = 3; } /* de Sair vai para linha 3 */
        else if (row == 0) { row = 4; } /* do topo vai para Sair */
        else row--;
        break;
    case KEY_DOWN:
        if (row == 4) { row = 0; } /* de Sair vai para linha 0 */
        else if (row == 3) { row = 4; } /* da última linha vai para Sair */
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
    if (row == 4) return 0; /* Sair */
    return col == 0 ? row + 1 : row + 5;
}

void desenha_menu(const Lista *li, int selecao)
{
    static const char *const ROTULOS[OPCAO_TOTAL] = {
        "Sair", "Inserir", "Remover", "Listar", "Consultar",
        "Retirar", "Repor", "Reinicializar", "(diferencial)"
    };
    char linhas[6][160];
    const char *ptrs[6];
    const char *neon  = ui_cor(UI_COR_VERDE_NEON);
    const char *lrj   = ui_cor(UI_COR_LARANJA);
    const char *bold  = ui_cor(UI_COR_NEGRITO);
    const char *rs    = ui_cor(UI_COR_RESET);
    const char *dim   = ui_cor(UI_COR_DIM);
    const char *bullet = UI_BULLET;
    int i, cols = ui_colunas_terminal();

    for (i = 0; i < 4; i++) {
        int esq_num = i + 1;
        int dir_num = i + 5;
        const char *ce = (selecao == esq_num) ? lrj  : neon;
        const char *be = (selecao == esq_num) ? bold : "";
        const char *cd = (selecao == dir_num) ? lrj  : neon;
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
        const char *cs = (selecao == 0) ? lrj : dim;
        const char *bs = (selecao == 0) ? bold : "";
        snprintf(linhas[4], sizeof linhas[4], " %s%s%s%s %s[0]%s %s",
                 bs, cs, bullet, rs, cs, rs, ROTULOS[OPCAO_SAIR]);
    }
    ptrs[4] = linhas[4];

    snprintf(linhas[5], sizeof linhas[5], " %sTipos:%s %-4d %s %sPeças:%s %lld",
             ui_cor(UI_COR_ROXO_NEON), rs, tamanho_lista(li),
             UI_BARRA,
             ui_cor(UI_COR_ROXO_NEON), rs, total_pecas(li));
    ptrs[5] = linhas[5];

    ui_caixa_centralizada(UI_MOLDURA_ARREDONDADA, UI_COR_LARANJA, " OhmFlow System ",
                          "Controle de Estoque 8-Bit", ptrs, 6, UI_LARGURA);

    /* Linha de dica centralizada */
    {
        const char *dica =
            "\xe2\x86\x91\xe2\x86\x93\xe2\x86\x90\xe2\x86\x92"
            "/WASD Navegar  \xc2\xb7  [0-8] Acesso r\xc3\xa1pido  \xc2\xb7  [Enter] Confirmar";
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
            if (buf[0] != '\0' && *fim == '\0' && op >= 0 && op < OPCAO_TOTAL)
                return (int)op;
            snprintf(erro, sizeof erro,
                     "Op\xc3\xa7\xc3\xa3o inv\xc3\xa1lida \"%s\". Escolha de 0 a %d.",
                     buf, OPCAO_TOTAL - 1);
        }
    }

    /* Modo interativo: navegação por teclado */
    protege_terminal();
    mostra_cursor(0); /* esconde cursor piscante durante navegação */
    for (;;) {
        int c, pad, i;
        ui_limpar_tela();

        /* Centralização vertical: pad linhas em branco acima do menu */
        pad = (ui_linhas_terminal() - MENU_LINHAS) / 2;
        for (i = 0; i < pad; i++) putchar('\n');

        desenha_menu(li, selecao);

        c = ler_tecla();

        /* Fim da entrada (EOF, Ctrl+D, terminal fechado) encerra como Sair */
        if (c == KEY_FIM) { mostra_cursor(1); return OPCAO_SAIR; }

        /* Acesso rápido por dígito */
        if (c >= '0' && c <= '8') { mostra_cursor(1); return c - '0'; }

        /* Enter confirma seleção atual */
        if (c == '\r' || c == '\n') { mostra_cursor(1); return selecao; }

        selecao = navegar_menu(selecao, c);
    }
}

/* ---- [3]+[4] inserir --------------------------------------------------- */

void tela_inserir(Lista *li)
{
    Resistor r;
    char txt[TAM_RES];
    int res;

    abre_tela("Inserir novo resistor");
    ui_dica("Formatos aceitos: 220 %s 4k7 %s 4.7k %s 1M %s 470m", UI_PONTO, UI_PONTO,
            UI_PONTO, UI_PONTO);
    ui_dica("(Enter vazio cancela)");
    putchar('\n');

    if (!ler_resistencia(&r.resistencia)) {
        feedback_cancelado();
        return;
    }
    formata_resistencia(r.resistencia, txt, sizeof txt);
    if (consulta_lista(li, r.resistencia, NULL) == LISTA_OK) {
        ui_caixa_feedback(UI_ERRO, "Resistor já cadastrado",
                          "%s já cadastrado - use [6] Repor", txt);
        voltar_ao_menu();
        return;
    }
    if (!ler_quantidade("Quantidade : ", 0, &r.quantidade)) {
        feedback_cancelado();
        return;
    }

    res = insere_lista_ordenada(li, r);
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

    abre_tela("Remover tipo de resistor");
    ui_dica("(Enter vazio cancela)");
    putchar('\n');
    if (!ler_cadastrado(li, &ohms, txt, &qtd)) return;

    if (!ui_confirmar("Remover %s (%d %s) do estoque?", txt, qtd,
                      plural(qtd, "unidade", "unidades"))) {
        feedback_cancelado();
        return;
    }
    if (remove_lista(li, ohms) == LISTA_OK)
        ui_caixa_feedback(UI_SUCESSO, "Resistor removido", "%s removido do estoque", txt);
    else
        ui_caixa_feedback(UI_ERRO, "Falha ao remover", "%s não está no estoque", txt);
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
    snprintf(c_res, sizeof c_res, "%s%7s %s%s", ui_cor(cor_grandeza(grandeza_resistencia(r->resistencia))),
             txt, espaco ? espaco + 1 : "", ui_cor(UI_COR_RESET));
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

    abre_tela("Estoque atual");
    if (lista_vazia(li)) {
        ui_mensagem(UI_INFO, "Estoque vazio. Use [1] Inserir.");
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

    abre_tela("Consultar estoque");
    ui_dica("(Enter vazio cancela)");
    putchar('\n');
    if (!ler_resistencia(&ohms)) {
        feedback_cancelado();
        return;
    }
    formata_resistencia(ohms, txt, sizeof txt);
    if (consulta_lista(li, ohms, &qtd) == LISTA_OK) {
        char linha[96];
        const char *linhas[1];
        snprintf(linha, sizeof linha, "%s%s%s %s %s%d%s %s", ui_cor(UI_COR_NEGRITO), txt,
                 ui_cor(UI_COR_RESET), UI_SETA,
                 qtd == 0 ? ui_cor(UI_COR_VERMELHO) : ui_cor(UI_COR_VERDE), qtd,
                 ui_cor(UI_COR_RESET), plural(qtd, "unidade", "unidades"));
        linhas[0] = linha;
        putchar('\n');
        ui_caixa(UI_MOLDURA_SIMPLES, UI_COR_BORDA, NULL, NULL, linhas, 1, 26);
    } else {
        ui_mensagem(UI_ERRO, "%s não está no estoque", txt);
    }
    /* a lista ordenada para no primeiro nó >= valor procurado */
    {
        int passos = lista_passos(li);
        ui_dica("Busca: %d %s", passos, plural(passos, "nó visitado", "nós visitados"));
    }
    voltar_ao_menu();
}

/* ---- [3]+[4] retirar --------------------------------------------------- */

void tela_retirar(Lista *li)
{
    double ohms;
    char txt[TAM_RES];
    int atual, qtd, res;

    abre_tela("Retirar do estoque");
    ui_dica("(Enter vazio cancela)");
    putchar('\n');
    if (!ler_cadastrado(li, &ohms, txt, &atual)) return;
    ui_dica("Em estoque: %d %s", atual, plural(atual, "unidade", "unidades"));
    if (!ler_quantidade("Quantidade a retirar: ", 1, &qtd)) {
        feedback_cancelado();
        return;
    }

    res = retira_estoque(li, ohms, qtd);
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

    abre_tela("Repor estoque");
    ui_dica("(Enter vazio cancela)");
    putchar('\n');
    if (!ler_cadastrado(li, &ohms, txt, &atual)) return;
    ui_dica("Em estoque: %d %s", atual, plural(atual, "unidade", "unidades"));
    if (!ler_quantidade("Quantidade a repor: ", 1, &qtd)) {
        feedback_cancelado();
        return;
    }

    res = repor_estoque(li, ohms, qtd);
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

    abre_tela("Inicializar / reinicializar");
    putchar('\n');
    if (lista_vazia(li)) {
        reinicializa_lista(li);
        ui_caixa_feedback(UI_INFO, "Estoque inicializado", "Lista vazia e pronta para uso");
        voltar_ao_menu();
        return;
    }
    if (!ui_confirmar("Apagar TODOS os %d %s (%lld %s)?", tipos, plural(tipos, "tipo", "tipos"),
                      pecas, plural(pecas, "peça", "peças"))) {
        feedback_cancelado();
        return;
    }
    reinicializa_lista(li);
    ui_caixa_feedback(UI_SUCESSO, "Estoque reinicializado", "%d %s removidos (%lld %s)", tipos,
                      plural(tipos, "tipo", "tipos"), pecas,
                      plural(pecas, "peça", "peças"));
    voltar_ao_menu();
}

/* ---- [8] diferencial (a definir) --------------------------------------- */

void tela_diferencial(void)
{
    abre_tela("Diferencial");
    ui_caixa_feedback(UI_INFO, "Em breve", "Funcionalidade diferencial em desenvolvimento");
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
    ui_caixa(UI_MOLDURA_DUPLA, UI_COR_BORDA, NULL, NULL, linhas, 2, UI_LARGURA);
}
