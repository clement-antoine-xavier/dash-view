cmake_minimum_required(VERSION 3.24)

project(DashView VERSION 0.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Quick Bluetooth Test)

qt_standard_project_setup(REQUIRES 6.5)

# -----------------------------------------------------------------------------
# Core: tagged telemetry model, mock/replay sources, recorder and vehicle state.
# Kept GUI- and Bluetooth-free so it can be tested headlessly.
# -----------------------------------------------------------------------------
add_library(dashview_core STATIC
    src/telemetry/signaltag.h
    src/telemetry/signaltag.cpp
    src/telemetry/telemetryvalue.h
    src/telemetry/telemetrysource.h
    src/telemetry/telemetrysource.cpp
    src/telemetry/telemetrycodec.h
    src/telemetry/telemetrycodec.cpp
    src/telemetry/mocktelemetrysource.h
    src/telemetry/mocktelemetrysource.cpp
    src/telemetry/replaytelemetrysource.h
    src/telemetry/replaytelemetrysource.cpp
    src/telemetry/telemetryrecorder.h
    src/telemetry/telemetryrecorder.cpp
    src/vehicle/vehiclestate.h
    src/vehicle/vehiclestate.cpp
)

target_include_directories(dashview_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(dashview_core PUBLIC Qt6::Core)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(dashview_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

# -----------------------------------------------------------------------------
# Application
# -----------------------------------------------------------------------------
qt_add_executable(dash_view
    src/main.cpp
    src/telemetry/bletelemetrysource.h
    src/telemetry/bletelemetrysource.cpp
)

set_source_files_properties(qml/Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

# QML sources live in qml/; use the current policy so Qt generates the
# per-directory qmldir files itself and the build stays warning-free.
if(COMMAND qt_policy)
    qt_policy(SET QTP0004 NEW)
endif()

qt_add_qml_module(dash_view
    URI DashView
    VERSION 1.0
    QML_FILES
        qml/Main.qml
        qml/Theme.qml
        qml/Dashboard.qml
        qml/DebugOverlay.qml
        qml/ArcGauge.qml
        qml/BarGauge.qml
        qml/GForcePad.qml
        qml/Panel.qml
        qml/ShiftLights.qml
        qml/StatusPill.qml
        qml/SteeringIndicator.qml
        qml/Tachometer.qml
        qml/Telltale.qml
        qml/Telltales.qml
        qml/WheelCell.qml
        qml/WheelSpeeds.qml
)

target_link_libraries(dash_view PRIVATE dashview_core Qt6::Quick Qt6::Gui Qt6::Bluetooth)

set_target_properties(dash_view PROPERTIES
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_BUNDLE_NAME "Dash View"
    MACOSX_BUNDLE_GUI_IDENTIFIER "dev.dashview.app"
    MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
    MACOSX_BUNDLE_COPYRIGHT "© 2026 Dash View"
    MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Info.plist.in"
)

include(CTest)
if(BUILD_TESTING)
    add_executable(tst_telemetry tests/tst_telemetry.cpp)
    target_link_libraries(tst_telemetry PRIVATE dashview_core Qt6::Test)
    add_test(NAME telemetry COMMAND tst_telemetry)
endif()
