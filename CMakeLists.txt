
cmake_minimum_required(VERSION 3.16)

project(player)


set(QT_QML_GENERATE_QMLLS_INI ON)
set(QT_MIN_VERSION "6.10.0")
set(KF6_MIN_VERSION "6.24.0")
set(KDE_COMPILERSETTINGS_LEVEL "6.0.0")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)



find_package(ECM ${KF6_MIN_VERSION} REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${ECM_MODULE_PATH})



include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)

find_package(Plasma REQUIRED)

find_package(Qt6 REQUIRED COMPONENTS
   CONFIG REQUIRED
    Core
    Qml
    Widgets
)

find_package(KF6 ${KF6_MIN_VERSION} REQUIRED COMPONENTS
    Config
    # I18n
)

find_package(Plasma ${PROJECT_DEP_VERSION} REQUIRED)


plasma_install_package(package dev.hunterwhite.player)

add_definitions(-DTRANSLATION_DOMAIN=\"plasma_applet_dev.hunterwhite.player\")


set(player_SRCS
    plugin/player.cpp
    plugin/playerItem.cpp
)

add_library(player SHARED ${player_SRCS})

# target_link_libraries(windowpager
#     Qt::Core
#     Qt::Qml
#     # KF5::Plasma
#     # KF5::I18n
# )

target_link_libraries(player
    PRIVATE
        Qt6::Qml
        Qt6::Quick
        Qt6::Widgets
        KF6::ConfigCore
        Plasma::Plasma
)

qt_add_qml_module(player
    URI dev.hunterwhite.player
    VERSION 1.0
    OUTPUT_DIRECTORY
        ${CMAKE_BINARY_DIR}/dev/hunterwhite/player
    SOURCES
        plugin/player.cpp
        plugin/playerItem.cpp
    QML_FILES
        package/contents/ui/main.qml
        package/contents/ui/configGeneral.qml
    RESOURCES
       package/contents/config/main.xml
)




# set(HOME_DIR $ENV{HOME})

# file(MAKE_DIRECTORY ${HOME_DIR}/.local/share/window-pager)


install(TARGETS player DESTINATION ${KDE_INSTALL_QMLDIR}/dev/hunterwhite/player)
install(FILES plugin/qmldir DESTINATION ${KDE_INSTALL_QMLDIR}/dev/hunterwhite/player)
