[CmdletBinding()]
param(
    [switch]$Clean,
    [ValidateSet('debug', 'release', 'relwithdebinfo')]
    [string]$Config = 'debug'
)

$ErrorActionPreference = 'Stop'

$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
if (-not (Test-Path $vswhere)) {
    Write-Error "vswhere.exe not found — Visual Studio Build Tools are not installed"
    exit 1
}

$vsPath = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $vsPath) {
    Write-Error "No Visual Studio installation with MSVC x64 tools found"
    exit 1
}

$configPreset = "windows-msvc-$Config"
$buildPreset  = $Config
$outDir       = Join-Path $PSScriptRoot "..\out\build\$configPreset"

if ($Clean -and (Test-Path $outDir)) {
    Write-Host "Cleaning $outDir ..." -ForegroundColor Yellow
    Remove-Item -Recurse -Force $outDir
}

# Strip GCC toolchains (Strawberry Perl, MSYS2, MinGW, Cygwin) from PATH before
# cmd.exe inherits it. VsDevCmd.bat then prepends MSVC tools to the clean PATH,
# so CMake sees only MSVC and never picks up a conflicting ld.exe or rc.exe.
$env:PATH = ($env:PATH -split ';' | Where-Object {
    $_ -notlike '*Strawberry*' -and
    $_ -notlike '*msys*'       -and
    $_ -notlike '*cygwin*'     -and
    $_ -notlike '*mingw*'
}) -join ';'

$vcvars = "$vsPath\Common7\Tools\VsDevCmd.bat"
$chain  = "`"$vcvars`" -arch=amd64 -no_logo && cmake --preset $configPreset && cmake --build --preset $buildPreset --parallel"

Write-Host "[build] Config: $Config" -ForegroundColor Cyan
cmd /c $chain
exit $LASTEXITCODE
