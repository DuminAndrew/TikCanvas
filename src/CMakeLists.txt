set(SOURCES
    main.cpp
    MainWindow.cpp
    widgets/MapCanvas.cpp
    widgets/DeviceConfigDialog.cpp
    core/MikroTikManager.cpp
    core/SyslogServer.cpp
    core/MndpScanner.cpp
    core/PdfRenderer.cpp
    core/ProjectFile.cpp
    core/BackupManager.cpp
    core/SshLauncher.cpp
    core/I18n.cpp
    core/ErrorAdvisor.cpp
    core/Vectorizer.cpp
)

set(HEADERS
    MainWindow.h
    widgets/MapCanvas.h
    widgets/DeviceConfigDialog.h
    widgets/DeviceListWidget.h
    core/MikroTikManager.h
    core/SyslogServer.h
    core/MndpScanner.h
    core/PdfRenderer.h
    core/ProjectFile.h
    core/BackupManager.h
    core/SshLauncher.h
    core/I18n.h
    core/ErrorAdvisor.h
    core/Vectorizer.h
)

set(FORMS
    MainWindow.ui
)

set(RESOURCES
    ${CMAKE_SOURCE_DIR}/resources/resources.qrc
)

if(WIN32)
    set(APP_RC ${CMAKE_CURRENT_SOURCE_DIR}/app.rc)
endif()

qt_add_executable(TikCanvas WIN32 MACOSX_BUNDLE
    ${SOURCES}
    ${HEADERS}
    ${FORMS}
    ${RESOURCES}
    ${APP_RC}
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
    Qt6::Pdf
)

set_target_properties(TikCanvas PROPERTIES OUTPUT_NAME "TikCanvas")
