$ErrorActionPreference = 'Stop'

# B-STAGE9-179: PowerShell does not fail the step on a native command's
# non-zero exit; every native call below is checked against $LASTEXITCODE.

# Get the libraries
if (-not (Test-Path 'C:\LIBS')) {
  Start-FileDownload "https://github.com/GoldenCheetah/WindowsSDK/releases/download/v0.1.1/gc-ci-libs.zip"
  7z x -y gc-ci-libs.zip -oC:\LIBS
  if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}

# Get jom
if (-not (Test-Path 'C:\JOM')) {
  Start-FileDownload "https://download.qt.io/official_releases/jom/jom_1_1_3.zip"
  7z x -y jom_1_1_3.zip -oC:\JOM\
  if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}

# GSL
# B-STAGE9-168: root vcpkg.json forces manifest mode; --classic keeps this a classic-mode install.
vcpkg install --classic gsl:x64-windows
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

# Get R
if (-not (Test-Path 'C:\R')) {
  # Lets use 4.1 until 4.2 issues are fixed
  #$rurl = $(ConvertFrom-JSON $(Invoke-WebRequest https://rversions.r-pkg.org/r-release-win).Content).URL
  $rurl = "https://cran.r-project.org/bin/windows/base/old/4.1.3/R-4.1.3-win.exe"
  Start-FileDownload $rurl "R-win.exe"
  $rProc = Start-Process -FilePath .\R-win.exe -ArgumentList "/VERYSILENT /DIR=C:\R" -NoNewWindow -Wait -PassThru
  if ($rProc.ExitCode -ne 0) { exit $rProc.ExitCode }
}
C:\R\bin\R --version
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
