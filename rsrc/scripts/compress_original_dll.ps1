#!powershell
# Author: Sean Pesce
#
# Compresses a file with the Windows Compression API (XPRESS_HUFF) so that it
# can be embedded as an RCDATA resource and decompressed at runtime. Output
# format: 4-byte little-endian uncompressed size, followed by the compressed
# stream. XPRESS_HUFF was chosen so decompression works on Windows 7+.

param(
    [Parameter(Mandatory=$true)][string]$In,
    [Parameter(Mandatory=$true)][string]$Out
)

$ErrorActionPreference = 'Stop'

if (-not (Test-Path -LiteralPath $In)) {
    throw "Input file not found: $In"
}

if (Test-Path -LiteralPath $Out) {
    $inTime  = (Get-Item -LiteralPath $In).LastWriteTimeUtc
    $outTime = (Get-Item -LiteralPath $Out).LastWriteTimeUtc
    if ($outTime -ge $inTime) {
        Write-Host "compress_original_dll.ps1: up to date ($Out)"
        exit 0
    }
}

Add-Type -TypeDefinition @"
using System;
using System.Runtime.InteropServices;
public static class Cabinet {
    public const uint COMPRESS_ALGORITHM_XPRESS_HUFF = 4;
    [DllImport("cabinet.dll", SetLastError=true)]
    public static extern bool CreateCompressor(uint Algorithm, IntPtr AllocationRoutines, out IntPtr Handle);
    [DllImport("cabinet.dll", SetLastError=true)]
    public static extern bool Compress(IntPtr Handle, byte[] UncompressedData, IntPtr UncompressedDataSize, byte[] CompressedBuffer, IntPtr CompressedBufferSize, out IntPtr CompressedDataSize);
    [DllImport("cabinet.dll", SetLastError=true)]
    public static extern bool CloseCompressor(IntPtr Handle);
}
"@ | Out-Null

$data = [System.IO.File]::ReadAllBytes($In)

$handle = [IntPtr]::Zero
if (-not [Cabinet]::CreateCompressor([Cabinet]::COMPRESS_ALGORITHM_XPRESS_HUFF, [IntPtr]::Zero, [ref]$handle)) {
    $err = [System.Runtime.InteropServices.Marshal]::GetLastWin32Error()
    throw "CreateCompressor failed: $err"
}

try {
    # Query required buffer size.
    $needed = [IntPtr]::Zero
    $ok = [Cabinet]::Compress($handle, $data, [IntPtr]$data.Length, $null, [IntPtr]::Zero, [ref]$needed)
    $err = [System.Runtime.InteropServices.Marshal]::GetLastWin32Error()
    # ERROR_INSUFFICIENT_BUFFER (122) is expected on the size-query call.
    if (-not $ok -and $err -ne 122) {
        throw "Compress (size query) failed: $err"
    }

    $bufSize = [int]$needed
    $buf = New-Object byte[] $bufSize
    $actual = [IntPtr]::Zero
    if (-not [Cabinet]::Compress($handle, $data, [IntPtr]$data.Length, $buf, [IntPtr]$bufSize, [ref]$actual)) {
        $err = [System.Runtime.InteropServices.Marshal]::GetLastWin32Error()
        throw "Compress failed: $err"
    }
    $actualSize = [int]$actual

    $header = [BitConverter]::GetBytes([uint32]$data.Length)
    $outDir = Split-Path -Parent $Out
    if ($outDir -and -not (Test-Path -LiteralPath $outDir)) {
        New-Item -ItemType Directory -Force -Path $outDir | Out-Null
    }
    $fs = [System.IO.File]::Open($Out, [System.IO.FileMode]::Create, [System.IO.FileAccess]::Write)
    try {
        $fs.Write($header, 0, 4)
        $fs.Write($buf, 0, $actualSize)
    } finally {
        $fs.Close()
    }
    $totalOut = $actualSize + 4
    $ratio = $totalOut / $data.Length
    Write-Host ("compress_original_dll.ps1: {0} bytes -> {1} bytes ({2:P1}) -> {3}" -f $data.Length, $totalOut, $ratio, $Out)
} finally {
    [void][Cabinet]::CloseCompressor($handle)
}
