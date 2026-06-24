# SPDX-License-Identifier: MIT
# vcpkg overlay port for jmpxx. A consumer installs it without waiting for the central
# registry: vcpkg install jmpxx --overlay-ports=<path-to>/packaging/vcpkg
#
# jmpxx is header-only, so the port installs the headers and the CMake package config
# and removes the empty debug and library trees. The SHA512 is the hash of the release
# tarball for the tag; set it when a version is tagged. vcpkg prints the expected value
# on the first install attempt, or compute it with vcpkg_hash.
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO DimitryQm/jmpxx
    REF "v${VERSION}"
    SHA512 0
    HEAD_REF main
)

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        -DJMPXX_BUILD_VERIFY=OFF
        -DJMPXX_BUILD_TESTS=OFF
        -DJMPXX_BUILD_BENCHMARKS=OFF
        -DJMPXX_BUILD_LINT=OFF
        -DJMPXX_INSTALL=ON
)
vcpkg_cmake_install()
vcpkg_cmake_config_fixup(PACKAGE_NAME jmpxx CONFIG_PATH lib/cmake/jmpxx)

# Header-only: no debug tree and no compiled libraries to keep.
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug" "${CURRENT_PACKAGES_DIR}/lib")
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
