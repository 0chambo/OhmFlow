/*
 * log.h — registro das ações do OhmFlow em arquivo de texto.
 *
 * Um arquivo por dia (logs/ohmflow_AAAA-MM-DD.log, UTF-8). Cada execução do
 * programa é uma sessão: cabeçalho ao iniciar, uma linha por ação e rodapé com
 * a duração ao encerrar. Linha de ação (colunas de largura fixa, separadas
 * por " | ", fáceis de ler e de quebrar depois na tela de Histórico):
 *
 *   27/10/2026 14:03:20 | INFO  | NOVO          | 4,7 kΩ     | qtd 50               | OK
 *
 * Pasta dos logs: OHMFLOW_LOGS_DIR, se definida; senão %APPDATA%\OhmFlow\logs
 * no build do instalador (-DOHM_LOGS_APPDATA); senão .\logs (desenvolvimento).
 * O log é "best effort": se a pasta ou o arquivo falharem, as funções viram
 * no-op e o programa segue normalmente.
 */
#ifndef LOG_H
#define LOG_H

#include <stddef.h>
#include <time.h>

/* Níveis de uma linha de ação */
enum { LOG_INFO = 0, LOG_AVISO, LOG_ERRO };

/* Abre (ou cria) o arquivo do dia e escreve o cabeçalho; 1 se ok, 0 se falhou */
int  log_iniciar(void);

/* Escreve o rodapé com a duração da sessão e fecha o arquivo */
void log_encerrar(void);

/* 1 se há sessão aberta */
int  log_ativo(void);

/* Registra uma ação; campos NULL viram "-". "|" e quebras de linha do texto
 * são trocados para não desalinhar as colunas */
void log_acao(int nivel, const char *acao, const char *alvo,
              const char *detalhe, const char *resultado);

/* Formata uma linha de ação (sem '\n') em buf; devolve o tamanho escrito */
size_t log_formata_linha(char *buf, size_t n, time_t t, int nivel, const char *acao,
                         const char *alvo, const char *detalhe, const char *resultado);

/* Caminho do arquivo do dia para a pasta e o relógio atuais; 0 se não couber */
int  log_caminho_do_dia(char *buf, size_t n);

/* ---- leitura (tela Histórico) ---- */

/* Quantos dias para trás log_ultimas procura arquivos */
#define LOG_DIAS_HISTORICO 30

/* Uma linha de ação lida de volta do arquivo, campo a campo (sem o preenchimento) */
typedef struct {
    char data[24];      /* "27/10/2026 14:03:20" */
    char nivel[8];      /* INFO, AVISO ou ERRO   */
    char acao[24];
    char alvo[64];
    char detalhe[64];
    char resultado[64];
} LogEntrada;

/* 1 se `linha` é uma linha de ação (6 colunas, começa com data) e preenche *e;
 * 0 para cabeçalho, rodapé, linha vazia ou qualquer outra coisa */
int  log_parse_linha(const char *linha, LogEntrada *e);

/* Lê os arquivos dos últimos LOG_DIAS_HISTORICO dias e guarda em v as `max`
 * ações mais recentes, em ordem cronológica; devolve quantas guardou */
int  log_ultimas(LogEntrada *v, int max);

/* Para testes: força a pasta (NULL volta ao padrão) e o relógio (NULL = time) */
void log_definir_pasta(const char *pasta);
void log_definir_relogio(time_t (*agora)(void));

#endif /* LOG_H */
