# NAT Data Collector

## How to set up Android Project

### Prerequisites

* Cmake 3.4.1 or higher
* Visual Studio 22 17.4 Preview 2 or higher
* Visual Studio Module : Mobile Development with C++
* Visual Studio Module : Desktop Development with C++
* devenv.exe must be set as environment variable (should be similar to C:\Program Files\Microsoft Visual Studio\2022\Preview\Common7\IDE)

### Set up

* Follow installation guide from [sample project](https://github.com/lectorguard/Android-CMake-VisualStudio-Sample)
* Execute GenerateAndroidVisualStudioProject.bat
* Set AndroidPackaging as Startup project
* Run the application on the android device

## How to set up Server Project

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

* Using ubuntu, the mongo DB can be accessed via mongosh : 
```
mongosh
db networkdata //Switching database
db.auth("name", "password") // Authenticate for server
show collections // shows all collections -> test
db.test.find() // shows content of test collection
```
* See [Mongosh cheat sheet](https://www.mongodb.com/developer/products/mongodb/cheat-sheet/) for more info

### Start Server on Ubuntu

#### Prerequisites

* Cmake 4.20 or higher
* C++ and C compiler

### Start Server on Windows for Testing

* Install gdb for WSL2 (Windows subsystem for linux)
* Visual studio max supported version is currently 10.2, see [issue](https://github.com/microsoft/vscode-cpptools/issues/9704) 
* For Ubuntu 22.04, you need to install gdb-10.2 from source, see [tutorial](http://www.gdbtutorial.com/tutorial/how-install-gdb)  
* Follow installation guide for [gcc visual studio cmake project](https://www.youtube.com/watch?v=IKI2w75aAow)
* Execute GenerateServerVisualStudioProject.bat
* Set Target System from **local machine** to **WSL 22.04**
* Set StartUp Item to **server-app** (It can take a while until it shows up)

# Debuggig android

* Under tools->Android Tools->logcat, you can see LOGI and LOGW. Filter for native-activity.
* Under windows make sure, to accept windows firewall window



# TODO

* Mongo DB datenbank an server anbinden // Done
* android app aufräumen // Done
* Logik schreiben um ports zu generieren und zu speichern
* Tool bauen um daten zu überprüfen //vllt telegram bot
* Sich gedanken über speicher format machen und wie die informationen zu bekommen sind (Standort, provider, ...) //see https://www.infobyip.com/


