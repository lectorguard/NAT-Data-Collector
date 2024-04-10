#pragma once

#include "JSerializer.h"
#include "type_traits"
#include "SharedTypes.h"
#include <variant>
#include "sstream"
#include <memory>
#include "SharedHelpers.h"
#include <Compression.h>

CREATE_DEFAULT_JSER_MANAGER_TYPE(SerializeManagerType);

namespace shared
{
	struct Error : public jser::JSerializable
	{
		ErrorType error = ErrorType::OK;
		std::vector<std::string> messages{};

		Error() {};
		Error(ErrorType e, std::vector<std::string> msg = {}) :
			error(e), messages(msg)
		{};

		jser::JserChunkAppender AddItem() override
		{
			return JSerializable::AddItem().Append(JSER_ADD(SerializeManagerType, error, messages));
		}

		template<ErrorType e>
		const bool Is()
		{
			return e == error;
		}

		operator bool() const
		{
			return error == ErrorType::ERROR;
		}
	};

	struct DataPackage : public jser::JSerializable
	{
		nlohmann::json data;
		nlohmann::json meta_data;
		Transaction transaction;
		Error error;

		DataPackage() {};

		jser::JserChunkAppender AddItem() override
		{
			return JSerializable::AddItem().Append(JSER_ADD(SerializeManagerType, error, data, meta_data, transaction));
		}

		template<typename T>
		T Get(MetaDataField field)
		{
			return meta_data[meta_data_to_string.at(field)];
		}

		template<typename T>
		Error Get(T& object)
		{
			std::vector<jser::JSerError> jser_errors;
			object.DeserializeObject(data, std::back_inserter(jser_errors));
			if (jser_errors.size() > 0)
			{
				return Error(ErrorType::ERROR, helper::JserErrorToString(jser_errors));
			}
			return Error(ErrorType::OK);
		}

		template<ErrorType E>
		static DataPackage Create(std::vector<std::string> messages = {})
		{
			DataPackage pkg{};
			pkg.error = Error(E, messages);
			return pkg;
		}

		static DataPackage Create(const Error& err)
		{
			DataPackage pkg{};
			pkg.error = err;
			return pkg;
		}

		static DataPackage Create(jser::JSerializable* data)
		{
			using namespace shared;
			if (!data)
			{
				return Create<ErrorType::ERROR>({ "passed data to create package is null" });
			}

			DataPackage pkg{};
			std::vector<jser::JSerError> jser_errors;
			pkg.data = data->SerializeObjectJson(std::back_inserter(jser_errors));
			if (jser_errors.size() > 0)
			{
				return Create<ErrorType::ERROR>(helper::JserErrorToString(jser_errors));
			}
			else
			{
				pkg.error = Error(ErrorType::OK);
			}
			return pkg;
		}

		template<typename T>
		DataPackage Add(shared::MetaDataField field, T value)
		{
			meta_data[meta_data_to_string.at(field)] = value;
			return *this;
		}

		operator bool() const
		{
			return !error;
		}

		std::optional<std::vector<uint8_t>> Compress()
		{
			std::vector<jser::JSerError> jser_errors;
			const nlohmann::json response_json = SerializeObjectJson(std::back_inserter(jser_errors));
			if (jser_errors.size() > 0)
			{
				std::cout << "Failed to deserialize request answer" << std::endl;
				return std::nullopt;
			}
			std::vector<uint8_t> data_compressed = shared::compressZstd(nlohmann::json::to_msgpack(response_json));
			data_compressed.reserve(MAX_MSG_LENGTH_DECIMALS);
			std::vector<uint8_t> data_length = shared::stringToVector(shared::encodeMsgLength(data_compressed.size()));
			data_compressed.insert(data_compressed.begin(), data_length.begin(), data_length.end());
			return data_compressed;
		}

		static DataPackage Decompress(const std::vector<uint8_t>& data)
		{
			nlohmann::json decompressed_answer = nlohmann::json::from_msgpack(shared::decompressZstd(data), true, false);
			if (decompressed_answer.is_null())
			{
				std::cout << "Decompress data package is invalid" << std::endl;
				return DataPackage::Create<ErrorType::ERROR>({ "Decompress data package is invalid" });
			}
			// Deserialize answer
			std::vector<jser::JSerError> errors;
			DataPackage data_package{};
			data_package.DeserializeObject(decompressed_answer, std::back_inserter(errors));
			if (errors.size() > 0)
			{
				auto messages = helper::JserErrorToString(errors);
				messages.push_back("Failed to deserialize Server Request");
				return DataPackage::Create<ErrorType::ERROR>(messages);
			}
			else
			{
				return data_package;
			}
		}

		// Start and End are exclusive
		template<typename Enum, template<Enum> typename Factory, Enum Start, Enum End, typename ...Args>
		static constexpr std::map<Enum, std::function<DataPackage(DataPackage, Args...)>> CreateTransactionMap()
		{

			std::map < Enum, std::function<DataPackage(DataPackage, Args...)>> new_mappings{};
			constexpr Enum next = Enum(uint16_t(Start) + 1);
			if constexpr (next != End)
			{
				new_mappings.emplace(next,
					[](DataPackage pkg, Args && ...args)
					{
						return Factory<next>::Handle(pkg, std::forward<Args>(args)...);
					});
				auto merged = DataPackage::CreateTransactionMap<Enum, Factory, next, End, Args ...>();
				merged.insert(new_mappings.begin(), new_mappings.end());
				return merged;
			}
			return new_mappings;
		}
	};
}
