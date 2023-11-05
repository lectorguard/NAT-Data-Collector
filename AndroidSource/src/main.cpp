#include "Application/Application.h"
#include "imgui.h"

/**
 * This is the main entry point of a native application that is using
 * android_native_app_glue.  It runs in its own thread, with its own
 * event loop for receiving input events and doing other things.
 */
void android_main(struct android_app* state) 
{
    Application app;
	app.run(state);
}
//END_INCLUDE(all)
