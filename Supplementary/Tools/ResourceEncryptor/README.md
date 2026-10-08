# 资源包加密工具 / Resource Pack Cipher Tool

[中文](#中文) | [English](#english)

---

## 中文

把素材文件夹打包成加密资源包，同时生成配套的程序代码。**包和代码必须一起换**，只换其中一个会加载失败。

本工具只提供 exe，不带源码。先到这里下载 `generator.exe` 和 `key_gen.exe`：

https://github.com/handama/FA2sp/tree/master/Supplementary/Tools/ResourceEncryptor

把这两个文件放进同一个文件夹（例如 `D:\pack`），后面的命令都在这个文件夹里执行。在文件夹地址栏输入 `cmd` 回车就打开了命令行。

### 1. 素材怎么放

每个一级子文件夹就是一个资源包，文件夹名就是包名：

```
assets\
├─ data1\            打包后得到 data1.pack
│  ├─ htk.vxl
│  └─ ...
└─ data2\            打包后得到 data2.pack
   ├─ HTNK.vxl
   └─ ...
```

子文件夹里的文件就是包里的文件。`.mix` 会自动展开，不用先手动解包。文件名大小写不敏感。

### 2. 三步

**① 生成种子**

```
key_gen.exe
```

屏幕会打印一串随机种子。记下来单独保管，**不要提交到仓库，也不要随包发出去**。

**② 打包并生成代码**

只打资源包：

```
generator.exe --seed <种子> --assets <素材根目录> --out-dir <包输出目录>
```

连代码一起直接写进工程：

```
generator.exe --seed <种子> --assets <素材根目录> --out-dir <包输出目录> --cpp-out "<工程目录>\FA2sp\Ext\CLoading\Body.Encryption.cpp" --header-out "<工程目录>\FA2sp\Ext\CLoading\Body.Cipher.h"
```

例如：

```
generator.exe --seed acca4483...2fdc --assets D:\myassets --out-dir D:\mypacks --cpp-out "D:\FA2spHDM\FA2sp\Ext\CLoading\Body.Encryption.cpp" --header-out "D:\FA2spHDM\FA2sp\Ext\CLoading\Body.Cipher.h"
```

产出三个东西：`<包输出目录>\<包名>.pack`、`Body.Encryption.cpp`、`Body.Cipher.h`。后面两个如果不指定位置，就写在 **exe 所在的文件夹**里。

跑完最后会提示这两个文件该放哪：

```
[NEXT] copy <实现> -> FA2sp\Ext\CLoading\Body.Encryption.cpp, then rebuild
[NEXT] copy <头文件> -> FA2sp\Ext\CLoading\Body.Cipher.h, then rebuild
```

如果已经写进工程了，这里会变成 `... is already in place; rebuild`。

常用参数：

| 参数 | 说明 |
|---|---|
| `--seed` | 发行种子，必填 |
| `--assets` | 素材根目录，必填 |
| `--out-dir` | 资源包输出目录，默认 exe 所在文件夹 |
| `--cpp-out` | 生成的代码写到哪，默认 exe 所在文件夹的 `Body.Encryption.cpp` |
| `--header-out` | 生成的接口头写到哪，默认 exe 所在文件夹的 `Body.Cipher.h` |
| `--force` | 目标文件不像本工具的产物时，强行覆盖 |

**③ 编译**

把 `Body.Encryption.cpp` 和 `Body.Cipher.h` 放进 `FA2sp\Ext\CLoading\`（或用上面两个参数直接生成到那里），然后重编 FA2sp。

### 3. 让 FA2 用上这些包

把 `.pack` 放到你要的位置，在 `FAData.ini` 里登记：

```ini
[ExtraPackages]
; true = 相对 FA2 安装目录，false = 相对 MIX 所在目录（游戏目录）
data1.pack=false
data2.pack=false
```

启动后看日志：`[MixLoader][Package] <路径> loaded.` 表示加载成功；`failed!` 表示这个包和当前程序不是同一次生成的。

### 4. 以后再加新包

要给已经发布出去的版本加一个新包（比如 `data3`），按原来的三步再做一遍就行：素材根里**保留所有旧包**、把新包放进去，用**同一个种子**重新生成一次。

对已经装过旧版的用户来说，**他们只要下载新包和新程序，旧包不用重下。**

- 每次都要**包和程序一起换**。
- **换了种子**（或者本来就是另一版发行）就不适用了，所有包都得重新下。

### 记住三件事

1. **种子是唯一的秘密。** 丢了就做不出同一版；泄露了这一版就能被还原。
2. **改了素材、或者要发新版本，就重跑一次生成器，并且把包和程序一起换掉。** 多个包要在同一次运行里一起生成。
3. 报错 `refusing to overwrite ...`，说明那个目标文件不是本工具的产物；确认没问题就加 `--force`。

---

## English

Pack your asset folders into encrypted resource packs and generate the matching program code in one go. **The packs and the code must be replaced together** — swapping only one of them makes loading fail.

This tool ships as exe files only, without source. Download `generator.exe` and `key_gen.exe` from here:

https://github.com/handama/FA2sp/tree/master/Supplementary/Tools/ResourceEncryptor

Put both files into one folder (say `D:\pack`) and run the commands below from that folder. Typing `cmd` in the folder's address bar opens a command prompt there.

### 1. How to lay out the assets

Every first level sub folder is one resource pack, and the folder name is the pack name:

```
assets\
├─ data1\            packs into data1.pack
│  ├─ htk.vxl
│  └─ ...
└─ data2\            packs into data2.pack
   ├─ HTNK.vxl
   └─ ...
```

The files inside a sub folder are the files inside that pack. `.mix` files are expanded for you, no need to unpack them first. Names are case insensitive.

### 2. Three steps

**① Generate a seed**

```
key_gen.exe
```

It prints a random seed. Write it down and keep it separately: **do not commit it, and do not ship it with the packs.**

**② Pack and generate the code**

Packs only:

```
generator.exe --seed <seed> --assets <asset root> --out-dir <pack output dir>
```

Packs plus the code, written straight into the project:

```
generator.exe --seed <seed> --assets <asset root> --out-dir <pack output dir> --cpp-out "<project>\FA2sp\Ext\CLoading\Body.Encryption.cpp" --header-out "<project>\FA2sp\Ext\CLoading\Body.Cipher.h"
```

For example:

```
generator.exe --seed acca4483...2fdc --assets D:\myassets --out-dir D:\mypacks --cpp-out "D:\FA2spHDM\FA2sp\Ext\CLoading\Body.Encryption.cpp" --header-out "D:\FA2spHDM\FA2sp\Ext\CLoading\Body.Cipher.h"
```

You get three things: `<pack output dir>\<pack name>.pack`, `Body.Encryption.cpp` and `Body.Cipher.h`. If you do not give the last two a location, they are written next to the exe.

The end of the output tells you where those two files have to go:

```
[NEXT] copy <implementation> -> FA2sp\Ext\CLoading\Body.Encryption.cpp, then rebuild
[NEXT] copy <header> -> FA2sp\Ext\CLoading\Body.Cipher.h, then rebuild
```

If they were written into the project already, this becomes `... is already in place; rebuild`.

Common options:

| Option | Meaning |
|---|---|
| `--seed` | release seed, required |
| `--assets` | asset root, required |
| `--out-dir` | where the packs go, defaults to the exe's folder |
| `--cpp-out` | where the generated code goes, defaults to `Body.Encryption.cpp` next to the exe |
| `--header-out` | where the generated header goes, defaults to `Body.Cipher.h` next to the exe |
| `--force` | overwrite the target even if it is not a file this tool wrote |

**③ Build**

Put `Body.Encryption.cpp` and `Body.Cipher.h` into `FA2sp\Ext\CLoading\` (or generate them there directly with the two options above), then rebuild FA2sp.

### 3. Make FA2 use the packs

Put the `.pack` files where you want them and register them in `FAData.ini`:

```ini
[ExtraPackages]
; true = relative to the FA2 install directory, false = relative to the MIX directory (the game directory)
data1.pack=false
data2.pack=false
```

Watch the log at startup: `[MixLoader][Package] <path> loaded.` means it loaded; `failed!` means that pack and the current program do not come from the same run.

### 4. Adding another pack later

To add a new pack to a release you have already shipped (say `data3`), just do the three steps again: keep **all the old packs** in the asset root, drop the new one in, and run the generator with the **same seed**.

For players who already installed the previous version, the good news is: **they only need the new pack and the new program; the old packs stay as they are.**

- Always replace the packs and the program together.
- This only holds for the **same seed**. A different seed means everyone downloads everything again.

### Three things to remember

1. **The seed is the only secret.** Lose it and you cannot rebuild the same release; leak it and this release can be recovered.
2. **After changing assets, or when you ship a new release, run the generator again and replace the packs and the program together.** All packs must come from one run.
3. If you get `refusing to overwrite ...`, that target is not a file this tool wrote. Add `--force` once you are sure.
