param(
    [string]$TestType = 'unit'
)

switch ($TestType) {
    'unit' {
        Write-Host "Running unit tests (idf.py test)..."
        idf.py test
        break
    }
    default {
        Write-Host "Unknown TestType: $TestType"
        exit 1
    }
}
