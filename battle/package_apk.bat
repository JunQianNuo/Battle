@echo off
REM ========================================
REM  Battle APK Package Script
REM ========================================

REM Set Android SDK environment
set ANDROID_HOME=C:\Users\Nuo\AppData\Local\Android\Sdk
set ANDROID_SDK_ROOT=C:\Users\Nuo\AppData\Local\Android\Sdk
set NDKROOT=C:\Users\Nuo\AppData\Local\Android\Sdk\ndk\27.2.12479018
set ANDROID_NDK_ROOT=C:\Users\Nuo\AppData\Local\Android\Sdk\ndk\27.2.12479018
set NDK_ROOT=C:\Users\Nuo\AppData\Local\Android\Sdk\ndk\27.2.12479018
set JAVA_HOME=D:\Android Studio\jbr

echo ========================================
echo  Environment:
echo    ANDROID_HOME=%ANDROID_HOME%
echo    NDKROOT=%NDKROOT%
echo    JAVA_HOME=%JAVA_HOME%
echo ========================================

echo.
echo Starting packaging...
echo.

cd /d D:\PROJECT\battle

call "D:\UE_5.8\Engine\Build\BatchFiles\RunUAT.bat" BuildCookRun ^
  -project="D:\PROJECT\battle\battle.uproject" ^
  -platform=Android ^
  -clientconfig=Development ^
  -cook ^
  -stage ^
  -pak ^
  -archive ^
  -archivedirectory="D:\PROJECT\battle\android" ^
  -cookflavor=ASTC ^
  -build ^
  -package ^
  -iostore ^
  -compressed ^
  -utf8output

echo.
echo Packaging complete. Exit code: %ERRORLEVEL%
pause
