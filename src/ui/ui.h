/*
 * ui.h — primitivas de interface de terminal.
 *
 * Cores ANSI, molduras (caixas e tabelas), leitura de linhas, limpar tela e
 * pausa. Em modo não interativo (stdin/stdout redirecionados, CI, e2e) não há
 * cores, limpeza de tela nem pausa, e a entrada lida é ecoada na saída.
 * Cores também são desligadas pela variável de ambiente NO_COLOR.
 * Compilar com -DOHM_ASCII troca molduras/ícones Unicode por ASCII.
 */
#ifndef UI_H
#define UI_H

#include <stdio.h>
#include <stddef.h>

/* Largura interna padrão das caixas */
#define UI_LARGURA 38

/* Tipos de mensagem / feedback */
enum { UI_SUCESSO, UI_ERRO, UI_AVISO, UI_INFO };

/* Estilos de moldura */
enum { UI_MOLDURA_DUPLA, UI_MOLDURA_SIMPLES, UI_MOLDURA_ARREDONDADA };

/* Posição da borda de tabela */
enum { UI_TAB_TOPO, UI_TAB_MEIO, UI_TAB_BASE };

/* Cores (ui_cor devolve "" quando cor está desligada).
 * Paleta: PRIMARIA = laranja (bordas, prompt, destaque);
 * SECUNDARIA = branco (títulos, texto de apoio, info).
 * BORDA, PROMPT e TITULO derivam dessas duas. */
enum {
    UI_COR_RESET, UI_COR_NEGRITO, UI_COR_DIM,
    UI_COR_VERMELHO, UI_COR_VERDE, UI_COR_AMARELO, UI_COR_AZUL,
    UI_COR_MAGENTA, UI_COR_CIANO, UI_COR_BRANCO,
    UI_COR_BORDA, UI_COR_TITULO, UI_COR_PROMPT,
    UI_COR_LARANJA, UI_COR_VERDE_NEON, UI_COR_ROXO_NEON,
    UI_COR_PRIMARIA, UI_COR_SECUNDARIA,
    UI_COR_TOTAL
};

#ifdef OHM_ASCII
#define UI_ICONE_OK    "[OK]"
#define UI_ICONE_ERRO  "[ERRO]"
#define UI_ICONE_AVISO "[!]"
#define UI_ICONE_INFO  "[i]"
#define UI_PROMPT      ">>"
#define UI_SETA        "->"
#define UI_PONTO       "-"
#define UI_BARRA       "|"
#define UI_BULLET      "*"
#else
#define UI_ICONE_OK    "\xE2\x9C\x94" /* ✔ */
#define UI_ICONE_ERRO  "\xE2\x9C\x96" /* ✖ */
#define UI_ICONE_AVISO "\xE2\x9A\xA0" /* ⚠ */
#define UI_ICONE_INFO  "\xE2\x84\xB9" /* ℹ */
#define UI_PROMPT      "\xC2\xBB"     /* » */
#define UI_SETA        "\xE2\x86\x92" /* → */
#define UI_PONTO       "\xC2\xB7"     /* · */
#define UI_BARRA       "\xE2\x94\x82" /* │ */
#define UI_BULLET      "\xE2\x97\x8F" /* ● */
#endif

/* Estado / configuração */
void        ui_init(void);
void        ui_definir_saida(FILE *f);
void        ui_definir_cor(int ativa);
int         ui_cor_ativa(void);
int         ui_interativo(void);
void        ui_definir_interativo(int ativo); /* testes: força (ou não) modo terminal */
const char *ui_cor(int cor);

/* Tela */
void ui_limpar_tela(void);
void ui_pausar(const char *msg);
void ui_esperar_enter(void); /* espera Enter no terminal, sem ecoar o que for digitado */
/* Consome teclas de `proxima` até Enter ('\r' ou '\n'); 1 = Enter, 0 = EOF.
 * Prefixo 0/224 (tecla estendida do _getch) descarta também o byte seguinte. */
int  ui_espera_enter_de(int (*proxima)(void *ctx), void *ctx);
int  ui_colunas_terminal(void);
int  ui_linhas_terminal(void);
void ui_cursor(int visivel); /* 1 = mostra, 0 = esconde */

/* Modo centralizado (desligado por padrão; main liga quando é terminal):
 * caixas e tabelas saem centralizadas pela própria largura, e mensagens,
 * dicas e prompts alinhados a um bloco central de UI_LARGURA + 2 colunas */
void ui_definir_centralizado(int ativo);
int  ui_centralizado(void);
int  ui_margem(void);                 /* recuo do bloco central (0 se desligado) */
void ui_espaco_vertical(int altura);  /* linhas em branco p/ centralizar `altura` linhas */

/* Caixas */
void ui_caixa(int estilo, int cor_borda, const char *titulo_borda,
              const char *cabecalho, const char *const *linhas, int n,
              int largura_min);
void ui_caixa_centralizada(int estilo, int cor_borda, const char *titulo_borda,
                           const char *cabecalho, const char *const *linhas, int n,
                           int largura_min);
void ui_caixa_feedback(int tipo, const char *titulo, const char *fmt, ...);

/* Mensagens de uma linha */
void ui_mensagem(int tipo, const char *fmt, ...);
void ui_dica(const char *fmt, ...);

/* Entrada */
int  ui_ler_linha(const char *prompt, char *buf, size_t n); /* 0 em EOF */
int  ui_confirmar(const char *fmt, ...);                     /* 1 = sim  */

/* Alinhamento por colunas visíveis (UTF-8 e códigos ANSI considerados) */
size_t ui_largura(const char *s);
void   ui_pad(FILE *out, const char *s, int largura, int alinhar_dir);

/* Tabelas com bordas */
void ui_tabela_borda(int pos, const int *larguras, int ncol);
void ui_tabela_linha(const char *const *celulas, const int *larguras,
                     const int *alinhar_dir, int ncol);

#endif /* UI_H */
