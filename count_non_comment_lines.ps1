# Define the target folder
$folder = "src"

# Initialize total lines counter and a list to store file results
$totalLines = 0
$fileResults = [System.Collections.Generic.List[object]]::new()

# Check if the folder exists
if (-not (Test-Path $folder)) {
    Write-Host "Folder '$folder' does not exist." -ForegroundColor Red
    exit 1
}

Write-Host "Processing files in '$folder'..."

# Get all files recursively
Get-ChildItem -Path $folder -File -Recurse | ForEach-Object {
    $file = $_
    $lineCount = 0

    try {
        # Read file content and count non-comment lines
        Get-Content $file.FullName -ErrorAction Stop | ForEach-Object {
            # Check if the line is not empty/whitespace and not a comment
            if ($_.Trim() -ne '' -and -not $_.TrimStart().StartsWith("//")) {
                $lineCount++
            }
        }

        # Add the result for this file to our list
        $fileResults.Add([PSCustomObject]@{
            Name      = $file.Name
            LineCount = $lineCount
            FullName  = $file.FullName # Keep FullName if needed later
        })

        # Add to the overall total
        $totalLines += $lineCount

    } catch {
        # Warn if a file couldn't be processed
        Write-Warning "Could not process file '$($file.FullName)': $_"
    }
}

# Sort the collected results by LineCount in descending order
$sortedResults = $fileResults | Sort-Object -Property LineCount -Descending

# --- Display the sorted results ---
Write-Host "`n--- File Non-Comment Line Counts (Descending) ---" -ForegroundColor Cyan

$sortedResults | ForEach-Object {
    Write-Host "File: $($_.Name) - Non-comment lines: $($_.LineCount)"
}

# --- Display the total ---
Write-Host "-------------------------------------------------" -ForegroundColor Cyan
Write-Host "Total non-comment lines: $totalLines" -ForegroundColor Green