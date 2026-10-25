/*
 * lista.h — TAD Lista Dinâmica Ordenada de resistores.
 *
 * Adaptação da Lista Dinâmica (encadeada) vista em aula: cada nó guarda um
 * Resistor e a lista é mantida SEMPRE em ordem crescente de resistência (Ω).
 * "Lista" é um ponteiro para o primeiro nó; as funções recebem Lista*
 * (ponteiro para a cabeça) para poder alterar o início da lista.
 */
#ifndef LISTA_H
#define LISTA_H

/* Faixa aceita de resistência: 1 mΩ até 1000 MΩ */
#define RESISTENCIA_MIN 1e-3
#define RESISTENCIA_MAX 1e9

typedef struct {
    double resistencia; /* valor em Ω (chave de ordenação)   */
    int    quantidade;  /* unidades disponíveis no estoque    */
} Resistor;

typedef struct elemento *Lista;

/* Códigos de retorno */
enum {
    LISTA_OK             =  1,
    LISTA_ERRO           =  0, /* lista NULL ou falta de memória      */
    LISTA_NAO_ENCONTRADO = -1, /* resistência não cadastrada          */
    LISTA_DUPLICADO      = -2, /* resistência já cadastrada           */
    LISTA_SEM_ESTOQUE    = -3, /* retirada maior que o disponível     */
    LISTA_INVALIDO       = -4  /* valor/quantidade fora do permitido  */
};

Lista *cria_lista(void);
void   libera_lista(Lista *li);
int    reinicializa_lista(Lista *li);

int    tamanho_lista(Lista *li);
long long total_pecas(Lista *li); /* long long: long tem 32 bits no Windows */
int    lista_vazia(Lista *li);

int    insere_lista_ordenada(Lista *li, Resistor r);
int    remove_lista(Lista *li, double ohms);
int    consulta_lista(Lista *li, double ohms, int *qtd);
int    retira_estoque(Lista *li, double ohms, int qtd);
int    repor_estoque(Lista *li, double ohms, int qtd);

/* Chama f(resistor, ctx) para cada nó, em ordem crescente */
void   percorre_lista(Lista *li, void (*f)(const Resistor *r, void *ctx), void *ctx);

/* 1 se a e b representam a mesma resistência (tolerância relativa 1e-9) */
int    mesma_resistencia(double a, double b);

#endif /* LISTA_H */
