/*
 * test_novo.c — testes da tela Novo (inserção unitária e em massa) e dos
 * ajustes de tela das issues #26/#28: parse_item, fluxos alimentados por
 * stdin (via freopen) e cores da tabela de estoque.
 */
#include <stdlib.h>
#include <string.h>
#include "test_framework.h"
#include "telas.h"
#include "ui.h"
#include "lista.h"

#define TAM_SAIDA 32768
#define ARQ_ENTRADA "build/_tmp_test_novo_entrada.txt"

static char saida_buf[TAM_SAIDA];

/* ---- auxiliares ---------------------------------------------------------- */

static FILE *abre_temp(void)
{
    FILE *f = tmpfile();
    if (f == NULL) f = fopen("build/_tmp_test_novo.txt", "w+b");
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

/* Faz stdin ler `texto`; 0 se não conseguiu (o teste é pulado) */
static int entrada(const char *texto)
{
    FILE *f = fopen(ARQ_ENTRADA, "wb");
    if (f == NULL) return 0;
    fputs(texto, f);
    fclose(f);
    return freopen(ARQ_ENTRADA, "rb", stdin) != NULL;
}

/* Prepara a ui sem terminal (sem pausa nem limpeza de tela) gravando em f */
static void prepara_ui(FILE *f, int cor)
{
    ui_init();
    ui_definir_interativo(0);
    ui_definir_saida(f);
    ui_definir_cor(cor);
}

static void restaura_ui(FILE *f)
{
    conteudo(f, saida_buf, sizeof saida_buf);
    ui_definir_saida(stdout);
    ui_definir_cor(0);
    fclose(f);
}

static Resistor resistor(double ohms, int qtd)
{
    Resistor r;
    r.resistencia = ohms;
    r.quantidade = qtd;
    return r;
}

/* ---- parse_item ---------------------------------------------------------- */

static void teste_parse_item_completo(void)
{
    Resistor r;
    ASSERT_EQ_INT(ITEM_COMPLETO, parse_item("4k7 50", &r));
    ASSERT_EQ_DBL(4700.0, r.resistencia, 1e-12);
    ASSERT_EQ_INT(50, r.quantidade);

    ASSERT_EQ_INT(ITEM_COMPLETO, parse_item("  1M\t2  ", &r));
    ASSERT_EQ_DBL(1e6, r.resistencia, 1e-12);
    ASSERT_EQ_INT(2, r.quantidade);

    ASSERT_EQ_INT(ITEM_COMPLETO, parse_item("470m 0", &r)); /* quantidade 0 vale */
    ASSERT_EQ_INT(0, r.quantidade);
}

static void teste_parse_item_so_resistencia(void)
{
    Resistor r;
    ASSERT_EQ_INT(ITEM_SO_RESISTENCIA, parse_item("220", &r));
    ASSERT_EQ_DBL(220.0, r.resistencia, 1e-12);
    ASSERT_EQ_INT(ITEM_SO_RESISTENCIA, parse_item("  4.7k  ", &r));
    ASSERT_EQ_DBL(4700.0, r.resistencia, 1e-12);
}

static void teste_parse_item_invalido(void)
{
    Resistor r;
    ASSERT_EQ_INT(ITEM_INVALIDO, parse_item("", &r));
    ASSERT_EQ_INT(ITEM_INVALIDO, parse_item("   ", &r));
    ASSERT_EQ_INT(ITEM_INVALIDO, parse_item("abc 5", &r));      /* resistência ruim */
    ASSERT_EQ_INT(ITEM_INVALIDO, parse_item("4k7 abc", &r));    /* quantidade ruim  */
    ASSERT_EQ_INT(ITEM_INVALIDO, parse_item("4k7 -1", &r));     /* negativa         */
    ASSERT_EQ_INT(ITEM_INVALIDO, parse_item("4k7 5x", &r));
    ASSERT_EQ_INT(ITEM_INVALIDO, parse_item("4k7 2147483648", &r)); /* > INT_MAX */
    ASSERT_EQ_INT(ITEM_INVALIDO, parse_item("4k7 5 9", &r));    /* campo a mais     */
    ASSERT_EQ_INT(ITEM_INVALIDO, parse_item("2000M 5", &r));    /* fora da faixa    */
    ASSERT_EQ_INT(ITEM_INVALIDO, parse_item(NULL, &r));
}

static void teste_parse_item_limite_int(void)
{
    Resistor r;
    ASSERT_EQ_INT(ITEM_COMPLETO, parse_item("1k 2147483647", &r));
    ASSERT_EQ_INT(2147483647, r.quantidade);
}

/* ---- tela Novo: fluxo unitário (comportamento antigo) -------------------- */

static void teste_novo_unitario(void)
{
    Lista *li;
    FILE *f = abre_temp();
    int qtd = -1;
    if (f == NULL || !entrada("4k7\n10\n")) { if (f) fclose(f); return; }

    li = cria_lista();
    prepara_ui(f, 0);
    tela_inserir(li);
    restaura_ui(f);

    ASSERT_EQ_INT(1, tamanho_lista(li));
    ASSERT_EQ_INT(LISTA_OK, consulta_lista(li, 4700.0, &qtd));
    ASSERT_EQ_INT(10, qtd);
    ASSERT_TRUE(strstr(saida_buf, "Novo resistor") != NULL);
    ASSERT_TRUE(strstr(saida_buf, "inserido (10 un.)") != NULL);
    ASSERT_TRUE(strstr(saida_buf, "Inseridos") == NULL); /* sem resumo em massa */
    libera_lista(li);
}

static void teste_novo_unitario_duplicado(void)
{
    Lista *li;
    FILE *f = abre_temp();
    if (f == NULL || !entrada("4k7\n")) { if (f) fclose(f); return; }

    li = cria_lista();
    insere_lista_ordenada(li, resistor(4700.0, 3));
    prepara_ui(f, 0);
    tela_inserir(li);
    restaura_ui(f);

    ASSERT_EQ_INT(1, tamanho_lista(li));
    ASSERT_TRUE(strstr(saida_buf, "já cadastrado") != NULL);
    libera_lista(li);
}

/* ---- tela Novo: inserção em massa ---------------------------------------- */

static void teste_novo_em_massa(void)
{
    Lista *li;
    FILE *f = abre_temp();
    int qtd = -1;
    /* 3 válidos (um só com resistência + quantidade em separado),
     * 2 duplicados (um já no estoque, outro repetido no lote), 2 inválidos */
    const char *roteiro =
        "10k 5\n"
        "22k 3\n"
        "220 1\n"      /* duplicado: já cadastrado */
        "lixo 2\n"     /* inválido */
        "10k 9\n"      /* duplicado: repetido no lote */
        "33k\n"        /* só resistência: pede a quantidade */
        "7\n"
        "1k abc\n"     /* inválido */
        "\n";          /* Enter vazio encerra */
    if (f == NULL || !entrada(roteiro)) { if (f) fclose(f); return; }

    li = cria_lista();
    insere_lista_ordenada(li, resistor(220.0, 50));
    prepara_ui(f, 0);
    tela_inserir(li);
    restaura_ui(f);

    ASSERT_EQ_INT(4, tamanho_lista(li));
    ASSERT_EQ_INT(LISTA_OK, consulta_lista(li, 10e3, &qtd));
    ASSERT_EQ_INT(5, qtd); /* duplicado não alterou a quantidade */
    ASSERT_EQ_INT(LISTA_OK, consulta_lista(li, 22e3, &qtd));
    ASSERT_EQ_INT(3, qtd);
    ASSERT_EQ_INT(LISTA_OK, consulta_lista(li, 33e3, &qtd));
    ASSERT_EQ_INT(7, qtd);
    ASSERT_EQ_INT(LISTA_OK, consulta_lista(li, 220.0, &qtd));
    ASSERT_EQ_INT(50, qtd);
    ASSERT_EQ_INT(65, total_pecas(li));

    ASSERT_TRUE(strstr(saida_buf, "Inseridos : 3") != NULL);
    ASSERT_TRUE(strstr(saida_buf, "Duplicados: 2") != NULL);
    ASSERT_TRUE(strstr(saida_buf, "Inválidos : 2") != NULL);
    libera_lista(li);
}

static void teste_novo_em_massa_eof_encerra(void)
{
    Lista *li;
    FILE *f = abre_temp();
    if (f == NULL || !entrada("1k 1\n2k 2")) { if (f) fclose(f); return; } /* sem \n final */

    li = cria_lista();
    prepara_ui(f, 0);
    tela_inserir(li);
    restaura_ui(f);

    ASSERT_EQ_INT(2, tamanho_lista(li));
    ASSERT_TRUE(strstr(saida_buf, "Inseridos : 2") != NULL);
    libera_lista(li);
}

static void teste_novo_primeira_linha_invalida_tenta_de_novo(void)
{
    Lista *li;
    FILE *f = abre_temp();
    if (f == NULL || !entrada("abc\n1k 4\n\n")) { if (f) fclose(f); return; }

    li = cria_lista();
    prepara_ui(f, 0);
    tela_inserir(li);
    restaura_ui(f);

    ASSERT_EQ_INT(1, tamanho_lista(li));
    ASSERT_TRUE(strstr(saida_buf, "Valor inválido") != NULL);
    ASSERT_TRUE(strstr(saida_buf, "Inseridos : 1") != NULL);
    libera_lista(li);
}

/* ---- #26: menu "Novo", Consultar sem debug, aviso no Reinicializar ------- */

static void teste_menu_rotulo_novo(void)
{
    Lista *li;
    FILE *f = abre_temp();
    if (f == NULL) return;

    li = cria_lista();
    prepara_ui(f, 0);
    desenha_menu(li, 1);
    restaura_ui(f);

    ASSERT_TRUE(strstr(saida_buf, "[1] Novo") != NULL);
    ASSERT_TRUE(strstr(saida_buf, "Inserir") == NULL);
    libera_lista(li);
}

static void teste_listar_vazio_sugere_novo(void)
{
    Lista *li;
    FILE *f = abre_temp();
    if (f == NULL) return;

    li = cria_lista();
    prepara_ui(f, 0);
    tela_listar(li);
    restaura_ui(f);

    ASSERT_TRUE(strstr(saida_buf, "[1] Novo") != NULL);
    libera_lista(li);
}

static void teste_consultar_sem_nos_visitados(void)
{
    Lista *li;
    FILE *f = abre_temp();
    if (f == NULL || !entrada("4k7\n")) { if (f) fclose(f); return; }

    li = cria_lista();
    insere_lista_ordenada(li, resistor(4700.0, 7));
    prepara_ui(f, 0);
    tela_consultar(li);
    restaura_ui(f);

    ASSERT_TRUE(strstr(saida_buf, "7 unidades") != NULL);
    ASSERT_TRUE(strstr(saida_buf, "Busca:") == NULL);
    ASSERT_TRUE(strstr(saida_buf, "visitado") == NULL);
    libera_lista(li);
}

static void teste_reinicializar_avisa_antes(void)
{
    Lista *li;
    FILE *f = abre_temp();
    const char *aviso, *pergunta;
    if (f == NULL || !entrada("n\n")) { if (f) fclose(f); return; }

    li = cria_lista();
    insere_lista_ordenada(li, resistor(4700.0, 7));
    prepara_ui(f, 0);
    tela_reinicializar(li);
    restaura_ui(f);

    aviso = strstr(saida_buf, "Tem certeza disso, você está apagando sua base de dados.");
    pergunta = strstr(saida_buf, "Apagar TODOS");
    ASSERT_TRUE(aviso != NULL);
    ASSERT_TRUE(pergunta != NULL);
    ASSERT_TRUE(aviso != NULL && pergunta != NULL && aviso < pergunta);
    ASSERT_EQ_INT(1, tamanho_lista(li)); /* respondeu n: nada apagado */
    libera_lista(li);
}

static void teste_reinicializar_vazio_sem_aviso(void)
{
    Lista *li;
    FILE *f = abre_temp();
    if (f == NULL) return;

    li = cria_lista();
    prepara_ui(f, 0);
    tela_reinicializar(li);
    restaura_ui(f);

    ASSERT_TRUE(strstr(saida_buf, "Tem certeza disso") == NULL);
    libera_lista(li);
}

/* ---- #28: valores da tabela sem cor por grandeza ------------------------- */

static void teste_listar_valores_sem_cor(void)
{
    Lista *li;
    FILE *f = abre_temp();
    if (f == NULL) return;

    li = cria_lista();
    insere_lista_ordenada(li, resistor(470e-3, 1));
    insere_lista_ordenada(li, resistor(4700.0, 2));
    insere_lista_ordenada(li, resistor(1e6, 0));
    prepara_ui(f, 1); /* cor ligada */
    tela_listar(li);
    restaura_ui(f);

    /* o valor vem logo após o espaço da célula, sem código de cor antes */
    ASSERT_TRUE(strstr(saida_buf, "     470 m\xCE\xA9") != NULL);
    ASSERT_TRUE(strstr(saida_buf, "     4,7 k\xCE\xA9") != NULL);
    ASSERT_TRUE(strstr(saida_buf, "       1 M\xCE\xA9") != NULL);
    ASSERT_TRUE(strstr(saida_buf, "\033[34m") == NULL); /* azul (MΩ)    */
    ASSERT_TRUE(strstr(saida_buf, "\033[35m") == NULL); /* magenta (mΩ) */
    ASSERT_TRUE(strstr(saida_buf, "\033[31m0") != NULL); /* qtd 0 segue vermelha */
    libera_lista(li);
}

/* ---- main ---------------------------------------------------------------- */

int main(void)
{
    printf("=== test_novo ===\n");

    RUN_TEST(teste_parse_item_completo);
    RUN_TEST(teste_parse_item_so_resistencia);
    RUN_TEST(teste_parse_item_invalido);
    RUN_TEST(teste_parse_item_limite_int);
    RUN_TEST(teste_novo_unitario);
    RUN_TEST(teste_novo_unitario_duplicado);
    RUN_TEST(teste_novo_em_massa);
    RUN_TEST(teste_novo_em_massa_eof_encerra);
    RUN_TEST(teste_novo_primeira_linha_invalida_tenta_de_novo);
    RUN_TEST(teste_menu_rotulo_novo);
    RUN_TEST(teste_listar_vazio_sugere_novo);
    RUN_TEST(teste_consultar_sem_nos_visitados);
    RUN_TEST(teste_reinicializar_avisa_antes);
    RUN_TEST(teste_reinicializar_vazio_sem_aviso);
    RUN_TEST(teste_listar_valores_sem_cor);

    remove(ARQ_ENTRADA);
    return tf_resumo("novo");
}
