$folder = "src"
$totalLines = 0

if (-not (Test-Path $folder)) {
    Write-Host "Folder '$folder' does not exist." -ForegroundColor Red
    exit 1
}

Get-ChildItem -Path $folder -File -Recurse | ForEach-Object {
    $file = $_.FullName
    $lineCount = 0
    Get-Content $file | ForEach-Object {
        if (-not $_.TrimStart().StartsWith("//")) {
            $lineCount++
        }
    }
    $totalLines += $lineCount
    Write-Host "File: $($_.Name) - Non-comment lines: $lineCount"
}

Write-Host "Total non-comment lines: $totalLines" -ForegroundColor Green