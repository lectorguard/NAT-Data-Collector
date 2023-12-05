#include "ConnectionReader.h"
#include "chrono"
#include "Application/Application.h"


void ConnectionReader::Activate(Application* app)
{
}

bool ConnectionReader::Start()
{
	if (current == ConnectionReaderStep::Idle)
	{
		current = ConnectionReaderStep::StartReadConnection;
		return true;
	}
	return false;
}


void ConnectionReader::Update(class Application* app)
{
	switch (current)
	{
	case ConnectionReaderStep::Idle:
		break;
	case ConnectionReaderStep::StartReadConnection:
	{
		const auto newConType = ReadConnectionType(app->android_state);
		shared::ConnectionType oldValue;
		do 
		{
			oldValue = connection_type.load();
		} 
		while (!connection_type.compare_exchange_strong(oldValue, newConType));
		current = ConnectionReaderStep::StartWait;
		break;
	}
	case ConnectionReaderStep::StartWait:
	{
		wait_timer.ExpiresFromNow(std::chrono::milliseconds(1000));
		current = ConnectionReaderStep::UpdateWait;
		break;
	}
	case ConnectionReaderStep::UpdateWait:
	{
		if (wait_timer.HasExpired())
		{
			wait_timer.SetActive(false);
			current = ConnectionReaderStep::StartReadConnection;
		}
		break;
	}
	default:
		break;
	}
}

inline shared::ConnectionType ConnectionReader::ReadConnectionType(android_app* native_app)
{
	if (!native_app)
	{
		return shared::ConnectionType::NOT_CONNECTED;
	}

	jint lResult;

	JavaVM* lJavaVM = native_app->activity->vm;
	JNIEnv* lJNIEnv = native_app->activity->env;

	JavaVMAttachArgs lJavaVMAttachArgs;
	lJavaVMAttachArgs.version = JNI_VERSION_1_6;
	lJavaVMAttachArgs.name = "NativeThread";
	lJavaVMAttachArgs.group = NULL;


	// Calls the following JAVA code
	// ConnectivityManager cm = (ConnectivityManager)context.getSystemService(Context.CONNECTIVITY_SERVICE);
	// NetworkInfo info = cm.getActiveNetworkInfo();
	// if (info == null || !info.isConnected())
	// 	return "-"; // not connected
	// if (info.getType() == ConnectivityManager.TYPE_WIFI)
	// 	return "WIFI";
	// if (info.getType() == ConnectivityManager.TYPE_MOBILE) {
	// 	int networkType = info.getSubtype();

	shared::ConnectionType connection_type = shared::ConnectionType::NOT_CONNECTED;
	for (;;)
	{
		lResult = lJavaVM->AttachCurrentThread(&lJNIEnv, &lJavaVMAttachArgs);
		if (lResult == JNI_ERR)
			break;

		// Retrieves NativeActivity.
		jobject lNativeActivity = native_app->activity->clazz;

		// Retrieves Context.INPUT_METHOD_SERVICE.
		jclass ClassContext = lJNIEnv->FindClass("android/content/Context");

		jmethodID getSystemServiceMethod = lJNIEnv->GetMethodID(ClassContext, "getSystemService", "(Ljava/lang/String;)Ljava/lang/Object;");
		jstring connectivityService = lJNIEnv->NewStringUTF("connectivity");
		jobject connectivityManagerObj = lJNIEnv->CallObjectMethod(lNativeActivity, getSystemServiceMethod, connectivityService);

		// Get the active network info from ConnectivityManager
		if (connectivityManagerObj == NULL)
			break;

		jclass connectivityManagerClass = lJNIEnv->GetObjectClass(connectivityManagerObj);
		jmethodID getActiveNetworkInfoMethod = lJNIEnv->GetMethodID(connectivityManagerClass, "getActiveNetworkInfo", "()Landroid/net/NetworkInfo;");
		jobject networkInfoObj = lJNIEnv->CallObjectMethod(connectivityManagerObj, getActiveNetworkInfoMethod);
		if (networkInfoObj == NULL)
			break;

		jclass networkInfoClass = lJNIEnv->GetObjectClass(networkInfoObj);

		// Check if the network is connected
		jmethodID isConnectedMethod = lJNIEnv->GetMethodID(networkInfoClass, "isConnected", "()Z");
		jboolean isConnected = lJNIEnv->CallBooleanMethod(networkInfoObj, isConnectedMethod);

		if (!isConnected)
			break;

		// see enum stuff : https://developer.android.com/reference/android/net/NetworkInfo#getType()
		jmethodID getTypeMethod = lJNIEnv->GetMethodID(networkInfoClass, "getType", "()I");
		jint networkType = lJNIEnv->CallIntMethod(networkInfoObj, getTypeMethod);

		shared::ConnectionType temp_connect_type = static_cast<shared::ConnectionType>((uint16_t)networkType);
		if (temp_connect_type == shared::ConnectionType::MOBILE)
		{
			jmethodID getSubtypeMethod = lJNIEnv->GetMethodID(networkInfoClass, "getSubtype", "()I");
			jint networkSubtype = lJNIEnv->CallIntMethod(networkInfoObj, getSubtypeMethod);
			connection_type = static_cast<shared::ConnectionType>((uint16_t)networkSubtype * 10);
		}
		else
		{
			connection_type = temp_connect_type;
		}
		break;
	}
	lJavaVM->DetachCurrentThread();
	return connection_type;
}
