#pragma once
#include "asio.hpp"
#include "SharedProtocol.h"
#include "JSerializer.h"
#include "SharedHelpers.h"
#include "imgui.h"
#include "jni.h"
#include "UI/StyleConstants.h"
#include "android_native_app_glue.h"
#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <unistd.h>

namespace utilities
{
	using namespace shared;

	inline std::optional<uint32_t> GetMaxOpenFiles()
	{
		FILE* file = std::fopen("/proc/self/limits", "r");
		if (!file) {
			return std::nullopt;
		}

		char line[256];
		std::optional<uint32_t> max_fds = std::nullopt;

		while (std::fgets(line, sizeof(line), file)) {
			if (std::strstr(line, "Max open files")) {
				char* token = std::strtok(line, " ");
				for (int i = 0; i < 3 && token != nullptr; ++i) {
					token = std::strtok(nullptr, " ");
				}
				if (token) {
					max_fds = std::atoi(token);
				}
				break;
			}
		}
		std::fclose(file);
		return max_fds;
	}

	inline std::optional<uint32_t> GetCurrentOpenFiles()
	{
		uint32_t count = 0;
		DIR* dirp = opendir("/proc/self/fd");
		if (dirp) {
			struct dirent* entry;
			while ((entry = readdir(dirp)) != nullptr) {
				if (entry->d_type == DT_LNK) {
					count++;
				}
			}
			closedir(dirp);
			return count;
		}
		return std::nullopt;
	}

	inline std::optional<uint32_t> GetNumberOfRemainingFiles()
	{
		if (auto curr = GetCurrentOpenFiles())
		{
			if (auto all = GetMaxOpenFiles())
			{
				return *all - *curr;
			}
		}
		return std::nullopt;
	}

	// Only produces warning
	inline Error ClampIfNotEnoughFiles(uint16_t& sample_size)
	{
		auto max_files = GetMaxOpenFiles();
		auto curr_files = GetCurrentOpenFiles();
		if (max_files && curr_files)
		{
			uint32_t rem95 = (*max_files - *curr_files) * 0.95f;
			if (sample_size > rem95)
			{
				Error err{ ErrorType::WARNING,
					{
						"Device runs out of file descriptors for sockets",
						"Requested size : " + std::to_string(sample_size) + " Remaining files : " + std::to_string(rem95),
						"Overriding requested size with " + std::to_string(rem95)
					}
				};
				sample_size = rem95;
				return err;
			}
		}
		return Error{ ErrorType::OK };
	}

	inline void ShutdownTCPSocket(asio::ip::tcp::socket& socket, asio::error_code& ec)
	{
		socket.shutdown(asio::ip::tcp::socket::shutdown_both, ec);
		// Ignore this case
		if (ec == asio::error::not_connected)
		{
			ec = asio::error_code{};
		}
		if (ec)
			return;
		socket.close(ec);
	}

	template<typename T>
	inline std::optional<T> TryGetFuture(std::future<T>& fut)
	{
		if (!fut.valid())
			return std::nullopt;
		if (fut.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready)
			return std::nullopt;
		return fut.get();
	}

	inline std::optional<std::string> GetAndroidID(struct android_app* native_app)
	{
		jint lResult;

		JavaVM* lJavaVM = native_app->activity->vm;
		JNIEnv* lJNIEnv = native_app->activity->env;

		JavaVMAttachArgs lJavaVMAttachArgs;
		lJavaVMAttachArgs.version = JNI_VERSION_1_6;
		lJavaVMAttachArgs.name = "NativeThread";
		lJavaVMAttachArgs.group = NULL;

		std::string result_id{};
		for (;;)
		{
			lResult = lJavaVM->AttachCurrentThread(&lJNIEnv, &lJavaVMAttachArgs);
			if (lResult == JNI_ERR) {
				break;
			}

			// Retrieves NativeActivity.
			jobject lNativeActivity = native_app->activity->clazz;

			// Retrieves Context.INPUT_METHOD_SERVICE.
			jclass ClassContext = lJNIEnv->FindClass("android/content/Context");

			jmethodID getContentResolverMID = lJNIEnv->GetMethodID(ClassContext, "getContentResolver", "()Landroid/content/ContentResolver;");
			if (getContentResolverMID == NULL) {
				break;
			}

			jobject contentResolverObj = lJNIEnv->CallObjectMethod(lNativeActivity, getContentResolverMID);
			if (contentResolverObj == NULL) {
				break;
			}

			jclass settingsSecureClass = lJNIEnv->FindClass("android/provider/Settings$Secure");
			if (settingsSecureClass == NULL) {
				break;
			}

			jmethodID getStringMID = lJNIEnv->GetStaticMethodID(settingsSecureClass, "getString", "(Landroid/content/ContentResolver;Ljava/lang/String;)Ljava/lang/String;");
			if (getStringMID == NULL) {
				break;
			}

			jstring idStr = (jstring)lJNIEnv->NewStringUTF("android_id");
			// Do relevant error checking, and then:
			jstring androidId = (jstring)lJNIEnv->CallStaticObjectMethod(settingsSecureClass, getStringMID, contentResolverObj, idStr);
			if (androidId == NULL) {
				break;
			}
			jboolean isCopy;
			const char* id = lJNIEnv->GetStringUTFChars(androidId, &isCopy);
			result_id = std::string(id, strlen(id));
			break;
		}
		lJavaVM->DetachCurrentThread();
		if (result_id.empty())
		{
			return std::nullopt;
		}
		else
		{
			return result_id;
		}
		return std::nullopt;
	}


	inline shared::Error WriteToClipboard(struct android_app* native_app, const std::string& label, const std::string& content)
	{
		using namespace shared;

		jint lResult;

		JavaVM* lJavaVM = native_app->activity->vm;
		JNIEnv* lJNIEnv = native_app->activity->env;

		JavaVMAttachArgs lJavaVMAttachArgs;
		lJavaVMAttachArgs.version = JNI_VERSION_1_6;
		lJavaVMAttachArgs.name = "NativeThread";
		lJavaVMAttachArgs.group = NULL;


		lResult = lJavaVM->AttachCurrentThread(&lJNIEnv, &lJavaVMAttachArgs);
		if (lResult == JNI_ERR)
		{
			return Error(ErrorType::ERROR, { "Failed to attach to JNI thread" });
		}

		Error result{ErrorType::OK};
		{
			// Retrieves NativeActivity.
			jobject lNativeActivity = native_app->activity->clazz;

			// Retrieves Context.INPUT_METHOD_SERVICE.
			jclass ClassContext = lJNIEnv->FindClass("android/content/Context");

			jmethodID getSystemServiceMethod = lJNIEnv->GetMethodID(ClassContext, "getSystemService", "(Ljava/lang/String;)Ljava/lang/Object;");
			jstring clipboardService = lJNIEnv->NewStringUTF("clipboard");
			jobject clipboardManagerObj = lJNIEnv->CallObjectMethod(lNativeActivity, getSystemServiceMethod, clipboardService);

			if (clipboardManagerObj != NULL) {
				// Get the ClipData class
				jclass clipDataClass = lJNIEnv->FindClass("android/content/ClipData");
				jmethodID newPlainTextMethod = lJNIEnv->GetStaticMethodID(clipDataClass, "newPlainText", "(Ljava/lang/CharSequence;Ljava/lang/CharSequence;)Landroid/content/ClipData;");

				// Create a new ClipData with the text
				jstring j_label = lJNIEnv->NewStringUTF(label.c_str());
				jstring j_content = lJNIEnv->NewStringUTF(content.c_str());
				jobject clipDataObj = lJNIEnv->CallStaticObjectMethod(clipDataClass, newPlainTextMethod, j_label, j_content);

				// Get the ClipboardManager's setPrimaryClip method
				jclass clipboardManagerClass = lJNIEnv->GetObjectClass(clipboardManagerObj);
				jmethodID setPrimaryClipMethod = lJNIEnv->GetMethodID(clipboardManagerClass, "setPrimaryClip", "(Landroid/content/ClipData;)V");

				// Set the ClipData to the clipboard
				lJNIEnv->CallVoidMethod(clipboardManagerObj, setPrimaryClipMethod, clipDataObj);

				// Release local references
				lJNIEnv->DeleteLocalRef(clipDataObj);
				lJNIEnv->DeleteLocalRef(clipDataClass);
				lJNIEnv->DeleteLocalRef(j_content);
				lJNIEnv->DeleteLocalRef(j_label);
				lJNIEnv->DeleteLocalRef(clipboardManagerObj);
			}

			// Release local references
			lJNIEnv->DeleteLocalRef(clipboardService);
			lJNIEnv->DeleteLocalRef(ClassContext);
			//
		}
		// Always detach
		lJavaVM->DetachCurrentThread();
		return result;
	}

	// https://developer.android.com/training/scheduling/wakelock
	inline shared::Error ActivateWakeLock(struct android_app* native_app, std::vector<std::string> powerManagerFlags, long duration, std::string context)
	{
		using namespace shared;

		jint lResult;

		JavaVM* lJavaVM = native_app->activity->vm;
		JNIEnv* lJNIEnv = native_app->activity->env;

		JavaVMAttachArgs lJavaVMAttachArgs;
		lJavaVMAttachArgs.version = JNI_VERSION_1_6;
		lJavaVMAttachArgs.name = "NativeThread";
		lJavaVMAttachArgs.group = NULL;


		lResult = lJavaVM->AttachCurrentThread(&lJNIEnv, &lJavaVMAttachArgs);
		if (lResult == JNI_ERR)
		{
			return Error(ErrorType::ERROR, { "Failed to attach to JNI thread" });
		}

		// Retrieves NativeActivity.
		jobject lNativeActivity = native_app->activity->clazz;

		// Retrieves Context.INPUT_METHOD_SERVICE.
		jclass ClassContext = lJNIEnv->FindClass("android/content/Context");

		jstring powerService = lJNIEnv->NewStringUTF("power");
		jobject powerManager =
			lJNIEnv->CallObjectMethod(lNativeActivity, lJNIEnv->GetMethodID(ClassContext, "getSystemService", "(Ljava/lang/String;)Ljava/lang/Object;"), powerService);
		lJNIEnv->DeleteLocalRef(powerService);

		// Get PowerManager.PARTIAL_WAKE_LOCK
		jclass powerManagerClass = lJNIEnv->GetObjectClass(powerManager);


		jint combinedFlags = 0;
		for (auto& flag : powerManagerFlags)
		{
			jfieldID partialWakeLockField = lJNIEnv->GetStaticFieldID(powerManagerClass, flag.c_str(), "I");
			jint partialWakeLockValue = lJNIEnv->GetStaticIntField(powerManagerClass, partialWakeLockField);
			combinedFlags |= partialWakeLockValue;
		}

		// Create wake lock
		jstring wakeLockTag = lJNIEnv->NewStringUTF(context.c_str());
		jmethodID newWakeLockMethod =
			lJNIEnv->GetMethodID(powerManagerClass, "newWakeLock", "(ILjava/lang/String;)Landroid/os/PowerManager$WakeLock;");
		jobject wakeLock = lJNIEnv->CallObjectMethod(powerManager, newWakeLockMethod, combinedFlags, wakeLockTag);
		lJNIEnv->DeleteLocalRef(wakeLockTag);

		// Acquire the wake lock
		jmethodID acquireMethod = lJNIEnv->GetMethodID(lJNIEnv->GetObjectClass(wakeLock), "acquire", "()V");
		lJNIEnv->CallVoidMethod(wakeLock, acquireMethod, duration);

		lJavaVM->DetachCurrentThread();
		return Error{ErrorType::OK};
	}

	inline bool IsScreenActive(struct android_app* native_app)
	{
		jint lResult;

		JavaVM* lJavaVM = native_app->activity->vm;
		JNIEnv* lJNIEnv = native_app->activity->env;

		JavaVMAttachArgs lJavaVMAttachArgs;
		lJavaVMAttachArgs.version = JNI_VERSION_1_6;
		lJavaVMAttachArgs.name = "NativeThread";
		lJavaVMAttachArgs.group = NULL;


		lResult = lJavaVM->AttachCurrentThread(&lJNIEnv, &lJavaVMAttachArgs);
		if (lResult == JNI_ERR)
		{
			// in doubt screen is off
			return false;
		}

		// Retrieves NativeActivity.
		jobject lNativeActivity = native_app->activity->clazz;

		// Retrieves Context.INPUT_METHOD_SERVICE.
		jclass ClassContext = lJNIEnv->FindClass("android/content/Context");

		jmethodID getSystemServiceMethod = lJNIEnv->GetMethodID(ClassContext, "getSystemService", "(Ljava/lang/String;)Ljava/lang/Object;");
		jobject powerManagerObj = lJNIEnv->CallObjectMethod(lNativeActivity, getSystemServiceMethod, lJNIEnv->NewStringUTF("power"));

		// Get the isScreenOn method from PowerManager class
		jclass powerManagerClass = lJNIEnv->FindClass("android/os/PowerManager");
		jmethodID isScreenOnMethod = lJNIEnv->GetMethodID(powerManagerClass, "isScreenOn", "()Z");

		// Call isScreenOn method
		const bool isActive = lJNIEnv->CallBooleanMethod(powerManagerObj, isScreenOnMethod);
		lJNIEnv->DeleteLocalRef(powerManagerObj);
		lJavaVM->DetachCurrentThread();
		return isActive;
	}

	inline bool StyledButton(const char* label, ImVec4& currentColor, bool selected = false, bool marked = false)
	{
		ImGui::PushStyleColor(ImGuiCol_Button, currentColor);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, currentColor);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, currentColor);

		bool released = ImGui::Button(label);
		if (ImGui::IsItemActive()) currentColor = ButtonColors::Pressed;
		else if (selected) currentColor = ButtonColors::Selected;
		else if (marked) currentColor = ButtonColors::Marked;
		else currentColor = ButtonColors::Unselected;

		ImGui::PopStyleColor(3);
		return released;
	}
}

