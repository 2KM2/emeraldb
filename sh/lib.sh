
PARENT_DIR=$(cd $(dirname $0);cd ..; pwd)
echo ${PARENT_DIR}
LIB=${PARENT_DIR}/build/relase/lib
export LD_LIBRARY_PATH=${LIB}:$LD_LIBRARY_PATH