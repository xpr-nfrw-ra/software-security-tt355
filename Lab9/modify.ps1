# Patches hello.exe: replaces "Hello, World!" with "Hacked, World!"
# Both strings are 13 bytes, so the binary layout is preserved.

$src = "hello.exe"
$dst = "hello_modified.exe"
$needle = "Hello, World!"
$replacement = "Hacker World!"

if ($needle.Length -ne $replacement.Length) {
    throw "Replacement length must match original ($($needle.Length) bytes)."
}

$bytes = [System.IO.File]::ReadAllBytes($src)
$text  = [System.Text.Encoding]::ASCII.GetString($bytes)
$offset = $text.IndexOf($needle)

if ($offset -lt 0) { throw "String '$needle' not found in $src." }

Write-Host "Found '$needle' at offset $offset"

$newBytes = [System.Text.Encoding]::ASCII.GetBytes($replacement)
for ($i = 0; $i -lt $newBytes.Length; $i++) {
    $bytes[$offset + $i] = $newBytes[$i]
}

[System.IO.File]::WriteAllBytes($dst, $bytes)
Write-Host "Wrote $dst ($($bytes.Length) bytes)"
