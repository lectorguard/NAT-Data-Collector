#pragma once
#include "asio.hpp"
#include "SharedProtocol.h"
#include "JSerializer.h"

namespace utilities
{
	inline void ShutdownTCPSocket(asio::ip::tcp::socket& socket)
	{
		socket.shutdown(asio::ip::tcp::socket::shutdown_both);
		socket.close();
	}

	inline shared::ServerResponse HandleAsioError(asio::error_code ec, const std::string& context)
	{
		using namespace shared;
		if (ec == asio::error::eof)
		{
			return ServerResponse::Error({ "Connection Rejected during Transaction Attempt : Context : " + context });
		}
		else if (ec)
		{
			return ServerResponse::Error({ "Server Connection Error " + ec.message() });
		}
		return ServerResponse::OK();
	}

	inline shared::ServerResponse HandleJserError(const std::vector<jser::JSerError>& jserErrors, const std::string& context)
	{
		using namespace shared;
		if (jserErrors.size() > 0)
		{
			std::vector<std::string> stringErrors;
			std::transform(jserErrors.begin(), jserErrors.end(), stringErrors.begin(), [](auto e) {return e.Message; });
			stringErrors.push_back(context);
			return ServerResponse::Error({ "(De)Serialization error - context : " });
		}
		return ServerResponse::OK();
	}

	template<typename T>
	inline std::optional<T> TryGetFuture(std::future<T>& fut)
	{
		if (!fut.valid())
			return std::nullopt;
		if (fut.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready)
			return std::nullopt;
		return fut.get();
	}
}

