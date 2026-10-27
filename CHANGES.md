# CHANGES — Paleta monocromática, cards, badges e foco do menu

## Resumo

Acabamento visual "minimalista" da interface:

1. **Paleta monocromática com um destaque só**: texto em branco 255, bordas finas em cinza 240, dicas em cinza 244, rótulos e metadados em cinza 245. O laranja 208 continua sendo a cor da marca e passa a ser o **único destaque** (seleção, prompt, barras). A cor de destaque **não mudou para azul**: o banner, o logo e o ícone já são laranja.
2. **Hierarquia**: títulos em negrito branco com tracking (`E S T O Q U E   A T U A L`), subtítulos e metadados em cinza.
3. **Cards**: moldura arredondada fina com respiro (2 espaços nas laterais, uma linha em branco acima e abaixo do conteúdo) e título embutido na borda (`╭── Título ───╮`).
4. **Badges de status**: `[ OK ]`, `[ ATENÇÃO ]`, `[ ERRO ]`, `[ INFO ]`, `[ VAZIO ]`, com texto escuro sobre fundo semântico quando há cor.
5. **Foco do menu**: a opção ativa tem chevron `›` laranja sobre fundo 236, e o chevron "desliza" para a nova opção num quadro intermediário de 30 ms.

## Detalhes

### Paleta (`src/ui/ui.h`, `src/ui/ui.c`)

| Cor | Antes | Agora | Uso |
| --- | --- | --- | --- |
| `UI_COR_SECUNDARIA` | `97m` | `38;5;255m` | texto principal |
| `UI_COR_TITULO` | `1;97m` | `1;38;5;255m` | títulos (borda e cabeçalho) |
| `UI_COR_BORDA` | laranja 208 | `38;5;240m` | molduras e tabelas |
| `UI_COR_DIM` | `2m` | `38;5;244m` | dicas, parte vazia das barras |
| `UI_COR_CINZA` (nova) | — | `38;5;245m` | rótulos e metadados |
| `UI_COR_FUNDO_FOCO` (nova) | — | `48;5;236m` | fundo da opção ativa |
| `UI_COR_PRIMARIA` / `PROMPT` | laranja 208 | inalteradas | destaque |

O DIM virou um cinza de verdade: o `\033[2m` dependia do terminal e às vezes sumia.

### Cards e molduras (`src/ui/ui.c`)

- `largura_interna` calcula a largura da caixa num lugar só (antes o cálculo estava duplicado em `ui_caixa_centralizada`).
- `respiro(estilo)`: margem interna de 2 colunas na moldura arredondada e de 1 nas demais. O card arredondado ganha uma linha em branco antes e depois do conteúdo. Uma caixa só com título (topo de tela) continua com 2 linhas.
- O título na borda agora sai após dois traços (`╭── Título ─╮`), em negrito branco.

### Badges (`ui_badge`)

- `ui_badge(tipo, rotulo, buf, n)`, com o novo tipo `UI_NEUTRO` no enum de mensagens.
- Sem cor, sai `[ OK ]`; com cor, sai `  OK  ` com a mesma largura, em negrito escuro sobre o fundo: verde 71, âmbar 214, vermelho 167, laranja 208 (INFO) ou cinza 240 com texto branco (VAZIO).
- Rótulos padrão: OK, ERRO, ATENÇÃO, INFO, VAZIO. Os nomes do tipo de feedback passaram de "SUCESSO" para "OK".
- `ui_caixa_feedback`: o título da borda passa a ser o badge, e a moldura agora é cinza (a cor semântica fica só no badge, em vez de pintar a caixa inteira).
- **Listar**: tipo com quantidade 0 mostra `[ VAZIO ]` na coluna Estoque, no lugar da barra vazia com "0%".
- **Consultar**: com quantidade 0, a linha da fatia mostra `[ VAZIO ] sem unidades`.
- **Menu**: com o estoque vazio, aparece `[ VAZIO ]` depois de "Tipos · Peças".

### Tracking de títulos (`ui_espacar`)

- Converte para maiúsculas, incluindo os acentos do Latin-1 em UTF-8 (`ó` -> `Ó`), com um espaço entre as letras e três entre as palavras.
- Se o resultado não couber em `max_colunas`, devolve 0 e copia o texto original.
- `titulo_da_tela` (em `telas.c`) aplica o tracking nos títulos das telas (`abre_tela`, Novo com o resistor, Checkpoint) só no terminal interativo. Sem terminal (e2e, testes) fica o texto original, por isso o e2e não mudou.
- O título do menu é sempre `O H M F L O W`.

### Menu (`src/ui/telas.c`)

- `celula_menu` monta cada opção com a mesma largura visível nos três estados:
  - inativa: `   [n] Rótulo`, com o número em cinza e o rótulo em branco;
  - ativa: ` › [n] Rótulo`, em laranja negrito sobre fundo 236;
  - fase 0 da animação: `›  [n] Rótulo`, com o chevron em cinza uma coluna à esquerda.
- Os rótulos são alinhados por coluna (9 à esquerda, 13 à direita).
- `desenha_menu_fase` (estática) desenha o menu; `desenha_menu` (pública) desenha a fase final.
- O cabeçalho virou metadado em cinza ("Controle de Estoque · 8-Bit"), há uma linha em branco antes dos totais, e os totais têm rótulos em cinza e valores em branco negrito.
- `quadro_menu` desenha um quadro completo. Quando a seleção muda, sai primeiro o quadro da fase 0, depois uma pausa de `MENU_ANIMACAO_MS` (30 ms) e então o quadro final.
- A animação é pulada quando já há outra tecla na fila (`ui_tecla_pendente`), então segurar uma seta continua imediato.
- `MENU_LINHAS` passou de 11 para 14.
- Novas primitivas `ui_pausa_ms` (Sleep/nanosleep; sem terminal não espera) e `ui_tecla_pendente` (`_kbhit` no Windows; 0 no POSIX).
- Novo símbolo `UI_CHEVRON`: `›`, ou `>` com `OHM_ASCII`.

### Testes

- `tests/test_ui.c`:
  - paleta atualizada (255/240/244/245/236);
  - `teste_interface_sem_ciano_nem_azul` agora checa a borda cinza e a ausência de neon;
  - a barra vazia é testada em cinza 244;
  - novos: `teste_badge_sem_cor`, `teste_badge_com_cor`, `teste_card_arredondado_com_respiro`, `teste_card_so_titulo_sem_respiro`, `teste_feedback_com_badge`, `teste_espacar_titulo`.
- `tests/test_telas.c`:
  - borda do menu cinza com o laranja no foco;
  - branco 255 e cinza 245 no menu;
  - novos: `teste_menu_foco_chevron` (um único `›` e um único fundo 236, na opção ativa), `teste_menu_foco_sem_cor_mantem_chevron` (o foco aparece sem cor, e o estoque vazio mostra `[ VAZIO ]`) e `teste_listar_zerado_mostra_badge_vazio`.
- Validação: `make clean; make EXTRA_CFLAGS=-Werror all test e2e cppcheck` passou (test_ui com 42 testes, test_telas com 37, e2e com 70 verificações). `test_ui` e `test_telas` também passam com `-DOHM_ASCII`.

## Arquivos alterados

- `src/ui/ui.h`, `src/ui/ui.c`
- `src/ui/telas.c`
- `tests/test_ui.c`, `tests/test_telas.c`
- `AGENTS.md` (local, ignorado pelo git)
