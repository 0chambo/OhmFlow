/*
 * checkpoint.c — desfazer/refazer com duas pilhas (ver checkpoint.h).
 *
 * Toda inversa usa só a API pública da lista. Como as pilhas registram TODA
 * mudança do estoque na sessão, ao desfazer a ação do topo a lista está
 * exatamente como ficou logo depois dela, então a inversa sempre se aplica;
 * mesmo assim cada passo confere o retorno e, se falhar (falta de memória),
 * desfaz o que já tinha mudado e devolve a operação para a pilha de origem.
 */
#include <stdlib.h>
#include <string.h>
#include "checkpoint.h"

struct checkpoint {
    Pilha *desfazer; /* ações aplicadas (topo = mais recente) */
    Pilha *refazer;  /* ações desfeitas (topo = próxima)      */
};

Checkpoint *cria_checkpoint(void)
{
    Checkpoint *c = malloc(sizeof *c);
    if (c == NULL) return NULL;
    c->desfazer = cria_pilha();
    c->refazer = cria_pilha();
    if (c->desfazer == NULL || c->refazer == NULL) {
        libera_checkpoint(c);
        return NULL;
    }
    return c;
}

void libera_checkpoint(Checkpoint *c)
{
    if (c == NULL) return;
    libera_pilha(c->desfazer);
    libera_pilha(c->refazer);
    free(c);
}

int ck_posicao(const Checkpoint *c) { return c != NULL ? tamanho_pilha(c->desfazer) : 0; }

int ck_total(const Checkpoint *c)
{
    return c != NULL ? tamanho_pilha(c->desfazer) + tamanho_pilha(c->refazer) : 0;
}

/* ---- registro ---------------------------------------------------------- */

void ck_descarta(Checkpoint *c)
{
    if (c == NULL) return;
    esvazia_pilha(c->desfazer);
    esvazia_pilha(c->refazer);
}

int ck_registra(Checkpoint *c, Operacao op)
{
    if (c == NULL) {
        libera_operacao(&op);
        return CK_ERRO;
    }
    esvazia_pilha(c->refazer);
    op.quando = time(NULL);
    if (empilha(c->desfazer, op) != PILHA_OK) {
        /* sem esta ação, as inversas mais antigas não batem mais com a lista */
        ck_descarta(c);
        return CK_ERRO;
    }
    return CK_OK;
}

static Operacao operacao_vazia(int tipo)
{
    Operacao op;
    op.tipo = tipo;
    op.quando = 0;
    op.resistencia = 0;
    op.qtd = 0;
    op.itens = NULL;
    op.n_itens = 0;
    return op;
}

/* Registra uma operação com cópia de `n` itens */
static int registra_com_itens(Checkpoint *c, int tipo, const Resistor *itens, int n)
{
    Operacao op = operacao_vazia(tipo);
    if (c == NULL || itens == NULL || n <= 0) return CK_ERRO;
    op.itens = malloc((size_t)n * sizeof *op.itens);
    if (op.itens == NULL) {
        ck_descarta(c); /* mesma regra de ck_registra */
        return CK_ERRO;
    }
    memcpy(op.itens, itens, (size_t)n * sizeof *op.itens);
    op.n_itens = n;
    return ck_registra(c, op);
}

int ck_registra_novo(Checkpoint *c, const Resistor *itens, int n)
{
    return registra_com_itens(c, OP_NOVO, itens, n);
}

int ck_registra_remover(Checkpoint *c, Resistor removido)
{
    return registra_com_itens(c, OP_REMOVER, &removido, 1);
}

int ck_registra_movimento(Checkpoint *c, int tipo, double ohms, int qtd)
{
    Operacao op;
    if (c == NULL || (tipo != OP_RETIRAR && tipo != OP_REPOR)) return CK_ERRO;
    op = operacao_vazia(tipo);
    op.resistencia = ohms;
    op.qtd = qtd;
    return ck_registra(c, op);
}

/* Foto da lista: copia cada nó para o vetor do contexto */
typedef struct {
    Resistor *itens;
    int       n;
} Foto;

static void tira_foto(const Resistor *r, void *ctx)
{
    Foto *f = ctx;
    f->itens[f->n++] = *r;
}

int ck_registra_reinicializar(Checkpoint *c, const Lista *li)
{
    Foto f;
    int n, res;
    if (c == NULL || li == NULL) return CK_ERRO;
    n = tamanho_lista(li);
    if (n == 0) return CK_OK; /* nada a restaurar: não vira checkpoint */
    f.itens = malloc((size_t)n * sizeof *f.itens);
    f.n = 0;
    if (f.itens == NULL) {
        ck_descarta(c);
        return CK_ERRO;
    }
    percorre_lista(li, tira_foto, &f);
    res = registra_com_itens(c, OP_REINICIALIZAR, f.itens, f.n);
    free(f.itens);
    return res;
}

/* ---- aplicar inversas ----------------------------------------------------- */

/* Insere itens[0..n); se um falhar, remove os já inseridos */
static int insere_itens(Lista *li, const Resistor *itens, int n)
{
    int i;
    for (i = 0; i < n; i++) {
        if (insere_lista_ordenada(li, itens[i]) != LISTA_OK) {
            while (i-- > 0) remove_lista(li, itens[i].resistencia);
            return CK_ERRO;
        }
    }
    return CK_OK;
}

/* Remove itens[0..n); se um falhar, reinsere os já removidos */
static int remove_itens(Lista *li, const Resistor *itens, int n)
{
    int i;
    for (i = 0; i < n; i++) {
        if (remove_lista(li, itens[i].resistencia) != LISTA_OK) {
            while (i-- > 0) insere_lista_ordenada(li, itens[i]);
            return CK_ERRO;
        }
    }
    return CK_OK;
}

/* Desfaz (desfazer = 1) ou refaz (0) op na lista */
static int aplica(Lista *li, const Operacao *op, int desfazer)
{
    int res;
    switch (op->tipo) {
    case OP_NOVO:
        return desfazer ? remove_itens(li, op->itens, op->n_itens)
                        : insere_itens(li, op->itens, op->n_itens);
    case OP_REMOVER:
        return desfazer ? insere_itens(li, op->itens, op->n_itens)
                        : remove_itens(li, op->itens, op->n_itens);
    case OP_REINICIALIZAR:
        if (desfazer) return insere_itens(li, op->itens, op->n_itens);
        return reinicializa_lista(li) == LISTA_OK ? CK_OK : CK_ERRO;
    case OP_RETIRAR:
        res = desfazer ? repor_estoque(li, op->resistencia, op->qtd)
                       : retira_estoque(li, op->resistencia, op->qtd);
        return res == LISTA_OK ? CK_OK : CK_ERRO;
    case OP_REPOR:
        res = desfazer ? retira_estoque(li, op->resistencia, op->qtd)
                       : repor_estoque(li, op->resistencia, op->qtd);
        return res == LISTA_OK ? CK_OK : CK_ERRO;
    default:
        return CK_ERRO;
    }
}

/* Tira o topo de `de`, aplica e empilha em `para`; se a aplicação falhar,
 * a operação volta para `de` */
static int move_topo(Pilha *de, Pilha *para, Lista *li, int desfazer)
{
    Operacao op;
    if (desempilha(de, &op) != PILHA_OK) return CK_VAZIO;
    if (aplica(li, &op, desfazer) != CK_OK) {
        empilha(de, op);
        return CK_ERRO;
    }
    if (empilha(para, op) != PILHA_OK) {
        /* a lista mudou e a operação se perdeu: o resto do histórico não bate
         * mais com ela, então é descartado (a lista segue válida) */
        esvazia_pilha(de);
        esvazia_pilha(para);
        return CK_ERRO; /* = ck_descarta, sem o Checkpoint à mão */
    }
    return CK_OK;
}

int ck_desfazer(Checkpoint *c, Lista *li)
{
    if (c == NULL || li == NULL) return CK_ERRO;
    return move_topo(c->desfazer, c->refazer, li, 1);
}

int ck_refazer(Checkpoint *c, Lista *li)
{
    if (c == NULL || li == NULL) return CK_ERRO;
    return move_topo(c->refazer, c->desfazer, li, 0);
}

int ck_ir_para(Checkpoint *c, Lista *li, int alvo,
               void (*aviso)(int sentido, const Operacao *op, void *ctx), void *ctx)
{
    const Operacao *op;
    if (c == NULL || li == NULL) return CK_ERRO;
    if (alvo < 0 || alvo > ck_total(c)) return CK_INVALIDO;
    while (ck_posicao(c) > alvo) {
        if (ck_desfazer(c, li) != CK_OK) return CK_ERRO;
        if (aviso != NULL && topo_pilha(c->refazer, &op) == PILHA_OK)
            aviso(CK_DESFEITA, op, ctx);
    }
    while (ck_posicao(c) < alvo) {
        if (ck_refazer(c, li) != CK_OK) return CK_ERRO;
        if (aviso != NULL && topo_pilha(c->desfazer, &op) == PILHA_OK)
            aviso(CK_REFEITA, op, ctx);
    }
    return CK_OK;
}

/* ---- consulta ----------------------------------------------------------- */

/* Procura o nó na profundidade `alvo` (0 = topo) durante percorre_pilha */
typedef struct {
    int             alvo, atual;
    const Operacao *achada;
} Busca;

static void busca_profundidade(const Operacao *op, void *ctx)
{
    Busca *b = ctx;
    if (b->atual++ == b->alvo) b->achada = op;
}

const Operacao *ck_operacao(const Checkpoint *c, int i)
{
    Busca b;
    int pos = ck_posicao(c);
    if (c == NULL || i < 1 || i > ck_total(c)) return NULL;
    b.atual = 0;
    b.achada = NULL;
    if (i <= pos) {               /* aplicada: a i-ésima está pos - i abaixo do topo */
        b.alvo = pos - i;
        percorre_pilha(c->desfazer, busca_profundidade, &b);
    } else {                      /* desfeita: a pos+1 está no topo de refazer */
        b.alvo = i - pos - 1;
        percorre_pilha(c->refazer, busca_profundidade, &b);
    }
    return b.achada;
}
