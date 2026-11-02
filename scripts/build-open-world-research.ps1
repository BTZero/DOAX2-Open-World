[CmdletBinding()]
param(
    [ValidateSet('Baseline','Probe')][string]$Mode = 'Baseline',
    [ValidateRange(1,32)][int]$Jobs = 8
)
$ErrorActionPreference = 'Stop'
. "$PSScriptRoot/dev-shell.ps1"
$projectRoot = Split-Path $PSScriptRoot -Parent
$sdkPrefix = Join-Path $projectRoot 'sdk/rexglue-sdk/out/install/win-amd64'
$diagnostics = if ($Mode -eq 'Probe') { 'ON' } else { 'OFF' }
$buildDirectory = Join-Path $projectRoot ('out/open-world-' + $Mode.ToLowerInvariant())
Push-Location $projectRoot
try {
    # The shared preset also supplies a C compiler; this project enables only
    # C++. Avoid an unused-variable warning aborting redirected Windows PS logs.
    cmake --no-warn-unused-cli --preset win-amd64-release -B $buildDirectory "-DCMAKE_PREFIX_PATH=$sdkPrefix" "-DDOAX2_OPEN_WORLD_DIAGNOSTICS=$diagnostics"
    if ($LASTEXITCODE) { throw 'Research build configuration failed.' }
    # Do not build launcher targets: the developer launcher writes to the root.
    cmake --build $buildDirectory --target doax2 --parallel $Jobs
    if ($LASTEXITCODE) { throw 'Research game build failed.' }
} finally { Pop-Location }
