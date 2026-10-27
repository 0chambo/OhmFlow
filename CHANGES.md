# CHANGES — sistema de logs completo

## Resumo

Toda ação do OhmFlow passa a ser registrada num arquivo de log de texto, formatado em colunas e legível no Bloco de Notas. É a base do futuro Histórico (issue #27). Estoque continua só em RAM (persistência, #7, fica para depois).

## Formato

- Um arquivo por dia: `ohmflow_AAAA-MM-DD.log` (UTF-8). Cada execução é uma sessão acrescentada ao arquivo do dia.
- Cabeçalho `OhmFlow — sessão iniciada em dd/mm/aaaa HH:MM:SS` e rodapé `sessão encerrada em ... (Xm Ys)` / `(Xh Ym Zs)`, entre linhas de `=`.
- Linha de ação com colunas de largura fixa separadas por ` | `:
  `data hora | NÍVEL | AÇÃO | alvo | detalhe | resultado` (larguras 19, 5, 13, 10, 20, livre; padding conta caracteres UTF-8 como `Ω`, `→`, `·`).
- Campos vazios viram `-`; `|`, `\r`, `\n` e tab no texto do usuário são trocados (`/` ou espaço) para não quebrar as colunas; espaços sobrando no fim da linha são removidos.
- `fflush` após cada linha: o registro sobrevive a janela fechada ou travamento.

## O que é registrado (`src/ui/telas.c`)

- Novo auxiliar `registra(nivel, acao, alvo, resultado, fmt, ...)` e `feedback_cancelado(acao, alvo)` (agora também loga `CANCELADO`).
- `ler_resistencia`, `ler_quantidade` e `ler_cadastrado` recebem a ação (e o alvo) para registrar entradas inválidas como `ERRO` (`VALOR INVÁLIDO`, `QUANTIDADE INVÁLIDA`, texto digitado entre aspas) e resistor não cadastrado como `AVISO` (`NÃO ENCONTRADO`).
- Inserir: `OK` (qtd), `JÁ CADASTRADO`, `SEM MEMÓRIA`/`VALOR INVÁLIDO`, `CANCELADO`.
- Remover: `OK` (qtd removida), `NÃO ENCONTRADO`, `CANCELADO`.
- Listar: `OK` com `N tipos · M peças`.
- Consultar: `OK` (qtd), `NÃO ENCONTRADO`.
- Retirar: `OK` `-q (a→b)`, `OK - ESTOQUE ZERADO` (AVISO), `SEM ESTOQUE` `-q (tem a)`, `QUANTIDADE INVÁLIDA`.
- Repor: `OK` `+q (a→b)`, `LIMITE EXCEDIDO`.
- Reinicializar: `OK` com tipos e peças apagados, `CANCELADO`.
- Diferencial (placeholder): `EM BREVE`.
- Menu (modo não interativo): opção inválida como `ERRO` `OPÇÃO INVÁLIDA`.

## Novo módulo `src/log.h` / `src/log.c`

- API: `log_iniciar`, `log_encerrar`, `log_ativo`, `log_acao`, `log_formata_linha` (pura, testável), `log_caminho_do_dia`, e para testes `log_definir_pasta` / `log_definir_relogio`.
- Pasta, em ordem de prioridade: `log_definir_pasta` (testes) > variável `OHMFLOW_LOGS_DIR` > `%APPDATA%\OhmFlow\logs` (somente quando compilado com `-DOHM_LOGS_APPDATA`) > `logs` na pasta atual.
- Cria pastas intermediárias que faltarem (`_mkdir` no Windows, `mkdir` no POSIX).
- Best effort: se a pasta ou o arquivo falharem, `log_iniciar` devolve 0 e todas as chamadas viram no-op.
- Independente da UI (não inclui `ui.h`), seguindo a separação lógica/interface do projeto.

## `src/main.c`

- `log_iniciar()` logo após `ui_init()`; se falhar, um único aviso "Não foi possível abrir o arquivo de log; seguindo sem registro."
- `log_encerrar()` após a tela de encerramento.

## Build e instalador

- `Makefile`: `src/log.c` em `SRC_LIB`; `test_log` em `TESTES`.
- `make e2e`: roda com `OHMFLOW_LOGS_DIR=build/e2e_logs` (não suja o repositório) e confere no log o cabeçalho, `INSERIR 4,7 kΩ qtd 10 OK`, `RETIRAR ... SEM ESTOQUE`, `ERRO INSERIR "abc"` e o rodapé.
- `make setup`: compila um exe próprio `dist/instalador/ohmflow.exe` com `-DOHM_LOGS_APPDATA` e passa `/DExeOrigem=...` ao Inno Setup. Em desenvolvimento (`make`, `make run`) os logs vão para `.\logs`.
- `installer/ohmflow.iss`: `#define ExeOrigem` (padrão `dist\ohmflow.exe`) e `Source: "{#ExeOrigem}"; DestName: "ohmflow.exe"`. BOM UTF-8 preservado.
- `tests/setup/test_setup.ps1`: roda o exe instalado com `APPDATA` temporário e confere que o log foi criado em `%APPDATA%\OhmFlow\logs` com `INSERIR` e rodapé, e que nada foi gravado na pasta de instalação; apaga o APPDATA temporário (os logs reais do usuário não são tocados).
- `.gitignore`: `logs/`.

## Testes

- Novo `tests/test_log.c` (11 testes): alinhamento da linha, níveis, campos nulos, sanitização de `|`/quebras, truncamento em buffer pequeno, criação de pasta + cabeçalho, ação + rodapé com duração, duração com horas, segunda sessão anexada, pasta inválida sem quebrar, chamadas sem sessão como no-op.

## Documentação

- README: estrutura, nova seção "Registro de ações (logs)" com exemplo, tabela de pastas, observação no instalador e nos testes.
- AGENTS.md: arquitetura e comandos atualizados.

## Validação

- `make clean; make EXTRA_CFLAGS=-Werror all test e2e cppcheck`: tudo passa (log: 11 testes; e2e 36 verificações + checagem do log).
- `make teste-setup`: SETUP OK (17 verificações).
