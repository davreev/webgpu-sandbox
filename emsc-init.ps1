# EMSDK_DIR is expected to be the absolute path of the Emscripten SDK root dir

# Install and activate sdks/tools
$EMSDK_VERSION="4.0.14"
& $EMSDK_DIR/emsdk.ps1 install $EMSDK_VERSION
& $EMSDK_DIR/emsdk.ps1 activate $EMSDK_VERSION

# Setup environment variables
. $EMSDK_DIR/emsdk_env.ps1
