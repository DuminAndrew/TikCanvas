set(SOURCES
    main.cpp
    MainWindow.cpp
    widgets/MapCanvas.cpp
    core/MikroTikManager.cpp
    core/SyslogServer.cpp
    core/MndpScanner.cpp
    core/PdfRenderer.cpp
)

set(HEADERS
    MainWindow.h
    widgets/MapCanvas.h
    core/MikroTikManager.h
    core/SyslogServer.h
    core/MndpScanner.h
    core/PdfRenderer.h
)

set(FORMS
    MainWindow.ui
)

set(RESOURCES
    ${CMAKE_SOURCE_DIR}/resources/resources.qrc
)

qt_add_executable(TikCanvas WIN32 MACOSX_BUNDLE
    ${SOURCES}
    ${HEADERS}
    ${FORMS}
    ${RESOURCES}
)

target_include_directories(TikCanvas PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/core
    ${CMAKE_CURRENT_SOURCE_DIR}/widgets
)

target_link_libraries(TikCanvas PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
    Qt6::Concurrent
)

if(POPPLER_QT6_FOUND)
    target_link_libraries(TikCanvas PRIVATE PkgConfig::POPPLER_QT6)
endif()

set_target_properties(TikCanvas PROPERTIES OUTPUT_NAME "TikCanvas")
