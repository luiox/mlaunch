param(
  [Parameter(Mandatory = $true)]
  [string]$ZipPath,

  [Parameter(Mandatory = $false)]
  [string]$TargetDir = "vendor\feather"
)

$ErrorActionPreference = 'Stop'

$root = Split-Path -Parent $PSScriptRoot
$zipPathAbs = [System.IO.Path]::GetFullPath((Join-Path $root $ZipPath))
$targetAbs = [System.IO.Path]::GetFullPath((Join-Path $root "..\$TargetDir"))

if (-not (Test-Path $zipPathAbs)) {
  throw "zip not found: $zipPathAbs"
}

if (Test-Path $targetAbs) {
  Remove-Item -Recurse -Force $targetAbs
}

New-Item -ItemType Directory -Force -Path $targetAbs | Out-Null
Expand-Archive -Path $zipPathAbs -DestinationPath $targetAbs -Force
Write-Host "Extracted Feather to: $targetAbs"
