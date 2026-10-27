# 06 — Repor (somar ao estoque)

← Anterior: [05 — Retirar](05-retirar.md) · [Funcionalidades](README.md) · Próximo: [07 — Reinicializar](07-reinicializar.md)

**Menu:** `[6] Repor`
**Código:** `tela_repor` em [src/ui/telas.c](../../src/ui/telas.c); `repor_estoque` em [src/lista.c](../../src/lista.c)
**Testes:** [tests/test_lista.c](../../tests/test_lista.c), [tests/test_checkpoint.c](../../tests/test_checkpoint.c), e2e

---

## 1. Para que serve

Registrar a **chegada de peças** de um tipo que já existe no estoque (compra, devolução, doação).

## 2. Como usar

```text
» Resistência: 220
  Em estoque: 50 unidades
» Quantidade a repor: 10
┌─ ✔ SUCESSO ──────────────────────────┐
│ Reposição concluída                  │
│ 220 Ω: 50 → 60 unidades              │
└──────────────────────────────────────┘
```

## 3. Regras e casos especiais

| Situação | Resultado |
| --- | --- |
| Tipo cadastrado (inclusive com 0) | Soma a quantidade: *"470 mΩ: 0 → 1 unidade"* |
| Tipo **não** cadastrado | *"Resistor não encontrado"*; para um valor novo, use [1] Novo |
| Soma passaria de 2.147.483.647 | ✖ *"Quantidade inválida — O total excederia o limite de 2147483647 unidades"*; nada muda |
| Quantidade 0, negativa ou texto | Pede de novo (até 3) |
| Enter vazio | *"Operação cancelada"* |

## 4. Por dentro

`repor_estoque(li, ohms, qtd)` acha o nó com `busca` e soma. A proteção contra estouro é escrita como:

```c
if (no->dados.quantidade > INT_MAX - qtd) return LISTA_INVALIDO; /* overflow */
```

e **não** como `quantidade + qtd > INT_MAX`, porque essa soma já estouraria antes de ser comparada (em C, estouro de `int` é comportamento indefinido). O total `pecas` do descritor é `long long` e não estoura.

Só com `LISTA_OK`: `ck_registra_movimento(OP_REPOR, ohms, qtd)`.

## 5. Registro no log

```text
| INFO  | REPOR         | 220 Ω      | +10 (50→60)          | OK
| ERRO  | REPOR         | 220 Ω      | +2147483647 (tem 60) | LIMITE EXCEDIDO
| AVISO | REPOR         | 5 kΩ       | -                    | NÃO ENCONTRADO
```

## 6. Desfazer

Desfazer uma reposição de *q* = **retirar *q***; refazer = repor *q*. Ver [10 — Desfazer e refazer](10-desfazer-refazer.md).

## 7. Por que é assim

| Decisão | Motivo |
| --- | --- |
| Repor só em tipo existente | Separa "chegou mais do que já temos" (Repor) de "valor novo" (Novo), evitando cadastros por erro de digitação |
| Checagem de estouro antes da soma | Correção formal em C; o limite é absurdo na prática, mas o teste `teste_repor_overflow` garante |
| Mesma tela do Retirar | Consistência: a pessoa aprende uma vez |

## 8. Testes

- `test_lista.c`: `teste_repor`, `teste_repor_overflow`, `teste_total_pecas_sem_overflow`.
- `test_checkpoint.c`: desfazer retirar e repor.
- e2e: repõe 10 em `220` (50 → 60) e 1 em `470m` zerado (0 → 1).

---

← Anterior: [05 — Retirar](05-retirar.md) · [Funcionalidades](README.md) · Próximo: [07 — Reinicializar](07-reinicializar.md)
