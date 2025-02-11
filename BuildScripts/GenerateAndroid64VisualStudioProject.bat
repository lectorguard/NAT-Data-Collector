cmake -G "Visual Studio 17 2022" -A ARM64 -B ../bin/Android ^
	-DCMAKE_SYSTEM_NAME=Android ^
	-DCMAKE_SYSTEM_VERSION=29 ^
	-DCMAKE_ANDROID_ARCH_ABI=arm64-v8a  ^
	-DANDROID_ABI=arm64-v8a ^
	-DCMAKE_ANDROID_NDK="C:/Program Files (x86)/Android/android-sdk/ndk-bundle/" ^
	-DCMAKE_ANDROID_STL_TYPE=c++_static ^
	"-DCMAKE_VS_GLOBALS=UseMultiToolTask=true;EnforceProcessCountAcrossBuilds=true" ^
	..

devenv ..\bin\Android\AndroidNativeActivity.sln ^
/Command "File.AddExistingProject %cd%\..\AndroidPackaging\NAT.androidproj" 