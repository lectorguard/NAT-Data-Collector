#pragma once
#include "Components/UDPCollectTask.h"

namespace CollectConfig
{
	inline const shared::CollectingConfig::Stage candidates
	{
		{1, {
			20'000, // sample size
			3,		// sample rate
			10'000,	// start echo service
			1,	// echo services
			0,		// local port
			true,	// close sockets early
			false	// use shutdown condition
	}}
	};

	inline const shared::CollectingConfig::Stage intersect
	{
		{1, {20'000, 3,10'003, 1, 0, true, false}}
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
		/*max rate ms*/		35,
		/*start_echo_service*/ 10'000,
		/*num_echo_services*/ 1,
		/*local_port*/ 0,
		/*close_sockets_early*/ false,
		/*use_shutdown_condition*/ false
	};

	inline const shared::CollectingConfig::OverrideStage overr_stage
	{
		0, // stage index
		{{"isp","Vodafone D2 GmbH"}}, // equal conditions from ClientMetaData and StageConfig
		{{"sample_rate_ms", 14}, {"sample_size", 100}} // overridden config fields
	};

	inline const shared::CollectingConfig config(
		"f4b",
		180'000u,
		{
			candidates,
			intersect
		}, 
		{},
		{ overr_stage }
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