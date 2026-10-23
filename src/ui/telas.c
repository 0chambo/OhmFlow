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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "telas.h"
#include "ui.h"
#include "formatacao.h"

#define TENTATIVAS  3
#define TAM_ENTRADA 128
#define TAM_RES     32

/* Larguras das colunas da tabela de estoque: #, Resistência, Quantidade */
static const int TAB_LARG[] = { 2, 12, 10 };

/* Largura do conteúdo da caixa de boas-vindas (logo tem 45 colunas) */
#define LARG_BOAS_VINDAS 50
static const int TAB_DIR[]  = { 1, 0, 1 };

/* ---- auxiliares ------------------------------------------------------- */

static const char *plural(long n, const char *singular, const char *plur)
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
    ui_caixa_titulo(titulo);
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

/* Escreve em dst o texto s (pode ter cor) centralizado em `largura` colunas */
static void centraliza(char *dst, size_t n, const char *s, int largura)
{
    int esq = (largura - (int)ui_largura(s)) / 2;
    snprintf(dst, n, "%*s%s", esq > 0 ? esq : 0, "", s);
}

void tela_boas_vindas(void)
{
    static const char *const TEXTOS[2] = {
        "Estoque de Resistores - v1.0",
        "Estrutura de Dados e Recuperação da Informação"
    };
    char arte[5][128], cor[128], textos[2][128];
    const char *linhas[10];
    static const char *const LOGO[5] = {
        "  ___  _               _____ _               ",
        " / _ \\| |__  _ __ ___ |  ___| | _____      __",
        "| | | | '_ \\| '_ ` _ \\| |_  | |/ _ \\ \\ /\\ / /",
        "| |_| | | | | | | | | |  _| | | (_) \\ V  V / ",
        " \\___/|_| |_|_| |_| |_|_|   |_|\\___/ \\_/\\_/  "
    };
    int i, n = 0;

    ui_limpar_tela();
    linhas[n++] = "";
    for (i = 0; i < 5; i++) {
        snprintf(cor, sizeof cor, "%s%s%s", ui_cor(UI_COR_PROMPT), LOGO[i],
                 ui_cor(UI_COR_RESET));
        centraliza(arte[i], sizeof arte[i], cor, LARG_BOAS_VINDAS);
        linhas[n++] = arte[i];
    }
    linhas[n++] = "";
    for (i = 0; i < 2; i++) {
        centraliza(textos[i], sizeof textos[i], TEXTOS[i], LARG_BOAS_VINDAS);
        linhas[n++] = textos[i];
    }
    linhas[n++] = "";
    ui_caixa(UI_MOLDURA_DUPLA, UI_COR_BORDA, NULL, NULL, linhas, n, LARG_BOAS_VINDAS + 2);
    ui_pausar("Pressione Enter para começar...");
}

/* ---- [2] menu principal ----------------------------------------------- */

static void desenha_menu(Lista *li)
{
    static const char *const ROTULOS[OPCAO_TOTAL] = {
        "Sair", "Inserir", "Remover", "Listar", "Consultar",
        "Retirar", "Repor", "Reinicializar", "(diferencial)"
    };
    char linhas[5][160];
    const char *ptrs[5];
    const char *d = ui_cor(UI_COR_DESTAQUE), *rs = ui_cor(UI_COR_RESET);
    int i;

    for (i = 0; i < 4; i++) {
        snprintf(linhas[i], sizeof linhas[i], " %s[%d]%s %-13s %s[%d]%s %s",
                 d, i + 1, rs, ROTULOS[i + 1], d, i + 5, rs, ROTULOS[i + 5]);
        ptrs[i] = linhas[i];
    }
    snprintf(linhas[4], sizeof linhas[4], " %s[0]%s %s", d, rs, ROTULOS[OPCAO_SAIR]);
    ptrs[4] = linhas[4];

    ui_caixa(UI_MOLDURA_DUPLA, UI_COR_BORDA, NULL,
             "OhmFlow - Estoque de Resistores", ptrs, 5, UI_LARGURA);
    ui_dica("Tipos: %d %s Peças: %ld", tamanho_lista(li), UI_BARRA, total_pecas(li));
}

int tela_menu(Lista *li)
{
    char buf[TAM_ENTRADA], erro[TAM_ENTRADA + 64];
    char *fim;

    erro[0] = '\0';
    for (;;) {
        long op;
        ui_limpar_tela();
        putchar('\n');
        desenha_menu(li);
        if (erro[0]) ui_mensagem(UI_ERRO, "%s", erro);
        if (!ui_ler_linha("Opção: ", buf, sizeof buf)) return OPCAO_SAIR;
        op = strtol(buf, &fim, 10);
        if (buf[0] != '\0' && *fim == '\0' && op >= 0 && op < OPCAO_TOTAL) return (int)op;
        snprintf(erro, sizeof erro, "Opção inválida \"%s\". Escolha de 0 a %d.",
                 buf, OPCAO_TOTAL - 1);
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

void tela_listar(Lista *li)
{
    char cab[3][48];
    const char *cabecalho[3];
    const char *ng = ui_cor(UI_COR_NEGRITO), *rs = ui_cor(UI_COR_RESET);
    int indice = 0, tipos = tamanho_lista(li);
    long pecas = total_pecas(li);

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
    ui_dica("%d %s %s %ld %s", tipos, plural(tipos, "tipo", "tipos"), UI_PONTO,
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
                          "%s: %d %s %d unidades", txt, atual, UI_SETA, atual + qtd);
    else
        ui_caixa_feedback(UI_ERRO, "Quantidade inválida",
                          "O total excederia o limite de %d unidades", INT_MAX);
    voltar_ao_menu();
}

/* ---- [3]+[4] reinicializar --------------------------------------------- */

void tela_reinicializar(Lista *li)
{
    int tipos = tamanho_lista(li);
    long pecas = total_pecas(li);

    abre_tela("Inicializar / reinicializar");
    putchar('\n');
    if (lista_vazia(li)) {
        reinicializa_lista(li);
        ui_caixa_feedback(UI_INFO, "Estoque inicializado", "Lista vazia e pronta para uso");
        voltar_ao_menu();
        return;
    }
    if (!ui_confirmar("Apagar TODOS os %d %s (%ld %s)?", tipos, plural(tipos, "tipo", "tipos"),
                      pecas, plural(pecas, "peça", "peças"))) {
        feedback_cancelado();
        return;
    }
    reinicializa_lista(li);
    ui_caixa_feedback(UI_SUCESSO, "Estoque reinicializado", "%d %s removidos (%ld %s)", tipos,
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

void tela_encerramento(Lista *li)
{
    char resumo[96];
    const char *linhas[2];
    int tipos = tamanho_lista(li);
    long pecas = total_pecas(li);

    snprintf(resumo, sizeof resumo, "Estoque final: %d %s %s %ld %s", tipos,
             plural(tipos, "tipo", "tipos"), UI_PONTO, pecas,
             plural(pecas, "peça", "peças"));
    linhas[0] = "Obrigado por usar o OhmFlow!";
    linhas[1] = resumo;
    ui_limpar_tela();
    putchar('\n');
    ui_caixa(UI_MOLDURA_DUPLA, UI_COR_BORDA, NULL, NULL, linhas, 2, UI_LARGURA);
}
