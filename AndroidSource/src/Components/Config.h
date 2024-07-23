#pragma once

using namespace shared;

class Config
{

public:
	struct Data : public jser::JSerializable
	{
		struct Mongo : public jser::JSerializable
		{
			std::string db_name{};
			std::string coll_version_name{};
			std::string coll_information_name{};
			std::string coll_users_name{};
			std::string coll_collect_config{};

			Mongo() {};
			jser::JserChunkAppender AddItem() override
			{
				return JSerializable::AddItem().Append(JSER_ADD(SerializeManagerType, db_name, 
					coll_version_name, coll_information_name, coll_users_name,coll_collect_config));
			}
		};
		
		struct NATIdent : public jser::JSerializable
		{
			uint32_t sample_size = 5;
			uint16_t max_delta_progressing_nat = 50;

			NATIdent() {};
			jser::JserChunkAppender AddItem() override
			{
				return JSerializable::AddItem().Append(JSER_ADD(SerializeManagerType, sample_size, max_delta_progressing_nat));
			}
		};

		struct App : public jser::JSerializable
		{
			bool random_nat_required = false;
			bool traversal_feature_enabled = true;
			uint32_t max_log_lines = 400;
			bool use_debug_collect_config = false;

			App() {};
			jser::JserChunkAppender AddItem() override
			{
				return JSerializable::AddItem().Append(JSER_ADD(SerializeManagerType, random_nat_required, traversal_feature_enabled,
					max_log_lines, use_debug_collect_config));
			}
		};

		std::string server_address{};
		uint16_t server_transaction_port{};
		uint16_t server_echo_start_port{};
		uint16_t server_echo_port_size{};
		uint32_t global_socket_timeout_ms = SOCKET_TIMEOUT_MS;
		std::string app_version = APP_VERSION;
		uint32_t max_msg_length_decimals = MAX_MSG_LENGTH_DECIMALS;
		Mongo mongo{};
		NATIdent nat_ident{};
		App app{};


		Data() {};
		jser::JserChunkAppender AddItem() override
		{
			return JSerializable::AddItem().Append(
				JSER_ADD(SerializeManagerType, server_address, server_transaction_port, server_echo_start_port, server_echo_port_size,
					mongo, nat_ident, app));
		}
	};

	static DataPackage ReadConfigFile(class Application* app);
private:
	static DataPackage ReadAssetFile(AAssetManager* manager, const std::string& path);
	static inline const std::string config_path = "config.json";
};
