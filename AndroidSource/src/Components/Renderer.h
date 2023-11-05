#pragma once
#include <EGL/egl.h>
#include <GLES/gl.h>
#include <android_native_app_glue.h>
#include <cassert>
#include <stdexcept>
#include <memory>
#include <string>

class Renderer
{
public:
    Renderer() {};
    ~Renderer() {};

    void Activate(class Application* app);
    void Deactivate(class Application* app) {};

    void OnAndroidEvent(struct android_app* app, int32_t cmd);
    void InitDisplay(struct android_app* app);
    void AndroidShutdown(struct android_app* app);

    void StartFrame();
    void EndFrame();

    void SetFontSize(float fontSizePixels);
    void SetImguiScale(float scaleValue);

    bool CanDraw() const;
    bool IsAnimating() const { return _animating; };
private:
    EGLDisplay _display = NULL;
    EGLSurface _surface = NULL;
    EGLContext _context = NULL;
    std::string g_IniFilename{};

    int _animating = 0;
    void InitEGL(struct android_app* app);
    void InitImGUI(struct android_app* app);

};