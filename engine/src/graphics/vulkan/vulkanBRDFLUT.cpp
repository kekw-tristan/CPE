#include "vulkanBRDFLUT.h"

#include "graphics/vulkan/vulkanBuffer.h"
#include "graphics/vulkan/vulkanCommands.h"
#include "graphics/vulkan/vulkanDevice.h"

#include "math/vector3.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <vector>

// -------------------------------------------------------------------------------------------------------------------------

namespace Engine::GFX
{

    namespace
    {

        struct sFloat2
        {
            float x;
            float y;
        };

        // ---------------------------------------------------------------------------------------------------------------------

        float Saturate(float _value)
        {
            return std::clamp(_value, 0.0f, 1.0f);
        }

        // ---------------------------------------------------------------------------------------------------------------------

        float RadicalInverseVdC(uint32_t _bits)
        {
            _bits = (_bits << 16u) | (_bits >> 16u);
            _bits = ((_bits & 0x55555555u) << 1u) | ((_bits & 0xAAAAAAAAu) >> 1u);
            _bits = ((_bits & 0x33333333u) << 2u) | ((_bits & 0xCCCCCCCCu) >> 2u);
            _bits = ((_bits & 0x0F0F0F0Fu) << 4u) | ((_bits & 0xF0F0F0F0u) >> 4u);
            _bits = ((_bits & 0x00FF00FFu) << 8u) | ((_bits & 0xFF00FF00u) >> 8u);

            return static_cast<float>(_bits) * 2.3283064365386963e-10f;
        }

        // ---------------------------------------------------------------------------------------------------------------------

        sFloat2 Hammersley(uint32_t _index, uint32_t _sampleCount)
        {
            return
            {
                static_cast<float>(_index) / static_cast<float>(_sampleCount),
                RadicalInverseVdC(_index)
            };
        }

        // ---------------------------------------------------------------------------------------------------------------------

        Math::cVec3f ImportanceSampleGGX(const sFloat2& _rXi, const Math::cVec3f& _rNormal, float _roughness)
        {
            constexpr float c_pi = 3.14159265358979323846f;

            const float alpha = _roughness * _roughness;
            const float alphaSquared = alpha * alpha;

            const float phi = 2.0f * c_pi * _rXi.x;

            const float cosTheta = std::sqrt((1.0f - _rXi.y) / (1.0f + (alphaSquared - 1.0f) * _rXi.y));
            const float sinTheta = std::sqrt(std::max(0.0f, 1.0f - cosTheta * cosTheta));

            const Math::cVec3f halfVectorTangent =
            {
                std::cos(phi) * sinTheta,
                std::sin(phi) * sinTheta,
                cosTheta
            };

            const Math::cVec3f up = std::abs(_rNormal.z()) < 0.999f ? Math::cVec3f{ 0.0f, 0.0f, 1.0f } : Math::cVec3f{ 1.0f, 0.0f, 0.0f };

            const Math::cVec3f tangent = up.cross(_rNormal).normalized();
            const Math::cVec3f bitangent = _rNormal.cross(tangent);

            return (tangent * halfVectorTangent.x() + bitangent * halfVectorTangent.y() + _rNormal * halfVectorTangent.z()).normalized();
        }

        // ---------------------------------------------------------------------------------------------------------------------

        float GeometrySchlickGGXIBL(float _NdotV, float _roughness)
        {
            const float k = (_roughness * _roughness) * 0.5f;

            return _NdotV / std::max(_NdotV * (1.0f - k) + k, 0.000001f);
        }

        // ---------------------------------------------------------------------------------------------------------------------

        float GeometrySmithIBL(float _NdotV, float _NdotL, float _roughness)
        {
            const float geometryView = GeometrySchlickGGXIBL(_NdotV, _roughness);
            const float geometryLight = GeometrySchlickGGXIBL(_NdotL, _roughness);

            return geometryView * geometryLight;
        }

        // ---------------------------------------------------------------------------------------------------------------------

        sFloat2 IntegrateBRDF(float _NdotV, float _roughness)
        {
            constexpr uint32_t c_sampleCount = 256;

            const Math::cVec3f normal = { 0.0f, 0.0f, 1.0f };

            const Math::cVec3f viewDirection =
            {
                std::sqrt(std::max(0.0f, 1.0f - _NdotV * _NdotV)),
                0.0f,
                _NdotV
            };

            float scale = 0.0f;
            float bias = 0.0f;

            for (uint32_t sampleIndex = 0; sampleIndex < c_sampleCount; ++sampleIndex)
            {
                const sFloat2 xi = Hammersley(sampleIndex, c_sampleCount);
                const Math::cVec3f halfVector = ImportanceSampleGGX(xi, normal, _roughness);

                const float VdotH = Saturate(viewDirection.dot(halfVector));

                const Math::cVec3f lightDirection = (halfVector * (2.0f * VdotH) - viewDirection).normalized();

                const float NdotL = Saturate(lightDirection.z());
                const float NdotH = Saturate(halfVector.z());

                if (NdotL <= 0.0f)
                {
                    continue;
                }

                const float geometry = GeometrySmithIBL(_NdotV, NdotL, _roughness);
                const float geometryVisibility = (geometry * VdotH) / std::max(NdotH * _NdotV, 0.000001f);

                const float fresnel = std::pow(1.0f - VdotH, 5.0f);

                scale += (1.0f - fresnel) * geometryVisibility;
                bias += fresnel * geometryVisibility;
            }

            const float inverseSampleCount = 1.0f / static_cast<float>(c_sampleCount);

            return
            {
                scale * inverseSampleCount,
                bias * inverseSampleCount
            };
        }

    }

    // -------------------------------------------------------------------------------------------------------------------------

    cVulkanBRDFLUT::cVulkanBRDFLUT()
        : m_image()
        , m_sampler(VK_NULL_HANDLE)
    {
    }

    // -------------------------------------------------------------------------------------------------------------------------

    cVulkanBRDFLUT::~cVulkanBRDFLUT()
    {
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cVulkanBRDFLUT::Create(cVulkanDevice& _rDevice, cVulkanCommands& _rCommands)
    {
        constexpr uint32_t lutSize = 256;
        constexpr uint32_t channelCount = 2;

        std::vector<float> pixels(static_cast<size_t>(lutSize) * lutSize * channelCount);

        for (uint32_t y = 0; y < lutSize; ++y)
        {
            for (uint32_t x = 0; x < lutSize; ++x)
            {
                const float NdotV = (static_cast<float>(x) + 0.5f) / static_cast<float>(lutSize);
                const float roughness = (static_cast<float>(y) + 0.5f) / static_cast<float>(lutSize);

                const sFloat2 integratedBRDF = IntegrateBRDF(NdotV, roughness);

                const size_t pixelIndex = (static_cast<size_t>(y) * lutSize + x) * channelCount;

                pixels[pixelIndex + 0] = integratedBRDF.x;
                pixels[pixelIndex + 1] = integratedBRDF.y;
            }
        }

        const VkDeviceSize totalSize = static_cast<VkDeviceSize>(pixels.size()) * sizeof(float);

        cVulkanBuffer stagingBuffer;

        stagingBuffer.Create(_rDevice, totalSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        stagingBuffer.Map(_rDevice, totalSize, 0);
        stagingBuffer.Write(pixels.data(), totalSize, 0);
        stagingBuffer.Unmap(_rDevice);

        m_image.Create(_rDevice, lutSize, lutSize, VK_FORMAT_R32G32_SFLOAT, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT, VK_SAMPLE_COUNT_1_BIT);

        VkCommandBuffer pCommandBuffer = _rCommands.BeginSingleTimeCommands(_rDevice);

        m_image.TransitionLayout(_rDevice, pCommandBuffer, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);
        m_image.CopyFromBuffer(pCommandBuffer, stagingBuffer.GetBuffer(), totalSize, VK_IMAGE_ASPECT_COLOR_BIT);
        m_image.TransitionLayout(_rDevice, pCommandBuffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);

        _rCommands.EndSingleTimeCommands(_rDevice, pCommandBuffer);

        stagingBuffer.Shutdown(_rDevice);

        VkSamplerCreateInfo samplerInfo{};

        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.mipLodBias = 0.0f;
        samplerInfo.anisotropyEnable = VK_FALSE;
        samplerInfo.maxAnisotropy = 1.0f;
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = 0.0f;
        samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;

        if (vkCreateSampler(_rDevice.GetDevice(), &samplerInfo, nullptr, &m_sampler) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create Vulkan BRDF LUT sampler!");
        }
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cVulkanBRDFLUT::Destroy(cVulkanDevice& _rDevice)
    {
        if (m_sampler != VK_NULL_HANDLE)
        {
            vkDestroySampler(_rDevice.GetDevice(), m_sampler, nullptr);
            m_sampler = VK_NULL_HANDLE;
        }

        m_image.Destroy(_rDevice);
    }

    // -------------------------------------------------------------------------------------------------------------------------

    VkImageView cVulkanBRDFLUT::GetImageView() const
    {
        return m_image.GetImageView();
    }

    // -------------------------------------------------------------------------------------------------------------------------

    VkSampler cVulkanBRDFLUT::GetSampler() const
    {
        return m_sampler;
    }

    // -------------------------------------------------------------------------------------------------------------------------

}

// -------------------------------------------------------------------------------------------------------------------------