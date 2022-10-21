git clean -fdX

cmake -G "Unix Makefiles" -B bin/Server
cd bin/Server
make
cd ServerSource
./server-app