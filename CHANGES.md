# CHANGES

## docs: documentação completa e modular do projeto em `docs/`

### O que foi feito

Criada a pasta `docs/` com a documentação completa do OhmFlow, escrita em módulos e pensada para qualquer leitor, inclusive sem conhecimento técnico (cada termo é explicado na primeira vez; glossário no fim). O código em `src/` e os testes são a fonte da verdade: a documentação descreve o estado commitado (até `b090fa3`).

### Estrutura (37 arquivos)

- **`docs/README.md`**: índice geral, trilhas de leitura por público (usuário, leigo, estudante, desenvolvedor, quem publica versões) e convenções.
- **Parte I — produto**: `01-visao-geral.md` (problema, objetivo acadêmico, requisitos do enunciado × onde são atendidos, limites, equipe, tecnologias) e `02-guia-do-usuario.md` (instalar, menu, cada opção, logs, problemas comuns).
- **`docs/funcionalidades/`** — **um arquivo por feature** (README + 14): `00-boas-vindas-e-menu`, `01-novo`, `02-remover`, `03-listar`, `04-consultar`, `05-retirar`, `06-repor`, `07-reinicializar`, `08-diferencial`, `09-historico`, `10-desfazer-refazer`, `11-registro-de-acoes`, `12-entrada-de-valores`, `13-sair`. Cada um traz: para que serve, como usar (com telas de exemplo), regras e casos especiais, caminho no código, linhas gravadas no log, se é desfazível, porquês e testes.
- **Parte II — código**: `03-conceitos-fundamentais.md` (memória, ponteiros, struct, lista encadeada, pilha, TAD, O(n), ponto flutuante, modo interativo, testes — do zero), `04-arquitetura.md` (árvore, camadas, dependências, ciclo de vida, caminho de uma ação, tabela de todas as estruturas de dados, chaves de compilação) e **`docs/modulos/`** com um documento por arquivo de `src/`: `lista`, `formatacao`, `pilha`, `checkpoint`, `log`, `ui`, `telas`, `main` (estruturas, API, algoritmos com diagramas Mermaid, invariantes, decisões, testes).
- **Parte III — build/entrega**: `05-build-e-ambiente.md` (MSYS2 passo a passo, Makefile alvo por alvo, opções, problemas), `06-testes.md` (framework próprio, 8 suítes, roteiro do e2e etapa por etapa, teste do instalador, cppcheck/ASan) e `07-ci-e-instalador.md` (workflow do GitHub Actions, Inno Setup, Releases).
- **Parte IV — contexto**: `08-decisoes-de-projeto.md` (16 registros de decisão D1–D16), `09-contribuindo.md` (fluxo do AGENTS.md, convenções, roteiro para adicionar uma feature usando o diferencial como exemplo, checklist de PR, como manter a doc) e `10-glossario-e-faq.md`.

### Validação

- Script de checagem de links relativos e âncoras em todos os `.md` de `docs/`: 0 links quebrados.
- Fatos conferidos contra o código: códigos de retorno, mensagens de tela, textos de log, roteiro do e2e (`tests/e2e/entrada.txt` / `esperado.txt`), alvos do Makefile, CI e `.iss`.

### Não coberto

- O trabalho em andamento do item diferencial (código de cores / desenho do resistor na tela Novo, `telas_definir_resistor`, `resistor.h`), ainda não commitado: `docs/funcionalidades/08-diferencial.md` continua descrevendo o placeholder "Em breve" e deve ser atualizado quando essa feature entrar.
- Diagramas Mermaid não foram renderizados visualmente aqui (sintaxe padrão, que o GitHub desenha).

### Conferência final e README

- Docs atualizadas para o resistor colorido (commits `7534158`, `2dd92bd`): novos `docs/modulos/resistor.md` e `docs/funcionalidades/14-resistor-colorido.md`; ajustes em índices, arquitetura, Novo, telas, ui (API completa), testes (9 suítes, 153 testes), build, diferencial, decisões (D17), glossário e contribuindo.
- `README.md` da raiz virou cartão de boas-vindas: logo, badges, espaços para prints (`docs/img/`), destaques, começar, links para `docs/` e equipe com links para o GitHub de cada integrante.
- `.github/CONTRIBUTING.md`: link de instalação aponta para `docs/05-build-e-ambiente.md` (a âncora antiga do README não existe mais).
