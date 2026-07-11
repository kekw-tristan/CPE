#include "vulkanPipeline.h"

#include "graphics/pushConstants.h"

#include "graphics/vulkan/vulkanDevice.h"
#include "graphics/vulkan/vulkanSwapchain.h"
#include "graphics/vulkan/vulkanVertex.h"

#include <fstream>
#include <stdexcept>
#include <array>

// -------------------------------------------------------------------------------------------------------------------------

namespace Engine::GFX
{

    // -------------------------------------------------------------------------------------------------------------------------

    void cVulkanPipeline::Init(cVulkanDevice& _rDevice, cVulkanSwapchain& _rSwapchain)
    {
        auto vertShaderCode = ReadFile("assets/shaders/bin/main.vert.spv");
        auto fragShaderCode = ReadFile("assets/shaders/bin/main.frag.spv");

        VkShaderModule vertShaderModule = CreateShaderModule(_rDevice, vertShaderCode);
        VkShaderModule fragShaderModule = CreateShaderModule(_rDevice, fragShaderCode);


        VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
        
        vertShaderStageInfo.sType   = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertShaderStageInfo.stage   = VK_SHADER_STAGE_VERTEX_BIT;
        vertShaderStageInfo.module  = vertShaderModule;
        vertShaderStageInfo.pName   = "VSMain";

        VkPipelineShaderStageCreateInfo fragShaderStageInfo{};

        fragShaderStageInfo.sType   = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragShaderStageInfo.stage   = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragShaderStageInfo.module  = fragShaderModule;
        fragShaderStageInfo.pName   = "PSMain";

        VkPipelineShaderStageCreateInfo shaderStages[] = 
        {
            vertShaderStageInfo,
            fragShaderStageInfo
        };

        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        
        auto bindingDescription     = sVulkanVertex::GetBindingDescription(); 
        auto attributeDescriptions  = sVulkanVertex::GetAttributeDescriptions();

        vertexInputInfo.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount   = 1;
        vertexInputInfo.pVertexBindingDescriptions      = &bindingDescription;
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
        vertexInputInfo.pVertexAttributeDescriptions    = attributeDescriptions.data();

        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};

        inputAssembly.sType                     = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology                  = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable    = VK_FALSE;

        VkPipelineViewportStateCreateInfo viewportState{};

        viewportState.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.scissorCount  = 1;

        VkPipelineRasterizationStateCreateInfo rasterizer{};

        rasterizer.sType                    = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable         = VK_FALSE;
        rasterizer.rasterizerDiscardEnable  = VK_FALSE;
        rasterizer.polygonMode              = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth                = 1.0f;
        rasterizer.cullMode                 = VK_CULL_MODE_NONE;
        rasterizer.frontFace                = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizer.depthBiasEnable          = VK_FALSE;

        VkPipelineMultisampleStateCreateInfo multisampling{};
        
        multisampling.sType                 = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable   = VK_FALSE;
        multisampling.rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT;

        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        
        colorBlendAttachment.colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT |
            VK_COLOR_COMPONENT_G_BIT |
            VK_COLOR_COMPONENT_B_BIT |
            VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_FALSE;

        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType             = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable     = VK_FALSE;
        colorBlending.attachmentCount   = 1;
        colorBlending.pAttachments      = &colorBlendAttachment;

        std::array<VkDynamicState, 2> dynamicStates = 
        {
            VK_DYNAMIC_STATE_VIEWPORT, 
            VK_DYNAMIC_STATE_SCISSOR
        };

        
        VkPipelineDynamicStateCreateInfo dynamicState{};

        dynamicState.sType              = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount  = static_cast<uint32_t>(dynamicStates.size());
        dynamicState.pDynamicStates     = dynamicStates.data();


        CreateFrameUniformDescriptorSetLayout(_rDevice);
        VkDescriptorSetLayout setLayouts[] =  
        {
            GetFrameUniformDescriptorSetLayout()
        };

        VkPushConstantRange pushConstantRange{};
        
        pushConstantRange.stageFlags    = VK_SHADER_STAGE_VERTEX_BIT;
        pushConstantRange.offset        = 0;
        pushConstantRange.size          = sizeof(sPushConstants); 

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        
        pipelineLayoutInfo.sType                    = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount           = 1;
        pipelineLayoutInfo.pSetLayouts              = setLayouts;
        pipelineLayoutInfo.pushConstantRangeCount   = 1;
        pipelineLayoutInfo.pPushConstantRanges      = &pushConstantRange;

        if (vkCreatePipelineLayout(_rDevice.GetDevice(), &pipelineLayoutInfo, nullptr, &m_pPipelineLayout) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create pipeline layout!"); 
        }

        VkFormat colorAttachmentFormat = _rSwapchain.GetImageFormat(); 

        VkPipelineRenderingCreateInfo pipelineRenderingInfo{};
        pipelineRenderingInfo.sType                     = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
        pipelineRenderingInfo.colorAttachmentCount      = 1;
        pipelineRenderingInfo.pColorAttachmentFormats   = &colorAttachmentFormat;
        pipelineRenderingInfo.depthAttachmentFormat     = VK_FORMAT_D32_SFLOAT;
        pipelineRenderingInfo.stencilAttachmentFormat   = VK_FORMAT_UNDEFINED;

        VkPipelineDepthStencilStateCreateInfo depthStencil{};

        depthStencil.sType                  = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable        = VK_TRUE;
        depthStencil.depthWriteEnable       = VK_TRUE;
        depthStencil.depthCompareOp         = VK_COMPARE_OP_LESS;
        depthStencil.depthBoundsTestEnable  = VK_FALSE;
        depthStencil.minDepthBounds         = 0.0f;
        depthStencil.maxDepthBounds         = 1.0f;
        depthStencil.stencilTestEnable      = VK_FALSE;

        VkGraphicsPipelineCreateInfo pipelineInfo{};

        pipelineInfo.sType                  = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.pNext                  = &pipelineRenderingInfo;
        pipelineInfo.stageCount             = 2;
        pipelineInfo.pStages                = shaderStages;
        pipelineInfo.pVertexInputState      = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState    = &inputAssembly;
        pipelineInfo.pViewportState         = &viewportState;
        pipelineInfo.pRasterizationState    = &rasterizer;
        pipelineInfo.pMultisampleState      = &multisampling;
        pipelineInfo.pDepthStencilState     = &depthStencil;
        pipelineInfo.pColorBlendState       = &colorBlending;
        pipelineInfo.pDynamicState          = &dynamicState;
        pipelineInfo.layout                 = m_pPipelineLayout;
        pipelineInfo.renderPass             = VK_NULL_HANDLE;
        pipelineInfo.subpass                = 0;
        pipelineInfo.basePipelineHandle     = VK_NULL_HANDLE;

        if (vkCreateGraphicsPipelines(_rDevice.GetDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_pGraphicsPipeline) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create graphics pipeline!");
        }

        vkDestroyShaderModule(_rDevice.GetDevice(), fragShaderModule, nullptr);
        vkDestroyShaderModule(_rDevice.GetDevice(), vertShaderModule, nullptr);
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cVulkanPipeline::Shutdown(cVulkanDevice& _rDevice)
    {
        VkDevice device = _rDevice.GetDevice(); 

        if (m_pGraphicsPipeline != VK_NULL_HANDLE)
        {
            vkDestroyPipeline(device, m_pGraphicsPipeline, nullptr);
            m_pGraphicsPipeline = VK_NULL_HANDLE;
        }

        if (m_pPipelineLayout != VK_NULL_HANDLE)
        {
            vkDestroyPipelineLayout(device, m_pPipelineLayout, nullptr); 
            m_pPipelineLayout = VK_NULL_HANDLE;
        }

        if (m_pFrameUniformDescriptorSetLayout != VK_NULL_HANDLE)
        {
            vkDestroyDescriptorSetLayout(device, m_pFrameUniformDescriptorSetLayout, nullptr);
            m_pFrameUniformDescriptorSetLayout = VK_NULL_HANDLE;
        }
    }

    // -------------------------------------------------------------------------------------------------------------------------

    VkPipeline cVulkanPipeline::GetPipeline()
    {
        return m_pGraphicsPipeline;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    VkPipelineLayout cVulkanPipeline::GetPipelineLayout()
    {
        return m_pPipelineLayout;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    VkDescriptorSetLayout cVulkanPipeline::GetFrameUniformDescriptorSetLayout()
    {
        return m_pFrameUniformDescriptorSetLayout;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    std::vector<char> cVulkanPipeline::ReadFile(const std::string &_rFileName)
    {
        std::ifstream file(_rFileName, std::ios::ate | std::ios::binary);

        if (!file.is_open())
        {
            throw std::runtime_error("Failed to open file: " + _rFileName);
        }

        size_t fileSize = static_cast<size_t>(file.tellg()); 
        std::vector<char> buffer(fileSize); 

        file.seekg(0);
        file.read(buffer.data(), fileSize);
        file.close();

        return buffer;
    }

    // -------------------------------------------------------------------------------------------------------------------------

    VkShaderModule cVulkanPipeline::CreateShaderModule(cVulkanDevice &_rDevice, const std::vector<char> &_rCode)
    {
        VkShaderModuleCreateInfo createInfo{};

        createInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = _rCode.size();
        createInfo.pCode    = reinterpret_cast<const uint32_t*>(_rCode.data());

        VkShaderModule shaderModule = VK_NULL_HANDLE;

        if (vkCreateShaderModule(_rDevice.GetDevice(), &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create shader module!");
        }

        return shaderModule; 

    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cVulkanPipeline::CreateFrameUniformDescriptorSetLayout(cVulkanDevice &_rDevice)
    {
        VkDescriptorSetLayoutBinding frameBinding{}; 

        frameBinding.binding            = 0;
        frameBinding.descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        frameBinding.descriptorCount    = 1;
        frameBinding.stageFlags         = VK_SHADER_STAGE_VERTEX_BIT;
        frameBinding.pImmutableSamplers = nullptr;

        VkDescriptorSetLayoutCreateInfo layoutInfo{};

        layoutInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = 1;
        layoutInfo.pBindings    = &frameBinding;

        if (vkCreateDescriptorSetLayout(_rDevice.GetDevice(), &layoutInfo, nullptr, &m_pFrameUniformDescriptorSetLayout) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create frame uniform descriptor set layout");
        }

    }

    // -------------------------------------------------------------------------------------------------------------------------

}

// -------------------------------------------------------------------------------------------------------------------------