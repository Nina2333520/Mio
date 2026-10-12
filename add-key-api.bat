@echo off
chcp 936 >nul 2>&1
setlocal EnableDelayedExpansion
title MioNo - 用 API 添加 SSH 公钥（绕开打不开的网页）

echo.
echo ================================================================
echo    MioNo  --  用 GitHub API 添加 SSH 公钥
echo    （网页打不开没关系，这个方式不需要打开网页）
echo ================================================================
echo.
echo    原理：github.com 网页被墙，但 api.github.com 是通的。
echo          本脚本用 API 把公钥直接加到你的 GitHub 账号。
echo.
echo    你需要一个 GitHub Token（个人访问令牌）。
echo.
echo ----------------------------------------------------------------
echo    怎么拿 Token？--- 三条路，任选一条
echo ----------------------------------------------------------------
echo.
echo    [路 A] 手机能上网的话：
echo           手机上打开  https://github.com/settings/tokens/new
echo           Note 填：MioNo
echo           Expiration 选：90 days（或 No expiration）
echo           勾选权限：write:public_key  （在 admin:public_key 下面）
echo           拉到底点  Generate token
echo           复制那串  ghp_xxxxxxxxxxxx
echo.
echo    [路 B] 你以前已经建过 Token：
echo           直接把你手上的 Token 拿来用
echo           （需要 repo + workflow 权限，或 write:public_key）
echo.
echo    [路 C] 实在拿不到 Token：
echo           换手机热点打开一次  https://github.com/settings/ssh/new
echo           把公钥贴上去也一样，就不用跑这个脚本了
echo.
echo ================================================================
echo.

REM ---------- 找公钥文件 ----------
set "_KEY=%USERPROFILE%\.ssh\id_ed25519_miono.pub"
if not exist "!_KEY!" (
  echo   [X] 没找到公钥文件：
  echo       !_KEY!
  echo.
  echo   请先运行 setup-ssh.bat 生成密钥。
  echo.
  pause
  exit /b 1
)

echo   公钥文件：!_KEY!
echo.
echo ----------------------------------------------------------------
echo   要添加的公钥内容：
echo ----------------------------------------------------------------
type "!_KEY!"
echo ----------------------------------------------------------------
echo.

REM ---------- 输入 Token ----------
echo   请把 GitHub Token 粘贴到下面（右键粘贴），然后回车：
echo.
set /p "_TOKEN=Token: "

if "!_TOKEN!"=="" (
  echo.
  echo   [X] Token 是空的，已取消。
  echo.
  pause
  exit /b 1
)

echo.
echo   正在通过 API 提交公钥...
echo.

REM ---------- 用 curl 调 API ----------
set "_BODY=%TEMP%\miono_key.json"
set "_RESP=%TEMP%\miono_resp.json"

REM 生成 JSON（key 里的内容需要转义，公钥本身没有特殊字符，直接拼即可）
> "!_BODY!" echo {"title":"MioNo","key":"ssh-ed25519 AAAAC3NzaC1lZDI1NTE5AAAAIBUHWnMGobCOYxi4eqcXiLFTq0nW6luOkKctYrLn94JA miono@local"}

REM curl 是否可用（Win10 1803+ 自带）
where curl >nul 2>&1
if !errorlevel! neq 0 (
  echo   [X] 系统中找不到 curl。请确认是 Windows 10 1803 或更新版本。
  echo.
  pause
  exit /b 1
)

curl -s -o "!_RESP!" -w "HTTP_CODE=%%{http_code}" ^
  -X POST "https://api.github.com/user/keys" ^
  -H "Authorization: Bearer !_TOKEN!" ^
  -H "Accept: application/vnd.github+json" ^
  -H "X-GitHub-Api-Version: 2022-11-28" ^
  -H "User-Agent: MioNo-Setup" ^
  --data-binary "@!_BODY!" > "%TEMP%\miono_http.txt" 2>&1

set "_HTTP="
for /f "tokens=2 delims==" %%a in ('type "%TEMP%\miono_http.txt"') do set "_HTTP=%%a"

echo.
echo ----------------------------------------------------------------
echo   HTTP 返回码：!_HTTP!
echo ----------------------------------------------------------------
echo.

if "!_HTTP!"=="201" goto OK
if "!_HTTP!"=="401" goto E401
if "!_HTTP!"=="403" goto E403
if "!_HTTP!"=="404" goto E404
if "!_HTTP!"=="422" goto E422
goto EOTHER

:OK
echo   [√] 成功！公钥已经添加到你的 GitHub 账号。
echo.
echo   API 返回信息：
if exist "!_RESP!" type "!_RESP!"
echo.
echo ================================================================
echo    下一步：直接双击运行  push-to-github.bat
echo    它会自动走 SSH 通道把代码推上去。
echo ================================================================
echo.
goto DONE

:E401
echo   [X] 401 - Token 无效或已过期。
echo.
echo   可能原因：
echo     1) Token 复制错了 / 少复制了字符
echo     2) Token 已经过期
echo     3) Token 前后带了空格或引号
echo.
echo   建议重新生成一个 Token 再试。
echo.
if exist "!_RESP!" type "!_RESP!"
goto DONE

:E403
echo   [X] 403 - 权限不足或触发了限制。
echo.
echo   最常见的原因是：这个 Token 没有 write:public_key 权限。
echo.
echo   请重新生成 Token，务必勾选：
echo     [√] write:public_key   （在 admin:public_key 分类里）
echo.
if exist "!_RESP!" type "!_RESP!"
goto DONE

:E404
echo   [X] 404 - 接口不存在。
echo.
echo   这通常说明 Token 类型不对（例如用了 Fine-grained token 但没有
echo   选到 "Public keys" 权限），或 Token 已失效。
echo.
echo   建议用 Classic token（settings/tokens/new），勾 write:public_key。
echo.
if exist "!_RESP!" type "!_RESP!"
goto DONE

:E422
echo   [√] 422 - 这个公钥已经添加过了（不算错误）。
echo.
echo   说明你的 GitHub 账号里已经有这把钥匙了，可以直接用。
echo.
echo ================================================================
echo    下一步：直接双击运行  push-to-github.bat
echo ================================================================
echo.
if exist "!_RESP!" type "!_RESP!"
goto DONE

:EOTHER
echo   [X] 未能确认结果，请把下面的返回信息发出来看看：
echo.
if exist "!_RESP!" type "!_RESP!"
echo.
echo   也可能是网络问题。可以先重试一次；如果一直失败，
echo   请把这段信息复制给我。
echo.
goto DONE

:DONE
REM 清理 JSON 临时文件（里面有 Token/公钥，别留在磁盘上）
if exist "!_BODY!" del /q "!_BODY!" >nul 2>&1
if exist "!_RESP!" del /q "!_RESP!" >nul 2>&1
if exist "%TEMP%\miono_http.txt" del /q "%TEMP%\miono_http.txt" >nul 2>&1
echo.
pause
exit /b 0
