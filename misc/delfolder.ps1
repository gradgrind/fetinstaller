function DelFolder {
    param(
        [string]$Path
    )

    $subfolders = Get-ChildItem -Path $Path -Directory -Force -ErrorAction SilentlyContinue

    foreach ($folder in $subfolders) {
        DelFolder -Path $folder.FullName
    }

    $files = Get-ChildItem -Path $Path -File -Force -ErrorAction SilentlyContinue

    foreach ($file in $files) {
        Remove-Item -Path $file.FullName -Force -ErrorAction SilentlyContinue
    }
    
    Remove-Item -Path $Path -Force -ErrorAction SilentlyContinue
}

# Delete the folder containing this script!
DelFolder -Path "$PSScriptRoot"

