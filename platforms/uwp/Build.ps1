[CmdletBinding()]
param(
    [ValidateSet('Smoke', 'Game', 'Desktop')][string]$Mode = 'Smoke',
    [string]$DependencyRoot = '',
    [string]$Publisher = 'CN=FoxhollowPersonal',
    [string]$CertificatePath = '',
    [SecureString]$CertificatePassword,
    [int]$BuildJobs = 6,
    [ValidateSet('Debug', 'Release', 'RelWithDebInfo')][string]$Configuration = 'Release',
    [string]$SourceRoot = '',
    [string]$JpegRoot = '',
    [switch]$Package
)
$ErrorActionPreference = 'Stop'
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
$vsRoot = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (!$vsRoot) { throw 'MSVC x64' }
$vcvars = Join-Path $vsRoot 'VC/Auxiliary/Build/vcvars64.bat'
$environmentDir = Join-Path $repoRoot '.tools'
New-Item -ItemType Directory -Path $environmentDir -Force | Out-Null
$environmentScript = Join-Path $environmentDir 'msvc-environment.cmd'
@('@echo off', ('call "{0}" >nul' -f $vcvars), 'if errorlevel 1 exit /b 1', 'set') |
    Set-Content -LiteralPath $environmentScript -Encoding ascii
$buildEnvironment = & $env:ComSpec /d /c $environmentScript
if ($LASTEXITCODE -ne 0) { throw 'vcvars64' }
foreach ($line in $buildEnvironment) {
    if ($line -match '^([^=]+)=(.*)$') {
        [Environment]::SetEnvironmentVariable($matches[1], $matches[2], 'Process')
    }
}
function Resolve-BuildTool([string]$name, [string]$fallback) {
    $found = Get-Command $name -ErrorAction SilentlyContinue
    if ($found) { return $found.Source }
    $local = Join-Path $repoRoot $fallback
    if (Test-Path -LiteralPath $local) { return $local }
    throw $name
}
$cmake = Resolve-BuildTool 'cmake.exe' '.tools/python/cmake/data/bin/cmake.exe'
$ninja = Resolve-BuildTool 'ninja.exe' '.tools/python/bin/ninja.exe'
$sdkBin = Join-Path $env:WindowsSdkDir ('bin/{0}/x64' -f $env:WindowsSDKVersion.TrimEnd('\'))
$source = $repoRoot
if ($SourceRoot) { $source = (Resolve-Path -LiteralPath $SourceRoot).Path }
$build = Join-Path $repoRoot ('build-{0}-ninja' -f $Mode.ToLowerInvariant())
if ($SourceRoot) { $build = Join-Path $source 'build-debug' }
$configure = @('-S', $source, '-B', $build, '-G', 'Ninja', "-DCMAKE_MAKE_PROGRAM=$ninja", "-DCMAKE_BUILD_TYPE=$Configuration")
if ($JpegRoot) {
    $configure += "-DJPEG_LIBRARY=$JpegRoot/lib/jpeg-static.lib", "-DJPEG_INCLUDE_DIR=$JpegRoot/include"
}
$target = 'foxhollow'
if ($Mode -eq 'Smoke') {
    $configure[1] = $PSScriptRoot
    $target = 'foxhollow-uwp-smoke'
} else {
    $clang = Resolve-BuildTool 'clang-cl.exe' '.tools/llvm/bin/clang-cl.exe'
    $env:PATH = "$(Split-Path $clang);$env:PATH"
    $configure += "-DCMAKE_C_COMPILER=$clang", "-DCMAKE_CXX_COMPILER=$clang"
}
if ($Mode -ne 'Desktop') {
    if ($Configuration -ne 'Release') { throw 'UWP: Release' }
    $configure += '-DCMAKE_SYSTEM_NAME=WindowsStore', '-DCMAKE_SYSTEM_VERSION=10.0', '-DCMAKE_SYSTEM_PROCESSOR=AMD64', '-DFOXHOLLOW_ENABLE_CODE_MODS=OFF',
        '-DFOXHOLLOW_DEBUG_SHORTCUTS=OFF', '-DBUILD_SHARED_LIBS=OFF',
        "-DFOXHOLLOW_UWP_PUBLISHER=$Publisher", "-DFOXHOLLOW_MAKEAPPX=$(Join-Path $sdkBin 'MakeAppx.exe')"
    if ($DependencyRoot) { $configure += "-DFOXHOLLOW_UWP_DEP_ROOT=$DependencyRoot" }
}
& $cmake @configure 2>&1 | Tee-Object -FilePath (Join-Path $environmentDir ('configure-{0}.log' -f $Mode))
if ($LASTEXITCODE -ne 0) { throw 'CMake configure' }
if ($Package -and $Mode -ne 'Desktop') { $target = 'foxhollow-uwp-package' }
& $cmake --build $build --target $target --parallel $BuildJobs 2>&1 |
    Tee-Object -FilePath (Join-Path $environmentDir ('build-{0}.log' -f $Mode))
if ($LASTEXITCODE -ne 0) { throw 'CMake build' }
if ($Package -and $CertificatePath -and $Mode -ne 'Desktop') {
    $appx = Join-Path $build 'foxhollow-Release-x64.appx'
    $signArgs = @('sign', '/fd', 'SHA256', '/f', $CertificatePath)
    $passwordHandle = [IntPtr]::Zero
    try {
        if ($CertificatePassword) {
            $passwordHandle = [Runtime.InteropServices.Marshal]::SecureStringToBSTR($CertificatePassword)
            $signArgs += '/p', [Runtime.InteropServices.Marshal]::PtrToStringBSTR($passwordHandle)
        }
        & (Join-Path $sdkBin 'SignTool.exe') @signArgs $appx
    } finally {
        if ($passwordHandle -ne [IntPtr]::Zero) {
            [Runtime.InteropServices.Marshal]::ZeroFreeBSTR($passwordHandle)
        }
        $signArgs = $null
    }
    if ($LASTEXITCODE -ne 0) { throw 'SignTool' }
}
