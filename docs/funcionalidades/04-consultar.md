# 04 — Consultar (quantas peças tenho?)

← Anterior: [03 — Listar](03-listar.md) · [Funcionalidades](README.md) · Próximo: [05 — Retirar](05-retirar.md)

**Menu:** `[4] Consultar`
**Código:** `tela_consultar` em [src/ui/telas.c](../../src/ui/telas.c); `consulta_lista` em [src/lista.c](../../src/lista.c)
**Testes:** [tests/test_lista.c](../../tests/test_lista.c), [tests/test_novo.c](../../tests/test_novo.c), e2e

---

## 1. Para que serve

Responder rápido: **"quantas unidades de tal resistência existem?"**, sem precisar olhar a tabela inteira.

## 2. Como usar

```text
» Resistência: 4k7

┌──────────────────────────┐
│ 4,7 kΩ → 7 unidades      │
└──────────────────────────┘
```

A quantidade aparece em **verde** se houver peças e em **vermelho** se for zero.

## 3. Regras e casos especiais

| Situação | Resultado |
| --- | --- |
| Valor cadastrado | Caixa com a quantidade |
| Valor não cadastrado | ✖ *"5 kΩ não está no estoque"* |
| Quantidade 0 | *"0 unidades"* em vermelho (o tipo existe, mas está zerado) |
| Valor inválido 3× ou Enter vazio | *"Operação cancelada"* |
| Qualquer grafia (`4k7`, `4700`, `4,7k`) | Mesmo resultado ([12 — Entrada de valores](12-entrada-de-valores.md)) |

## 4. Por dentro

`consulta_lista(li, ohms, &qtd)` chama `busca`, que usa `localiza`: percorre a lista **do início e para no primeiro nó maior ou igual** ao procurado. Graças à ordenação, procurar um valor que não existe raramente percorre a lista toda.

**Tabela 1 — Nós examinados na lista `220 Ω → 4,7 kΩ → 10 kΩ`**

| Consulta | Nós examinados | Resultado |
| --- | --- | --- |
| 100 Ω | 1 | não encontrado (o primeiro já é maior) |
| 4,7 kΩ | 2 | 12 unidades |
| 5 kΩ | 3 | não encontrado (parou no 10 kΩ) |

O número de passos fica em `lista_passos(li)` e é usado nos testes e no relatório; a tela **não** o mostra (teste `teste_consultar_sem_nos_visitados`).

## 5. Registro no log

```text
| INFO  | CONSULTAR     | 4,7 kΩ     | qtd 7                | OK
| AVISO | CONSULTAR     | 5 kΩ       | -                    | NÃO ENCONTRADO
```

## 6. Desfazer

Não se aplica: Consultar não altera o estoque.

## 7. Por que é assim

| Decisão | Motivo |
| --- | --- |
| Mesma busca de todas as operações | Um só lugar com a regra de comparação (`localiza`) |
| Parada antecipada | Benefício direto da lista ordenada, bom para explicar no relatório |
| Contagem de passos fora da tela | Informação técnica; útil para o estudo, ruído para o usuário |

## 8. Testes

- `test_lista.c`: `teste_consulta`, `teste_passos_busca`.
- `test_novo.c`: a tela não mostra "nós visitados".
- e2e: consulta `4k7` e confere *"4,7 kΩ → 7 unidades"*.

---

← Anterior: [03 — Listar](03-listar.md) · [Funcionalidades](README.md) · Próximo: [05 — Retirar](05-retirar.md)
