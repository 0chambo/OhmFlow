# 09 — Histórico

← Anterior: [08 — Diferencial](08-diferencial.md) · [Funcionalidades](README.md) · Próximo: [10 — Desfazer e refazer](10-desfazer-refazer.md)

**Menu:** `[9] Histórico`
**Código:** `tela_historico`, `tabela_log`, `corta`, `cor_do_nivel` em [src/ui/telas.c](../../src/ui/telas.c); `log_ultimas` em [src/log.c](../../src/log.c)
**Testes:** [tests/test_telas.c](../../tests/test_telas.c), [tests/test_log.c](../../tests/test_log.c), e2e

---

## 1. Para que serve

Responder **"o que foi feito por último?"** — inclusive em execuções anteriores do programa — e dar acesso ao **checkpoint** para voltar o estoque a um ponto da sessão atual.

A tela junta duas fontes:

| Parte | Fonte | Alcance |
| --- | --- | --- |
| Tabela de últimas ações | Arquivos de log em disco ([11 — Registro de ações](11-registro-de-acoes.md)) | Últimos **30 dias**, todas as sessões |
| Linha do tempo do checkpoint | Pilhas na memória ([10 — Desfazer e refazer](10-desfazer-refazer.md)) | **Só a sessão atual** |

## 2. Como usar

```text
╭─ Histórico ──────────────────────────╮
╰──────────────────────────────────────╯
┌─────────────┬───────────────┬────────────┬──────────────────┬────────────────┐
│ Quando      │ Ação          │ Resistor   │ Detalhe          │ Resultado      │
├─────────────┼───────────────┼────────────┼──────────────────┼────────────────┤
│ 27/10 14:06 │ LISTAR        │ -          │ 3 tipos · 15 pe… │ OK             │
│ 27/10 14:05 │ NOVO          │ -          │ 3 ok · 1 dup · … │ LOTE CONCLUÍDO │
│ 27/10 14:04 │ RETIRAR       │ 4,7 kΩ     │ -100 (tem 7)     │ SEM ESTOQUE    │
└─────────────┴───────────────┴────────────┴──────────────────┴────────────────┘
  15 ações mais recentes (últimos 30 dias)
  [C] Voltar a um checkpoint desta sessão · [Enter] Voltar ao menu
```

- Até **15** ações, a mais recente em cima.
- **Quando** é encurtado para `dd/mm hh:mm`; textos longos são cortados com `…`.
- **Resultado** colorido pelo nível: verde (INFO), amarelo (AVISO), vermelho (ERRO).
- Se houve mudanças no estoque nesta sessão: `C` abre a linha do tempo; `Enter`/`Esc` voltam ao menu.

## 3. Regras e casos especiais

| Situação | Resultado |
| --- | --- |
| Nenhum log nos últimos 30 dias | *"Nenhuma ação registrada nos últimos 30 dias."* |
| Nenhuma mudança no estoque nesta sessão | Só a tabela; não oferece `[C]` |
| Sem terminal (testes) | A linha do tempo é impressa logo abaixo e o programa pergunta *"Ir para o ponto nº (Enter mantém):"* |
| Log desativado (pasta sem permissão) | A tabela fica vazia, mas o checkpoint funciona (está na memória) |

## 4. Por dentro

1. `log_ultimas(v, 15)` abre os arquivos `ohmflow_AAAA-MM-DD.log` dos **30 dias** anteriores, do mais antigo para hoje, e guarda as linhas de ação num **buffer circular** de 15 posições (ver [modulos/log.md](../modulos/log.md#5-leitura-o-buffer-circular-de-log_ultimas)).
2. `tabela_log` desenha do fim para o começo do vetor (mais recente em cima).
3. A tela registra a própria visita no log **depois** de ler, para não listar a si mesma.
4. Se `ck_total(checkpoint) > 0`, oferece o checkpoint ([10 — Desfazer e refazer](10-desfazer-refazer.md)).

## 5. Registro no log

```text
| INFO  | HISTÓRICO     | -          | 15 ações             | OK
| INFO  | HISTÓRICO     | -          | -                    | VAZIO
```

## 6. Por que é assim

| Decisão | Motivo |
| --- | --- |
| Ler do log em vez de guardar na memória | Mostra também ações de execuções anteriores |
| Limite de 15 e 30 dias | Cabe na tela e a leitura continua rápida mesmo com muitos arquivos |
| Buffer circular | Memória fixa, não importa o tamanho dos arquivos |
| Tabela (log) e linha do tempo (checkpoint) separadas | O log guarda texto formatado e várias sessões; o desfazer precisa dos valores exatos e só da sessão atual |

## 7. Testes

- `test_log.c`: parse de linhas, ignorar cabeçalho/rodapé, últimas ações em ordem, juntar dias, sem arquivos.
- `test_telas.c`: tela mostra as ações; `9` abre o Histórico; sem checkpoint não pergunta.
- e2e: abre o Histórico três vezes, confere a tabela (*"Quando"*, *"LISTAR"*, *"LOTE CONCLUÍDO"*) e usa o checkpoint.

---

← Anterior: [08 — Diferencial](08-diferencial.md) · [Funcionalidades](README.md) · Próximo: [10 — Desfazer e refazer](10-desfazer-refazer.md)
