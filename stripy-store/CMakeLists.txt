cmake_minimum_required(VERSION 3.21)
project(stripy-store VERSION 0.2.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(STRIPY_BUILD_APP "Build the Qt application (disable to only run the Qt-free tests)" ON)

# ── Qt-free tests: validation / ranking logic and catalog schema ──
enable_testing()
add_executable(test_logic tests/test_logic.cpp)
add_test(NAME logic COMMAND test_logic)
find_program(PYTHON3 python3)
if(PYTHON3)
    add_test(NAME catalog COMMAND ${PYTHON3} ${CMAKE_SOURCE_DIR}/tools/validate_catalog.py ${CMAKE_SOURCE_DIR}/data/catalog.json)
endif()

if(STRIPY_BUILD_APP)
    find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2)
    qt_standard_project_setup()

    qt_add_executable(stripy-store
        src/main.cpp src/backend.cpp src/backend.h src/appmodel.cpp src/appmodel.h src/logic.h)

    set_source_files_properties(qml/Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)
    qt_add_qml_module(stripy-store URI Stripy VERSION 1.0
        QML_FILES
            qml/Theme.qml qml/Main.qml qml/AppBadge.qml qml/AppCard.qml
            qml/HomePage.qml qml/CategoriesPage.qml qml/AppListPage.qml qml/UpdatesPage.qml
            qml/DetailsPanel.qml qml/SettingsDialog.qml qml/ConfirmDialog.qml qml/OperationBar.qml)

    qt_add_resources(stripy-store data PREFIX "/" FILES data/catalog.json)
    file(GLOB STRIPY_ICONS ${CMAKE_SOURCE_DIR}/assets/icons/*.svg)
    if(STRIPY_ICONS)
        qt_add_resources(stripy-store icons PREFIX "/icons" BASE ${CMAKE_SOURCE_DIR}/assets/icons FILES ${STRIPY_ICONS})
    endif()

    target_compile_definitions(stripy-store PRIVATE STRIPY_VERSION="${PROJECT_VERSION}")
    target_link_libraries(stripy-store PRIVATE Qt6::Core Qt6::Gui Qt6::Qml Qt6::Quick Qt6::QuickControls2)

    install(TARGETS stripy-store RUNTIME DESTINATION bin)
    install(FILES data/stripy-store.desktop DESTINATION share/applications)
endif()
