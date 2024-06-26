@echo off
REM Server Config
echo {	> ..\server_template_config.json
echo    "udp_address_server1_port": 7777, >> ..\server_template_config.json
echo    "udp_address_server2_port": 7778, >> ..\server_template_config.json
echo    "tcp_session_server_port": 7779, >> ..\server_template_config.json
echo    "mongo_server_url": "mongodb://<user>:<password>@<server_ip_address>/?authSource=<db name>", >> ..\server_template_config.json
echo    "mongo_app_name": "<mongo app name>" >> ..\server_template_config.json
echo } >> ..\server_template_config.json
REM Android Config
echo cmake_minimum_required(VERSION 3.20) > ..\android.config
echo. >> ..\android.config
echo set(SERVER_IP "192.168.2.110") # your server ip address >> ..\android.config
echo set(SERVER_NAT_UDP_PORT_1 7777) # server port for nat information request >> ..\android.config
echo set(SERVER_NAT_UDP_PORT_2 7778) # server port for nat information request >> ..\android.config
echo set(SERVER_TRANSACTION_TCP_PORT 7779) # server port to upload >> ..\android.config
echo set(NAT_IDENT_REQUEST_FREQUNCY_MS 20) # Frequency of nat requests for NAT identification >> ..\android.config
echo set(NAT_IDENT_AMOUNT_SAMPLES_USED 5) # Number of samples used, to identify NAT type >> ..\android.config
echo set(NAT_IDENT_MAX_PROG_SYM_DELTA 50) # Maximum delta between 2 consecutive ports, to classify NAT as progressing >> ..\android.config
echo set(NAT_COLLECT_REQUEST_DELAY_MS 1) # Time between NAT requests during collection step >> ..\android.config
echo set(NAT_COLLECT_PORTS_PER_SAMPLE 60000) # Total amount of ports requested during collection step >> ..\android.config
echo set(NAT_COLLECT_EXTRA_TIME_MS 60000) # Request delay * ports per sample + extra time = max duration for collection step >> ..\android.config
echo set(NAT_COLLECT_SAMPLE_DELAY_MS 180000) # Delay between two consecutive NAT collection steps >> ..\android.config
echo set(NAT_COLLECT_UPLOAD_DELAY_MS 30000) # Delay before sample is uploaded to database >> ..\android.config
echo set(NAT_TRAVERSE_ANALYZE_PORTS 1000) # Amount of ports collected during analyze phase >> ..\android.config
echo set(NAT_TRAVERSE_ATTEMPTS 7000) # Number of traversal attempts/created sockets during traversal >> ..\android.config
echo set(NAT_TRAVERSE_DEADLINE_MS 9000) # Deadline, after which traversal attempt counts as failure >> ..\android.config
echo set(MONGO_DB_NAME "NatInfo") # Mongo db name, where collected NAT Samples will be stored >> ..\android.config
echo set(MONGO_NAT_TRAVERSAL_COLL_NAME "traversal") # Mongo collection name, where traversal results are stored >> ..\android.config
echo set(MONGO_NAT_SAMPLES_COLL_NAME "data") # Mongo collection name, where collected NAT Samples will be stored >> ..\android.config
echo set(MONGO_NAT_USERS_COLL_NAME "users") # Mongo collection name, where app user information are stored >> ..\android.config
echo set(MONGO_VERSION_COLL_NAME "VersionUpdate") # Mongo collection name, where new version pop up information are stored >> ..\android.config
echo set(MONGO_INFORMATION_COLL_NAME "InformationUpdate") # Mongo collection name, where general information for popups are stored >> ..\android.config
echo set(RANDOM_SYM_NAT_REQUIRED 1) #If set to 1, ports are only collected if NAT type is random symmetric >> ..\android.config
echo set(TRAVERSAL_FEATURE_ENABLED 0) #If set to 1, the traversal tab shows in main screen >> ..\android.config
echo set(MAX_LOG_LINES 400) # Sets the maximum number of lines in the log, before old messages get discarded >> ..\android.config
