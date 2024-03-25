#pragma once
#include <EGL/egl.h>
#include <GLES/gl.h>
#include <android_native_app_glue.h>
#include <cassert>
#include <stdexcept>
#include <memory>
#include <string>

enum class ClearColor
{
    BLACK,
    DEFAULT
};

class Renderer
{
public:
    inline static struct ImFont* large_font = nullptr;
    inline static struct ImFont* medium_font = nullptr;
    inline static struct ImFont* small_font = nullptr;
    inline static struct ImFont* very_small_font = nullptr;

    Renderer() {};
    ~Renderer() {};

    void Activate(class Application* app);
    void Deactivate(class Application* app);

    

    void OnAndroidEvent(struct android_app* app, int32_t cmd);
    void InitDisplay(struct android_app* app);

    void InitDisplayDensity(struct android_app* app);
    void StartFrame();
    void EndFrame();

    
    ImFont* CreateFontbySizePixels(float fontSizePixels);
    void SetImguiScale(float scaleValue);
    static float CentimeterToPixel(float fontsize_cm);

    bool CanDraw() const;
    bool IsAnimating() const { return _animating; };

    void SetDarkMode(bool isActive);
    void FlipDarkMode() { SetDarkMode(!IsDarkMode()); }
    bool IsDarkMode() { return clear_type == ClearColor::BLACK; }
    
private:
    ClearColor clear_type = ClearColor::DEFAULT;

	inline static int32_t displayDensity = 0;
	// Density independant pixels
	inline static int32_t DIPX = 0;
	// Density independant pixels
	inline static int32_t DIPY = 0;

    EGLDisplay _display = NULL;
    EGLSurface _surface = NULL;
    EGLContext _context = NULL;
    std::string g_IniFilename{};

    int _animating = 0;
    void InitEGL(struct android_app* app);
    void InitImGUI(struct android_app* app);

};