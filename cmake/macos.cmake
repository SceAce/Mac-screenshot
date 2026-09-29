set(MARK_SHOT_MACOS_BUNDLE_ID "io.github.scemac.MarkShot" CACHE STRING "macOS bundle identifier")
target_compile_definitions(mark-shot PRIVATE MARK_SHOT_MACOS_BUNDLE_ID="${MARK_SHOT_MACOS_BUNDLE_ID}")
set_target_properties(mark-shot PROPERTIES
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_GUI_IDENTIFIER "${MARK_SHOT_MACOS_BUNDLE_ID}"
    MACOSX_BUNDLE_BUNDLE_NAME "Mark Shot"
    MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
    MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/packaging/macos/Info.plist.in"
    INSTALL_RPATH "@executable_path/../Frameworks"
)
target_sources(mark-shot PRIVATE
    src/platform/macos/macos_capture.h
    src/platform/macos/macos_capture.mm
    src/platform/macos/macos_permissions.mm
    src/platform/macos/macos_capture_compositor.cpp
    src/platform/macos/macos_capture_compositor.h
    src/platform/macos/macos_clipboard.h
    src/platform/macos/macos_clipboard.mm
    src/platform/macos/macos_window_detection.h
    src/platform/macos/macos_window_detection.mm
)
set_source_files_properties(
    src/platform/macos/macos_capture.mm
    src/platform/macos/macos_permissions.mm
    src/platform/macos/macos_clipboard.mm
    src/platform/macos/macos_window_detection.mm
    src/providers/ocr/ocr_vision_task.mm
    PROPERTIES COMPILE_OPTIONS "-fobjc-arc"
)
find_library(MARK_SHOT_SCREENCAPTUREKIT ScreenCaptureKit REQUIRED)
find_library(MARK_SHOT_APPKIT AppKit REQUIRED)
find_library(MARK_SHOT_COREGRAPHICS CoreGraphics REQUIRED)
find_library(MARK_SHOT_VISION Vision REQUIRED)

function(mark_shot_add_macos_window_detection target_name)
    target_sources(${target_name} PRIVATE
        src/platform/macos/macos_window_detection.h
        src/platform/macos/macos_window_detection.mm
    )
    target_link_libraries(${target_name} PRIVATE ${MARK_SHOT_COREGRAPHICS})
endfunction()

target_link_libraries(mark-shot PRIVATE
    ${MARK_SHOT_SCREENCAPTUREKIT}
    ${MARK_SHOT_APPKIT}
    ${MARK_SHOT_COREGRAPHICS}
    ${MARK_SHOT_VISION}
)

set(MARK_SHOT_MACOS_ICON "${CMAKE_CURRENT_SOURCE_DIR}/packaging/macos/mark-shot.icns")
set_source_files_properties("${MARK_SHOT_MACOS_ICON}" PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
target_sources(mark-shot PRIVATE "${MARK_SHOT_MACOS_ICON}")
