# 腾讯云机器翻译（TMT）插件：TC3-HMAC-SHA256 签名 + 批量翻译，批量接口不可用时降级为单段翻译

# qt_add_plugin 在 Qt 6.2 会忽略位置参数形式的源文件列表，链接出没有插件符号的空库，
# 源文件必须用 target_sources 显式添加
qt_add_plugin(mark-shot-translate-tencent SHARED)

target_sources(mark-shot-translate-tencent PRIVATE
    tencent_tc3_signer.cpp
    tencent_tc3_signer.h
    tencent_translate_config.cpp
    tencent_translate_config.h
    tencent_translate_plugin.cpp
    tencent_translate_plugin.h
    tencent_translate_request.cpp
    tencent_translate_request.h
)

target_include_directories(mark-shot-translate-tencent PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/plugin-sdk
)
target_link_libraries(mark-shot-translate-tencent
    PRIVATE
        mark-shot-translate-common
        Qt6::Core
        Qt6::Network
)

install(TARGETS mark-shot-translate-tencent
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/mark-shot/plugins
        RUNTIME DESTINATION ${CMAKE_INSTALL_LIBDIR}/mark-shot/plugins)

message(STATUS "mark-shot: translate-tencent plugin enabled")
