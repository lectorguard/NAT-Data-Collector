#include "asio.hpp"
#include <iostream>
#include <android_native_app_glue.h>
#include <android/log.h>
#include "array"
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>


#define LOGWARN(...) ((void)__android_log_print(ANDROID_LOG_WARN, "native-activity", __VA_ARGS__))

class TCPClient
{
public:
	// Rule of 5, std::thread implicitly deletes copy constructor
	TCPClient() = default;
	~TCPClient() 
	{
		if (t1.joinable())
		{
			t1.join();
		}
	};
	TCPClient(TCPClient&& other) : t1(std::move(other.t1)) {};
	TCPClient(const TCPClient&) = delete;
	TCPClient& operator=(const TCPClient& other) = delete;
	TCPClient& operator=(TCPClient&& other) 
	{ 
		t1 = std::move(other.t1);
		return *this;
	};


	void Activate(class Application* app);
	void Deactivate(class Application* app);
	static void StartCommunication();

	std::thread t1;
};