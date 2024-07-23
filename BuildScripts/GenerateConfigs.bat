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
echo { > ..\AndroidPackaging\app\src\main\assets\config.json
echo   "server_address": "192.168.2.109", >> ..\AndroidPackaging\app\src\main\assets\config.json
echo   "server_transaction_port": 9999,  >> ..\AndroidPackaging\app\src\main\assets\config.json
echo   "server_echo_start_port": 10000,  >> ..\AndroidPackaging\app\src\main\assets\config.json
echo   "server_echo_port_size": 1000,  >> ..\AndroidPackaging\app\src\main\assets\config.json
echo   "mongo": {  >> ..\AndroidPackaging\app\src\main\assets\config.json
echo     "db_name": "NatInfo", >> ..\AndroidPackaging\app\src\main\assets\config.json
echo     "coll_version_name": "VersionUpdate", >> ..\AndroidPackaging\app\src\main\assets\config.json
echo     "coll_information_name": "InformationUpdate", >> ..\AndroidPackaging\app\src\main\assets\config.json
echo     "coll_users_name": "testUsers", >> ..\AndroidPackaging\app\src\main\assets\config.json
echo     "coll_collect_config": "CollectConfig" >> ..\AndroidPackaging\app\src\main\assets\config.json
echo   }, >> ..\AndroidPackaging\app\src\main\assets\config.json
echo   "nat_ident": { >> ..\AndroidPackaging\app\src\main\assets\config.json
echo     "sample_size": 5, >> ..\AndroidPackaging\app\src\main\assets\config.json
echo     "max_delta_progressing_nat": 50 >> ..\AndroidPackaging\app\src\main\assets\config.json
echo   }, >> ..\AndroidPackaging\app\src\main\assets\config.json
echo   "app": { >> ..\AndroidPackaging\app\src\main\assets\config.json
echo     "random_nat_required": false, >> ..\AndroidPackaging\app\src\main\assets\config.json
echo     "traversal_feature_enabled": true, >> ..\AndroidPackaging\app\src\main\assets\config.json
echo     "max_log_lines": 410, >> ..\AndroidPackaging\app\src\main\assets\config.json
echo     "use_debug_collect_config": false >> ..\AndroidPackaging\app\src\main\assets\config.json
echo   } >> ..\AndroidPackaging\app\src\main\assets\config.json
echo } >> ..\AndroidPackaging\app\src\main\assets\config.json

