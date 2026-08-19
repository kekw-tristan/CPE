#include "vulkanEnvironment.h"

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

        // ---------------------------------------------------------------------------------------------------------------------

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

        Math::cVec3f GetCubeDirection(uint32_t _face, uint32_t _x, uint32_t _y, uint32_t _size)
        {
            const float u = (2.0f * (static_cast<float>(_x) + 0.5f) / static_cast<float>(_size)) - 1.0f;
            const float v = (2.0f * (static_cast<float>(_y) + 0.5f) / static_cast<float>(_size)) - 1.0f;

            Math::cVec3f direction;

            switch (_face)
            {
                case 0: direction = { 1.0f, -v,    -u }; break;     // +X
                case 1: direction = { -1.0f, -v,     u }; break;    // -X
                case 2: direction = { u,     1.0f,  v }; break;     // +Y
                case 3: direction = { u,    -1.0f, -v }; break;     // -Y
                case 4: direction = { u,    -v,     1.0f }; break;  // +Z
                case 5: direction = { -u,    -v,    -1.0f }; break; // -Z
                default: direction = { 0.0f, 1.0f, 0.0f }; break;
            }

            return direction.normalized();
        }

        // ---------------------------------------------------------------------------------------------------------------------

        Math::cVec3f EvaluateEnvironment(const Math::cVec3f& _rDirection)
        {
            // -------------------------------------------------------------------------------------------------------------------------
            // Base environment
            // -------------------------------------------------------------------------------------------------------------------------

            const Math::cVec3f groundColor = { 0.030f, 0.045f, 0.035f };
            const Math::cVec3f horizonColor = { 0.14f, 0.18f, 0.24f };
            const Math::cVec3f skyColor = { 0.035f, 0.075f, 0.18f };

            float hemisphere = Saturate(_rDirection.y() * 0.5f + 0.5f);

            // Smooth the hemisphere transition.
            hemisphere = hemisphere * hemisphere * (3.0f - 2.0f * hemisphere);

            Math::cVec3f color = Math::cVec3f::lerp(groundColor, skyColor, hemisphere);

            // -------------------------------------------------------------------------------------------------------------------------
            // Soft horizon
            // -------------------------------------------------------------------------------------------------------------------------

            const float horizonBase = Saturate(1.0f - std::abs(_rDirection.y()));
            const float horizonFactor = std::pow(horizonBase, 4.0f) * 0.30f;

            color += horizonColor * horizonFactor;

            // -------------------------------------------------------------------------------------------------------------------------
            // Warm key reflection
            // -------------------------------------------------------------------------------------------------------------------------

            const Math::cVec3f keyDirection = Math::cVec3f(-0.65f, 0.45f, -0.55f).normalized();
            const Math::cVec3f keyColor = { 2.2f, 1.65f, 1.05f };

            const float keyFactor = std::pow(Saturate(_rDirection.dot(keyDirection)), 64.0f);

            color += keyColor * keyFactor;

            // -------------------------------------------------------------------------------------------------------------------------
            // Cool fill reflection
            // -------------------------------------------------------------------------------------------------------------------------

            const Math::cVec3f fillDirection = Math::cVec3f(0.70f, 0.15f, 0.45f).normalized();
            const Math::cVec3f fillColor = { 0.18f, 0.38f, 1.0f };

            const float fillFactor = std::pow(Saturate(_rDirection.dot(fillDirection)), 28.0f);

            color += fillColor * fillFactor;

            // -------------------------------------------------------------------------------------------------------------------------
            // Overhead reflection
            // -------------------------------------------------------------------------------------------------------------------------

            const Math::cVec3f topDirection = { 0.0f, 1.0f, 0.0f };
            const Math::cVec3f topColor = { 0.55f, 0.65f, 0.85f };

            const float topFactor = std::pow(Saturate(_rDirection.dot(topDirection)), 8.0f);

            color += topColor * topFactor;

            // -------------------------------------------------------------------------------------------------------------------------
            // Ground bounce
            // -------------------------------------------------------------------------------------------------------------------------

            const float groundFactor = std::pow(Saturate(-_rDirection.y()), 2.0f);

            color += Math::cVec3f(0.025f, 0.055f, 0.025f) * groundFactor;
            return { 0.15f, 0.18f, 0.22f };
            return color;
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

        Math::cVec3f PrefilterEnvironment(const Math::cVec3f& _rReflectionDirection, float _roughness)
        {
            if (_roughness <= 0.001f)
            {
                return EvaluateEnvironment(_rReflectionDirection);
            }

            constexpr uint32_t c_sampleCount = 128;

            const Math::cVec3f normal = _rReflectionDirection;
            const Math::cVec3f viewDirection = _rReflectionDirection;

            Math::cVec3f prefilteredColor = { 0.0f, 0.0f, 0.0f };
            float totalWeight = 0.0f;

            for (uint32_t sampleIndex = 0; sampleIndex < c_sampleCount; ++sampleIndex)
            {
                const sFloat2 xi = Hammersley(sampleIndex, c_sampleCount);
                const Math::cVec3f halfVector = ImportanceSampleGGX(xi, normal, _roughness);

                const float VdotH = std::max(viewDirection.dot(halfVector), 0.0f);

                const Math::cVec3f lightDirection = (halfVector * (2.0f * VdotH) - viewDirection).normalized();

                const float NdotL = std::max(normal.dot(lightDirection), 0.0f);

                if (NdotL <= 0.0f)
                {
                    continue;
                }

                prefilteredColor += EvaluateEnvironment(lightDirection) * NdotL;
                totalWeight += NdotL;
            }

            if (totalWeight <= 0.000001f)
            {
                return EvaluateEnvironment(_rReflectionDirection);
            }

            return prefilteredColor / totalWeight;
        }

        // ---------------------------------------------------------------------------------------------------------------------

        Math::cVec3f CosineSampleHemisphere(const sFloat2& _rXi, const Math::cVec3f& _rNormal)
        {
            constexpr float c_pi = 3.14159265358979323846f;

            const float radius = std::sqrt(_rXi.x);
            const float phi = 2.0f * c_pi * _rXi.y;

            const float x = radius * std::cos(phi);
            const float y = radius * std::sin(phi);
            const float z = std::sqrt(std::max(0.0f, 1.0f - _rXi.x));

            const Math::cVec3f up = std::abs(_rNormal.z()) < 0.999f ? Math::cVec3f{ 0.0f, 0.0f, 1.0f } : Math::cVec3f{ 1.0f, 0.0f, 0.0f };

            const Math::cVec3f tangent = up.cross(_rNormal).normalized();
            const Math::cVec3f bitangent = _rNormal.cross(tangent);

            return (tangent * x + bitangent * y + _rNormal * z).normalized();
        }

        // ---------------------------------------------------------------------------------------------------------------------

        Math::cVec3f EvaluateDiffuseIrradiance(const Math::cVec3f& _rNormal)
        {
            constexpr uint32_t c_sampleCount = 256;

            Math::cVec3f irradiance = { 0.0f, 0.0f, 0.0f };

            for (uint32_t sampleIndex = 0; sampleIndex < c_sampleCount; ++sampleIndex)
            {
                const sFloat2 xi = Hammersley(sampleIndex, c_sampleCount);
                const Math::cVec3f sampleDirection = CosineSampleHemisphere(xi, _rNormal);

                irradiance += EvaluateEnvironment(sampleDirection);
            }

            return irradiance / static_cast<float>(c_sampleCount);
        }

    }

    // -------------------------------------------------------------------------------------------------------------------------

    cVulkanEnvironment::cVulkanEnvironment()
        : m_environmentImage()
        , m_irradianceImage()
        , m_sampler(VK_NULL_HANDLE)
    {
    }

    // -------------------------------------------------------------------------------------------------------------------------

    cVulkanEnvironment::~cVulkanEnvironment()
    {
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cVulkanEnvironment::Create(cVulkanDevice& _rDevice, cVulkanCommands& _rCommands)
    {
        constexpr uint32_t cubeSize = 128;
        constexpr uint32_t irradianceSize = 32;
        constexpr uint32_t channelCount = 4;
        constexpr uint32_t faceCount = 6;

        const uint32_t mipLevels = static_cast<uint32_t>(std::floor(std::log2(static_cast<float>(cubeSize)))) + 1;

        // ---------------------------------------------------------------------------------------------------------------------
        // GGX prefiltered environment
        // ---------------------------------------------------------------------------------------------------------------------

        std::vector<float> environmentPixels;

        size_t totalPixelCount = 0;

        uint32_t mipWidth = cubeSize;
        uint32_t mipHeight = cubeSize;

        for (uint32_t mipLevel = 0; mipLevel < mipLevels; ++mipLevel)
        {
            totalPixelCount += static_cast<size_t>(mipWidth) * mipHeight * faceCount;

            mipWidth = std::max(1u, mipWidth / 2);
            mipHeight = std::max(1u, mipHeight / 2);
        }

        environmentPixels.reserve(totalPixelCount * channelCount);

        mipWidth = cubeSize;
        mipHeight = cubeSize;

        for (uint32_t mipLevel = 0; mipLevel < mipLevels; ++mipLevel)
        {
            const float roughness = mipLevels > 1 ? static_cast<float>(mipLevel) / static_cast<float>(mipLevels - 1) : 0.0f;

            for (uint32_t face = 0; face < faceCount; ++face)
            {
                for (uint32_t y = 0; y < mipHeight; ++y)
                {
                    for (uint32_t x = 0; x < mipWidth; ++x)
                    {
                        const Math::cVec3f direction = GetCubeDirection(face, x, y, mipWidth);
                        const Math::cVec3f color = PrefilterEnvironment(direction, roughness);

                        environmentPixels.push_back(color.x());
                        environmentPixels.push_back(color.y());
                        environmentPixels.push_back(color.z());
                        environmentPixels.push_back(1.0f);
                    }
                }
            }

            mipWidth = std::max(1u, mipWidth / 2);
            mipHeight = std::max(1u, mipHeight / 2);
        }

        const VkDeviceSize environmentTotalSize = static_cast<VkDeviceSize>(environmentPixels.size()) * sizeof(float);

        cVulkanBuffer environmentStagingBuffer;

        environmentStagingBuffer.Create(_rDevice, environmentTotalSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        environmentStagingBuffer.Map(_rDevice, environmentTotalSize, 0);
        environmentStagingBuffer.Write(environmentPixels.data(), environmentTotalSize, 0);
        environmentStagingBuffer.Unmap(_rDevice);

        m_environmentImage.Create(_rDevice, cubeSize, cubeSize, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT, VK_SAMPLE_COUNT_1_BIT, faceCount, VK_IMAGE_VIEW_TYPE_CUBE, VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT, mipLevels);

        VkCommandBuffer pEnvironmentCommandBuffer = _rCommands.BeginSingleTimeCommands(_rDevice);

        m_environmentImage.TransitionLayout(_rDevice, pEnvironmentCommandBuffer, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);
        m_environmentImage.CopyMipChainFromBuffer(pEnvironmentCommandBuffer, environmentStagingBuffer.GetBuffer(), channelCount * sizeof(float), VK_IMAGE_ASPECT_COLOR_BIT);
        m_environmentImage.TransitionLayout(_rDevice, pEnvironmentCommandBuffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);

        _rCommands.EndSingleTimeCommands(_rDevice, pEnvironmentCommandBuffer);

        environmentStagingBuffer.Shutdown(_rDevice);

        // ---------------------------------------------------------------------------------------------------------------------
        // Diffuse irradiance
        // ---------------------------------------------------------------------------------------------------------------------

        const VkDeviceSize irradianceFaceSize = static_cast<VkDeviceSize>(irradianceSize) * irradianceSize * channelCount * sizeof(float);
        const VkDeviceSize irradianceTotalSize = irradianceFaceSize * faceCount;

        std::vector<float> irradiancePixels(static_cast<size_t>(irradianceSize) * irradianceSize * channelCount * faceCount);

        for (uint32_t face = 0; face < faceCount; ++face)
        {
            for (uint32_t y = 0; y < irradianceSize; ++y)
            {
                for (uint32_t x = 0; x < irradianceSize; ++x)
                {
                    const Math::cVec3f direction = GetCubeDirection(face, x, y, irradianceSize);
                    const Math::cVec3f irradiance = EvaluateDiffuseIrradiance(direction);

                    const size_t pixelIndex = (((static_cast<size_t>(face) * irradianceSize + y) * irradianceSize + x) * channelCount);

                    irradiancePixels[pixelIndex + 0] = irradiance.x();
                    irradiancePixels[pixelIndex + 1] = irradiance.y();
                    irradiancePixels[pixelIndex + 2] = irradiance.z();
                    irradiancePixels[pixelIndex + 3] = 1.0f;
                }
            }
        }

        cVulkanBuffer irradianceStagingBuffer;

        irradianceStagingBuffer.Create(_rDevice, irradianceTotalSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        irradianceStagingBuffer.Map(_rDevice, irradianceTotalSize, 0);
        irradianceStagingBuffer.Write(irradiancePixels.data(), irradianceTotalSize, 0);
        irradianceStagingBuffer.Unmap(_rDevice);

        m_irradianceImage.Create(_rDevice, irradianceSize, irradianceSize, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT, VK_SAMPLE_COUNT_1_BIT, faceCount, VK_IMAGE_VIEW_TYPE_CUBE, VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT);

        VkCommandBuffer pIrradianceCommandBuffer = _rCommands.BeginSingleTimeCommands(_rDevice);

        m_irradianceImage.TransitionLayout(_rDevice, pIrradianceCommandBuffer, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);
        m_irradianceImage.CopyFromBuffer(pIrradianceCommandBuffer, irradianceStagingBuffer.GetBuffer(), irradianceFaceSize, VK_IMAGE_ASPECT_COLOR_BIT);
        m_irradianceImage.TransitionLayout(_rDevice, pIrradianceCommandBuffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);

        _rCommands.EndSingleTimeCommands(_rDevice, pIrradianceCommandBuffer);

        irradianceStagingBuffer.Shutdown(_rDevice);

        // ---------------------------------------------------------------------------------------------------------------------
        // Sampler
        // ---------------------------------------------------------------------------------------------------------------------

        VkSamplerCreateInfo samplerInfo{};

        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.mipLodBias = 0.0f;
        samplerInfo.anisotropyEnable = VK_FALSE;
        samplerInfo.maxAnisotropy = 1.0f;
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
        samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = static_cast<float>(m_environmentImage.GetMipLevels() - 1);

        if (vkCreateSampler(_rDevice.GetDevice(), &samplerInfo, nullptr, &m_sampler) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create Vulkan environment sampler!");
        }
    }

    // -------------------------------------------------------------------------------------------------------------------------

    void cVulkanEnvironment::Destroy(cVulkanDevice& _rDevice)
    {
        if (m_sampler != VK_NULL_HANDLE)
        {
            vkDestroySampler(_rDevice.GetDevice(), m_sampler, nullptr);
            m_sampler = VK_NULL_HANDLE;
        }

        m_irradianceImage.Destroy(_rDevice);
        m_environmentImage.Destroy(_rDevice);
    }

    // -------------------------------------------------------------------------------------------------------------------------

    VkImageView cVulkanEnvironment::GetImageView() const
    {
        return m_environmentImage.GetImageView();
    }

    // -------------------------------------------------------------------------------------------------------------------------

    VkImageView cVulkanEnvironment::GetIrradianceImageView() const
    {
        return m_irradianceImage.GetImageView();
    }

    // -------------------------------------------------------------------------------------------------------------------------

    VkSampler cVulkanEnvironment::GetSampler() const
    {
        return m_sampler;
    }

    // -------------------------------------------------------------------------------------------------------------------------

}

// -------------------------------------------------------------------------------------------------------------------------