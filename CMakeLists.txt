# SPDX-License-Identifier: LGPL-3.0-or-later
cmake_minimum_required(VERSION 3.16)
project(Lantern VERSION 2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets WebEngineWidgets Network)

set(SOURCES
    src/main.cpp
    src/MainWindow.cpp
    src/BrowserTab.cpp
    src/HistoryManager.cpp
    src/BookmarkManager.cpp
    src/DownloadManager.cpp
    src/AiAssistant.cpp
    src/Settings.cpp
    src/WelcomePage.cpp
)

set(HEADERS
    src/MainWindow.h
    src/BrowserTab.h
    src/HistoryManager.h
    src/BookmarkManager.h
    src/DownloadManager.h
    src/AiAssistant.h
    src/Settings.h
    src/WelcomePage.h
)

add_executable(Lantern ${SOURCES} ${HEADERS})

target_link_libraries(Lantern PRIVATE
    Qt6::Widgets
    Qt6::WebEngineWidgets
    Qt6::Network
)
