cmake -G "Visual Studio 17 2022" -A ARM64 -B ../bin/Android ^
	-DCMAKE_SYSTEM_NAME=Android ^
	-DCMAKE_SYSTEM_VERSION=24	^
	-DCMAKE_ANDROID_ARCH_ABI=arm64-v8a  ^
	-DANDROID_ABI=arm64-v8a ^
	-DCMAKE_ANDROID_NDK=C:/Microsoft/AndroidNDK/android-ndk-r23c ^
	-DCMAKE_ANDROID_STL_TYPE=c++_static ..

devenv ..\bin\Android\AndroidNativeActivity.sln ^
/Command "File.AddExistingProject %cd%\..\AndroidPackaging\NAT.androidproj" 