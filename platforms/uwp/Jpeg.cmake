include(ExternalProject)
set(FOXHOLLOW_JPEG_INSTALL "${CMAKE_BINARY_DIR}/jpeg-install")
file(MAKE_DIRECTORY "${FOXHOLLOW_JPEG_INSTALL}/include")
set(_jpeg_args
    "-DCMAKE_INSTALL_PREFIX=${FOXHOLLOW_JPEG_INSTALL}"
    "-DCMAKE_INSTALL_LIBDIR=lib"
    "-DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}"
    "-DCMAKE_MAKE_PROGRAM=${CMAKE_MAKE_PROGRAM}"
    "-DCMAKE_SYSTEM_NAME=${CMAKE_SYSTEM_NAME}"
    "-DCMAKE_SYSTEM_VERSION=${CMAKE_SYSTEM_VERSION}"
    "-DCMAKE_SYSTEM_PROCESSOR=AMD64"
    "-DCMAKE_C_FLAGS=/DWINAPI_FAMILY=WINAPI_FAMILY_APP"
    "-DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreadedDLL"
    "-DCMAKE_BUILD_TYPE=Release"
    -DENABLE_SHARED=OFF -DENABLE_STATIC=ON -DWITH_TURBOJPEG=OFF
    -DWITH_TOOLS=OFF -DWITH_TESTS=OFF -DWITH_SIMD=OFF -DWITH_CRT_DLL=ON)
ExternalProject_Add(foxhollow_jpeg
    BINARY_DIR "${CMAKE_BINARY_DIR}/jpeg-build-uwp-x64"
    URL https://github.com/libjpeg-turbo/libjpeg-turbo/archive/refs/tags/3.1.0.tar.gz
    URL_HASH SHA256=35fec2e1ddfb05ecf6d93e50bc57c1e54bc81c16d611ddf6eff73fff266d8285
    DOWNLOAD_EXTRACT_TIMESTAMP FALSE
    CMAKE_ARGS ${_jpeg_args}
    BUILD_BYPRODUCTS "${FOXHOLLOW_JPEG_INSTALL}/lib/jpeg-static.lib")
add_library(foxhollow_jpeg_static STATIC IMPORTED GLOBAL)
set_target_properties(foxhollow_jpeg_static PROPERTIES
    IMPORTED_LOCATION "${FOXHOLLOW_JPEG_INSTALL}/lib/jpeg-static.lib"
    INTERFACE_INCLUDE_DIRECTORIES "${FOXHOLLOW_JPEG_INSTALL}/include")
add_dependencies(foxhollow_jpeg_static foxhollow_jpeg)
add_library(JPEG::JPEG ALIAS foxhollow_jpeg_static)
