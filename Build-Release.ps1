# ==============================================================================
# Build-Release.ps1
# Closes any running WinLuach instance, then compiles the Release x64 build.
# Run from the WinLuach project root (right-click → Run with PowerShell).
# ==============================================================================

param(
    [switch]$Publish
)

$ErrorActionPreference = 'Stop'

$slnPath = Join-Path $PSScriptRoot "WinLuach.slnx"

# --- 1. Close any running WinLuach instance ---
$proc = Get-Process -Name "WinLuach" -ErrorAction SilentlyContinue
if ($proc) {
    Write-Host "Closing WinLuach.exe..." -ForegroundColor Yellow
    $proc | Stop-Process -Force
    Start-Sleep -Seconds 2
    Write-Host "Done." -ForegroundColor Green
} else {
    Write-Host "WinLuach is not running." -ForegroundColor Cyan
}

# --- 2. Locate MSBuild via vswhere ---
$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
if (-not (Test-Path $vswhere)) {
    $vswhere = "${env:ProgramFiles}\Microsoft Visual Studio\Installer\vswhere.exe"
}
if (-not (Test-Path $vswhere)) {
    Write-Error "vswhere.exe not found. Is Visual Studio installed?"
}

$msbuild = & $vswhere -latest -requires Microsoft.Component.MSBuild `
    -find "MSBuild\**\Bin\MSBuild.exe" | Select-Object -First 1

if (-not $msbuild -or -not (Test-Path $msbuild)) {
    Write-Error "MSBuild.exe not found via vswhere."
}

Write-Host "Using MSBuild: $msbuild" -ForegroundColor Cyan

# --- 3. Clean stale obj/pch files, then build Release x64 ---
# /t:Rebuild forces a full clean+build so stale .obj files from prior
# failed builds never cause phantom compile errors.
Write-Host "`nBuilding Release|x64 (full rebuild) ..." -ForegroundColor Yellow

& $msbuild $slnPath `
    /t:Rebuild `
    /p:Configuration=Release `
    /p:Platform=x64 `
    /m `
    /nologo `
    /verbosity:minimal

if ($LASTEXITCODE -ne 0) {
    Write-Error "Build FAILED (exit code $LASTEXITCODE)."
}

Write-Host "`nBuild succeeded!" -ForegroundColor Green

# --- 4. Show output location ---
$exeOut = Join-Path $PSScriptRoot "x64\Release\WinLuach.exe"
if (Test-Path $exeOut) {
    Write-Host "Output: $exeOut" -ForegroundColor Green
} else {
    Write-Host "Note: could not find output at expected path: $exeOut" -ForegroundColor Yellow
}

# --- 5. Read version from Version.h ---
$versionHeader = Join-Path $PSScriptRoot "WinLuach\Version.h"
$major = (Select-String '#define WINLUACH_VERSION_MAJOR\s+(\d+)' $versionHeader).Matches[0].Groups[1].Value
$minor = (Select-String '#define WINLUACH_VERSION_MINOR\s+(\d+)' $versionHeader).Matches[0].Groups[1].Value
$build = (Select-String '#define WINLUACH_VERSION_BUILD\s+(\d+)' $versionHeader).Matches[0].Groups[1].Value
$version = "$major.$minor.$build"
$tag     = "v$version"
Write-Host "Version: $tag" -ForegroundColor Cyan

# --- 6. Ask whether to publish ---
Write-Host ""
if (-not $Publish) {
    $answer = Read-Host "Publish $tag to GitHub Releases? [Y/N]"
    $Publish = $answer -match '^[Yy]'
}
if (-not $Publish) {
    Write-Host "Skipped GitHub publish." -ForegroundColor Cyan
} else {

    # --- 7. Require gh CLI ---
    if (-not (Get-Command gh -ErrorAction SilentlyContinue)) {
        Write-Error "gh CLI not found. Install it with: winget install GitHub.cli — then run: gh auth login"
    }

    # --- 8. Commit and push any pending source changes ---
    $pendingChanges = git status --porcelain
    if ($pendingChanges) {
        Write-Host "Committing pending source changes ..." -ForegroundColor Yellow
        git add -A

        # Guard: never publish personal data, local tooling, or nested repos
        $newFiles = @(git diff --cached --name-only --diff-filter=A)
        $nested   = @(git diff --cached --raw | Where-Object { $_ -match '^:\S+ 160000 ' } | ForEach-Object { ($_ -split "`t")[-1] })
        $blocked  = @($newFiles | Where-Object {
            $_ -match '(^|/)(settings|events|locations)\.json$' -or
            $_ -match '(^|/)WinLuach_.*backup.*\.json$' -or
            $_ -match '(^|/)\.claude/' -or $_ -match '^temp-clone/' -or $_ -match '\.log$'
        }) + $nested
        if ($blocked.Count -gt 0) {
            git reset -q
            Write-Error "Refusing to publish private/local files (nothing was committed):`n  $($blocked -join "`n  ")"
        }
        if ($newFiles.Count -gt 0) {
            Write-Host "These NEW files will be published to GitHub:" -ForegroundColor Yellow
            $newFiles | ForEach-Object { Write-Host "  $_" }
            $ok = Read-Host "Publish these new files? [Y/N]"
            if ($ok -notmatch '^[Yy]') {
                git reset -q
                Write-Error "Publish cancelled. Nothing was committed."
            }
        }

        git commit -m "Release $version"
        if ($LASTEXITCODE -ne 0) { Write-Error "git commit failed." }
    }
    Write-Host "Pushing source to origin ..." -ForegroundColor Yellow
    git push origin
    if ($LASTEXITCODE -ne 0) { Write-Error "git push failed." }

    # --- 10. Create and push git tag ---
    $existingTag = git tag -l $tag
    if ($existingTag) {
        Write-Host "Tag $tag already exists locally — skipping tag creation." -ForegroundColor Yellow
    } else {
        git tag $tag
        if ($LASTEXITCODE -ne 0) { Write-Error "Failed to create git tag $tag." }
    }

    Write-Host "Pushing tag $tag ..." -ForegroundColor Yellow
    git push origin $tag
    if ($LASTEXITCODE -ne 0) { Write-Error "Failed to push tag $tag to origin." }

    # --- 11. Create GitHub release and upload asset ---
    Write-Host "Creating GitHub release $tag and uploading WinLuach.exe ..." -ForegroundColor Yellow
    gh release create $tag $exeOut `
        --title "WinLuach $version" `
        --notes "Release $version"

    if ($LASTEXITCODE -ne 0) { Write-Error "gh release create failed." }

    Write-Host "Published: https://github.com/akivacp/WinLuach/releases/tag/$tag" -ForegroundColor Green
}

Write-Host "`nPress any key to exit..."
$null = $Host.UI.RawUI.ReadKey("NoEcho,IncludeKeyDown")
