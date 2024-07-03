PROJECT_NAME="RaftServer"
BUILD_DIR="build"
if [ ! -d "$BUILD_DIR" ]; then
    mkdir "$BUILD_DIR"
fi
cd "$BUILD_DIR"
cmake .. || exit
cmake --build . || exit
# ./"$PROJECT_NAME" 
