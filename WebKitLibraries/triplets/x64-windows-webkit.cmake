set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE dynamic)

# To temporarily force release libraries due to an issue with a dependency copy
# the following block of code replacing `<port-name>` and `<port-version>` with
# the applicable values.
#[[
set(VCPKG_BUILD_TYPE release)
set(_BAD_PORT "<port-name>")
set(_BAD_VERSION "<port-version>")
message(WARNING "Overriding VCPKG_BUILD_TYPE to ${VCPKG_BUILD_TYPE} due to an issue in ${_BAD_PORT} v${_BAD_VERSION}")

if (PORT STREQUAL _BAD_PORT AND VERSION)
    if (NOT VERSION VERSION_EQUAL _BAD_VERSION)
        message(WARNING 
            "Updating ${PORT} to v${VERSION} from v${_BAD_VERSION}\n"
            "The previous version had an issue forcing release binaries to be used. Perform a Debug build to see if the issue is resolved\n"
            "If it is remove this check; otherwise update _BAD_VERSION to ${VERSION}"
        )
        message(FATAL_ERROR "Comment me out until you determine if the update fixes the issue")
    endif ()
endif ()
]]

# Force curl to be built in release mode
# https://github.com/curl/curl/issues/16174
set(VCPKG_BUILD_TYPE release)
set(_BAD_PORT "curl")
set(_BAD_VERSION "8.11.1")
message(WARNING "Overriding VCPKG_BUILD_TYPE to ${VCPKG_BUILD_TYPE} due to an issue in ${_BAD_PORT} v${_BAD_VERSION}")

if (PORT STREQUAL _BAD_PORT AND VERSION)
    if (NOT VERSION VERSION_EQUAL _BAD_VERSION)
        message(WARNING 
            "Updating ${PORT} to v${VERSION} from v${_BAD_VERSION}\n"
            "The previous version had an issue forcing release binaries to be used. Perform a Debug build to see if the issue is resolved\n"
            "If it is remove this check; otherwise update _BAD_VERSION to ${VERSION}"
        )
        message(FATAL_ERROR "Comment me out until you determine if the update fixes the issue")
    endif ()
endif ()

# The following libraries should always be static
if (PORT STREQUAL "highway")
    set(VCPKG_LIBRARY_LINKAGE static)
elseif (PORT STREQUAL "pixman")
    set(VCPKG_LIBRARY_LINKAGE static)
endif ()

# Turn on zlib compatibility
if (PORT STREQUAL "zlib-ng")
    set(ZLIB_COMPAT ON)
endif ()
