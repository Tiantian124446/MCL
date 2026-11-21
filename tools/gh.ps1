param(
  [Parameter(Mandatory=$true)][string]$Action,
  [string]$Name = 'MCL',
  [string]$Message = 'MCL launcher and deployment server',
  [string]$Tag = 'v1.1.0',
  [string]$Asset = 'E:\MCL\build\mcl_test.exe',
  [string]$NotesFile = ''
)
$ErrorActionPreference = 'Stop'
$Target = 'GitHub - https://api.github.com/Tiantian124446'
$User = 'Tiantian124446'
$sig = @'
using System;
using System.Runtime.InteropServices;
public class CredReadHelper {
 [StructLayout(LayoutKind.Sequential, CharSet=CharSet.Unicode)] public struct CREDENTIAL { public uint Flags; public uint Type; public string TargetName; public string Comment; public System.Runtime.InteropServices.ComTypes.FILETIME LastWritten; public uint CredentialBlobSize; public IntPtr CredentialBlob; public uint Persist; public uint AttributeCount; public IntPtr Attributes; public string TargetAlias; public string UserName; }
 [DllImport("advapi32.dll", SetLastError=true, CharSet=CharSet.Unicode)] public static extern bool CredRead(string target, uint type, uint reservedFlag, out IntPtr credentialPtr);
 [DllImport("advapi32.dll", SetLastError=true)] public static extern void CredFree(IntPtr cred);
}
'@
if (-not ('CredReadHelper' -as [type])) { Add-Type $sig }
$ptr = [IntPtr]::Zero
if (-not [CredReadHelper]::CredRead($Target, 1, 0, [ref]$ptr)) { throw 'GitHub token not found in Credential Manager' }
$c = [System.Runtime.InteropServices.Marshal]::PtrToStructure($ptr, [type][CredReadHelper+CREDENTIAL])
$bytes = New-Object byte[] $c.CredentialBlobSize
if ($c.CredentialBlobSize -gt 0) { [System.Runtime.InteropServices.Marshal]::Copy($c.CredentialBlob, $bytes, 0, $c.CredentialBlobSize) }
$token = [System.Text.Encoding]::UTF8.GetString($bytes)
[CredReadHelper]::CredFree($ptr)
if ([string]::IsNullOrWhiteSpace($token)) { throw 'empty token' }
$headers = @{ Authorization = "token $token"; 'User-Agent' = 'DSH-MCL'; 'Accept' = 'application/vnd.github+json' }
$api = 'https://api.github.com'

function Invoke-GH([string]$Method, [string]$Url, $Body = $null) {
  $params = @{ Method = $Method; Uri = $Url; Headers = $headers; TimeoutSec = 30 }
  if ($null -ne $Body) { $params.Body = ($Body | ConvertTo-Json -Depth 6); $params.ContentType = 'application/json; charset=utf-8' }
  try { return Invoke-RestMethod @params }
  catch {
    $msg = $_.Exception.Message
    if ($_.Exception.Response) {
      try { $reader = New-Object System.IO.StreamReader($_.Exception.Response.GetResponseStream()); $detail = $reader.ReadToEnd(); $msg += " :: $detail" } catch {}
    }
    throw $msg
  }
}

switch ($Action.ToLowerInvariant()) {
  'whoami' {
    $me = Invoke-GH 'GET' "$api/user"
    $repo = $null
    try { $repo = Invoke-GH 'GET' "$api/repos/$User/$Name" } catch { }
    [pscustomobject]@{ login = $me.login; name = $me.name; repos = $me.public_repos; targetRepo = $(if ($repo) { $repo.full_name } else { '(missing)' }) } | ConvertTo-Json
  }
  'ensurerepo' {
    try {
      $repo = Invoke-GH 'GET' "$api/repos/$User/$Name"
      "exists: $($repo.html_url)"
    } catch {
      $body = @{ name = $Name; description = 'MCL Minecraft Java launcher + Win10 release server'; homepage = 'http://192.168.3.173:7891'; private = $false; has_issues = $true; has_wiki = $false }
      $repo = Invoke-GH 'POST' "$api/user/repos" $body
      "created: $($repo.html_url)"
    }
  }
  'push' {
    Push-Location 'E:\MCL'
    try {
      git config user.name 'Tiantian124446' | Out-Null
      git config user.email 'tiantian1244464446@outlook.com' | Out-Null
      git add -A
      git commit -m $Message 2>&1 | Out-Null
      $url = "https://$token@github.com/$User/$Name.git"
      $out = git -c credential.helper= push $url HEAD:main 2>&1
      $text = ($out | Out-String)
      $text = $text.Replace($token, '***')
      $text
    } finally { Pop-Location }
  }
  'release' {
    $release = $null
    try { $release = Invoke-GH 'GET' "$api/repos/$User/$Name/releases/tags/$Tag" } catch { }
    if (-not $release) {
      $notes = 'MCL 启动器 v1.1.0'
      if ($NotesFile -and (Test-Path $NotesFile)) { $notes = [IO.File]::ReadAllText($NotesFile, [Text.Encoding]::UTF8) }
      $release = Invoke-GH 'POST' "$api/repos/$User/$Name/releases" @{ tag_name = $Tag; target_commitish = 'main'; name = "MCL $Tag"; body = $notes; draft = $false; prerelease = $false }
      "release created: $($release.html_url)"
    } else { "release exists: $($release.html_url)" }
    if (Test-Path $Asset) {
      $assetName = 'MCLauncher_v' + $Tag.TrimStart('v') + '.exe'
      $uploadUrl = $release.upload_url -replace '\{.*$', ''
      $bytes = [IO.File]::ReadAllBytes($Asset)
      $req = [System.Net.HttpWebRequest]::Create("$uploadUrl?name=$assetName")
      $req.Method = 'POST'
      $req.Headers.Add('Authorization', "token $token")
      $req.Headers.Add('User-Agent', 'DSH-MCL')
      $req.ContentType = 'application/octet-stream'
      $req.ContentLength = $bytes.Length
      $s = $req.GetRequestStream(); $s.Write($bytes, 0, $bytes.Length); $s.Close()
      $resp = $req.GetResponse()
      $reader = New-Object IO.StreamReader($resp.GetResponseStream())
      $json = $reader.ReadToEnd() | ConvertFrom-Json
      "asset uploaded: $($json.browser_download_url)"
    }
  }
  default { throw "unknown action $Action" }
}
