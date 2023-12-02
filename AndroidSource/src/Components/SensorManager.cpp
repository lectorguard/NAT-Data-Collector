#include "SensorManager.h"
#include "Application/Application.h"



void SensorManager::Activate(class Application* app)
{
	app->AndroidStartEvent.Subscribe([this](Application* app) {OnAppStart(app); });
	app->AndroidCommandEvent.Subscribe([this](struct android_app* state, int32_t cmd) { OnAndroidEvent(state, cmd); });
}

void SensorManager::OnAppStart(Application* app)
{
#if __ANDROID_API__ >= 26
	_sensorManager = ASensorManager_getInstanceForPackage(GetPackageName(app->android_state));
#else
	_sensorManager = ASensorManager_getInstance();
#endif
	_accelerometerSensor = ASensorManager_getDefaultSensor(
		_sensorManager,
		ASENSOR_TYPE_ACCELEROMETER);
	_sensorEventQueue = ASensorManager_createEventQueue(
		_sensorManager,
		app->android_state->looper, LOOPER_ID_USER,
		nullptr, nullptr);
}

void SensorManager::OnAndroidEvent(struct android_app* app, int32_t cmd)
{
	switch (cmd)
	{
	case APP_CMD_GAINED_FOCUS:
	{
		// When our app gains focus, we start monitoring the accelerometer.
		if (_accelerometerSensor != nullptr)
		{
			ASensorEventQueue_enableSensor(_sensorEventQueue, _accelerometerSensor);
			// We'd like to get 60 events per second (in us).
			ASensorEventQueue_setEventRate(_sensorEventQueue, _accelerometerSensor,
				(1000L / 60) * 1000);
		}
		break;
	}
	case APP_CMD_LOST_FOCUS:
	{
		// When our app loses focus, we stop monitoring the accelerometer.
		// This is to avoid consuming battery while not being used.
		if (_accelerometerSensor != nullptr)
		{
			ASensorEventQueue_disableSensor(_sensorEventQueue, _accelerometerSensor);
		}
		break;
	}
	default:
		break;
	}
}

void SensorManager::ProcessSensorData(int ident)
{
	// If a sensor has data, process it now.
	if (ident == LOOPER_ID_USER) {
		if (_accelerometerSensor != nullptr)
		{
			ASensorEvent event;
			while (ASensorEventQueue_getEvents(_sensorEventQueue,
				&event, 1) > 0)
			{
				//LOGI("accelerometer: x=%f y=%f z=%f",
				//     event.acceleration.x, event.acceleration.y,
				//     event.acceleration.z);
			}
		}
	}
}

const char* SensorManager::GetPackageName(struct android_app* app)
{
	JNIEnv* env = nullptr;
	app->activity->vm->AttachCurrentThread(&env, nullptr);

	jclass android_content_Context = env->GetObjectClass(app->activity->clazz);
	jmethodID midGetPackageName = env->GetMethodID(android_content_Context,
		"getPackageName",
		"()Ljava/lang/String;");
	auto packageName = (jstring)env->CallObjectMethod(app->activity->clazz,
		midGetPackageName);

	return env->GetStringUTFChars(packageName, nullptr);
}
