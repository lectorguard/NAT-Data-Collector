@echo off
"C:\Program Files (x86)\Android\android-sdk\platform-tools\adb.exe" logcat > logcat.txt && C:\Microsoft\AndroidNDK\android-ndk-r23c\ndk-stack.cmd -sym ..\bin\Android\AndroidSource\Debug -dump logcat.txt
