# NAT Data Collector

## Clone Repository

* Clone repository recursively with `git clone --recursive git@github.com:lectorguard/NAT-Data-Collector.git`
* If you forgot to clone recursively use `git submodule update --init --remote --recursive`

## Config files

### Android Config

* Create a new file named `android.config` in the top level folder in the repository
* Copy the following settings and modify it if necessary

```
cmake_minimum_required(VERSION 3.20)

set(APP_VERSION "v0.0.1") # specify the current version of the app
set(SERVER_IP "192.168.2.110") # your server ip address
set(SERVER_NAT_UDP_PORT_1 7777) # server port for nat information request
set(SERVER_NAT_UDP_PORT_2 7778) # server port for nat information request
set(SERVER_TRANSACTION_TCP_PORT 7779) # server port to upload 
set(NAT_IDENT_REQUEST_FREQUNCY_MS 20) # Frequency of nat requests for NAT identification
set(NAT_IDENT_AMOUNT_SAMPLES_USED 5) # Number of samples used, to identify NAT type
set(NAT_IDENT_MAX_PROG_SYM_DELTA 50) # Maximum delta between 2 consecutive ports, to classify NAT as progressing
set(NAT_COLLECT_REQUEST_DELAY_MS 200) # Time between NAT requests during collection step 
set(NAT_COLLECT_PORTS_PER_SAMPLE 200) # Total amount of ports requested during collection step
set(NAT_COLLECT_SAMPLE_DELAY_MS 30000) # Delay between two consecutive NAT collection steps
set(MONGO_DB_NAME "NatInfo") # Mongo db name, where collected NAT Samples will be stored
set(MONGO_NAT_SAMPLES_COLL_NAME "data") # Monog collection name, where collected NAT Samples will be stored
set(MONGO_NAT_USERS_COLL_NAME "users") # Mongo collection name, where app user information are stored
set(MONGO_VERSION_COLL_NAME "VersionUpdate") # Mongo collection name, where new version pop up information are stored
set(MONGO_INFORMATION_COLL_NAME "InformationUpdate") # Mongo collection name, where general information for popups are stored
set(RANDOM_SYM_NAT_REQUIRED 1) #If set to 1, ports are only collected if NAT type is random symmetric
```

### Server Config

* Create a new file named `server-template-config.json` in the top level folder in the repository
* This is a template file, it will be copied next to the executable output and will be loaded at runtime
* Copy the following settings and modify it if necessary

```
{
	"udp_address_server1_port": 7777,
	"udp_address_server2_port": 7778,
	"tcp_session_server_port": 7779,
	"mongo_server_url": "mongodb://<user>:<password>@<server_ip_address>/?authSource=<db name>",
	"mongo_app_name": "<mongo app name>"
}
```


## How to set up Android Project

### Prerequisites

* Cmake 3.4.1 or higher
* Visual Studio 22 17.4 Preview 2 or higher
* Visual Studio Module : Mobile Development with C++
* Visual Studio Module : Desktop Development with C++
* devenv.exe must be set as environment variable (should be similar to C:\Program Files\Microsoft Visual Studio\2022\Preview\Common7\IDE)

### Set up

* Follow installation guide from [sample project](https://github.com/lectorguard/Android-CMake-VisualStudio-Sample)
* Open Visual Studio, continue without code
* Navigate to `Tools->Options->CrossPlatform->C++`
* Check if the following paths are correctly set
```
Android SDK : C:\Program Files (x86)\Android\android-sdk // Check content, should be part of VS Module : Mobile Dev Kit
Android NDK : C:\Microsoft\AndroidNDK\android-ndk-r23c // Check content, should be part of VS Module : Mobile Dev Kit
Java SE Development Kit : C:\Program Files\Microsoft\jdk-11.0.16.101-hotspot // Should be installed by default
Apache Ant : 
```
* In case `Java SE Development Kit` is missing, you can install it from [here](https://aka.ms/download-jdk/microsoft-jdk-11.0.12.7.1-windows-x64.msi) 
* Execute `GenerateAndroidVisualStudioProject.bat`
* Set `AndroidPackaging `as Startup project
* Run the application on the android device

## How to set up Mongo DB on Ubuntu

### Setup Mongo DB on Ubuntu

* Follow MongoDB [installation guide](https://www.mongodb.com/docs/manual/tutorial/install-mongodb-on-ubuntu/)
* Most important commands :
```
sudo systemctl start mongod
sudo systemctl daemon-reload
sudo systemctl status mongod
sudo systemctl stop mongod
sudo systemctl restart mongod
```
* Configure Mongo DB for [remote access](https://indianceo.medium.com/how-to-connect-to-your-remote-mongodb-server-68725a8e53f)

### Mongosh Usage

* The project expects there is a database called `NatInfo`
* Using ubuntu, the mongo DB can be accessed via mongosh : 
```
mongosh
use NatInfo
db NatInfo //Switching database
db.auth("name", "password") // Authenticate for server
show collections // shows all collections -> test
db.test.find() // shows content of test collection
```
* See [Mongosh cheat sheet](https://www.mongodb.com/developer/products/mongodb/cheat-sheet/) for more info

## How to set up Server App on Ubuntu

### Prerequisites

* The project requires multiple packages
* The following commands checks the packages and installs them if necessary

`sudo apt-get install git cmake g++ pkg-config libssl-dev libsasl2-dev`

* Open file `ServerSource/mongo_config_template.json`
* Fill out mongo server URL and application based on the setup mongo server (**Mongo Server is required for this application**)

### Run Server Application

* Simply run `./StartServer.sh`
* Make sure you have correct permissions to do so

## How to set up Server App on Windows using WSL (Visual Studio, Debugging)

* Open file `ServerSource/mongo_config_template.json`
* Fill out mongo server URL and application based on the setup mongo server (**Mongo Server is required for this application**)
* Prepare WSL for the usage with [Visual Studio and CMake](https://learn.microsoft.com/en-us/cpp/build/walkthrough-build-debug-wsl2?view=msvc-170)
* **Donwgrade** GDB for WSL (Windows Subsystem for Linux) 
  * Visual studio max supported version is currently 10.2, see [issue](https://github.com/microsoft/vscode-cpptools/issues/9704)
  * Delete the current gdb package with `sudo apt-get purge --auto-remove gdb`
  * GDB installation by hand requires texinfo, install it with `sudo apt-get install texinfo`
  * For Ubuntu 22.04, you need to install gdb-10.2 from source, see [tutorial](http://www.gdbtutorial.com/tutorial/how-install-gdb)  
* Follow installation guide for [gcc visual studio cmake project](https://www.youtube.com/watch?v=IKI2w75aAow)
* Execute `GenerateServerVisualStudioProject.bat`
* Set Target System from **local machine** to **WSL 22.04**
* Set StartUp Item to **server-app** (It can take a while until it shows up)

# Debuggig android

* Under tools->Android Tools->logcat, you can see LOGI and LOGW. Filter for native-activity.
* Under windows make sure, to accept windows firewall window

# Issues 

## Windows Defender blocks socket communication between WSL server app and android app (silent failure)

### Solution 1: 

* Rerun `GenerateServerVisualStudioProject.bat` to delete old Windows Defender Rules

### Solution 2:

* Delete `bin` folder and select `Project/Delete Cache and Reconfigure` in the cmake server VS project
* Rebuild and Run the server project
* If `Windows Security Alert` **shows** and access is allowed, everything is correctly configured
* If `Windows Security Alert` does not show
	* Type `Advanced Security Windows Defender` in the windows search
	* Select `Windows Defender Firewall with Advanced Security`
	* Select `Inbound Rules`
	* Search for `server-app`, if not found try click `refresh` button on the right
	* Delete all `server-app` entries and repeat from above

## Notify users about new version

* Connect to mongo db via mongosh
* Drop old entries (only first one is used) `db.VersionUpdate.drop()`
* Add new entry
```
db.VersionUpdate.insertOne({"download_link":"https://github.com/lectorguard/NAT-Data-Collector/releases","latest_version":"v0.0.2","version_details":"A new version of the NAT Collector is available. Please copy the link below, delete this version and download the new version. You progress in the scoreboard will be maintained. :) Thank you.","version_header":"New Version 0.0.2"})
```

## Notify user about new general informationen

* Connect to mongo db via mongosh
* Drop old entries (only first one is used) `db.InformationUpdate.drop()`
* Add new entry
```
db.InformationUpdate.insertOne({"identifier":"test","information_details":"There are very important information available.","information_header":"New Information"})
```


