# Guia de Contribuição — OhmFlow

Obrigado por considerar contribuir com o OhmFlow! Este guia explica como configurar o ambiente, as convenções adotadas e o fluxo de trabalho esperado.

---

## Pré-requisitos

Antes de começar, certifique-se de ter instalado:

| Ferramenta | Versão mínima | Para que serve |
|------------|---------------|----------------|
| `gcc`      | 11+           | Compilar o projeto |
| `make`     | 4.3+          | Executar targets de build |
| `git`      | 2.30+         | Controle de versão |
| `cppcheck` | 2.10+ (opt.)  | Análise estática |

Veja as instruções de instalação por SO no [README](../README.md#instalação-dos-pré-requisitos).

---

## Configuração do ambiente

```sh
# 1. Fork do repositório pelo GitHub, depois clone o seu fork
git clone https://github.com/<seu-usuario>/OhmFlow.git
cd OhmFlow

# 2. Adicione o repositório original como upstream
git remote add upstream https://github.com/Grolla05/OhmFlow.git

# 3. Verifique que tudo compila e passa
make EXTRA_CFLAGS=-Werror all test e2e cppcheck
```

---

## Fluxo de trabalho

```
main
 └── sua-branch-descritiva
       ├── commits atômicos
       └── pull request → main
```

1. **Sincronize** com o upstream antes de começar:

   ```sh
   git fetch upstream
   git rebase upstream/main
   ```

2. **Crie uma branch** com nome descritivo:

   ```sh
   git checkout -b fix/parse-virgula-decimal
   # ou
   git checkout -b feat/busca-por-faixa
   ```

3. **Implemente** em commits pequenos e atômicos (um commit = uma mudança coesa).

4. **Valide** localmente antes de abrir o PR:

   ```sh
   make EXTRA_CFLAGS=-Werror all test e2e cppcheck
   # No Linux, também:
   make asan
   ```

5. **Abra um Pull Request** para `main` usando o template disponível.

---

## Convenção de commits

Seguimos o padrão **Conventional Commits**. Leia o guia completo em [`.github/COMMIT_CONVENTION.md`](COMMIT_CONVENTION.md).

Resumo rápido:

```
feat(lista): adiciona busca por faixa de resistência
fix(formatacao): corrige parse de "4,7k"
test(lista): cobre remoção de único elemento
```

---

## Padrões de código

- **C11** (`-std=c11`); sem extensões específicas de compilador
- Sem warnings com `-Wall -Wextra -Wpedantic -Werror`
- `cppcheck` sem novos erros ou warnings
- Funções públicas documentadas em `.h` com propósito, parâmetros e retorno
- Sem `printf` de debug no código enviado

---

## Testes

Todo código novo ou modificado deve ter testes correspondentes em `tests/`.

| Tipo         | Comando         | Descrição |
|--------------|-----------------|-----------|
| Unitário     | `make test`     | Lógica isolada de cada módulo |
| Ponta a ponta| `make e2e`      | Fluxo completo via stdin simulado |
| Memória      | `make asan`     | AddressSanitizer + UBSan (Linux) |

Para adicionar um caso de teste unitário, siga o padrão em `tests/test_lista.c` ou `tests/test_formatacao.c`. O framework está em `tests/test_framework.h` (sem dependências externas).

---

## Reportando bugs

Use o template [Bug Report](ISSUE_TEMPLATE/bug_report.md) no GitHub Issues.  
Inclua sempre: SO, versão do compilador, passos para reproduzir e saída do terminal.

---

## Sugerindo funcionalidades

Use o template [Feature Request](ISSUE_TEMPLATE/feature_request.md) no GitHub Issues.

---

## Código de conduta

Este projeto adota um ambiente colaborativo e respeitoso.  
Seja construtivo nas revisões e receptivo ao feedback.

---

## Dúvidas?

Abra uma [Discussion](https://github.com/Grolla05/OhmFlow/discussions) ou mencione `@Grolla05` em uma issue.
