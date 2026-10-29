# Windows targets, included from CMakeLists.txt when WIN32 (and not PICO_BUILD):
#   tff.exe   console tool and remapper (same commands as tff on Linux)
#   tffw.exe  remapper with a tray icon, no console window (for autostart)
#   tests     test_windows_platform (Win32 glue), tff_e2e (real hook + SendInput)
#   packages  ZIP (default) and MSI (`cpack -G WIX`, needs the WiX .NET tool)

enable_testing()

set(TFF_WIN_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/platform/windows)

add_library(windows_platform
    ${TFF_WIN_DIR}/windows_platform.cpp
    ${TFF_WIN_DIR}/win_util.cpp
)
target_include_directories(windows_platform PUBLIC ${TFF_WIN_DIR})
target_link_libraries(windows_platform PUBLIC tff_core win_input user32 gdi32 shell32 ole32 advapi32 wtsapi32 winmm)

# Icons, version info and the manifest (UTF-8 code page, Per-Monitor DPI awareness)
set(TFF_WIN_RESOURCES
    ${TFF_WIN_DIR}/tff.rc
    ${TFF_WIN_DIR}/tff.manifest
)

add_executable(tff
    ${TFF_WIN_DIR}/main_windows.cpp
    ${TFF_WIN_RESOURCES}
)
target_link_libraries(tff windows_platform)

add_executable(tffw WIN32
    ${TFF_WIN_DIR}/tray_windows.cpp
    ${TFF_WIN_RESOURCES}
)
target_link_libraries(tffw windows_platform)
if(MSVC)
    # wWinMain entry point
    target_link_options(tffw PRIVATE /ENTRY:wWinMainCRTStartup)
endif()

# Tests
add_executable(test_windows_platform
    tests/test_windows_platform.cpp
    ${TFF_WIN_DIR}/tff.manifest
)
target_link_libraries(test_windows_platform windows_platform)
add_test(NAME test_windows_platform COMMAND test_windows_platform)

# End-to-end test: injects keys with SendInput while tff.exe runs and checks what tff.exe
# injects back. Needs an interactive desktop; exits with 77 (skipped) when there is none.
add_executable(tff_e2e
    tests/windows/tff_e2e.cpp
    ${TFF_WIN_DIR}/tff.manifest
)
target_link_libraries(tff_e2e win_input tff_core user32 winmm)
add_dependencies(tff_e2e tff)
add_test(NAME test_windows_e2e
    COMMAND tff_e2e $<TARGET_FILE:tff> ${CMAKE_CURRENT_SOURCE_DIR}/tests/windows/e2e.yaml)
set_tests_properties(test_windows_e2e PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 120 RUN_SERIAL ON)

set(TFF_WINDOWS_TEST_TARGETS test_windows_platform tff_e2e)

# Installation layout: everything in one folder, default config next to the programs
install(TARGETS tff tffw RUNTIME DESTINATION .)
install(FILES config/tff-combos.yaml DESTINATION .)
install(FILES README.md docs/windows.md DESTINATION doc)
install(FILES LICENSE DESTINATION . RENAME LICENSE.txt)

set(CPACK_PACKAGE_NAME "tff2")
set(CPACK_PACKAGE_VENDOR "Thomas Guettler")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Ten Flying Fingers (TFF) Keyboard Remapper")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/guettli/tff2")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "TFF2")
set(CPACK_PACKAGE_FILE_NAME "tff2-${PROJECT_VERSION}-windows-x64")
set(CPACK_GENERATOR "ZIP")

# MSI (cpack -G WIX): installs into Program Files (CPack WIX doesn't support per-user
# installs). The ZIP is the no-admin option. The upgrade GUID must never change, so new
# versions replace old ones.
set(CPACK_WIX_VERSION 4)
set(CPACK_WIX_INSTALL_SCOPE "perMachine")
# The WiX license page needs a .txt (or .rtf) file
configure_file(LICENSE ${CMAKE_CURRENT_BINARY_DIR}/LICENSE.txt COPYONLY)
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_BINARY_DIR}/LICENSE.txt")
set(CPACK_WIX_UPGRADE_GUID "E758079D-89DE-4B5E-A32E-D129F34FF9F2")
set(CPACK_WIX_PRODUCT_ICON "${TFF_WIN_DIR}/tff_active.ico")
set(CPACK_WIX_PROGRAM_MENU_FOLDER "TFF2")
set(CPACK_WIX_PROPERTY_ARPURLINFOABOUT "https://github.com/guettli/tff2")
set(CPACK_WIX_PATCH_FILE "${CMAKE_CURRENT_SOURCE_DIR}/packaging/windows/wix-patch.xml")
set(CPACK_WIX_EXTENSIONS "WixToolset.Util.wixext")
set_property(INSTALL "tffw.exe" PROPERTY CPACK_START_MENU_SHORTCUTS "TFF2 (Ten Flying Fingers)")
include(CPack)
