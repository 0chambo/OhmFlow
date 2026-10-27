# 08 — Item diferencial

← Anterior: [07 — Reinicializar](07-reinicializar.md) · [Funcionalidades](README.md) · Próximo: [09 — Histórico](09-historico.md)

**Menu:** `[8] (diferencial)`
**Código:** `tela_diferencial` em [src/ui/telas.c](../../src/ui/telas.c)
**Status:** 🟡 **a definir** — hoje é um espaço reservado

---

## 1. Para que serve

O enunciado pede:

> "Inserir um item diferencial em seu Menu, cada grupo deverá ter um diferencial de especificidade dentro deste Menu em seu algoritmo."

Ou seja, cada grupo cria uma funcionalidade extra própria. A opção 8 já existe no menu para recebê-la.

## 2. Comportamento atual

Escolher `[8]` mostra:

```text
┌─ ℹ INFO ─────────────────────────────┐
│ Em breve                             │
│ Funcionalidade diferencial em desenvolvimento
└──────────────────────────────────────┘
```

e registra no log:

```text
| INFO  | DIFERENCIAL   | -          | -                    | EM BREVE
```

## 3. Candidatas

As ideias estão comparadas em [refs/BRAINSTORM.md](../../refs/BRAINSTORM.md), que é a referência atualizada. Duas delas já viraram funcionalidades **fora** da opção 8:

| Ideia | Situação |
| --- | --- |
| Desfazer (pilha) | ✅ Implementada como [Desfazer e refazer](10-desfazer-refazer.md), dentro do `[9] Histórico` |
| Código de cores das faixas | ✅ Implementada como [Resistor colorido](14-resistor-colorido.md), dentro do `[1] Novo` |
| Resistor mais próximo / substituto | Candidata (usa os vizinhos na lista ordenada) |
| Alerta de estoque baixo | Candidata |
| Salvar/carregar estoque em arquivo | Candidata (com evolução futura para SQLite) |

## 4. Como implementar (roteiro)

O passo a passo para transformar o placeholder em funcionalidade está em [09 — Contribuindo](../09-contribuindo.md#4-roteiro-adicionar-uma-funcionalidade-nova), usando o diferencial como exemplo. Ao concluir, **atualize este documento**.

---

← Anterior: [07 — Reinicializar](07-reinicializar.md) · [Funcionalidades](README.md) · Próximo: [09 — Histórico](09-historico.md)
