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
#ifdef _WIN32
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#endif
#include "lista.h"
#include "ui.h"
#include "telas.h"

#ifdef _WIN32
/*
 * O executável usa o subsistema GUI (-mwindows): duplo clique abre uma janela
 * de console própria, sem passar pelo Windows Terminal. Se a saída já está
 * redirecionada (e2e/CI) nada é alterado; se foi lançado de um terminal,
 * reaproveita o console do pai.
 */
static void ligar_console(void)
{
    DWORD tipo = GetFileType(GetStdHandle(STD_OUTPUT_HANDLE));
    HANDLE saida, entrada;

    if (tipo == FILE_TYPE_PIPE || tipo == FILE_TYPE_DISK)
        return;

    if (!AttachConsole(ATTACH_PARENT_PROCESS))
        (void)AllocConsole();

    saida = CreateFileA("CONOUT$", GENERIC_READ | GENERIC_WRITE,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL, OPEN_EXISTING, 0, NULL);
    if (saida != INVALID_HANDLE_VALUE) {
        SetStdHandle(STD_OUTPUT_HANDLE, saida);
        SetStdHandle(STD_ERROR_HANDLE, saida);
        (void)freopen("CONOUT$", "w", stdout);
        (void)freopen("CONOUT$", "w", stderr);
    }

    tipo = GetFileType(GetStdHandle(STD_INPUT_HANDLE));
    if (tipo != FILE_TYPE_PIPE && tipo != FILE_TYPE_DISK) {
        entrada = CreateFileA("CONIN$", GENERIC_READ | GENERIC_WRITE,
                              FILE_SHARE_READ | FILE_SHARE_WRITE,
                              NULL, OPEN_EXISTING, 0, NULL);
        if (entrada != INVALID_HANDLE_VALUE) {
            SetStdHandle(STD_INPUT_HANDLE, entrada);
            (void)freopen("CONIN$", "r", stdin);
        }
    }
}
#endif

int main(void)
{
    Lista *estoque;
    int opcao;

#ifdef _WIN32
    ligar_console();
#endif
    ui_init();
    estoque = cria_lista(); /* inicialização da lista (vazia) */
    if (estoque == NULL) {
        fprintf(stderr, "Erro: memória insuficiente para criar a lista.\n");
        return EXIT_FAILURE;
    }

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

    libera_lista(estoque);
    return EXIT_SUCCESS;
}
