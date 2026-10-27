# 08 — Decisões de projeto

← Anterior: [07 — CI e instalador](07-ci-e-instalador.md) · [Índice](README.md) · Próximo: [09 — Contribuindo](09-contribuindo.md)

> Este módulo reúne **o porquê** das escolhas importantes, no formato de registros de decisão: contexto, decisão, alternativas e consequências. Serve para quem quer entender o projeto e para quem precisa mudar algo sem desfazer uma escolha consciente.

---

## D1 — Lista encadeada ordenada como estrutura central

- **Contexto:** o enunciado exige uma Lista Dinâmica Ordenada adaptada da desordenada vista em aula.
- **Decisão:** lista simplesmente encadeada, inserção já na posição certa, chave = resistência.
- **Alternativas:** vetor ordenado (busca binária O(log n), mas inserção desloca elementos); árvore de busca (fora do escopo).
- **Consequências:** busca O(n) com parada antecipada; listagem sem ordenação; inserção/remoção trocam só ponteiros.

## D2 — Nó descritor com totais

- **Contexto:** o menu mostra tipos e peças e é redesenhado a cada tecla.
- **Decisão:** um struct descritor com `inicio`, `tipos`, `pecas` e `passos`, atualizado a cada operação.
- **Alternativa:** recontar a lista a cada desenho (O(n) por tecla).
- **Consequências:** totais em O(1); toda operação precisa manter os contadores (garantido por `teste_contadores_invariantes`).

## D3 — Tipos opacos (TAD)

- **Decisão:** `Lista`, `Pilha` e `Checkpoint` são declarados no `.h` sem os campos.
- **Por quê:** ninguém fora do módulo consegue quebrar a ordenação, os totais ou a posse da memória.
- **Custo:** acesso só por funções (inclusive `percorre_lista` com callback para listar).

## D4 — Ponteiro para ponteiro em `localiza`

- **Decisão:** a busca devolve o endereço do ponteiro que aponta para o nó (`Elem **`).
- **Alternativa:** o par `ant`/`atual` da aula, com caso especial para o início.
- **Consequências:** inserir e remover em qualquer posição com as mesmas duas linhas; menos ramos, menos bugs. É uma técnica clássica, boa para explicar no relatório.

## D5 — Resistência em `double`, em ohms, com chave canônica

- **Contexto:** valores de 0,001 a 1.000.000.000 Ω, com casas decimais.
- **Decisão:** guardar em `double` sempre em Ω; converter o texto com um único `strtod` sobre a forma "A.Be<exp>"; comparar com tolerância relativa 1e-9.
- **Alternativas:** inteiro em mΩ (1000 MΩ = 10¹² mΩ não cabe em `int`); multiplicar por 0,001 (gera `double` diferentes para a mesma grafia).
- **Consequências:** `470m` e `0.47` são a mesma chave; nenhum duplicado invisível.

## D6 — Regra de negócio separada da interface

- **Decisão:** `lista`, `pilha`, `checkpoint` e `formatacao` não imprimem; devolvem códigos. `telas` traduz para mensagens.
- **Consequências:** módulos testáveis isoladamente; interface trocável; relatório mais claro.

## D7 — Estoque só em memória (sem persistência)

- **Contexto:** o enunciado pede a lista dinâmica; não pede arquivos.
- **Decisão:** o estoque vive na RAM e é liberado ao sair.
- **Consequências:** ao reabrir, o estoque está vazio. O desfazer vale só para a sessão. Salvar/carregar em arquivo é candidato a [diferencial](funcionalidades/08-diferencial.md) (ver [refs/BRAINSTORM.md](../refs/BRAINSTORM.md), inclusive a ideia futura de SQLite).

## D8 — Desfazer com duas pilhas de operações

- **Decisão:** registrar a **operação** (com o necessário para invertê-la) em vez de cópias do estoque; duas pilhas, desfazer e refazer.
- **Alternativas:** foto do estoque a cada ação (memória × ações); ler o log (valores já formatados, imprecisos).
- **Consequências:** memória proporcional às ações; segunda estrutura de dados no projeto; regra de segurança: se faltar memória para registrar, o histórico é descartado (nunca uma inversa errada).

## D9 — Log em texto, um arquivo por dia, "best effort"

- **Decisão:** linhas de colunas fixas em UTF-8, `fflush` a cada linha, falhas de I/O viram no-op.
- **Por quê:** legível por qualquer pessoa; sobrevive a fechamentos bruscos; nunca atrapalha o controle de estoque.

## D10 — Interface de terminal própria, sem bibliotecas

- **Alternativas:** `ncurses`/`PDCurses`, interface gráfica.
- **Decisão:** primitivas próprias (`ui.c`) com códigos ANSI e a API de console do Windows.
- **Consequências:** zero dependências; exe autossuficiente; modo não interativo permite testes e e2e; o projeto controla o alinhamento com UTF-8 e cores.

## D11 — Somente Windows

- **Contexto:** a turma e o laboratório usam Windows; manter três plataformas custaria CI e testes.
- **Decisão:** suporte oficial só a Windows 10/11 (MSYS2 UCRT64). Os ramos `#ifndef _WIN32` continuam no código, mas não são testados.
- **Consequências:** CI com um único job; instalador nativo; Linux/macOS "podem funcionar", sem garantia.

## D12 — Subsistema console (CUI)

- **Contexto:** um build com subsistema GUI fazia o `cmd`/PowerShell não esperarem o programa (issue nº 11).
- **Decisão:** manter o subsistema console padrão e verificá-lo no e2e com `objdump`.

## D13 — Instalador por usuário, logs em `%APPDATA%`

- **Decisão:** instalar sem administrador por padrão; o exe do instalador grava logs em `%APPDATA%\OhmFlow\logs`.
- **Por quê:** a maioria das pessoas não tem (ou não quer usar) senha de administrador; `Program Files` não aceita escrita.

## D14 — Mini framework de testes próprio

- **Alternativas:** Unity, CMocka, Check.
- **Decisão:** ~80 linhas em `test_framework.h`.
- **Por quê:** zero dependências, roda em qualquer lugar onde o gcc rode, fácil de entender para a turma.

## D15 — Tudo em português

- **Decisão:** nomes de funções, comentários, mensagens, commits e documentação em pt-BR.
- **Por quê:** projeto acadêmico de turma brasileira; o código serve de material de estudo.

## D16 — Sair como tecla, Histórico como item 9

- **Decisão:** `Esc`/`0` saem a qualquer momento do menu; a linha de baixo do menu, antes "[0] Sair", virou `[9] Histórico`.
- **Por quê:** acesso direto ao histórico sem aumentar o menu; `Esc` é o gesto universal de sair.

## D17 — Código de cores dentro do Novo, desenho só no terminal com cores

- **Contexto:** o código de cores era uma das ideias de diferencial; ele faz mais sentido no momento do cadastro, com o componente na mão.
- **Decisão:** módulo `resistor` separado (lógica pura testável) e desenho em pixel-art de meio-blocos com cor de 24 bits na caixa da tela Novo; desligado sem terminal, sem cores ou com `-DOHM_ASCII`; modo forçável (`telas_definir_resistor`) para os testes.
- **Consequências:** o e2e e os testes de texto não mudam; a opção `[8]` continua livre para outro diferencial.

---

← Anterior: [07 — CI e instalador](07-ci-e-instalador.md) · [Índice](README.md) · Próximo: [09 — Contribuindo](09-contribuindo.md)
