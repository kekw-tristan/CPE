project "engine"
    location (path.join(RootDir, "engine"))

    kind "StaticLib"
    language "C++"
    cppdialect "C++20"
    staticruntime "off"

    targetdir (path.join(RootDir, "bin", outputdir, "%{prj.name}"))
    objdir (path.join(RootDir, "bin-int", outputdir, "%{prj.name}"))

    files
    {
        path.join(RootDir, "engine", "src", "**.h"),
        path.join(RootDir, "engine", "src", "**.hpp"),
        path.join(RootDir, "engine", "src", "**.cpp"),

        path.join(RootDir, "external", "imgui-docking", "*.h"),
        path.join(RootDir, "external", "imgui-docking", "*.cpp"),

        path.join(RootDir, "external", "imgui-docking", "backends", "imgui_impl_glfw.h"),
        path.join(RootDir, "external", "imgui-docking", "backends", "imgui_impl_glfw.cpp"),

        path.join(RootDir, "external", "imgui-docking", "backends", "imgui_impl_vulkan.h"),
        path.join(RootDir, "external", "imgui-docking", "backends", "imgui_impl_vulkan.cpp")
    }

    includedirs
    {
        IncludeDir["Engine"],
        path.join(RootDir, "external", "imgui-docking"),
        path.join(RootDir, "external", "imgui-docking", "backends")
    }


    filter "system:windows"
        systemversion "latest"

        defines
        {
            "ENGINE_PLATFORM_WINDOWS",
            "NOMINMAX",
            "WIN32_LEAN_AND_MEAN",
            "GLFW_INCLUDE_NONE"
        }

        includedirs
        {
            IncludeDir["VulkanSDK"]
        }

        libdirs
        {
            LibraryDir["VulkanSDK"]
        }

        links
        {
            "vulkan-1"
        }


    filter "system:linux"
        pic "On"

        defines
        {
            "ENGINE_PLATFORM_LINUX",
            "GLFW_INCLUDE_NONE"
        }

        includedirs
        {
            path.join(RootDir, "vcpkg_installed", "x64-linux", "include")
        }

        links
        {
            "vulkan"
        }


    filter "configurations:Debug"
        defines
        {
            "ENGINE_DEBUG"
        }

        runtime "Debug"
        symbols "On"


    filter "configurations:Release"
        defines
        {
            "ENGINE_RELEASE"
        }

        runtime "Release"
        optimize "On"


    filter "configurations:Dist"
        defines
        {
            "ENGINE_DIST"
        }

        runtime "Release"
        optimize "Full"


    filter { "system:windows", "configurations:Dist" }
        staticruntime "on"


    filter {}