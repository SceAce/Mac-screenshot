qt_add_executable(mark-shot-translate-segments-test
    tests/translate_segments_test.cpp
    src/providers/translate/translate_segments.cpp
    src/providers/translate/translate_segments.h
    src/ocr_result.cpp
    src/ocr_result.h
)
target_include_directories(mark-shot-translate-segments-test PRIVATE src)
target_link_libraries(mark-shot-translate-segments-test
    PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Test
)
add_test(NAME translate-segments COMMAND mark-shot-translate-segments-test)

qt_add_executable(mark-shot-ocr-provider-factory-test
    tests/ocr_provider_factory_test.cpp
    src/debug_log.cpp
    src/debug_log.h
    src/providers/ocr/ocr_plugin_task.cpp
    src/providers/ocr/ocr_plugin_task.h
    src/providers/ocr/ocr_provider_factory.cpp
    src/providers/ocr/ocr_provider_factory.h
    src/providers/ocr/ocr_tesseract_task.cpp
    src/providers/ocr/ocr_tesseract_task.h
    src/providers/provider_plugin_registry.cpp
    src/providers/provider_plugin_registry.h
    src/providers/provider_plugin_paths.cpp
    src/providers/provider_plugin_paths.h
    src/providers/provider_process_task.cpp
    src/providers/provider_process_task.h
    src/providers/provider_task.cpp
    src/providers/provider_task.h
    src/shell_command.cpp
    src/shell_command.h
)
target_include_directories(mark-shot-ocr-provider-factory-test PRIVATE src plugin-sdk)
target_link_libraries(mark-shot-ocr-provider-factory-test
    PRIVATE
        Qt6::Core
        Qt6::Concurrent
        Qt6::Gui
        Qt6::Test
)
add_test(NAME ocr-provider-factory COMMAND mark-shot-ocr-provider-factory-test)

qt_add_executable(mark-shot-rapid-ocr-word-segments-test
    tests/rapid_ocr_word_segments_test.cpp
    plugins/ocr-rapid/rapid_ocr_word_segments.cpp
    plugins/ocr-rapid/rapid_ocr_word_segments.h
)
target_include_directories(mark-shot-rapid-ocr-word-segments-test PRIVATE
    plugins/ocr-rapid
)
target_link_libraries(mark-shot-rapid-ocr-word-segments-test
    PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Test
)
add_test(NAME rapid-ocr-word-segments COMMAND mark-shot-rapid-ocr-word-segments-test)

if(TARGET mark-shot-ocr-rapid)
    # pkg_check_modules 的 IMPORTED target 为目录作用域，测试目录内重新探测
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(OnnxRuntimeTest QUIET IMPORTED_TARGET libonnxruntime)
    endif()
    if(NOT OnnxRuntimeTest_FOUND)
        find_package(onnxruntime CONFIG QUIET)
    endif()
endif()

if(TARGET mark-shot-ocr-rapid AND (OnnxRuntimeTest_FOUND OR onnxruntime_FOUND))
    qt_add_executable(mark-shot-ocr-rapid-plugin-test
        tests/ocr_rapid_plugin_test.cpp
        plugins/ocr-rapid/rapid_det_model.cpp
        plugins/ocr-rapid/rapid_det_model.h
        plugins/ocr-rapid/rapid_model_paths.cpp
        plugins/ocr-rapid/rapid_model_paths.h
        plugins/ocr-rapid/rapid_ocr_plugin.cpp
        plugins/ocr-rapid/rapid_ocr_plugin.h
        plugins/ocr-rapid/rapid_ocr_word_segments.cpp
        plugins/ocr-rapid/rapid_ocr_word_segments.h
        plugins/ocr-rapid/rapid_onnx_session.cpp
        plugins/ocr-rapid/rapid_onnx_session.h
        plugins/ocr-rapid/rapid_rec_model.cpp
        plugins/ocr-rapid/rapid_rec_model.h
    )
    target_include_directories(mark-shot-ocr-rapid-plugin-test PRIVATE
        plugins/ocr-rapid
        plugin-sdk
    )
    target_link_libraries(mark-shot-ocr-rapid-plugin-test
        PRIVATE
            Qt6::Core
            Qt6::Gui
            Qt6::Test
    )
    if(OnnxRuntimeTest_FOUND)
        target_link_libraries(mark-shot-ocr-rapid-plugin-test PRIVATE PkgConfig::OnnxRuntimeTest)
    else()
        target_link_libraries(mark-shot-ocr-rapid-plugin-test PRIVATE onnxruntime::onnxruntime)
    endif()
    add_test(NAME ocr-rapid-plugin COMMAND mark-shot-ocr-rapid-plugin-test)
endif()

if(TARGET mark-shot-translate-openai)
    qt_add_executable(mark-shot-translate-openai-plugin-test
        tests/translate_openai_plugin_test.cpp
        plugins/translate-openai/openai_translate_config.cpp
        plugins/translate-openai/openai_translate_config.h
        plugins/translate-openai/openai_translate_plugin.cpp
        plugins/translate-openai/openai_translate_plugin.h
        plugins/translate-openai/openai_translation_parser.cpp
        plugins/translate-openai/openai_translation_parser.h
        src/providers/provider_task.cpp
        src/providers/provider_task.h
        src/providers/translate/translate_openai_task.cpp
        src/providers/translate/translate_openai_task.h
        src/providers/translate/translate_segments.cpp
        src/ocr_result.cpp
    )
    target_include_directories(mark-shot-translate-openai-plugin-test PRIVATE
        plugins/translate-openai
        plugin-sdk
        src
    )
    target_link_libraries(mark-shot-translate-openai-plugin-test
        PRIVATE
            mark-shot-translate-common
            Qt6::Core
            Qt6::Gui
            Qt6::Network
            Qt6::Test
    )
    add_test(NAME translate-openai-plugin COMMAND mark-shot-translate-openai-plugin-test)
endif()

if(TARGET mark-shot-translate-tencent)
    qt_add_executable(mark-shot-translate-tencent-plugin-test
        tests/translate_tencent_plugin_test.cpp
        plugins/translate-tencent/tencent_tc3_signer.cpp
        plugins/translate-tencent/tencent_tc3_signer.h
        plugins/translate-tencent/tencent_translate_config.cpp
        plugins/translate-tencent/tencent_translate_config.h
        plugins/translate-tencent/tencent_translate_plugin.cpp
        plugins/translate-tencent/tencent_translate_plugin.h
        plugins/translate-tencent/tencent_translate_request.cpp
        plugins/translate-tencent/tencent_translate_request.h
    )
    target_include_directories(mark-shot-translate-tencent-plugin-test PRIVATE
        plugins/translate-tencent
        plugin-sdk
    )
    target_link_libraries(mark-shot-translate-tencent-plugin-test
        PRIVATE
            mark-shot-translate-common
            Qt6::Core
            Qt6::Network
            Qt6::Test
    )
    add_test(NAME translate-tencent-plugin COMMAND mark-shot-translate-tencent-plugin-test)
endif()

if(TARGET mark-shot-translate-baidu)
    qt_add_executable(mark-shot-translate-baidu-plugin-test
        tests/translate_baidu_plugin_test.cpp
        plugins/translate-baidu/baidu_translate_config.cpp
        plugins/translate-baidu/baidu_translate_config.h
        plugins/translate-baidu/baidu_translate_plugin.cpp
        plugins/translate-baidu/baidu_translate_plugin.h
        plugins/translate-baidu/baidu_translate_request.cpp
        plugins/translate-baidu/baidu_translate_request.h
        plugins/translate-baidu/baidu_translate_signer.cpp
        plugins/translate-baidu/baidu_translate_signer.h
    )
    target_include_directories(mark-shot-translate-baidu-plugin-test PRIVATE
        plugins/translate-baidu
        plugin-sdk
    )
    target_link_libraries(mark-shot-translate-baidu-plugin-test
        PRIVATE
            mark-shot-translate-common
            Qt6::Core
            Qt6::Network
            Qt6::Test
    )
    add_test(NAME translate-baidu-plugin COMMAND mark-shot-translate-baidu-plugin-test)
endif()

if(TARGET mark-shot-translate-youdao)
    qt_add_executable(mark-shot-translate-youdao-plugin-test
        tests/translate_youdao_plugin_test.cpp
        plugins/translate-youdao/youdao_translate_config.cpp
        plugins/translate-youdao/youdao_translate_config.h
        plugins/translate-youdao/youdao_translate_plugin.cpp
        plugins/translate-youdao/youdao_translate_plugin.h
        plugins/translate-youdao/youdao_translate_request.cpp
        plugins/translate-youdao/youdao_translate_request.h
        plugins/translate-youdao/youdao_translate_signer.cpp
        plugins/translate-youdao/youdao_translate_signer.h
    )
    target_include_directories(mark-shot-translate-youdao-plugin-test PRIVATE
        plugins/translate-youdao
        plugin-sdk
    )
    target_link_libraries(mark-shot-translate-youdao-plugin-test
        PRIVATE
            mark-shot-translate-common
            Qt6::Core
            Qt6::Network
            Qt6::Test
    )
    add_test(NAME translate-youdao-plugin COMMAND mark-shot-translate-youdao-plugin-test)
endif()

if(TARGET mark-shot-code-scan-zxing AND MARK_SHOT_ZXING_WRITER_SUPPORTED)
    qt_add_executable(mark-shot-code-scan-zxing-plugin-test
        tests/code_scan_zxing_plugin_test.cpp
        plugins/code-scan-zxing/zxing_code_scan_plugin.cpp
        plugins/code-scan-zxing/zxing_code_scan_plugin.h
    )
    target_include_directories(mark-shot-code-scan-zxing-plugin-test PRIVATE
        plugins/code-scan-zxing
        plugin-sdk
    )
    target_compile_definitions(mark-shot-code-scan-zxing-plugin-test PRIVATE ${MARK_SHOT_ZXING_COMPILE_DEFINITIONS})
    target_link_libraries(mark-shot-code-scan-zxing-plugin-test
        PRIVATE
            Qt6::Core
            Qt6::Gui
            Qt6::Test
    )
    if(ZXing_FOUND)
        target_link_libraries(mark-shot-code-scan-zxing-plugin-test PRIVATE ZXing::ZXing)
    elseif(ZXingCpp_FOUND)
        target_link_libraries(mark-shot-code-scan-zxing-plugin-test PRIVATE PkgConfig::ZXingCpp)
    elseif(TARGET PkgConfig::ZXingPlugin)
        target_link_libraries(mark-shot-code-scan-zxing-plugin-test PRIVATE PkgConfig::ZXingPlugin)
    endif()
    add_test(NAME code-scan-zxing-plugin COMMAND mark-shot-code-scan-zxing-plugin-test)
endif()
