# Política de Segurança — OhmFlow

## Versões suportadas

| Branch / Tag | Suportada |
|--------------|-----------|
| `main`       | ✅ Sim    |
| Outras       | ❌ Não    |

## Reportando uma vulnerabilidade

O OhmFlow é um programa de linha de comando sem rede, sem banco de dados e sem tratamento de dados sensíveis. O escopo de segurança relevante inclui:

- Vulnerabilidades de corrupção de memória (buffer overflow, use-after-free, etc.)
- Comportamento indefinido que possa ser explorado com entrada maliciosa
- Leitura de arquivos arbitrários via caminhos controlados pelo usuário

**Não reporte via issue pública** se a vulnerabilidade puder ser explorada antes da correção.

### Como reportar

Envie um e-mail para **fegrolla0210@gmail.com** com:

1. Descrição clara da vulnerabilidade
2. Passos para reproduzir (entrada, flags de compilação, OS)
3. Impacto potencial
4. Sugestão de correção (se houver)

Resposta esperada em até **7 dias úteis**.

## Processo de divulgação responsável

1. Você reporta em privado
2. Confirmamos o recebimento em até 48 h
3. Investigamos e desenvolvemos a correção
4. Publicamos o fix + release notes com crédito ao relator (salvo solicitação de anonimato)
5. Após o merge, a issue pública pode ser aberta
