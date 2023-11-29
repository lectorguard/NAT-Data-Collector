#pragma once

#include "nlohmann/json.hpp"
#include "zstd.h"



namespace shared
{
	inline const std::vector<uint8_t> JsonToMsgPack(const nlohmann::json& j)
	{
		return nlohmann::json::to_msgpack(j);
	}

	inline const std::vector<uint8_t> MsgPackToJson(const std::vector<uint8_t>& j)
	{
		return nlohmann::json::from_msgpack(j);
	}

	inline const std::vector<uint8_t> compressZstd(const std::vector<uint8_t>& data, int compression_level = 1)
	{
		std::vector<uint8_t> buffer;
		size_t const cBuffSize = ZSTD_compressBound(data.size());
		buffer.reserve(cBuffSize);
		size_t const cSize = ZSTD_compress((void*)data.data(), data.size(), buffer.data(), buffer.size(), compression_level);
		return buffer;
	}

	inline const std::vector<uint8_t> decompressZstd(const std::vector<uint8_t>& data)
	{
		std::vector<uint8_t> buffer{};
		unsigned long long const rSize = ZSTD_getFrameContentSize(data.data(), data.size());
		buffer.reserve(rSize);
		size_t const dSize = ZSTD_decompress(buffer.data(), buffer.size(), data.data(), data.size());
		return buffer;
	}
}


