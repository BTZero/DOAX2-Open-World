[CmdletBinding()]
param(
    [ValidateSet('Baseline','Probe','ProbeDisabled','TerrainTrial','RelaxationHoldTrial','PictorialResearch','PictorialHoldTrial','PictorialLifetimeTrial','PictorialCameraTrial','PictorialActorTrial','PictorialLongActorTrial','PictorialOrbitTrial','PictorialIdleBankTrial','PictorialIdleTrial','PictorialMovementTrial','MotionResearch')][string]$Mode = 'Baseline',
    [string]$BuildDirectory = ''
)
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path $PSScriptRoot -Parent
$buildName = if ($Mode -eq 'Baseline') { 'open-world-baseline' } else { 'open-world-probe' }
$buildRoot = if ($BuildDirectory) { [IO.Path]::GetFullPath((Join-Path $projectRoot $BuildDirectory)) } else { Join-Path $projectRoot "out/$buildName" }
$outputRoot = [IO.Path]::GetFullPath((Join-Path $projectRoot 'out')) + [IO.Path]::DirectorySeparatorChar
if (-not $buildRoot.StartsWith($outputRoot, [StringComparison]::OrdinalIgnoreCase)) {
    throw 'Research build must be inside the workspace out directory.'
}
$gameExe = Join-Path $buildRoot 'doax2.exe'
$gameRoot = Join-Path $projectRoot 'extracted game files here'
$researchRoot = Join-Path $projectRoot 'out/open-world-research'
$userRoot = Join-Path $researchRoot 'userdata'
if (-not (Test-Path -LiteralPath $gameExe)) { throw "Build missing: out/$buildName/doax2.exe" }
$hash = (Get-FileHash -LiteralPath (Join-Path $gameRoot 'default.xex') -Algorithm SHA256).Hash
if ($hash -ne 'C4A14853E1F7AF951BE34F380604113703638780B36D5E74E674FE7431992870') {
    throw 'Unsupported local executable; refusing to run address-specific research.'
}
$alreadyRunning = Get-Process -Name doax2 -ErrorAction SilentlyContinue
if ($alreadyRunning) { throw 'A DOAX2 process is already running; close it normally before research launch.' }
New-Item -ItemType Directory -Force -Path $userRoot,(Join-Path $researchRoot 'logs') | Out-Null
$logPath = Join-Path $researchRoot ('logs/{0}-{1}.log' -f $Mode.ToLowerInvariant(),[DateTime]::UtcNow.ToString('yyyyMMdd-HHmmss-fff'))
$arguments = @(
    ('--game_data_root="{0}"' -f $gameRoot),
    ('--user_data_root="{0}"' -f $userRoot),
    ('--log_file="{0}"' -f $logPath),
    '--gpu_plugin=xenos', '--render_target_path_d3d12=rtv',
    '--gpu_allow_invalid_fetch_constants=true',
    '--video_mode_refresh_rate=60', '--video_mode_width=1280', '--video_mode_height=720',
    '--window_width=1280', '--window_height=720', '--resolution_scale=1',
    '--doax2_engine_60fps=false', '--doax2_native60_tick_scale=1',
    '--doax2_skip_intro_movies=true',
    ('--doax2_pictorial_unlock=' + ($Mode -in @('PictorialResearch','PictorialHoldTrial','PictorialLifetimeTrial','PictorialCameraTrial','PictorialActorTrial','PictorialLongActorTrial','PictorialOrbitTrial','PictorialIdleBankTrial','PictorialIdleTrial','PictorialMovementTrial')).ToString().ToLowerInvariant()),
    '--fullscreen=false', '--vsync=true', '--d3d12_vsync=false',
    '--execute_unclipped_draw_vs_on_cpu=true', '--execute_unclipped_draw_vs_on_cpu_with_scissor=true',
    '--d3d12_doax2_batch_uploads=true', '--d3d12_doax2_async_occlusion_query=true',
    '--mnk_mode=true', '--mnk_mouse=false', '--keybind_a=Space', '--keybind_b=Backspace',
    '--keybind_x=F', '--keybind_y=R',
    '--keybind_start=Return', '--keybind_back=Tab',
    '--keybind_dpad_up=Up', '--keybind_dpad_down=Down',
    '--keybind_dpad_left=Left', '--keybind_dpad_right=Right',
    '--keybind_rstick_up=I', '--keybind_rstick_down=K',
    '--keybind_rstick_left=J', '--keybind_rstick_right=L'
)
if ($Mode -ne 'Baseline') {
    $arguments += '--doax2_open_world_probe=' + ($Mode -in @('Probe','TerrainTrial','RelaxationHoldTrial','PictorialResearch','PictorialHoldTrial','PictorialLifetimeTrial','PictorialCameraTrial','PictorialActorTrial','PictorialLongActorTrial','PictorialOrbitTrial','PictorialIdleBankTrial','PictorialIdleTrial','PictorialMovementTrial','MotionResearch')).ToString().ToLowerInvariant()
    $arguments += '--doax2_open_world_pictorial_idle_bank_trial=' + ($Mode -eq 'PictorialIdleBankTrial').ToString().ToLowerInvariant()
    $arguments += '--doax2_open_world_pictorial_idle_trial=' + ($Mode -eq 'PictorialIdleTrial').ToString().ToLowerInvariant()
    $arguments += '--doax2_open_world_pictorial_movement_trial=' + ($Mode -eq 'PictorialMovementTrial').ToString().ToLowerInvariant()
    $arguments += '--doax2_open_world_pictorial_movement_displacement=' + ($Mode -eq 'PictorialMovementTrial').ToString().ToLowerInvariant()
    $arguments += '--doax2_open_world_pictorial_exploration=' + ($Mode -eq 'PictorialMovementTrial').ToString().ToLowerInvariant()
    $arguments += '--doax2_open_world_motion_research=' + ($Mode -eq 'MotionResearch').ToString().ToLowerInvariant()
    $arguments += '--doax2_open_world_terrain_trial=' + ($Mode -eq 'TerrainTrial').ToString().ToLowerInvariant()
    $arguments += '--doax2_open_world_relaxation_hold_trial=' + ($Mode -eq 'RelaxationHoldTrial').ToString().ToLowerInvariant()
    $arguments += '--doax2_open_world_pictorial_unlock=false'
    $arguments += '--doax2_open_world_pictorial_hold_trial=' + ($Mode -eq 'PictorialHoldTrial').ToString().ToLowerInvariant()
    $arguments += '--doax2_open_world_pictorial_lifetime_trial=' + ($Mode -eq 'PictorialLifetimeTrial').ToString().ToLowerInvariant()
    $arguments += '--doax2_open_world_pictorial_actor_trial=' + ($Mode -eq 'PictorialActorTrial').ToString().ToLowerInvariant()
    $arguments += '--doax2_open_world_pictorial_long_actor_trial=' + ($Mode -eq 'PictorialLongActorTrial').ToString().ToLowerInvariant()
    $arguments += '--doax2_open_world_pictorial_orbit_trial=' + ($Mode -eq 'PictorialOrbitTrial').ToString().ToLowerInvariant()
    $arguments += '--doax2_open_world_pictorial_camera_trial=' + ($Mode -eq 'PictorialCameraTrial').ToString().ToLowerInvariant()
}
$started = Start-Process -FilePath $gameExe -ArgumentList $arguments -WorkingDirectory $projectRoot -WindowStyle Hidden -PassThru
$session = [pscustomobject]@{
    ProcessId = $started.Id; Mode = $Mode; Log = $logPath; UserData = $userRoot
    Executable = $gameExe; XexSha256 = $hash
    StartedUtc = $started.StartTime.ToUniversalTime().ToString('o')
}
$session | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $researchRoot 'session.json') -Encoding UTF8
$session | Select-Object ProcessId, Mode, Log, UserData
