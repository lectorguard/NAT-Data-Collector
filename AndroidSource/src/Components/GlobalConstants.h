#pragma once
#include <string>
#include <vector>
#include "Components/UDPCollectTask.h"

namespace GlobServerConst
{
	inline const std::string server_address = SERVER_IP;
	inline const uint16_t server_transaction_port = SERVER_TRANSACTION_TCP_PORT;
	inline const uint16_t server_echo_start_port = SERVER_ECHO_UDP_START_PORT;
	inline const uint16_t server_echo_port_size = SERVER_ECHO_UDP_SERVICES;
	inline const uint32_t global_socket_timeout_ms = SOCKET_TIMEOUT_MS;
	inline const std::string app_version = APP_VERSION;
	inline const uint32_t max_msg_length_decimals = MAX_MSG_LENGTH_DECIMALS;
	
	namespace Mongo
	{
		inline const std::string db_name = MONGO_DB_NAME;
		inline const std::string coll_version_name = MONGO_VERSION_COLL_NAME;
		inline const std::string coll_information_name = MONGO_INFORMATION_COLL_NAME;
		inline const std::string coll_users_name = MONGO_NAT_USERS_COLL_NAME;
	}
}

namespace NATConfig
{
	inline const uint32_t sample_size = 5;
	inline const uint16_t max_delta_progressing_nat = 50;
}

namespace AppConfig
{
	inline const bool random_nat_required = true;
	inline const bool traversal_feature_enabled = true;
	inline const uint32_t max_log_lines = 400;
}

namespace CollectConfig
{
	struct Conf
	{
		uint16_t sample_rate;
		uint16_t sample_size;
	};

	inline const std::string coll_name = "DifferentTraversalFrequencies";
	inline const uint32_t delay_collection_steps_ms = 180'000;

	namespace Candidates
	{
		inline const uint16_t num_echo_services = 1'000;
		inline const uint16_t local_port = 0;
		inline const bool close_sockets_early = true;

		inline const std::vector<Conf> config{ 7, {0,10'000} };
	}

	namespace Duplicates
	{
		inline const uint16_t num_echo_services = 1'000;
		inline const uint16_t local_port = 0;
		inline const bool close_sockets_early = true;

		inline const std::vector<Conf> config{ 7, {10,12'000} };
	}

	namespace Traverse
	{
		inline const uint16_t num_echo_services = 1;
		inline const uint16_t local_port = 0;
		inline const bool close_sockets_early = false;

		inline const std::vector<Conf> config{ {0 , 10'000}, {3 , 10'000},  {6 , 10'000},
											   {9 , 10'000}, {12 , 10'000}, {15 , 10'000}, {18 , 10'000} };
	}
}

namespace TraversalConfig
{
	inline const std::string coll_traversal_name = "traverseResult";

	namespace AnalyzeConeNAT
	{
		inline const UDPCollectTask::Stage config
		{
			/* remote address */				GlobServerConst::server_address,
			/* start port */					GlobServerConst::server_echo_start_port,
			/* num port services */				1,
			/* local port */					7744,
			/* amount of ports */				1,
			/* time between requests in ms */	0,
			/* close sockets early */			true,
			/* shutdown condition */			nullptr
		};
	}

	namespace TraverseConeNAT
	{
		inline const uint16_t traversal_size = 1;
		inline const uint16_t traversal_rate_ms = 0;
		inline const uint32_t deadline_duration_ms = 125'000;
		inline const uint32_t keep_alive_rate_ms = 5'000;
	}

	namespace AnalyzeRandomSym
	{
		inline const UDPCollectTask::Stage candidates
		{
			/* remote address */				GlobServerConst::server_address,
			/* start port */					GlobServerConst::server_echo_start_port,
			/* num port services */				1'000,
			/* local port */					0,
			/* amount of ports */				10'000,
			/* time between requests in ms */	0,
			/* close sockets early */			true,
			/* shutdown condition */			nullptr //is set at later stage
		};

		const UDPCollectTask::Stage duplicates
		{
			/* remote address */				GlobServerConst::server_address,
			/* start port */					GlobServerConst::server_echo_start_port,
			/* num port services */				1'000,
			/* local port */					0,
			/* amount of ports */				12'000,
			/* time between requests in ms */	10,
			/* close sockets early */			true,
			/* shutdown condition */			nullptr
		};

		inline const uint16_t first_n_predict = 250;
	}

	namespace TraverseRandomNAT
	{
		inline const uint16_t traversal_size = 10'000;
		inline const uint16_t traversal_rate_ms = 12;
		inline const uint16_t local_port = 0;
		inline const uint32_t deadline_duration_ms = 125'000;
		inline const uint32_t keep_alive_rate_ms = 0;
	}
}