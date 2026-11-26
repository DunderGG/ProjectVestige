# Copyright (C) 2026 David Bennehag and the Project Vestige contributors.
# SPDX-License-Identifier: GPL-3.0-or-later

<#
.SYNOPSIS
Builds the VestigeEditor target and runs the Vestige automation tests, then prints a short summary.

.DESCRIPTION
Run it from PowerShell with the Unreal editor closed. It never edits code or assets: it only builds,
runs the tests and reports. The full logs go to Saved\AgentWork\<Task>\ (Saved\ is ignored by Git),
so a caller reads the summary instead of searching thousands of log lines.

The last line starts with VERIFY: and ends the output. The exit code is 0 only when the build
succeeded and, unless -BuildOnly is given, at least one test ran and none failed.

.PARAMETER Task
A short name for the task, used as the log folder name (letters, digits, - and _).

.PARAMETER Filter
Which tests to run: Vestige for the whole suite, or Vestige.<Area> for one area.

.PARAMETER BuildOnly
Build without running the tests.

.PARAMETER EngineRoot
The Unreal Engine install folder (the one that contains Engine). When omitted, the script tries the
UE_ROOT environment variable, then the Epic launcher's install list, then the registry (the engine list, then the install entry), then
C:\Program Files\Epic Games\UE_<version>, where <version> is the project's EngineAssociation.

.PARAMETER TestTimeoutMinutes
Stops the test run after this many minutes and counts it as a failure.

.EXAMPLE
.\Tools\Verify-Vestige.ps1 -Task crops-stage-days
#>
[CmdletBinding()]
param(
	[ValidatePattern('^[A-Za-z0-9_-]+$')]
	[string]$Task = 'verify',

	[ValidatePattern('^[A-Za-z0-9_.]+$')]
	[string]$Filter = 'Vestige',

	[switch]$BuildOnly,

	[string]$EngineRoot,

	[ValidateRange(1, 120)]
	[int]$TestTimeoutMinutes = 30
)

$MaxLinesShown = 15
$ProjectRoot = Split-Path -Parent $PSScriptRoot
$Uproject = Join-Path $ProjectRoot 'Vestige.uproject'
$LogDir = Join-Path $ProjectRoot "Saved\AgentWork\$Task"
$BuildLog = Join-Path $LogDir 'build.log'
$TestLog = Join-Path $LogDir 'tests.log'

function Stop-Verify([string]$Reason)
{
	Write-Output "VERIFY: stopped, $Reason"
	exit 2
}

function Write-Lines([string]$Title, $Lines)
{
	$Lines = @($Lines)
	if ($Lines.Count -eq 0)
	{
		return
	}
	Write-Output "${Title}:"
	$Lines | Select-Object -First $MaxLinesShown | ForEach-Object { Write-Output "  $($_.ToString().Trim())" }
	if ($Lines.Count -gt $MaxLinesShown)
	{
		Write-Output "  ... and $($Lines.Count - $MaxLinesShown) more in the log"
	}
}

function Test-EngineRoot([string]$Candidate)
{
	return (-not [string]::IsNullOrWhiteSpace($Candidate)) -and (Test-Path -LiteralPath (Join-Path $Candidate 'Engine\Build\BatchFiles\Build.bat'))
}

# Finds the engine: -EngineRoot, UE_ROOT, the launcher's install list, the registry, the default folder.
function Resolve-EngineRoot([string]$Explicit)
{
	if ($Explicit)
	{
		if (Test-EngineRoot $Explicit)
		{
			return $Explicit
		}
		Stop-Verify "-EngineRoot '$Explicit' has no Engine\Build\BatchFiles\Build.bat. Pass the folder that contains Engine."
	}

	$Version = '5.8'
	if (Test-Path -LiteralPath $Uproject)
	{
		$Association = (Get-Content -LiteralPath $Uproject -Raw | ConvertFrom-Json).EngineAssociation
		if ($Association -match '^\d+\.\d+$')
		{
			$Version = $Association
		}
	}

	$Tried = @()
	$Candidates = [ordered]@{}
	$Candidates['UE_ROOT environment variable'] = $env:UE_ROOT

	$LauncherList = Join-Path $env:ProgramData 'Epic\UnrealEngineLauncher\LauncherInstalled.dat'
	if (Test-Path -LiteralPath $LauncherList)
	{
		try
		{
			$Installed = (Get-Content -LiteralPath $LauncherList -Raw | ConvertFrom-Json).InstallationList | Where-Object { $_.AppName -eq "UE_$Version" } | Select-Object -First 1
			$Candidates['Epic launcher install list'] = $Installed.InstallLocation
		}
		catch
		{
			$Tried += "the Epic launcher install list (unreadable: $LauncherList)"
		}
	}

	# The engine list Windows uses to map an EngineAssociation like "5.8" to a folder (Docs/Setup.md, troubleshooting).
	$Candidates['registry engine list'] = (Get-ItemProperty -Path 'HKCU:\SOFTWARE\Epic Games\Unreal Engine\Builds' -Name $Version -ErrorAction SilentlyContinue).$Version
	$Candidates['registry install entry'] = (Get-ItemProperty -Path "HKLM:\SOFTWARE\EpicGames\Unreal Engine\$Version" -Name InstalledDirectory -ErrorAction SilentlyContinue).InstalledDirectory
	$Candidates['default install folder'] = "C:\Program Files\Epic Games\UE_$Version"

	foreach ($Source in $Candidates.Keys)
	{
		if (Test-EngineRoot $Candidates[$Source])
		{
			return $Candidates[$Source]
		}
		$Tried += "$Source ($(if ($Candidates[$Source]) { $Candidates[$Source] } else { 'not set' }))"
	}
	Stop-Verify "Unreal Engine $Version not found. Tried: $($Tried -join '; '). Pass -EngineRoot <folder that contains Engine> or set UE_ROOT."
}

$EngineRoot = Resolve-EngineRoot $EngineRoot
$BuildBat = Join-Path $EngineRoot 'Engine\Build\BatchFiles\Build.bat'
$EditorCmd = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'

foreach ($RequiredPath in @($Uproject, $BuildBat, $EditorCmd))
{
	if (-not (Test-Path -LiteralPath $RequiredPath))
	{
		Stop-Verify "not found: $RequiredPath"
	}
}

$RunningEditors = @(Get-Process -ErrorAction SilentlyContinue | Where-Object { $_.ProcessName -like 'UnrealEditor*' })
if ($RunningEditors.Count -gt 0)
{
	Stop-Verify "UnrealEditor is running (process $($RunningEditors[0].Id)). Close it first: a build and a test run can't share the project with an open editor."
}

New-Item -ItemType Directory -Force -Path $LogDir | Out-Null

# Build. Output goes to a file; the summary below is all the caller sees.
& $BuildBat VestigeEditor Win64 Development "-Project=$Uproject" -WaitMutex 2>&1 | Out-File -FilePath $BuildLog -Encoding utf8
$BuildExitCode = $LASTEXITCODE

$ResultMatch = Select-String -Path $BuildLog -Pattern '^\s*Result:\s*(\w+)' | Select-Object -Last 1
$BuildResult = if ($ResultMatch) { $ResultMatch.Matches[0].Groups[1].Value } else { 'Unknown' }
$BuildErrors = @(Select-String -Path $BuildLog -Pattern '(: (fatal )?error\b|^\s*error:)' | ForEach-Object { $_.Line } | Sort-Object -Unique)
$OwnWarnings = @(Select-String -Path $BuildLog -Pattern ': warning\b' | Where-Object { $_.Line -match 'Source[\\/]Vestige[\\/]' } | ForEach-Object { $_.Line } | Sort-Object -Unique)
$BuildSucceeded = ($BuildResult -eq 'Succeeded') -and ($BuildExitCode -eq 0)

Write-Output "Build: $BuildResult (exit code $BuildExitCode), $($OwnWarnings.Count) warning(s) in Source\Vestige"
Write-Lines 'Build errors' $BuildErrors
Write-Lines 'Warnings in our code' $OwnWarnings

if (-not $BuildSucceeded)
{
	Write-Output "VERIFY: FAILED, the build did not succeed. Logs: $LogDir"
	exit 1
}

if ($BuildOnly)
{
	Write-Output "VERIFY: OK, build succeeded, tests not run (-BuildOnly). Logs: $LogDir"
	exit 0
}

# Tests. The editor writes its own log; its console is hidden so nothing floods the caller.
if (Test-Path -LiteralPath $TestLog)
{
	Remove-Item -LiteralPath $TestLog
}
$TestArguments = "`"$Uproject`" -ExecCmds=`"Automation RunTests $Filter; Quit`" -unattended -nopause -nullrhi -nosplash -testexit=`"Automation Test Queue Empty`" -abslog=`"$TestLog`""
$TestProcess = Start-Process -FilePath $EditorCmd -ArgumentList $TestArguments -PassThru -WindowStyle Hidden
$null = $TestProcess.Handle
$TimedOut = $false
if (-not $TestProcess.WaitForExit($TestTimeoutMinutes * 60 * 1000))
{
	$TimedOut = $true
	Stop-Process -Id $TestProcess.Id -Force
}

$EditorExitCode = if ($TimedOut) { $null } else { $TestProcess.ExitCode }
$Completed = @()
if (Test-Path -LiteralPath $TestLog)
{
	$Completed = @(Select-String -Path $TestLog -Pattern 'Test Completed\. Result=\{(\w+)\}')
}
$Failed = @($Completed | Where-Object { $_.Matches[0].Groups[1].Value -ne 'Success' })
$FailedNames = @($Failed | ForEach-Object {
		if ($_.Line -match 'Path=\{([^}]*)\}') { $Matches[1] } elseif ($_.Line -match 'Name=\{([^}]*)\}') { $Matches[1] } else { $_.Line }
	})
$Passed = $Completed.Count - $Failed.Count
$FinishedMatch = if (Test-Path -LiteralPath $TestLog) { Select-String -Path $TestLog -Pattern 'TEST COMPLETE\. EXIT CODE:\s*(-?\d+)' | Select-Object -Last 1 }
$LoggedExitCode = if ($FinishedMatch) { [int]$FinishedMatch.Matches[0].Groups[1].Value } else { $null }
$ExitDisplay = if ($null -ne $LoggedExitCode) { $LoggedExitCode } else { 'not logged' }

Write-Output "Tests ($Filter): $($Completed.Count) run, $Passed passed, $($Failed.Count) failed, editor exit code $ExitDisplay"
Write-Lines 'Failed tests' $FailedNames

if ($TimedOut)
{
	Write-Output "VERIFY: FAILED, the tests did not finish within $TestTimeoutMinutes minutes. Logs: $LogDir"
	exit 1
}
if ($null -eq $LoggedExitCode)
{
	Write-Output "VERIFY: FAILED, the test run did not finish normally (no TEST COMPLETE line in the log; the editor probably crashed). Logs: $LogDir"
	exit 1
}
if ($Completed.Count -eq 0)
{
	Write-Output "VERIFY: FAILED, no test result was logged (does the filter match a test?). Logs: $LogDir"
	exit 1
}
if ($Failed.Count -gt 0)
{
	Write-Output "VERIFY: FAILED, $($Failed.Count) test(s) failed. Logs: $LogDir"
	exit 1
}

if (($LoggedExitCode -ne 0) -or ($EditorExitCode -ne 0))
{
	Write-Output "VERIFY: FAILED, the editor ended with exit code $LoggedExitCode (process exit code $EditorExitCode). Logs: $LogDir"
	exit 1
}

Write-Output "VERIFY: OK, build succeeded and $Passed test(s) passed. Logs: $LogDir"
exit 0
