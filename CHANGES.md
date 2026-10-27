# CHANGES

## [9] Histórico no lugar do "[0] Sair"

A linha de baixo do menu deixa de ser "[0] Sair" e vira **[9] Histórico**. A tecla `0` continua saindo do programa, igual ao `Esc`, mas Sair não aparece mais na caixa do menu.

### Menu (`src/ui/telas.h`, `src/ui/telas.c`, `src/main.c`)
- Enum: `OPCAO_INSERIR = 1` ... `OPCAO_DIFERENCIAL = 8`, `OPCAO_HISTORICO = 9`, `OPCAO_TOTAL = 10`.
- `OPCAO_SAIR` vira `#define OPCAO_SAIR (-2)`, fora do menu: acionado por `Esc`, tecla `0` e fim da entrada.
- `navegar_menu`: a linha de baixo (antes seleção 0 = Sair) agora é `OPCAO_HISTORICO`; navegação igual.
- `menu_tecla_opcao`: `0` -> `OPCAO_SAIR`; atalhos `1` a `9`.
- `desenha_menu`: linha "● [9] Histórico"; dica "[1-9] Acesso rápido · [Enter] Confirmar · [Esc/0] Sair".
- Menu não interativo (e2e): `0` sai como antes; `9` abre o Histórico.
- `main.c`: `case OPCAO_HISTORICO: tela_historico();`.

### Leitura dos logs (`src/log.h`, `src/log.c`)
- `LogEntrada` (data, nível, ação, alvo, detalhe, resultado) e `LOG_DIAS_HISTORICO` (30).
- `log_parse_linha`: lê de volta uma linha de ação (6 colunas, começa com data); ignora cabeçalho, rodapé e linhas estranhas.
- `log_ultimas(v, max)`: percorre os arquivos dos últimos 30 dias, do mais antigo para hoje, e guarda as `max` ações mais recentes em ordem cronológica (buffer circular + rotação).
- `log_caminho_do_dia` reaproveita o novo `caminho_do_dia_de(t)`.

### Tela Histórico (`tela_historico`)
- Tabela com as 15 ações mais recentes, mais nova primeiro: Quando (`dd/mm hh:mm`), Ação, Resistor, Detalhe, Resultado (cor pelo nível: verde/amarelo/vermelho). Largura total 80 colunas; textos maiores são cortados com "…" (corte respeita UTF-8).
- Sem ações: mensagem "Nenhuma ação registrada nos últimos 30 dias."
- A própria abertura é registrada no log (`HISTÓRICO`) depois da leitura, para não listar a si mesma.
- Centralizada como as demais telas (`ALTURA_HISTORICO_BASE` + uma linha por ação).

### Testes
- `tests/test_log.c`: parse de linha de ação, ignora o que não é ação, últimas N em ordem, junta arquivos de dias diferentes, pasta sem arquivos.
- `tests/test_telas.c`: testes de navegação passam a esperar `OPCAO_HISTORICO` na linha de baixo; `9` abre o Histórico e `0` sai; menu mostra "[9] Histórico" sem `[0]` e sem "Sair"; tela Histórico lista as ações, mais recente primeiro.
- e2e: `9` antes do `0` final; `esperado.txt` confere título, cabeçalho e linhas do Histórico.

### Documentação
- README: tabela de funcionalidades e teclas do menu.
- AGENTS.md: tabela de opções e fluxo da UI.

### Validação
- `make EXTRA_CFLAGS=-Werror all test e2e cppcheck`: tudo passa (log 16, telas 23 sem o teste da paleta, novo 15; e2e 54 verificações + log), validado no índice isolado, sem as mudanças da paleta.
