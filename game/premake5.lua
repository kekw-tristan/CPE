project "game"
    location (path.join(RootDir, "game"))

    kind "ConsoleApp"
    language "C++"
    cppdialect "C++20"
    staticruntime "off"

    targetdir (
        path.join(
            RootDir,
            "bin",
            outputdir,
            "%{prj.name}"
        )
    )

    objdir (
        path.join(
            RootDir,
            "bin-int",
            outputdir,
            "%{prj.name}"
        )
    )

    debugdir "%{cfg.targetdir}"

    files
    {
        path.join(RootDir, "game", "src", "**.h"),
        path.join(RootDir, "game", "src", "**.hpp"),
        path.join(RootDir, "game", "src", "**.cpp")
    }

    includedirs
    {
        IncludeDir["Engine"]
    }

    links
    {
        "engine"
    }

    postbuildcommands
    {
        '{COPYDIR} "' ..
        path.join(RootDir, "game", "assets") ..
        '" "%{cfg.targetdir}/assets"'
    }

    filter "system:windows"
        defines
        {
            "GAME_PLATFORM_WINDOWS",
            "GAME_ASSET_PATH=\"assets\"",
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
    defines
        {
            "GAME_PLATFORM_LINUX",
            "GAME_ASSET_PATH=\"game/assets\"",
            "GLFW_INCLUDE_NONE"
        }

        links
        {
            "vulkan",
            "glfw"
        }

    filter "configurations:Debug"
        defines
        {
            "GAME_DEBUG"
        }

        runtime "Debug"
        symbols "On"

    filter "configurations:Release"
        defines
        {
            "GAME_RELEASE"
        }

        runtime "Release"
        optimize "On"

    filter "configurations:Dist"
        defines
        {
            "GAME_DIST"
        }

        runtime "Release"
        optimize "Full"

    filter {}