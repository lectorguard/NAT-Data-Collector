#pragma once

#include "JSerializer.h"
#include "type_traits"
#include "SharedTypes.h"
#include <variant>
#include "sstream"
#include <memory>
#include "SharedHelpers.h"
#include <Compression.h>
#include "asio.hpp"

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

		void Add(Error e)
		{
			if (static_cast<uint16_t>(e.error) > static_cast<uint16_t>(error))
			{
				error = e.error;
			}
			messages.insert(messages.end(), e.messages.begin(), e.messages.end());
		}

		static Error FromAsio(asio::error_code ec, const std::string& context = "")
		{
			using namespace shared;
			if (ec == asio::error::eof)
			{
				const std::vector<std::string> msg
				{
					"Connection Rejected during Transaction Attempt",
					"Context : " + context
				};
				return Error(ErrorType::ERROR, msg);
			}
			else if (ec)
			{

				return Error(ErrorType::ERROR, {"Networking Error : " +  ec.message()});
			}
			return Error(ErrorType::OK);
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

	template<typename ... Args>
	struct MetaVariant
	{
	public:
		MetaVariant(const std::tuple<Args ...>& values, const Error& err, const nlohmann::json& meta_data) :
			values(values), error(err), meta_data(meta_data)
		{};

		template<typename M>
		auto Get(MetaDataField field) -> MetaVariant<Args ..., M>
		{
			if (error)
			{
				return MetaVariant<Args ..., M>(std::tuple_cat(values, std::make_tuple(M())), error, meta_data);
			}

			if (!meta_data_to_string.contains(field))
			{
				error.Add(Error{ ErrorType::ERROR, {"Meta data field has no string representation" } });
				return MetaVariant<Args ..., M>(std::tuple_cat(values, std::make_tuple(M())), error, meta_data);
			}

			if (!meta_data.contains(meta_data_to_string.at(field)))
			{
				error.Add(Error{ ErrorType::ERROR, {"Meta data json misses mandatory field : " + meta_data_to_string.at(field)} });
				return MetaVariant<Args ..., M>(std::tuple_cat(values, std::make_tuple(M())), error, meta_data);
			}
			M obj = meta_data[meta_data_to_string.at(field)];
			auto merged = std::tuple_cat(values, std::make_tuple(obj));
			return MetaVariant<Args ..., M>(merged, error, meta_data);
		}

		std::tuple<Args...> values{};
		Error error{};
	private:
		nlohmann::json meta_data{};
	};

	// Allow single param
	template <>
	struct MetaVariant<> {
	public:
		MetaVariant(const std::tuple<>& values, const Error& err, const nlohmann::json& meta_data) :
			values(values), error(err), meta_data(meta_data)
		{};

		template<typename M>
		auto Get(MetaDataField field) -> MetaVariant<M>
		{
			if (error)
			{
				return MetaVariant<M>(std::tuple_cat(values, std::make_tuple(M())), error, meta_data);
			}

			if (!meta_data_to_string.contains(field))
			{
				error.Add(Error{ ErrorType::ERROR, {"Meta data field has no string representation" } });
				return MetaVariant<M>(std::tuple_cat(values, std::make_tuple(M())), error, meta_data);
			}

			if (!meta_data.contains(meta_data_to_string.at(field)))
			{
				error.Add(Error{ ErrorType::ERROR, {"Meta data json misses mandatory field : " + meta_data_to_string.at(field)} });
				return MetaVariant<M>(std::tuple_cat(values, std::make_tuple(M())), error, meta_data);
			}
			M obj = meta_data[meta_data_to_string.at(field)];
			auto merged = std::tuple_cat(values, std::make_tuple(obj));
			return MetaVariant<M>(merged, error, meta_data);
		}

		std::tuple<> values{};
		Error error{};
	private:
		nlohmann::json meta_data{};
	
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
		auto Get(MetaDataField field) -> MetaVariant<T>
		{
			MetaVariant<> meta_var{ std::make_tuple(), error, meta_data };
			return meta_var.Get<T>(field);
		}

		template<typename T>
		Error Get(T& object)
		{
			if (!error.Is<ErrorType::ANSWER>())
			{
				return error;
			}

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

		static DataPackage Create(jser::JSerializable* data, Transaction transaction)
		{
			using namespace shared;
			DataPackage pkg{};
			if (data)
			{
				std::vector<jser::JSerError> jser_errors;
				pkg.data = data->SerializeObjectJson(std::back_inserter(jser_errors));
				if (jser_errors.size() > 0)
				{
					return Create<ErrorType::ERROR>(helper::JserErrorToString(jser_errors));
				}
				else
				{
					pkg.error = Error(ErrorType::ANSWER);
				}
			}
			pkg.transaction = transaction;
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

		std::optional<std::vector<uint8_t>> Compress(bool prepend_msg_length = true)
		{
			std::vector<jser::JSerError> jser_errors;
			const nlohmann::json response_json = SerializeObjectJson(std::back_inserter(jser_errors));
			if (jser_errors.size() > 0)
			{
				std::cout << "Failed to deserialize request answer" << std::endl;
				return std::nullopt;
			}

			std::vector<uint8_t> data_compressed = shared::compressZstd(nlohmann::json::to_msgpack(response_json));
			if (!prepend_msg_length)
			{
				return data_compressed;
			}
			else
			{
				data_compressed.reserve(MAX_MSG_LENGTH_DECIMALS);
				std::vector<uint8_t> data_length = shared::stringToVector(shared::encodeMsgLength(data_compressed.size()));
				data_compressed.insert(data_compressed.begin(), data_length.begin(), data_length.end());
				return data_compressed;
			}
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
