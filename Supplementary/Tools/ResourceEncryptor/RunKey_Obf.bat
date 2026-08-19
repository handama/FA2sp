@echo off
set /p user_input=请输入key_gen.py生成的密钥:
python key_obf.py %user_input%
pause