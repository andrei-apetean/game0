#include "render_module/vulkan_pipeline.h"

#include <stdio.h>
#include <vulkan/vulkan_core.h>

#include "math_types.h"
#include "render_module/render_types.h"
#include "render_module/vulkan_types.h"

static int32_t create_render_pass(vulkan_device*          device,
                                  vulkan_pipeline_config* config,
                                  VkRenderPass*           out_renderpass);

static int32_t create_pipeline_layout(vulkan_device*    device,
                                      VkPipelineLayout* out_layout);

static int32_t create_graphics_pipeline(vulkan_device*          device,
                                        vulkan_pipeline*        pipeline,
                                        vulkan_pipeline_config* config);

int32_t vulkan_pipeline_create(vulkan_device* device, vulkan_pipeline* pipeline,
                               vulkan_pipeline_config* config) {
    int32_t err = 0;
    err = create_render_pass(device, config, &pipeline->renderpass);
    if (err) return err;

    err = create_pipeline_layout(device, &pipeline->layout);
    if (err) return err;

    err = create_graphics_pipeline(device, pipeline, config);
    return 0;
}

void vulkan_pipeline_destroy(vulkan_device* device, vulkan_pipeline* pipeline) {
    vkDestroyRenderPass(device->logical_device, pipeline->renderpass,
                        device->allocator);
    vkDestroyPipelineLayout(device->logical_device, pipeline->layout,
                            device->allocator);
    vkDestroyPipeline(device->logical_device, pipeline->handle, device->allocator);
}

static int32_t create_render_pass(vulkan_device*          device,
                                  vulkan_pipeline_config* config,
                                  VkRenderPass*           out_renderpass) {
    int32_t      err = 0;
    VkRenderPass renderpass;

    VkAttachmentDescription depth_attachment = {
        .format = config->depth_format,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    };

    VkAttachmentDescription color_attachment = {
        .format = config->color_format,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
    };

    VkAttachmentReference color_attachment_ref = {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };

    VkAttachmentReference depth_attachment_ref = {
        .attachment = 1,
        .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    };
    VkSubpassDescription subpass = {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments = &color_attachment_ref,
        .pDepthStencilAttachment = &depth_attachment_ref,
    };

    VkAttachmentDescription attachments[2] = {
        color_attachment,
        depth_attachment,
    };

    VkRenderPassCreateInfo renderpass_info = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = sizeof(attachments) / sizeof(attachments[0]),
        .pAttachments = attachments,
        .subpassCount = 1,
        .pSubpasses = &subpass,
    };

    VK_CHECK(vkCreateRenderPass(device->logical_device, &renderpass_info,
                                device->allocator, &renderpass));
    *out_renderpass = renderpass;
    printf("Renderpass created successfully\n");
    return 0;
}

static int32_t create_pipeline_layout(vulkan_device*    device,
                                      VkPipelineLayout* out_layout) {
    VkPushConstantRange push_constant_range = {
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        .offset = 0,
        .size = sizeof(mat4),
    };

    VkPipelineLayoutCreateInfo pipeline_layout_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pushConstantRangeCount = 1,
        .pPushConstantRanges = &push_constant_range,
        .setLayoutCount = 0,
        .pSetLayouts = NULL,
    };
    VkPipelineLayout pipeline_layout;
    vkCreatePipelineLayout(device->logical_device, &pipeline_layout_info,
                           device->allocator, &pipeline_layout);
    *out_layout = pipeline_layout;
    return 0;
}

static int32_t create_graphics_pipeline(vulkan_device*          device,
                                        vulkan_pipeline*        pipeline,
                                        vulkan_pipeline_config* config) {
    VkPipelineShaderStageCreateInfo shader_stages[2] = {
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_VERTEX_BIT,
            .module = config->vertex_shader,
            .pName = "main",
        },
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
            .module = config->fragment_shader,
            .pName = "main",
        }};

    VkPipelineViewportStateCreateInfo viewport_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .pViewports = NULL,  // ignored
        .scissorCount = 1,
        .pScissors = NULL};

    VkDynamicState dynamic_states[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
        VK_DYNAMIC_STATE_LINE_WIDTH,
    };

    VkPipelineDynamicStateCreateInfo dynamic_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = sizeof(dynamic_states) / sizeof(dynamic_states[0]),
        .pDynamicStates = dynamic_states,
    };

    VkVertexInputBindingDescription vertex_binding_desc = {
        .binding = 0,
        .stride = sizeof(vertex),  // todo
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
    };

    VkVertexInputAttributeDescription vertex_attr_desc[] = {
        {
            .binding = 0,
            .location = 0,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = 0,
        },
        {

            .binding = 0,
            .location = 1,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = offsetof(vertex, r),
        },
    };

    // Vertex input
    VkPipelineVertexInputStateCreateInfo vertex_input_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = &vertex_binding_desc,
        .vertexAttributeDescriptionCount = 2,
        .pVertexAttributeDescriptions = vertex_attr_desc,
    };

    // Input assembly
    VkPipelineInputAssemblyStateCreateInfo input_assembly = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE,
    };

    // Rasterizer
    VkPipelineRasterizationStateCreateInfo rasterizer = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .lineWidth = 1.0f,
        .cullMode = VK_CULL_MODE_NONE,
        .frontFace = VK_FRONT_FACE_CLOCKWISE,
        .depthBiasEnable = VK_FALSE,
    };

    // Multisampling
    VkPipelineMultisampleStateCreateInfo multisampling = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        .sampleShadingEnable = VK_FALSE,
    };

    // Color blending
    VkPipelineColorBlendAttachmentState color_blend_attachment = {
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                          VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
        .blendEnable = VK_FALSE,
    };

    VkPipelineColorBlendStateCreateInfo color_blending = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &color_blend_attachment,
    };

    VkPipelineDepthStencilStateCreateInfo depth_stencil = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable = VK_TRUE,
        .depthWriteEnable = VK_TRUE,
        .depthCompareOp = VK_COMPARE_OP_LESS,
        .depthBoundsTestEnable = VK_FALSE,
        .stencilTestEnable = VK_FALSE,
    };

    VkGraphicsPipelineCreateInfo pipeline_info = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount = 2,
        .pStages = shader_stages,
        .pVertexInputState = &vertex_input_info,
        .pInputAssemblyState = &input_assembly,
        .pViewportState = &viewport_state,
        .pRasterizationState = &rasterizer,
        .pMultisampleState = &multisampling,
        .pDepthStencilState = &depth_stencil,
        .pColorBlendState = &color_blending,
        .pDynamicState = &dynamic_state,
        .layout = pipeline->layout,
        .renderPass = pipeline->renderpass,  // If using dynamic rendering
        .subpass = 0,
    };
    VK_CHECK(vkCreateGraphicsPipelines(device->logical_device, VK_NULL_HANDLE, 1,
                                       &pipeline_info, device->allocator,
                                       &pipeline->handle));
    printf("Graphics pipeline created successfully\n");
    return 0;
}
