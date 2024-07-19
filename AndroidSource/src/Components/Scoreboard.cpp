#include "Scoreboard.h"
#include "Application/Application.h"
#include "CustomCollections/Log.h"
#include "Data/Address.h"
#include "Utilities/NetworkHelpers.h"
#include "UserData.h"
#include "GlobalConstants.h"


 void Scoreboard::Activate(Application* app)
 {
 	app->UpdateEvent
		.Subscribe([this](Application* app) { client.Update(); });
	app->_components
		.Get<NatCollectorModel>()
		.SubscribeTabEvent(NatCollectorTabState::Scoreboard,
			[this, app](auto st) { RequestScores(app); },	// Start Tab
			nullptr,										// Update Tab	
			nullptr);										// End Tab
	client.Subscribe({ Transaction::ERROR }, [this](auto pkg) { Shutdown(pkg); });
	client.Subscribe({ Transaction::CLIENT_CONNECTED, Transaction::CLIENT_RECEIVE_SCORES }, [this, app](auto pkg) {HandleTransaction(app, pkg); });
 }

 void Scoreboard::Shutdown(DataPackage pkg)
 {
	 Log::HandleResponse(pkg, "Scoreboard Request");
	 client.Disconnect();
 }

 void Scoreboard::HandleTransaction(Application* app, DataPackage pkg)
 {
	 UserData& user_data = app->_components.Get<UserData>();
	 NatCollectorModel& model = app->_components.Get<NatCollectorModel>();
	 Error err;
	 switch (pkg.transaction)
	 {
	 case Transaction::CLIENT_CONNECTED:
	 {
		 using namespace GlobServerConst;
		 // Create client id
		 ClientID client_id{ model.GetClientMetaData().android_id, user_data.info.username, user_data.info.show_score, 0 };
		 DataPackage pkg = DataPackage::Create(&client_id, Transaction::SERVER_GET_SCORES)
			 .Add<std::string>(MetaDataField::DB_NAME, Mongo::db_name)
			 .Add<std::string>(MetaDataField::USERS_COLL_NAME, Mongo::coll_users_name)
			 .Add<std::string>(MetaDataField::ANDROID_ID, model.GetClientMetaData().android_id);
		 client.ServerTransactionAsync(pkg);
		 break;
	 }
	 case Transaction::CLIENT_RECEIVE_SCORES:
	 {
		 if((err = pkg.Get(scores)))break;
		 std::sort(scores.scores.begin(), scores.scores.end(), [](auto l, auto r) {return l.uploaded_samples > r.uploaded_samples; });
		 Shutdown(DataPackage());
		 Log::Info("Successfully Received Scores");
		 break;
	 } 
	 default:
		 break;
	 }
 }

 void Scoreboard::RequestScores(Application* app)
{
	 if (client.IsRunning())
	 {
		 Log::Warning("Scoreboard is already requested. Wait until request finishes");
		 return;
	 }

	 auto& model = app->_components.Get<NatCollectorModel>();
	 auto user_data = app->_components.Get<UserData>();
	 if (model.GetClientMetaData().android_id.empty())
	 {
		 Log::Warning("Invalid android id, can not retrieve scoreboard");
	 }
	 else if (auto error = user_data.ValidateUsername())
	 {
		 Log::HandleResponse(error, "Requesting Scoreboard Scores");
	 }
	 else
	 {
		 user_data.WriteToDisc();
		 client.ConnectAsync(GlobServerConst::server_address, GlobServerConst::server_transaction_port);
	 }
 }

void Scoreboard::Deactivate(Application* app)
{
}



