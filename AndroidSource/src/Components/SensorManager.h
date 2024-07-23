#pragma once

class SensorManager
{
public:
	SensorManager() {};
	~SensorManager() {};

	void Activate(class Application* app);
	void Deactivate(class Application* app) {};

	void OnAppStart(Application* app);

	void OnAndroidEvent(struct android_app* app, int32_t cmd);
	void ProcessSensorData(int ident);
private:
	const char* GetPackageName(struct android_app* app);
	ASensorManager* _sensorManager = nullptr;
	const ASensor* _accelerometerSensor = nullptr;
	ASensorEventQueue* _sensorEventQueue = nullptr;
};
