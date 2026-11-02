# Read-only image snapshot for local source/address research.
# Output is copyrighted game-derived data and stays in ignored out/.
[CmdletBinding()]
param([Parameter(Mandatory)][int]$ProcessId, [switch]$TerrainGeometry, [switch]$MotionBanks,
      [ValidateRange(0,3)][int]$MotionBankSlot = 0)
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path $PSScriptRoot -Parent
$outputRoot = [IO.Path]::GetFullPath((Join-Path $projectRoot 'out')) + [IO.Path]::DirectorySeparatorChar
$gameProcess = Get-Process -Id $ProcessId -ErrorAction Stop
if ($gameProcess.ProcessName -ne 'doax2' -or
    -not $gameProcess.Path.StartsWith($outputRoot,[StringComparison]::OrdinalIgnoreCase)) {
    throw 'Only the workspace research game process can be read.'
}
$sessionPath = Join-Path $projectRoot 'out/open-world-research/session.json'
if (-not (Test-Path -LiteralPath $sessionPath)) { throw 'Launch with run-open-world-research.ps1 first.' }
$session = Get-Content -LiteralPath $sessionPath -Raw | ConvertFrom-Json
if ($session.ProcessId -ne $ProcessId -or $session.Executable -ne $gameProcess.Path -or
    $session.XexSha256 -ne 'C4A14853E1F7AF951BE34F380604113703638780B36D5E74E674FE7431992870' -or
    $session.StartedUtc -ne $gameProcess.StartTime.ToUniversalTime().ToString('o')) {
    throw 'Research session metadata does not match this process and supported input.'
}
$arenaLine = Get-Content -LiteralPath $session.Log | Select-String 'Guest memory arena mapped: virtual base 0x([0-9a-fA-F]+)' | Select-Object -First 1
if (-not $arenaLine) { throw 'Guest arena is not recorded in the runtime log yet.' }
$arena = [Convert]::ToInt64($arenaLine.Matches[0].Groups[1].Value,16)
$header = Get-Content -LiteralPath (Join-Path $projectRoot 'generated/default/doax2_pch.h') -Raw
$baseMatch = [regex]::Match($header, '#define REX_IMAGE_BASE 0x([0-9a-fA-F]+)')
$sizeMatch = [regex]::Match($header, '#define REX_IMAGE_SIZE 0x([0-9a-fA-F]+)')
if (-not $baseMatch.Success -or -not $sizeMatch.Success) { throw 'Generated image bounds are unavailable.' }
$imageBase = [Convert]::ToInt64($baseMatch.Groups[1].Value,16)
$imageSize = [Convert]::ToInt32($sizeMatch.Groups[1].Value,16)
if ($imageSize -le 0 -or $imageSize -gt 0x8000000) { throw 'Unexpected image size; refusing snapshot.' }
Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
public static class ResearchImageReader {
    [DllImport("kernel32.dll", SetLastError=true)] public static extern IntPtr OpenProcess(uint access, bool inherit, int pid);
    [DllImport("kernel32.dll", SetLastError=true)] public static extern bool ReadProcessMemory(IntPtr p, IntPtr address, byte[] data, UIntPtr size, out UIntPtr read);
    [DllImport("kernel32.dll")] public static extern bool CloseHandle(IntPtr handle);
}
'@
# Derive the arena from this run's log and bounds from generated image metadata.
$handle = [ResearchImageReader]::OpenProcess(0x410, $false, $ProcessId)
if ($handle -eq [IntPtr]::Zero) { throw 'Cannot open research process for read-only inspection.' }
try {
    $imageBytes = New-Object byte[] $imageSize
    $bytesRead = [UIntPtr]::Zero
    $readSize = [UIntPtr]::new([uint64]$imageBytes.Length)
    if (-not [ResearchImageReader]::ReadProcessMemory($handle,[IntPtr]($arena + $imageBase),$imageBytes,$readSize,[ref]$bytesRead) -or
        $bytesRead.ToUInt64() -ne $imageBytes.Length) {
        throw 'Image snapshot failed; verify the arena base and image bounds in this build.'
    }
    $directory = Join-Path $projectRoot 'out/open-world-research'
    New-Item -ItemType Directory -Force -Path $directory | Out-Null
    $path = Join-Path $directory ('image-{0}-{1}.bin' -f $ProcessId,[DateTime]::UtcNow.ToString('yyyyMMdd-HHmmss-fff'))
    [IO.File]::WriteAllBytes($path, $imageBytes)
    $path
    if ($MotionBanks) {
        function Read-MotionBytes([uint64]$address, [int]$count) {
            if (-not $address -or $count -lt 1 -or $count -gt 512 -or
                $address + $count -gt 0x100000000L) { return $null }
            $payload = New-Object byte[] $count
            $payloadRead = [UIntPtr]::Zero
            $aliasOffset = if ($address -ge 0xE0000000L) { 0x1000 } else { 0 }
            if (-not [ResearchImageReader]::ReadProcessMemory($handle,
                [IntPtr]($arena + [int64]$address + $aliasOffset), $payload,
                [UIntPtr]::new([uint64]$count), [ref]$payloadRead) -or
                $payloadRead.ToUInt64() -ne $count) { return $null }
            return ,$payload
        }
        function Read-MotionWord([uint64]$address) {
            $payload = Read-MotionBytes $address 4
            if ($null -eq $payload) { return $null }
            [Array]::Reverse($payload)
            return [BitConverter]::ToUInt32($payload, 0)
        }
        $player = 0x833B8520L
        $playerBytes = Read-MotionBytes $player 152
        $phase = Read-MotionWord 0x83DB9C54L
        $record = Read-MotionWord 0x83DB99CCL
        if ($null -eq $playerBytes -or $phase -ne 7 -or $record -ne 0x83DA61BCL -or
            (Read-MotionWord $record) -ne 58 -or $playerBytes[4] -ge 64) {
            throw 'Motion-bank capture requires the active reference pictorial scene 58.'
        }
        $bankRoot = 0x833B87A8L
        $inspectBankIndex = if ($PSBoundParameters.ContainsKey('MotionBankSlot')) { $MotionBankSlot } else { $playerBytes[4] }
        $banks = foreach ($bank in @(
            @{Name='character'; ScriptSlot=($bankRoot + 16 + $inspectBankIndex*64);
              ClipSlot=($bankRoot + 20 + $inspectBankIndex*64); IDs=@(27,270,271,272,273,274,275,541,543)},
            @{Name='shared'; ScriptSlot=($bankRoot + 4); ClipSlot=($bankRoot + 8); IDs=@(0,1)}
        )) {
            $wrapper = Read-MotionWord $bank.ScriptSlot
            $scripts = if ($wrapper) { Read-MotionWord $wrapper } else { $null }
            $clips = Read-MotionWord $bank.ClipSlot
            $entries = foreach ($motionId in $bank.IDs) {
                $commandPointer = if ($scripts) { Read-MotionWord ($scripts + $motionId*4) } else { $null }
                $clipPointer = if ($clips) { Read-MotionWord ($clips + $motionId*4) } else { $null }
                $commands = if ($commandPointer) { Read-MotionBytes $commandPointer 128 } else { $null }
                $header = if ($clipPointer) { Read-MotionBytes $clipPointer 32 } else { $null }
                [pscustomobject]@{
                    ID=$motionId; CommandPointer=$commandPointer; ClipPointer=$clipPointer
                    CommandHalfwords=if ($commands) {
                        @(for ($i=0; $i -lt $commands.Length; $i+=2) {
                            [int]$commands[$i]*256 + $commands[$i+1]
                        })
                    } else { @() }
                    ClipHeaderHex=if ($header) { [BitConverter]::ToString($header) } else { $null }
                }
            }
            [pscustomobject]@{Name=$bank.Name; Wrapper=$wrapper; ScriptTable=$scripts;
                ClipTable=$clips; Entries=@($entries)}
        }
        $motionPath = [IO.Path]::ChangeExtension($path, '.motions.json')
        [pscustomobject]@{ProcessId=$ProcessId; Phase=$phase; Selected=$record;
            CharacterByte1=$playerBytes[1]; BankIndex=$playerBytes[4]; InspectedBank=$inspectBankIndex; Banks=@($banks)
        } | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $motionPath -Encoding UTF8
        $motionPath
    }
    if ($TerrainGeometry) {
        # Match the generated Windows guest accessor, including its physical
        # alias guard-page adjustment. Reading a high guest address directly
        # would return bytes 4096 bytes before the actual resource.
        if (-not $header.Contains('#define REX_PHYS_HOST_OFFSET(addr) (((u32)(addr) >= 0xE0000000u) ? 0x1000u : 0u)')) {
            throw 'Unsupported generated physical alias mapping; refusing terrain capture.'
        }
        # Layout verified from 825BAD40 and the triangle iterator 825BB530:
        # +20 vertices (16-byte records), +24 triangles (8-byte records),
        # +40 vertex count, +44 triangle count. Read only this active object.
        function Read-ImageWord([int64]$address) {
            $offset = [int]($address - $imageBase)
            if ($offset -lt 0 -or $offset + 4 -gt $imageBytes.Length) { throw 'Metadata outside image.' }
            $word = [byte[]]$imageBytes[$offset..($offset + 3)]
            [Array]::Reverse($word)
            [BitConverter]::ToUInt32($word, 0)
        }
        $terrainObject = 0x82E2DB50L
        $vertexPointer = Read-ImageWord ($terrainObject + 20)
        $trianglePointer = Read-ImageWord ($terrainObject + 24)
        $vertexCount = Read-ImageWord ($terrainObject + 40)
        $triangleCount = Read-ImageWord ($terrainObject + 44)
        if ($vertexCount -lt 3 -or $vertexCount -gt 65536 -or
            $triangleCount -lt 1 -or $triangleCount -gt 131072) {
            throw 'No supported active terrain geometry; image snapshot retained.'
        }
        $terrainStem = Join-Path $directory ('terrain-{0}-{1}' -f $ProcessId,[DateTime]::UtcNow.ToString('yyyyMMdd-HHmmss-fff'))
        foreach ($part in @(
            @{Name='vertices'; Pointer=$vertexPointer; Count=$vertexCount; Stride=16},
            @{Name='triangles'; Pointer=$trianglePointer; Count=$triangleCount; Stride=8}
        )) {
            $partSize = [int]($part.Count * $part.Stride)
            if (-not $part.Pointer -or [uint64]$part.Pointer + $partSize -gt 0x100000000L) {
                throw 'Invalid terrain range; refusing read.'
            }
            $partBytes = New-Object byte[] $partSize
            $partRead = [UIntPtr]::Zero
            $aliasOffset = if ([uint64]$part.Pointer -ge 0xE0000000L) { 0x1000 } else { 0 }
            if (-not [ResearchImageReader]::ReadProcessMemory($handle,[IntPtr]($arena + $part.Pointer + $aliasOffset),
                $partBytes,[UIntPtr]::new([uint64]$partSize),[ref]$partRead) -or $partRead.ToUInt64() -ne $partSize) {
                throw 'Terrain read failed; no guest data was modified.'
            }
            [IO.File]::WriteAllBytes(($terrainStem + '-' + $part.Name + '.bin'), $partBytes)
        }
        [pscustomobject]@{
            SchemaVersion=2; ProcessId=$ProcessId; ImageSnapshot=(Split-Path $path -Leaf)
            VertexPointer=$vertexPointer; VertexCount=$vertexCount; VertexStride=16
            TrianglePointer=$trianglePointer; TriangleCount=$triangleCount; TriangleStride=8
            ScreenId=$imageBytes[[int](0x834BCFE0L - $imageBase)]
            ScreenTable=(Read-ImageWord (0x834BCFE0L + 68))
            ActivityMode=(Read-ImageWord 0x833B7454L)
        } | ConvertTo-Json | Set-Content -LiteralPath ($terrainStem + '.json') -Encoding UTF8
        $terrainStem + '.json'
    }
} finally { [ResearchImageReader]::CloseHandle($handle) | Out-Null }
