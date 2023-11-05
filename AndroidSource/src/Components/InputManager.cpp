#include "Components/InputManager.h"
#include "Application/Application.h"
#include "backends/imgui_impl_android.h"

void InputManager::Activate(class Application* app)
{
	app->AndroidStartEvent.Subscribe([this](struct android_app* state) {OnAppStart(state); });
	app->UpdateEvent.Subscribe([this](auto ignore) {Update(); });
}

int32_t InputManager::HandleInput(struct android_app* state, AInputEvent* ev)
{
	const int result =  ImGui_ImplAndroid_HandleInputEvent(ev);

	switch (AInputEvent_getType(ev))
	{
	case AINPUT_EVENT_TYPE_KEY:
	{
		switch (AKeyEvent_getAction(ev))
		{
		case AKEY_EVENT_ACTION_DOWN:
		{
			int key = AKeyEvent_getKeyCode(ev);
			int metaState = AKeyEvent_getMetaState(ev);
			int unicode = metaState ?
				GetUnicodeCharacter(state, AKEY_EVENT_ACTION_DOWN, key, metaState) :
				GetUnicodeCharacter(state, AKEY_EVENT_ACTION_DOWN, key, 0);
			LOGW("Unicode key is: %d", unicode);
			break;
		}
		default:
			break;
		}
		break;
	}
	default:
		break;
	}
	return result;
}

void InputManager::OnAppStart(struct android_app* state)
{
	state->onInputEvent = &InputManager::HandleInput;
	native_app = state;
	simple.ExpiresFromNow(std::chrono::milliseconds(5000));
}

void InputManager::ShowKeyboard(bool isVisible)
{
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

	if (isVisible) {
		// Runs lInputMethodManager.showSoftInput(...).
		jmethodID MethodShowSoftInput = lJNIEnv->GetMethodID(
			ClassInputMethodManager, "showSoftInput",
			"(Landroid/view/View;I)Z");
		jboolean lResult = lJNIEnv->CallBooleanMethod(
			lInputMethodManager, MethodShowSoftInput,
			lDecorView, lFlags);
	}
	else {
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
	}

	// Finished with the JVM.
	lJavaVM->DetachCurrentThread();
}

void InputManager::Update()
{
	if (simple.HasExpired())
	{
		ShowKeyboard(current);
		current = !current;
		simple.ExpiresFromNow(std::chrono::milliseconds(5000));
		LOGW("Open Soft Keyboard");
	}
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