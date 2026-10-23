## 📝 O que muda

<!-- Resumo em 1 ou 2 frases. -->

## 🎯 Por que

<!-- Qual problema resolve? Ex.: Closes #12 -->

## 🧩 Módulo afetado

- [ ] `lista` (src/lista.*)
- [ ] `formatacao` (src/formatacao.*)
- [ ] `ui` (src/ui/ui.*)
- [ ] `telas` (src/ui/telas.*)
- [ ] `main` (src/main.c)
- [ ] `tests/`
- [ ] Build / Makefile
- [ ] CI / GitHub Actions
- [ ] Documentação

## ✅ Checklist

**Qualidade de código**
- [ ] `make EXTRA_CFLAGS=-Werror all` compila sem warnings
- [ ] `make test` passa sem falhas
- [ ] `make e2e` passa sem falhas
- [ ] `make cppcheck` sem novos warnings
- [ ] `make asan` executado localmente no Linux **ou** CI passa (para mudanças em lógica de memória)

**Boas práticas**
- [ ] Não há tokens, chaves ou senhas neste PR
- [ ] Commits seguem a [convenção do projeto](.github/COMMIT_CONVENTION.md)
- [ ] Documentação atualizada (README, comentários de `.h`) se necessário
- [ ] Sem `printf` de debug no código enviado

## 🧪 Como testar

<!-- Passos para o revisor reproduzir e validar. -->
1.
2.
3.

## 📸 Saída do terminal (se aplicável)

<details>
<summary>Saída</summary>

```
(cole aqui)
```

</details>
