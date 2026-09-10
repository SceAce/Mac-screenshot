# 云翻译插件共享基础：配置读取、同步 HTTP、语言归一化与分批
# 仅作为静态库链接进各厂商插件，不单独安装

add_library(mark-shot-translate-common STATIC
    translate_config_source.cpp
    translate_config_source.h
    translate_http_client.cpp
    translate_http_client.h
    translate_language_code.cpp
    translate_language_code.h
    translate_segment_batch.cpp
    translate_segment_batch.h
)

set_target_properties(mark-shot-translate-common PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)

target_include_directories(mark-shot-translate-common PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/plugin-sdk
)

target_link_libraries(mark-shot-translate-common
    PUBLIC
        Qt6::Core
        Qt6::Network
)
