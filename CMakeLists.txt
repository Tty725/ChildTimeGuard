cmake_minimum_required(VERSION 3.20)

project(ChildTimeGuard LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# MSVC: use UTF-8 for source and execution charset to avoid C4819 with non-ASCII comments
if(MSVC)
    add_compile_options(/utf-8)
endif()

# Build date for FILEVERSION / FileVersion in ChildTimeGuard.rc (generated at configure time).
string(TIMESTAMP BUILD_DATE_YEAR "%Y")
string(TIMESTAMP BUILD_DATE_MONTH "%m")
string(TIMESTAMP BUILD_DATE_DAY "%d")
string(TIMESTAMP BUILD_DATE_STRING "%Y%m%d")
string(REGEX REPLACE "^0" "" BUILD_DATE_MONTH "${BUILD_DATE_MONTH}")
string(REGEX REPLACE "^0" "" BUILD_DATE_DAY "${BUILD_DATE_DAY}")
if(BUILD_DATE_MONTH STREQUAL "")
    set(BUILD_DATE_MONTH "0")
endif()
if(BUILD_DATE_DAY STREQUAL "")
    set(BUILD_DATE_DAY "0")
endif()
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/res/version_build.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/version_build.h
    @ONLY)

add_executable(ChildTimeGuard WIN32
    src/app/main.cpp
    src/app/App.cpp

    src/utils/TimeUtils.cpp
    src/utils/StringUtils.cpp
    src/utils/Base64Util.cpp

    src/i18n/ResourceManager.cpp

    src/config/AppConfig.cpp
    src/config/ConfigManager.cpp
    src/config/JsonConfigStore.cpp

    src/core/TimeController.cpp

    src/platform/HotkeyManager.cpp
    src/platform/AutoStartManager.cpp
    src/platform/ShutdownController.cpp

    src/ui/ControlDialog.cpp
    src/ui/ConfigDialog.cpp
    src/ui/ReminderDialog.cpp
    src/ui/LockScreenWindow.cpp

    src/res/ChildTimeGuard.rc
)

add_executable(ChildTimeGuardTests
    tests/main.cpp
    tests/TestTimeUtils.cpp
    tests/TestStringUtils.cpp
    tests/TestHashAndConfig.cpp
    tests/TestResourceManager.cpp
    tests/TestJsonConfigStore.cpp
    tests/TestTimeController.cpp

    src/utils/TimeUtils.cpp
    src/utils/StringUtils.cpp
    src/utils/Base64Util.cpp

    src/i18n/ResourceManager.cpp

    src/config/AppConfig.cpp
    src/config/ConfigManager.cpp
    src/config/JsonConfigStore.cpp

    src/core/TimeController.cpp
)

target_include_directories(ChildTimeGuardTests PRIVATE
    src/include
)

target_include_directories(ChildTimeGuard PRIVATE
    src/include
    ${CMAKE_CURRENT_BINARY_DIR}
)

