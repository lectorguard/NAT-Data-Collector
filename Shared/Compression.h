#pragma once

#include "nlohmann/json.hpp"
#include "zstd.h"
#include "iostream"
#include "sstream"



namespace shared
{
	inline const std::vector<uint8_t> compressZstd(const std::vector<uint8_t>& data, int compression_level = 1)
	{
		std::vector<uint8_t> buf;
		size_t const cBuffSize = ZSTD_compressBound(data.size());
		buf.resize(cBuffSize);
		ZSTD_compress(buf.data(), buf.size(), data.data(), data.size(), compression_level);
		buf.shrink_to_fit();
		return buf;
	}

	inline const std::vector<uint8_t> decompressZstd(const std::vector<uint8_t>& data)
	{
		std::vector<uint8_t> buf;
		unsigned long long const rSize = ZSTD_getFrameContentSize(data.data(), data.size());
		buf.resize(rSize);
		ZSTD_decompress(buf.data(), buf.size(), data.data(), data.size());
		buf.shrink_to_fit();
		return buf;
	}

	inline const std::string encodeMsgLength(uint64_t msg_length)
	{
		std::stringstream ss;
		ss << std::setw(MAX_MSG_LENGTH_DECIMALS) << std::setfill('0') << msg_length;
		return ss.str();
	}

	inline static std::vector<uint8_t> stringToVector(const std::string& msg)
	{
		std::vector<uint8_t> buffer;
		buffer.resize(msg.size());
		std::copy(msg.begin(), msg.end(), buffer.begin());
		return buffer;
	}


}


