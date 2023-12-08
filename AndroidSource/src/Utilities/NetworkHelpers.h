#pragma once
#include "asio.hpp"
#include "SharedProtocol.h"
#include "JSerializer.h"
#include "SharedHelpers.h"
#include "imgui.h"
#include "jni.h"
#include "android_native_app_glue.h"

namespace utilities
{
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

	template<typename T>
	inline std::variant<shared::ServerResponse, T> TryGetObjectFromResponse(const shared::ServerResponse::Helper& response)
	{
		if (response.resp_type == shared::ResponseType::ANSWER)
		{
			T toDeserialize;
			std::vector<jser::JSerError> jser_errors;
			toDeserialize.DeserializeObject(response.answer, std::back_inserter(jser_errors));
			if (jser_errors.size() > 0)
			{
				return shared::helper::HandleJserError(jser_errors, "TryGetObjectFromResponse : Error during deserialization of server response");
			}
			return toDeserialize;
		}
		return shared::ServerResponse(response.resp_type, response.messages, nullptr);
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
	}


	inline shared::ServerResponse WriteToClipboard(struct android_app* native_app, const std::string& label, const std::string& content)
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
			return shared::ServerResponse::Error({ "Failed to attach to JNI thread" });
		}

		shared::ServerResponse result = shared::ServerResponse::OK();
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
	inline shared::ServerResponse ActivateWakeLock(struct android_app* native_app, std::vector<std::string> powerManagerFlags, long duration, std::string context)
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
			return shared::ServerResponse::Error({ "Failed to attach to JNI thread" });
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
		return shared::ServerResponse::OK();
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

	inline bool StyledButton(const char* label, ImVec4& currentColor, bool isSelected = false)
	{
		const ImVec4 Pressed = { 163 / 255.0f, 163 / 255.0f, 163 / 255.0f,1.0f };
		const ImVec4 Selected = { 88 / 255.0f, 88 / 255.0f, 88 / 255.0f, 1.0f };
		const ImVec4 Unselected = { 46 / 255.0f, 46 / 255.0f, 46 / 255.0f, 1.0f };

		ImGui::PushStyleColor(ImGuiCol_Button, currentColor);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, currentColor);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, currentColor);
		bool released = ImGui::Button(label);
		if (ImGui::IsItemActive()) currentColor = Pressed;
		else if (isSelected) currentColor = Selected;
		else currentColor = Unselected;
		ImGui::PopStyleColor(3);
		return released;
	}
}

