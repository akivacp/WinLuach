param(
    [string]$ProjectDir = $PSScriptRoot
)

$ErrorActionPreference = 'Stop'

$projectPath = Resolve-Path -LiteralPath $ProjectDir
$buildPath   = Join-Path $projectPath 'version.build'
$headerPath  = Join-Path $projectPath 'Version.h'

# ============================================================
# VERSION BASELINE
# Change these manually when bumping major/minor versions
# ============================================================

$major = 0
$minor = 8

# ============================================================
# BUILD NUMBER
# Auto-increments every build
# ============================================================

$build = 0

if (Test-Path -LiteralPath $buildPath) {
    $raw = (Get-Content -LiteralPath $buildPath -Raw).Trim()

    if ($raw -match '^\d+$') {
        $build = [int]$raw
    }
}

$build++

Set-Content -LiteralPath $buildPath -Value $build -Encoding ASCII

# ============================================================
# COMPOSE VERSION
# ============================================================

$version = "$major.$minor.$build"

$content = @"
#pragma once

#define WINLUACH_VERSION_MAJOR $major
#define WINLUACH_VERSION_MINOR $minor
#define WINLUACH_VERSION_BUILD $build
#define WINLUACH_VERSION_TEXT L"$version"
"@

Set-Content -LiteralPath $headerPath -Value $content -Encoding ASCII