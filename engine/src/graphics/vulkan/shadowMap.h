#pragma once

#include "graphics/vulkan/vulkanImage.h"

#include <vulkan/vulkan.h>
#include<vector>

namespace Engine::GFX
{
	class cShadowMap
	{
		public:

			cShadowMap(); 
		   ~cShadowMap();

		   cShadowMap(const cShadowMap&)					= delete;
		   const cShadowMap& operator= (const cShadowMap&)	= delete;

		public:

			void Create(cVulkanDevice& _rDevice, uint32_t _width, uint32_t _height, uint32_t _layerCount);
			void Destroy(cVulkanDevice& _rDevice); 

		public:

			VkImageView GetImageView()	const;
			VkImage     GetImage()		const;
			VkSampler   GetSampler()	const;
			VkFormat    GetFormat()		const;

			uint32_t GetWidth() const;
			uint32_t GetHeight() const;

			VkImageView GetLayerImageView(uint32_t _layer) const;
			uint32_t GetLayerCount() const;

			cVulkanImage& GetImageResource();
			const cVulkanImage& GetImageResource() const;

		private:

			cVulkanImage	m_depthImage; 
			VkSampler		m_sampler;

			std::vector<VkImageView> m_layerImageViews;
			uint32_t m_layerCount;


	};
}

