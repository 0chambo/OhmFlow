# test_setup.ps1 - teste do instalador dist\ohm_flow_setup.exe
#
# Instala em silencio numa pasta temporaria (so para o usuario atual), confere
# os arquivos e a chave de desinstalacao, roda o programa instalado com a
# entrada do e2e e desinstala, conferindo que nada ficou para tras.
# Uso: make teste-setup  (ou: powershell -File tests\setup\test_setup.ps1)
# Mensagens sem acento: o PowerShell 5.1 le .ps1 sem BOM como ANSI.

$ErrorActionPreference = 'Stop'

$raiz    = Resolve-Path (Join-Path $PSScriptRoot '..\..')
$setup   = Join-Path $raiz 'dist\ohm_flow_setup.exe'
$entrada = Join-Path $raiz 'tests\e2e\entrada.txt'
$appId   = '{8F3C2A71-5D4B-4E9A-B6C1-2A7E9D0F4B53}'
$chave   = "HKCU:\Software\Microsoft\Windows\CurrentVersion\Uninstall\${appId}_is1"
$destino = Join-Path ([IO.Path]::GetTempPath()) ("ohmflow_teste_setup_" + [guid]::NewGuid().ToString('N'))
$saida   = Join-Path ([IO.Path]::GetTempPath()) ("ohmflow_teste_saida_" + [guid]::NewGuid().ToString('N') + '.txt')
$verificacoes = 0

function Falha([string]$msg) {
    Write-Host "SETUP FALHOU: $msg"
    exit 1
}

function Confere([bool]$cond, [string]$msg) {
    if (-not $cond) { Falha $msg }
    $script:verificacoes++
}

# 1. O instalador foi gerado
Confere (Test-Path $setup) "instalador nao encontrado em $setup (rode make setup)"

# 2. Instalacao silenciosa, sem tarefas opcionais (atalho no desktop, PATH)
$p = Start-Process -FilePath $setup -Wait -PassThru -ArgumentList @(
    '/VERYSILENT', '/SUPPRESSMSGBOXES', '/CURRENTUSER', '/NORESTART',
    "/DIR=`"$destino`"", '/TASKS=""')
Confere ($p.ExitCode -eq 0) "instalacao saiu com codigo $($p.ExitCode)"

# 3. Arquivos instalados
foreach ($arq in 'ohmflow.exe', 'README.md', 'LICENSE', 'unins000.exe') {
    Confere (Test-Path (Join-Path $destino $arq)) "arquivo ausente apos instalar: $arq"
}

# 4. O programa instalado roda o roteiro do e2e ate o fim. APPDATA aponta para
#    uma pasta temporaria: os logs reais do usuario nao sao tocados
$appdataOriginal = $env:APPDATA
$appdataTeste = Join-Path ([IO.Path]::GetTempPath()) ("ohmflow_teste_appdata_" + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory $appdataTeste | Out-Null
$env:APPDATA = $appdataTeste
$env:NO_COLOR = '1'
$p = Start-Process -FilePath (Join-Path $destino 'ohmflow.exe') -Wait -PassThru -NoNewWindow `
    -RedirectStandardInput $entrada -RedirectStandardOutput $saida
Remove-Item Env:\NO_COLOR
$env:APPDATA = $appdataOriginal
Confere ($p.ExitCode -eq 0) "ohmflow.exe instalado saiu com codigo $($p.ExitCode)"
$texto = Get-Content -Raw -Encoding UTF8 $saida
Confere ($texto -match 'Obrigado por usar o OhmFlow') "saida do programa instalado sem a tela de encerramento (veja $saida)"
Remove-Item $saida

# 5. O exe do instalador grava os logs em %APPDATA%\OhmFlow\logs
$logs = @(Get-ChildItem (Join-Path $appdataTeste 'OhmFlow\logs') -Filter 'ohmflow_*.log' -ErrorAction SilentlyContinue)
Confere ($logs.Count -eq 1) "log do dia ausente em $appdataTeste\OhmFlow\logs"
$log = Get-Content -Raw -Encoding UTF8 $logs[0].FullName
Confere ($log -match '\| INSERIR ') "log sem a acao INSERIR: $($logs[0].FullName)"
Confere ($log -match 'encerrada em') "log sem o rodape da sessao: $($logs[0].FullName)"
Confere (-not (Test-Path (Join-Path $destino 'logs'))) "exe instalado criou logs na pasta de instalacao"
Remove-Item -Recurse -Force $appdataTeste

# 6. Registro de desinstalacao (aparece em "Aplicativos instalados")
Confere (Test-Path $chave) "chave de desinstalacao ausente: $chave"
$nome = (Get-ItemProperty $chave).DisplayName
Confere ($nome -like 'OhmFlow*') "DisplayName inesperado: '$nome'"

# 7. Desinstalacao silenciosa remove pasta e chave
$p = Start-Process -FilePath (Join-Path $destino 'unins000.exe') -Wait -PassThru `
    -ArgumentList '/VERYSILENT', '/SUPPRESSMSGBOXES', '/NORESTART'
Confere ($p.ExitCode -eq 0) "desinstalacao saiu com codigo $($p.ExitCode)"
# O desinstalador se copia para %TEMP% e termina de apagar logo depois
for ($i = 0; $i -lt 20 -and (Test-Path $destino); $i++) { Start-Sleep -Milliseconds 250 }
Confere (-not (Test-Path $destino)) "pasta de instalacao ainda existe: $destino"
Confere (-not (Test-Path $chave)) "chave de desinstalacao ainda existe: $chave"

Write-Host "SETUP OK ($verificacoes verificacoes)"
