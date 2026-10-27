# 05 — Retirar (dar baixa no estoque)

← Anterior: [04 — Consultar](04-consultar.md) · [Funcionalidades](README.md) · Próximo: [06 — Repor](06-repor.md)

**Menu:** `[5] Retirar`
**Código:** `tela_retirar` em [src/ui/telas.c](../../src/ui/telas.c); `retira_estoque` em [src/lista.c](../../src/lista.c)
**Testes:** [tests/test_lista.c](../../tests/test_lista.c), [tests/test_checkpoint.c](../../tests/test_checkpoint.c), e2e

---

## 1. Para que serve

Registrar que **peças saíram** do almoxarifado (para uma aula, um projeto, um conserto). A quantidade diminui; o tipo continua cadastrado.

## 2. Como usar

1. Digite a resistência (precisa estar cadastrada).
2. O programa mostra *"Em estoque: 10 unidades"*.
3. Digite a quantidade a retirar.

```text
» Resistência: 4k7
  Em estoque: 10 unidades
» Quantidade a retirar: 3
┌─ ✔ SUCESSO ──────────────────────────┐
│ Retirada concluída                   │
│ 4,7 kΩ: 10 → 7 unidades              │
└──────────────────────────────────────┘
```

## 3. Regras e casos especiais

| Situação | Resultado | Cor |
| --- | --- | --- |
| Menos do que há | *"4,7 kΩ: 10 → 7 unidades"* | Verde |
| Exatamente o que há | *"Retirada concluída - estoque zerado"*, *"470 mΩ: 5 → 0 unidades"*; **o tipo continua cadastrado** | Amarelo |
| Mais do que há | *"Estoque insuficiente — 4,7 kΩ tem 7, pedido 100"*; **nada muda** | Vermelho |
| Valor não cadastrado | *"Resistor não encontrado"* (não pergunta a quantidade) | Vermelho |
| Quantidade 0, negativa ou texto | *"Quantidade inválida: informe um inteiro de 1 a 2147483647."*; até 3 tentativas | — |
| Enter vazio | *"Operação cancelada"* | Amarelo |

## 4. Por dentro

1. `ler_cadastrado` lê a resistência e consulta a quantidade atual.
2. `ler_quantidade(..., minimo = 1, ...)`.
3. `retira_estoque(li, ohms, qtd)`:
   - `qtd <= 0` → `LISTA_INVALIDO`;
   - não achou → `LISTA_NAO_ENCONTRADO`;
   - `qtd > quantidade` → `LISTA_SEM_ESTOQUE`;
   - senão `quantidade -= qtd` e `pecas -= qtd` (total do descritor).
4. Só com `LISTA_OK`: `ck_registra_movimento(OP_RETIRAR, ohms, qtd)`.

Só o campo `quantidade` do nó muda; **ponteiros e ordem continuam iguais** (ver [modulos/lista.md](../modulos/lista.md#53-retirar-repor-e-consultar)).

## 5. Registro no log

```text
| INFO  | RETIRAR       | 4,7 kΩ     | -3 (10→7)            | OK
| AVISO | RETIRAR       | 470 mΩ     | -5 (5→0)             | OK - ESTOQUE ZERADO
| AVISO | RETIRAR       | 4,7 kΩ     | -100 (tem 7)         | SEM ESTOQUE
| ERRO  | RETIRAR       | 4,7 kΩ     | "abc"                | QUANTIDADE INVÁLIDA
| AVISO | RETIRAR       | 5 kΩ       | -                    | NÃO ENCONTRADO
```

O zeramento vai como **AVISO** de propósito: é sucesso, mas indica que é hora de repor.

## 6. Desfazer

Desfazer uma retirada de *q* = **repor *q***; refazer = retirar *q* de novo. Ver [10 — Desfazer e refazer](10-desfazer-refazer.md).

## 7. Por que é assim

| Decisão | Motivo |
| --- | --- |
| Mostrar o estoque antes de perguntar | A pessoa sabe o limite antes de digitar |
| Recusar retirada maior (sem retirar o que dá) | Retirada parcial silenciosa geraria divergência entre o sistema e a gaveta |
| Zerar mantém o tipo | O valor continua sendo um item do almoxarifado, só sem peças; apagar é papel do Remover |
| Mínimo 1 | Retirar 0 não faz sentido e poluiria o histórico |

## 8. Testes

- `test_lista.c`: retirada parcial e total, além do estoque, quantidades inválidas.
- `test_checkpoint.c`: desfazer retirar e repor.
- e2e: retira 3 de `4k7` (10 → 7); tenta 100 (estoque insuficiente); retira 5 de `470m` (estoque zerado).

---

← Anterior: [04 — Consultar](04-consultar.md) · [Funcionalidades](README.md) · Próximo: [06 — Repor](06-repor.md)
