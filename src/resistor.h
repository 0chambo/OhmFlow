/*
 * resistor.h — código de cores de 4 faixas e desenho do resistor.
 *
 * Código de cores (codigo_cores), conforme a tabela de cores do resistor:
 *   1ª faixa  = 1º algarismo (preto 0 ... branco 9)
 *   2ª faixa  = 2º algarismo (preto 0 ... branco 9)
 *   3ª faixa  = multiplicador: nº de zeros (preto 0 ... branco 9),
 *               dourado x0,1 ou prata x0,01
 *   4ª faixa  = tolerância, sempre dourada (5%)
 * Valores com 3 ou mais algarismos significativos são arredondados para 2;
 * abaixo de 0,1 Ω não há código de 4 faixas.
 *
 * Desenho (resistor_arte): pixel-art de 32x12 (mesma forma do
 * src/assets/ressistor_preencher.svg) em RESISTOR_LINHAS linhas de texto com
 * meio-bloco (▀/▄) e cor ANSI 24 bits. Sem faixas, só o contorno e os
 * terminais; com faixas, corpo bege e as quatro faixas coloridas.
 */
#ifndef RESISTOR_H
#define RESISTOR_H

#include <stddef.h>

/* Cores das faixas, na ordem do código (o valor é o algarismo/nº de zeros) */
enum {
    COR_PRETO, COR_MARROM, COR_VERMELHO, COR_LARANJA, COR_AMARELO,
    COR_VERDE, COR_AZUL, COR_VIOLETA, COR_CINZA, COR_BRANCO,
    COR_DOURADO, COR_PRATA,
    RESISTOR_CORPO,    /* bege do corpo (só para resistor_rgb_faixa) */
    RESISTOR_CONTORNO, /* contorno do desenho                        */
    RESISTOR_TERMINAL  /* fios dos terminais                         */
};

/* Faixa sem cor definida: desenhada com a cor do corpo */
#define SEM_FAIXA (-1)

/* Retorno de codigo_cores */
enum { CORES_EXATO, CORES_APROXIMADO, CORES_FORA };

#define RESISTOR_FAIXAS    4
#define RESISTOR_LINHAS    6    /* 12 pixels de altura, 2 por linha        */
#define RESISTOR_COLUNAS   32
#define RESISTOR_RECUO     2    /* espaços à esquerda (centra na caixa)    */
#define RESISTOR_TAM_LINHA 2048 /* cabe 32 células com cor de frente/fundo */

/* Preenche faixas[0..3] (algarismo, algarismo, multiplicador, dourado).
 * CORES_EXATO: o código representa o valor; CORES_APROXIMADO: arredondado
 * para 2 algarismos; CORES_FORA: sem código (faixas 0..2 = SEM_FAIXA) */
int codigo_cores(double ohms, int faixas[RESISTOR_FAIXAS]);

/* Nome da cor em pt-BR ("violeta", "dourado"); "-" se inválida/SEM_FAIXA */
const char *nome_cor_faixa(int cor);

/* Escreve em buf ";2;R;G;Bm", trecho da sequência ANSI 24 bits da cor */
void resistor_rgb_faixa(int cor, char *buf, size_t n);

/* Desenha o resistor; faixas NULL = sem cor (só contorno e terminais).
 * Cada linha tem RESISTOR_RECUO + RESISTOR_COLUNAS colunas visíveis e
 * termina com reset de cor */
void resistor_arte(const int *faixas, char linhas[RESISTOR_LINHAS][RESISTOR_TAM_LINHA]);

#endif /* RESISTOR_H */
