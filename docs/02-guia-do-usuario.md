# 02 — Guia do usuário

← Anterior: [01 — Visão geral](01-visao-geral.md) · [Índice](README.md) · Próximo: [03 — Conceitos fundamentais](03-conceitos-fundamentais.md)

> Este módulo é para quem vai **usar** o OhmFlow. Não precisa saber programar. Para compilar a partir do código, veja [05 — Build e ambiente](05-build-e-ambiente.md).

---

## 1. Instalar (Windows 10/11)

1. Abra a página de [Releases do projeto](https://github.com/Grolla05/OhmFlow/releases) e baixe o arquivo `ohm_flow_setup.exe` da versão mais recente.
2. Dê dois cliques no arquivo. Como o instalador não tem assinatura digital paga, o Windows pode mostrar **"O Windows protegeu o computador"**. Clique em **Mais informações → Executar assim mesmo**.
3. Avance o assistente. As opções estão na Tabela 1.
4. Abra o **OhmFlow** pelo Menu Iniciar (ou pelo atalho da área de trabalho, se marcou essa opção).

**Tabela 1 — Opções do instalador**

| Opção | Efeito | Precisa de administrador? |
| --- | --- | --- |
| Instalar só para mim (padrão) | Instala em `%LOCALAPPDATA%\Programs\OhmFlow` | Não |
| Instalar para todos os usuários | Instala em `C:\Program Files\OhmFlow` | Sim |
| Criar um atalho na área de trabalho | Atalho no desktop | — |
| Adicionar o OhmFlow ao PATH | Permite digitar `ohmflow` em qualquer terminal (abra um terminal novo depois) | — |

**Desinstalar:** Configurações → Aplicativos → Aplicativos instalados → OhmFlow → Desinstalar (ou o atalho "Desinstalar OhmFlow" no Menu Iniciar). O PATH é limpo automaticamente.

O programa só depende de arquivos que já vêm no Windows. Não é preciso instalar mais nada.

## 2. Abrir o programa

Ao abrir, aparece a **tela de boas-vindas** com o logo "OhmFlow" em laranja e a frase *"Sistema pronto. Pressione Enter para continuar"*. Pressione **Enter**. O que mais for digitado nessa tela é ignorado.

> **Dica:** use o **Windows Terminal**, o PowerShell ou o Prompt de Comando. Em terminais que não entendem UTF-8, símbolos como `Ω` e as bordas podem aparecer como `Î©` ou `â•”`; veja a [seção 8](#8-problemas-comuns).

## 3. O menu principal

A Figura 1 mostra o menu. A opção selecionada aparece em laranja e negrito; as outras, em branco. Embaixo, o menu mostra quantos **tipos** de resistor estão cadastrados e quantas **peças** há no total.

```text
╭─  OhmFlow System  ───────────────────╮
│      Controle de Estoque 8-Bit       │
├──────────────────────────────────────┤
│  ● [1] Novo         ● [5] Retirar    │
│  ● [2] Remover      ● [6] Repor      │
│  ● [3] Listar       ● [7] Reinicializar
│  ● [4] Consultar    ● [8] (diferencial)
│  ● [9] Histórico                     │
│  Tipos: 3    │ Peças: 20             │
╰──────────────────────────────────────╯
 ↑↓←→/WASD Navegar · [1-9] Acesso rápido · [Enter] Confirmar · [Esc/0] Sair
```

**Figura 1 — Menu principal (a caixa se alarga para caber o texto mais longo)**

A Tabela 2 lista as teclas do menu.

**Tabela 2 — Teclas do menu**

| Tecla | O que faz |
| --- | --- |
| Setas `↑ ↓ ← →` ou `W A S D` | Move a seleção. `←/→` trocam de coluna; `↑/↓` andam na coluna e passam pela linha do Histórico, dando a volta |
| `1` a `9` | Abre a opção direto, sem precisar navegar |
| `Enter` | Abre a opção selecionada |
| `Esc` ou `0` | Sai do programa na hora |

## 4. Como digitar um valor de resistência

O programa aceita os jeitos de escrever usados na bancada. A Tabela 3 mostra exemplos; todos os jeitos de escrever o mesmo valor dão exatamente o mesmo resultado.

**Tabela 3 — Formatos aceitos**

| Você digita | O programa entende |
| --- | --- |
| `220`, `220R`, `220 ohm`, `220 ohms`, `220Ω` | 220 Ω |
| `4700`, `4.7k`, `4,7k`, `4k7`, `4K7`, `4.7 k` | 4,7 kΩ |
| `1M`, `1M5` | 1 MΩ, 1,5 MΩ |
| `470m`, `0.47`, `0,47` | 470 mΩ |
| `4R7` | 4,7 Ω |

Regras importantes:

- **`m` minúsculo é mili (÷1000) e `M` maiúsculo é mega (×1.000.000).** `470m` são 0,47 Ω; `470M` são 470 milhões de ohms.
- Na notação com a letra no meio (`4k7`, `1M5`, `4R7`), a letra ocupa o lugar da vírgula: `4k7` = 4,7 kΩ.
- A faixa aceita vai de **1 mΩ** (0,001 Ω) a **1000 MΩ** (1 bilhão de ohms).
- **Deixar a resposta em branco (só Enter) cancela a operação** em qualquer pergunta.
- Cada pergunta dá **3 tentativas**. Depois de 3 entradas inválidas, a operação é cancelada.

As quantidades são números inteiros sem sinal (`0`, `10`, `250`...). O máximo é 2.147.483.647 por tipo.

## 5. Cada opção do menu, passo a passo

### [1] Novo — cadastrar resistores

A **primeira linha** que você digita decide o modo:

| Primeira linha | Modo | O que acontece |
| --- | --- | --- |
| Só a resistência (ex.: `4k7`) | **Um item** | O programa pergunta a quantidade e cadastra |
| Resistência e quantidade (ex.: `4k7 50`) | **Em massa** | Cadastra e continua pedindo um item por linha até você dar Enter vazio |

**Um item:**

```text
» Resistência: 4k7
» Quantidade : 10
┌─ ✔ SUCESSO ──────────────────────────┐
│ Resistor inserido                    │
│ Resistor 4,7 kΩ inserido (10 un.)    │
└──────────────────────────────────────┘
```

Se o valor já estiver cadastrado, o programa avisa *"4,7 kΩ já cadastrado - use [6] Repor"* e não pergunta a quantidade.

**Resistor colorido:** no terminal com cores, a caixa do título desenha um resistor. Depois que você digita um valor válido, ele aparece com as **faixas do código de cores** (ex.: `4k7` → *Faixas: amarelo · violeta · vermelho · dourado*), para conferir com o componente na mão. Valores com 3 ou mais algarismos mostram as cores do valor arredondado e "(aprox.)"; abaixo de 0,1 Ω não há código de 4 faixas. Ver [14 — Resistor colorido](funcionalidades/14-resistor-colorido.md).

**Em massa:** digite `<resistência> <quantidade>` por linha. Uma linha só com a resistência faz o programa perguntar a quantidade daquele item. Valores repetidos e linhas inválidas **não interrompem**: são contados e aparecem no resumo final.

```text
» Resistência: 10k 5
  ✔ 10 kΩ inserido (5 un.)
» Resistência [quantidade]: 22k 3
  ✔ 22 kΩ inserido (3 un.)
» Resistência [quantidade]: 10k 1
  ⚠ 10 kΩ já cadastrado - ignorado
» Resistência [quantidade]: lixo 2
  ✖ Linha inválida: "lixo 2"
» Resistência [quantidade]:
┌─ ✔ SUCESSO ──────────────────────────┐
│ Inserção em massa concluída          │
│ Inseridos : 2                        │
│ Duplicados: 1                        │
│ Inválidos : 1                        │
└──────────────────────────────────────┘
```

Um lote inteiro conta como **uma única ação** no desfazer (ver [9] Histórico).

### [2] Remover — apagar um tipo

Digite a resistência. Se ela existir, o programa pergunta *"Remover 1 MΩ (2 unidades) do estoque? (s/n)"*. Responda `s` para apagar ou `n` para cancelar. Qualquer outra resposta pede de novo (até 3 vezes).

Remover apaga o **tipo inteiro**, com toda a quantidade. Para tirar só algumas peças, use [5] Retirar.

### [3] Listar — ver o estoque

Mostra uma tabela com todos os tipos, do menor para o maior valor, e o total no rodapé. Quantidades zeradas aparecem em vermelho.

```text
┌────┬──────────────┬────────────┐
│  # │ Resistência  │ Quantidade │
├────┼──────────────┼────────────┤
│  1 │     470 mΩ   │          5 │
│  2 │     220 Ω    │         50 │
│  3 │     4,7 kΩ   │         10 │
└────┴──────────────┴────────────┘
  3 tipos · 65 peças
```

Com o estoque vazio, aparece *"Estoque vazio. Use [1] Novo."*.

### [4] Consultar — quantas peças tenho?

Digite a resistência e veja a quantidade: `4,7 kΩ → 7 unidades` (verde se houver, vermelho se for zero). Se não estiver cadastrada: *"5 kΩ não está no estoque"*.

### [5] Retirar — dar baixa

1. Digite a resistência (precisa estar cadastrada).
2. O programa mostra *"Em estoque: 10 unidades"*.
3. Digite quanto quer retirar (mínimo 1).

| Situação | Resultado |
| --- | --- |
| Pediu menos do que tem | ✔ *"4,7 kΩ: 10 → 7 unidades"* |
| Pediu exatamente o que tem | ⚠ *"Retirada concluída - estoque zerado"*; o tipo **continua cadastrado** com 0 |
| Pediu mais do que tem | ✖ *"Estoque insuficiente — 4,7 kΩ tem 7, pedido 100"*; nada muda |

### [6] Repor — somar ao estoque

Igual ao Retirar, mas soma: *"220 Ω: 50 → 60 unidades"*. Funciona também em tipos com quantidade zero. Se a soma passar do limite de 2.147.483.647, a operação é recusada.

### [7] Reinicializar — esvaziar tudo

Se houver resistores, o programa mostra um aviso em amarelo (*"Tem certeza disso, você está apagando sua base de dados."*) e pergunta *"Apagar TODOS os 3 tipos (68 peças)? (s/n)"*. Com `s`, a lista fica vazia. Se a lista já estiver vazia, só confirma que está pronta para uso.

> Arrependeu? Enquanto o programa estiver aberto, dá para desfazer pelo **[9] Histórico**.

### [8] Diferencial

Ainda em desenvolvimento: mostra *"Em breve"*.

### [9] Histórico — o que aconteceu e voltar no tempo

A tela tem duas partes.

**Parte 1 — Últimas ações (do arquivo de log).** Uma tabela com as 15 ações mais recentes dos últimos 30 dias, a mais nova em cima, com data/hora, ação, resistor, detalhe e resultado (verde = ok, amarelo = aviso, vermelho = erro). Inclui ações de execuções anteriores do programa.

**Parte 2 — Checkpoint da sessão.** Se você mudou o estoque nesta execução, aparece *"[C] Voltar a um checkpoint desta sessão · [Enter] Voltar ao menu"*. Com `C`, abre a **linha do tempo**:

```text
┌─────┬──────────┬───────────────┬────────────┬────────────┬────────────┐
│ #   │ Hora     │ Ação          │ Resistor   │ Detalhe    │ Estado     │
├─────┼──────────┼───────────────┼────────────┼────────────┼────────────┤
│ 3   │ 14:05:02 │ REINICIALIZAR │ -          │ 3 tipos    │ » atual    │
│ 2   │ 14:04:10 │ RETIRAR       │ 4,7 kΩ     │ -3         │ aplicada   │
│ 1   │ 14:03:20 │ NOVO          │ 3 tipos    │ lote       │ aplicada   │
│ 0   │ -        │ INÍCIO        │ -          │ sessão     │ -          │
└─────┴──────────┴───────────────┴────────────┴────────────┴────────────┘
```

- `↑`/`W` sobe para ações mais recentes; `↓`/`S` desce para as mais antigas. O ponto escolhido fica em laranja.
- `Enter` leva o estoque até aquele ponto. Ir para o ponto **2** no exemplo desfaz o Reinicializar e traz os 3 tipos de volta.
- Ações desfeitas aparecem em cinza ("desfeita") e podem ser **refeitas** escolhendo um ponto acima.
- `Esc` volta sem mudar nada.
- O ponto **0** é o estoque como estava ao abrir o programa (vazio).

> **Atenção:** se você desfizer ações e depois fizer uma ação **nova**, as ações desfeitas são descartadas (não dá mais para refazê-las). É o mesmo comportamento do "Ctrl+Z / Ctrl+Y" de editores de texto.

O desfazer vale **só para a sessão atual**. Ao fechar o programa, a linha do tempo é perdida (o log em arquivo continua).

### Sair

`Esc` ou `0` no menu mostram *"Obrigado por usar o OhmFlow!"* com o estoque final e fecham o programa, liberando toda a memória.

## 6. Onde ficam os registros (logs)

Cada ação vira uma linha em um arquivo de texto por dia, `ohmflow_AAAA-MM-DD.log`:

| Como o programa foi instalado | Pasta dos logs |
| --- | --- |
| Pelo instalador | `%APPDATA%\OhmFlow\logs\` (cole na barra de endereços do Explorador de Arquivos) |
| Compilado do código (`make run`) | `logs\` dentro da pasta de onde foi executado |
| Variável de ambiente `OHMFLOW_LOGS_DIR` definida | A pasta indicada nela |

Exemplo de conteúdo:

```text
================================================================
 OhmFlow — sessão iniciada em 27/10/2026 14:03:12
================================================================
27/10/2026 14:03:20 | INFO  | NOVO          | 4,7 kΩ     | qtd 50               | OK
27/10/2026 14:04:01 | AVISO | RETIRAR       | 4,7 kΩ     | -70 (tem 50)         | SEM ESTOQUE
27/10/2026 14:04:15 | ERRO  | NOVO          | "abc"      | -                    | VALOR INVÁLIDO
================================================================
 OhmFlow — sessão encerrada em 27/10/2026 14:06:10 (2m 58s)
================================================================
```

Os arquivos podem ser abertos no Bloco de Notas e apagados à vontade; o programa cria de novo quando precisar. Se a pasta não puder ser criada, o programa avisa uma vez e continua funcionando sem registro.

## 7. Opções para terminais especiais

| Situação | O que fazer |
| --- | --- |
| Quer tudo sem cores | No PowerShell: `$env:NO_COLOR=1` antes de abrir o programa |
| O terminal não mostra `Ω` e as bordas | Use o Windows Terminal, ou compile com `make EXTRA_CFLAGS=-DOHM_ASCII` (troca por `ohm` e `+-|`) |

## 8. Problemas comuns

| Sintoma | Causa e solução |
| --- | --- |
| "O Windows protegeu o computador" ao instalar | O instalador não é assinado. Clique em **Mais informações → Executar assim mesmo** |
| `Ω` aparece como `Î©`, bordas como `â•”` | O terminal não está em UTF-8. Use o Windows Terminal |
| Sem cores, sem limpar a tela, sem pausas | O programa está num terminal que ele não reconhece como interativo (ex.: mintty do MSYS2) ou com entrada redirecionada. Abra no Windows Terminal |
| `ohmflow` não é reconhecido no terminal | Marque "Adicionar ao PATH" no instalador e abra um terminal **novo** |
| "Valor inválido" para `4.7.1` ou `4kk` | O formato não é aceito. Veja a [Tabela 3](#4-como-digitar-um-valor-de-resistência) |
| O estoque sumiu ao abrir de novo | Comportamento esperado: o estoque não é salvo entre execuções ([por quê](08-decisoes-de-projeto.md)) |

---

← Anterior: [01 — Visão geral](01-visao-geral.md) · [Índice](README.md) · Próximo: [03 — Conceitos fundamentais](03-conceitos-fundamentais.md)
