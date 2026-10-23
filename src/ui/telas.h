/*
 * telas.h — telas do OhmFlow, na ordem do roteiro de navegação:
 *   [1] boas-vindas -> [2] menu -> [3] tela da função -> [4] feedback -> [2] ...
 *   [0] sair -> [5] encerramento
 */
#ifndef TELAS_H
#define TELAS_H

#include "lista.h"

/* Opções do menu principal */
enum {
    OPCAO_SAIR = 0,
    OPCAO_INSERIR,
    OPCAO_REMOVER,
    OPCAO_LISTAR,
    OPCAO_CONSULTAR,
    OPCAO_RETIRAR,
    OPCAO_REPOR,
    OPCAO_REINICIALIZAR,
    OPCAO_DIFERENCIAL,
    OPCAO_TOTAL
};

void tela_boas_vindas(void);
int  tela_menu(Lista *li); /* devolve OPCAO_*; EOF devolve OPCAO_SAIR */
void tela_inserir(Lista *li);
void tela_remover(Lista *li);
void tela_listar(Lista *li);
void tela_consultar(Lista *li);
void tela_retirar(Lista *li);
void tela_repor(Lista *li);
void tela_reinicializar(Lista *li);
void tela_diferencial(void);
void tela_encerramento(Lista *li);

#endif /* TELAS_H */
