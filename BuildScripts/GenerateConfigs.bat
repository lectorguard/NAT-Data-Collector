@echo off
REM Server Config
echo {	> ..\server_template_config.json
echo    "udp_starting_port" : 10000, >> ..\server_template_config.json
echo    "udp_amount_services" : 1000, >> ..\server_template_config.json
echo    "tcp_session_server_port" : 9999, >> ..\server_template_config.json
echo    "mongo_server_url": "mongodb://<user>:<password>@<server_ip_address>/?authSource=<db name>", >> ..\server_template_config.json
echo    "mongo_app_name": "<mongo app name>" >> ..\server_template_config.json
echo } >> ..\server_template_config.json
REM Android Config
echo cmake_minimum_required(VERSION 3.20) > ..\android.config
echo. >> ..\android.config
echo set(SERVER_IP "192.168.18.10") # your server ip address	>> ..\android.config
echo set(SERVER_TRANSACTION_TCP_PORT 9999) # server port to perform transactions (TCP) >> ..\android.config
echo set(SERVER_ECHO_UDP_START_PORT 10000) # first server echo service runs on this port >> ..\android.config
echo set(SERVER_ECHO_UDP_SERVICES 1000) # amount of echo services supported by server >> ..\android.config
echo set(MONGO_DB_NAME "NatInfo") # Mongo db name, where collected NAT Samples will be stored >> ..\android.config
echo set(MONGO_NAT_USERS_COLL_NAME "users") # Mongo collection name, where app user information are stored >> ..\android.config
echo set(MONGO_VERSION_COLL_NAME "VersionUpdate") # Mongo collection name, where new version pop up information are stored >> ..\android.config
echo set(MONGO_INFORMATION_COLL_NAME "InformationUpdate") # Mongo collection name, where general information for popups are stored >> ..\android.config
echo set(MONGO_COLL_CONFIG_NAME "CollectConfig") # Name of mongo collection, where nat collect config is stored >> ..\android.config
echo set(APP_VERSION "v0.0.2") # Version of the app >> ..\android.config

