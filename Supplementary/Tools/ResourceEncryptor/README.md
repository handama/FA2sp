# 资源包加密工具 / Resource Pack Cipher Tool

[中文](#中文) | [English](#english)

---

## 中文

本工具把素材文件夹打包成加密资源包，并生成配套的程序代码。资源包和代码必须来自同一次生成，只替换其中一半会加载失败。

工具只提供 exe，不含源码。先下载 `generator.exe` 和 `key_gen.exe`：

https://github.com/handama/FA2sp/tree/master/Supplementary/Tools/ResourceEncryptor

把这两个文件放进同一个文件夹（例如 `D:\pack`），后续命令都在这个文件夹里执行。在该文件夹的地址栏输入 `cmd` 并回车，就能在这里打开命令行。

### 1. 素材目录怎么放

素材根目录下的每个一级子文件夹对应一个资源包，文件夹名就是包名：

```
assets\
├─ data1\            打包后得到 data1.pack
│  ├─ htk.vxl
│  └─ ...
└─ data2\            打包后得到 data2.pack
   ├─ HTNK.vxl
   └─ ...
```

子文件夹里的文件就是包内的文件。`.mix` 会自动展开，不需要提前手动解包。文件名不区分大小写。

### 2. 操作步骤

**① 生成种子**

```
key_gen.exe
```

程序会打印一串随机种子。请把它单独保存好，不要分发。

**② 打包并生成代码**

如果只需要资源包：

```
generator.exe --seed <种子> --assets <素材根目录> --out-dir <包输出目录>
```

如果同时还要把代码写进工程：

```
generator.exe --seed <种子> --assets <素材根目录> --out-dir <包输出目录> --cpp-out "<工程目录>\FA2sp\Ext\CLoading\Body.Encryption.cpp" --header-out "<工程目录>\FA2sp\Ext\CLoading\Body.Cipher.h"
```

例如：

```
generator.exe --seed acca4483...2fdc --assets D:\myassets --out-dir D:\mypacks --cpp-out "D:\FA2spHDM\FA2sp\Ext\CLoading\Body.Encryption.cpp" --header-out "D:\FA2spHDM\FA2sp\Ext\CLoading\Body.Cipher.h"
```

生成器会输出三类文件：`<包输出目录>\<包名>.pack`、`Body.Encryption.cpp` 和 `Body.Cipher.h`。后两个如果没有指定路径，会写到 exe 所在的文件夹里。

运行结束后，程序会提示这两个文件应该放到哪里：

```
[NEXT] copy <实现> -> FA2sp\Ext\CLoading\Body.Encryption.cpp, then rebuild
[NEXT] copy <头文件> -> FA2sp\Ext\CLoading\Body.Cipher.h, then rebuild
```

如果生成时已经直接写到了工程里的目标位置，提示会变成 `... is already in place; rebuild`。

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

把 `Body.Encryption.cpp` 和 `Body.Cipher.h` 放进 `FA2sp\Ext\CLoading\`（或者上一步直接用参数生成到那里），然后重新编译 FA2sp。

### 3. 让 FA2 加载这些包

把 `.pack` 文件放到合适的位置，然后在 `FAData.ini` 里登记：

```ini
[ExtraPackages]
; true = 相对 FA2 安装目录，false = 相对 MIX 所在目录（游戏目录）
data1.pack=false
data2.pack=false
```

启动后查看日志：`[MixLoader][Package] <路径> loaded.` 表示加载成功，`failed!` 表示这个包和当前程序不是同一次生成的。

### 4. 以后新增资源包

如果某个版本已经发布出去，之后想再加一个包（比如 `data3`），按之前的三个步骤再走一遍即可。素材根目录里要保留所有旧包，把新包一起放进去，用同一个种子重新生成。

对已经装过旧版的用户来说，只需要下载新包和新程序，旧包不用重新下载。

- 每次发布都要把资源包和程序一起替换。
- 只有种子没变时才能这样增量替换。换了种子（或者本来就跟上一个版本不是同一次生成），所有包都得重新下载。

### 注意事项

1. 种子是唯一的秘密。丢了就做不出同一版，泄露了这一版的资源就能被还原。
2. 改了素材或者要发新版本，就重新运行一次生成器，并把包和程序一起替换掉。多个包必须在同一次运行里一起生成。
3. 如果报错 `refusing to overwrite ...`，说明那个目标文件不是本工具的产物；确认没问题后加上 `--force`。

---

## English

This tool packs asset folders into encrypted resource packs and generates the matching program code at the same time. The packs and the code must come from the same run; replacing only one of the two makes loading fail.

The tool is distributed as exe files only, without source. Download `generator.exe` and `key_gen.exe` first:

https://github.com/handama/FA2sp/tree/master/Supplementary/Tools/ResourceEncryptor

Put both files in one folder (for example `D:\pack`) and run the commands below from that folder. Typing `cmd` in the folder's address bar opens a command prompt there.

### 1. Asset layout

Each first-level subfolder of the asset root is one resource pack, and the folder name becomes the pack name:

```
assets\
├─ data1\            packs into data1.pack
│  ├─ htk.vxl
│  └─ ...
└─ data2\            packs into data2.pack
   ├─ HTNK.vxl
   └─ ...
```

The files inside a subfolder are the files inside that pack. `.mix` files are expanded automatically, so there is no need to unpack them first. Names are case insensitive.

### 2. Steps

**① Generate a seed**

```
key_gen.exe
```

The program prints a random seed. Keep it somewhere safe and do not distribute it.

**② Pack and generate the code**

If you only need the packs:

```
generator.exe --seed <seed> --assets <asset root> --out-dir <pack output dir>
```

If you also want the code written straight into the project:

```
generator.exe --seed <seed> --assets <asset root> --out-dir <pack output dir> --cpp-out "<project>\FA2sp\Ext\CLoading\Body.Encryption.cpp" --header-out "<project>\FA2sp\Ext\CLoading\Body.Cipher.h"
```

For example:

```
generator.exe --seed acca4483...2fdc --assets D:\myassets --out-dir D:\mypacks --cpp-out "D:\FA2spHDM\FA2sp\Ext\CLoading\Body.Encryption.cpp" --header-out "D:\FA2spHDM\FA2sp\Ext\CLoading\Body.Cipher.h"
```

The generator writes three kinds of files: `<pack output dir>\<pack name>.pack`, `Body.Encryption.cpp` and `Body.Cipher.h`. If you do not give the last two a path, they are written next to the exe.

At the end it prints where those two files belong:

```
[NEXT] copy <implementation> -> FA2sp\Ext\CLoading\Body.Encryption.cpp, then rebuild
[NEXT] copy <header> -> FA2sp\Ext\CLoading\Body.Cipher.h, then rebuild
```

If they were written to the project location directly, this becomes `... is already in place; rebuild`.

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

### 3. Make FA2 load the packs

Put the `.pack` files wherever you like and register them in `FAData.ini`:

```ini
[ExtraPackages]
; true = relative to the FA2 install directory, false = relative to the MIX directory (the game directory)
data1.pack=false
data2.pack=false
```

Watch the log at startup: `[MixLoader][Package] <path> loaded.` means the pack loaded, `failed!` means that pack and the current program do not come from the same run.

### 4. Adding a pack later

If you have already shipped a release and want to add another pack (say `data3`), go through the same three steps again. Keep all the old packs in the asset root, drop the new one in, and run the generator with the same seed.

For players who already installed the previous version, only the new pack and the new program are needed; the old packs do not have to be downloaded again.

- Always replace the packs and the program together.
- This only works while the seed stays the same. A different seed (or a release that simply was not generated together with the previous one) means everyone downloads everything again.

### Notes

1. The seed is the only secret. Lose it and you cannot build the same release again; leak it and the assets of this release can be recovered.
2. After changing assets, or when you ship a new release, run the generator again and replace the packs and the program together. All packs must be generated in one run.
3. If you get `refusing to overwrite ...`, that target is not a file this tool wrote. Add `--force` once you are sure.
