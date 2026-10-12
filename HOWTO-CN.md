# MioNo C++ 版 —— 从零到 exe 的完整步骤

> 全程不用装任何开发环境。你只需要一个浏览器 + 双击两次。
> 整个过程 **约 10 分钟**（其中 5 分钟是等 GitHub 帮你编译）。

---

## 第 0 步：确认你手上有这些

- ✅ 一个 GitHub 账号（没有就去 https://github.com 注册，免费）
- ✅ 电脑上装了 Git（没有的话，**第一步就是装它**）
- ✅ 我给你的 `MioNo-Cpp-Source.zip`

**怎么确认装了 Git**：在**桌面空白处点右键**，如果菜单里能看到
**「Git Bash Here」**或 **「Open Git GUI Here」**，就说明装好了。

> 💡 装过 Git 但没勾 PATH 也没关系 —— 脚本会**自动去常见安装目录里翻**，
> 翻到了就能用，你什么都不用做。

**完全没装 Git**：去 https://git-scm.com/download/win 下载，双击安装，
一路点「Next」到结束即可。装到最后一步「Adjusting your PATH environment」时，
**建议选第二项** `Git from the command line and also from 3rd-party software`
（这样以后在命令行也能直接用 `git`）。装完**重启一下电脑**最稳妥。

> ⚠️ 如果脚本报 **「没找到 git」**，说明 Git 没装或者装在很偏的位置。
> 脚本会把两种解决办法（重装 / 手动加 PATH）打在窗口里，照着做即可。

---

## 第 1 步：在 GitHub 上建一个空仓库

1. 浏览器打开 **https://github.com/new**

2. 照着填（**只有这三处要动**）：

   | 字段 | 填什么 |
   |---|---|
   | **Repository name** | `MioNo` |
   | **Description** | 随便写，可以空着 |
   | **Public / Private** | 选 **Public**（私有仓库跑 Actions 也免费，但公开更省事） |

3. ⚠️ **下面三个方框一个都不要勾**：

   - ❌ Add a README file
   - ❌ Add .gitignore
   - ❌ Choose a license

   > **为什么？** 勾了的话仓库里会先有文件，你推的时候会冲突，脚本会报错。
   > 建一个**完全空的**仓库最省事。

4. 点最下面的绿色按钮 **Create repository**

5. 建完之后页面会跳转，长这样：

   ```
   https://github.com/你的用户名/MioNo
   ```

   **把这个地址复制下来**，下一步要用。也可以直接用页面上
   `Quick setup` 那一块给你的 `https://github.com/你的用户名/MioNo.git`（多个 `.git` 也一样，脚本会自动处理）。

---

## 第 2 步：解压 + 双击推送脚本

1. 找到我给你的 **`MioNo-Cpp-Source.zip`**，右键 → **全部解压**（不要直接在里面双击！）

   解压出来是一个 **`MioNoCpp` 文件夹**，里面有：

   ```
   MioNoCpp/
   ├── push-to-github.bat     ← 双击这个
   ├── setup-ssh.bat          ← 推送失败(网络被挡)时才用
   ├── add-key-api.bat        ← 网页打不开时，用 Token 加公钥
   ├── undo-ssh.bat           ← 想关掉 SSH 通道时用
   ├── QUICKSTART-CN.md       ← 三步速查（懒人版）
   ├── HOWTO-CN.md            ← 你正在看的这份详细说明
   ├── Source/                 ← C++ 代码
   ├── .github/workflows/      ← 自动编译配置
   ├── CMakeLists.txt
   └── assets/app.ico
   ```

2. **双击 `push-to-github.bat`**

   窗口里会先显示一行：

   ```
   找到 Git: C:\Program Files\Git\cmd\git.exe
   ```

   看到这行就放心了，说明 Git 找到了。

3. 接着它会问你「仓库地址」

   **把第 1 步复制的地址粘贴进去**（在命令行里粘贴是**点右键**，不是 Ctrl+V）

   ```
   仓库地址: https://github.com/nina520/MioNo.git
   ```

4. 按回车，等它跑

> 💡 **地址可以偷懒**：直接填 `你的用户名/MioNo` 就行，脚本会自动补成完整地址。

---

## 第 2.5 步：如果提示「连不上 GitHub」

大陆网络访问 `github.com` 的 HTTPS 经常被干扰。如果脚本报：

```
Recv failure: Connection was reset
Could not connect to github.com:443
```

**这不是你的错，也不是脚本的错**，是网络线路的问题。解决办法是用 **SSH 通道**
（走 `ssh.github.com:443`，这条线路在实测中不会被挡）。

1. 回到 `MioNoCpp` 文件夹，**双击 `setup-ssh.bat`**

2. 它会自动：
   - 生成一对专属密钥
   - 配置好 SSH 通道
   - **自动打开 GitHub 的添加公钥页面**，并把公钥显示在窗口里

3. 在浏览器页面里：
   - **Title** 填 `MioNo`
   - **Key** 框里粘贴**窗口里显示的那一整行公钥**（`ssh-ed25519 AAAA...` 开头）
   - 点 **Add SSH key**

> ⚠️ **如果浏览器里这个网址打不开（一直转圈 / 无法访问）**
>
> 这是正常的 —— 被挡的正是 `github.com` 网页本身。别急，有两招：
>
> **招 A（推荐）：换网络打开一次**
> 手机开热点让电脑连一下，或者用手机流量打开 `https://github.com/settings/ssh/new`，
> 把公钥贴上去。就这一次，之后永远不用再开网页。
>
> **招 B：用 `add-key-api.bat` 绕开网页（全程命令行）**
> 双击 `add-key-api.bat`，它会用 `api.github.com`（这条线路是通的）帮你把公钥加到账号上。
> 你需要一个 GitHub Token，脚本里会教你怎么拿（手机上打开一次
> `https://github.com/settings/tokens/new` 即可，勾 `write:public_key`）。
> 把 Token 粘贴进窗口，回车，搞定。

4. 回到黑窗口按任意键，它会自动测试连接

   看到 `Hi xxx! You've successfully authenticated` 就成功了
   （如果看到 `Permission denied (publickey)`，说明公钥还没加到 GitHub，
   回到上面第 3 步处理）。

5. 之后**再双击 `push-to-github.bat`**，地址填 `你的用户名/MioNo` 即可 ——
   脚本会自动识别并走 SSH 通道，以后都不会再被网络挡住。

> **一次性配置**，之后每次推送都自动走 SSH，不用再管。

> **想关掉 SSH 通道？** 双击 `undo-ssh.bat` 就行 —— 它会问你要不要确认，
> 然后从 `config` 里精确删掉那段配置（原来的备份成 `config.bak`），
> 推送脚本会自动改回 HTTPS 方式。密钥文件会保留，以后想再开直接跑 `setup-ssh.bat` 即可。

---

## 第 3 步：登录 GitHub

第一次推送会**弹出登录窗口**（Git Credential Manager）。

推荐做法：

1. 弹窗里选 **Sign in with your browser**（用浏览器登录）
2. 浏览器自动打开 GitHub，点 **Authorize** 或 **Continue**
3. 看到绿色勾就成功了，回到黑窗口继续跑

> 如果**没弹窗**，而是在命令行里问 `Username:` / `Password:`，那就是要手动填：
> - `Username:` 填你的 GitHub 用户名
> - `Password:` **不能填账号密码**！要填 **Token**，见下面的常见问题②

---

## 第 4 步：等 GitHub 帮你编译

推送成功后，黑窗口会显示「推送成功！」并**自动帮你打开 Actions 页面**。

手动去也行：**你的仓库 → 顶部 `Actions` 标签**

你会看到：

```
Build MioNo (Windows)          ← 点左边的这个
  ● 第 1 次运行   🟡 黄色转圈   ← 正在编译，等它
```

**第一次要 4~6 分钟**（正在下载并编译 JUCE 框架，300 多 MB 的东西）。
之后你再改代码重推，有缓存了，**1~2 分钟**就跑完。

等它变成：

```
  ● 第 1 次运行   ✅ 绿色勾
```

---

## 第 5 步：下载 exe

1. **点进那次绿色的运行记录**
2. 页面滑到**最底部**
3. 找到 **Artifacts** 这一栏
4. 点 **`MioNo-win64.zip`** 下载

解压，里面是：

```
MioNo/
├── MioNo.exe        ← 双击这个
├── Source/          ← 源码（方便你对照）
└── README.md
```

**双击 `MioNo.exe` 就能用了** 🎉

---

## 常见问题

### ① 提示 `repository not found`

仓库没建，或者地址打错了。

→ 回第 1 步，去 https://github.com/new 建一个。
→ 或者检查地址：必须是 `https://github.com/你的用户名/MioNo.git` 这种格式。

### ② 提示 `Authentication failed`

**密码处不能填你的 GitHub 登录密码**（GitHub 早就不支持了），要填 **Token**。

生成 Token 的步骤：

1. 打开 **https://github.com/settings/tokens/new**
2. 照填：

   | 字段 | 填什么 |
   |---|---|
   | **Note** | `MioNo` |
   | **Expiration** | `90 days` |

3. **勾选 `repo`**（就勾最上面那个大类的复选框，下面会自动全勾上）
4. 滑到最下面，点 **Generate token**
5. 页面上会出现一串 **`ghp_` 开头**的字符

   ⚠️ **只显示这一次！** 先复制下来存着。
6. 重跑脚本，弹窗时：
   - Username：你的 GitHub 用户名
   - Password：**粘贴这串 Token**

### ③ Actions 页面报红色 ❌

点进那次失败的运行，看**哪一步是红叉**，然后把**那一步展开**，把红色报错截图发我。

**常见几种**：

| 红色的是哪步 | 大概原因 |
|---|---|
| `配置 CMake` | CMakeLists.txt 有问题（工程配置错误） |
| `编译（Release）` | C++ 代码有编译错误 |
| `拉取 JUCE` | 网络问题，重新跑一次（Actions 页右上 `Re-run jobs`） |

好消息是：**CI 失败不影响你本地**，改完重推就行。

> 新版工作流会在失败时**自动把日志末尾 200 行打出来**，
> 而且 `配置 CMake` 失败时还会把 CMakeLists.txt 全文打出来，
> 所以你看红叉那步的文件就行，不用到处找。

### ④ 一直卡着不动

网络问题。开个加速器/梯子再试。

### ⑤ 提示 `src refspec main does not match any`

意思是「本地还没有 main 分支」，也就是**没有任何文件被提交**。

→ 直接**重跑一次脚本**（新版会自愈）。

> 如果重跑还是不行，把 `MioNoCpp` 文件夹里的 `.git` 和 `.git.bak` 手动删掉，再跑一次。

### ⑥ 报 `this operation must be run in a work tree`

这个报错的含义是「当前目录不是一个可用的 git 工作区」。两种原因：

**原因一：你在 `.git` 文件夹里面双击了脚本**

→ 关掉窗口，退到上一级，在 **`MioNoCpp` 文件夹里**双击 `push-to-github.bat`。

**原因二：文件夹里有个坏的 `.git`**

判断方法：看 `MioNoCpp` 里有没有 `CMakeLists.txt`、`Source`、`.github` 这三样。

- **有** → 目录结构正常，让新版脚本自己修
- **没有，只有一堆零散文件** → 解压歪了，**重新解压一次**，注意选「解压到 MioNoCpp\」

> 好消息：**新版脚本会自己处理这种情况**。
> 它检测到不可用的 `.git` 会先备份成 `.git.bak` 再重建，你直接重跑一次就行。

### ⑦ 我改了代码，怎么重新出 exe？

把新的源码文件覆盖掉旧的，**再双击一次脚本**就行了。
GitHub 会自动重新编译，还是去 Actions 页面下新的 zip。

### ⑧ 提示「网络连不上」/ 一直转圈

先自己在浏览器里打开 **https://github.com** 试试。

- 打得开 → 多试一次脚本，可能是临时抽风
- 打不开 → 是网络问题，开个加速器/梯子再跑脚本

> 脚本推代码走的是 **https 443 端口**，跟浏览器同一套网络。
> 浏览器能开 GitHub，脚本基本就能推。

### ⑨ 报 `no argument after keyword` / `bad configuration options`

这是 `~/.ssh/config` 文件**开头多了 BOM 字符**导致的（老版本 `setup-ssh.bat` 的 bug）。

现在这个版本的 `setup-ssh.bat` 已经修好了：它写配置时只写纯 ASCII，
写完还会自动用 PowerShell 清一遍 BOM 和乱码。**重新跑一次 `setup-ssh.bat` 即可。**

如果你想手动确认，可以在命令行运行：

```
ssh -T github-miono
```

- 返回 `Permission denied (publickey)` → 配置是好的，只是公钥还没加到 GitHub
- 返回 `no argument after keyword` → 配置文件有问题，重跑 `setup-ssh.bat`

### ⑩ 加了公钥还是连不上 / 公钥怎么都加不上去

按顺序检查：

1. 公钥**整行**都粘了吗？必须是 `ssh-ed25519 AAAA....` 开头、
   `miono@local` 结尾的**一整行**，中间不能断行
2. 是不是粘了两遍？GitHub 会报 `Key is already in use`
3. 网页打不开的话，用 `add-key-api.bat`（见第 2.5 步的招 B）

---

## 对比一下两个版本

| | Python 版（已有） | **C++ 版（这个）** |
|---|---|---|
| 包大小 | 47 MB | **约 10~15 MB** |
| 内存占用 | 141 MB | **约 40~70 MB** |
| 启动速度 | 1~3 秒 | **0.3~1 秒** |
| ASIO | 支持 | 支持 |
| VST3 插件 | ❌ 不支持 | ✅ **支持** |
| 怎么拿到 | 我已经发你了 | 走这套流程自己编 |

两个版本**互不冲突**，Python 版你照常能用。
