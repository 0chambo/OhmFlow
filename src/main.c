/*
 * OhmFlow — controle de estoque de resistores de um almoxarifado.
 *
 * Projeto 01 de Estrutura de Dados e Recuperação da Informação: a estrutura
 * central é uma Lista Dinâmica Ordenada (lista.h), ordenada pela resistência.
 * Este arquivo apenas cria a lista, conduz o roteiro de telas (ui/telas.h)
 * e libera a memória ao final.
 */
#include <stdio.h>
#include <stdlib.h>
#include "lista.h"
#include "log.h"
#include "ui.h"
#include "telas.h"

int main(void)
{
    Lista *estoque;
    int opcao;

    ui_init();
    estoque = cria_lista(); /* inicialização da lista (vazia) */
    if (estoque == NULL) {
        fprintf(stderr, "Erro: memória insuficiente para criar a lista.\n");
        return EXIT_FAILURE;
    }
    /* cada execução é uma sessão no log do dia (logs/ohmflow_AAAA-MM-DD.log) */
    if (!log_iniciar())
        ui_mensagem(UI_AVISO, "Não foi possível abrir o arquivo de log; "
                    "seguindo sem registro.");

    tela_boas_vindas();
    while ((opcao = tela_menu(estoque)) != OPCAO_SAIR) {
        switch (opcao) {
        case OPCAO_INSERIR:       tela_inserir(estoque);       break;
        case OPCAO_REMOVER:       tela_remover(estoque);       break;
        case OPCAO_LISTAR:        tela_listar(estoque);        break;
        case OPCAO_CONSULTAR:     tela_consultar(estoque);     break;
        case OPCAO_RETIRAR:       tela_retirar(estoque);       break;
        case OPCAO_REPOR:         tela_repor(estoque);         break;
        case OPCAO_REINICIALIZAR: tela_reinicializar(estoque); break;
        case OPCAO_DIFERENCIAL:   tela_diferencial();          break;
        default:                                               break;
        }
    }
    tela_encerramento(estoque);
    log_encerrar();

    libera_lista(estoque);
    return EXIT_SUCCESS;
}
