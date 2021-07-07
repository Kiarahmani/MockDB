rm -rf build-files
mkdir build-files
cd build-files/

# Build weak-isolation-mock-db
cmake ../ > log.txt

# Build applications
#export CC=/usr/local/opt/llvm/bin/clang  
#export CXX=/usr/local/opt/llvm/bin/clang++

cmake --build ./ --target integer_counter_app > app_log.txt
