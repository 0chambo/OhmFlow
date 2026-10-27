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
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#  include <conio.h>
#else
#  include <termios.h>
#  include <unistd.h>
#endif
#include "telas.h"
#include "ui.h"
#include "formatacao.h"
#include "log.h"
#include "resistor.h"

#define TENTATIVAS  3
#define TAM_ENTRADA 128
#define TAM_RES     32

/* Altura aproximada de cada tela (linhas), para a centralização vertical:
 * título (3) + dicas + perguntas + caixa de feedback (5) + pausa (2) */
#define ALTURA_NOVO           17 /* + RESISTOR_LINHAS com o desenho */
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

/* Larguras das colunas da tabela de estoque: #, Resistência, Quantidade,
 * Estoque (barra + porcentagem do total de peças) */
#define TAB_COLUNAS  4
#define BARRA_LISTAR 10
#define BARRA_CONSULTAR 16
static const int TAB_LARG[] = { 2, 12, 10, BARRA_LISTAR + 5 };
static const int TAB_DIR[]  = { 1, 0, 1, 0 };

/* Checkpoint da sessão (telas_definir_checkpoint); NULL = não registra */
static Checkpoint *checkpoint = NULL;

void telas_definir_checkpoint(Checkpoint *ck) { checkpoint = ck; }

/* Desenho do resistor na tela Novo (telas_definir_resistor); -1 = automático */
static int modo_resistor = -1;

void telas_definir_resistor(int modo) { modo_resistor = modo; }

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

/* Título de tela com tracking ("E S T O Q U E   A T U A L") no terminal,
 * se couber na caixa; sem terminal (e2e, testes) fica o texto original */
static const char *titulo_da_tela(const char *titulo, char *buf, size_t n)
{
    if (!ui_interativo()) return titulo;
    ui_espacar(titulo, buf, n, UI_LARGURA - 5);
    return buf;
}

/* Limpa a tela e desenha o título. `altura` é a estimativa de linhas da tela
 * inteira (título, perguntas, feedback e pausa), usada para centralizá-la na
 * vertical quando o modo centralizado está ligado */
static void abre_tela(const char *titulo, int altura)
{
    char t[128];
    ui_limpar_tela();
    ui_espaco_vertical(altura);
    ui_caixa(UI_MOLDURA_ARREDONDADA, UI_COR_BORDA, titulo_da_tela(titulo, t, sizeof t),
             NULL, NULL, 0, UI_LARGURA);
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

/* "OHMFLOW" na fonte figlet ANSI Shadow (6 x 63): █ é o corpo da letra e o
 * traço duplo (╔═╗║╚╝) é a sombra deslocada para baixo/direita */
static const char *const BANNER[BANNER_LINHAS] = {
    " \xE2\x96\x88\xE2\x96\x88\xE2\x96\x88\xE2\x96\x88\xE2\x96\x88"
    "\xE2\x96\x88\xE2\x95\x97 \xE2\x96\x88\xE2\x96\x88\xE2\x95\x97  "
    "\xE2\x96\x88\xE2\x96\x88\xE2\x95\x97\xE2\x96\x88\xE2\x96\x88\xE2\x96\x88"
    "\xE2\x95\x97   \xE2\x96\x88\xE2\x96\x88\xE2\x96\x88\xE2\x95\x97"
    "\xE2\x96\x88\xE2\x96\x88\xE2\x96\x88\xE2\x96\x88\xE2\x96\x88\xE2\x96\x88"
    "\xE2\x96\x88\xE2\x95\x97\xE2\x96\x88\xE2\x96\x88\xE2\x95\x97      "
    "\xE2\x96\x88\xE2\x96\x88\xE2\x96\x88\xE2\x96\x88\xE2\x96\x88\xE2\x96\x88"
    "\xE2\x95\x97 \xE2\x96\x88\xE2\x96\x88\xE2\x95\x97    \xE2\x96\x88"
    "\xE2\x96\x88\xE2\x95\x97",
    "\xE2\x96\x88\xE2\x96\x88\xE2\x95\x94\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90"
    "\xE2\x96\x88\xE2\x96\x88\xE2\x95\x97\xE2\x96\x88\xE2\x96\x88\xE2\x95\x91"
    "  \xE2\x96\x88\xE2\x96\x88\xE2\x95\x91\xE2\x96\x88\xE2\x96\x88"
    "\xE2\x96\x88\xE2\x96\x88\xE2\x95\x97 \xE2\x96\x88\xE2\x96\x88"
    "\xE2\x96\x88\xE2\x96\x88\xE2\x95\x91\xE2\x96\x88\xE2\x96\x88\xE2\x95\x94"
    "\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x9D\xE2\x96\x88"
    "\xE2\x96\x88\xE2\x95\x91     \xE2\x96\x88\xE2\x96\x88\xE2\x95\x94"
    "\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x96\x88\xE2\x96\x88\xE2\x95\x97"
    "\xE2\x96\x88\xE2\x96\x88\xE2\x95\x91    \xE2\x96\x88\xE2\x96\x88"
    "\xE2\x95\x91",
    "\xE2\x96\x88\xE2\x96\x88\xE2\x95\x91   \xE2\x96\x88\xE2\x96\x88"
    "\xE2\x95\x91\xE2\x96\x88\xE2\x96\x88\xE2\x96\x88\xE2\x96\x88\xE2\x96\x88"
    "\xE2\x96\x88\xE2\x96\x88\xE2\x95\x91\xE2\x96\x88\xE2\x96\x88\xE2\x95\x94"
    "\xE2\x96\x88\xE2\x96\x88\xE2\x96\x88\xE2\x96\x88\xE2\x95\x94\xE2\x96\x88"
    "\xE2\x96\x88\xE2\x95\x91\xE2\x96\x88\xE2\x96\x88\xE2\x96\x88\xE2\x96\x88"
    "\xE2\x96\x88\xE2\x95\x97  \xE2\x96\x88\xE2\x96\x88\xE2\x95\x91     "
    "\xE2\x96\x88\xE2\x96\x88\xE2\x95\x91   \xE2\x96\x88\xE2\x96\x88"
    "\xE2\x95\x91\xE2\x96\x88\xE2\x96\x88\xE2\x95\x91 \xE2\x96\x88"
    "\xE2\x95\x97 \xE2\x96\x88\xE2\x96\x88\xE2\x95\x91",
    "\xE2\x96\x88\xE2\x96\x88\xE2\x95\x91   \xE2\x96\x88\xE2\x96\x88"
    "\xE2\x95\x91\xE2\x96\x88\xE2\x96\x88\xE2\x95\x94\xE2\x95\x90\xE2\x95\x90"
    "\xE2\x96\x88\xE2\x96\x88\xE2\x95\x91\xE2\x96\x88\xE2\x96\x88\xE2\x95\x91"
    "\xE2\x95\x9A\xE2\x96\x88\xE2\x96\x88\xE2\x95\x94\xE2\x95\x9D\xE2\x96\x88"
    "\xE2\x96\x88\xE2\x95\x91\xE2\x96\x88\xE2\x96\x88\xE2\x95\x94\xE2\x95\x90"
    "\xE2\x95\x90\xE2\x95\x9D  \xE2\x96\x88\xE2\x96\x88\xE2\x95\x91     "
    "\xE2\x96\x88\xE2\x96\x88\xE2\x95\x91   \xE2\x96\x88\xE2\x96\x88"
    "\xE2\x95\x91\xE2\x96\x88\xE2\x96\x88\xE2\x95\x91\xE2\x96\x88\xE2\x96\x88"
    "\xE2\x96\x88\xE2\x95\x97\xE2\x96\x88\xE2\x96\x88\xE2\x95\x91",
    "\xE2\x95\x9A\xE2\x96\x88\xE2\x96\x88\xE2\x96\x88\xE2\x96\x88\xE2\x96\x88"
    "\xE2\x96\x88\xE2\x95\x94\xE2\x95\x9D\xE2\x96\x88\xE2\x96\x88\xE2\x95\x91"
    "  \xE2\x96\x88\xE2\x96\x88\xE2\x95\x91\xE2\x96\x88\xE2\x96\x88"
    "\xE2\x95\x91 \xE2\x95\x9A\xE2\x95\x90\xE2\x95\x9D \xE2\x96\x88"
    "\xE2\x96\x88\xE2\x95\x91\xE2\x96\x88\xE2\x96\x88\xE2\x95\x91     "
    "\xE2\x96\x88\xE2\x96\x88\xE2\x96\x88\xE2\x96\x88\xE2\x96\x88\xE2\x96\x88"
    "\xE2\x96\x88\xE2\x95\x97\xE2\x95\x9A\xE2\x96\x88\xE2\x96\x88\xE2\x96\x88"
    "\xE2\x96\x88\xE2\x96\x88\xE2\x96\x88\xE2\x95\x94\xE2\x95\x9D\xE2\x95\x9A"
    "\xE2\x96\x88\xE2\x96\x88\xE2\x96\x88\xE2\x95\x94\xE2\x96\x88\xE2\x96\x88"
    "\xE2\x96\x88\xE2\x95\x94\xE2\x95\x9D",
    " \xE2\x95\x9A\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90"
    "\xE2\x95\x90\xE2\x95\x9D \xE2\x95\x9A\xE2\x95\x90\xE2\x95\x9D  "
    "\xE2\x95\x9A\xE2\x95\x90\xE2\x95\x9D\xE2\x95\x9A\xE2\x95\x90\xE2\x95\x9D"
    "     \xE2\x95\x9A\xE2\x95\x90\xE2\x95\x9D\xE2\x95\x9A\xE2\x95\x90"
    "\xE2\x95\x9D     \xE2\x95\x9A\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90"
    "\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x9D \xE2\x95\x9A"
    "\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x90\xE2\x95\x9D"
    "  \xE2\x95\x9A\xE2\x95\x90\xE2\x95\x90\xE2\x95\x9D\xE2\x95\x9A"
    "\xE2\x95\x90\xE2\x95\x90\xE2\x95\x9D ",
};

/* Degradê do corpo: creme no topo, laranja (primária) na última linha com █;
 * sombra num laranja escuro, como o contorno do efeito "3D" */
static const int BANNER_TOPO[3]   = { 255, 232, 176 };
static const int BANNER_BASE[3]   = { 255, 135, 0 };
static const int BANNER_SOMBRA[3] = { 138, 74, 0 };

int banner_disponivel(int colunas)
{
#ifdef OHM_ASCII
    (void)colunas;
    return 0;
#else
    return colunas >= BANNER_COLUNAS + 2;
#endif
}

/* Acrescenta texto em buf[*pos], sem passar de n (corta em silêncio) */
static void anexa(char *buf, size_t n, size_t *pos, const char *fmt, ...)
{
    va_list ap;
    int k;
    if (*pos >= n) return;
    va_start(ap, fmt);
    k = vsnprintf(buf + *pos, n - *pos, fmt, ap);
    va_end(ap);
    if (k < 0) return;
    *pos = (size_t)k < n - *pos ? *pos + (size_t)k : n - 1;
}

int banner_linha(int i, char *buf, size_t n)
{
    const unsigned char *p;
    const int *cor_atual = NULL;
    int corpo[3], c, colunas = 0, cor = ui_cor_ativa();
    size_t pos = 0;

    if (buf == NULL || n == 0) return 0;
    buf[0] = '\0';
    if (i < 0 || i >= BANNER_LINHAS) return 0;

    /* interpola a cor do corpo entre a 1ª e a penúltima linha (a última só
     * tem sombra) */
    for (c = 0; c < 3; c++)
        corpo[c] = BANNER_TOPO[c]
                 + (BANNER_BASE[c] - BANNER_TOPO[c]) * i / (BANNER_LINHAS - 2);

    for (p = (const unsigned char *)BANNER[i]; *p; ) {
        size_t tam = *p < 0x80 ? 1 : (*p >= 0xF0 ? 4 : (*p >= 0xE0 ? 3 : 2));
        const int *alvo = NULL;
        if (*p != ' ')
            alvo = (p[0] == 0xE2 && p[1] == 0x96 && p[2] == 0x88) /* █ */
                 ? corpo : BANNER_SOMBRA;
        if (cor && alvo != NULL && alvo != cor_atual) {
            anexa(buf, n, &pos, "\033[38;2;%d;%d;%dm", alvo[0], alvo[1], alvo[2]);
            cor_atual = alvo;
        }
        anexa(buf, n, &pos, "%.*s", (int)tam, (const char *)p);
        p += tam;
        colunas++;
    }
    if (cor_atual != NULL) anexa(buf, n, &pos, "%s", ui_cor(UI_COR_RESET));
    return colunas;
}

/* Logo antigo (figlet "big", só ASCII): terminal estreito ou -DOHM_ASCII */
static void boas_vindas_texto(int cols, int rows)
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
    const char *laranja = ui_cor(UI_COR_PRIMARIA);
    const char *dim     = ui_cor(UI_COR_DIM);
    const char *rs      = ui_cor(UI_COR_RESET);
    int i, esq, topo;

    /* Altura do conteúdo: 6 logo + 1 blank + subtítulo + 2 blank + msg = 11 */
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
}

/* Banner ANSI Shadow + subtítulo + "► [ PRESSIONE ENTER ] ◄" em caixa dupla */
static void boas_vindas_banner(int cols, int rows)
{
    static const char SUBTITULO[] =
        "\xE2\x96\xBA\xE2\x96\xBA\xE2\x96\xBA ESTOQUE DE RESISTORES // ESTRUTURA DE DADOS "
        "\xE2\x97\x84\xE2\x97\x84\xE2\x97\x84";                       /* ►►► ... ◄◄◄ */
    static const char DETALHE[] = "[ LISTA DIN\xC3\x82MICA ORDENADA ]";
    static const char *const MSG[1] = {
        "\xE2\x96\xBA [ PRESSIONE ENTER PARA COME\xC3\x87" "AR ] \xE2\x97\x84"
    };
    char linha[BANNER_TAM_LINHA];
    const char *sec = ui_cor(UI_COR_SECUNDARIA);
    const char *dim = ui_cor(UI_COR_DIM);
    const char *rs  = ui_cor(UI_COR_RESET);
    int i, esq, topo;

    /* Altura: 6 banner + 1 blank + 2 subtítulos + 1 blank + caixa (3) = 13 */
    topo = (rows - 13) / 2;
    if (topo < 1) topo = 1;
    for (i = 0; i < topo; i++) putchar('\n');

    esq = (cols - BANNER_COLUNAS) / 2;
    for (i = 0; i < BANNER_LINHAS; i++) {
        banner_linha(i, linha, sizeof linha);
        printf("%*s%s\n", esq, "", linha);
    }
    putchar('\n');

    esq = (cols - (int)ui_largura(SUBTITULO)) / 2;
    printf("%*s%s%s%s\n", esq > 0 ? esq : 0, "", sec, SUBTITULO, rs);
    esq = (cols - (int)ui_largura(DETALHE)) / 2;
    printf("%*s%s%s%s\n\n", esq > 0 ? esq : 0, "", dim, DETALHE, rs);
    fflush(stdout); /* a caixa sai pela saída da ui */

    ui_caixa_centralizada(UI_MOLDURA_DUPLA, UI_COR_PRIMARIA, NULL, NULL, MSG, 1, 0);
}

void tela_boas_vindas(void)
{
    int cols = ui_colunas_terminal();
    int rows = ui_linhas_terminal();

    ui_limpar_tela();
    // cppcheck-suppress knownConditionTrueFalse ; sempre 0 só com -DOHM_ASCII
    if (banner_disponivel(cols) && ui_interativo())
        boas_vindas_banner(cols, rows);
    else
        boas_vindas_texto(cols, rows);

    /* Aguarda Enter sem ecoar o que for digitado */
    if (ui_interativo()) {
        fflush(stdout);
        ui_descartar_entrada(); /* teclas apertadas durante a abertura */
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
#else
    if (cursor_oculto) { /* Ctrl+C: escreve direto no console, sem stdio */
        DWORD escritos;
        WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), "\033[?25h", 6, &escritos, NULL);
    }
#endif
    signal(sinal, SIG_DFL);
    raise(sinal);
}

/* Saída por qualquer caminho (exit, fim da main) com o cursor escondido */
static void mostra_cursor_ao_sair(void)
{
    if (cursor_oculto) ui_cursor(1);
}

/* Instala (uma vez) os tratadores que devolvem terminal e cursor ao normal */
static void protege_terminal(void)
{
    static int instalado = 0;
    if (instalado) return;
    instalado = 1;
    atexit(mostra_cursor_ao_sair);
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
    int c;
    fflush(stdout); /* stdout tem buffer completo: o quadro sai antes de esperar */
    c = _getch();
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
    fflush(stdout);
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

/* Larguras do rótulo em cada coluna do menu (maior rótulo de cada lado) */
#define MENU_ROTULO_ESQ 9  /* "Consultar"     */
#define MENU_ROTULO_DIR 13 /* "Reinicializar" */

/* Célula de uma opção do menu. A opção ativa tem o chevron › em laranja
 * sobre o fundo 236; na `fase` 0 (meio da animação) o chevron ainda está
 * uma coluna à esquerda e em cinza, e na fase 1 desliza para o lugar */
static void celula_menu(char *buf, size_t n, int num, const char *rotulo, int largura,
                        int ativa, int fase)
{
    const char *rs = ui_cor(UI_COR_RESET);
    if (!ativa) {
        snprintf(buf, n, "   %s[%d]%s %s%-*s%s ", ui_cor(UI_COR_CINZA), num, rs,
                 ui_cor(UI_COR_SECUNDARIA), largura, rotulo, rs);
    } else if (fase == 0) {
        snprintf(buf, n, "%s%s%s%s  %s[%d] %-*s %s", ui_cor(UI_COR_FUNDO_FOCO),
                 ui_cor(UI_COR_DIM), UI_CHEVRON, ui_cor(UI_COR_SECUNDARIA),
                 ui_cor(UI_COR_NEGRITO), num, largura, rotulo, rs);
    } else {
        snprintf(buf, n, "%s%s%s %s %s[%d] %-*s %s", ui_cor(UI_COR_FUNDO_FOCO),
                 ui_cor(UI_COR_PRIMARIA), ui_cor(UI_COR_NEGRITO), UI_CHEVRON,
                 "", num, largura, rotulo, rs);
    }
}

/* Desenha o menu; `fase` 0 é o quadro intermediário da animação do foco */
static void desenha_menu_fase(const Lista *li, int selecao, int fase)
{
    static const char *const ROTULOS[OPCAO_TOTAL] = {
        "", "Novo", "Remover", "Listar", "Consultar",
        "Retirar", "Repor", "Reinicializar", "(diferencial)", "Hist\xc3\xb3rico"
    };
    char linhas[7][320], esq[160], dir[160], titulo[64], cab[96], vazio[96];
    const char *ptrs[7];
    const char *sec   = ui_cor(UI_COR_SECUNDARIA);
    const char *cinza = ui_cor(UI_COR_CINZA);
    const char *bold  = ui_cor(UI_COR_NEGRITO);
    const char *rs    = ui_cor(UI_COR_RESET);
    const char *dim   = ui_cor(UI_COR_DIM);
    int i, cols = ui_colunas_terminal();

    for (i = 0; i < 4; i++) {
        int num_esq = i + 1, num_dir = i + 5;
        celula_menu(esq, sizeof esq, num_esq, ROTULOS[num_esq], MENU_ROTULO_ESQ,
                    selecao == num_esq, fase);
        celula_menu(dir, sizeof dir, num_dir, ROTULOS[num_dir], MENU_ROTULO_DIR,
                    selecao == num_dir, fase);
        snprintf(linhas[i], sizeof linhas[i], "%s%s", esq, dir);
        ptrs[i] = linhas[i];
    }
    /* [9] Histórico na linha de baixo */
    celula_menu(linhas[4], sizeof linhas[4], OPCAO_HISTORICO, ROTULOS[OPCAO_HISTORICO],
                MENU_ROTULO_ESQ, selecao == OPCAO_HISTORICO, fase);
    ptrs[4] = linhas[4];
    linhas[5][0] = '\0';
    ptrs[5] = linhas[5];

    /* metadados: rótulos em cinza, valores em branco; estoque vazio = badge */
    vazio[0] = '\0';
    if (lista_vazia(li)) {
        char badge[64];
        ui_badge(UI_NEUTRO, NULL, badge, sizeof badge);
        snprintf(vazio, sizeof vazio, "  %s", badge);
    }
    snprintf(linhas[6], sizeof linhas[6], "%sTipos%s %s%s%d%s %s%s%s %sPe\xc3\xa7" "as%s %s%s%lld%s%s",
             cinza, rs, bold, sec, tamanho_lista(li), rs,
             cinza, UI_PONTO, rs,
             cinza, rs, bold, sec, total_pecas(li), rs, vazio);
    ptrs[6] = linhas[6];

    ui_espacar("OhmFlow", titulo, sizeof titulo, UI_LARGURA);
    snprintf(cab, sizeof cab, "%sControle de Estoque %s 8-Bit%s", cinza, UI_PONTO, rs);
    ui_caixa_centralizada(UI_MOLDURA_ARREDONDADA, UI_COR_BORDA, titulo, cab, ptrs, 7,
                          UI_LARGURA);

    /* Linha de dica centralizada */
    {
        const char *dica =
            "\xe2\x86\x91\xe2\x86\x93\xe2\x86\x90\xe2\x86\x92"
            "/WASD Navegar \xc2\xb7 [1-9] Acesso r\xc3\xa1pido \xc2\xb7 [Enter] Confirmar"
            " \xc2\xb7 [Esc/0] Sair";
        int m = (cols - (int)ui_largura(dica)) / 2;
        if (m < 0) m = 0;
        printf("%*s%s%s%s\n", m, "", dim, dica, rs);
    }
}

void desenha_menu(const Lista *li, int selecao)
{
    desenha_menu_fase(li, selecao, 1);
}

/* Linhas que o bloco do menu ocupa na tela (caixa + dica) */
#define MENU_LINHAS 14

/* Duração do quadro intermediário do chevron deslizante */
#define MENU_ANIMACAO_MS 30

/* Um quadro do menu, desenhado por cima do anterior (sem limpar a tela) */
static void quadro_menu(const Lista *li, int selecao, int fase)
{
    int pad, i;
    ui_quadro_inicio();
    /* Centralização vertical: pad linhas em branco acima do menu */
    pad = (ui_linhas_terminal() - MENU_LINHAS) / 2;
    for (i = 0; i < pad; i++) putchar('\n');
    desenha_menu_fase(li, selecao, fase);
    ui_quadro_fim();
}

int tela_menu(const Lista *li)
{
    int selecao = 1, animar = 0;

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
    ui_quadro_reset(); /* vindo de outra tela: o 1º quadro limpa tudo */
    ui_descartar_entrada(); /* teclas que sobraram da tela anterior */
    for (;;) {
        int c, op, nova;
        if (animar) { /* chevron desliza até a opção nova */
            quadro_menu(li, selecao, 0);
            ui_pausa_ms(MENU_ANIMACAO_MS);
        }
        quadro_menu(li, selecao, 1);

        c = ler_tecla();

        /* Esc, EOF, dígito ou Enter encerram o menu com uma opção */
        op = menu_tecla_opcao(c, selecao);
        if (op != MENU_CONTINUA) { mostra_cursor(1); return op; }

        /* seta segurada (tecla já na fila): sem animação, resposta imediata */
        nova = navegar_menu(selecao, c);
        animar = nova != selecao && !ui_tecla_pendente();
        selecao = nova;
    }
}

/* ---- [3]+[4] novo (unitário ou em massa) ------------------------------- */

/* Contadores da inserção em massa e itens inseridos (para o checkpoint
 * desfazer o lote inteiro de uma vez) */
typedef struct {
    int inseridos, duplicados, invalidos;
    Resistor *ok;      /* itens que deram LISTA_OK            */
    int       cap_ok;  /* capacidade de `ok`                  */
    int       sem_ck;  /* faltou memória para guardar um item */
} Lote;

static void colore_resistor(double ohms, const char *prompt, const char *digitado,
                            const Lote *lote);

/* Guarda r em lote->ok, dobrando o vetor quando enche */
static void guarda_no_lote(Lote *lote, Resistor r)
{
    if (lote->inseridos > lote->cap_ok) {
        int cap = lote->cap_ok ? lote->cap_ok * 2 : 8;
        Resistor *novo = realloc(lote->ok, (size_t)cap * sizeof *novo);
        if (novo == NULL) { lote->sem_ck = 1; return; }
        lote->ok = novo;
        lote->cap_ok = cap;
    }
    lote->ok[lote->inseridos - 1] = r;
}

/* Insere um item do lote, conta o resultado e mostra uma linha de feedback */
static void insere_do_lote(Lista *li, Resistor r, Lote *lote)
{
    char txt[TAM_RES];
    int res = insere_lista_ordenada(li, r);
    formata_resistencia(r.resistencia, txt, sizeof txt);
    if (res == LISTA_OK) {
        lote->inseridos++;
        if (!lote->sem_ck) guarda_no_lote(lote, r);
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
    Lote lote = { 0, 0, 0, NULL, 0, 0 };
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
        /* cada item válido redesenha o resistor com as suas cores */
        colore_resistor(r.resistencia, "Resistência [quantidade]: ", buf, &lote);
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

    /* o lote vira UMA ação no checkpoint; se um item não pôde ser guardado,
     * o histórico não bate mais com o estoque e é descartado */
    if (lote.sem_ck) ck_descarta(checkpoint);
    else if (lote.inseridos > 0) ck_registra_novo(checkpoint, lote.ok, lote.inseridos);
    free(lote.ok);

    registra(LOG_INFO, "NOVO", NULL, "LOTE CONCLUÍDO", "%d ok %s %d dup %s %d inv",
             lote.inseridos, UI_PONTO, lote.duplicados, UI_PONTO, lote.invalidos);
    ui_caixa_feedback(lote.inseridos > 0 ? UI_SUCESSO : UI_AVISO, "Inserção em massa concluída",
                      "Inseridos : %d\nDuplicados: %d\nInválidos : %d",
                      lote.inseridos, lote.duplicados, lote.invalidos);
    voltar_ao_menu();
}

/* O desenho do resistor só aparece no terminal com cores (sem terminal,
 * NO_COLOR ou -DOHM_ASCII a tela fica como antes, e o e2e não muda) */
static int mostra_resistor(void)
{
#ifdef OHM_ASCII
    return 0;
#else
    if (modo_resistor >= 0) return modo_resistor;
    return ui_interativo() && ui_cor_ativa();
#endif
}

/* Título da tela Novo com o resistor dentro da caixa (faixas NULL = sem cor)
 * e as dicas de formato */
static void abre_tela_novo(const int *faixas)
{
    // cppcheck-suppress knownConditionTrueFalse ; sempre 0 só com -DOHM_ASCII
    if (mostra_resistor()) {
        static char arte[RESISTOR_LINHAS][RESISTOR_TAM_LINHA];
        const char *linhas[RESISTOR_LINHAS];
        char t[128];
        int i;
        resistor_arte(faixas, arte);
        for (i = 0; i < RESISTOR_LINHAS; i++) linhas[i] = arte[i];
        ui_limpar_tela();
        ui_espaco_vertical(ALTURA_NOVO + RESISTOR_LINHAS);
        ui_caixa(UI_MOLDURA_ARREDONDADA, UI_COR_BORDA, titulo_da_tela("Novo resistor", t, sizeof t),
                 NULL, linhas, RESISTOR_LINHAS, UI_LARGURA);
    } else {
        abre_tela("Novo resistor", ALTURA_NOVO);
    }
    ui_dica("Formatos aceitos: 220 %s 4k7 %s 4.7k %s 1M %s 470m", UI_PONTO, UI_PONTO,
            UI_PONTO, UI_PONTO);
    ui_dica("Em massa: <resistência> <quantidade> por linha (ex.: 4k7 50)");
    ui_dica("(Enter vazio cancela; em massa, encerra a lista)");
    putchar('\n');
}

/* Redesenha a tela Novo com o resistor colorido pelo código de cores de
 * `ohms` (tolerância sempre dourada), repete a linha já digitada e mostra
 * o nome das faixas. Em massa (`lote` != NULL), mostra também o placar do
 * lote até ali, já que o redesenho apaga as linhas anteriores */
static void colore_resistor(double ohms, const char *prompt, const char *digitado,
                            const Lote *lote)
{
    int faixas[RESISTOR_FAIXAS], cod;
    // cppcheck-suppress knownConditionTrueFalse ; sempre 0 só com -DOHM_ASCII
    if (!mostra_resistor()) return;
    cod = codigo_cores(ohms, faixas);
    abre_tela_novo(faixas);
    if (lote != NULL)
        ui_dica("Lote até agora: %d inseridos %s %d duplicados %s %d inválidos",
                lote->inseridos, UI_PONTO, lote->duplicados, UI_PONTO, lote->invalidos);
    ui_linha_respondida(prompt, digitado);
    if (cod == CORES_FORA)
        ui_mensagem(UI_AVISO, "Abaixo de 0,1 %s: sem código de 4 faixas", SIMBOLO_OHM);
    else
        ui_dica("Faixas: %s %s %s %s %s %s %s%s", nome_cor_faixa(faixas[0]), UI_PONTO,
                nome_cor_faixa(faixas[1]), UI_PONTO, nome_cor_faixa(faixas[2]), UI_PONTO,
                nome_cor_faixa(faixas[3]), cod == CORES_APROXIMADO ? " (aprox.)" : "");
}

void tela_inserir(Lista *li)
{
    Resistor r;
    char buf[TAM_ENTRADA], txt[TAM_RES];
    int t, tipo = ITEM_INVALIDO, res;

    abre_tela_novo(NULL);

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
    colore_resistor(r.resistencia, "Resistência: ", buf, NULL);
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
    if (res == LISTA_OK) {
        ck_registra_novo(checkpoint, &r, 1);
        registra(LOG_INFO, "NOVO", txt, "OK", "qtd %d", r.quantidade);
    }
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
        Resistor removido;
        removido.resistencia = ohms;
        removido.quantidade = qtd;
        ck_registra_remover(checkpoint, removido);
        registra(LOG_INFO, "REMOVER", txt, "OK", "qtd %d", qtd);
        ui_caixa_feedback(UI_SUCESSO, "Resistor removido", "%s removido do estoque", txt);
    } else {
        registra(LOG_AVISO, "REMOVER", txt, "NÃO ENCONTRADO", NULL);
        ui_caixa_feedback(UI_ERRO, "Falha ao remover", "%s não está no estoque", txt);
    }
    voltar_ao_menu();
}

/* ---- [3] listar -------------------------------------------------------- */

/* Fatia de `qtd` no total de peças, em 0..1 (0 com estoque vazio) */
static double fatia(int qtd, long long total)
{
    return total > 0 ? (double)qtd / (double)total : 0.0;
}

/* "█████▌░░░░  56%": barra da fatia e a porcentagem; zerado = [ VAZIO ] */
static void celula_fatia(int qtd, long long total, int largura, char *buf, size_t n)
{
    char barra[256];
    double f = fatia(qtd, total);
    if (qtd == 0) { /* zerado: badge em vez de barra vazia e 0% */
        ui_badge(UI_NEUTRO, NULL, buf, n);
        return;
    }
    ui_barra(f, largura, UI_COR_PRIMARIA, barra, sizeof barra);
    snprintf(buf, n, "%s %3d%%", barra, (int)(f * 100 + 0.5));
}

/* Contexto de linha_estoque: número da linha e total de peças */
typedef struct {
    int indice;
    long long total;
} LinhaEstoque;

static void linha_estoque(const Resistor *r, void *ctx)
{
    LinhaEstoque *le = ctx;
    char txt[TAM_RES], c_idx[16], c_res[96], c_qtd[48], c_est[320];
    const char *celulas[TAB_COLUNAS];
    char *espaco;

    formata_resistencia(r->resistencia, txt, sizeof txt);
    espaco = strrchr(txt, ' '); /* separa número e unidade p/ alinhar */
    if (espaco != NULL) *espaco = '\0';

    snprintf(c_idx, sizeof c_idx, "%d", ++le->indice);
    /* valor no texto padrão: só a moldura da tabela é colorida */
    snprintf(c_res, sizeof c_res, "%7s %s", txt, espaco ? espaco + 1 : "");
    snprintf(c_qtd, sizeof c_qtd, "%s%d%s", r->quantidade == 0 ? ui_cor(UI_COR_VERMELHO) : "",
             r->quantidade, ui_cor(UI_COR_RESET));

    celulas[0] = c_idx;
    celulas[1] = c_res;
    celulas[2] = c_qtd;
    celula_fatia(r->quantidade, le->total, BARRA_LISTAR, c_est, sizeof c_est);
    celulas[3] = c_est;
    ui_tabela_linha(celulas, TAB_LARG, TAB_DIR, TAB_COLUNAS);
}

void tela_listar(const Lista *li)
{
    char cab[TAB_COLUNAS][48];
    const char *cabecalho[TAB_COLUNAS];
    const char *ng = ui_cor(UI_COR_NEGRITO), *rs = ui_cor(UI_COR_RESET);
    int c, tipos = tamanho_lista(li);
    long long pecas = total_pecas(li);
    LinhaEstoque le;

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
    snprintf(cab[3], sizeof cab[3], "%sEstoque%s", ng, rs);
    for (c = 0; c < TAB_COLUNAS; c++) cabecalho[c] = cab[c];

    le.indice = 0;
    le.total = pecas;
    ui_tabela_borda(UI_TAB_TOPO, TAB_LARG, TAB_COLUNAS);
    ui_tabela_linha(cabecalho, TAB_LARG, TAB_DIR, TAB_COLUNAS);
    ui_tabela_borda(UI_TAB_MEIO, TAB_LARG, TAB_COLUNAS);
    percorre_lista(li, linha_estoque, &le);
    ui_tabela_borda(UI_TAB_BASE, TAB_LARG, TAB_COLUNAS);
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
        char linha[96], fatia_txt[320], linha_fatia[352];
        const char *linhas[2];
        registra(LOG_INFO, "CONSULTAR", txt, "OK", "qtd %d", qtd);
        snprintf(linha, sizeof linha, "%s%s%s %s %s%d%s %s", ui_cor(UI_COR_NEGRITO), txt,
                 ui_cor(UI_COR_RESET), UI_SETA,
                 qtd == 0 ? ui_cor(UI_COR_VERMELHO) : ui_cor(UI_COR_VERDE), qtd,
                 ui_cor(UI_COR_RESET), plural(qtd, "unidade", "unidades"));
        linhas[0] = linha;
        celula_fatia(qtd, total_pecas(li), BARRA_CONSULTAR, fatia_txt, sizeof fatia_txt);
        snprintf(linha_fatia, sizeof linha_fatia, "%s %s%s%s", fatia_txt, ui_cor(UI_COR_DIM),
                 qtd == 0 ? "sem unidades" : "do estoque", ui_cor(UI_COR_RESET));
        linhas[1] = linha_fatia;
        putchar('\n');
        ui_caixa(UI_MOLDURA_SIMPLES, UI_COR_BORDA, NULL, NULL, linhas, 2, 26);
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
    if (res == LISTA_OK) ck_registra_movimento(checkpoint, OP_RETIRAR, ohms, qtd);
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
    if (res == LISTA_OK) {
        ck_registra_movimento(checkpoint, OP_REPOR, ohms, qtd);
        registra(LOG_INFO, "REPOR", txt, "OK", "+%d (%d%s%d)", qtd, atual, UI_SETA, atual + qtd);
    } else
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
    ck_registra_reinicializar(checkpoint, li); /* foto antes de apagar */
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

/* ---- [9] histórico: auxiliares das tabelas -------------------------- */

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

/* ---- checkpoint da sessão (dentro do Histórico) ---------------------- */

/* Larguras das colunas: ponto, hora, ação, resistor, detalhe, estado */
static const int CK_LARG[] = { 3, 8, 13, 10, 10, 10 };
#define CK_COLUNAS 6
#define ALTURA_CHECKPOINT_BASE 12 /* + uma linha por ação */

static const char *nome_operacao(int tipo)
{
    static const char *const NOMES[] = { "NOVO", "REMOVER", "RETIRAR", "REPOR", "REINICIALIZAR" };
    return (tipo >= OP_NOVO && tipo <= OP_REINICIALIZAR) ? NOMES[tipo] : "?";
}

/* Resistor e detalhe de uma operação, como aparecem na tabela e no log */
static void descreve_operacao(const Operacao *op, char *alvo, size_t na, char *det, size_t nd)
{
    alvo[0] = '\0';
    det[0] = '\0';
    switch (op->tipo) {
    case OP_NOVO:
        if (op->n_itens == 1) {
            formata_resistencia(op->itens[0].resistencia, alvo, na);
            snprintf(det, nd, "qtd %d", op->itens[0].quantidade);
        } else {
            snprintf(alvo, na, "%d tipos", op->n_itens);
            snprintf(det, nd, "lote");
        }
        break;
    case OP_REMOVER:
        formata_resistencia(op->itens[0].resistencia, alvo, na);
        snprintf(det, nd, "qtd %d", op->itens[0].quantidade);
        break;
    case OP_RETIRAR:
    case OP_REPOR:
        formata_resistencia(op->resistencia, alvo, na);
        snprintf(det, nd, "%c%d", op->tipo == OP_RETIRAR ? '-' : '+', op->qtd);
        break;
    case OP_REINICIALIZAR:
        snprintf(alvo, na, "-");
        snprintf(det, nd, "%d %s", op->n_itens, plural(op->n_itens, "tipo", "tipos"));
        break;
    default:
        break;
    }
}

/* Uma linha da linha do tempo: ponto i (0 = início da sessão). `cursor` é o
 * ponto destacado no modo interativo (-1 = nenhum) */
static void linha_checkpoint(int i, int pos, int cursor)
{
    const char *rs = ui_cor(UI_COR_RESET);
    const char *cor = i == cursor ? ui_cor(UI_COR_PRIMARIA)
                    : i > pos     ? ui_cor(UI_COR_DIM) : "";
    const char *ng = i == cursor ? ui_cor(UI_COR_NEGRITO) : "";
    char brutos[CK_COLUNAS][48], cel[CK_COLUNAS][160];
    const char *ptrs[CK_COLUNAS];
    int c;

    snprintf(brutos[0], sizeof brutos[0], "%d", i);
    if (i == 0) {
        snprintf(brutos[1], sizeof brutos[1], "-");
        snprintf(brutos[2], sizeof brutos[2], "IN\xC3\x8D" "CIO");
        snprintf(brutos[3], sizeof brutos[3], "-");
        snprintf(brutos[4], sizeof brutos[4], "sess\xC3\xA3o");
    } else {
        const Operacao *op = ck_operacao(checkpoint, i);
        const struct tm *tm = op != NULL ? localtime(&op->quando) : NULL;
        if (tm == NULL || strftime(brutos[1], sizeof brutos[1], "%H:%M:%S", tm) == 0)
            snprintf(brutos[1], sizeof brutos[1], "--:--:--");
        snprintf(brutos[2], sizeof brutos[2], "%s", op != NULL ? nome_operacao(op->tipo) : "?");
        if (op != NULL)
            descreve_operacao(op, brutos[3], sizeof brutos[3], brutos[4], sizeof brutos[4]);
        else
            brutos[3][0] = brutos[4][0] = '\0';
    }
    if (i == pos) snprintf(brutos[5], sizeof brutos[5], "%s atual", UI_PROMPT);
    else snprintf(brutos[5], sizeof brutos[5], "%s",
                  i == 0 ? "-" : i > pos ? "desfeita" : "aplicada");

    for (c = 0; c < CK_COLUNAS; c++) {
        char texto[96];
        corta(brutos[c], texto, sizeof texto, CK_LARG[c]);
        snprintf(cel[c], sizeof cel[c], "%s%s%s%s", ng, cor, texto, rs);
        ptrs[c] = cel[c];
    }
    ui_tabela_linha(ptrs, CK_LARG, NULL, CK_COLUNAS);
}

/* Tabela com todos os pontos da sessão, o mais recente em cima */
static void desenha_checkpoint(int cursor)
{
    static const char *const TITULOS[CK_COLUNAS] = {
        "#", "Hora", "A\xC3\xA7\xC3\xA3o", "Resistor", "Detalhe", "Estado"
    };
    char cab[CK_COLUNAS][48];
    const char *ptrs[CK_COLUNAS];
    int c, i, pos = ck_posicao(checkpoint), total = ck_total(checkpoint);

    for (c = 0; c < CK_COLUNAS; c++) {
        snprintf(cab[c], sizeof cab[c], "%s%s%s", ui_cor(UI_COR_NEGRITO), TITULOS[c],
                 ui_cor(UI_COR_RESET));
        ptrs[c] = cab[c];
    }
    ui_tabela_borda(UI_TAB_TOPO, CK_LARG, CK_COLUNAS);
    ui_tabela_linha(ptrs, CK_LARG, NULL, CK_COLUNAS);
    ui_tabela_borda(UI_TAB_MEIO, CK_LARG, CK_COLUNAS);
    for (i = total; i >= 0; i--) linha_checkpoint(i, pos, cursor);
    ui_tabela_borda(UI_TAB_BASE, CK_LARG, CK_COLUNAS);
}

int checkpoint_navegar(int pos, int tecla, int total)
{
    if (tecla == KEY_UP || tecla == 'w' || tecla == 'W') return pos < total ? pos + 1 : pos;
    if (tecla == KEY_DOWN || tecla == 's' || tecla == 'S') return pos > 0 ? pos - 1 : pos;
    return pos;
}

typedef struct {
    int desfeitas, refeitas;
} Passos;

/* Aviso de ck_ir_para: conta e registra no log cada ação desfeita/refeita */
static void registra_passo(int sentido, const Operacao *op, void *ctx)
{
    Passos *p = ctx;
    char alvo[TAM_RES], det[48];
    descreve_operacao(op, alvo, sizeof alvo, det, sizeof det);
    if (sentido == CK_DESFEITA) p->desfeitas++;
    else p->refeitas++;
    registra(LOG_INFO, sentido == CK_DESFEITA ? "DESFAZER" : "REFAZER", alvo, "OK",
             "%s %s", nome_operacao(op->tipo), det);
}

/* Leva o estoque ao ponto `alvo` e mostra o resultado */
static void aplica_checkpoint(Lista *li, int alvo)
{
    Passos p = { 0, 0 };
    int de = ck_posicao(checkpoint), total = ck_total(checkpoint), n;

    if (alvo == de) {
        ui_caixa_feedback(UI_INFO, "Nada a fazer", "O estoque j\xC3\xA1 est\xC3\xA1 no ponto %d",
                          alvo);
        return;
    }
    if (ck_ir_para(checkpoint, li, alvo, registra_passo, &p) != CK_OK) {
        registra(LOG_ERRO, "CHECKPOINT", NULL, "FALHOU", "ponto %d %s %d", de, UI_SETA, alvo);
        ui_caixa_feedback(UI_ERRO, "Falha ao voltar no hist\xC3\xB3rico",
                          "Mem\xC3\xB3ria insuficiente; estoque no ponto %d",
                          ck_posicao(checkpoint));
        return;
    }
    registra(LOG_INFO, "CHECKPOINT", NULL, "OK", "ponto %d %s %d", de, UI_SETA, alvo);
    n = p.desfeitas > 0 ? p.desfeitas : p.refeitas;
    ui_caixa_feedback(UI_SUCESSO, "Checkpoint aplicado", "%d %s %s\nAgora no ponto %d de %d",
                      n, plural(n, "a\xC3\xA7\xC3\xA3o", "a\xC3\xA7\xC3\xB5" "es"),
                      p.desfeitas > 0 ? plural(n, "desfeita", "desfeitas")
                                      : plural(n, "refeita", "refeitas"),
                      alvo, total);
}

/* Sem terminal (e2e, testes): mostra a linha do tempo e lê o número do ponto */
static void checkpoint_texto(Lista *li)
{
    char buf[TAM_ENTRADA];
    int alvo, total = ck_total(checkpoint);

    putchar('\n');
    ui_mensagem(UI_INFO, "Checkpoint desta sess\xC3\xA3o (%d %s)", total,
                plural(total, "a\xC3\xA7\xC3\xA3o", "a\xC3\xA7\xC3\xB5" "es"));
    desenha_checkpoint(-1);
    if (!ui_ler_linha("Ir para o ponto n\xC2\xBA (Enter mant\xC3\xA9m): ", buf, sizeof buf)
        || buf[0] == '\0')
        return;
    if (!parse_quantidade(buf, 0, &alvo) || alvo > total) {
        char citado[40];
        snprintf(citado, sizeof citado, "\"%.24s\"", buf);
        registra(LOG_ERRO, "CHECKPOINT", citado, "PONTO INV\xC3\x81LIDO", NULL);
        ui_mensagem(UI_ERRO, "Ponto inv\xC3\xA1lido: escolha de 0 a %d.", total);
        return;
    }
    aplica_checkpoint(li, alvo);
}

/* No terminal: setas movem o marcador, Enter aplica, Esc volta sem mudar */
static void checkpoint_interativo(Lista *li)
{
    int cursor = ck_posicao(checkpoint);
    char t[128];
    protege_terminal();
    mostra_cursor(0);
    ui_quadro_reset();
    for (;;) {
        int c, total = ck_total(checkpoint);
        ui_quadro_inicio(); /* setas redesenham por cima, sem limpar a tela */
        ui_espaco_vertical(ALTURA_CHECKPOINT_BASE + total);
        ui_caixa(UI_MOLDURA_ARREDONDADA, UI_COR_BORDA,
                 titulo_da_tela("Checkpoint da sess\xC3\xA3o", t, sizeof t), NULL,
                 NULL, 0, UI_LARGURA);
        desenha_checkpoint(cursor);
        ui_dica("\xE2\x86\x91\xE2\x86\x93/WS Escolher o ponto %s [Enter] Ir para ele %s [Esc] Voltar",
                UI_PONTO, UI_PONTO);
        ui_quadro_fim();
        c = ler_tecla();
        if (c == KEY_ESC || c == KEY_FIM || c == '0') { mostra_cursor(1); return; }
        if (c == '\r' || c == '\n') break;
        cursor = checkpoint_navegar(cursor, c, total);
    }
    mostra_cursor(1);
    putchar('\n');
    aplica_checkpoint(li, cursor);
    voltar_ao_menu();
}

/* ---- [9] histórico ---------------------------------------------------- */

/* Tabela das `n` últimas ações lidas dos logs, a mais recente em cima */
static void tabela_log(const LogEntrada *v, int n)
{
    static const char *const TITULOS[5] = {
        "Quando", "Ação", "Resistor", "Detalhe", "Resultado"
    };
    char cab[5][48], cel[5][128], quando[16];
    const char *ptrs[5];
    const char *rs = ui_cor(UI_COR_RESET);
    int i, c;

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
}

void tela_historico(Lista *li)
{
    LogEntrada v[HISTORICO_MAX];
    int n = log_ultimas(v, HISTORICO_MAX);
    int tem_ck = li != NULL && ck_total(checkpoint) > 0;

    abre_tela("Histórico", ALTURA_HISTORICO_BASE + n
              + (tem_ck && !ui_interativo() ? ALTURA_CHECKPOINT_BASE + ck_total(checkpoint) : 0));
    if (n == 0) {
        ui_mensagem(UI_INFO, "Nenhuma ação registrada nos últimos %d dias.",
                    LOG_DIAS_HISTORICO);
        registra(LOG_INFO, "HISTÓRICO", NULL, "VAZIO", NULL);
    } else {
        tabela_log(v, n);
        ui_dica("%d %s mais %s (últimos %d dias)", n,
                plural(n, "ação", "ações"),
                plural(n, "recente", "recentes"), LOG_DIAS_HISTORICO);
        /* registrado depois da leitura, para a tela não listar a si mesma */
        registra(LOG_INFO, "HISTÓRICO", NULL, "OK", "%d %s", n,
                 plural(n, "ação", "ações"));
    }

    if (!tem_ck) {
        voltar_ao_menu();
        return;
    }
    if (!ui_interativo()) {
        checkpoint_texto(li);
        voltar_ao_menu();
        return;
    }
    /* no terminal, a linha do tempo abre numa tela própria */
    ui_dica("[C] Voltar a um checkpoint desta sess\xC3\xA3o %s [Enter] Voltar ao menu", UI_PONTO);
    for (;;) {
        int t = ler_tecla();
        if (t == 'c' || t == 'C') break;
        if (t == '\r' || t == '\n' || t == KEY_ESC || t == KEY_FIM) return;
    }
    checkpoint_interativo(li);
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
