#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 1 ]]; then
    echo "Usage: $0 <platform>"
    exit 1
fi

PLATFORM="$1"
PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${PROJECT_DIR}/build/${PLATFORM}"
PLATFORM_DIR="${PROJECT_DIR}/src/platform/${PLATFORM}"
CONFIG_FILE="${PLATFORM_DIR}/toolchain.conf"

# Check platform configuration
if [[ ! -f "${CONFIG_FILE}" ]]; then
    echo "Error: Platform configuration not found: ${CONFIG_FILE}" >&2
    exit 1
fi

# Load platform-specific toolchain settings
source "${CONFIG_FILE}"

# Validate required settings
: "${TOOLCHAIN_VERSION:?Missing TOOLCHAIN_VERSION in ${CONFIG_FILE}}"
: "${TOOLCHAIN_ARCHIVE:?Missing TOOLCHAIN_ARCHIVE in ${CONFIG_FILE}}"
: "${TOOLCHAIN_URL:?Missing TOOLCHAIN_URL in ${CONFIG_FILE}}"

TOOLCHAIN_DIR="${BUILD_DIR}/toolchain"
TOOLCHAIN_ROOT="${TOOLCHAIN_DIR}/${TOOLCHAIN_ARCHIVE}"
TOOLCHAIN_BIN="${TOOLCHAIN_ROOT}/bin"

mkdir -p "${BUILD_DIR}"

# Download and extract the toolchain if it is missing
if [[ ! -x "${TOOLCHAIN_BIN}/${TOOLCHAIN_PREFIX}-gcc" ||
      ! -x "${TOOLCHAIN_BIN}/${TOOLCHAIN_PREFIX}-g++" ]]; then

    echo "Downloading toolchain ${TOOLCHAIN_VERSION} for ${PLATFORM}..."

    mkdir -p "${TOOLCHAIN_DIR}"

    ARCHIVE_PATH="${TOOLCHAIN_DIR}/${TOOLCHAIN_ARCHIVE}.tar.xz"
    TEMP_ARCHIVE="${ARCHIVE_PATH}.tmp"

    curl --fail --location --retry 3 \
        "${TOOLCHAIN_URL}" -o "${TEMP_ARCHIVE}"

    mv "${TEMP_ARCHIVE}" "${ARCHIVE_PATH}"

    echo "Extracting toolchain..."
    tar -xJf "${ARCHIVE_PATH}" -C "${TOOLCHAIN_DIR}"

    rm -f "${ARCHIVE_PATH}"

    # Verify extraction
    if [[ ! -x "${TOOLCHAIN_BIN}/${TOOLCHAIN_PREFIX}-gcc" ||
          ! -x "${TOOLCHAIN_BIN}/${TOOLCHAIN_PREFIX}-g++" ]]; then
        echo "Error: Toolchain extraction failed or archive layout differs from expected." >&2
        exit 1
    fi
fi

echo "Configuring platform: ${PLATFORM}"
echo "Toolchain version: ${TOOLCHAIN_VERSION}"

# Now is a good time to download the submodules for the specific platforms, 
# before doing anything else.
if command -v git >/dev/null 2>&1; then
    git -C ${PLATFORM_DIR} submodule update --init --recursive
else
    echo "Git is not installed or not available in PATH."
    exit 1
fi

cmake -S "${PROJECT_DIR}/src" -B "${BUILD_DIR}" \
    -DCMAKE_C_COMPILER="${TOOLCHAIN_BIN}/${TOOLCHAIN_PREFIX}-gcc" \
    -DCMAKE_CXX_COMPILER="${TOOLCHAIN_BIN}/${TOOLCHAIN_PREFIX}-g++" \
    -DCMAKE_ASM_COMPILER="${TOOLCHAIN_BIN}/${TOOLCHAIN_PREFIX}-gcc" \
    -DCMAKE_PLATFORM="${PLATFORM}"

cmake --build "${BUILD_DIR}"
