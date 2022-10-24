# NAT Data Collector

## How to set up Android Project

* Follow installation guide from [sample project](https://github.com/lectorguard/Android-CMake-VisualStudio-Sample)
* Execute GenerateAndroidVisualStudioProject.bat
* Set AndroidPackaging as Startup project
* Run the application on the android device

## How to set up Server Project

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

* Mongo DB datenbank an server anbinden
* android app aufräumen //screen abstrahieren usw
* Logik schreiben um ports zu generieren und zu speichern
* Tool bauen um daten zu überprüfen //vllt telegram bot
* Sich gedanken über speicher format machen und wie die informationen zu bekommen sind (Standort, provider, ...) //see https://www.infobyip.com/




