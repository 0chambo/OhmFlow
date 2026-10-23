# Convenção de Commits — OhmFlow

Seguimos o padrão **Conventional Commits 1.0** com escopo obrigatório para mudanças em código.

## Formato

```
<tipo>(<escopo>): <descrição curta>

[corpo opcional]

[rodapé(s) opcional(is)]
```

- Linha de assunto: **máx. 72 caracteres**
- Verbo no imperativo, minúsculas: "adiciona", não "adicionado" nem "Adiciona"
- Sem ponto final no assunto

---

## Tipos

| Tipo       | Quando usar |
|------------|-------------|
| `feat`     | Nova funcionalidade visível ao usuário |
| `fix`      | Correção de bug |
| `refactor` | Mudança interna sem alterar comportamento externo |
| `test`     | Adiciona ou corrige testes |
| `docs`     | Apenas documentação (README, comentários de código) |
| `build`    | Mudança no sistema de build ou dependências (Makefile, CI) |
| `ci`       | Pipelines de CI/CD (`.github/workflows/`) |
| `style`    | Formatação, espaços, sem mudança de lógica |
| `chore`    | Tarefas de manutenção que não encaixam acima |
| `perf`     | Melhoria de desempenho |
| `revert`   | Reverte um commit anterior |

---

## Escopos

| Escopo       | O que cobre |
|--------------|-------------|
| `lista`      | TAD lista dinâmica (`src/lista.*`) |
| `formatacao` | Parse e formatação de resistências (`src/formatacao.*`) |
| `ui`         | Módulo de interface (`src/ui/`) |
| `telas`      | Telas e menus (`src/ui/telas.*`) |
| `main`       | Ponto de entrada (`src/main.c`) |
| `tests`      | Qualquer arquivo em `tests/` |
| `ci`         | Workflows do GitHub Actions |
| `deps`       | Dependências externas |
| `release`    | Versionamento e notas de lançamento |

---

## Exemplos

```
feat(lista): adiciona operação de busca por faixa de resistência

fix(formatacao): corrige parse de valores com vírgula decimal (4,7k)

refactor(ui): extrai lógica de moldura para função auxiliar

test(lista): adiciona casos de borda para lista vazia

docs: documenta formatos de entrada no README

build: adiciona alvo `make asan` para Linux

ci: habilita cache de pacotes no workflow do Ubuntu
```

---

## Breaking Changes

Qualquer mudança que quebre compatibilidade de API ou comportamento documentado:

```
feat(formatacao)!: remove suporte a sufixo 'ohm' por extenso

BREAKING CHANGE: strings como "220 ohm" não são mais aceitas.
Use "220R" ou somente "220".
```

---

## Referências em rodapé

| Palavra-chave         | Efeito no GitHub           |
|-----------------------|----------------------------|
| `Closes #123`         | Fecha a issue ao fazer merge |
| `Fixes #123`          | Idem |
| `Refs #123`           | Vincula sem fechar |
| `Co-authored-by: ...` | Atribuição compartilhada |
