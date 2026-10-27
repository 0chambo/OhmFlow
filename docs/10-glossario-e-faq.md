# 10 — Glossário e FAQ

← Anterior: [09 — Contribuindo](09-contribuindo.md) · [Índice](README.md)

---

## 1. Glossário

### Eletrônica

| Termo | Significado |
| --- | --- |
| **Resistor** | Componente que dificulta a passagem da corrente elétrica; o item do estoque |
| **Resistência** | O "quanto" o resistor dificulta a corrente; medida em ohms |
| **Ohm (Ω)** | Unidade de resistência |
| **mΩ, kΩ, MΩ** | Miliohm (0,001 Ω), kiloohm (1.000 Ω), megaohm (1.000.000 Ω) |
| **Ordem de grandeza** | A "faixa" do valor (mΩ, Ω, kΩ ou MΩ), escolhida para escrever o número de forma curta |
| **Notação RKM** | Escrita com a letra no lugar da vírgula: `4k7` = 4,7 kΩ, `4R7` = 4,7 Ω |
| **Almoxarifado** | Depósito de materiais do laboratório |
| **Código de cores** | Faixas pintadas no resistor que indicam o valor: 2 algarismos, multiplicador e tolerância |
| **Tolerância** | Margem de erro do valor real do resistor (dourado = 5%) |
| **Pixel-art / meio-bloco** | Desenho feito de "quadradinhos"; o caractere `▀` mostra 2 pixels por célula do terminal |
| **Cor de 24 bits (true color)** | Cor definida por vermelho, verde e azul (0–255 cada), usada no desenho do resistor |

### Programação e estruturas de dados

| Termo | Significado |
| --- | --- |
| **Código-fonte** | O texto do programa, escrito em C (`.c`, `.h`) |
| **Compilador (gcc)** | Traduz o código-fonte em executável |
| **Executável (.exe)** | O arquivo que o Windows roda |
| **Cabeçalho (.h)** | Arquivo que declara o que um módulo oferece |
| **Variável / tipo** | Nome de um espaço na memória / o que cabe nele (`int`, `double`...) |
| **Struct** | Ficha que agrupa vários campos (ex.: `Resistor`) |
| **Ponteiro** | Variável que guarda um endereço de memória |
| **`NULL`** | Ponteiro que não aponta para nada (fim da lista) |
| **`malloc` / `free`** | Pedir / devolver memória durante a execução |
| **Vazamento de memória** | Memória pedida e nunca devolvida |
| **Nó** | Cada elemento de uma lista ou pilha encadeada |
| **Lista encadeada** | Nós ligados por ponteiros, como vagões de um trem |
| **Lista ordenada** | Lista mantida sempre em ordem pela chave |
| **Chave** | Campo usado para ordenar e buscar (aqui, a resistência) |
| **Nó descritor** | Ficha com o início e os totais da lista |
| **Pilha (LIFO)** | Estrutura em que o último a entrar é o primeiro a sair |
| **TAD** | Tipo Abstrato de Dados: estrutura + operações, com o interior escondido |
| **Tipo opaco** | Tipo declarado sem mostrar os campos, para forçar o uso das funções |
| **Callback** | Função passada como parâmetro para ser chamada depois (ex.: em `percorre_lista`) |
| **O(1), O(n)** | Trabalho constante / proporcional ao número de elementos |
| **Ponto flutuante (`double`)** | Formato de números com casas decimais; nem todo decimal é exato |
| **Buffer circular** | Vetor fixo reaproveitado em círculo, guardando só os últimos itens |
| **Código de retorno** | Número que uma função devolve para dizer o que aconteceu |
| **Comportamento indefinido** | Situação em que a linguagem C não garante o resultado (ex.: estouro de `int`) |

### Ferramentas e processo

| Termo | Significado |
| --- | --- |
| **Terminal / console** | Janela de texto onde o programa roda |
| **ANSI** | Códigos invisíveis que mudam cor e posição do texto no terminal |
| **UTF-8** | Codificação de texto que representa acentos, `Ω`, `✔` |
| **Modo interativo** | Há uma pessoa no teclado e na tela |
| **Entrada redirecionada** | O "teclado" é um arquivo (usado nos testes) |
| **make / Makefile** | Ferramenta de automação / sua receita |
| **MSYS2, UCRT64** | Ambiente que traz gcc e make ao Windows / variante que usa a biblioteca C moderna do Windows |
| **Teste unitário** | Verifica um módulo isolado |
| **e2e (ponta a ponta)** | Verifica o programa inteiro, como um usuário usaria |
| **cppcheck** | Analisador que procura erros sem executar o código |
| **Sanitizer (ASan/UBSan)** | Instrumentação que detecta erros de memória durante a execução |
| **Git, commit, branch, PR** | Controle de versão / um registro de mudança / uma linha de trabalho / pedido para juntar mudanças |
| **CI** | Integração contínua: verificações automáticas a cada envio |
| **Artifact / Release** | Arquivo gerado pelo CI / versão publicada no GitHub |
| **Inno Setup** | Programa que gera o instalador |
| **Log** | Registro de ações em arquivo |
| **Checkpoint** | Ponto da linha do tempo da sessão para onde o estoque pode voltar |

## 2. Perguntas frequentes

**Por que o estoque some quando fecho o programa?**
Porque os dados vivem só na memória RAM, como pede o foco da disciplina (estruturas dinâmicas). O que fica salvo é o registro de ações. Ver [D7](08-decisoes-de-projeto.md#d7--estoque-só-em-memória-sem-persistência).

**Posso desfazer uma ação de ontem?**
Não. O desfazer vale só para a sessão aberta. As ações de ontem aparecem no Histórico (log), mas só para consulta.

**Qual a diferença entre Remover e Retirar?**
Retirar tira algumas peças e mantém o tipo (mesmo com zero). Remover apaga o tipo inteiro.

**Digitei `470M` e apareceu um valor enorme. Por quê?**
`M` maiúsculo é mega. Para miliohm, use `m` minúsculo: `470m`.

**Por que não posso cadastrar um valor que já existe para somar?**
Somar é papel do [6] Repor. Separar evita que um erro de digitação passe despercebido.

**Como corrijo um valor cadastrado errado?**
Remova e cadastre de novo. A chave de ordenação nunca é editada no lugar, para a lista continuar ordenada.

**O programa funciona no Linux ou no Mac?**
O código tem trechos para esses sistemas, mas só o Windows 10/11 é suportado e testado.

**Onde estão os logs do programa instalado?**
Em `%APPDATA%\OhmFlow\logs` (cole no Explorador de Arquivos).

**Posso apagar os arquivos de log?**
Sim. O Histórico só perde as linhas antigas; o programa cria arquivos novos quando precisar.

**Por que o Windows avisa que o instalador pode ser perigoso?**
Porque ele não tem assinatura digital paga. Clique em "Mais informações → Executar assim mesmo".

**O que é o item [8] Diferencial?**
A funcionalidade extra exigida pelo enunciado; ainda está sendo definida. Ver [funcionalidades/08-diferencial.md](funcionalidades/08-diferencial.md).

**Quantos resistores o programa aguenta?**
Tantos tipos quanto couberem na memória; até 2.147.483.647 peças por tipo, e o total de peças usa um inteiro de 64 bits.

**Como rodo os testes?**
`make test` (unitários), `make e2e` (ponta a ponta). Ver [06 — Testes](06-testes.md).

---

← Anterior: [09 — Contribuindo](09-contribuindo.md) · [Índice](README.md)
