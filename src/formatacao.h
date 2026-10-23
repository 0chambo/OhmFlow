/*
 * formatacao.h — conversão texto <-> resistência.
 *
 * Entrada aceita (parse_resistencia):
 *   4700 · 0.47 · 0,47 · 4.7k · 4k7 · 4K7 · 10M · 1M5 · 470m · 4R7
 *   sufixo opcional "ohm"/"ohms"/"Ω"; espaços nas pontas e antes do multiplicador.
 *   Multiplicadores: m = mili, R/r = 1, k/K = kilo, M = mega (m e M diferem!).
 *
 * Saída (formata_resistencia): ordem de grandeza mΩ, Ω, kΩ ou MΩ, até 3 casas,
 * sem zeros à direita e com vírgula decimal: "470 mΩ", "4,7 kΩ", "2,2 MΩ".
 */
#ifndef FORMATACAO_H
#define FORMATACAO_H

#include <stddef.h>

#ifdef OHM_ASCII
#define SIMBOLO_OHM "ohm"
#else
#define SIMBOLO_OHM "\xCE\xA9" /* Ω em UTF-8 */
#endif

/* Grandezas retornadas por grandeza_resistencia */
enum { GRANDEZA_MILI = 0, GRANDEZA_UNIDADE, GRANDEZA_KILO, GRANDEZA_MEGA };

/* 1 se válido (e grava em *ohms), 0 se inválido ou fora da faixa da lista */
int  parse_resistencia(const char *s, double *ohms);

/* Grandeza usada na impressão (já considera o arredondamento p/ 3 casas) */
int  grandeza_resistencia(double ohms);

/* Escreve em buf (tamanho n) o valor formatado, ex.: "4,7 kΩ" */
void formata_resistencia(double ohms, char *buf, size_t n);

#endif /* FORMATACAO_H */
