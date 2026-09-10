# 网易有道智云文本翻译插件：复用 translate-common 的配置、HTTP 与分批能力

# qt_add_plugin 在 Qt 6.2 会忽略位置参数形式的源文件列表，链接出没有插件符号的空库，
# 源文件必须用 target_sources 显式添加
qt_add_plugin(mark-shot-translate-youdao SHARED)

target_sources(mark-shot-translate-youdao PRIVATE
    youdao_translate_config.cpp
    youdao_translate_config.h
    youdao_translate_plugin.cpp
    youdao_translate_plugin.h
    youdao_translate_request.cpp
    youdao_translate_request.h
    youdao_translate_signer.cpp
    youdao_translate_signer.h
)

target_include_directories(mark-shot-translate-youdao PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/plugin-sdk
)
target_link_libraries(mark-shot-translate-youdao
    PRIVATE
        mark-shot-translate-common
        Qt6::Core
        Qt6::Network
)

install(TARGETS mark-shot-translate-youdao
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/mark-shot/plugins
        RUNTIME DESTINATION ${CMAKE_INSTALL_LIBDIR}/mark-shot/plugins)

message(STATUS "mark-shot: translate-youdao plugin enabled")
