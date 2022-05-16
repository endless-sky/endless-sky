# PowerShell test script (for testing on Windows CI services)
param(
  [Parameter(Mandatory=$true, HelpMessage="Path to the Endless Sky executable file")]
  $EndlessSky #The input binary to execute
)
# Ensure the argument is a path to the binary (lest Start-Process complain).
$EndlessSky = Resolve-Path -Path "$EndlessSky" -Relative;

$testName = "Parse Game Data";
# Remove any existing error files first.
$FILEDIR = if ($IsWindows -or $Env:OS.ToLower().Contains('windows')) `
           { "$env:APPDATA\endless-sky" } `
           else { "$env:HOME/.local/share/endless-sky" };
$ERR_FILE = Join-Path -Path $FILEDIR -ChildPath "errors.txt";
if (Test-Path -Path $ERR_FILE) { Remove-Item -Path $ERR_FILE; }

# Parse the game data files
$start = $(Get-Date);
$p = Start-Process -FilePath "$EndlessSky" -ArgumentList '-p' -Wait -PassThru;
$dur = New-TimeSpan -Start $start -End $(Get-Date);

# Assert there is no content in the "errors.txt" file.
if ((Test-Path -Path "$ERR_FILE") -and ((Get-Content -Path "$ERR_FILE" -Raw).Length -gt 0))
{
  $err_msg = "Assertion failed: content written to file $ERR_FILE";
  $content = Get-Content -Path "$ERR_FILE" -Raw;
  Write-Host $content;

  Write-Error -Message $err_msg -Category ParserError;
  exit 1;
}
else { Write-Host "No data-parsing errors were encountered"; }
exit $p.ExitCode;
