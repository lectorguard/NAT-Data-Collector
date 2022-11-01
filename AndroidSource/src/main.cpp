/*
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

//BEGIN_INCLUDE(all)
#include <initializer_list>
#include <memory>
#include <cstdlib>
#include <cstring>
#include <jni.h>
#include <cerrno>
#include <cassert>

#include <EGL/egl.h>
#include <GLES/gl.h>

#include <android/sensor.h>
#include <android/log.h>
#include <android_native_app_glue.h>
#include "TCPClient.h"
#include <thread>
#include <variant>

#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "native-activity", __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, "native-activity", __VA_ARGS__))

class SensorManager
{
public:
	SensorManager() {};
	~SensorManager() {};

	void OnAppStart(struct android_app* state)
    {
		#if __ANDROID_API__ >= 26
			_sensorManager = ASensorManager_getInstanceForPackage(GetPackageName(state));
		#else
			_sensorManager = ASensorManager_getInstance();
		#endif
		_accelerometerSensor = ASensorManager_getDefaultSensor(
			_sensorManager,
			ASENSOR_TYPE_ACCELEROMETER);
		_sensorEventQueue = ASensorManager_createEventQueue(
			_sensorManager,
			state->looper, LOOPER_ID_USER,
			nullptr, nullptr);
    }

    void OnAndroidEvent(struct android_app* app, int32_t cmd)
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
    void ProcessSensorData(int ident)
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


private:
	const char* GetPackageName(struct android_app* app)
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
	ASensorManager* _sensorManager = nullptr;
	const ASensor* _accelerometerSensor = nullptr;
	ASensorEventQueue* _sensorEventQueue = nullptr;
};
class Renderer
{
public:
	Renderer() {};
	~Renderer() {};

    void OnAndroidEvent(struct android_app* app, int32_t cmd)
    {
		switch (cmd)
		{
		case APP_CMD_INIT_WINDOW:
		{
			// The window is being shown, get it ready.
			if (app->window != nullptr)
			{
				InitDisplay(app);
				DrawFrame(1,1,1);
			}
			break;
		}
		case APP_CMD_TERM_WINDOW:
		{
			// The window is being hidden or closed, clean it up.
			ShutdownDisplay(app);
			break;
		}
		case APP_CMD_LOST_FOCUS:
		{
			_animating = 0;
			DrawFrame(1,1,1);
			break;
		}
		default:
			break;
		}
    }
    void InitDisplay(struct android_app* app)
    {
        // initialize OpenGL ES and EGL

		/*
		 * Here specify the attributes of the desired configuration.
		 * Below, we select an EGLConfig with at least 8 bits per color
		 * component compatible with on-screen windows
		 */
        const EGLint attribs[] = {
                EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
                EGL_BLUE_SIZE, 8,
                EGL_GREEN_SIZE, 8,
                EGL_RED_SIZE, 8,
                EGL_NONE
        };
        EGLint w, h, format;
        EGLint numConfigs;
        EGLConfig config = nullptr;
        EGLSurface surface;
        EGLContext context;

        EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);

        eglInitialize(display, nullptr, nullptr);

        /* Here, the application chooses the configuration it desires.
         * find the best match if possible, otherwise use the very first one
         */
        eglChooseConfig(display, attribs, nullptr, 0, &numConfigs);
        std::unique_ptr<EGLConfig[]> supportedConfigs(new EGLConfig[numConfigs]);
        assert(supportedConfigs);
        eglChooseConfig(display, attribs, supportedConfigs.get(), numConfigs, &numConfigs);
        assert(numConfigs);
        auto i = 0;
        for (; i < numConfigs; i++) {
            auto& cfg = supportedConfigs[i];
            EGLint r, g, b, d;
            if (eglGetConfigAttrib(display, cfg, EGL_RED_SIZE, &r) &&
                eglGetConfigAttrib(display, cfg, EGL_GREEN_SIZE, &g) &&
                eglGetConfigAttrib(display, cfg, EGL_BLUE_SIZE, &b) &&
                eglGetConfigAttrib(display, cfg, EGL_DEPTH_SIZE, &d) &&
                r == 8 && g == 8 && b == 8 && d == 0) {

                config = supportedConfigs[i];
                break;
            }
        }
        if (i == numConfigs) {
            config = supportedConfigs[0];
        }

        if (config == nullptr) {
            // LOGW("Unable to initialize EGLConfig");
            throw std::runtime_error("Unable to initialize EGLConfig");
        }

        /* EGL_NATIVE_VISUAL_ID is an attribute of the EGLConfig that is
         * guaranteed to be accepted by ANativeWindow_setBuffersGeometry().
         * As soon as we picked a EGLConfig, we can safely reconfigure the
         * ANativeWindow buffers to match, using EGL_NATIVE_VISUAL_ID. */
        eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &format);
        surface = eglCreateWindowSurface(display, config, app->window, nullptr);
        context = eglCreateContext(display, config, nullptr, nullptr);

        if (eglMakeCurrent(display, surface, surface, context) == EGL_FALSE) {
            //LOGW("Unable to eglMakeCurrent");
            throw std::runtime_error("Unable to eglMakeCurrent");
        }

        eglQuerySurface(display, surface, EGL_WIDTH, &w);
        eglQuerySurface(display, surface, EGL_HEIGHT, &h);

        _display = display;
        _context = context;
        _surface = surface;
        _width = w;
        _height = h;

        // Check openGL on the system
        auto opengl_info = { GL_VENDOR, GL_RENDERER, GL_VERSION, GL_EXTENSIONS };
        for (auto name : opengl_info) {
            auto info = glGetString(name);
            // LOGI("OpenGL Info: %s", info);
        }
        // Initialize GL state.
        glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);
        glEnable(GL_CULL_FACE);
        glShadeModel(GL_SMOOTH);
        glDisable(GL_DEPTH_TEST);
    }
    void ShutdownDisplay(struct android_app* app)
	{
		if (_display != EGL_NO_DISPLAY) {
			eglMakeCurrent(_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
			if (_context != EGL_NO_CONTEXT) {
				eglDestroyContext(_display, _context);
			}
			if (_surface != EGL_NO_SURFACE) {
				eglDestroySurface(_display, _surface);
			}
			eglTerminate(_display);
		}
		_animating = 0;
		_display = EGL_NO_DISPLAY;
		_context = EGL_NO_CONTEXT;
		_surface = EGL_NO_SURFACE;
	}
    void DrawFrame(float x, float y, float angle)
    {
		if (_animating)
		{
			if (_display == nullptr) {
				// No display.
				return;
			}

			// Just fill the screen with a color.
			glClearColor(x/_width, angle , y/_height, 1);
			glClear(GL_COLOR_BUFFER_BIT);

			eglSwapBuffers(_display, _surface);
		}
    }

	int _animating = 0;
	EGLDisplay _display = NULL;
	EGLSurface _surface = NULL;
	EGLContext _context = NULL;
	int32_t _width = 0;
	int32_t _height = 0;
};
class InputManager
{
public:
	InputManager() {};
	virtual ~InputManager() {};

    static int32_t HandleInput(struct android_app* state, AInputEvent* event);
	void OnAppStart(struct android_app* state)
	{
		state->onInputEvent = &InputManager::HandleInput;
	}

    void Update()
    {
		angle += .01f;
		if (angle > 1) 
        {
			angle = 0;
		}
    }
	float angle = 0;
	int32_t x = 0;
	int32_t y = 0;
};




template<typename Owner, typename ... Args>
struct Components
{
	using VariantType = typename std::variant<Args ...>;

	Components(Owner* owner)
	{
		using TupleType = typename std::tuple<Args...>;
		std::apply([&](auto&& ... args) { _components = { {decltype(args)(owner) ...} }; }, TupleType());
	}

	template<typename T>
	constexpr T& Get() 
	{
		for (auto& variant : _components)
		{
			if(std::holds_alternative<T>(variant))
			{
				return std::get<T>(variant);
			};
		};
		throw std::runtime_error("Type must exist");
	}

private:
	std::array<VariantType, std::variant_size_v<VariantType>> _components;
};


class Application
{
	template<typename ... args>
	using event_t = std::vector<std::function<void(args ...)>>;
	using ComponentsType = Components<Application, SensorManager, Renderer, InputManager>;

public:
	Application(){};
	~Application() {};

	static void AndroidHandleCommands(struct android_app* state, int32_t cmd)
	{
		auto* app = (Application*)state->userData;
		app->_components.Get<SensorManager>().OnAndroidEvent(state, cmd);
		app->_components.Get<Renderer>().OnAndroidEvent(state, cmd);
	}

	void run(struct android_app* state)
	{
		state->userData = this;
		_components.Get<SensorManager>().OnAppStart(state);
		state->onAppCmd = AndroidHandleCommands;
		state->onInputEvent = _components.Get<InputManager>().HandleInput;

		while (true) {
			// Read all pending events.
			int ident;
			int events;
			struct android_poll_source* source;

			// If not animating, we will block forever waiting for events.
			// If animating, we loop until all events are read, then continue
			// to draw the next frame of animation.
			while ((ident = ALooper_pollAll(_components.Get<Renderer>()._animating ? 0 : -1, nullptr, &events,
				(void**)&source)) >= 0) {

				// Process this event.
				if (source != nullptr) {
					source->process(state, source);
				}

				_components.Get<SensorManager>().ProcessSensorData(ident);

				// Check if we are exiting.
				if (state->destroyRequested != 0) {
					_components.Get<Renderer>().ShutdownDisplay(state);
					return;
				}
			}

			if (_components.Get<Renderer>()._animating) {
				InputManager& inputManager = _components.Get<InputManager>();
				inputManager.Update();
				// Drawing is throttled to the screen update rate, so there
				// is no need to do timing here.
				_components.Get<Renderer>().DrawFrame(inputManager.x, inputManager.y, inputManager.angle);
			}
		}
	}

	ComponentsType _components{ this };
};


int32_t InputManager::HandleInput(struct android_app* state, AInputEvent* event)
{
	if (auto* app = (Application*)state->userData)
	{
		if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION)
		{
			float x = AMotionEvent_getX(event, 0);
			float y = AMotionEvent_getY(event, 0);
			app->_components.Get<Renderer>()._animating = 1;
			app->_components.Get<InputManager>().x = x;
			app->_components.Get<InputManager>().y = y;
			return 1;
		}
	}
	return 0;
}

/**
 * This is the main entry point of a native application that is using
 * android_native_app_glue.  It runs in its own thread, with its own
 * event loop for receiving input events and doing other things.
 */
void android_main(struct android_app* state) {
    Application app;
	app.run(state);
}
//END_INCLUDE(all)
