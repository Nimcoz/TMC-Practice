# Full-file hashes, using one buffer to avoid costly tiny sandbox stream reads.
function Get-TestFileHash {
    param([string[]]$LiteralPath,[ValidateSet('SHA1','SHA256')][string]$Algorithm='SHA256')
    foreach ($entry in $LiteralPath) {
        $resolved = (Resolve-Path -LiteralPath $entry -ErrorAction Stop).Path
        $hasher = [Security.Cryptography.HashAlgorithm]::Create($Algorithm)
        try {
            $digest = [BitConverter]::ToString($hasher.ComputeHash([IO.File]::ReadAllBytes($resolved))).Replace('-','')
            [pscustomobject]@{Algorithm=$Algorithm;Hash=$digest;Path=$resolved}
        } finally { $hasher.Dispose() }
    }
}
