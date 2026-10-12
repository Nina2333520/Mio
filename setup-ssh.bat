@echo off
setlocal enabledelayedexpansion
title MioNo - ï¿½ï¿½ï¿½ï¿½ SSH Í¨ï¿½ï¿½ï¿½ï¿½ï¿½Æ¹ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½Ï£ï¿?
cls

echo ================================================================
echo    MioNo  --  SSH Í¨ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½
echo ================================================================
echo.
echo   ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ç»·ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿? github.com ï¿½ï¿½ HTTPSï¿½ï¿½443 ï¿½ï¿½ï¿½ï¿½ï¿½Ã£ï¿½ï¿½ï¿½
echo   ï¿½ï¿½ SSH Í¨ï¿½ï¿½ï¿½ï¿½Í¨ï¿½Ä¡ï¿½ï¿½ï¿½ï¿½ï¿½Å±ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½Ð»ï¿½ï¿½ï¿½ SSHï¿½ï¿½
echo.
echo   Ö»ï¿½ï¿½Òªï¿½ï¿½Ò»ï¿½Î£ï¿½ï¿½Ôºï¿½ï¿½ï¿½ï¿½Í¶ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½Ù±ï¿½Ç½ï¿½ï¿½
echo.
echo ================================================================
echo.
pause
cls

REM ---------- ï¿½ï¿½ git ï¿½Ô´ï¿½ï¿½ï¿½ ssh-keygen ----------
set "GIT_EXE="
for /f "delims=" %%i in ('where git 2^>nul') do (
  if not defined GIT_EXE set "GIT_EXE=%%i"
)
if not defined GIT_EXE (
  for %%p in (
    "%ProgramFiles%\Git\cmd\git.exe"
    "%ProgramFiles(x86)%\Git\cmd\git.exe"
    "%LocalAppData%\Programs\Git\cmd\git.exe"
    "C:\Program Files\Git\cmd\git.exe"
    "C:\Program Files (x86)\Git\cmd\git.exe"
    "D:\Program Files\Git\cmd\git.exe"
    "D:\Git\cmd\git.exe"
  ) do if not defined GIT_EXE if exist %%p set "GIT_EXE=%%~p"
)
if not defined GIT_EXE (
  echo [Í£Ö¹] Ã»ï¿½Òµï¿½ gitï¿½ï¿½ï¿½ï¿½×° Git ï¿½ï¿½ï¿½ï¿½ï¿½Ð±ï¿½ï¿½Å±ï¿½ï¿½ï¿½
  pause
  exit /b 1
)
echo    ï¿½Òµï¿½ Git: !GIT_EXE!
echo.

REM ---------- ssh-keygen Â·ï¿½ï¿½ ----------
set "SK="
for %%p in (
  "C:\Windows\System32\OpenSSH\ssh-keygen.exe"
  "C:\Program Files\OpenSSH\ssh-keygen.exe"
) do if not defined SK if exist %%p set "SK=%%~p"
if not defined SK (
  for /f "delims=" %%i in ('where ssh-keygen 2^>nul') do (
    if not defined SK set "SK=%%i"
  )
)
if not defined SK (
  echo [Í£Ö¹] Ã»ï¿½Òµï¿½ ssh-keygenï¿½ï¿½
  echo        ï¿½ï¿½ ï¿½ï¿½ï¿½ï¿½ - ÏµÍ³ - ï¿½ï¿½Ñ¡ï¿½ï¿½ï¿½ï¿½ - ï¿½ï¿½ï¿½Ó¹ï¿½ï¿½Ü£ï¿½
  echo        ï¿½ï¿½×° "OpenSSH ï¿½Í»ï¿½ï¿½ï¿½" ï¿½ï¿½ï¿½ï¿½ï¿½Ü±ï¿½ï¿½Å±ï¿½ï¿½ï¿½
  pause
  exit /b 1
)
echo    ï¿½Òµï¿½ ssh-keygen: !SK!
echo.

REM ---------- ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½Ô¿ ----------
set "_SSHDIR=%USERPROFILE%\.ssh"
set "_KEY=%_SSHDIR%\id_ed25519_miono"
if not exist "!_SSHDIR!" mkdir "!_SSHDIR!" >nul 2>&1

if exist "!_KEY!" (
  echo    ï¿½ï¿½Ô¿ï¿½Ñ´ï¿½ï¿½Ú£ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½É¡ï¿½
) else (
  echo    ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½Ô¿...
  "!SK!" -t ed25519 -f "!_KEY!" -N "" -C "miono@local" -q
  if not exist "!_KEY!" (
    echo [Í£Ö¹] ï¿½ï¿½Ô¿ï¿½ï¿½ï¿½ï¿½Ê§ï¿½Ü¡ï¿½
    pause
    exit /b 1
  )
  echo    ï¿½ï¿½Ô¿ï¿½ï¿½ï¿½ï¿½ï¿½É¡ï¿½
)
echo.

REM ---------- Ð´ SSH config ----------
set "_CFG=%_SSHDIR%\config"
findstr /i "ssh.github.com" "!_CFG!" >nul 2>&1
if errorlevel 1 (
  REM Ö»Ð´Èë ASCII ÄÚÈÝ£¬¾ø²»´ø BOM / ÖÐÎÄ£¬±£Ö¤ ssh ÄÜÕý³£¶ÁÈ¡
  echo.>>"!_CFG!"
  echo Host github-miono>>"!_CFG!"
  echo   HostName ssh.github.com>>"!_CFG!"
  echo   User git>>"!_CFG!"
  echo   Port 443>>"!_CFG!"
  echo   IdentityFile ~/.ssh/id_ed25519_miono>>"!_CFG!"
  echo   StrictHostKeyChecking accept-new>>"!_CFG!"
  echo     SSH ÅäÖÃÒÑÐ´Èë: !_CFG!
) else (
  echo    SSH ÅäÖÃÒÑ´æÔÚ£¬Ìø¹ý¡£
)

REM ---------- ÇåÀí config ÀïµÄ BOM / ÂÒÂë ----------
REM ËµÃ÷£ºconfig ÎÄ¼þ¿ªÍ·µÄ BOM »áÈÃ ssh ±¨ "no argument after keyword"
powershell -NoProfile -ExecutionPolicy Bypass -Command "$p='!_CFG!'; if(Test-Path $p){ $t=[IO.File]::ReadAllText($p); $t=$t.TrimStart([char]0xFEFF).TrimStart(); $t=($t -split \"`n\" | Where-Object { $_ -match '^\s*(Host|HostName|User|Port|IdentityFile|StrictHostKeyChecking|#)' -or $_ -eq '' }) -join [Environment]::NewLine; [IO.File]::WriteAllText($p, $t.TrimEnd() + [Environment]::NewLine, (New-Object Text.UTF8Encoding $false)); Write-Host '    config ÒÑÇåÀíÍê³É' }" 
echo.

REM ---------- ï¿½ï¿½Ê¾ï¿½ï¿½Ô¿ ----------
cls
echo ================================================================
echo    ï¿½ï¿½Ò»ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½Ð¹ï¿½Ô¿ï¿½ï¿½ï¿½Æµï¿½ GitHub
echo ================================================================
echo.
echo    ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½Ö·ï¿½ï¿?
echo      https://github.com/settings/ssh/new
echo.
echo    Title ï¿½î£º MioNo
echo    Key   ï¿½î£º ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½Ò»ï¿½ï¿½ï¿½Ð£ï¿½È«Ñ¡ï¿½ï¿½ï¿½Æ£ï¿½
echo.
echo    ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½  Add SSH key
echo.
echo ----------------------------------------------------------------
type "!_KEY!.pub"
echo ----------------------------------------------------------------
echo.
echo    ï¿½ï¿½ï¿½Æ·ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½Ñ¡ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½Ò»ï¿½ï¿? -^> ï¿½Ò¼ï¿½ -^> ï¿½ï¿½ï¿½ï¿½
echo.
echo ================================================================
echo    ï¿½ï¿½ï¿½Æ²ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½Éºó£?»Øµï¿½ï¿½ï¿½ï¿½ï£¬ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿?
echo ================================================================
echo.
start "" "https://github.com/settings/ssh/new"
echo.
echo    ! Èç¹ûä¯ÀÀÆ÷´ò²»¿ªÉÏÃæµÄÍøÖ·£¨github.com ÍøÒ³¿ÉÄÜ±»Ç½£©£º
echo      ²»ÓÃ¹ÜËü£¬Ö±½Ó¹Øµôä¯ÀÀÆ÷£¬»ØÀ´ÔËÐÐ  add-key-api.bat
echo      ÄÇ¸ö½Å±¾ÓÃ API ¼Ó¹«Ô¿£¬ÍêÈ«²»ÐèÒª´ò¿ªÍøÒ³¡£
pause

REM ---------- ï¿½ï¿½ï¿½ï¿½ SSH ï¿½ï¿½ï¿½ï¿½ ----------
cls
echo ================================================================
echo    ï¿½Ú¶ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ SSH ï¿½ï¿½ï¿½ï¿½
echo ================================================================
echo.

set "SSH_EXE="
for %%p in (
  "C:\Windows\System32\OpenSSH\ssh.exe"
) do if not defined SSH_EXE if exist %%p set "SSH_EXE=%%~p"
if not defined SSH_EXE (
  for /f "delims=" %%i in ('where ssh 2^>nul') do (
    if not defined SSH_EXE set "SSH_EXE=%%i"
  )
)

if not defined SSH_EXE (
  echo    [ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½] Ã»ï¿½Òµï¿½ ssh.exeï¿½ï¿½ï¿½ï¿½ï¿½ï¿½Ó°ï¿½ï¿½ï¿½ï¿½ï¿½Í¡ï¿½
  goto DONE
)

"!SSH_EXE!" -T -o StrictHostKeyChecking=accept-new -o ConnectTimeout=20 -o IdentityFile="!_KEY!" -p 443 git@ssh.github.com
set "RC=!errorlevel!"

echo.
if !RC! neq 1 (
  REM GitHub ï¿½É¹ï¿½Ê±ï¿½á·µï¿½ï¿½ 1 ï¿½ï¿½ï¿½ï¿½Ó¡ Hi xxx! You've successfully authenticated
  echo    ------------------------------------------
  echo    ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½Ã»ï¿½ï¿½ï¿½ï¿?  "successfully authenticated"
  echo    Ëµï¿½ï¿½ï¿½ï¿½Ô¿ï¿½ï¿½Ã»ï¿½ï¿½ï¿½ï¿½È¥ï¿½ï¿½ï¿½ï¿½ï¿½ß¼Ó´ï¿½ï¿½Ë¡ï¿½
  echo    ------------------------------------------
  echo.
  echo    ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿?
  echo      - ï¿½ï¿½Ô¿ï¿½ï¿½Ã»Õ³ï¿½ï¿½ï¿½ï¿½ GitHub -^> ï¿½Øµï¿½ï¿½ï¿½Ò»ï¿½ï¿½
  echo      - Õ³ï¿½ï¿½Ê±Â©ï¿½Ë¿ï¿½Í·ï¿½ï¿½ ssh-ed25519    -^> ï¿½ï¿½ï¿½Â¸ï¿½ï¿½ï¿½
  echo      - Õ³ï¿½ï¿½Ê±ï¿½ï¿½ï¿½Ë½ï¿½Î²ï¿½ï¿½ miono@local    -^> ï¿½ï¿½ï¿½Â¸ï¿½ï¿½ï¿½
  echo.
) else (
  echo    ------------------------------------------
  echo    ï¿½ï¿½ï¿½ï¿½ "successfully authenticated" ï¿½Í³É¹ï¿½ï¿½ï¿½
  echo    ------------------------------------------
)

:DONE
echo.
echo ================================================================
echo    ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿?
echo ================================================================
echo.
echo    ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½
echo.
echo    1. Ë«ï¿½ï¿½  push-to-github.bat
echo.
echo    2. ï¿½Ö¿ï¿½ï¿½Ö·ï¿½ï¿½Ò»ï¿½ï¿½ï¿½ï¿½ï¿½î£?
echo         Nina2333520/Mio
echo         ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ https://github.com/Nina2333520/Mio.git ï¿½ï¿½ï¿½Ð£ï¿½
echo           ï¿½Å±ï¿½ï¿½ï¿½ï¿½Ô¶ï¿½×ªï¿½ï¿½ SSH ï¿½ï¿½Ê½ï¿½ï¿½
echo.
echo    3. Ö®ï¿½ï¿½Í»ï¿½ï¿½Ô¶ï¿½ï¿½ï¿½ï¿½Í£ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½Ù±ï¿½ï¿½ï¿½ï¿½çµ²×¡ï¿½ï¿?
echo.
echo ================================================================
echo.
pause
exit /b 0
