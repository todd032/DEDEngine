Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$cacheRoot = Join-Path $repoRoot ".external-cache"
$emsdkRoot = Join-Path $cacheRoot "emsdk"
$emsdkBat = Join-Path $emsdkRoot "emsdk.bat"
$emsdkVersion = if ($env:EMSDK_VERSION) { $env:EMSDK_VERSION } else { "latest" }

function Add-ToPath([string]$Directory)
{
    if ([string]::IsNullOrWhiteSpace($Directory) -or -not (Test-Path $Directory)) {
        return
    }

    $parts = $env:PATH -split ";"
    if ($parts -contains $Directory) {
        return
    }

    $env:PATH = "$Directory;$env:PATH"
}

function Invoke-NativeCommand
{
    param(
        [Parameter(Mandatory = $true)]
        [string]$FilePath,

        [Parameter()]
        [string[]]$Arguments = @()
    )

    & $FilePath @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "Command failed with exit code ${LASTEXITCODE}: $FilePath $($Arguments -join ' ')"
    }
}

function Resolve-CMakeExecutable
{
    $local = Get-ChildItem -Path (Join-Path $cacheRoot "cmake") -Filter cmake.exe -Recurse -ErrorAction SilentlyContinue |
        Select-Object -First 1 -ExpandProperty FullName
    if ($local) {
        return $local
    }

    $fromPath = Get-Command cmake -ErrorAction SilentlyContinue
    if ($fromPath) {
        return $fromPath.Source
    }

    $visualStudioRoots = @(
        "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin",
        "C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin",
        "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin",
        "C:\Program Files\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin"
    )

    foreach ($candidate in $visualStudioRoots) {
        $cmake = Join-Path $candidate "cmake.exe"
        if (Test-Path $cmake) {
            return $cmake
        }
    }

    return $null
}

function Resolve-NinjaDirectory
{
    $pathNinja = Get-Command ninja -ErrorAction SilentlyContinue
    if ($pathNinja) {
        return Split-Path -Parent $pathNinja.Source
    }

    $local = Get-ChildItem -Path (Join-Path $cacheRoot "ninja") -Filter ninja.exe -Recurse -ErrorAction SilentlyContinue |
        Select-Object -First 1 -ExpandProperty FullName
    if ($local) {
        return Split-Path -Parent $local
    }

    $visualStudioRoots = @(
        "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja",
        "C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja",
        "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja",
        "C:\Program Files\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja"
    )

    foreach ($candidate in $visualStudioRoots) {
        if (Test-Path (Join-Path $candidate "ninja.exe")) {
            return $candidate
        }
    }

    return $null
}

function Resolve-EmsdkPython
{
    $python = Get-ChildItem -Path (Join-Path $emsdkRoot "python") -Filter python.exe -Recurse -ErrorAction SilentlyContinue |
        Sort-Object FullName -Descending |
        Select-Object -First 1 -ExpandProperty FullName
    return $python
}

function Resolve-EmsdkNode
{
    $node = Get-ChildItem -Path (Join-Path $emsdkRoot "node") -Filter node.exe -Recurse -ErrorAction SilentlyContinue |
        Sort-Object FullName -Descending |
        Select-Object -First 1 -ExpandProperty FullName
    return $node
}

if (-not (Test-Path $cacheRoot)) {
    New-Item -ItemType Directory -Path $cacheRoot | Out-Null
}

if (-not (Test-Path $emsdkRoot)) {
    $git = Get-Command git -ErrorAction SilentlyContinue
    if (-not $git) {
        throw "git is required to clone emsdk into .external-cache\emsdk."
    }

    Invoke-NativeCommand -FilePath $git.Source -Arguments @("clone", "https://github.com/emscripten-core/emsdk.git", $emsdkRoot)
}

if (-not (Test-Path $emsdkBat)) {
    throw "Expected emsdk.bat under $emsdkRoot."
}

$cmakeExe = Resolve-CMakeExecutable
if (-not $cmakeExe) {
    throw "cmake was not found. Install it into PATH, restore a repo-local cache under .external-cache\cmake, or use a Visual Studio 2022 install that includes CMake."
}

$ninjaDir = Resolve-NinjaDirectory
if (-not $ninjaDir) {
    throw "ninja was not found. Install ninja into PATH, restore .external-cache\ninja, or use the Visual Studio CMake ninja toolset."
}

Add-ToPath (Split-Path -Parent $cmakeExe)
Add-ToPath $ninjaDir

Invoke-NativeCommand -FilePath $emsdkBat -Arguments @("install", $emsdkVersion)
Invoke-NativeCommand -FilePath $emsdkBat -Arguments @("activate", $emsdkVersion)

$emsdkPython = Resolve-EmsdkPython
$emsdkNode = Resolve-EmsdkNode
if (-not $emsdkPython -or -not $emsdkNode) {
    throw "emsdk activation completed, but the bundled Python or Node runtime was not found."
}

$env:EMSDK = $emsdkRoot.Replace("\", "/")
$env:EMSDK_PYTHON = $emsdkPython
$env:EMSDK_NODE = $emsdkNode

Add-ToPath $emsdkRoot
Add-ToPath (Join-Path $emsdkRoot "upstream\emscripten")
Add-ToPath (Split-Path -Parent $emsdkNode)
Add-ToPath (Split-Path -Parent $emsdkPython)

Push-Location $repoRoot
try {
    Invoke-NativeCommand -FilePath $cmakeExe -Arguments @("--preset", "web-emscripten")
    Invoke-NativeCommand -FilePath $cmakeExe -Arguments @("--build", "--preset", "web-release")
}
finally {
    Pop-Location
}

Write-Host "Web build ready at $repoRoot\build\web\site\index.html"
