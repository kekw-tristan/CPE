workspace "CPE"
    architecture "x86_64"
    startproject "game"

    configurations
    {
        "Debug",
        "Release",
        "Dist"
    }

    multiprocessorcompile "On"

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

RootDir = _MAIN_SCRIPT_DIR

VulkanSDK = os.getenv("VULKAN_SDK")

if os.host() == "windows" then
    if VulkanSDK == nil or VulkanSDK == "" then
        error(
            "VULKAN_SDK wurde nicht gefunden.\n" ..
            "Installiere das LunarG Vulkan SDK und starte das Terminal neu."
        )
    end

    local vulkanHeader =
        path.join(VulkanSDK, "Include", "vulkan", "vulkan.h")

    local vulkanLibrary =
        path.join(VulkanSDK, "Lib", "vulkan-1.lib")

    if not os.isfile(vulkanHeader) then
        error(
            "Der Vulkan-Header wurde nicht gefunden:\n" ..
            vulkanHeader
        )
    end

    if not os.isfile(vulkanLibrary) then
        error(
            "Die Vulkan-Bibliothek wurde nicht gefunden:\n" ..
            vulkanLibrary
        )
    end
end

IncludeDir = {}
IncludeDir["Engine"] =
    path.join(RootDir, "engine", "src")

LibraryDir = {}

if VulkanSDK ~= nil and VulkanSDK ~= "" then
    IncludeDir["VulkanSDK"] =
        path.join(VulkanSDK, "Include")

    LibraryDir["VulkanSDK"] =
        path.join(VulkanSDK, "Lib")
end

include "engine"
include "game"

