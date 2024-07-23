#include "pch.h"
#include "Config.h"
#include "Application/Application.h"

DataPackage Config::ReadConfigFile(Application* app)
{
	AAssetManager* manager = nullptr;
	if (app &&
		app->android_state &&
		app->android_state->activity &&
		app->android_state->activity->assetManager)
	{
		manager = app->android_state->activity->assetManager;
	}
	return ReadAssetFile(manager, config_path);
}

shared::DataPackage Config::ReadAssetFile(AAssetManager* manager, const std::string& path)
{
	if (manager == nullptr)
		return DataPackage::Create<ErrorType::ERROR>({ "No AssetManager, Initialization Error" });
	AAsset* file = AAssetManager_open(manager, path.c_str(), AASSET_MODE_BUFFER);
	if (file == nullptr)
		return DataPackage::Create<ErrorType::ERROR>({ "Invalid file path" });
	size_t fileLength = AAsset_getLength(file);
	std::vector<uint8_t> buf(fileLength, 0);
	AAsset_read(file, buf.data(), fileLength);
	AAsset_close(file);
	DataPackage pkg;
	pkg.error = Error(ErrorType::ANSWER);
	pkg.transaction = Transaction::NO_TRANSACTION;
	pkg.data = nlohmann::json::parse(std::string(buf.begin(), buf.end()), nullptr, false);
	return pkg;
}
