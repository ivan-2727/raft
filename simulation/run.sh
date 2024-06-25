PROJECT_NAME="RaftSimulation"
BUILD_DIR="build"
if [ ! -d "$BUILD_DIR" ]; then
    mkdir "$BUILD_DIR"
fi
cmake || exit
cmake --build ./"$BUILD_DIR" || exit
./"$BUILD_DIR"/"$PROJECT_NAME" 
