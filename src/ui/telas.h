/*
 * telas.h — telas do OhmFlow, na ordem do roteiro de navegação:
 *   [1] boas-vindas -> [2] menu -> [3] tela da função -> [4] feedback -> [2] ...
 *   Esc ou tecla 0 no menu -> [5] encerramento
 */
#ifndef TELAS_H
#define TELAS_H

#include "lista.h"

/* Opções do menu principal. [9] Histórico ocupa a linha de baixo, onde
 * ficava o antigo "[0] Sair" */
enum {
    OPCAO_INSERIR = 1,
    OPCAO_REMOVER,
    OPCAO_LISTAR,
    OPCAO_CONSULTAR,
    OPCAO_RETIRAR,
    OPCAO_REPOR,
    OPCAO_REINICIALIZAR,
    OPCAO_DIFERENCIAL,
    OPCAO_HISTORICO,
    OPCAO_TOTAL
};

/* Sair não é item do menu: Esc, tecla 0 ou fim da entrada */
#define OPCAO_SAIR (-2)

/* Teclas de navegação para navegar_menu() */
#define KEY_UP    256
#define KEY_DOWN  257
#define KEY_LEFT  258
#define KEY_RIGHT 259
#define KEY_FIM      260 /* fim da entrada (EOF, Ctrl+D): menu encerra como Sair */
#define KEY_IGNORADA 261 /* Alt+tecla ou sequência desconhecida               */
#define KEY_ESC      262 /* tecla Esc sozinha: no menu, aciona Sair na hora   */

/* Retorno de menu_tecla_opcao() quando a tecla não encerra o menu */
#define MENU_CONTINUA (-1)

/* Retorno de `proximo` quando nenhum byte chega dentro do prazo curto */
#define TECLA_TIMEOUT (-2)

/*
 * Converte os bytes de uma tecla em caractere ou KEY_*. `proximo(ctx, 1)`
 * espera o primeiro byte (EOF no fim da entrada); os bytes seguintes de uma
 * sequência ESC são lidos com `proximo(ctx, 0)`, que devolve TECLA_TIMEOUT
 * se nada chegar, para que ESC sozinho não trave à espera de outra tecla.
 */
int decodifica_tecla(int (*proximo)(void *ctx, int bloqueia), void *ctx);

/* Retorna nova seleção (1-9) dado a seleção atual e uma tecla KEY_* */
int navegar_menu(int selecao, int tecla);

/* Opção escolhida pela tecla no menu (Esc/0/EOF -> OPCAO_SAIR, dígito 1-9,
 * Enter na `selecao`) ou MENU_CONTINUA se a tecla só navega ou é ignorada */
int menu_tecla_opcao(int tecla, int selecao);

/* Redesenha o menu com a opção `selecao` destacada */
void desenha_menu(const Lista *li, int selecao);

/* Resultado de parse_item() */
enum { ITEM_INVALIDO = 0, ITEM_SO_RESISTENCIA, ITEM_COMPLETO };

/* Interpreta uma linha da tela Novo: "<resistência> [quantidade]", ex.:
 * "4k7 50" -> ITEM_COMPLETO; "4k7" -> ITEM_SO_RESISTENCIA (quantidade 0);
 * qualquer outra coisa (campo a mais, valor ou quantidade inválidos) ->
 * ITEM_INVALIDO. Quantidade: inteiro de 0 a INT_MAX. */
int parse_item(const char *s, Resistor *r);

void tela_boas_vindas(void);
int  tela_menu(const Lista *li); /* devolve OPCAO_*; EOF devolve OPCAO_SAIR */
void tela_inserir(Lista *li);
void tela_remover(Lista *li);
void tela_listar(const Lista *li);
void tela_consultar(Lista *li);
void tela_retirar(Lista *li);
void tela_repor(Lista *li);
void tela_reinicializar(Lista *li);
void tela_diferencial(void);
void tela_historico(void); /* últimas ações lidas dos logs */
void tela_encerramento(const Lista *li);

#endif /* TELAS_H */
