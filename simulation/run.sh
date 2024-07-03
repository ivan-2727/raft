PROJECT_NAME="RaftSimulation"
BUILD_DIR="build"
if [ ! -d "$BUILD_DIR" ]; then
    mkdir "$BUILD_DIR"
fi
RESULT_DIR="result"
if [ ! -d "$RESULT_DIR" ]; then
    mkdir "$RESULT_DIR"
fi
cd "$BUILD_DIR"
cmake .. || exit
cmake --build . || exit
./"$PROJECT_NAME" 
