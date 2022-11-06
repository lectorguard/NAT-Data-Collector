wget https://github.com/mongodb/mongo-c-driver/releases/download/1.23.1/mongo-c-driver-1.23.1.tar.gz
tar xzf mongo-c-driver-1.23.1.tar.gz
rm mongo-c-driver-1.23.1.tar.gz
cd mongo-c-driver-1.23.1
mkdir cmake-build
cd cmake-build
cmake -DENABLE_AUTOMATIC_INIT_AND_CLEANUP=OFF ..
cmake --build .
cmake --build . --target install
