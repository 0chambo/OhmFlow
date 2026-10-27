---
name: "🐛 Bug report (texto livre)"
about: Relate um bug sem usar o formulário guiado
title: "[Bug] "
labels: bug
assignees: Grolla05
---

## Descrição do bug

<!-- Descreva claramente o que acontece de errado. -->

## Como reproduzir

1. Compilar com `make all`
2. Executar `./dist/ohmflow`
3. Escolher a opção **X**
4. Digitar **Y**
5. Ver o erro

## Comportamento esperado

<!-- O que deveria acontecer? -->

## Comportamento atual

<!-- O que realmente acontece? Cole a saída do terminal. -->

```
(saída do terminal)
```

## Gravidade

- [ ] Baixa (incômodo visual ou mensagem errada)
- [ ] Média (resultado incorreto, mas o programa continua)
- [ ] Alta (crash, travamento ou dado perdido)
- [ ] Crítica (corrupção de memória / undefined behavior)

## Ambiente

| Campo           | Valor |
|-----------------|-------|
| OS              | <!-- ex.: Windows 11, Ubuntu 24.04, macOS 15 --> |
| Terminal        | <!-- ex.: Windows Terminal, MSYS2 UCRT64, bash --> |
| gcc             | <!-- saída de `gcc --version` --> |
| make            | <!-- saída de `make --version` --> |
| Commit / branch | <!-- saída de `git log --oneline -1` --> |

## Módulo afetado

- [ ] `lista` (src/lista.*)
- [ ] `formatacao` (src/formatacao.*)
- [ ] `ui` (src/ui/ui.*)
- [ ] `telas` (src/ui/telas.*)
- [ ] `main` (src/main.c)
- [ ] Build / Makefile
- [ ] CI / GitHub Actions
- [ ] Outro

## Logs / saída de erro

```
(cole aqui)
```
