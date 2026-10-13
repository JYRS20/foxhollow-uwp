set(FOXHOLLOW_UWP_PUBLISHER "CN=FoxhollowPersonal" CACHE STRING "")
configure_file("${CMAKE_CURRENT_LIST_DIR}/Package.appxmanifest.in"
    "${CMAKE_BINARY_DIR}/AppxManifest.xml" @ONLY)
set(FOXHOLLOW_UWP_PLATFORM_DIR "${CMAKE_CURRENT_LIST_DIR}")

function(foxhollow_uwp_package target)
    set_target_properties(${target} PROPERTIES OUTPUT_NAME foxhollow)
    set(stage "${CMAKE_BINARY_DIR}/appx/$<CONFIG>")
    add_custom_target(foxhollow-uwp-stage
        COMMAND ${CMAKE_COMMAND} -E make_directory "${stage}"
        COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:${target}>" "${stage}/foxhollow.exe"
        COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_BINARY_DIR}/AppxManifest.xml" "${stage}/AppxManifest.xml"
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${FOXHOLLOW_UWP_PLATFORM_DIR}/Assets" "${stage}/Assets"
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${FOXHOLLOW_UWP_DEP_ROOT}/lic" "${stage}/Licenses"
        COMMAND ${CMAKE_COMMAND} -E copy "${FOXHOLLOW_UWP_DEP_ROOT}/x64/include/zlib.h" "${stage}/Licenses/zlib.h"
        COMMAND ${CMAKE_COMMAND} -E copy "${FOXHOLLOW_UWP_DEP_ROOT}/x64/include/libpng16/png.h" "${stage}/Licenses/png.h"
        DEPENDS ${target} VERBATIM)
    foreach (dll IN ITEMS SDL3 libuwp webgpu_dawn dxcompiler dxil nod libpng16 z)
        add_custom_command(TARGET foxhollow-uwp-stage POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy "${FOXHOLLOW_UWP_DEP_ROOT}/x64/bin/${dll}.dll" "${stage}/${dll}.dll"
            VERBATIM)
    endforeach ()
    if (TARGET game)
        set(_license_files
            "${CMAKE_SOURCE_DIR}/LICENSE|Foxhollow-LICENSE.txt"
            "${CMAKE_SOURCE_DIR}/extern/aurora/LICENSE|Aurora-LICENSE.txt"
            "${CMAKE_SOURCE_DIR}/extern/musyx/LICENSE|Musyx-LICENSE.txt"
            "${CMAKE_SOURCE_DIR}/extern/aurora/include/magic_enum.hpp|magic_enum.hpp")
        foreach (entry IN ITEMS "abseil-cpp|LICENSE" "fmt|LICENSE" "xxhash|LICENSE"
                "imgui|LICENSE.txt" "zstd|LICENSE" "tracy|LICENSE"
                "freetype|LICENSE.TXT" "freetype|docs/FTL.TXT"
                "freetype|src/bdf/README" "freetype|src/pcf/README")
            string(REPLACE "|" ";" fields "${entry}")
            list(GET fields 0 dependency)
            list(GET fields 1 filename)
            FetchContent_GetProperties(${dependency} SOURCE_DIR dependency_source)
            string(REPLACE "/" "-" output_name "${filename}")
            list(APPEND _license_files "${dependency_source}/${filename}|${dependency}-${output_name}")
        endforeach ()
        get_property(jpeg_source TARGET foxhollow_jpeg PROPERTY _EP_SOURCE_DIR)
        list(APPEND _license_files "${jpeg_source}/LICENSE.md|JPEG-LICENSE.md"
            "${jpeg_source}/README.ijg|JPEG-README.ijg")
        foreach (entry IN LISTS _license_files)
            string(REPLACE "|" ";" fields "${entry}")
            list(GET fields 0 source)
            list(GET fields 1 destination)
            add_custom_command(TARGET foxhollow-uwp-stage POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy "${source}" "${stage}/Licenses/${destination}"
                VERBATIM)
        endforeach ()
    endif ()
    set(FOXHOLLOW_MAKEAPPX "" CACHE FILEPATH "")
    if (NOT FOXHOLLOW_MAKEAPPX)
        find_program(FOXHOLLOW_MAKEAPPX_FOUND MakeAppx.exe
            HINTS "$ENV{WindowsSdkVerBinPath}/x64" "$ENV{WindowsSdkDir}/bin/$ENV{WindowsSDKVersion}/x64")
        set(FOXHOLLOW_MAKEAPPX "${FOXHOLLOW_MAKEAPPX_FOUND}")
    endif ()
    if (FOXHOLLOW_MAKEAPPX AND EXISTS "${FOXHOLLOW_MAKEAPPX}")
        add_custom_target(foxhollow-uwp-package
            COMMAND "${FOXHOLLOW_MAKEAPPX}" pack /o /d "${stage}" /p "${CMAKE_BINARY_DIR}/foxhollow-$<CONFIG>-x64.appx"
            DEPENDS foxhollow-uwp-stage VERBATIM)
    endif ()
endfunction()
