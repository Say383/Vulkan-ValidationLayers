# ~~~
# Copyright (c) 2014-2023 Valve Corporation
# Copyright (c) 2014-2023 LunarG, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
# ~~~
add_library(VkLayer_utils STATIC)
target_sources(VkLayer_utils PRIVATE
    containers/custom_containers.h
    error_message/logging.h
    error_message/logging.cpp
    external/xxhash.h
    external/xxhash.cpp
    generated/vk_format_utils.h
    generated/vk_format_utils.cpp
    generated/vk_validation_error_messages.h
    generated/vk_layer_dispatch_table.h
    generated/vk_dispatch_table_helper.h
    generated/vk_safe_struct.h
    generated/vk_safe_struct.cpp
    generated/vk_enum_string_helper.h
    generated/vk_object_types.h
    generated/vk_extension_helper.h
    generated/vk_typemap_helper.h
    utils/cast_utils.h
    utils/hash_util.h
    utils/hash_vk_types.h
    utils/vk_layer_extension_utils.cpp
    utils/vk_layer_extension_utils.h
    utils/vk_layer_utils.cpp
    utils/vk_layer_utils.h
    vk_layer_config.h
    vk_layer_config.cpp
)

# XXH_NO_LONG_LONG: removes compilation of algorithms relying on 64-bit types (XXH3 and XXH64). Only XXH32 will be compiled.
# We only need XXH32 due to restrictions requiring a 32 bit hash. This also reduces binary size.
#
# v0.8.1 also has compilation issues that are removed by setting this define.
# https://github.com/KhronosGroup/Vulkan-ValidationLayers/pull/4639
# https://github.com/KhronosGroup/Vulkan-ValidationLayers/issues/4640
target_compile_definitions(VkLayer_utils PUBLIC XXH_NO_LONG_LONG)

target_link_libraries(VkLayer_utils PUBLIC Vulkan::Headers)
target_include_directories(VkLayer_utils SYSTEM PRIVATE external)
target_include_directories(VkLayer_utils PUBLIC .)

find_package(robin_hood CONFIG)
option(USE_ROBIN_HOOD_HASHING "robin_hood provides faster versions of std::unordered_map and std::unordered_set" ${robin_hood_FOUND})
if (USE_ROBIN_HOOD_HASHING)
    target_link_libraries(VkLayer_utils PRIVATE robin_hood::robin_hood)
    target_compile_definitions(robin_hood::robin_hood INTERFACE USE_ROBIN_HOOD_HASHING)
endif()

# TODO: This should be removed once the official "Vulkan-Utility-Libraries" library is available for consumption
if(BUILD_LAYER_SUPPORT_FILES)
    install(FILES ${CMAKE_SOURCE_DIR}/layers/generated/vk_enum_string_helper.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/vulkan)
    install(FILES ${CMAKE_SOURCE_DIR}/layers/containers/custom_containers.h
            DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/vulkan/containers)
    install(DIRECTORY ${CMAKE_SOURCE_DIR}/layers/error_message
            DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/vulkan
            FILES_MATCHING PATTERN "logging.*")
    install(DIRECTORY ${CMAKE_SOURCE_DIR}/layers/external
            DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/vulkan
            FILES_MATCHING PATTERN "xxhash.*")
    install(FILES ${CMAKE_SOURCE_DIR}/layers/generated/vk_format_utils.h
                  ${CMAKE_SOURCE_DIR}/layers/generated/vk_format_utils.cpp
                  ${CMAKE_SOURCE_DIR}/layers/generated/vk_validation_error_messages.h
                  ${CMAKE_SOURCE_DIR}/layers/generated/vk_layer_dispatch_table.h
                  ${CMAKE_SOURCE_DIR}/layers/generated/vk_dispatch_table_helper.h
                  ${CMAKE_SOURCE_DIR}/layers/generated/vk_safe_struct.h
                  ${CMAKE_SOURCE_DIR}/layers/generated/vk_safe_struct.cpp
                  ${CMAKE_SOURCE_DIR}/layers/generated/vk_enum_string_helper.h
                  ${CMAKE_SOURCE_DIR}/layers/generated/vk_object_types.h
                  ${CMAKE_SOURCE_DIR}/layers/generated/vk_extension_helper.h
                  ${CMAKE_SOURCE_DIR}/layers/generated/vk_typemap_helper.h
            DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/vulkan/generated)
    install(FILES ${CMAKE_SOURCE_DIR}/layers/utils/cast_utils.h
                  ${CMAKE_SOURCE_DIR}/layers/utils/hash_util.h
                  ${CMAKE_SOURCE_DIR}/layers/utils/hash_vk_types.h
                  ${CMAKE_SOURCE_DIR}/layers/utils/vk_layer_extension_utils.cpp
                  ${CMAKE_SOURCE_DIR}/layers/utils/vk_layer_extension_utils.h
                  ${CMAKE_SOURCE_DIR}/layers/utils/vk_layer_utils.cpp
                  ${CMAKE_SOURCE_DIR}/layers/utils/vk_layer_utils.h
            DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/vulkan/utils)
    install(FILES vk_layer_config.h
                  vk_layer_config.cpp
            DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/vulkan)
    install(TARGETS VkLayer_utils)
endif()

if(${CMAKE_CXX_COMPILER_ID} MATCHES "Clang")
    target_compile_options(VkLayer_utils PRIVATE
        -Wno-sign-conversion
        -Wno-implicit-int-conversion
    )
endif()

if (NOT BUILD_LAYERS)
    return()
endif()

# Represents all SPIRV libraries we need
add_library(VVL-SPIRV-LIBS INTERFACE)

find_package(SPIRV-Headers REQUIRED CONFIG QUIET)
target_link_libraries(VVL-SPIRV-LIBS INTERFACE SPIRV-Headers::SPIRV-Headers)

find_package(SPIRV-Tools-opt REQUIRED CONFIG QUIET)
target_link_libraries(VVL-SPIRV-LIBS INTERFACE SPIRV-Tools-opt)

find_package(SPIRV-Tools REQUIRED CONFIG QUIET)

# See https://github.com/KhronosGroup/SPIRV-Tools/issues/3909 for background on this.
# The targets available from SPIRV-Tools change depending on how SPIRV_TOOLS_BUILD_STATIC is set.
# Try to handle all possible combinations so that we work with externally built packages.
if (TARGET SPIRV-Tools)
    target_link_libraries(VVL-SPIRV-LIBS INTERFACE SPIRV-Tools)
elseif(TARGET SPIRV-Tools-static)
    target_link_libraries(VVL-SPIRV-LIBS INTERFACE SPIRV-Tools-static)
elseif(TARGET SPIRV-Tools-shared)
    target_link_libraries(VVL-SPIRV-LIBS INTERFACE SPIRV-Tools-shared)
else()
    message(FATAL_ERROR "Cannot determine SPIRV-Tools target name")
endif()

# NOTE: Our custom code generation target isn't desirable for system package managers or add_subdirectory users.
# So this target needs to be off by default to avoid obtuse build errors or patches.
option(VVL_CODEGEN "Enable vulkan validation layer code generation")
if (VVL_CODEGEN)
    find_package(PythonInterp 3 REQUIRED)
    add_custom_target(vvl_codegen
        COMMAND ${CMAKE_COMMAND} -E env PYTHONPATH=${VULKAN_HEADERS_REGISTRY_DIRECTORY} ${PYTHON_EXECUTABLE} "${VVL_SOURCE_DIR}/scripts/generate_source.py"
            ${VULKAN_HEADERS_REGISTRY_DIRECTORY} "${SPIRV_HEADERS_INSTALL_DIR}/include/spirv/unified1"
            --incremental --generated-version ${VulkanHeaders_VERSION}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/generated
    )
endif()

add_library(VkLayer_khronos_validation MODULE)

target_sources(VkLayer_khronos_validation PRIVATE
    best_practices/best_practices_error_enums.h
    best_practices/best_practices_utils.cpp
    best_practices/best_practices_validation.h
    containers/qfo_transfer.h
    containers/range_vector.h
    containers/sparse_containers.h
    containers/subresource_adapter.cpp
    containers/subresource_adapter.h
    core_checks/android_cc_validation.cpp
    core_checks/buffer_cc_validation.cpp
    core_checks/buffer_address_cc_validation.h
    core_checks/cmd_buffer_dynamic_cc_validation.cpp
    core_checks/cmd_buffer_cc_validation.cpp
    core_checks/copy_blit_resolve_cc_validation.cpp
    core_checks/core_validation.h
    core_checks/descriptor_cc_validation.cpp
    core_checks/device_cc_validation.cpp
    core_checks/device_memory_cc_validation.cpp
    core_checks/drawdispatch_cc_validation.cpp
    core_checks/external_object_cc_validation.cpp
    core_checks/image_cc_validation.cpp
    core_checks/image_layout_cc_validation.cpp
    core_checks/pipeline_cc_validation.cpp
    core_checks/query_cc_validation.cpp
    core_checks/queue_cc_validation.cpp
    core_checks/ray_tracing_cc_validation.cpp
    core_checks/render_pass_cc_validation.cpp
    core_checks/shader_cc_validation.cpp
    core_checks/shader_cc_validation.h
    core_checks/synchronization_cc_validation.cpp
    core_checks/video_cc_validation.cpp
    core_checks/wsi_cc_validation.cpp
    core_checks/ycbcr_cc_validation.cpp
    error_message/core_error_location.cpp
    error_message/core_error_location.h
    error_message/validation_error_enums.h
    external/vma/vma.h
    external/vma/vma.cpp
    generated/best_practices.cpp
    generated/best_practices.h
    generated/chassis.cpp
    generated/valid_param_values.cpp
    generated/command_validation.cpp
    generated/enum_flag_bits.h
    generated/layer_chassis_dispatch.cpp
    generated/object_tracker.cpp
    generated/object_tracker.h
    generated/parameter_validation.cpp
    generated/parameter_validation.h
    generated/spirv_grammar_helper.cpp
    generated/spirv_validation_helper.cpp
    generated/sync_validation_types.cpp
    generated/thread_safety.cpp
    generated/thread_safety.h
    generated/vk_safe_struct.cpp
    generated/vk_safe_struct.h
    gpu_validation/debug_printf.cpp
    gpu_validation/debug_printf.h
    gpu_validation/gpu_utils.cpp
    gpu_validation/gpu_utils.h
    gpu_validation/gpu_validation.cpp
    gpu_validation/gpu_validation.cpp
    gpu_validation/gpu_validation.h
    object_tracker/object_lifetime_validation.h
    object_tracker/object_tracker_utils.cpp
    state_tracker/base_node.cpp
    state_tracker/base_node.h
    state_tracker/buffer_state.cpp
    state_tracker/buffer_state.h
    state_tracker/cmd_buffer_state.cpp
    state_tracker/cmd_buffer_state.h
    state_tracker/descriptor_sets.cpp
    state_tracker/descriptor_sets.h
    state_tracker/device_memory_state.cpp
    state_tracker/device_memory_state.h
    state_tracker/device_state.h
    state_tracker/image_layout_map.cpp
    state_tracker/image_layout_map.h
    state_tracker/image_state.cpp
    state_tracker/image_state.h
    state_tracker/pipeline_layout_state.cpp
    state_tracker/pipeline_layout_state.h
    state_tracker/pipeline_state.cpp
    state_tracker/pipeline_state.h
    state_tracker/pipeline_sub_state.cpp
    state_tracker/pipeline_sub_state.h
    state_tracker/query_state.h
    state_tracker/queue_state.cpp
    state_tracker/queue_state.h
    state_tracker/ray_tracing_state.h
    state_tracker/render_pass_state.cpp
    state_tracker/render_pass_state.h
    state_tracker/sampler_state.h
    state_tracker/shader_instruction.cpp
    state_tracker/shader_instruction.h
    state_tracker/shader_module.cpp
    state_tracker/shader_module.h
    state_tracker/state_tracker.cpp
    state_tracker/state_tracker.h
    state_tracker/video_session_state.cpp
    state_tracker/video_session_state.h
    stateless/buffer_sl_validation.cpp
    stateless/cmd_buffer_dynamic_sl_validation.cpp
    stateless/cmd_buffer_sl_validation.cpp
    stateless/descriptor_sl_validation.cpp
    stateless/device_memory_sl_validation.cpp
    stateless/external_object_sl_validation.cpp
    stateless/framebuffer_sl_validation.cpp
    stateless/image_sl_validation.cpp
    stateless/instance_device_sl_validation.cpp
    stateless/pipeline_sl_validation.cpp
    stateless/ray_tracing_sl_validation.cpp
    stateless/render_pass_sl_validation.cpp
    stateless/synchronization_sl_validation.cpp
    stateless/wsi_sl_validation.cpp
    stateless/stateless_validation.h
    sync/sync_validation.cpp
    sync/sync_validation.h
    sync/sync_utils.cpp
    sync/sync_utils.h
    sync/sync_vuid_maps.cpp
    sync/sync_vuid_maps.h
    layer_options.cpp
    utils/convert_to_renderpass2.cpp
    vk_layer_settings_ext.h
)

if(MSVC)
    target_link_options(VkLayer_khronos_validation PRIVATE /DEF:${CMAKE_CURRENT_SOURCE_DIR}/VkLayer_khronos_validation.def)
    target_compile_options(VkLayer_khronos_validation PRIVATE /bigobj)
elseif(MINGW)
    target_sources(VkLayer_khronos_validation PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/VkLayer_khronos_validation.def)
    target_compile_options(VkLayer_khronos_validation PRIVATE -Wa,-mbig-obj)
elseif(APPLE)
    message(DEBUG "Functions are exported via VVL_EXPORT")
    set_target_properties(VkLayer_khronos_validation PROPERTIES SUFFIX ".dylib")
elseif(ANDROID)
    message(DEBUG "Functions are exported via VVL_EXPORT")
else()
    target_link_options(VkLayer_khronos_validation PRIVATE LINKER:--version-script=${CMAKE_CURRENT_SOURCE_DIR}/libVkLayer_khronos_validation.map,-Bsymbolic,--exclude-libs,ALL)
endif()

if(${CMAKE_CXX_COMPILER_ID} MATCHES "(GNU|Clang)")
    target_compile_options(VkLayer_khronos_validation PRIVATE
        -Wno-unused-parameter
    )
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(VkLayer_khronos_validation PRIVATE
            -Wno-sign-conversion
            -Wno-implicit-int-conversion
        )
    endif()
endif()

# Khronos validation additional dependencies
if (USE_ROBIN_HOOD_HASHING)
    target_link_libraries(VkLayer_khronos_validation PRIVATE robin_hood::robin_hood)
endif()
# Order matters here. VkLayer_utils should be the last link library to ensure mimalloc overrides are picked up correctly.
# Otherwise, libraries after VkLayer_utils will not benefit from this performance improvement.
target_link_libraries(VkLayer_khronos_validation PRIVATE VVL-SPIRV-LIBS VkLayer_utils)

# Using mimalloc on non-Windows OSes currently results in unit test instability with some
# OS version / driver combinations. On 32-bit systems, using mimalloc cause an increase in
# the amount of virtual address space needed, which can also cause stability problems.
if (MSVC AND CMAKE_SIZEOF_VOID_P EQUAL 8)
   find_package(mimalloc CONFIG)
   option(USE_MIMALLOC "Use mimalloc, a fast malloc/free replacement library" ${mimalloc_FOUND})
   if (USE_MIMALLOC)
      target_compile_definitions(mimalloc-static INTERFACE USE_MIMALLOC)
      target_link_libraries(VkLayer_khronos_validation PRIVATE mimalloc-static)
   endif()
endif()

target_include_directories(VkLayer_khronos_validation SYSTEM PRIVATE external)

# There are 2 primary deliverables for the validation layers.
# - The actual library VkLayer_khronos_validation.(dll|so|dylib)
# - The respective json file, VkLayer_khronos_validation.json
# This code generates the appropriate json for both local testing and the installation.
# NOTE: For WIN32 the JSON and dll MUST be placed in the same location, due to Win32 using a relative path for installation.
set(INPUT_FILE "${CMAKE_CURRENT_SOURCE_DIR}/VkLayer_khronos_validation.json.in")
set(INTERMEDIATE_FILE "${CMAKE_CURRENT_BINARY_DIR}/json/validation.json")
set(OUTPUT_FILE_FINAL_NAME "VkLayer_khronos_validation.json")
set(LAYER_INSTALL_DIR ${CMAKE_INSTALL_LIBDIR})
if (WIN32)
    set(LAYER_INSTALL_DIR ${CMAKE_INSTALL_BINDIR}) # WIN32/MINGW expect the dll in the `bin` dir, this matches our WIN32 SDK process
endif()

if (WIN32)
    set(JSON_LIBRARY_PATH ".\\\\VkLayer_khronos_validation.dll")
elseif(APPLE)
    set(JSON_LIBRARY_PATH "./libVkLayer_khronos_validation.dylib")
else()
    set(JSON_LIBRARY_PATH "./libVkLayer_khronos_validation.so")
endif()

configure_file(${INPUT_FILE} ${INTERMEDIATE_FILE} @ONLY)

# To support both multi/single configuration generators just copy the json to the correct directory
add_custom_command(TARGET VkLayer_khronos_validation POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${INTERMEDIATE_FILE} $<TARGET_FILE_DIR:VkLayer_khronos_validation>/${OUTPUT_FILE_FINAL_NAME}
)

# For UNIX-based systems, `library_path` should not contain a relative path (indicated by "./") before installing to system directories
# This json isn't used for regular local development, it's used for installation
if (UNIX)
    set(UNIX_INTERMEDIATE_FILE "${CMAKE_CURRENT_BINARY_DIR}/json/unix_install_validation.json")

    if(APPLE)
        set(JSON_LIBRARY_PATH "libVkLayer_khronos_validation.dylib")
    else()
        set(JSON_LIBRARY_PATH "libVkLayer_khronos_validation.so")
    endif()

    configure_file(${INPUT_FILE} ${UNIX_INTERMEDIATE_FILE} @ONLY)

    install(FILES ${UNIX_INTERMEDIATE_FILE} DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/vulkan/explicit_layer.d RENAME ${OUTPUT_FILE_FINAL_NAME})
endif()

if (WIN32)
    install(FILES ${INTERMEDIATE_FILE} DESTINATION ${LAYER_INSTALL_DIR} RENAME ${OUTPUT_FILE_FINAL_NAME})
endif()
if (MSVC)
    install(FILES $<TARGET_PDB_FILE:VkLayer_khronos_validation> DESTINATION ${LAYER_INSTALL_DIR})
endif()

install(TARGETS VkLayer_khronos_validation DESTINATION ${LAYER_INSTALL_DIR})
