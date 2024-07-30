#pragma once
#include "Components/UDPCollectTask.h"

namespace CollectConfig
{
	inline const shared::CollectingConfig::Stage candidates
	{
		{7, {
			10'000, // sample size
			0,		// sample rate
			10'000,	// start echo service
			1000,	// echo services
			0,		// local port
			true,	// close sockets early
			true	// use shutdown condition
	}}
	};

	inline const shared::CollectingConfig::Stage intersect
	{
		{7, {10000, 10,10'000, 1000, 0, true, true}}
	};

	inline const shared::CollectingConfig::Stage traverse
	{ {
		{10'000, 0, 10'000, 1, 0, false, false},
		{10'000, 3, 10'000, 1, 0, false, false},
		{10'000, 6, 10'000, 1, 0, false, false},
		{10'000, 9, 10'000, 1, 0, false, false},
		{10'000, 12, 10'000, 1, 0, false, false},
		{10'000, 15, 10'000, 1, 0, false, false},
		{10'000, 18, 10'000, 1, 0, false, false},
	} };

	inline const shared::CollectingConfig::DynamicStage dynTraverse
	{
		/*k_multiple*/ 3,
		/*max_sockets*/ 10'000,
		/*start_echo_service*/ 10'000,
		/*num_echo_services*/ 1,
		/*local_port*/ 0,
		/*close_sockets_early*/ false,
		/*use_shutdown_condition*/ false
	};

	inline const shared::CollectingConfig config(
		"coll_trash",
		180'000u,
		{
			candidates,
			intersect
		}, 
		{ dynTraverse }
	);
}


namespace TraversalConfig
{
	inline const std::string coll_traversal_name = "traverseResult";

	namespace AnalyzeConeNAT
	{
		inline const UDPCollectTask::Stage config
		{
			/* remote address */				"",
			/* start port */					10'000,
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
			/* remote address */				"",
			/* start port */					10000,
			/* num port services */				1'000,
			/* local port */					0,
			/* amount of ports */				10'000,
			/* time between requests in ms */	0,
			/* close sockets early */			true,
			/* shutdown condition */			nullptr //is set at later stage
		};

		const UDPCollectTask::Stage duplicates
		{
			/* remote address */				"",
			/* start port */					10000,
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