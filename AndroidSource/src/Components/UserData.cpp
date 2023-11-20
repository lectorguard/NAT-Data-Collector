#include "Components/UserData.h"
#include "Application/Application.h"
#include "CustomCollections/Log.h"
#include "SharedHelpers.h"
#include "Utilities/NetworkHelpers.h"


void UserData::Activate(class Application* app)
{
	app->AndroidStartEvent.Subscribe([this](struct android_app* state) { OnStart(state); });
}

void UserData::OnStart(struct android_app* native_app)
{
	std::visit(shared::helper::Overloaded
		{
			[this](std::string dir) { external_files_dir = dir; },
			[](shared::ServerResponse resp) { Log::HandleResponse(resp, "Try retrieving external files directory from JNI"); }
		}, GetExternalFilesDir(native_app));

	std::visit(shared::helper::Overloaded
		{
			[this](Information user_info) { info = user_info; },
			[](shared::ServerResponse resp) { Log::HandleResponse(resp, "Read User Data from Disc"); }
		}, ReadFromDisc());
}

shared::ServerResponse UserData::ValidateUsername()
{
	if (!info.username.empty() && info.username.length() < maxUsernameLength)
	{
		return shared::ServerResponse::OK();
	}
	else
	{
		return shared::ServerResponse::Warning({ "Please enter valid username first.",
			"Username must have at least 1 and max" + std::to_string(maxUsernameLength) + " characters." });
	}
}


std::variant<UserData::Information, shared::ServerResponse> UserData::ReadFromDisc()
{
	const auto user_data_path = GetAbsoluteUserDataPath();
	if (!user_data_path)
	{
		return shared::ServerResponse::Error({ "Absolute Android User Data Path is invalid" });
	}

	if (FILE* appConfigFile = std::fopen(user_data_path->c_str(), "r+"))
	{
		fseek(appConfigFile, 0, SEEK_END);
		long fsize = ftell(appConfigFile);
		fseek(appConfigFile, 0, SEEK_SET);
		//malloc
		char* string = (char*)malloc(fsize + 1);
		fread(string, fsize, 1, appConfigFile);
		fclose(appConfigFile);

		std::string file_data(string, fsize + 1);
		std::vector<jser::JSerError> jser_errors;
		UserData::Information user_info;
		user_info.DeserializeObject(file_data, std::back_inserter(jser_errors));
		//free
		free(string);

		if (jser_errors.size() > 0)
		{
			return shared::helper::HandleJserError(jser_errors, "Read User data from disc");
		}
		return user_info;
	}
	else
	{
		// Failed to open user data file, file might not exist yet
		return shared::ServerResponse::OK();
	}
	
}

shared::ServerResponse UserData::WriteToDisc()
{
	// Deserialize user data
	std::vector<jser::JSerError> jser_errors;
	const std::string user_info = info.SerializeObjectString(std::back_inserter(jser_errors));
	if (jser_errors.size() > 0)
	{
		return shared::helper::HandleJserError(jser_errors, "Write to disc : Serialize user data"); 
	}

	const auto user_data_path = GetAbsoluteUserDataPath();
	if (!user_data_path)
	{
		return shared::ServerResponse::Error({ "Absolute Android User Data Path is invalid" });
	}

	// Write to file
	shared::ServerResponse result = shared::ServerResponse::OK();
	if (FILE* appConfigFile = std::fopen(user_data_path->c_str(), "w+"))
	{
		auto res = std::fwrite(user_info.c_str(), sizeof(char), user_info.size(), appConfigFile);
		if (res != user_info.size())
		{
			result = shared::ServerResponse::Error({ "Failed to write User Info to disc, write error" });
		}
		std::fclose(appConfigFile);
	}
	else
	{
		result = shared::ServerResponse::Error({ "Failed to open user data file : " + *user_data_path });
	}
	return result;
}



std::optional<std::string> UserData::GetAbsoluteUserDataPath() const
{
	return external_files_dir ? *external_files_dir + "/" + user_data_file : external_files_dir;
}

std::variant<shared::ServerResponse, std::string> UserData::GetExternalFilesDir(struct android_app* native_app) const
{
	if (!native_app)
	{
		return shared::ServerResponse::Error({ "Passed android native app pointer is invalid" });
	}

	jint lResult;

	JavaVM* lJavaVM = native_app->activity->vm;
	JNIEnv* lJNIEnv = native_app->activity->env;

	JavaVMAttachArgs lJavaVMAttachArgs;
	lJavaVMAttachArgs.version = JNI_VERSION_1_6;
	lJavaVMAttachArgs.name = "NativeThread";
	lJavaVMAttachArgs.group = NULL;

	lResult = lJavaVM->AttachCurrentThread(&lJNIEnv, &lJavaVMAttachArgs);
	if (lResult == JNI_ERR)
	{
		return shared::ServerResponse::Error({ "Failed to attach to JNI thread from native activity" });
	}

	// Return information
	std::string result_id{};
	shared::ServerResponse response = shared::ServerResponse::OK();

	// Retrieves NativeActivity.
	jobject lNativeActivity = native_app->activity->clazz;

	// Retrieves Context.INPUT_METHOD_SERVICE.
	jclass ClassContext = lJNIEnv->FindClass("android/content/Context");

	jmethodID getExternalFilesDirMethod = lJNIEnv->GetMethodID(ClassContext, "getExternalFilesDir", "(Ljava/lang/String;)Ljava/io/File;");
	jstring nullString = NULL; // Passing null as the argument
	jobject externalFilesDirObj = lJNIEnv->CallObjectMethod(lNativeActivity, getExternalFilesDirMethod, nullString);

	if (externalFilesDirObj != NULL) {
		// Get the absolute path from the File object
		jclass fileClass = lJNIEnv->GetObjectClass(externalFilesDirObj);
		jmethodID getAbsolutePathMethod = lJNIEnv->GetMethodID(fileClass, "getAbsolutePath", "()Ljava/lang/String;");
		jstring absolutePath = (jstring)lJNIEnv->CallObjectMethod(externalFilesDirObj, getAbsolutePathMethod);

		// Convert the Java string to a C++ string
		const char* pathStr = lJNIEnv->GetStringUTFChars(absolutePath, NULL);
		result_id = std::string(pathStr);

		// Release local references
		lJNIEnv->ReleaseStringUTFChars(absolutePath, pathStr);
		lJNIEnv->DeleteLocalRef(fileClass);
		lJNIEnv->DeleteLocalRef(externalFilesDirObj);
	}
	else
	{
		response = shared::ServerResponse::Error({ "Failed retrieving ExternalFilesDirObject from JNI" });
	}
	// Make sure to detach always
	lJavaVM->DetachCurrentThread();
	if (response)
	{
		return result_id;
	}
	else
	{
		return response;
	}
}
