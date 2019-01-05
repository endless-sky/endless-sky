# PowerShell test script (for testing on Windows CI services)
param(
  [Parameter(Mandatory=$true, HelpMessage="Path to the Endless Sky executable file")]
  $EndlessSky #The input binary to execute
)
$av = !!$env:APPVEYOR;
$testName = "Parse Game Data";
if ($av)
{
  Add-AppveyorTest -Name $testName -Framework 'ES' -FileName $EndlessSky -Outcome Running;
}
# Remove any existing error files first.
$FILEDIR = if ($IsWindows) { "$env:APPDATA\endless-sky\" } `
           else { "$env:HOME/.local/share/endless-sky" };
$ERR_FILE = Join-Path -Path $FILEDIR -ChildPath "errors.txt";
if (Test-Path -Path $ERR_FILE) { Remove-Item -Path $ERR_FILE; }

# Parse the game data files
$start = $(Get-Date);
$p = Start-Process -FilePath $EndlessSky -ArgumentList '-p' -Wait -PassThru;
$dur = New-TimeSpan -Start $start -End $(Get-Date);

# Assert there is no "errors.txt" file
if (Test-Path -Path "$ERR_FILE")
{
  $err_msg = "Assertion failed: parsing files created file 'errors.txt' in $FILEDIR";
  if ($av)
  {
    $messages = @();
    $(Get-Content -Path $ERR_FILE -Raw).Split("`n") | ForEach-Object `
    {
      if ($_.StartsWith(' ') -or $_.StartsWith('file'))
        { $messages[-1] += "`n$_"; }
      elseif ($_) { $messages += $_; }
    }
    Update-AppveyorTest -Name $testName -Framework 'ES' -FileName $EndlessSky `
      -Outcome Failed -Duration $dur.TotalMilliseconds -ErrorMessage $err_msg `
      -StdErr ($messages -Join "`n`n");
    Push-AppveyorArtifact (Resolve-Path -Path $ERR_FILE -Relative)
  }

  Write-Error -Message $err_msg -Category ParserError;
  $host.SetShouldExit(1);
}
elseif ($av)
{
  Update-AppveyorTest -Name $testName -Framework 'ES' `
    -FileName $EndlessSky -Outcome Passed -Duration $dur.TotalMilliseconds;
}
else { Write-Host "No data-parsing errors were encountered"; }
$host.SetShouldExit($p.ExitCode);
