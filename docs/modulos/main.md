# Módulo Main — ponto de entrada e integração com o Windows

← Anterior: [Telas](telas.md) · [Módulos](README.md)

**Arquivos:** [src/main.c](../../src/main.c) · [src/assets/ohmflow.rc](../../src/assets/ohmflow.rc) · `src/assets/logo/`
**Testes:** cobertos pelo teste ponta a ponta (`make e2e`) e pelo teste do instalador ([06 — Testes](../06-testes.md))

> **Resumo:** `main.c` é só a **costura**: prepara o terminal, cria a lista e o checkpoint, abre o log, roda o laço do menu e, no fim, libera tudo. `ohmflow.rc` embute no `.exe` o ícone e as informações de versão.

---

## 1. Explicação para leigos

É o **gerente que abre e fecha a loja**: acende as luzes (terminal), monta as prateleiras vazias (lista), abre o livro de ocorrências (log), chama o atendente para cada cliente (telas) e, ao fim do expediente, guarda tudo e apaga as luzes (libera a memória).

## 2. O código, passo a passo

```c
int main(void)
{
    Lista *estoque;
    Checkpoint *checkpoint;
    int opcao;

    ui_init();                                 /* 1 */
    ui_definir_centralizado(ui_interativo());  /* 2 */
    estoque = cria_lista();                    /* 3 */
    checkpoint = cria_checkpoint();            /* 4 */
    if (estoque == NULL || checkpoint == NULL) { ... return EXIT_FAILURE; } /* 5 */
    telas_definir_checkpoint(checkpoint);      /* 6 */
    if (!log_iniciar()) ui_mensagem(UI_AVISO, "...seguindo sem registro."); /* 7 */

    tela_boas_vindas();                        /* 8 */
    while ((opcao = tela_menu(estoque)) != OPCAO_SAIR) {   /* 9 */
        switch (opcao) { case OPCAO_INSERIR: tela_inserir(estoque); break; ... }
    }
    tela_encerramento(estoque);                /* 10 */
    log_encerrar();                            /* 11 */
    telas_definir_checkpoint(NULL);            /* 12 */
    libera_checkpoint(checkpoint);
    libera_lista(estoque);
    return EXIT_SUCCESS;
}
```

**Tabela 1 — O que cada passo faz**

| Passo | O que faz | Por quê |
| --- | --- | --- |
| 1 | `ui_init`: UTF-8, cores, título, ícone | Precisa vir antes de qualquer saída |
| 2 | Centraliza as telas só se houver terminal | Nos testes, saída previsível sem recuos variáveis |
| 3 | Cria a lista vazia | Requisito "inicializar a lista" |
| 4 | Cria as pilhas de desfazer/refazer | Usadas pelo [9] Histórico |
| 5 | Se faltar memória, avisa e sai liberando o que foi criado | `libera_*` aceitam `NULL`, então é seguro liberar os dois |
| 6 | Entrega o checkpoint às telas | As telas registram cada ação |
| 7 | Abre a sessão do log; se falhar, segue sem log | Log é "best effort" ([Log](log.md)) |
| 8–9 | Boas-vindas e laço do menu | Uma volta por opção escolhida |
| 10–11 | Tela final e rodapé do log com a duração | — |
| 12 | Desliga o checkpoint das telas e libera tudo | **Nenhum vazamento**: cada `cria_*` tem seu `libera_*` |

## 3. Integração com o Windows

### 3.1 Programa de console (CUI)

O executável é gerado com o **subsistema console** (CUI), o padrão do `gcc`. Isso faz o `cmd` e o PowerShell **esperarem o programa terminar** antes de mostrar o prompt de novo, e faz o Windows abrir uma janela de console ao dar duplo clique. O `make e2e` confere isso com `objdump -p` procurando `(Windows CUI)` (a issue nº 11 do repositório registra o problema que isso evita).

### 3.2 Ícone e versão (`ohmflow.rc`)

```text
src/assets/logo/logo.png --(make icone: Python + Pillow)--> src/assets/logo/ohmflow.ico
src/assets/ohmflow.rc    --(windres)-------------------------> build/ohmflow_res.o
build/ohmflow_res.o      --(linkado junto)-------------------> ohmflow.exe com ícone e versão
```

- `1 ICON "src/assets/logo/ohmflow.ico"` cria o recurso de ícone de **id 1**. O Explorer usa esse ícone para o arquivo, e `ui_init` o aplica à janela do console (`LoadIconA` + `WM_SETICON`).
- `1 VERSIONINFO` grava produto, versão, empresa (`Grolla05`) e copyright, visíveis em **Propriedades → Detalhes**. A versão vem do `Makefile` (`VERSAO=1.0.0` → `VERSAO_NUM=1,0,0,0`).
- O `.ico` (16, 32, 48 e 256 px) é **versionado no Git**, para o build não exigir Python. Só é preciso `make icone` ao trocar o logo.

### 3.3 Dependências do executável

O `ohmflow.exe` depende só de `KERNEL32`, `USER32` e da UCRT, que já vêm no Windows 10/11. Por isso pode ser copiado para outro computador ou instalado sem MSYS2.

## 4. Testes

`main.c` não tem teste unitário próprio (só costura). Ele é exercitado inteiro pelo **e2e**, que roda o executável real com um roteiro de entradas, e pelo **teste do instalador**, que roda o exe instalado. Ver [06 — Testes](../06-testes.md).

---

← Anterior: [Telas](telas.md) · [Módulos](README.md) · Próximo: [Funcionalidades](../funcionalidades/README.md)
