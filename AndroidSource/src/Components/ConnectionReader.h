#pragma once

#include "SharedProtocol.h"
#include "SharedTypes.h"
#include "atomic"
#include "CustomCollections/SimpleTimer.h"


enum class ConnectionReaderStep : uint16_t
{
	Idle = 0,
	StartReadConnection,
	StartWait,
	UpdateWait
};

class ConnectionReader
{
public:
	void Activate(class Application* app);
	void Update(class Application* app);
	void Deactivate(class Application* app) {};

	shared::ConnectionType Get() { return connection_type.load(); }
	std::atomic<shared::ConnectionType>& GetAtomicRef() { return connection_type;}
private:
	inline static shared::ConnectionType ReadConnectionType(struct android_app* native_app);

	// State
	ConnectionReaderStep current = ConnectionReaderStep::Idle;
	SimpleTimer wait_timer{};
	std::atomic<shared::ConnectionType> connection_type;
};