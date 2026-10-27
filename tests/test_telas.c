/* test_telas.c — testes da lógica de navegação do menu */
#include <stdlib.h>
#include <string.h>
#include "test_framework.h"
#include "telas.h"
#include "ui.h"
#include "lista.h"
#include "log.h"
#include "checkpoint.h"

/* ---- navegar_menu: navegação vertical ------------------------------------ */

static void teste_navegar_baixo_simples(void)
{
    ASSERT_EQ_INT(2, navegar_menu(1, KEY_DOWN));
    ASSERT_EQ_INT(3, navegar_menu(2, KEY_DOWN));
    ASSERT_EQ_INT(4, navegar_menu(3, KEY_DOWN));
    ASSERT_EQ_INT(6, navegar_menu(5, KEY_DOWN));
    ASSERT_EQ_INT(7, navegar_menu(6, KEY_DOWN));
    ASSERT_EQ_INT(8, navegar_menu(7, KEY_DOWN));
}

static void teste_navegar_baixo_para_sair(void)
{
    /* últimas linhas de cada coluna descem para o Histórico */
    ASSERT_EQ_INT(OPCAO_HISTORICO, navegar_menu(4, KEY_DOWN));
    ASSERT_EQ_INT(OPCAO_HISTORICO, navegar_menu(8, KEY_DOWN));
}

static void teste_navegar_baixo_de_sair(void)
{
    /* do Histórico volta ao topo da coluna */
    int r = navegar_menu(OPCAO_HISTORICO, KEY_DOWN);
    ASSERT_TRUE(r == 1 || r == 5); /* col preservada; col padrão = 0 → 1 */
}

static void teste_navegar_cima_simples(void)
{
    ASSERT_EQ_INT(1, navegar_menu(2, KEY_UP));
    ASSERT_EQ_INT(2, navegar_menu(3, KEY_UP));
    ASSERT_EQ_INT(3, navegar_menu(4, KEY_UP));
    ASSERT_EQ_INT(5, navegar_menu(6, KEY_UP));
    ASSERT_EQ_INT(6, navegar_menu(7, KEY_UP));
    ASSERT_EQ_INT(7, navegar_menu(8, KEY_UP));
}

static void teste_navegar_cima_para_sair(void)
{
    /* topo de cada coluna sobe para o Histórico */
    ASSERT_EQ_INT(OPCAO_HISTORICO, navegar_menu(1, KEY_UP));
    ASSERT_EQ_INT(OPCAO_HISTORICO, navegar_menu(5, KEY_UP));
}

static void teste_navegar_cima_de_sair(void)
{
    /* do Histórico volta ao fundo da coluna (linha 3 = opções 4 ou 8) */
    int r = navegar_menu(OPCAO_HISTORICO, KEY_UP);
    ASSERT_TRUE(r == 4 || r == 8);
}

/* ---- navegar_menu: navegação horizontal ---------------------------------- */

static void teste_navegar_direita(void)
{
    ASSERT_EQ_INT(5, navegar_menu(1, KEY_RIGHT));
    ASSERT_EQ_INT(6, navegar_menu(2, KEY_RIGHT));
    ASSERT_EQ_INT(7, navegar_menu(3, KEY_RIGHT));
    ASSERT_EQ_INT(8, navegar_menu(4, KEY_RIGHT));
}

static void teste_navegar_esquerda(void)
{
    ASSERT_EQ_INT(1, navegar_menu(5, KEY_LEFT));
    ASSERT_EQ_INT(2, navegar_menu(6, KEY_LEFT));
    ASSERT_EQ_INT(3, navegar_menu(7, KEY_LEFT));
    ASSERT_EQ_INT(4, navegar_menu(8, KEY_LEFT));
}

static void teste_navegar_sem_mover_na_borda(void)
{
    /* col 0 não vai mais para esquerda */
    ASSERT_EQ_INT(1, navegar_menu(1, KEY_LEFT));
    ASSERT_EQ_INT(4, navegar_menu(4, KEY_LEFT));
    /* col 1 não vai mais para direita */
    ASSERT_EQ_INT(5, navegar_menu(5, KEY_RIGHT));
    ASSERT_EQ_INT(8, navegar_menu(8, KEY_RIGHT));
}

static void teste_navegar_horizontal_de_sair(void)
{
    /* Histórico não se move para esquerda/direita */
    ASSERT_EQ_INT(OPCAO_HISTORICO, navegar_menu(OPCAO_HISTORICO, KEY_LEFT));
    ASSERT_EQ_INT(OPCAO_HISTORICO, navegar_menu(OPCAO_HISTORICO, KEY_RIGHT));
}

/* ---- navegar_menu: teclas irrelevantes ----------------------------------- */

static void teste_navegar_tecla_ignorada(void)
{
    /* tecla qualquer não muda seleção */
    ASSERT_EQ_INT(3, navegar_menu(3, 'x'));
    ASSERT_EQ_INT(7, navegar_menu(7, 'z'));
}

/* ---- decodifica_tecla: sequências de bytes do terminal ------------------- */

/* Fonte falsa de bytes: devolve a sequência e conta leituras bloqueantes
 * feitas depois da primeira (não podem existir: travariam o menu). */
typedef struct {
    const int *bytes;
    int n, pos;
    int bloqueios_extras;
} Fonte;

static int fonte_proximo(void *ctx, int bloqueia)
{
    Fonte *f = ctx;
    if (bloqueia && f->pos > 0) f->bloqueios_extras++;
    if (f->pos >= f->n) return bloqueia ? EOF : TECLA_TIMEOUT;
    return f->bytes[f->pos++];
}

static int decodifica(const int *bytes, int n, Fonte *f)
{
    f->bytes = bytes;
    f->n = n;
    f->pos = 0;
    f->bloqueios_extras = 0;
    return decodifica_tecla(fonte_proximo, f);
}

static void teste_tecla_comum(void)
{
    static const int w[] = { 'w' }, enter[] = { '\n' }, dig[] = { '7' };
    Fonte f;
    ASSERT_EQ_INT('w', decodifica(w, 1, &f));
    ASSERT_EQ_INT('\n', decodifica(enter, 1, &f));
    ASSERT_EQ_INT('7', decodifica(dig, 1, &f));
}

static void teste_tecla_setas(void)
{
    static const int cima[] = { 27, '[', 'A' }, baixo[] = { 27, '[', 'B' };
    static const int dir[] = { 27, '[', 'C' }, esq[] = { 27, '[', 'D' };
    static const int ss3[] = { 27, 'O', 'A' }; /* modo "application cursor" */
    Fonte f;
    ASSERT_EQ_INT(KEY_UP, decodifica(cima, 3, &f));
    ASSERT_EQ_INT(KEY_DOWN, decodifica(baixo, 3, &f));
    ASSERT_EQ_INT(KEY_RIGHT, decodifica(dir, 3, &f));
    ASSERT_EQ_INT(KEY_LEFT, decodifica(esq, 3, &f));
    ASSERT_EQ_INT(KEY_UP, decodifica(ss3, 3, &f));
    ASSERT_EQ_INT(0, f.bloqueios_extras);
}

static void teste_tecla_fim_da_entrada(void)
{
    static const int ctrl_d[] = { 4 };
    Fonte f;
    ASSERT_EQ_INT(KEY_FIM, decodifica(NULL, 0, &f)); /* EOF / terminal fechado */
    ASSERT_EQ_INT(KEY_FIM, decodifica(ctrl_d, 1, &f));
}

static void teste_tecla_esc_isolado_nao_trava(void)
{
    static const int esc[] = { 27 };
    Fonte f;
    ASSERT_EQ_INT(KEY_ESC, decodifica(esc, 1, &f)); /* ESC sozinho: tecla Esc */
    ASSERT_EQ_INT(0, f.bloqueios_extras); /* não esperou outra tecla */
}

static void teste_menu_esc_sai(void)
{
    /* Esc no menu aciona Sair na hora; demais teclas seguem o fluxo normal */
    ASSERT_EQ_INT(OPCAO_SAIR, menu_tecla_opcao(KEY_ESC, 3));
    ASSERT_EQ_INT(OPCAO_SAIR, menu_tecla_opcao(KEY_ESC, OPCAO_HISTORICO));
    ASSERT_EQ_INT(OPCAO_SAIR, menu_tecla_opcao(KEY_FIM, 5));
    ASSERT_EQ_INT(OPCAO_LISTAR, menu_tecla_opcao('3', 7));
    ASSERT_EQ_INT(6, menu_tecla_opcao('\r', 6));
    ASSERT_EQ_INT(6, menu_tecla_opcao('\n', 6));
    ASSERT_EQ_INT(MENU_CONTINUA, menu_tecla_opcao(KEY_UP, 2));
    ASSERT_EQ_INT(MENU_CONTINUA, menu_tecla_opcao(KEY_IGNORADA, 2));
    ASSERT_EQ_INT(MENU_CONTINUA, menu_tecla_opcao('x', 2));
}

static void teste_historico_opcao_9(void)
{
    /* [9] Histórico na linha de baixo (onde ficava o "[0] Sair"); a tecla 0
     * continua saindo, igual ao Esc */
    ASSERT_EQ_INT(9, OPCAO_HISTORICO);
    ASSERT_EQ_INT(OPCAO_HISTORICO, menu_tecla_opcao('9', 2));
    ASSERT_EQ_INT(OPCAO_SAIR, menu_tecla_opcao('0', 3));
    ASSERT_EQ_INT(OPCAO_SAIR, menu_tecla_opcao('0', OPCAO_HISTORICO));
    ASSERT_EQ_INT(OPCAO_HISTORICO, menu_tecla_opcao('\r', OPCAO_HISTORICO));
    ASSERT_EQ_INT(OPCAO_HISTORICO, menu_tecla_opcao('\n', OPCAO_HISTORICO));
    ASSERT_EQ_INT(OPCAO_HISTORICO, navegar_menu(4, KEY_DOWN));
    ASSERT_EQ_INT(OPCAO_LISTAR, menu_tecla_opcao('3', OPCAO_HISTORICO));
    ASSERT_TRUE(OPCAO_SAIR < 0 || OPCAO_SAIR >= OPCAO_TOTAL);
    ASSERT_TRUE(OPCAO_SAIR != MENU_CONTINUA);
}

static void teste_tecla_esc_sequencia_desconhecida(void)
{
    static const int alt_x[] = { 27, 'x' }, csi_z[] = { 27, '[', 'Z' };
    static const int csi_cortado[] = { 27, '[' };
    Fonte f;
    ASSERT_EQ_INT(KEY_IGNORADA, decodifica(alt_x, 2, &f));
    ASSERT_EQ_INT(KEY_IGNORADA, decodifica(csi_z, 3, &f));
    ASSERT_EQ_INT(KEY_IGNORADA, decodifica(csi_cortado, 2, &f));
    ASSERT_EQ_INT(0, f.bloqueios_extras);
}

static void teste_navegar_ignora_teclas_especiais(void)
{
    ASSERT_EQ_INT(3, navegar_menu(3, KEY_IGNORADA));
    ASSERT_EQ_INT(6, navegar_menu(6, KEY_FIM));
    ASSERT_EQ_INT(4, navegar_menu(4, KEY_ESC));
}

/* ---- desenha_menu: borda laranja + destaque ------------------------------ */

static FILE *abre_temp(void)
{
    FILE *f = tmpfile();
    if (f == NULL) f = fopen("build/_tmp_test_telas.txt", "w+b");
    return f;
}

static const char *conteudo(FILE *f, char *buf, size_t n)
{
    size_t lido;
    fflush(f);
    rewind(f);
    lido = fread(buf, 1, n - 1, f);
    buf[lido] = '\0';
    return buf;
}

static void teste_menu_cor_laranja_na_borda(void)
{
    char buf[16384];
    FILE *f = abre_temp();
    Lista *li;
    if (f == NULL) return; /* skip se não conseguiu temp */

    li = cria_lista();
    ui_init();
    ui_definir_saida(f);
    ui_definir_cor(1); /* força cor ligada */

    desenha_menu(li, 1);

    conteudo(f, buf, sizeof buf);
    ui_definir_saida(stdout);
    fclose(f);
    libera_lista(li);

    /* Borda fina cinza; o laranja fica só no destaque (opção ativa) */
    ASSERT_TRUE(strstr(buf, "\033[38;5;240m") != NULL);
    ASSERT_TRUE(strstr(buf, "\033[38;5;208m") != NULL);
}

static void teste_menu_opcao_selecionada_laranja(void)
{
    char buf[16384];
    FILE *f = abre_temp();
    Lista *li;
    if (f == NULL) return;

    li = cria_lista();
    ui_init();
    ui_definir_saida(f);
    ui_definir_cor(1);

    desenha_menu(li, 3); /* seleciona Listar */

    conteudo(f, buf, sizeof buf);
    ui_definir_saida(stdout);
    fclose(f);
    libera_lista(li);

    /* Opção selecionada usa laranja */
    ASSERT_TRUE(strstr(buf, "\033[38;5;208m") != NULL);
    /* Rótulo da opção aparece no output */
    ASSERT_TRUE(strstr(buf, "Listar") != NULL);
}

static void teste_menu_so_laranja_e_branco(void)
{
    char buf[16384];
    FILE *f = abre_temp();
    Lista *li;
    if (f == NULL) return;

    li = cria_lista();
    ui_init();
    ui_definir_saida(f);
    ui_definir_cor(1);

    desenha_menu(li, 3);

    conteudo(f, buf, sizeof buf);
    ui_definir_cor(0);
    ui_definir_saida(stdout);
    fclose(f);
    libera_lista(li);

    /* Não selecionadas em branco 255, metadados em cinza, sem neon */
    ASSERT_TRUE(strstr(buf, "\033[38;5;255m") != NULL);
    ASSERT_TRUE(strstr(buf, "\033[38;5;245m") != NULL);
    ASSERT_TRUE(strstr(buf, "\033[38;5;46m") == NULL);  /* verde neon */
    ASSERT_TRUE(strstr(buf, "\033[38;5;141m") == NULL); /* roxo neon  */
}

/* Quantas vezes `agulha` aparece em `s` */
static int conta(const char *s, const char *agulha)
{
    int n = 0;
    size_t t = strlen(agulha);
    while ((s = strstr(s, agulha)) != NULL) { n++; s += t; }
    return n;
}

/* Foco do menu: só a opção ativa tem o chevron › e o fundo 236 */
static void teste_menu_foco_chevron(void)
{
    char buf[16384];
    FILE *f = abre_temp();
    Lista *li;
    if (f == NULL) return;

    li = cria_lista();
    ui_init();
    ui_definir_saida(f);
    ui_definir_cor(1);
    desenha_menu(li, 6); /* Repor */
    conteudo(f, buf, sizeof buf);
    ui_definir_cor(0);
    ui_definir_saida(stdout);
    fclose(f);
    libera_lista(li);

    ASSERT_EQ_INT(1, conta(buf, UI_CHEVRON));
    ASSERT_EQ_INT(1, conta(buf, "\033[48;5;236m"));
    /* chevron vem antes do rótulo da opção ativa */
    ASSERT_TRUE(strstr(buf, UI_CHEVRON) < strstr(buf, "Repor"));
    ASSERT_TRUE(strstr(buf, UI_CHEVRON) > strstr(buf, "Remover"));
}

static void teste_menu_foco_sem_cor_mantem_chevron(void)
{
    char buf[16384];
    FILE *f = abre_temp();
    Lista *li;
    if (f == NULL) return;

    li = cria_lista();
    ui_init();
    ui_definir_saida(f);
    ui_definir_cor(0);
    desenha_menu(li, 1);
    conteudo(f, buf, sizeof buf);
    ui_definir_saida(stdout);
    fclose(f);
    libera_lista(li);

    ASSERT_EQ_INT(1, conta(buf, UI_CHEVRON)); /* foco visível sem cor */
    ASSERT_TRUE(strchr(buf, '\033') == NULL);
    ASSERT_TRUE(strstr(buf, "[ VAZIO ]") != NULL); /* estoque vazio */
}

static void teste_tela_historico_mostra_acoes(void)
{
    char buf[16384], caminho[512];
    FILE *f = abre_temp();
    if (f == NULL) return;

    log_definir_pasta("build/test_hist_tmp/logs");
    if (log_caminho_do_dia(caminho, sizeof caminho)) remove(caminho);
    ASSERT_EQ_INT(1, log_iniciar());
    log_acao(LOG_INFO, "RETIRAR", "4,7 k\xCE\xA9", "-3 (10\xE2\x86\x92" "7)", "OK");
    log_acao(LOG_AVISO, "CONSULTAR", "1 M\xCE\xA9", NULL, "N\xC3\x83O ENCONTRADO");

    ui_init();
    ui_definir_interativo(0); /* sem pausa nem limpar tela */
    ui_definir_saida(f);
    ui_definir_cor(0);
    tela_historico(NULL);
    log_encerrar();
    log_definir_pasta(NULL);

    conteudo(f, buf, sizeof buf);
    ui_definir_saida(stdout);
    fclose(f);

    ASSERT_TRUE(strstr(buf, "Hist\xC3\xB3rico") != NULL);
    ASSERT_TRUE(strstr(buf, "RETIRAR") != NULL);
    ASSERT_TRUE(strstr(buf, "4,7 k\xCE\xA9") != NULL);
    ASSERT_TRUE(strstr(buf, "CONSULTAR") != NULL);
    /* mais recente primeiro */
    ASSERT_TRUE(strstr(buf, "CONSULTAR") < strstr(buf, "RETIRAR"));
}

static void teste_menu_mostra_historico_9(void)
{
    char buf[16384];
    FILE *f = abre_temp();
    Lista *li;
    if (f == NULL) return;

    li = cria_lista();
    ui_init();
    ui_definir_saida(f);
    ui_definir_cor(0);

    desenha_menu(li, OPCAO_HISTORICO);

    conteudo(f, buf, sizeof buf);
    ui_definir_saida(stdout);
    fclose(f);
    libera_lista(li);

    ASSERT_TRUE(strstr(buf, "[9] Hist\xC3\xB3rico") != NULL);
    ASSERT_TRUE(strstr(buf, "[0]") == NULL);  /* "[0] Sair" saiu da caixa */
    ASSERT_TRUE(strstr(buf, "Sair") == NULL); /* sair agora é Esc ou 0 */
}

/* ---- checkpoint (desfazer/refazer no Histórico) -------------------------- */

static void teste_checkpoint_navegar(void)
{
    /* cima = ação mais recente, baixo = mais antiga; trava nas bordas */
    ASSERT_EQ_INT(3, checkpoint_navegar(2, KEY_UP, 5));
    ASSERT_EQ_INT(3, checkpoint_navegar(2, 'w', 5));
    ASSERT_EQ_INT(1, checkpoint_navegar(2, KEY_DOWN, 5));
    ASSERT_EQ_INT(1, checkpoint_navegar(2, 'S', 5));
    ASSERT_EQ_INT(5, checkpoint_navegar(5, KEY_UP, 5));
    ASSERT_EQ_INT(0, checkpoint_navegar(0, KEY_DOWN, 5));
    ASSERT_EQ_INT(2, checkpoint_navegar(2, KEY_LEFT, 5));
    ASSERT_EQ_INT(2, checkpoint_navegar(2, 'x', 5));
}

#define ARQ_ENTRADA_TELAS "build/_tmp_test_telas_entrada.txt"

/* Faz stdin ler `texto`; 0 se não conseguiu (o teste é pulado) */
static int entrada(const char *texto)
{
    FILE *f = fopen(ARQ_ENTRADA_TELAS, "wb");
    if (f == NULL) return 0;
    fputs(texto, f);
    fclose(f);
    return freopen(ARQ_ENTRADA_TELAS, "rb", stdin) != NULL;
}

/* Lista com 3 ações registradas: novo 220 (50), novo 4k7 (10), retirar 4k7 (3) */
static void prepara_sessao(Lista *li, Checkpoint *ck)
{
    Resistor r;
    r.resistencia = 220;
    r.quantidade = 50;
    insere_lista_ordenada(li, r);
    ck_registra_novo(ck, &r, 1);
    r.resistencia = 4700;
    r.quantidade = 10;
    insere_lista_ordenada(li, r);
    ck_registra_novo(ck, &r, 1);
    retira_estoque(li, 4700, 3);
    ck_registra_movimento(ck, OP_RETIRAR, 4700, 3);
}

/* Roda o Histórico sem terminal com `roteiro` na entrada; devolve a saída */
static int roda_historico(Lista *li, Checkpoint *ck, const char *roteiro,
                          char *buf, size_t n)
{
    FILE *f = abre_temp();
    if (f == NULL || !entrada(roteiro)) { if (f) fclose(f); return 0; }
    log_definir_pasta("build/test_hist_tmp/logs");
    ui_init();
    ui_definir_interativo(0);
    ui_definir_saida(f);
    ui_definir_cor(0);
    telas_definir_checkpoint(ck);
    tela_historico(li);
    telas_definir_checkpoint(NULL);
    log_definir_pasta(NULL);
    conteudo(f, buf, n);
    ui_definir_saida(stdout);
    fclose(f);
    return 1;
}

static void teste_historico_volta_ao_checkpoint(void)
{
    char buf[16384];
    Lista *li = cria_lista();
    Checkpoint *ck = cria_checkpoint();
    int q = 0;
    prepara_sessao(li, ck);

    if (roda_historico(li, ck, "1\n", buf, sizeof buf)) {
        ASSERT_TRUE(strstr(buf, "Checkpoint") != NULL);
        ASSERT_TRUE(strstr(buf, "RETIRAR") != NULL);
        ASSERT_EQ_INT(1, ck_posicao(ck));
        ASSERT_EQ_INT(1, tamanho_lista(li));           /* só o 220 */
        ASSERT_EQ_INT(LISTA_OK, consulta_lista(li, 220, &q));
        ASSERT_EQ_INT(50, q);
        ASSERT_TRUE(strstr(buf, "2 a\xC3\xA7\xC3\xB5" "es desfeitas") != NULL);
    }
    /* voltou demais: avança de novo até a ação 2 */
    if (roda_historico(li, ck, "2\n", buf, sizeof buf)) {
        ASSERT_EQ_INT(2, ck_posicao(ck));
        ASSERT_EQ_INT(LISTA_OK, consulta_lista(li, 4700, &q));
        ASSERT_EQ_INT(10, q);
        ASSERT_TRUE(strstr(buf, "1 a\xC3\xA7\xC3\xA3o refeita") != NULL);
    }
    libera_checkpoint(ck);
    libera_lista(li);
}

static void teste_historico_checkpoint_enter_mantem(void)
{
    char buf[16384];
    Lista *li = cria_lista();
    Checkpoint *ck = cria_checkpoint();
    prepara_sessao(li, ck);
    if (roda_historico(li, ck, "\n", buf, sizeof buf)) {
        ASSERT_EQ_INT(3, ck_posicao(ck));
        ASSERT_EQ_INT(2, tamanho_lista(li));
    }
    /* número fora da faixa não mexe no estoque */
    if (roda_historico(li, ck, "9\n", buf, sizeof buf)) {
        ASSERT_EQ_INT(3, ck_posicao(ck));
        ASSERT_TRUE(strstr(buf, "inv\xC3\xA1lid") != NULL);
    }
    libera_checkpoint(ck);
    libera_lista(li);
}

static void teste_historico_sem_checkpoint_nao_pergunta(void)
{
    char buf[16384];
    Lista *li = cria_lista();
    Checkpoint *ck = cria_checkpoint(); /* vazio: nada a desfazer */
    if (roda_historico(li, ck, "", buf, sizeof buf))
        ASSERT_TRUE(strstr(buf, "Ir para") == NULL);
    libera_checkpoint(ck);
    libera_lista(li);
}

/* ---- main ---------------------------------------------------------------- */

/* ---- boas-vindas: banner ANSI Shadow ----------------------------------- */

static void teste_banner_sem_cor_literal(void)
{
#ifndef OHM_ASCII
    char buf[BANNER_TAM_LINHA];
    ui_definir_cor(0);
    banner_linha(0, buf, sizeof buf);
    ASSERT_STR_EQ(" \xE2\x96\x88\xE2\x96\x88\xE2\x96\x88\xE2\x96\x88\xE2\x96\x88\xE2\x96\x88"
                  "\xE2\x95\x97 \xE2\x96\x88\xE2\x96\x88\xE2\x95\x97  \xE2\x96\x88\xE2\x96\x88"
                  "\xE2\x95\x97\xE2\x96\x88\xE2\x96\x88\xE2\x96\x88\xE2\x95\x97   \xE2\x96\x88"
                  "\xE2\x96\x88\xE2\x96\x88\xE2\x95\x97\xE2\x96\x88\xE2\x96\x88\xE2\x96\x88"
                  "\xE2\x96\x88\xE2\x96\x88\xE2\x96\x88\xE2\x96\x88\xE2\x95\x97\xE2\x96\x88"
                  "\xE2\x96\x88\xE2\x95\x97      \xE2\x96\x88\xE2\x96\x88\xE2\x96\x88\xE2\x96\x88"
                  "\xE2\x96\x88\xE2\x96\x88\xE2\x95\x97 \xE2\x96\x88\xE2\x96\x88\xE2\x95\x97    "
                  "\xE2\x96\x88\xE2\x96\x88\xE2\x95\x97", buf);
    ASSERT_TRUE(strchr(buf, '\033') == NULL);
#endif
}

static void teste_banner_largura(void)
{
#ifndef OHM_ASCII
    char buf[BANNER_TAM_LINHA];
    int i;
    for (i = 0; i < BANNER_LINHAS; i++) {
        ui_definir_cor(0);
        ASSERT_EQ_INT(BANNER_COLUNAS, banner_linha(i, buf, sizeof buf));
        ASSERT_EQ_INT(BANNER_COLUNAS, ui_largura(buf));
        ui_definir_cor(1);
        ASSERT_EQ_INT(BANNER_COLUNAS, banner_linha(i, buf, sizeof buf));
        ASSERT_EQ_INT(BANNER_COLUNAS, ui_largura(buf));
    }
    ui_definir_cor(0);
#endif
}

static void teste_banner_cores(void)
{
#ifndef OHM_ASCII
    char buf[BANNER_TAM_LINHA];
    ui_definir_cor(1);
    banner_linha(0, buf, sizeof buf);                         /* topo creme      */
    ASSERT_TRUE(strstr(buf, "\033[38;2;255;232;176m\xE2\x96\x88") != NULL);
    ASSERT_TRUE(strstr(buf, "\033[38;2;138;74;0m\xE2\x95\x97") != NULL); /* sombra */
    banner_linha(BANNER_LINHAS - 2, buf, sizeof buf);         /* base laranja    */
    ASSERT_TRUE(strstr(buf, "\033[38;2;255;135;0m\xE2\x96\x88") != NULL);
    ASSERT_TRUE(strstr(buf, "\033[0m") != NULL);              /* termina com reset */
    ui_definir_cor(0);
#endif
}

static void teste_banner_linha_invalida(void)
{
    char buf[BANNER_TAM_LINHA];
    ASSERT_EQ_INT(0, banner_linha(-1, buf, sizeof buf));
    ASSERT_STR_EQ("", buf);
    ASSERT_EQ_INT(0, banner_linha(BANNER_LINHAS, buf, sizeof buf));
    ASSERT_STR_EQ("", buf);
}

static void teste_banner_fallback(void)
{
#ifdef OHM_ASCII
    ASSERT_EQ_INT(0, banner_disponivel(200));
#else
    ASSERT_EQ_INT(1, banner_disponivel(80));
    ASSERT_EQ_INT(1, banner_disponivel(BANNER_COLUNAS + 2));
    ASSERT_EQ_INT(0, banner_disponivel(BANNER_COLUNAS + 1)); /* estreito: logo antigo */
    ASSERT_EQ_INT(0, banner_disponivel(40));
#endif
}

/* ---- listar: barra de proporção ---------------------------------------- */

static void teste_listar_mostra_porcentagem(void)
{
    char buf[16384];
    FILE *f = abre_temp();
    Lista *li;
    Resistor a = { 220.0, 30 }, b = { 4700.0, 10 };
    if (f == NULL) return;

    li = cria_lista();
    insere_lista_ordenada(li, a);
    insere_lista_ordenada(li, b);
    ui_init();
    ui_definir_interativo(0);
    ui_definir_saida(f);
    ui_definir_cor(0);
    log_definir_pasta("build/test_listar_tmp/logs");
    tela_listar(li);
    log_definir_pasta(NULL);

    conteudo(f, buf, sizeof buf);
    ui_definir_saida(stdout);
    fclose(f);
    libera_lista(li);

    ASSERT_TRUE(strstr(buf, "Estoque") != NULL);
    ASSERT_TRUE(strstr(buf, "75%") != NULL);
    ASSERT_TRUE(strstr(buf, "25%") != NULL);
    ASSERT_TRUE(strstr(buf, "75%") < strstr(buf, "25%")); /* ordem crescente */
}

static void teste_listar_zerado_mostra_badge_vazio(void)
{
    char buf[16384];
    FILE *f = abre_temp();
    Lista *li;
    Resistor a = { 220.0, 0 }, b = { 4700.0, 10 };
    if (f == NULL) return;

    li = cria_lista();
    insere_lista_ordenada(li, a);
    insere_lista_ordenada(li, b);
    ui_init();
    ui_definir_interativo(0);
    ui_definir_saida(f);
    ui_definir_cor(0);
    tela_listar(li);

    conteudo(f, buf, sizeof buf);
    ui_definir_saida(stdout);
    fclose(f);
    libera_lista(li);

    ASSERT_TRUE(strstr(buf, "[ VAZIO ]") != NULL);
    ASSERT_TRUE(strstr(buf, "100%") != NULL);
}

int main(void)
{
    printf("=== test_telas ===\n");

    RUN_TEST(teste_navegar_baixo_simples);
    RUN_TEST(teste_navegar_baixo_para_sair);
    RUN_TEST(teste_navegar_baixo_de_sair);
    RUN_TEST(teste_navegar_cima_simples);
    RUN_TEST(teste_navegar_cima_para_sair);
    RUN_TEST(teste_navegar_cima_de_sair);
    RUN_TEST(teste_navegar_direita);
    RUN_TEST(teste_navegar_esquerda);
    RUN_TEST(teste_navegar_sem_mover_na_borda);
    RUN_TEST(teste_navegar_horizontal_de_sair);
    RUN_TEST(teste_navegar_tecla_ignorada);
    RUN_TEST(teste_tecla_comum);
    RUN_TEST(teste_tecla_setas);
    RUN_TEST(teste_tecla_fim_da_entrada);
    RUN_TEST(teste_tecla_esc_isolado_nao_trava);
    RUN_TEST(teste_tecla_esc_sequencia_desconhecida);
    RUN_TEST(teste_menu_esc_sai);
    RUN_TEST(teste_historico_opcao_9);
    RUN_TEST(teste_menu_mostra_historico_9);
    RUN_TEST(teste_tela_historico_mostra_acoes);
    RUN_TEST(teste_navegar_ignora_teclas_especiais);
    RUN_TEST(teste_menu_cor_laranja_na_borda);
    RUN_TEST(teste_menu_opcao_selecionada_laranja);
    RUN_TEST(teste_menu_so_laranja_e_branco);
    RUN_TEST(teste_checkpoint_navegar);
    RUN_TEST(teste_historico_volta_ao_checkpoint);
    RUN_TEST(teste_historico_checkpoint_enter_mantem);
    RUN_TEST(teste_historico_sem_checkpoint_nao_pergunta);

    RUN_TEST(teste_banner_sem_cor_literal);
    RUN_TEST(teste_banner_largura);
    RUN_TEST(teste_banner_cores);
    RUN_TEST(teste_banner_linha_invalida);
    RUN_TEST(teste_banner_fallback);
    RUN_TEST(teste_listar_mostra_porcentagem);
    RUN_TEST(teste_listar_zerado_mostra_badge_vazio);
    RUN_TEST(teste_menu_foco_chevron);
    RUN_TEST(teste_menu_foco_sem_cor_mantem_chevron);

    return tf_resumo("telas");
}
