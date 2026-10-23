# CHANGES

## Documentação: armazenamento de dados e algoritmos da lista

Alteração somente de documentação, sem mudança de código. Por isso não há testes novos (passo 1 do workflow não se aplica).

### [README.md](README.md)

Nova seção **"Como os dados são guardados"**, entre "Estrutura" e "Instalação dos pré-requisitos":

- Explica que os dados ficam só na RAM, em lista simplesmente encadeada ordenada por resistência, sem persistência em arquivo ou banco.
- Diagrama Mermaid da estrutura da lista e trecho em C de `Resistor` e `struct elemento`.
- **Inserir** (`insere_lista_ordenada`): fluxograma Mermaid do algoritmo, exemplo visual inserindo 5 kΩ em `220 Ω → 4,7 kΩ → 10 kΩ` e tabela de casos de borda (início, fim, duplicado, inválido).
- **Remover** (`remove_lista`): fluxograma Mermaid e exemplo removendo 4,7 kΩ.
- **Editar o estoque** (`retira_estoque` e `repor_estoque`): fluxograma Mermaid único com os dois ramos. Esclarece que não existe função de editar a resistência e que trocar o valor é remover e inserir de novo. Retirar até zerar mantém o tipo cadastrado.
- Ajuste: diagramas Mermaid de "Antes e Depois" também para Remover (4,7 kΩ de `220 Ω → 4,7 kΩ → 10 kΩ`), Retirar (12 → 7) e Repor (12 → 20), no mesmo formato do exemplo de inserção. Adicionada tabela de casos de borda da remoção (primeiro nó, último nó, não cadastrado).
- Tabela de Listar, Reinicializar e Sair (`percorre_lista`, `reinicializa_lista`, `libera_lista`).
