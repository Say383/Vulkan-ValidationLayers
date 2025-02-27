/*
 * Copyright (c) 2015-2023 The Khronos Group Inc.
 * Copyright (c) 2015-2023 Valve Corporation
 * Copyright (c) 2015-2023 LunarG, Inc.
 * Copyright (c) 2015-2023 Google, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 */

#include "../framework/layer_validation_tests.h"
#include "generated/vk_extension_helper.h"

#include <algorithm>
#include <array>
#include <chrono>

TEST_F(VkPositiveLayerTest, DynamicRenderingSimple) {
    TEST_DESCRIPTION("Most simple way to use dynamic rendering");
    SetTargetApiVersion(VK_API_VERSION_1_1);
    AddRequiredExtensions(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME);
    ASSERT_NO_FATAL_FAILURE(InitFramework());

    if (!AreRequiredExtensionsEnabled()) {
        GTEST_SKIP() << RequiredExtensionsNotSupported() << " not supported";
    }
    if (DeviceValidationVersion() < VK_API_VERSION_1_1) {
        GTEST_SKIP() << "At least Vulkan version 1.1 is required";
    }

    auto dynamic_rendering_features = LvlInitStruct<VkPhysicalDeviceDynamicRenderingFeaturesKHR>();
    GetPhysicalDeviceFeatures2(dynamic_rendering_features);
    if (!dynamic_rendering_features.dynamicRendering) {
        GTEST_SKIP() << "Test requires (unsupported) dynamicRendering";
    }

    ASSERT_NO_FATAL_FAILURE(InitState(nullptr, &dynamic_rendering_features));

    VkRenderingAttachmentInfoKHR color_attachment = LvlInitStruct<VkRenderingAttachmentInfoKHR>();
    color_attachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    auto begin_rendering_info = LvlInitStruct<VkRenderingInfoKHR>();
    begin_rendering_info.layerCount = 1;
    begin_rendering_info.renderArea = {{0, 0}, {64, 64}};
    begin_rendering_info.colorAttachmentCount = 1;
    begin_rendering_info.pColorAttachments = &color_attachment;

    m_commandBuffer->begin();
    m_commandBuffer->BeginRendering(begin_rendering_info);
    m_commandBuffer->EndRendering();
    m_commandBuffer->end();
}

TEST_F(VkPositiveLayerTest, DynamicRenderingDraw) {
    TEST_DESCRIPTION("Draw with Dynamic Rendering.");

    SetTargetApiVersion(VK_API_VERSION_1_1);

    AddRequiredExtensions(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME);

    ASSERT_NO_FATAL_FAILURE(InitFramework());

    if (!AreRequiredExtensionsEnabled()) {
        GTEST_SKIP() << RequiredExtensionsNotSupported() << " not supported";
    }

    if (DeviceValidationVersion() < VK_API_VERSION_1_1) {
        GTEST_SKIP() << "At least Vulkan version 1.1 is required";
    }

    auto dynamic_rendering_features = LvlInitStruct<VkPhysicalDeviceDynamicRenderingFeaturesKHR>();
    auto features2 = GetPhysicalDeviceFeatures2(dynamic_rendering_features);
    if (!dynamic_rendering_features.dynamicRendering) {
        GTEST_SKIP() << "Test requires (unsupported) dynamicRendering";
    }

    ASSERT_NO_FATAL_FAILURE(InitState(nullptr, &features2));

    VkShaderObj vs(this, bindStateVertShaderText, VK_SHADER_STAGE_VERTEX_BIT);
    VkShaderObj fs(this, bindStateFragColorOutputText, VK_SHADER_STAGE_FRAGMENT_BIT);

    VkPipelineObj pipe(m_device);
    pipe.AddShader(&vs);
    pipe.AddShader(&fs);
    pipe.AddDefaultColorAttachment();

    VkDescriptorSetLayoutBinding dslb = {0, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr};
    const VkDescriptorSetLayoutObj dsl(m_device, {dslb});
    const VkPipelineLayoutObj pl(m_device, {&dsl});

    VkFormat color_formats = VK_FORMAT_UNDEFINED;
    auto pipeline_rendering_info = LvlInitStruct<VkPipelineRenderingCreateInfoKHR>();
    pipeline_rendering_info.colorAttachmentCount = 1;
    pipeline_rendering_info.pColorAttachmentFormats = &color_formats;

    auto create_info = LvlInitStruct<VkGraphicsPipelineCreateInfo>();
    pipe.InitGraphicsPipelineCreateInfo(&create_info);
    create_info.pNext = &pipeline_rendering_info;

    pipe.CreateVKPipeline(pl.handle(), VK_NULL_HANDLE, &create_info);

    VkViewport viewport = {0, 0, 16, 16, 0, 1};
    VkRect2D scissor = {{0, 0}, {16, 16}};

    VkRenderingAttachmentInfoKHR color_attachment = LvlInitStruct<VkRenderingAttachmentInfoKHR>();
    color_attachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkRenderingInfoKHR begin_rendering_info = LvlInitStruct<VkRenderingInfoKHR>();
    begin_rendering_info.colorAttachmentCount = 1;
    begin_rendering_info.pColorAttachments = &color_attachment;
    begin_rendering_info.layerCount = 1;

    m_commandBuffer->begin();
    m_commandBuffer->BeginRendering(begin_rendering_info);
    vk::CmdBindPipeline(m_commandBuffer->handle(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipe.handle());
    vk::CmdSetViewport(m_commandBuffer->handle(), 0, 1, &viewport);
    vk::CmdSetScissor(m_commandBuffer->handle(), 0, 1, &scissor);
    vk::CmdDraw(m_commandBuffer->handle(), 3, 1, 0, 0);
    m_commandBuffer->EndRendering();
    m_commandBuffer->end();
}

TEST_F(VkPositiveLayerTest, DynamicRenderingDrawMultiBind) {
    TEST_DESCRIPTION("Draw with Dynamic Rendering and multiple CmdBindPipeline calls.");

    SetTargetApiVersion(VK_API_VERSION_1_1);

    AddRequiredExtensions(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME);

    ASSERT_NO_FATAL_FAILURE(InitFramework());

    if (!AreRequiredExtensionsEnabled()) {
        GTEST_SKIP() << RequiredExtensionsNotSupported() << " not supported";
    }

    if (DeviceValidationVersion() < VK_API_VERSION_1_1) {
        GTEST_SKIP() << "At least Vulkan version 1.1 is required";
    }

    auto dynamic_rendering_features = LvlInitStruct<VkPhysicalDeviceDynamicRenderingFeaturesKHR>();
    auto features2 = GetPhysicalDeviceFeatures2(dynamic_rendering_features);
    if (!dynamic_rendering_features.dynamicRendering) {
        GTEST_SKIP() << "Test requires (unsupported) dynamicRendering";
    }

    ASSERT_NO_FATAL_FAILURE(InitState(nullptr, &features2));

    VkShaderObj vs(this, bindStateVertShaderText, VK_SHADER_STAGE_VERTEX_BIT);
    VkShaderObj fs(this, bindStateFragColorOutputText, VK_SHADER_STAGE_FRAGMENT_BIT);

    const auto depth_format = FindSupportedDepthOnlyFormat(gpu());
    auto ds_ci = LvlInitStruct<VkPipelineDepthStencilStateCreateInfo>();

    VkPipelineObj pipe(m_device);
    pipe.AddShader(&vs);
    pipe.AddShader(&fs);
    pipe.SetDepthStencil(&ds_ci);
    pipe.AddDefaultColorAttachment();

    VkPipelineObj pipe2(m_device);
    pipe2.AddShader(&vs);
    pipe2.AddShader(&fs);
    pipe2.AddDefaultColorAttachment();

    VkDescriptorSetLayoutBinding dslb = {0, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr};
    const VkDescriptorSetLayoutObj dsl(m_device, {dslb});
    const VkPipelineLayoutObj pl(m_device, {&dsl});

    VkFormat color_formats = VK_FORMAT_UNDEFINED;
    auto pipeline_rendering_info = LvlInitStruct<VkPipelineRenderingCreateInfoKHR>();
    pipeline_rendering_info.colorAttachmentCount = 1;
    pipeline_rendering_info.pColorAttachmentFormats = &color_formats;
    pipeline_rendering_info.depthAttachmentFormat = depth_format;

    auto create_info = LvlInitStruct<VkGraphicsPipelineCreateInfo>();
    pipe.InitGraphicsPipelineCreateInfo(&create_info);
    create_info.pNext = &pipeline_rendering_info;
    pipe.CreateVKPipeline(pl.handle(), VK_NULL_HANDLE, &create_info);

    pipeline_rendering_info.depthAttachmentFormat = VK_FORMAT_UNDEFINED;
    pipe2.InitGraphicsPipelineCreateInfo(&create_info);
    create_info.pNext = &pipeline_rendering_info;
    pipe2.CreateVKPipeline(pl.handle(), VK_NULL_HANDLE, &create_info);

    VkViewport viewport = {0, 0, 16, 16, 0, 1};
    VkRect2D scissor = {{0, 0}, {16, 16}};

    VkRenderingAttachmentInfoKHR color_attachment = LvlInitStruct<VkRenderingAttachmentInfoKHR>();
    color_attachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkRenderingInfoKHR begin_rendering_info = LvlInitStruct<VkRenderingInfoKHR>();
    begin_rendering_info.colorAttachmentCount = 1;
    begin_rendering_info.pColorAttachments = &color_attachment;
    begin_rendering_info.layerCount = 1;

    m_commandBuffer->begin();
    m_commandBuffer->BeginRendering(begin_rendering_info);
    vk::CmdBindPipeline(m_commandBuffer->handle(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipe.handle());
    vk::CmdSetViewport(m_commandBuffer->handle(), 0, 1, &viewport);
    vk::CmdSetScissor(m_commandBuffer->handle(), 0, 1, &scissor);
    vk::CmdDraw(m_commandBuffer->handle(), 3, 1, 0, 0);
    m_commandBuffer->EndRendering();

    m_commandBuffer->BeginRendering(begin_rendering_info);
    // NOTE: Setting dynamic state does not count as "using" the currently bound pipeline.
    vk::CmdSetViewport(m_commandBuffer->handle(), 0, 1, &viewport);
    vk::CmdSetScissor(m_commandBuffer->handle(), 0, 1, &scissor);
    vk::CmdBindPipeline(m_commandBuffer->handle(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipe2.handle());
    vk::CmdDraw(m_commandBuffer->handle(), 3, 1, 0, 0);
    m_commandBuffer->EndRendering();
    m_commandBuffer->end();
}

TEST_F(VkPositiveLayerTest, TestBeginQueryInDynamicRendering) {
    TEST_DESCRIPTION("Test calling vkCmdBeginQuery with a dynamic render pass.");

    SetTargetApiVersion(VK_API_VERSION_1_3);

    ASSERT_NO_FATAL_FAILURE(InitFramework());

    if (DeviceValidationVersion() < VK_API_VERSION_1_3) {
        GTEST_SKIP() << "At least Vulkan version 1.3 is required";
    }

    auto vk13features = LvlInitStruct<VkPhysicalDeviceVulkan13Features>();
    auto features2 = GetPhysicalDeviceFeatures2(vk13features);
    if (!vk13features.dynamicRendering) {
        GTEST_SKIP() << "Test requires (unsupported) dynamicRendering";
    }
    if (!vk13features.synchronization2) {
        GTEST_SKIP() << "Test requires (unsupported) synchronization2";
    }

    ASSERT_NO_FATAL_FAILURE(InitState(nullptr, &features2));
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    PFN_vkCmdBeginRendering vkCmdBeginRendering =
        reinterpret_cast<PFN_vkCmdBeginRendering>(vk::GetDeviceProcAddr(m_device->device(), "vkCmdBeginRendering"));
    assert(vkCmdBeginRendering != nullptr);
    PFN_vkCmdEndRendering vkCmdEndRendering =
        reinterpret_cast<PFN_vkCmdEndRendering>(vk::GetDeviceProcAddr(m_device->device(), "vkCmdEndRendering"));
    assert(vkCmdEndRendering != nullptr);

    VkRenderingInfoKHR begin_rendering_info = LvlInitStruct<VkRenderingInfoKHR>();
    begin_rendering_info.layerCount = 1;

    VkQueryPoolCreateInfo qpci = LvlInitStruct<VkQueryPoolCreateInfo>();
    qpci.queryType = VK_QUERY_TYPE_OCCLUSION;
    qpci.queryCount = 2;

    vk_testing::QueryPool query_pool;
    query_pool.init(*m_device, qpci);

    m_commandBuffer->begin();

    vkCmdBeginRendering(m_commandBuffer->handle(), &begin_rendering_info);
    vk::CmdBeginQuery(m_commandBuffer->handle(), query_pool.handle(), 0, 0);
    vk::CmdEndQuery(m_commandBuffer->handle(), query_pool.handle(), 0);
    vkCmdEndRendering(m_commandBuffer->handle());

    m_commandBuffer->end();
}

TEST_F(VkPositiveLayerTest, DynamicRenderingPipeWithDiscard) {
    TEST_DESCRIPTION("Create dynamic rendering pipeline with rasterizer discard.");

    SetTargetApiVersion(VK_API_VERSION_1_1);

    AddRequiredExtensions(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME);

    ASSERT_NO_FATAL_FAILURE(InitFramework());

    if (!AreRequiredExtensionsEnabled()) {
        GTEST_SKIP() << RequiredExtensionsNotSupported() << " not supported";
    }

    if (DeviceValidationVersion() < VK_API_VERSION_1_1) {
        GTEST_SKIP() << "At least Vulkan version 1.1 is required";
    }

    auto dynamic_rendering_features = LvlInitStruct<VkPhysicalDeviceDynamicRenderingFeaturesKHR>();
    auto features2 = GetPhysicalDeviceFeatures2(dynamic_rendering_features);
    if (!dynamic_rendering_features.dynamicRendering) {
        GTEST_SKIP() << "Test requires (unsupported) dynamicRendering";
    }

    ASSERT_NO_FATAL_FAILURE(InitState(nullptr, &features2));

    VkShaderObj vs(this, bindStateVertShaderText, VK_SHADER_STAGE_VERTEX_BIT);
    VkShaderObj fs(this, bindStateFragShaderText, VK_SHADER_STAGE_FRAGMENT_BIT);

    VkPipelineDepthStencilStateCreateInfo ds_ci = LvlInitStruct<VkPipelineDepthStencilStateCreateInfo>();
    ds_ci.depthTestEnable = VK_TRUE;
    ds_ci.depthWriteEnable = VK_TRUE;

    VkPipelineObj pipe(m_device);
    pipe.AddShader(&vs);
    pipe.AddShader(&fs);
    pipe.AddDefaultColorAttachment();
    pipe.SetDepthStencil(&ds_ci);

    VkDescriptorSetLayoutBinding dslb = {0, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr};
    const VkDescriptorSetLayoutObj dsl(m_device, {dslb});
    const VkPipelineLayoutObj pl(m_device, {&dsl});

    auto create_info = LvlInitStruct<VkGraphicsPipelineCreateInfo>();
    pipe.InitGraphicsPipelineCreateInfo(&create_info);
    create_info.layout = pl.handle();
    create_info.renderPass = VK_NULL_HANDLE;

    VkFormat color_formats = {VK_FORMAT_R8G8B8A8_UNORM};
    auto pipeline_rendering_info = LvlInitStruct<VkPipelineRenderingCreateInfoKHR>();
    pipeline_rendering_info.colorAttachmentCount = 1;
    pipeline_rendering_info.pColorAttachmentFormats = &color_formats;
    pipeline_rendering_info.depthAttachmentFormat = VK_FORMAT_D16_UNORM;  // D16_UNORM has guaranteed support
    pipeline_rendering_info.pNext = create_info.pNext;
    create_info.pNext = &pipeline_rendering_info;

    const VkRenderPass render_pass = VK_NULL_HANDLE;
    pipe.CreateVKPipeline(pl.handle(), render_pass, &create_info);
}

TEST_F(VkPositiveLayerTest, UseStencilAttachmentWithIntegerFormatAndDepthStencilResolve) {
    TEST_DESCRIPTION("Use stencil attachment with integer format and depth stencil resolve extension");

    SetTargetApiVersion(VK_API_VERSION_1_1);
    AddRequiredExtensions(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME);
    AddRequiredExtensions(VK_KHR_DEPTH_STENCIL_RESOLVE_EXTENSION_NAME);
    ASSERT_NO_FATAL_FAILURE(InitFramework());

    if (DeviceValidationVersion() < VK_API_VERSION_1_1) {
        GTEST_SKIP() << "At least Vulkan version 1.1 is required";
    }
    if (!AreRequiredExtensionsEnabled()) {
        GTEST_SKIP() << RequiredExtensionsNotSupported() << " not supported";
    }

    auto dynamic_rendering_features = LvlInitStruct<VkPhysicalDeviceDynamicRenderingFeaturesKHR>();
    GetPhysicalDeviceFeatures2(dynamic_rendering_features);
    if (dynamic_rendering_features.dynamicRendering == VK_FALSE) {
        GTEST_SKIP() << "Test requires (unsupported) dynamicRendering";
    }

    ASSERT_NO_FATAL_FAILURE(InitState(nullptr, &dynamic_rendering_features));
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    if (!ImageFormatIsSupported(gpu(), VK_FORMAT_S8_UINT, VK_IMAGE_TILING_OPTIMAL)) {
        GTEST_SKIP() << "VK_FORMAT_S8_UINT format not supported";
    }

    auto depth_stencil_resolve_properties = LvlInitStruct<VkPhysicalDeviceDepthStencilResolveProperties>();
    auto properties2 = LvlInitStruct<VkPhysicalDeviceProperties2>(&depth_stencil_resolve_properties);
    GetPhysicalDeviceProperties2(properties2);
    if ((depth_stencil_resolve_properties.supportedStencilResolveModes & VK_RESOLVE_MODE_AVERAGE_BIT) == 0) {
        GTEST_SKIP() << "VK_RESOLVE_MODE_AVERAGE_BIT not supported for VK_FORMAT_S8_UINT";
    }

    VkImageCreateInfo image_create_info = LvlInitStruct<VkImageCreateInfo>();
    image_create_info.imageType = VK_IMAGE_TYPE_2D;
    image_create_info.format = VK_FORMAT_S8_UINT;
    image_create_info.extent = {32, 32, 1};
    image_create_info.mipLevels = 1;
    image_create_info.arrayLayers = 1;
    image_create_info.samples = VK_SAMPLE_COUNT_4_BIT;
    image_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_create_info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    VkImageObj image(m_device);
    image.Init(image_create_info);
    VkImageView image_view = image.targetView(VK_FORMAT_S8_UINT, VK_IMAGE_ASPECT_STENCIL_BIT);

    image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
    VkImageObj resolve_image(m_device);
    resolve_image.Init(image_create_info);
    VkImageView resolve_image_view = resolve_image.targetView(VK_FORMAT_S8_UINT, VK_IMAGE_ASPECT_STENCIL_BIT);

    VkRenderingAttachmentInfoKHR stencil_attachment = LvlInitStruct<VkRenderingAttachmentInfoKHR>();
    stencil_attachment.imageView = image_view;
    stencil_attachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    stencil_attachment.resolveMode = VK_RESOLVE_MODE_AVERAGE_BIT;
    stencil_attachment.resolveImageLayout = VK_IMAGE_LAYOUT_GENERAL;
    stencil_attachment.resolveImageView = resolve_image_view;

    VkRenderingInfoKHR begin_rendering_info = LvlInitStruct<VkRenderingInfoKHR>();
    begin_rendering_info.pStencilAttachment = &stencil_attachment;
    begin_rendering_info.layerCount = 1;

    m_commandBuffer->begin();
    m_commandBuffer->BeginRendering(begin_rendering_info);
    m_commandBuffer->EndRendering();
    m_commandBuffer->end();
}

TEST_F(VkPositiveLayerTest, DynamicRenderingFragmentDensityMapSubsampledBit) {
    TEST_DESCRIPTION("Test creating an image with subsampled bit.");

    SetTargetApiVersion(VK_API_VERSION_1_1);

    AddRequiredExtensions(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME);
    AddRequiredExtensions(VK_EXT_FRAGMENT_DENSITY_MAP_EXTENSION_NAME);

    ASSERT_NO_FATAL_FAILURE(InitFramework(m_errorMonitor));

    if (DeviceValidationVersion() < VK_API_VERSION_1_1) {
        GTEST_SKIP() << "At least Vulkan version 1.1 is required";
    }

    if (!AreRequiredExtensionsEnabled()) {
        GTEST_SKIP() << RequiredExtensionsNotSupported() << " not supported";
    }

    auto dynamic_rendering_features = LvlInitStruct<VkPhysicalDeviceDynamicRenderingFeaturesKHR>();
    auto features2 = GetPhysicalDeviceFeatures2(dynamic_rendering_features);
    if (!dynamic_rendering_features.dynamicRendering) {
        GTEST_SKIP() << "Test requires (unsupported) dynamicRendering";
    }

    ASSERT_NO_FATAL_FAILURE(InitState(nullptr, &features2));

    VkImageCreateInfo image_ci = LvlInitStruct<VkImageCreateInfo>();
    image_ci.flags = VK_IMAGE_CREATE_SUBSAMPLED_BIT_EXT;
    image_ci.imageType = VK_IMAGE_TYPE_2D;
    image_ci.format = VK_FORMAT_R8G8B8A8_UINT;
    image_ci.extent = {32, 32, 1};
    image_ci.mipLevels = 1;
    image_ci.arrayLayers = 1;
    image_ci.samples = VK_SAMPLE_COUNT_1_BIT;
    image_ci.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_ci.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    VkImageObj image(m_device);
    image.Init(image_ci);
    VkImageView image_view = image.targetView(VK_FORMAT_R8G8B8A8_UINT, VK_IMAGE_ASPECT_COLOR_BIT);

    image_ci.flags = 0;
    image_ci.usage = VK_IMAGE_USAGE_FRAGMENT_DENSITY_MAP_BIT_EXT;

    VkImageObj fdm_image(m_device);
    fdm_image.Init(image_ci);
    VkImageView fdm_image_view = fdm_image.targetView(VK_FORMAT_R8G8B8A8_UINT);

    VkRenderingAttachmentInfoKHR color_attachment = LvlInitStruct<VkRenderingAttachmentInfoKHR>();
    color_attachment.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    color_attachment.imageView = image_view;

    VkRenderingInfoKHR begin_rendering_info = LvlInitStruct<VkRenderingInfoKHR>();
    begin_rendering_info.colorAttachmentCount = 1;
    begin_rendering_info.pColorAttachments = &color_attachment;

    auto fragment_density_map = LvlInitStruct<VkRenderingFragmentDensityMapAttachmentInfoEXT>();
    fragment_density_map.imageView = fdm_image_view;
    fragment_density_map.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    begin_rendering_info.pNext = &fragment_density_map;
    begin_rendering_info.layerCount = 1;

    m_commandBuffer->begin();
    m_commandBuffer->BeginRendering(begin_rendering_info);
}

TEST_F(VkPositiveLayerTest, DynamicRenderingSuspendResumeDraw) {
    TEST_DESCRIPTION("Resume and suspend at vkCmdBeginRendering time");

    SetTargetApiVersion(VK_API_VERSION_1_1);

    AddRequiredExtensions(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME);

    ASSERT_NO_FATAL_FAILURE(InitFramework());

    if (!AreRequiredExtensionsEnabled()) {
        GTEST_SKIP() << RequiredExtensionsNotSupported() << " not supported";
    }

    if (DeviceValidationVersion() < VK_API_VERSION_1_1) {
        GTEST_SKIP() << "At least Vulkan version 1.1 is required";
    }

    auto dynamic_rendering_features = LvlInitStruct<VkPhysicalDeviceDynamicRenderingFeaturesKHR>();
    auto features2 = GetPhysicalDeviceFeatures2(dynamic_rendering_features);
    if (!dynamic_rendering_features.dynamicRendering) {
        GTEST_SKIP() << "Test requires (unsupported) dynamicRendering , skipping";
    }

    ASSERT_NO_FATAL_FAILURE(InitState(nullptr, &features2));

    VkShaderObj vs(this, bindStateVertShaderText, VK_SHADER_STAGE_VERTEX_BIT);
    VkShaderObj fs(this, bindStateFragColorOutputText, VK_SHADER_STAGE_FRAGMENT_BIT);

    VkPipelineObj pipe(m_device);
    pipe.AddShader(&vs);
    pipe.AddShader(&fs);
    pipe.AddDefaultColorAttachment();

    VkDescriptorSetLayoutBinding dslb = {0, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr};
    const VkDescriptorSetLayoutObj dsl(m_device, {dslb});
    const VkPipelineLayoutObj pl(m_device, {&dsl});

    VkFormat color_formats = VK_FORMAT_UNDEFINED;
    auto pipeline_rendering_info = LvlInitStruct<VkPipelineRenderingCreateInfoKHR>();
    pipeline_rendering_info.colorAttachmentCount = 1;
    pipeline_rendering_info.pColorAttachmentFormats = &color_formats;

    auto create_info = LvlInitStruct<VkGraphicsPipelineCreateInfo>();
    pipe.InitGraphicsPipelineCreateInfo(&create_info);
    create_info.pNext = &pipeline_rendering_info;

    pipe.CreateVKPipeline(pl.handle(), VK_NULL_HANDLE, &create_info);

    VkViewport viewport = {0, 0, 16, 16, 0, 1};
    VkRect2D scissor = {{0, 0}, {16, 16}};

    VkRenderingAttachmentInfoKHR color_attachment = LvlInitStruct<VkRenderingAttachmentInfoKHR>();
    color_attachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkRenderingInfoKHR begin_rendering_info = LvlInitStruct<VkRenderingInfoKHR>();
    begin_rendering_info.flags = VK_RENDERING_SUSPENDING_BIT_KHR;
    begin_rendering_info.colorAttachmentCount = 1;
    begin_rendering_info.pColorAttachments = &color_attachment;
    begin_rendering_info.layerCount = 1;

    VkCommandBufferObj cb1(m_device, m_commandPool);
    VkCommandBufferObj cb2(m_device, m_commandPool);

    m_commandBuffer->begin();
    m_commandBuffer->BeginRendering(begin_rendering_info);
    vk::CmdBindPipeline(m_commandBuffer->handle(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipe.handle());
    vk::CmdSetViewport(m_commandBuffer->handle(), 0, 1, &viewport);
    vk::CmdSetScissor(m_commandBuffer->handle(), 0, 1, &scissor);
    vk::CmdDraw(m_commandBuffer->handle(), 3, 1, 0, 0);
    m_commandBuffer->EndRendering();
    m_commandBuffer->end();

    begin_rendering_info.flags = VK_RENDERING_RESUMING_BIT_KHR | VK_RENDERING_SUSPENDING_BIT_KHR;
    cb1.begin();
    cb1.BeginRendering(begin_rendering_info);
    vk::CmdBindPipeline(cb1.handle(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipe.handle());
    vk::CmdSetViewport(cb1.handle(), 0, 1, &viewport);
    vk::CmdSetScissor(cb1.handle(), 0, 1, &scissor);
    vk::CmdDraw(cb1.handle(), 3, 1, 0, 0);
    cb1.EndRendering();
    cb1.end();

    begin_rendering_info.flags = VK_RENDERING_RESUMING_BIT_KHR;
    cb2.begin();
    cb2.BeginRendering(begin_rendering_info);
    vk::CmdBindPipeline(cb2.handle(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipe.handle());
    vk::CmdSetViewport(cb2.handle(), 0, 1, &viewport);
    vk::CmdSetScissor(cb2.handle(), 0, 1, &scissor);
    vk::CmdDraw(cb2.handle(), 3, 1, 0, 0);
    cb2.EndRendering();
    cb2.end();

    std::array<VkCommandBuffer, 3> cbs = {m_commandBuffer->handle(), cb1.handle(), cb2.handle()};
    auto submit = LvlInitStruct<VkSubmitInfo>();
    submit.commandBufferCount = static_cast<uint32_t>(cbs.size());
    submit.pCommandBuffers = cbs.data();
    vk::QueueSubmit(m_device->m_queue, 1, &submit, VK_NULL_HANDLE);

    vk::QueueWaitIdle(m_device->m_queue);
}

TEST_F(VkPositiveLayerTest, DynamicRenderingCreateGraphicsPipeline) {
    TEST_DESCRIPTION("Test for a creating a pipeline with VK_KHR_dynamic_rendering enabled");
    SetTargetApiVersion(VK_API_VERSION_1_1);

    AddRequiredExtensions(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME);

    ASSERT_NO_FATAL_FAILURE(InitFramework());

    if (!AreRequiredExtensionsEnabled()) {
        GTEST_SKIP() << RequiredExtensionsNotSupported() << " not supported";
    }

    auto dynamic_rendering_features = LvlInitStruct<VkPhysicalDeviceDynamicRenderingFeaturesKHR>();
    auto features2 = GetPhysicalDeviceFeatures2(dynamic_rendering_features);
    if (!dynamic_rendering_features.dynamicRendering) {
        GTEST_SKIP() << "Test requires (unsupported) dynamicRendering";
    }

    ASSERT_NO_FATAL_FAILURE(InitState(nullptr, &features2));

    char const* fsSource = R"glsl(
        #version 450
        layout(input_attachment_index=0, set=0, binding=0) uniform subpassInput x;
        layout(location=0) out vec4 color;
        void main() {
           color = subpassLoad(x);
        }
    )glsl";

    VkShaderObj vs(this, bindStateVertShaderText, VK_SHADER_STAGE_VERTEX_BIT);
    VkShaderObj fs(this, fsSource, VK_SHADER_STAGE_FRAGMENT_BIT);

    VkPipelineObj pipe(m_device);
    pipe.AddShader(&vs);
    pipe.AddShader(&fs);
    pipe.AddDefaultColorAttachment();

    VkDescriptorSetLayoutBinding dslb = {0, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr};
    const VkDescriptorSetLayoutObj dsl(m_device, {dslb});
    const VkPipelineLayoutObj pl(m_device, {&dsl});

    VkFormat color_format = VK_FORMAT_R8G8B8A8_UNORM;
    auto rendering_info = LvlInitStruct<VkPipelineRenderingCreateInfoKHR>();
    rendering_info.colorAttachmentCount = 1;
    rendering_info.pColorAttachmentFormats = &color_format;

    auto create_info = LvlInitStruct<VkGraphicsPipelineCreateInfo>();
    pipe.InitGraphicsPipelineCreateInfo(&create_info);
    create_info.pNext = &rendering_info;

    VkAttachmentReference attachment = {};
    attachment.layout = VK_IMAGE_LAYOUT_GENERAL;
    attachment.attachment = 0;

    VkAttachmentDescription attach_desc = {};
    attach_desc.format = VK_FORMAT_B8G8R8A8_UNORM;
    attach_desc.samples = VK_SAMPLE_COUNT_1_BIT;
    attach_desc.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
    attach_desc.finalLayout = VK_IMAGE_LAYOUT_GENERAL;

    VkSubpassDescription subpass = {};
    subpass.inputAttachmentCount = 1;
    subpass.pInputAttachments = &attachment;
    VkRenderPassCreateInfo render_pass_ci = LvlInitStruct<VkRenderPassCreateInfo>();
    render_pass_ci.subpassCount = 1;
    render_pass_ci.pSubpasses = &subpass;
    render_pass_ci.attachmentCount = 1;
    render_pass_ci.pAttachments = &attach_desc;

    vk_testing::RenderPass render_pass;
    render_pass.init(*m_device, render_pass_ci);

    pipe.CreateVKPipeline(pl.handle(), render_pass.handle(), &create_info);
}

TEST_F(VkPositiveLayerTest, DynamicRenderingCreateGraphicsPipelineNoInfo) {
    TEST_DESCRIPTION("Test for a creating a pipeline with VK_KHR_dynamic_rendering enabled but no rendering info struct.");
    SetTargetApiVersion(VK_API_VERSION_1_1);

    AddRequiredExtensions(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME);

    ASSERT_NO_FATAL_FAILURE(InitFramework());

    if (!AreRequiredExtensionsEnabled()) {
        GTEST_SKIP() << RequiredExtensionsNotSupported() << " not supported";
    }

    auto dynamic_rendering_features = LvlInitStruct<VkPhysicalDeviceDynamicRenderingFeaturesKHR>();
    auto features2 = GetPhysicalDeviceFeatures2(dynamic_rendering_features);
    if (!dynamic_rendering_features.dynamicRendering) {
        GTEST_SKIP() << "Test requires (unsupported) dynamicRendering";
    }

    ASSERT_NO_FATAL_FAILURE(InitState(nullptr, &features2));

    char const* fsSource = R"glsl(
        #version 450
        layout(input_attachment_index=0, set=0, binding=0) uniform subpassInput x;
        layout(location=0) out vec4 color;
        void main() {
           color = subpassLoad(x);
        }
    )glsl";

    VkShaderObj vs(this, bindStateVertShaderText, VK_SHADER_STAGE_VERTEX_BIT);
    VkShaderObj fs(this, fsSource, VK_SHADER_STAGE_FRAGMENT_BIT);

    VkPipelineObj pipe(m_device);
    pipe.AddShader(&vs);
    pipe.AddShader(&fs);
    pipe.AddDefaultColorAttachment();

    VkDescriptorSetLayoutBinding dslb = {0, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr};
    const VkDescriptorSetLayoutObj dsl(m_device, {dslb});
    const VkPipelineLayoutObj pl(m_device, {&dsl});

    VkAttachmentReference attachment = {};
    attachment.layout = VK_IMAGE_LAYOUT_GENERAL;
    attachment.attachment = 0;

    VkAttachmentDescription attach_desc = {};
    attach_desc.format = VK_FORMAT_B8G8R8A8_UNORM;
    attach_desc.samples = VK_SAMPLE_COUNT_1_BIT;
    attach_desc.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
    attach_desc.finalLayout = VK_IMAGE_LAYOUT_GENERAL;

    VkSubpassDescription subpass = {};
    subpass.inputAttachmentCount = 1;
    subpass.pInputAttachments = &attachment;
    VkRenderPassCreateInfo render_pass_ci = LvlInitStruct<VkRenderPassCreateInfo>();
    render_pass_ci.subpassCount = 1;
    render_pass_ci.pSubpasses = &subpass;
    render_pass_ci.attachmentCount = 1;
    render_pass_ci.pAttachments = &attach_desc;

    vk_testing::RenderPass render_pass;
    render_pass.init(*m_device, render_pass_ci);

    auto create_info = LvlInitStruct<VkGraphicsPipelineCreateInfo>();
    pipe.InitGraphicsPipelineCreateInfo(&create_info);
    // if there isn't a VkPipelineRenderingCreateInfoKHR, the driver is supposed to use safe default values
    pipe.CreateVKPipeline(pl.handle(), render_pass.handle(), &create_info);
}

TEST_F(VkPositiveLayerTest, DynamicRenderingWithDualSourceBlending) {
    TEST_DESCRIPTION("Test drawing with dynamic rendering and dual source blending.");

    SetTargetApiVersion(VK_API_VERSION_1_1);
    AddRequiredExtensions(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME);
    ASSERT_NO_FATAL_FAILURE(InitFramework(m_errorMonitor));

    if (DeviceValidationVersion() < VK_API_VERSION_1_1) {
        GTEST_SKIP() << "At least Vulkan version 1.1 is required";
    }
    if (!AreRequiredExtensionsEnabled()) {
        GTEST_SKIP() << RequiredExtensionsNotSupported() << " not supported";
    }

    auto dynamic_rendering_features = LvlInitStruct<VkPhysicalDeviceDynamicRenderingFeaturesKHR>();
    auto features2 = GetPhysicalDeviceFeatures2(dynamic_rendering_features);
    if (features2.features.dualSrcBlend == VK_FALSE) {
        GTEST_SKIP() << "Test requires (unsupported) dualSrcBlend";
    }
    if (dynamic_rendering_features.dynamicRendering == VK_FALSE) {
        GTEST_SKIP() << "Test requires (unsupported) dynamicRendering";
    }

    ASSERT_NO_FATAL_FAILURE(InitState(nullptr, &features2));

    char const* fsSource = R"glsl(
        #version 450
        layout(location = 0) out vec4 c0;
        void main(){
           c0 = vec4(1.0f);
        }
    )glsl";

    VkShaderObj fs(this, fsSource, VK_SHADER_STAGE_FRAGMENT_BIT);

    VkPipelineColorBlendAttachmentState cb_attachments = {};
    cb_attachments.blendEnable = VK_TRUE;
    cb_attachments.srcColorBlendFactor = VK_BLEND_FACTOR_SRC1_COLOR;  // bad!
    cb_attachments.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
    cb_attachments.colorBlendOp = VK_BLEND_OP_ADD;
    cb_attachments.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    cb_attachments.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    cb_attachments.alphaBlendOp = VK_BLEND_OP_ADD;

    VkFormat color_formats = VK_FORMAT_UNDEFINED;
    auto pipeline_rendering_info = LvlInitStruct<VkPipelineRenderingCreateInfoKHR>();
    pipeline_rendering_info.colorAttachmentCount = 1;
    pipeline_rendering_info.pColorAttachmentFormats = &color_formats;

    CreatePipelineHelper pipe(*this);
    pipe.InitInfo();
    pipe.gp_ci_.pNext = &pipeline_rendering_info;
    pipe.cb_attachments_[0] = cb_attachments;
    pipe.shader_stages_ = {pipe.vs_->GetStageCreateInfo(), fs.GetStageCreateInfo()};
    pipe.InitState();
    pipe.CreateGraphicsPipeline();

    VkRenderingAttachmentInfoKHR color_attachment = LvlInitStruct<VkRenderingAttachmentInfoKHR>();
    color_attachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkRenderingInfoKHR begin_rendering_info = LvlInitStruct<VkRenderingInfoKHR>();
    begin_rendering_info.colorAttachmentCount = 1;
    begin_rendering_info.pColorAttachments = &color_attachment;
    begin_rendering_info.layerCount = 1;

    m_commandBuffer->begin();
    m_commandBuffer->BeginRendering(begin_rendering_info);

    vk::CmdBindPipeline(m_commandBuffer->handle(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipe.pipeline_);

    vk::CmdDraw(m_commandBuffer->handle(), 3, 1, 0, 0);

    m_commandBuffer->EndRendering();
    m_commandBuffer->end();
}

TEST_F(VkPositiveLayerTest, DynamicRenderingSuspendPrimaryResumeInSecondary) {
    TEST_DESCRIPTION("Suspend in primary and resume in secondary");

    SetTargetApiVersion(VK_API_VERSION_1_1);

    AddRequiredExtensions(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME);

    ASSERT_NO_FATAL_FAILURE(InitFramework());

    if (!AreRequiredExtensionsEnabled()) {
        GTEST_SKIP() << RequiredExtensionsNotSupported() << " not supported";
    }

    if (DeviceValidationVersion() < VK_API_VERSION_1_1) {
        GTEST_SKIP() << "At least Vulkan version 1.1 is required";
    }

    auto dynamic_rendering_features = LvlInitStruct<VkPhysicalDeviceDynamicRenderingFeaturesKHR>();
    auto features2 = GetPhysicalDeviceFeatures2(dynamic_rendering_features);
    if (!dynamic_rendering_features.dynamicRendering) {
        GTEST_SKIP() << "Test requires (unsupported) dynamicRendering , skipping";
    }

    ASSERT_NO_FATAL_FAILURE(InitState(nullptr, &features2));

    VkShaderObj vs(this, bindStateVertShaderText, VK_SHADER_STAGE_VERTEX_BIT);
    VkShaderObj fs(this, bindStateFragColorOutputText, VK_SHADER_STAGE_FRAGMENT_BIT);

    VkPipelineObj pipe(m_device);
    pipe.AddShader(&vs);
    pipe.AddShader(&fs);
    pipe.AddDefaultColorAttachment();

    VkDescriptorSetLayoutBinding dslb = {0, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr};
    const VkDescriptorSetLayoutObj dsl(m_device, {dslb});
    const VkPipelineLayoutObj pl(m_device, {&dsl});

    VkFormat color_formats = {VK_FORMAT_R8G8B8A8_UNORM};
    auto pipeline_rendering_info = LvlInitStruct<VkPipelineRenderingCreateInfoKHR>();
    pipeline_rendering_info.colorAttachmentCount = 1;
    pipeline_rendering_info.pColorAttachmentFormats = &color_formats;

    auto create_info = LvlInitStruct<VkGraphicsPipelineCreateInfo>();
    pipe.InitGraphicsPipelineCreateInfo(&create_info);
    create_info.pNext = &pipeline_rendering_info;

    pipe.CreateVKPipeline(pl.handle(), VK_NULL_HANDLE, &create_info);

    VkViewport viewport = {0, 0, 16, 16, 0, 1};
    VkRect2D scissor = {{0, 0}, {16, 16}};

    VkRenderingAttachmentInfoKHR color_attachment = LvlInitStruct<VkRenderingAttachmentInfoKHR>();
    color_attachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkRenderingInfoKHR begin_rendering_info = LvlInitStruct<VkRenderingInfoKHR>();
    begin_rendering_info.flags = VK_RENDERING_SUSPENDING_BIT_KHR;
    begin_rendering_info.colorAttachmentCount = 1;
    begin_rendering_info.pColorAttachments = &color_attachment;
    begin_rendering_info.layerCount = 1;

    // Primary suspends render
    m_commandBuffer->begin();
    m_commandBuffer->BeginRendering(begin_rendering_info);
    vk::CmdBindPipeline(m_commandBuffer->handle(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipe.handle());
    vk::CmdSetViewport(m_commandBuffer->handle(), 0, 1, &viewport);
    vk::CmdSetScissor(m_commandBuffer->handle(), 0, 1, &scissor);
    vk::CmdDraw(m_commandBuffer->handle(), 3, 1, 0, 0);
    m_commandBuffer->EndRendering();

    // Secondary resumes render
    VkCommandBufferObj secondary(m_device, m_commandPool, VK_COMMAND_BUFFER_LEVEL_SECONDARY);
    begin_rendering_info.flags = VK_RENDERING_RESUMING_BIT_KHR;
    secondary.begin();
    secondary.BeginRendering(begin_rendering_info);
    vk::CmdBindPipeline(secondary.handle(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipe.handle());
    vk::CmdSetViewport(secondary.handle(), 0, 1, &viewport);
    vk::CmdSetScissor(secondary.handle(), 0, 1, &scissor);
    secondary.Draw(3, 1, 0, 0);
    secondary.EndRendering();
    secondary.end();

    // Execute secondary
    vk::CmdExecuteCommands(m_commandBuffer->handle(), 1, &secondary.handle());

    m_commandBuffer->end();

    // Submit
    auto submit = LvlInitStruct<VkSubmitInfo>();
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &m_commandBuffer->handle();
    vk::QueueSubmit(m_device->m_queue, 1, &submit, VK_NULL_HANDLE);

    vk::QueueWaitIdle(m_device->m_queue);
}

TEST_F(VkPositiveLayerTest, DynamicRenderingSuspendSecondaryResumeInPrimary) {
    TEST_DESCRIPTION("Suspend in secondary and resume in primary");

    SetTargetApiVersion(VK_API_VERSION_1_1);

    AddRequiredExtensions(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME);

    ASSERT_NO_FATAL_FAILURE(InitFramework());

    if (!AreRequiredExtensionsEnabled()) {
        GTEST_SKIP() << RequiredExtensionsNotSupported() << " not supported";
    }

    if (DeviceValidationVersion() < VK_API_VERSION_1_1) {
        GTEST_SKIP() << "At least Vulkan version 1.1 is required";
    }

    auto dynamic_rendering_features = LvlInitStruct<VkPhysicalDeviceDynamicRenderingFeaturesKHR>();
    auto features2 = GetPhysicalDeviceFeatures2(dynamic_rendering_features);
    if (!dynamic_rendering_features.dynamicRendering) {
        GTEST_SKIP() << "Test requires (unsupported) dynamicRendering , skipping";
    }

    ASSERT_NO_FATAL_FAILURE(InitState(nullptr, &features2));

    char const* fsSource = R"glsl(
        #version 450
        layout(location=0) out vec4 color;
        void main() {
           color = vec4(1.0f);
        }
    )glsl";

    VkShaderObj vs(this, bindStateVertShaderText, VK_SHADER_STAGE_VERTEX_BIT);
    VkShaderObj fs(this, fsSource, VK_SHADER_STAGE_FRAGMENT_BIT);

    VkPipelineObj pipe(m_device);
    pipe.AddShader(&vs);
    pipe.AddShader(&fs);
    pipe.AddDefaultColorAttachment();

    VkDescriptorSetLayoutBinding dslb = {0, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr};
    const VkDescriptorSetLayoutObj dsl(m_device, {dslb});
    const VkPipelineLayoutObj pl(m_device, {&dsl});

    VkFormat color_formats = {VK_FORMAT_R8G8B8A8_UNORM};
    auto pipeline_rendering_info = LvlInitStruct<VkPipelineRenderingCreateInfoKHR>();
    pipeline_rendering_info.colorAttachmentCount = 1;
    pipeline_rendering_info.pColorAttachmentFormats = &color_formats;

    auto create_info = LvlInitStruct<VkGraphicsPipelineCreateInfo>();
    pipe.InitGraphicsPipelineCreateInfo(&create_info);
    create_info.pNext = &pipeline_rendering_info;

    pipe.CreateVKPipeline(pl.handle(), VK_NULL_HANDLE, &create_info);

    VkViewport viewport = {0, 0, 16, 16, 0, 1};
    VkRect2D scissor = {{0, 0}, {16, 16}};

    VkRenderingAttachmentInfoKHR color_attachment = LvlInitStruct<VkRenderingAttachmentInfoKHR>();
    color_attachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkRenderingInfoKHR begin_rendering_info = LvlInitStruct<VkRenderingInfoKHR>();
    begin_rendering_info.flags = 0;
    begin_rendering_info.colorAttachmentCount = 1;
    begin_rendering_info.pColorAttachments = &color_attachment;
    begin_rendering_info.layerCount = 1;

    // First primary with secondary that suspends render
    m_commandBuffer->begin();
    m_commandBuffer->BeginRendering(begin_rendering_info);
    vk::CmdBindPipeline(m_commandBuffer->handle(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipe.handle());
    vk::CmdSetViewport(m_commandBuffer->handle(), 0, 1, &viewport);
    vk::CmdSetScissor(m_commandBuffer->handle(), 0, 1, &scissor);
    vk::CmdDraw(m_commandBuffer->handle(), 3, 1, 0, 0);
    m_commandBuffer->EndRendering();

    VkCommandBufferObj secondary(m_device, m_commandPool, VK_COMMAND_BUFFER_LEVEL_SECONDARY);
    begin_rendering_info.flags = VK_RENDERING_SUSPENDING_BIT_KHR;
    secondary.begin();
    secondary.BeginRendering(begin_rendering_info);
    vk::CmdBindPipeline(secondary.handle(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipe.handle());
    vk::CmdSetViewport(secondary.handle(), 0, 1, &viewport);
    vk::CmdSetScissor(secondary.handle(), 0, 1, &scissor);
    secondary.Draw(3, 1, 0, 0);
    secondary.EndRendering();
    secondary.end();

    // Execute secondary
    vk::CmdExecuteCommands(m_commandBuffer->handle(), 1, &secondary.handle());

    m_commandBuffer->end();

    // Second Primary resumes render
    VkCommandBufferObj cb(m_device, m_commandPool);
    begin_rendering_info.flags = VK_RENDERING_RESUMING_BIT_KHR;
    cb.begin();
    cb.BeginRendering(begin_rendering_info);
    vk::CmdBindPipeline(cb.handle(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipe.handle());
    vk::CmdSetViewport(cb.handle(), 0, 1, &viewport);
    vk::CmdSetScissor(cb.handle(), 0, 1, &scissor);
    vk::CmdDraw(cb.handle(), 3, 1, 0, 0);
    cb.EndRendering();
    cb.end();

    // Submit
    std::array<VkCommandBuffer, 2> cbs = {m_commandBuffer->handle(), cb.handle()};
    auto submit = LvlInitStruct<VkSubmitInfo>();
    submit.commandBufferCount = static_cast<uint32_t>(cbs.size());
    submit.pCommandBuffers = cbs.data();
    vk::QueueSubmit(m_device->m_queue, 1, &submit, VK_NULL_HANDLE);

    vk::QueueWaitIdle(m_device->m_queue);
}
