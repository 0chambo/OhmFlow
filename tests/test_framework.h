/*
 * test_framework.h — mini-harness de testes unitários, sem dependências.
 *
 * Uso:
 *   static void teste_algo(void) { ASSERT_EQ_INT(2, 1 + 1); }
 *   int main(void) { RUN_TEST(teste_algo); return tf_resumo("suite"); }
 */
#ifndef TEST_FRAMEWORK_H
#define TEST_FRAMEWORK_H

#include <stdio.h>
#include <string.h>
#include <math.h>

static int tf_testes = 0;        /* testes executados            */
static int tf_testes_falhos = 0; /* testes com ao menos 1 falha  */
static int tf_asserts = 0;       /* asserts avaliados            */
static int tf_falhas_atual = 0;  /* falhas no teste corrente     */

#define TF_FALHA(...)                                                   \
    do {                                                                \
        tf_falhas_atual++;                                              \
        printf("    FALHA %s:%d: ", __FILE__, __LINE__);                \
        printf(__VA_ARGS__);                                            \
        putchar('\n');                                                  \
    } while (0)

#define ASSERT_TRUE(cond)                                               \
    do {                                                                \
        tf_asserts++;                                                   \
        if (!(cond)) TF_FALHA("esperado verdadeiro: %s", #cond);        \
    } while (0)

#define ASSERT_EQ_INT(esp, obt)                                         \
    do {                                                                \
        long long tf_e_ = (long long)(esp), tf_o_ = (long long)(obt);   \
        tf_asserts++;                                                   \
        if (tf_e_ != tf_o_)                                             \
            TF_FALHA("%s == %s: esperado %lld, obtido %lld",            \
                     #esp, #obt, tf_e_, tf_o_);                         \
    } while (0)

/* Tolerância relativa (mínimo absoluto = tol) */
#define ASSERT_EQ_DBL(esp, obt, tol)                                    \
    do {                                                                \
        double tf_e_ = (esp), tf_o_ = (obt);                            \
        double tf_m_ = fabs(tf_e_) > 1.0 ? fabs(tf_e_) : 1.0;           \
        tf_asserts++;                                                   \
        if (!(fabs(tf_e_ - tf_o_) <= (tol) * tf_m_))                    \
            TF_FALHA("%s == %s: esperado %.12g, obtido %.12g",          \
                     #esp, #obt, tf_e_, tf_o_);                         \
    } while (0)

#define ASSERT_STR_EQ(esp, obt)                                         \
    do {                                                                \
        const char *tf_e_ = (esp), *tf_o_ = (obt);                      \
        tf_asserts++;                                                   \
        if (tf_e_ == NULL || tf_o_ == NULL || strcmp(tf_e_, tf_o_) != 0)\
            TF_FALHA("%s == %s: esperado \"%s\", obtido \"%s\"",        \
                     #esp, #obt, tf_e_ ? tf_e_ : "(null)",              \
                     tf_o_ ? tf_o_ : "(null)");                         \
    } while (0)

static void tf_executa(const char *nome, void (*fn)(void))
{
    tf_falhas_atual = 0;
    tf_testes++;
    fn();
    if (tf_falhas_atual) {
        tf_testes_falhos++;
        printf("  [FALHOU] %s\n", nome);
    } else {
        printf("  [ok]     %s\n", nome);
    }
}

#define RUN_TEST(fn) tf_executa(#fn, fn)

static int tf_resumo(const char *suite)
{
    printf("== %s: %d testes, %d falharam, %d asserts ==\n",
           suite, tf_testes, tf_testes_falhos, tf_asserts);
    return tf_testes_falhos ? 1 : 0;
}

#endif /* TEST_FRAMEWORK_H */
