#pragma once

#include "nlohmann/json.hpp"
#include "zstd.h"



namespace shared
{
	inline const std::vector<uint8_t> compressZstd(const std::vector<uint8_t>& data, int compression_level = 1)
	{
		std::vector<uint8_t> buf;
		size_t const cBuffSize = ZSTD_compressBound(data.size());
		buf.resize(cBuffSize);
		size_t const cSize = ZSTD_compress(buf.data(), buf.size(), data.data(), data.size(), compression_level);
		buf.resize(cSize);
		return buf;
	}

	inline const std::vector<uint8_t> decompressZstd(const std::vector<uint8_t>& data)
	{
		std::vector<uint8_t> buf;
		unsigned long long const rSize = ZSTD_getFrameContentSize(data.data(), data.size());
		buf.resize(rSize);
		size_t const dSize = ZSTD_decompress(buf.data(), buf.size(), data.data(), data.size());
		buf.resize(dSize);
		return buf;
	}

	inline const std::string encodeMsgLength(uint64_t msg_length)
	{
		std::stringstream ss;
		ss << std::setw(MAX_MSG_LENGTH_DECIMALS) << std::setfill('0') << msg_length;
		return ss.str();
	}


}


