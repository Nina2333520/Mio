@echo off
setlocal enabledelayedexpansion
title MioNo - 关闭 SSH 通道
cls

echo ================================================================
echo    MioNo  --  关闭 SSH 通道
echo ================================================================
echo.
echo   这个操作会：
echo     1. 从 ~/.ssh/config 里删掉 github-miono 那段配置
echo     2. 把推送脚本改回走 HTTPS
echo.
echo   密钥文件会被保留（不动），删了以后想再开要重新生成。
echo.
echo ================================================================
echo.
set /p _OK="确定要关闭吗？输入 Y 回车继续，其它键取消: "
if /i not "!_OK!"=="Y" (
  echo.
  echo   已取消，什么都没改。
  echo.
  pause
  exit /b 0
)

set "_SSHDIR=%USERPROFILE%\.ssh"
set "_CFG=%_SSHDIR%\config"

echo.
if not exist "!_CFG!" (
  echo   [情况] 没有找到 !_CFG!
  echo          说明本来就没配过 SSH，不用关。
  echo.
  pause
  exit /b 0
)

REM ---- 备份原 config ----
copy /y "!_CFG!" "!_CFG!.bak" >nul 2>&1
echo   [1/2] 已备份原配置到 config.bak

REM ---- 用 PowerShell 精确删除 github-miono 那一段 ----
powershell -NoProfile -Command ^
  "$p = '%_CFG%';" ^
  "$lines = Get-Content -LiteralPath $p -Encoding UTF8;" ^
  "$out = New-Object System.Collections.ArrayList;" ^
  "$skip = $false;" ^
  "foreach ($l in $lines) {" ^
  "  if ($l -match '^\s*Host\s+github-miono\s*$') { $skip = $true; continue }" ^
  "  if ($skip -and $l -match '^\s*Host\s+') { $skip = $false }" ^
  "  if ($skip) { continue }" ^
  "  [void]$out.Add($l)" ^
  "}" ^
  "Set-Content -LiteralPath $p -Value $out -Encoding UTF8;"

if errorlevel 1 (
  echo.
  echo   [失败] 删除配置时出错。
  echo          你可以手动打开 !_CFG! ，
  echo          把里面 Host github-miono 开头的那一整段删掉。
  echo.
  pause
  exit /b 1
)
echo   [2/2] 已从 config 里删除 github-miono 配置

echo.
echo ================================================================
echo   已关闭 SSH 通道
echo ================================================================
echo.
echo   现在的状态：
echo     - config 里的 github-miono 配置已移除
echo     - push-to-github.bat 会自动改回 HTTPS 方式
echo.
echo   备注：
echo     - 密钥文件仍在 !_SSHDIR!\id_ed25519_miono
echo       以后想重新开启，双击 setup-ssh.bat 即可（会复用这个密钥）
echo     - GitHub 网站上那条公钥也还在（想删的话去
echo       https://github.com/settings/keys 点 Delete）
echo     - 原配置备份在 !_CFG!.bak
echo.
echo ================================================================
echo.
pause
exit /b 0
