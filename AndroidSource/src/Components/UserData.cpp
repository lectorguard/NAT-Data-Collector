#include "Components/UserData.h"
#include "Application/Application.h"
#include "CustomCollections/Log.h"
#include "SharedHelpers.h"
#include "Utilities/NetworkHelpers.h"


void UserData::Activate(class Application* app)
{
	std::visit(shared::helper::Overloaded
		{
			[this](std::string dir) { external_files_dir = dir; },
			[](shared::Error err) { Log::HandleResponse(err, "Failed to retrieve external files directory from JNI"); }
		}, GetExternalFilesDir(app->android_state));

	if (auto err = ReadFromDisc().Get(info))
	{
		Log::HandleResponse(err, "Read User Data from Disc");
	}
	else
	{
		Log::Info("Read user data from disc");
	}
}

shared::Error UserData::ValidateUsername()
{
	if (!info.username.empty() && info.username.length() < maxUsernameLength)
	{
		return Error{ ErrorType::OK };
	}
	else
	{
		return Error{ 
			ErrorType::WARNING, 
			{ 
				"Please enter valid username first.",
				"Username must have at least 1 and max" + std::to_string(maxUsernameLength) + " characters." 
			} 
		};
	}
}


shared::DataPackage UserData::ReadFromDisc()
{
	const auto user_data_path = GetAbsoluteUserDataPath();
	if (!user_data_path)
	{
		return DataPackage::Create<ErrorType::ERROR>({ "Absolute Android User Data Path is invalid" });
	}

	if (FILE* appConfigFile = std::fopen(user_data_path->c_str(), "r+"))
	{
		fseek(appConfigFile, 0, SEEK_END);
		long fsize = ftell(appConfigFile);
		fseek(appConfigFile, 0, SEEK_SET);
		//malloc
		std::vector<uint8_t> buffer(fsize);
		fread(buffer.data(), fsize, 1, appConfigFile);
		fclose(appConfigFile);

		std::string file_data(buffer.begin(), buffer.end());
		std::vector<jser::JSerError> jser_errors;
		UserData::Information user_info;
		user_info.DeserializeObject(file_data, std::back_inserter(jser_errors));
		if (jser_errors.size() > 0)
		{
			auto messages = helper::JserErrorToString(jser_errors);
			messages.push_back("Read User data from disc");
			return DataPackage::Create<ErrorType::ERROR>(messages);
		}
		return DataPackage::Create(&user_info, Transaction::NO_TRANSACTION);
	}
	else
	{
		// Failed to open user data file, file might not exist yet
		return DataPackage::Create<ErrorType::OK>();
	}
}

shared::Error UserData::WriteToDisc()
{
	Log::Info("Write User Data to Disc");
	// Deserialize user data
	std::vector<jser::JSerError> jser_errors;
	const std::string user_info = info.SerializeObjectString(std::back_inserter(jser_errors));
	if (jser_errors.size() > 0)
	{
		auto messages = helper::JserErrorToString(jser_errors);
		messages.push_back("Read User data from disc");
		return Error{ ErrorType::ERROR,messages };
	}

	const auto user_data_path = GetAbsoluteUserDataPath();
	if (!user_data_path)
	{
		return Error(ErrorType::ERROR, { "Absolute Android User Data Path is invalid" });
	}

	// Write to file
	Error err{ ErrorType::OK };
	if (FILE* appConfigFile = std::fopen(user_data_path->c_str(), "w+"))
	{
		auto res = std::fwrite(user_info.c_str(), sizeof(char), user_info.size(), appConfigFile);
		if (res != user_info.size())
		{
			err.Add({ ErrorType::ERROR, { "Failed to write User Info to disc, write error" } });
		}
		std::fclose(appConfigFile);
	}
	else
	{
		err.Add({ ErrorType::ERROR, { "Failed to open user data file : " + *user_data_path } });
	}
	return err;
}



std::optional<std::string> UserData::GetAbsoluteUserDataPath() const
{
	return external_files_dir ? *external_files_dir + "/" + user_data_file : external_files_dir;
}

std::variant<shared::Error, std::string> UserData::GetExternalFilesDir(struct android_app* native_app) const
{
	if (!native_app)
	{
		return Error(ErrorType::ERROR, { "Passed android native app pointer is invalid" });
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
		return Error(ErrorType::ERROR, { "Failed to attach to JNI thread from native activity" });
	}

	// Return information
	std::string result_id{};
	shared::Error error{ErrorType::OK};

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
		error = Error(ErrorType::ERROR, { "Failed retrieving ExternalFilesDirObject from JNI" });
	}
	// Make sure to detach always
	lJavaVM->DetachCurrentThread();
	if (error) return error;
	else return result_id;
}
