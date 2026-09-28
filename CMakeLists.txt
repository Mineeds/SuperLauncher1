cmake_minimum_required(VERSION 3.16)
project(SuperLauncher VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 6.4 REQUIRED COMPONENTS Core Gui Widgets Network)

set(SOURCES
    src/main.cpp
    src/core/Paths.cpp
    src/core/Config.cpp
    src/core/Profiles.cpp
    src/core/VersionManager.cpp
    src/core/JavaFinder.cpp
    src/core/Launcher.cpp
    src/net/HttpClient.cpp
    src/auth/MsAuth.cpp
    src/auth/SiteAuth.cpp
    src/auth/TokenStore.cpp
    src/ui/MainWindow.cpp
    src/ui/DashboardContent.cpp
    src/ui/VersionsPage.cpp
    src/ui/ResourcesPage.cpp
    src/ui/ResourceRow.cpp
    src/ui/ServersPage.cpp
    src/ui/SettingsPage.cpp
    src/ui/CabinetPage.cpp
    src/ui/ModsPage.cpp
    src/ui/ModCard.cpp
    src/ui/LoginDialog.cpp
)

set(HEADERS
    src/core/Paths.h
    src/core/Config.h
    src/core/Profiles.h
    src/core/ProfileInfo.h
    src/core/VersionManager.h
    src/core/JavaFinder.h
    src/core/Launcher.h
    src/net/HttpClient.h
    src/auth/MsAuth.h
    src/auth/SiteAuth.h
    src/auth/TokenStore.h
    src/ui/MainWindow.h
    src/ui/DashboardContent.h
    src/ui/VersionsPage.h
    src/ui/ResourcesPage.h
    src/ui/ResourceRow.h
    src/ui/ServersPage.h
    src/ui/SettingsPage.h
    src/ui/CabinetPage.h
    src/ui/ModsPage.h
    src/ui/ModCard.h
    src/ui/LoginDialog.h
)

if (WIN32)
    qt_add_executable(SuperLauncher WIN32 ${SOURCES} ${HEADERS} assets/appicon.rc)
else()
    qt_add_executable(SuperLauncher ${SOURCES} ${HEADERS})
endif()
qt_add_resources(SuperLauncher "app_resources" PREFIX "/" FILES assets/resources.qrc)
target_compile_definitions(SuperLauncher PRIVATE SUPERLAUNCHER_VERSION="${PROJECT_VERSION}")

target_include_directories(SuperLauncher PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(SuperLauncher PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Network)

# QZipReader (private QtGui class) for natives jar extraction.
target_include_directories(SuperLauncher PRIVATE
    /usr/include/x86_64-linux-gnu/qt6/QtGui/6.4.2
    /usr/include/x86_64-linux-gnu/qt6/QtCore/6.4.2)

if (MSVC)
    target_compile_options(SuperLauncher PRIVATE /utf-8)
else()
    target_compile_options(SuperLauncher PRIVATE -Wall -Wextra)
endif()
