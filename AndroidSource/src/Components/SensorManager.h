#pragma once
#include <android/sensor.h>
#include <android_native_app_glue.h>

class SensorManager
{
public:
	SensorManager() {};
	~SensorManager() {};

	void Activate(class Application* app);
	void Deactivate(class Application* app) {};

	void OnAppStart(struct android_app* state);

	void OnAndroidEvent(struct android_app* app, int32_t cmd);
	void ProcessSensorData(int ident);
private:
	const char* GetPackageName(struct android_app* app);
	ASensorManager* _sensorManager = nullptr;
	const ASensor* _accelerometerSensor = nullptr;
	ASensorEventQueue* _sensorEventQueue = nullptr;
};
