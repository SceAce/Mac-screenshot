# OpenAI-compatible 翻译插件：复用现有 translation 配置与环境变量

# qt_add_plugin 在 Qt 6.2 会忽略位置参数形式的源文件列表，链接出没有插件符号的空库，
# 源文件必须用 target_sources 显式添加
qt_add_plugin(mark-shot-translate-openai SHARED)

target_sources(mark-shot-translate-openai PRIVATE
    openai_translate_config.cpp
    openai_translate_config.h
    openai_translate_plugin.cpp
    openai_translate_plugin.h
    openai_translation_parser.cpp
    openai_translation_parser.h
)

target_include_directories(mark-shot-translate-openai PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/plugin-sdk
)
target_link_libraries(mark-shot-translate-openai
    PRIVATE
        mark-shot-translate-common
        Qt6::Core
        Qt6::Network
)

install(TARGETS mark-shot-translate-openai
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/mark-shot/plugins
        RUNTIME DESTINATION ${CMAKE_INSTALL_LIBDIR}/mark-shot/plugins)

message(STATUS "mark-shot: translate-openai plugin enabled")
