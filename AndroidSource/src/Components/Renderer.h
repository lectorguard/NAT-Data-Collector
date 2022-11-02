#pragma once
#include <EGL/egl.h>
#include <GLES/gl.h>
#include <android_native_app_glue.h>
#include <cassert>
#include <stdexcept>
#include <memory>

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
    void DrawFrame(class Application* app);

    int _animating = 0;
    EGLDisplay _display = NULL;
    EGLSurface _surface = NULL;
    EGLContext _context = NULL;
    int32_t _width = 0;
    int32_t _height = 0;
};