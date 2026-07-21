SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"

export LD_LIBRARY_PATH="${SCRIPT_DIR}/src/lib/socket:${LD_LIBRARY_PATH}"
export DYLD_LIBRARY_PATH="${SCRIPT_DIR}/src/lib/socket:${DYLD_LIBRARY_PATH}"