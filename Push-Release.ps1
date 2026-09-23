# ==============================================================================
# Push-Release.ps1
# Commits pending changes, pushes to GitHub, tags, and creates a release.
# Does NOT build — use Build-Release.ps1 for a full build+publish cycle.
# Run from the WinLuach project root.
# ==============================================================================

param(
    [switch]$Publish
)

$ErrorActionPreference = 'Stop'

# --- 1. Show that no build is performed ---
Write-Host "Push-Release: skipping build (use Build-Release.ps1 to build)." -ForegroundColor Cyan

# --- 2. Read version from Version.h ---
$versionHeader = Join-Path $PSScriptRoot "WinLuach\Version.h"
$major = (Select-String '#define WINLUACH_VERSION_MAJOR\s+(\d+)' $versionHeader).Matches[0].Groups[1].Value
$minor = (Select-String '#define WINLUACH_VERSION_MINOR\s+(\d+)' $versionHeader).Matches[0].Groups[1].Value
$build = (Select-String '#define WINLUACH_VERSION_BUILD\s+(\d+)' $versionHeader).Matches[0].Groups[1].Value
$version = "$major.$minor.$build"
$tag     = "v$version"
$dateMatch = Select-String '#define WINLUACH_BUILD_DATE_TEXT\s+L"([^"]+)"' $versionHeader
$buildDate = if ($dateMatch) { $dateMatch.Matches[0].Groups[1].Value } else { "unknown" }
$releaseNotes = "Release $version`nBuilt $buildDate"
Write-Host "Version: $tag (built $buildDate)" -ForegroundColor Cyan

# --- 3. Find the existing release binary ---
$exeOut = Join-Path $PSScriptRoot "x64\Release\WinLuach.exe"
if (-not (Test-Path $exeOut)) {
    Write-Warning "Release binary not found at: $exeOut"
    Write-Host "The GitHub release will be created without an asset." -ForegroundColor Yellow
    $exeOut = $null
}

# --- 4. Ask whether to publish ---
Write-Host ""
if (-not $Publish) {
    $answer = Read-Host "Publish $tag to GitHub Releases? [Y/N]"
    $Publish = $answer -match '^[Yy]'
}
if (-not $Publish) {
    Write-Host "Skipped GitHub publish." -ForegroundColor Cyan
    Write-Host "`nPress any key to exit..."
    $null = $Host.UI.RawUI.ReadKey("NoEcho,IncludeKeyDown")
    return
}

# --- 5. Require gh CLI ---
if (-not (Get-Command gh -ErrorAction SilentlyContinue)) {
    Write-Error "gh CLI not found. Install it with: winget install GitHub.cli — then run: gh auth login"
}

# --- 6. Commit and push any pending source changes ---
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

# --- 7. Create and push git tag ---
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

# --- 8. Create GitHub release and upload asset ---
if ($exeOut) {
    Write-Host "Creating GitHub release $tag and uploading WinLuach.exe ..." -ForegroundColor Yellow
    gh release create $tag $exeOut `
        --title "WinLuach $version" `
        --notes $releaseNotes
} else {
    Write-Host "Creating GitHub release $tag (no asset) ..." -ForegroundColor Yellow
    gh release create $tag `
        --title "WinLuach $version" `
        --notes $releaseNotes
}

if ($LASTEXITCODE -ne 0) { Write-Error "gh release create failed." }

Write-Host "Published: https://github.com/akivacp/WinLuach/releases/tag/$tag" -ForegroundColor Green

Write-Host "`nPress any key to exit..."
$null = $Host.UI.RawUI.ReadKey("NoEcho,IncludeKeyDown")
