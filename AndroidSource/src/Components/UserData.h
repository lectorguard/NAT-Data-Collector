#pragma once
#include "JSerializer.h"
#include "SharedProtocol.h"

class UserData
{

public:
	struct Information : public jser::JSerializable
	{
		std::string username{};
		std::string version_update{};
		std::string information_identifier{};
		bool show_score = true;
		bool ignore_pop_up = false;
		Information() {};
		Information(std::string username, bool show_score) :
			username(username),
			show_score(show_score) {};


		jser::JserChunkAppender AddItem() override
		{
			return JSerializable::AddItem().Append(JSER_ADD(SerializeManagerType, username, show_score, ignore_pop_up, version_update, information_identifier));
		}
	};

	const int maxUsernameLength = 14;

	Information info;
	shared::ServerResponse WriteToDisc();
	shared::ServerResponse ValidateUsername();


	void Activate(class Application* app);
	void Deactivate(class Application* app) {};


private:
	std::variant<Information, shared::ServerResponse> ReadFromDisc();
	std::optional<std::string> GetAbsoluteUserDataPath() const;
	std::variant<shared::ServerResponse, std::string> GetExternalFilesDir(struct android_app* native_app) const;

	const std::string user_data_file = "user_data.json";
	std::optional<std::string> external_files_dir = std::nullopt;
};
