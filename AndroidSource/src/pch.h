#pragma  once
// Shared
#include "Data/Address.h"
#include "Data/IPMetaData.h"
#include "Data/ScoreboardData.h"
#include "Data/Traversal.h"
#include "Data/WindowData.h"
#include "Compression.h"
#include "SharedHelpers.h"
#include "SharedProtocol.h"
#include "SharedTypes.h"

// Custom Collections
#include "CustomCollections/ComponentManager.h"
#include "CustomCollections/ConcurrentQueue.h"
#include "CustomCollections/Event.h"
#include "CustomCollections/Log.h"
#include "CustomCollections/SimpleTimer.h"

// Libs
#include "asio.hpp"
#include "asio/awaitable.hpp"
#include "asio/use_awaitable.hpp"
#include "asio/deadline_timer.hpp"
#include "stdlib.h"
#include "JSerializer.h"
#include "EGL/egl.h"
#include "GLES/gl.h"
#include "imgui.h"
#include "backends/imgui_impl_android.h"
#include "backends/imgui_impl_opengl3.h"
#include "misc/cpp/imgui_stdlib.h"
#include "nlohmann/json.hpp"


// Android
#include "android_native_app_glue.h"
#include "android/sensor.h"
#include "android/keycodes.h"
#include "android/window.h"
#include "android/asset_manager.h"
#include "dirent.h"

// Networking
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>


