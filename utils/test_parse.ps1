# PowerShell test script (for testing on Windows CI services)
param(
  [Parameter(Mandatory=$true, HelpMessage="Path to the Endless Sky executable file")]
  $EndlessSky, #The input binary to execute
  [Parameter(HelpMessage="Path to the Endless Sky configuration")]
  $Config
)
# Ensure the argument is a path to the binary (lest Start-Process complain).
$EndlessSky = Resolve-Path -Path "$EndlessSky" -Relative;

# Remove any existing error files first.
$FILEDIR = if ($IsWindows) `
          { "$env:APPDATA\endless-sky" } `
          elseif ($IsMacOS) { "$env:HOME/Library/Application Support/endless-sky" } `
          else { "$env:HOME/.local/share/endless-sky" };
if ($Config) { $FILEDIR = $CONFIG; }
$ERR_FILE = Join-Path -Path $FILEDIR -ChildPath "errors.txt";
if (Test-Path -Path $ERR_FILE) { Remove-Item -Path $ERR_FILE; }

# Parse the game data files
if ($CONFIG)
{
  $p = Start-Process -FilePath "$EndlessSky" -ArgumentList '-p','--config',"$FILEDIR" -Wait -PassThru -NoNewWindow;
}
else
{
  $p = Start-Process -FilePath "$EndlessSky" -ArgumentList '-p' -Wait -PassThru -NoNewWindow;
}

# Assert there is no content in the "errors.txt" file.
if ((Test-Path -Path "$ERR_FILE") -and ((Get-Content -Path "$ERR_FILE" -Raw).Length -gt 0))
{
  $err_msg = "Assertion failed: content written to file $ERR_FILE";
  Write-Error -Message $err_msg;
  exit 1;
}
else { Write-Host "No data-parsing errors were encountered"; }
exit $p.ExitCode;
