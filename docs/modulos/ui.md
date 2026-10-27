# Módulo UI — primitivas de interface de terminal

← Anterior: [Log](log.md) · [Módulos](README.md) · Próximo: [Telas](telas.md)

**Arquivos:** [src/ui/ui.h](../../src/ui/ui.h) · [src/ui/ui.c](../../src/ui/ui.c)
**Testes:** [tests/test_ui.c](../../tests/test_ui.c)

> **Resumo:** caixa de ferramentas visual do programa: cores, molduras, tabelas alinhadas, mensagens de uma linha, leitura de linhas e de "Enter", limpar tela e centralizar. Não sabe nada de resistores; as [Telas](telas.md) usam estas peças para montar cada tela.

---

## 1. Explicação para leigos

Se as telas são os **cômodos** de uma casa, este módulo é a **loja de materiais**: vende molduras prontas, tinta (cores), réguas (alinhamento) e campainhas (perguntas). Quem constrói o cômodo não precisa saber fabricar uma moldura, só pedir "uma caixa verde com este texto".

## 2. Dois modos de funcionamento

A decisão mais importante do módulo é detectar **se há uma pessoa do outro lado**:

| | Modo interativo | Modo não interativo |
| --- | --- | --- |
| Quando | Teclado e tela são um terminal de verdade | Entrada ou saída redirecionada (testes, e2e, CI) |
| Detecção | `isatty(stdin) && isatty(stdout)` | qualquer um dos dois não é terminal |
| Cores | Sim (se o terminal suportar e `NO_COLOR` não estiver definida) | Não |
| Limpar tela, esconder cursor | Sim | Não |
| `ui_pausar` ("Pressione Enter") | Espera o Enter | Não espera |
| O que foi lido | Já aparece na tela (o terminal ecoa) | É **repetido na saída** (`eco`), para o arquivo de saída fazer sentido |
| Menu | Setas e teclas | Linhas de texto (`Opção: 3`) |

Sem isso, os testes automáticos ficariam presos esperando um Enter que nunca chega, e a saída ficaria cheia de códigos de cor.

## 3. Inicialização (`ui_init`)

No Windows:

1. Põe o console em **UTF-8** (`SetConsoleOutputCP`, `SetConsoleCP`) para `Ω`, `✔` e as molduras aparecerem certo.
2. Liga o **modo de terminal virtual** (`ENABLE_VIRTUAL_TERMINAL_PROCESSING`) para o console entender códigos ANSI. Se falhar (console muito antigo), as cores ficam desligadas.
3. Define o título da janela como `OhmFlow` e aplica o **ícone** do recurso nº 1 do executável (ver [Main](main.md)).

Fora do Windows, só verifica se `TERM` não é `dumb`. Por fim, liga as cores se a saída for terminal, o terminal suportar e `NO_COLOR` estiver vazia.

## 4. Cores

`ui_cor(UI_COR_X)` devolve o código ANSI, ou `""` se as cores estiverem desligadas. Assim quem chama nunca precisa testar se há cor.

**Tabela 1 — Paleta**

| Papel | Constante | Cor (ANSI 256) | Onde aparece |
| --- | --- | --- | --- |
| Primária | `UI_COR_PRIMARIA`, `UI_COR_BORDA`, `UI_COR_PROMPT` | Laranja (208) | Bordas, logo, prompt `»`, opção selecionada |
| Secundária | `UI_COR_SECUNDARIA`, `UI_COR_TITULO` | Branco brilhante (97) | Títulos, texto do menu |
| Sucesso | `UI_COR_VERDE` | Verde | Caixas e mensagens de sucesso |
| Erro | `UI_COR_VERMELHO` | Vermelho | Erros, quantidade zero |
| Aviso | `UI_COR_AMARELO` | Amarelo | Avisos, confirmações |
| Apoio | `UI_COR_DIM` | Esmaecido | Dicas, ações desfeitas |

A identidade visual é **laranja + branco**; os testes `teste_menu_so_laranja_e_branco` e `teste_interface_sem_ciano_nem_azul` garantem que outras cores não voltem por engano.

## 5. Molduras e caixas

Três estilos (`UI_MOLDURA_DUPLA` `╔═╗`, `UI_MOLDURA_SIMPLES` `┌─┐`, `UI_MOLDURA_ARREDONDADA` `╭─╮`), com versão ASCII (`+-|`) quando compilado com `-DOHM_ASCII`.

`ui_caixa(estilo, cor, titulo_borda, cabecalho, linhas, n, largura_min)` desenha:

```text
╭─ Título na borda ─────────────────╮
│        Cabeçalho centralizado      │
├────────────────────────────────────┤
│ linha 1                            │
│ linha 2                            │
╰────────────────────────────────────╯
```

A largura é a **maior** entre `largura_min` e o conteúdo: a caixa cresce para caber o texto, nunca corta.

`ui_caixa_feedback(tipo, titulo, fmt, ...)` é a caixa de resultado padrão: borda simples na cor do tipo, rótulo `✔ SUCESSO` / `✖ ERRO` / `⚠ ATENÇÃO` / `ℹ INFO`, título em negrito e a mensagem quebrada em linhas por `\n`.

## 6. O problema do alinhamento

Para as bordas fecharem certinho, é preciso saber **quantas colunas** um texto ocupa na tela. Isso não é o número de bytes:

- `Ω` ocupa 1 coluna, mas 2 bytes em UTF-8; `✔` ocupa 1 coluna e 3 bytes.
- `\033[32m` (verde) ocupa **0** colunas, mas 5 bytes.

`ui_largura(s)` resolve: pula as sequências ANSI inteiras (`ESC [ ... letra`) e conta só os bytes que **iniciam** um caractere UTF-8 (os que não seguem o padrão `10xxxxxx`). `ui_pad` usa essa largura para completar com espaços. Dezenas de testes conferem que **todas as linhas de uma caixa ou tabela têm a mesma largura visível**.

## 7. Mensagens, dicas e tabelas

| Função | Saída |
| --- | --- |
| `ui_mensagem(tipo, fmt, ...)` | `  ✔ texto` na cor do tipo |
| `ui_dica(fmt, ...)` | Texto esmaecido de orientação |
| `ui_tabela_borda(pos, larguras, ncol)` | Linha de topo `┌┬┐`, meio `├┼┤` ou base `└┴┘` |
| `ui_tabela_linha(celulas, larguras, alinhar_dir, ncol)` | `│ a │ b │`, cada célula alinhada à esquerda ou à direita |

## 8. Entrada

| Função | Faz |
| --- | --- |
| `ui_ler_linha(prompt, buf, n)` | Mostra `» prompt`, lê uma linha, descarta o excesso se for longa, apara espaços nas duas pontas; devolve 0 no fim da entrada (EOF) |
| `ui_confirmar(fmt, ...)` | Pergunta `⚠ ... (s/n):`; aceita `s`/`S` ou `n`/`N`; até 3 tentativas; EOF conta como "não" |
| `ui_linha_respondida(prompt, resposta)` | Reescreve `» prompt resposta` igual ao eco de `ui_ler_linha`; usada quando a tela é redesenhada depois da resposta (tela Novo com o resistor colorido) |
| `ui_esperar_enter()` | Espera Enter **sem ecoar** o que for digitado (`_getch` no Windows) |
| `ui_espera_enter_de(proxima, ctx)` | A lógica pura do item acima, testável: consome teclas até `\r`/`\n`; teclas especiais do Windows (prefixo 0 ou 224, como setas e F1) descartam o byte seguinte |

## 9. Centralização

`main` liga o **modo centralizado** quando roda num terminal. Nele:

- Caixas e tabelas são centralizadas pela **própria largura** na janela.
- Mensagens, dicas e prompts se alinham a um **bloco central** de `UI_LARGURA + 2` (40) colunas; textos mais largos que o bloco são centralizados pela própria largura.
- `ui_espaco_vertical(altura)` imprime linhas em branco para centralizar uma tela de `altura` linhas na vertical.

O tamanho da janela vem de `GetConsoleScreenBufferInfo` (Windows) ou `ioctl(TIOCGWINSZ)`, com mínimos de 40 colunas e 10 linhas.

Desligado (padrão e nos testes), tudo sai com recuo fixo de 2 espaços, previsível para comparar.

## 10. Limpar a tela

No Windows, `ui_limpar_tela` preenche **todo o buffer** do console com espaços (não só a parte visível) e depois envia `ESC[3J ESC[2J ESC[H`. Assim, rolar a janela para cima não mostra telas antigas.

## 11. Referência rápida da API

**Tabela 2 — Todas as funções públicas de `ui.h`**

| Grupo | Funções |
| --- | --- |
| Estado | `ui_init`, `ui_definir_saida`, `ui_definir_cor`, `ui_cor_ativa`, `ui_interativo`, `ui_definir_interativo`, `ui_cor` |
| Tela | `ui_limpar_tela`, `ui_pausar`, `ui_esperar_enter`, `ui_espera_enter_de`, `ui_colunas_terminal`, `ui_linhas_terminal`, `ui_cursor` (mostra/esconde o cursor) |
| Centralização | `ui_definir_centralizado`, `ui_centralizado`, `ui_margem` (recuo do bloco central), `ui_espaco_vertical` |
| Caixas | `ui_caixa`, `ui_caixa_centralizada` (sempre centraliza pela própria largura; o menu usa), `ui_caixa_feedback` |
| Mensagens | `ui_mensagem`, `ui_dica` |
| Entrada | `ui_ler_linha`, `ui_confirmar`, `ui_linha_respondida` |
| Alinhamento | `ui_largura`, `ui_pad` |
| Tabelas | `ui_tabela_borda`, `ui_tabela_linha` |

Símbolos com versão ASCII: `UI_ICONE_OK/ERRO/AVISO/INFO` (`✔ ✖ ⚠ ℹ`), `UI_PROMPT` (`»`), `UI_SETA` (`→`), `UI_PONTO` (`·`), `UI_BARRA` (`│`), `UI_BULLET` (`●`). Tipos de mensagem: `UI_SUCESSO`, `UI_ERRO`, `UI_AVISO`, `UI_INFO`. Bordas de tabela: `UI_TAB_TOPO`, `UI_TAB_MEIO`, `UI_TAB_BASE`. As cores `UI_COR_AZUL`, `UI_COR_CIANO`, `UI_COR_MAGENTA`, `UI_COR_BRANCO`, `UI_COR_LARANJA`, `UI_COR_VERDE_NEON` e `UI_COR_ROXO_NEON` existem na tabela de códigos (e são testadas), mas as telas atuais não as usam: usam os papéis `PRIMARIA`/`SECUNDARIA`/`BORDA`/`TITULO`/`PROMPT` e as cores de status. `UI_COR_RESET`, `UI_COR_NEGRITO` e `UI_COR_DIM` voltam ao normal, negritam e esmaecem.

## 12. Por que foi feito assim

| Decisão | Motivo |
| --- | --- |
| Biblioteca própria em vez de `ncurses`/`PDCurses` | Zero dependências; o `.exe` roda em qualquer Windows 10/11 |
| Saída configurável (`ui_definir_saida`) | Os testes mandam a tela para um arquivo temporário e conferem o texto |
| `ui_cor` devolve `""` sem cor | Nenhum `if` espalhado pelo código das telas |
| Largura visível calculada | Bordas sempre fecham, com acentos, `Ω` e cores |
| Modo não interativo | Testes automáticos e e2e sem pessoa nenhuma |
| `-DOHM_ASCII` | Plano B para terminais sem UTF-8 |

## 13. Testes

[tests/test_ui.c](../../tests/test_ui.c), 30 testes: largura ASCII/UTF-8/ANSI, `ui_pad`, emissão (ou não) de ANSI, caixas e tabelas alinhadas, caixa que cresce com o conteúdo, feedback, moldura arredondada, paleta laranja/branco, ausência de ciano e azul, tamanho do terminal, espera por Enter (descarta texto, aceita LF, tecla estendida, EOF) e todo o modo centralizado.

---

← Anterior: [Log](log.md) · [Módulos](README.md) · Próximo: [Telas](telas.md)
