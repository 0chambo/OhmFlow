<div align="center">

<img src="src/assets/logo/logo.png" alt="Logo do OhmFlow" width="160">

# OhmFlow

**Controle de estoque de resistores para o almoxarifado do laboratório — direto no terminal.**

[![CI](https://github.com/Grolla05/OhmFlow/actions/workflows/ci.yml/badge.svg)](https://github.com/Grolla05/OhmFlow/actions/workflows/ci.yml)
![C](https://img.shields.io/badge/linguagem-C-orange)
![Windows](https://img.shields.io/badge/plataforma-Windows%2010%2F11-blue)
[![Licença MIT](https://img.shields.io/badge/licen%C3%A7a-MIT-green)](LICENSE)

[**⬇️ Baixar**](https://github.com/Grolla05/OhmFlow/releases) · [**📖 Documentação**](docs/README.md) · [**🧭 Guia do usuário**](docs/02-guia-do-usuario.md)

</div>

---

## 👋 Bem-vindo

O **OhmFlow** guarda quantos resistores de cada valor existem no laboratório: cadastre, consulte, retire, reponha e acompanhe o histórico, com tudo sempre em ordem crescente de resistência. Por trás da interface está uma **lista dinâmica ordenada** em C, o tema do Projeto 01 da disciplina *Estrutura de Dados e Recuperação da Informação* (PUC-Campinas).

## 📸 Telas

| Boas-vindas | Menu principal |
| :---: | :---: |
| <!-- adicione: docs/img/boas-vindas.png --> _print em breve_ | <!-- adicione: docs/img/menu.png --> _print em breve_ |
| **Novo (resistor colorido)** | **Estoque atual** |
| <!-- adicione: docs/img/novo.png --> _print em breve_ | <!-- adicione: docs/img/listar.png --> _print em breve_ |
| **Histórico e checkpoint** | **Encerramento** |
| <!-- adicione: docs/img/historico.png --> _print em breve_ | <!-- adicione: docs/img/encerramento.png --> _print em breve_ |

> Para adicionar um print, salve a imagem em `docs/img/` e troque o comentário por `![Descrição](docs/img/arquivo.png)`.

## ✨ Destaques

- Cadastro um a um ou em massa, com `4k7`, `470m`, `1M5` e outros formatos de bancada
- Tabela por ordem de grandeza (mΩ, Ω, kΩ, MΩ) e totais sempre visíveis
- Resistor desenhado com o **código de cores** do valor digitado
- **Desfazer/refazer** qualquer ação da sessão e registro de tudo em log
- Instalador para Windows, sem precisar de compilador

## 🚀 Começar

**Usar:** baixe o `ohm_flow_setup.exe` em [Releases](https://github.com/Grolla05/OhmFlow/releases) e instale.

**Compilar:** com MSYS2 (gcc + make) configurado, rode `make` e depois `.\dist\ohmflow.exe`. O passo a passo está em [docs/05-build-e-ambiente.md](docs/05-build-e-ambiente.md).

## 📚 Documentação

Toda a parte técnica está em [**docs/**](docs/README.md), escrita para qualquer leitor: [visão geral](docs/01-visao-geral.md), [funcionalidades](docs/funcionalidades/README.md), [arquitetura](docs/04-arquitetura.md), [módulos do código](docs/modulos/README.md), [testes](docs/06-testes.md) e [como contribuir](docs/09-contribuindo.md).

## 👥 Equipe

| [<img src="https://github.com/Grolla05.png" width="80"><br>**Felipe Grolla Freitas**](https://github.com/Grolla05) | [<img src="https://github.com/Nogzzin.png" width="80"><br>**Guilherme Oliveira Nogueira**](https://github.com/Nogzzin) | [<img src="https://github.com/gisalvagnini.png" width="80"><br>**Giovanna Lima Salvagnini**](https://github.com/gisalvagnini) | [<img src="https://github.com/0chambo.png" width="80"><br>**Henrique Spadaccia Chambó**](https://github.com/0chambo) |
| :---: | :---: | :---: | :---: |

<div align="center">

Feito para a disciplina de Estrutura de Dados e Recuperação da Informação · PUC-Campinas · [Licença MIT](LICENSE)

</div>
