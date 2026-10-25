# CHANGES

## refactor(ui): remove código morto (cores, caixa de título e macro sem uso)

### Problema

A revisão de código achou símbolos definidos e mantidos que nenhum código de produção usa. Eles aumentam a API pública, confundem quem lê e, no caso de `UI_COR_SOMBRA`, tinham até um teste só para o código morto.

| Símbolo | Onde estava | Uso real antes desta mudança |
| --- | --- | --- |
| `LARG_BOAS_VINDAS` | `src/ui/telas.c` | nenhum; resto da versão antiga da tela de boas-vindas (o comentário ainda dizia "logo tem 45 colunas") |
| `ui_caixa_titulo()` | `src/ui/ui.h` / `src/ui/ui.c` | só `tests/test_ui.c`; as telas usam `abre_tela()` com `ui_caixa()` |
| `UI_COR_SOMBRA` | `src/ui/ui.h` / `src/ui/ui.c` (`CODIGOS`) | só o teste `teste_cor_sombra_retro` |
| `UI_COR_DESTAQUE` | `src/ui/ui.h` / `src/ui/ui.c` (`CODIGOS`) | nenhum |

### Alterações

[src/ui/telas.c](src/ui/telas.c):
- Removidos o `#define LARG_BOAS_VINDAS 50` e o comentário desatualizado dele. `TAB_DIR` volta a ficar junto de `TAB_LARG`, abaixo do comentário das colunas da tabela.

[src/ui/ui.h](src/ui/ui.h):
- O enum de cores perdeu `UI_COR_DESTAQUE` e `UI_COR_SOMBRA`. `UI_COR_TOTAL` continua o último item, então o tamanho de `CODIGOS` acompanha automaticamente.
- Removido o protótipo de `ui_caixa_titulo()`.

[src/ui/ui.c](src/ui/ui.c):
- Saem de `CODIGOS` as entradas `"\033[1;33m"` (DESTAQUE) e `"\033[38;5;130m"` (SOMBRA). A ordem das demais cores segue a do enum.
- Removida a função `ui_caixa_titulo()`.

[tests/test_ui.c](tests/test_ui.c):
- `teste_sem_cor_nao_emite_ansi` deixa de chamar `ui_caixa_titulo()`. O teste continua cobrindo `ui_caixa_feedback` e `ui_mensagem` sem cor.
- Removido o teste `teste_cor_sombra_retro` e o `RUN_TEST` dele. As cores retro em uso (laranja, verde neon, roxo neon) seguem cobertas por `teste_cores_retro_novas`.

Mantido de propósito: `ui_cor_ativa()`. É usado só nos testes, mas serve para observar o estado da UI e custa uma linha.

### Bugs encontrados na revisão (abertos como issues, não corrigidos aqui)

- #16: `ui_colunas_terminal` sempre devolve 80 no Linux/macOS.
- #17: o feedback de Repor mostra "1 unidades" (plural fixo).
- #18: na tela de boas-vindas, a altura do terminal fica fixa em 24 fora do Windows. A supressão `cppcheck-suppress knownConditionTrueFalse` em `tela_boas_vindas` depende deste bug e sai junto com a correção.

### Validação

- Windows 11 (MSYS2 UCRT64): `make clean; make EXTRA_CFLAGS=-Werror all test e2e` passou sem warnings.
  - `test_lista`: 18 testes; `test_formatacao`: 11; `test_ui`: 14 (antes 15, saiu o teste da cor morta); `test_telas`: 19. Nenhum falhou.
  - `E2E OK (31 verificacoes)`.
- `grep` por `SOMBRA`, `DESTAQUE`, `caixa_titulo` e `LARG_BOAS_VINDAS` em `src/`, `tests/` e `*.md`: nenhuma ocorrência.
- **Pendente:** `make cppcheck` não roda localmente (cppcheck não instalado nesta máquina). Ele roda no job Ubuntu do CI.
