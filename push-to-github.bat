@echo off
setlocal enabledelayedexpansion
title MioNo - 推到 GitHub

cls
echo ================================================================
echo    MioNo C++ 版  --  一键推送到 GitHub
echo ================================================================
echo.

cd /d "%~dp0"

REM ---------- 自检 1：不能跑在 .git 目录里面 ----------
for %%i in ("%~dp0.") do set "_HERE=%%~nxi"
if /i "!_HERE!"==".git" (
  echo.
  echo ================================================================
  echo   [停止] 你现在是在 .git 文件夹里面运行脚本
  echo ================================================================
  echo.
  echo   脚本必须放在 MioNoCpp 文件夹里双击，不能进 .git 里双击。
  echo.
  echo   请这样做：
  echo     1. 关掉这个窗口
  echo     2. 往上退一级，找到 MioNoCpp 文件夹
  echo     3. 在 MioNoCpp 文件夹里双击 push-to-github.bat
  echo.
  echo   如果你**没有** MioNoCpp 文件夹，只有一堆文件，
  echo   说明解压时把内容解到 .git 里去了，请重新解压一次。
  echo.
  pause
  exit /b 1
)

REM ---------- 找 git（PATH 里没有就去常见位置翻）----------
set "GIT_EXE="
for /f "delims=" %%i in ('where git 2^>nul') do (
  if not defined GIT_EXE set "GIT_EXE=%%i"
)

if not defined GIT_EXE (
  for %%p in (
    "%ProgramFiles%\Git\cmd\git.exe"
    "%ProgramFiles(x86)%\Git\cmd\git.exe"
    "%LocalAppData%\Programs\Git\cmd\git.exe"
    "%ProgramFiles%\Git\bin\git.exe"
    "%ProgramFiles(x86)%\Git\bin\git.exe"
    "%ProgramW6432%\Git\cmd\git.exe"
    "C:\Program Files\Git\cmd\git.exe"
    "C:\Program Files (x86)\Git\cmd\git.exe"
    "D:\Program Files\Git\cmd\git.exe"
    "D:\Git\cmd\git.exe"
    "E:\Git\cmd\git.exe"
  ) do if not defined GIT_EXE if exist %%p set "GIT_EXE=%%~p"
)

if not defined GIT_EXE (
  echo [错误] 没找到 git
  echo.
  echo 你已经装了 Git，但它没有加进系统 PATH。
  echo.
  echo 两个办法，任选一个：
  echo.
  echo   办法一（推荐）：重新装一遍 Git
  echo     去 https://git-scm.com/download/win 下载
  echo     安装到 "Adjusting your PATH environment" 这一步时
  echo     选第二项 "Git from the command line and also from 3rd-party software"
  echo     然后一路 Next 到底。
  echo.
  echo   办法二：手动加 PATH
  echo     1. 按 Win+R，输入 sysdm.cpl，回车
  echo     2. 高级 - 环境变量 - 系统变量里的 Path - 编辑 - 新建
  echo     3. 粘贴这一行：
  echo        C:\Program Files\Git\cmd
  echo     4. 一路确定，然后重启本脚本
  echo.
  pause
  exit /b 1
)

echo     找到 Git: !GIT_EXE!

REM 注意：这里【不】去改 PATH —— 改 PATH 很容易被特殊字符污染
REM （分号、反斜杠、shim 目录都会让 git 静默失效）。
REM 后面全程用 !GIT_EXE! 的绝对路径调用 git，最稳。

REM ---------- 检查是不是「能用的」git 仓库 ----------
REM 判断方式：直接看文件系统，不信 git 的返回值。
REM 一个正常的 .git 目录里必定有 HEAD 和 objects。
REM 只看「.git 存在与否」不够：残缺的 .git 会让 git add
REM 报 "this operation must be run in a work tree"。
set "_REPO_OK=0"
if exist ".git\HEAD" if exist ".git\objects" set "_REPO_OK=1"

if "!_REPO_OK!"=="0" (
  echo [准备] 初始化本地仓库...

  REM 如果 .git 已经存在但不可用，先挪走（不删，改名留着）
  if exist ".git" (
    echo        发现残留的 .git，先备份成 .git.bak...
    if exist ".git.bak" rmdir /s /q ".git.bak" 2>nul
    move /y ".git" ".git.bak" >nul 2>&1
  )

  "!GIT_EXE!" init -q
  if errorlevel 1 (
    echo.
    echo      [失败] git init 没能成功。请把窗口里的报错截图发我。
    echo.
    pause
    exit /b 1
  )

  REM 初始化后再看一遍文件系统
  set "_REPO_OK=0"
  if exist ".git\HEAD" if exist ".git\objects" set "_REPO_OK=1"
)

if "!_REPO_OK!"=="0" (
  echo.
  echo ================================================================
  echo   [失败] 当前目录不是一个可用的 git 工作区
  echo ================================================================
  echo.
  echo   两个可能的原因：
  echo.
  echo   1. 你现在是在 .git 文件夹里面运行脚本
  echo      请退到上一级，在 MioNoCpp 文件夹里重新双击
  echo.
  echo   2. 这个文件夹被解压工具弄坏了
  echo      请把 .git 文件夹手动删掉，再重跑一次脚本
  echo.
  pause
  exit /b 1
)

"!GIT_EXE!" branch -M main >nul 2>&1

REM ---------- 输入仓库地址 ----------
echo ----------------------------------------------------------------
echo   请粘贴你刚在 GitHub 建好的仓库地址
echo   形如： https://github.com/你的用户名/MioNo.git
echo ----------------------------------------------------------------
echo.
set /p REPO_URL="仓库地址: "

if "!REPO_URL!"=="" (
  echo.
  echo [取消] 没填地址。
  pause
  exit /b 1
)

REM ---------- 清理地址：去空格 + 去末尾斜杠 ----------
set "REPO_URL=!REPO_URL: =!"
for /l %%i in (1,1,3) do (
  if "!REPO_URL:~-1!"=="/" set "REPO_URL=!REPO_URL:~0,-1!"
)

REM ---------- 补 .git 后缀（检查末尾4字符，比 findstr 可靠）----------
set "_last4=!REPO_URL:~-4!"
if /i not "!_last4!"==".git" set "REPO_URL=!REPO_URL!.git"

echo.
echo [1/4] 设置远程仓库...
"!GIT_EXE!" remote remove origin >nul 2>&1
"!GIT_EXE!" remote add origin "!REPO_URL!"
if errorlevel 1 (
  echo      [失败] 地址不合法，检查一下有没有拼错。
  pause
  exit /b 1
)
echo      OK
echo.

echo [2/4] 检查分支...
"!GIT_EXE!" branch -M main
echo      OK
echo.

echo [3/4] 暂存全部文件...
"!GIT_EXE!" add -A
if errorlevel 1 (
  echo.
  echo      [失败] git add 出错，文件没能加入暂存区。
  echo      请把上面那行英文报错截图发我。
  echo.
  pause
  exit /b 1
)

"!GIT_EXE!" -c user.name=MioNo -c user.email=miono@local commit -q -m "MioNo C++ / JUCE" >nul 2>&1

REM 校验：本地是否真的有 main 分支了
"!GIT_EXE!" rev-parse --verify main >nul 2>&1
if errorlevel 1 (
  echo.
  echo      [失败] 没有生成 main 分支，本地提交没成功。
  echo.
  echo      最常见原因：这个文件夹里一个文件都没有，
  echo      或者你解压错了位置。
  echo.
  echo      请确认 MioNoCpp 文件夹里能看到这些：
  echo        push-to-github.bat
  echo        CMakeLists.txt
  echo        Source 文件夹
  echo        .github 文件夹
  echo.
  pause
  exit /b 1
)
echo      OK
echo.

echo [4/4] 推送到 GitHub...
echo.
echo   ------------------------------------------------------------
echo    如果弹窗要求登录，选 "Sign in with your browser"，
echo    浏览器里点一下授权就行。
echo   ------------------------------------------------------------
echo.
"!GIT_EXE!" push -u origin main
set "RC=!errorlevel!"
if !RC! neq 0 goto MERGE

:AFTER_PUSH
echo      OK

echo.
echo.
echo ================================================================
echo    推送成功 ！
echo ================================================================
echo.
echo    接下来的 5 步：
echo.
echo    1. 浏览器打开你的仓库页面
echo.
echo    2. 点顶部的  Actions  标签
echo.
echo    3. 左边选 "Build MioNo (Windows)"
echo       右边会有一次黄色/转圈的运行记录，等它变绿勾
echo       第一次要 4~6 分钟（在下载编译 JUCE）
echo.
echo    4. 点进那次运行，滑到页面最底部
echo       在  Artifacts  一栏下载  MioNo-win64.zip
echo.
echo    5. 解压，双击  MioNo.exe
echo.
echo ================================================================
echo.
echo （已帮你把 Actions 页面打开）
set "ACT_URL=!REPO_URL:.git=!"
start "" "!ACT_URL!/actions"
echo.
pause
exit /b 0

:MERGE
echo.
echo   ------------------------------------------------------------
echo   推送被拒绝：远程仓库里已经有内容了。
echo   大概率是上一次推送只成功了一部分。
echo   正在自动合并远程内容，稍等...
echo   ------------------------------------------------------------
echo.

REM 允许历史不相关，冲突时保留本地版本
"!GIT_EXE!" -c user.name=MioNo -c user.email=miono@local pull origin main --no-rebase --allow-unrelated-histories -X ours --no-edit
set "RC=!errorlevel!"

if !RC! neq 0 goto FORCE_PUSH

echo.
echo   合并完成，重新推送...
"!GIT_EXE!" push -u origin main
set "RC=!errorlevel!"
if !RC! neq 0 goto FORCE_PUSH
goto AFTER_PUSH

:FORCE_PUSH
echo.
echo   自动合并遇到麻烦，改为强制覆盖远程内容...
"!GIT_EXE!" push -u origin main --force
set "RC=!errorlevel!"
if !RC! neq 0 goto FAILED
goto AFTER_PUSH

:FAILED
echo.
echo ================================================================
echo    推送失败
echo ================================================================
echo.
echo   对照下面挑一个解决办法：
echo.
echo   [1] 提示 repository not found
echo       -^> 仓库还没建，或地址写错
echo       去 https://github.com/new 建一个
echo       名字填 MioNo，不要勾 Add README，点 Create
echo.
echo   [2] 提示 Authentication failed
echo       -^> 密码处不能填账号密码，要填 Token
echo       去 https://github.com/settings/tokens/new
echo          Note 随便写  MioNo
echo          Expiration 选 90 days
echo          勾选  repo  （整个大类）
echo          点最下面 Generate token
echo          复制那一串 ghp_xxxxx
echo       再跑一次本脚本，用户名填你的 GitHub 用户名，
echo       密码处粘贴刚才那串 Token
echo.
echo   [3] 一直卡住不动
echo       -^> 网络问题，挂个梯子再试
echo.
echo   [4] 提示 src refspec main does not match any
echo       -^> 本地还没提交，重跑一次脚本即可
echo.
pause
exit /b 1
