#include "Components/Renderer.h"
#include "Application/Application.h"
#include "imgui.h"
#include "backends/imgui_impl_android.h"
#include "backends/imgui_impl_opengl3.h"

void Renderer::Activate(class Application* app)
{
	app->AndroidCommandEvent.Subscribe([this](struct android_app* state, int32_t cmd) { OnAndroidEvent(state, cmd); });
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
		}
		break;
	}
	case APP_CMD_TERM_WINDOW:
	{
		// Shutdown rendering part completely
		Deactivate(nullptr);
		ANativeWindow_release(app->window);
		break;
	}
	case APP_CMD_LOST_FOCUS:
	{
		_animating = 0;
		break;
	}
	default:
		break;
	}
}

void Renderer::InitDisplay(struct android_app* app)
{
	InitEGL(app);
	InitImGUI(app);
	_animating = 1;
	ImGui::StyleColorsDark();
	SetFontSize(70.0f);
	SetImguiScale(7.0f);
}

void Renderer::Deactivate(class Application* app)
{
	if (_display != EGL_NO_DISPLAY)
	{
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplAndroid_Shutdown();
		ImGui::DestroyContext();

		eglMakeCurrent(_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

		if (_context != EGL_NO_CONTEXT)
			eglDestroyContext(_display, _context);

		if (_surface != EGL_NO_SURFACE)
			eglDestroySurface(_display, _surface);

		eglTerminate(_display);
	}
	_animating = 0;
	_display = EGL_NO_DISPLAY;
	_context = EGL_NO_CONTEXT;
	_surface = EGL_NO_SURFACE;
}

void Renderer::StartFrame()
{
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplAndroid_NewFrame();
	ImGui::NewFrame();
}

void Renderer::EndFrame()
{
	ImGuiIO& io = ImGui::GetIO();
	ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

	ImGui::Render();
	glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
	glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
	glClear(GL_COLOR_BUFFER_BIT);
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	eglSwapBuffers(_display, _surface);
}

void Renderer::SetFontSize(float fontSizePixels)
{
	if (!CanDraw())
	{
		LOGW("Can not cahnge font size, _display not initialized no context");
		return;
	}
	ImGuiIO& io = ImGui::GetIO();
	ImFontConfig font_cfg;
	font_cfg.SizePixels = fontSizePixels;
	io.Fonts->AddFontDefault(&font_cfg);
}

void Renderer::SetImguiScale(float scaleValue)
{
	ImGui::GetStyle().ScaleAllSizes(scaleValue);
}

bool Renderer::CanDraw() const
{
	return _display != nullptr && _animating;
}

void Renderer::InitEGL(struct android_app* app)
{
	// Initialize window
	ANativeWindow_acquire(app->window);

	_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
	if (_display == EGL_NO_DISPLAY)
		LOGW("eglGetDisplay(EGL_DEFAULT_DISPLAY) returned EGL_NO_DISPLAY");

	if (eglInitialize(_display, 0, 0) != EGL_TRUE)
		LOGW("eglInitialize() returned with an error");

	const EGLint egl_attributes[] = { EGL_BLUE_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_RED_SIZE, 8, EGL_DEPTH_SIZE, 24, EGL_SURFACE_TYPE, EGL_WINDOW_BIT, EGL_NONE };
	EGLint num_configs = 0;
	if (eglChooseConfig(_display, egl_attributes, nullptr, 0, &num_configs) != EGL_TRUE)
		LOGW("eglChooseConfig() returned with an error");
	if (num_configs == 0)
		LOGW("eglChooseConfig() returned 0 matching config");

	// Get the first matching config
	EGLConfig egl_config;
	eglChooseConfig(_display, egl_attributes, &egl_config, 1, &num_configs);
	EGLint egl_format;
	eglGetConfigAttrib(_display, egl_config, EGL_NATIVE_VISUAL_ID, &egl_format);
	ANativeWindow_setBuffersGeometry(app->window, 0, 0, egl_format);

	const EGLint egl_context_attributes[] = { EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE };
	_context = eglCreateContext(_display, egl_config, EGL_NO_CONTEXT, egl_context_attributes);

	if (_context == EGL_NO_CONTEXT)
		LOGW("eglCreateContext() returned EGL_NO_CONTEXT");

	_surface = eglCreateWindowSurface(_display, egl_config, app->window, nullptr);
	eglMakeCurrent(_display, _surface, _surface, _context);
}

void Renderer::InitImGUI(struct android_app* app)
{
	if (_display != nullptr)
	{
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();

		g_IniFilename = std::string(app->activity->internalDataPath) + "/imgui.ini";
		io.IniFilename = g_IniFilename.c_str();;

		// Setup Platform/Renderer backends
		ImGui_ImplAndroid_Init(app->window);
		ImGui_ImplOpenGL3_Init("#version 300 es");
	}
}

