#pragma once
#include <vector>
#include <functional>
#include <algorithm>
#include "JSerializer.h"
#include "chrono"
#include "asio.hpp"

namespace shared::helper
{


		template<typename P, typename R>
		inline std::vector<R> mapVector(const std::vector<P>& vec, std::function<R(const P&)> lambda)
		{
			std::vector<R> buffer{ vec.size() };
			std::transform(vec.begin(), vec.end(), buffer.begin(), lambda);
			return buffer;
		}

		inline const std::vector<std::string> JserErrorToString(const std::vector<jser::JSerError>& errors)
		{
			return mapVector<jser::JSerError, std::string>(errors, [](auto jser_err) {return jser_err.Message; });
		}

// 
// 		inline shared::ServerResponse HandleAsioError(const asio::error_code& ec, const std::string& context)
// 		{
// 			using namespace shared;
// 			if (ec == asio::error::eof)
// 			{
// 				return ServerResponse::Error({ "Connection Rejected during Transaction Attempt : Context : " + context });
// 			}
// 			else if (ec)
// 			{
// 				return ServerResponse::Error({ "Server Connection Error " + ec.message() });
// 			}
// 			return ServerResponse::OK();
// 		}


		inline uint16_t CreateTimeStampOnlyMS()
		{
			auto now = std::chrono::system_clock::now();
			auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
			return (uint16_t)(ms % 10'000);
		}

		inline std::string TimeStampToString(const std::chrono::system_clock::time_point& tp)
		{
			auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(tp.time_since_epoch()).count();

			// Convert milliseconds to seconds
			std::time_t t = ms / 1000;

			// Get the remaining milliseconds
			int milliseconds = ms % 1000;
			auto local_time = *std::localtime(&t);

			// Write to string
			std::ostringstream oss;
			oss << std::put_time(&local_time, "%d-%m-%Y %H-%M-%S");
			oss << "-" << std::setfill('0') << std::setw(3) << milliseconds;
			return oss.str();
		}

		inline std::string CreateTimeStampNow()
		{
			auto now = std::chrono::system_clock::now();
			return TimeStampToString(now);
		}

		inline uint64_t GetDeltaTimeMSNow(const std::chrono::system_clock::time_point& start)
		{
			return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() -  start).count();
		}

		template<class... Lambdas>
		struct Overloaded : Lambdas...
		{
			using Lambdas::operator()...;
		};

		template<class... Lambdas>
		Overloaded(Lambdas...) -> Overloaded<Lambdas...>;


		struct ScopeTimer
		{
		public:

			ScopeTimer(std::string_view operation_name = "") :
				operation_name(operation_name)
			{
				begin = std::chrono::system_clock::now();
			}

			~ScopeTimer()
			{
				auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - begin).count();
				std::cout << operation_name << " took : " << duration << " ms" << std::endl;
			}

		private:
			std::chrono::system_clock::time_point begin;
			std::string_view operation_name;
		};
}

