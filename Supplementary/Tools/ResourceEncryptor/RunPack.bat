@echo off
set /p param1=请输入要加密的文件夹名称:
set /p param2=请输入要生成的加密资源包名称:
set /p param3=请输入加密秘钥:
start "" "pack.exe" "%param1%" "%param2%" "%param3%"