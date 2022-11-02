#include "Components/Renderer.h"
#include "Application/Application.h"

void Renderer::Activate(class Application* app)
{
	app->AndroidCommandEvent.Subscribe([this](struct android_app* state, int32_t cmd) { OnAndroidEvent(state, cmd); });
	app->AndroidShutdownEvent.Subscribe([this](struct android_app* state) { AndroidShutdown(state); });
	app->UpdateEvent.Subscribe([this](Application* app) {DrawFrame(app); });
}

void Renderer::OnAndroidEvent(struct android_app* app, int32_t cmd)
{
	switch (cmd)
	{
	case APP_CMD_INIT_WINDOW:
	{
		// The window is being shown, get it ready.
		if (app->window != nullptr)
		{
			InitDisplay(app);
			DrawFrame((class Application*)app->userData);
		}
		break;
	}
	case APP_CMD_TERM_WINDOW:
	{
		// The window is being hidden or closed, clean it up.
		AndroidShutdown(app);
		break;
	}
	case APP_CMD_LOST_FOCUS:
	{
		_animating = 0;
		DrawFrame((class Application*)app->userData);
		break;
	}
	default:
		break;
	}
}

void Renderer::InitDisplay(struct android_app* app)
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
		LOGI("OpenGL Info: %s", info);
	}
	// Initialize GL state.
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);
	glEnable(GL_CULL_FACE);
	glShadeModel(GL_SMOOTH);
	glDisable(GL_DEPTH_TEST);
}

void Renderer::AndroidShutdown(struct android_app* app)
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

void Renderer::DrawFrame(class Application* app)
{
	if (_animating)
	{
		if (_display == nullptr) {
			// No display.
			return;
		}

		InputManager& inputManager = app->_components.Get<InputManager>();
		// Just fill the screen with a color.
		glClearColor(inputManager.x / (float)_width, inputManager.angle, inputManager.y / (float)_height, 1);
		glClear(GL_COLOR_BUFFER_BIT);

		eglSwapBuffers(_display, _surface);
	}
}

