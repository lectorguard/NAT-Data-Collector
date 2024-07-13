#pragma once
#include "imgui.h"
#include "vector"
#include "android/log.h"
#include "SharedProtocol.h"
#include "Utilities/NetworkHelpers.h"
#include "mutex"
#include "Components/GlobalConstants.h"

#define FORCE_LOG(x, ...) ((void)__android_log_buf_print(LOG_ID_CRASH, ANDROID_LOG_ERROR, "native-activity", x, __VA_ARGS__))

enum LogWarn
{
	Log_INFO = 0,
	Log_WARNING,
	Log_ERROR
};



struct Log
{
public:
	struct Entry
	{
		ImGuiTextBuffer buf;
		LogWarn level;
	};

	static void Info(const char* fmt, ...)
	{
		va_list args;
		va_start(args, fmt);
		Entry helper;
		helper.buf.appendfv(fmt, args);
		helper.level = Log_INFO;
		va_end(args);
		Log_Internal(helper);
	}

	static void Warning(const char* fmt, ...)
	{
		va_list args;
		va_start(args, fmt);
		Entry helper;
		helper.buf.appendfv(fmt, args);
		helper.level = Log_WARNING;
		va_end(args);
		Log_Internal(helper);
	}

	static void Error(const char* fmt, ...)
	{
		va_list args;
		va_start(args, fmt);
		Entry helper;
		helper.buf.appendfv(fmt, args);
		helper.level = Log_ERROR;
		va_end(args);
		Log_Internal(helper);
	}

	static void HandleResponse(const shared::DataPackage& pkg, const std::string& context)
	{
		return HandleResponse(pkg.error, context);
	}

	static void HandleResponse(const shared::Error& err, const std::string& context)
	{
		switch (err.error)
		{
		case shared::ErrorType::OK:
		case shared::ErrorType::ANSWER:
		{
			if (!context.empty())
			{
				Info("%s : OK", context.c_str());
			}
			for (const auto& msg : err.messages)
			{
				Info("%s", msg.c_str());
			}
			break;
		}
		case shared::ErrorType::WARNING:
		{
			Warning("%s : Produced Warning", context.c_str());
			Warning("---- START TRACE ----");
			for (const auto& msg : err.messages)
			{
				Warning("%s", msg.c_str());
			}
			Warning("----- END TRACE -----");
			break;
		}
		case shared::ErrorType::ERROR:
		{
			Error("%s : FAILED", context.c_str());
			Error("---- START TRACE ----");
			for (const auto& msg : err.messages)
			{
				Error("%s", msg.c_str());
			}
			Error("----- END TRACE -----");
			break;
		}
		default:
			break;
		}
	}

	 static const std::vector<Entry>& GetLog() 
	 {
		 std::scoped_lock lock{ log_mutex };
		 return log_buffer; 
	 }
	 static bool PopScrolltoBottom()
	 {
		 std::scoped_lock lock{ log_mutex };

		 auto last = scrollToBottom;
		 scrollToBottom = false;
		 return last;
	 }

	 static void CopyLogToClipboard(struct android_app* state)
	 {
		 std::stringstream ss;
		 for (auto info : Log::GetLog())
		 {
			 ss << info.buf.c_str() << "\r\n";
		 }
		 const std::string clipboard_content = ss.str();
		 const auto response = utilities::WriteToClipboard(state, "NAT Collector Log", clipboard_content);
		 Log::HandleResponse(response, "Write Log to clipboard");
	 }

private:

	static void Log_Internal(const Entry& helper)
	{
		std::scoped_lock lock{ log_mutex };
		switch (helper.level)
		{
		case Log_INFO:
			((void)__android_log_print(ANDROID_LOG_INFO, "native-activity","%s", helper.buf.begin()));
			break;
		case Log_ERROR:
			((void)__android_log_print(ANDROID_LOG_WARN, "native-activity", "%s", helper.buf.begin()));
			break;
		case Log_WARNING:
			((void)__android_log_print(ANDROID_LOG_ERROR, "native-activity", "%s", helper.buf.begin()));
			break;
		default:
			break;
		}
		if (log_buffer.size() >= AppConfig::max_log_lines)
		{
			// Not very efficient should be a deque instead
			log_buffer.erase(log_buffer.begin());
		}
		log_buffer.emplace_back(helper);
		scrollToBottom = true;
	}

	inline static std::vector<Entry> log_buffer{};
	inline static bool scrollToBottom = false;
	inline static std::mutex log_mutex{};
};
