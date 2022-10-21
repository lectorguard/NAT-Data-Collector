rm -r -f bin/Server
cmake -G "Unix Makefiles" -B bin/Server
cd bin/Server
make
cd ServerSource
./server-app