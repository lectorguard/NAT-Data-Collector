#include "Components/InputManager.h"
#include "Application/Application.h"
#include "backends/imgui_impl_android.h"
#include "imgui.h"
#include "CustomCollections/Log.h"

void InputManager::Activate(class Application* app)
{
	app->AndroidStartEvent.Subscribe([this](struct android_app* state) {OnAppStart(state); });
	app->DrawEvent.Subscribe([this](auto ignore) {UpdateSoftKeyboard(); });
}

int32_t InputManager::HandleInput(struct android_app* state, AInputEvent* ev)
{
	// Issue : Backspace key event is unreliable
	// Handle Input is not called every time backspace is pressed (Android Event issue)
	if (AInputEvent_getType(ev) == AINPUT_EVENT_TYPE_KEY && 
		AKeyEvent_getAction(ev) == AKEY_EVENT_ACTION_DOWN)
	{
		const int key = AKeyEvent_getKeyCode(ev);
		const int metaState = AKeyEvent_getMetaState(ev);
		ImGuiIO& io = ImGui::GetIO();
		const int unicode = metaState ?
				GetUnicodeCharacter(state, AKEY_EVENT_ACTION_DOWN, key, metaState) :
				GetUnicodeCharacter(state, AKEY_EVENT_ACTION_DOWN, key, 0);
		io.AddInputCharacter(unicode);
	}
	return ImGui_ImplAndroid_HandleInputEvent(ev);
}

void InputManager::OnAppStart(struct android_app* state)
{
	state->onInputEvent = &InputManager::HandleInput;
	native_app = state;
}

void InputManager::UpdateSoftKeyboard()
{
	ImGuiIO& io = ImGui::GetIO();
	ShowKeyboard(io.WantTextInput);
}


void InputManager::ShowKeyboard(bool newVisibility)
{
	static bool KeyboardVisibilityState = false;
	if (newVisibility == KeyboardVisibilityState)
	{
		return;
	}

	// Attaches the current thread to the JVM.
	jint lResult;
	jint lFlags = 0;

	JavaVM* lJavaVM = native_app->activity->vm;
	JNIEnv* lJNIEnv = native_app->activity->env;

	JavaVMAttachArgs lJavaVMAttachArgs;
	lJavaVMAttachArgs.version = JNI_VERSION_1_6;
	lJavaVMAttachArgs.name = "NativeThread";
	lJavaVMAttachArgs.group = NULL;

	lResult = lJavaVM->AttachCurrentThread(&lJNIEnv, &lJavaVMAttachArgs);
	if (lResult == JNI_ERR) {
		return;
	}

	// Retrieves NativeActivity.
	jobject lNativeActivity = native_app->activity->clazz;
	jclass ClassNativeActivity = lJNIEnv->GetObjectClass(lNativeActivity);

	// Retrieves Context.INPUT_METHOD_SERVICE.
	jclass ClassContext = lJNIEnv->FindClass("android/content/Context");
	jfieldID FieldINPUT_METHOD_SERVICE =
		lJNIEnv->GetStaticFieldID(ClassContext,
			"INPUT_METHOD_SERVICE", "Ljava/lang/String;");
	jobject INPUT_METHOD_SERVICE =
		lJNIEnv->GetStaticObjectField(ClassContext,
			FieldINPUT_METHOD_SERVICE);
	//jniCheck(INPUT_METHOD_SERVICE);

	// Runs getSystemService(Context.INPUT_METHOD_SERVICE).
	jclass ClassInputMethodManager = lJNIEnv->FindClass(
		"android/view/inputmethod/InputMethodManager");
	jmethodID MethodGetSystemService = lJNIEnv->GetMethodID(
		ClassNativeActivity, "getSystemService",
		"(Ljava/lang/String;)Ljava/lang/Object;");
	jobject lInputMethodManager = lJNIEnv->CallObjectMethod(
		lNativeActivity, MethodGetSystemService,
		INPUT_METHOD_SERVICE);

	// Runs getWindow().getDecorView().
	jmethodID MethodGetWindow = lJNIEnv->GetMethodID(
		ClassNativeActivity, "getWindow",
		"()Landroid/view/Window;");
	jobject lWindow = lJNIEnv->CallObjectMethod(lNativeActivity,
		MethodGetWindow);
	jclass ClassWindow = lJNIEnv->FindClass(
		"android/view/Window");
	jmethodID MethodGetDecorView = lJNIEnv->GetMethodID(
		ClassWindow, "getDecorView", "()Landroid/view/View;");
	jobject lDecorView = lJNIEnv->CallObjectMethod(lWindow,
		MethodGetDecorView);

	if (newVisibility) {
		// Runs lInputMethodManager.showSoftInput(...).
		jmethodID MethodShowSoftInput = lJNIEnv->GetMethodID(
			ClassInputMethodManager, "showSoftInput",
			"(Landroid/view/View;I)Z");
		jboolean lResult = lJNIEnv->CallBooleanMethod(
			lInputMethodManager, MethodShowSoftInput,
			lDecorView, lFlags);

		KeyboardVisibilityState = true;
	}
	else 
	{
		// Runs lWindow.getViewToken()
		jclass ClassView = lJNIEnv->FindClass(
			"android/view/View");
		jmethodID MethodGetWindowToken = lJNIEnv->GetMethodID(
			ClassView, "getWindowToken", "()Landroid/os/IBinder;");
		jobject lBinder = lJNIEnv->CallObjectMethod(lDecorView,
			MethodGetWindowToken);

		// lInputMethodManager.hideSoftInput(...).
		jmethodID MethodHideSoftInput = lJNIEnv->GetMethodID(
			ClassInputMethodManager, "hideSoftInputFromWindow",
			"(Landroid/os/IBinder;I)Z");
		jboolean lRes = lJNIEnv->CallBooleanMethod(
			lInputMethodManager, MethodHideSoftInput,
			lBinder, lFlags);

		KeyboardVisibilityState = false;
	}

	// Finished with the JVM.
	lJavaVM->DetachCurrentThread();
}

int InputManager::GetUnicodeCharacter(android_app* native_app, int eventType, int keyCode, int metaState)
{
	JavaVM* javaVM = native_app->activity->vm;
	JNIEnv* jniEnv = native_app->activity->env;

	JavaVMAttachArgs attachArgs;
	attachArgs.version = JNI_VERSION_1_6;
	attachArgs.name = "NativeThread";
	attachArgs.group = NULL;

	jint result = javaVM->AttachCurrentThread(&jniEnv, &attachArgs);
	if (result == JNI_ERR)
	{
		return 0;
	}

	jclass class_key_event = jniEnv->FindClass("android/view/KeyEvent");
	int unicodeKey;

	if (metaState == 0)
	{
		jmethodID method_get_unicode_char = jniEnv->GetMethodID(class_key_event, "getUnicodeChar", "()I");
		jmethodID eventConstructor = jniEnv->GetMethodID(class_key_event, "<init>", "(II)V");
		jobject eventObj = jniEnv->NewObject(class_key_event, eventConstructor, eventType, keyCode);

		unicodeKey = jniEnv->CallIntMethod(eventObj, method_get_unicode_char);
	}

	else
	{
		jmethodID method_get_unicode_char = jniEnv->GetMethodID(class_key_event, "getUnicodeChar", "(I)I");
		jmethodID eventConstructor = jniEnv->GetMethodID(class_key_event, "<init>", "(II)V");
		jobject eventObj = jniEnv->NewObject(class_key_event, eventConstructor, eventType, keyCode);

		unicodeKey = jniEnv->CallIntMethod(eventObj, method_get_unicode_char, metaState);
	}

	javaVM->DetachCurrentThread();
	return unicodeKey;
}