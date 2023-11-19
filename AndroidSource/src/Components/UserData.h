#pragma once
#include "JSerializer.h"
#include "SharedProtocol.h"

class UserData
{

public:
	struct Information : public jser::JSerializable
	{
		std::string username{};
		bool show_score = true;

		Information() {};
		Information(std::string username, bool show_score) :
			username(username),
			show_score(show_score) {};


		jser::JserChunkAppender AddItem() override
		{
			return JSerializable::AddItem().Append(JSER_ADD(SerializeManagerType, username, show_score));
		}
	};

	const int maxUsernameLength = 14;

	Information info;
	shared::ServerResponse WriteToDisc();
	shared::ServerResponse ValidateUsername();


	void Activate(class Application* app);
	void Deactivate(class Application* app) {};
	void OnStart(struct android_app* native_app);

private:
	std::variant<Information, shared::ServerResponse> ReadFromDisc();
	std::string GetAbsoluteUserDataPath() const;
	std::optional<std::string> GetExternalFilesDir(struct android_app* native_app) const;

	const std::string user_data_file = "user_data.json";
	std::string external_files_dir{};
};
