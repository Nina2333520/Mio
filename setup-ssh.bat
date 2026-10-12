@echo off
setlocal enabledelayedexpansion
title MioNo - 配置 SSH 通道（绕过网络阻断）
cls

echo ================================================================
echo    MioNo  --  SSH 通道配置
echo ================================================================
echo.
echo   你的网络环境连不上 github.com 的 HTTPS（443 被重置），
echo   但 SSH 通道是通的。这个脚本帮你把推送切换到 SSH。
echo.
echo   只需要做一次，以后推送都不会再被墙。
echo.
echo ================================================================
echo.
pause
cls

REM ---------- 找 git 自带的 ssh-keygen ----------
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
  echo [停止] 没找到 git，先装 Git 再运行本脚本。
  pause
  exit /b 1
)
echo    找到 Git: !GIT_EXE!
echo.

REM ---------- ssh-keygen 路径 ----------
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
  echo [停止] 没找到 ssh-keygen。
  echo        打开 设置 - 系统 - 可选功能 - 添加功能，
  echo        安装 "OpenSSH 客户端" 后重跑本脚本。
  pause
  exit /b 1
)
echo    找到 ssh-keygen: !SK!
echo.

REM ---------- 生成密钥 ----------
set "_SSHDIR=%USERPROFILE%\.ssh"
set "_KEY=%_SSHDIR%\id_ed25519_miono"
if not exist "!_SSHDIR!" mkdir "!_SSHDIR!" >nul 2>&1

if exist "!_KEY!" (
  echo    密钥已存在，跳过生成。
) else (
  echo    正在生成新密钥...
  "!SK!" -t ed25519 -f "!_KEY!" -N "" -C "miono@local" -q
  if not exist "!_KEY!" (
    echo [停止] 密钥生成失败。
    pause
    exit /b 1
  )
  echo    密钥已生成。
)
echo.

REM ---------- 写 SSH config ----------
set "_CFG=%_SSHDIR%\config"
findstr /i "ssh.github.com" "!_CFG!" >nul 2>&1
if errorlevel 1 (
  echo.>>"!_CFG!"
  echo Host github-miono>>"!_CFG!"
  echo   HostName ssh.github.com>>"!_CFG!"
  echo   User git>>"!_CFG!"
  echo   Port 443>>"!_CFG!"
  echo   IdentityFile ~/.ssh/id_ed25519_miono>>"!_CFG!"
  echo   StrictHostKeyChecking accept-new>>"!_CFG!"
  echo     SSH 配置已写入: !_CFG!
) else (
  echo    SSH 配置已存在，跳过。
)
echo.

REM ---------- 显示公钥 ----------
cls
echo ================================================================
echo    第一步：把下面这行公钥复制到 GitHub
echo ================================================================
echo.
echo    打开这个网址：
echo      https://github.com/settings/ssh/new
echo.
echo    Title 填： MioNo
echo    Key   填： 下面这一整行（全选复制）
echo.
echo    填完点最下面的  Add SSH key
echo.
echo ----------------------------------------------------------------
type "!_KEY!.pub"
echo ----------------------------------------------------------------
echo.
echo    复制方法：鼠标选中上面那一行 -^> 右键 -^> 复制
echo.
echo ================================================================
echo    复制并添加完成后，回到这里，按任意键继续
echo ================================================================
echo.
start "" "https://github.com/settings/ssh/new"
pause

REM ---------- 测试 SSH 连接 ----------
cls
echo ================================================================
echo    第二步：测试 SSH 连接
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
  echo    [跳过测试] 没找到 ssh.exe，但不影响推送。
  goto DONE
)

"!SSH_EXE!" -T -o StrictHostKeyChecking=accept-new -o ConnectTimeout=20 -o IdentityFile="!_KEY!" -p 443 git@ssh.github.com
set "RC=!errorlevel!"

echo.
if !RC! neq 1 (
  REM GitHub 成功时会返回 1 并打印 Hi xxx! You've successfully authenticated
  echo    ------------------------------------------
  echo    如果上面没看到  "successfully authenticated"
  echo    说明公钥还没加上去，或者加错了。
  echo    ------------------------------------------
  echo.
  echo    常见情况：
  echo      - 公钥还没粘贴到 GitHub -^> 回到第一步
  echo      - 粘贴时漏了开头的 ssh-ed25519    -^> 重新复制
  echo      - 粘贴时少了结尾的 miono@local    -^> 重新复制
  echo.
) else (
  echo    ------------------------------------------
  echo    看到 "successfully authenticated" 就成功了
  echo    ------------------------------------------
)

:DONE
echo.
echo ================================================================
echo    配置完成
echo ================================================================
echo.
echo    接下来：
echo.
echo    1. 双击  push-to-github.bat
echo.
echo    2. 仓库地址那一栏，填：
echo         Nina2333520/Mio
echo         （或者 https://github.com/Nina2333520/Mio.git 都行，
echo           脚本会自动转成 SSH 格式）
echo.
echo    3. 之后就会自动推送，不会再被网络挡住了
echo.
echo ================================================================
echo.
pause
exit /b 0
