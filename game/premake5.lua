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

    debugdir("%{cfg.targetdir}")

    files
    {
        path.join(RootDir, "game", "src", "**.h"),
        path.join(RootDir, "game", "src", "**.hpp"),
        path.join(RootDir, "game", "src", "**.cpp")
    }

    includedirs
    {
        IncludeDir["Engine"],
        path.join(RootDir, "external", "imgui-docking")
    }

    links
    {
        "engine"
    }

    postbuildcommands
    {
        '{COPYDIR} "' .. path.join(RootDir, "game", "assets") .. '" "%{cfg.targetdir}/assets"'
    }


    -- ---------------------------------------------------------------------------------------------------------------------
    -- Windows
    -- ---------------------------------------------------------------------------------------------------------------------

    filter "system:windows"
        systemversion "latest"

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


    -- ---------------------------------------------------------------------------------------------------------------------
    -- Linux
    -- ---------------------------------------------------------------------------------------------------------------------

    filter "system:linux"
        defines
        {
            "GAME_PLATFORM_LINUX",
            "GAME_ASSET_PATH=\"assets\"",
            "GLFW_INCLUDE_NONE"
        }

        links
        {
            "vulkan",
            "glfw"
        }
        postbuildcommands
        {
            'cp -r "' .. path.join(RootDir, "game", "assets") .. '" "%{cfg.targetdir}/"'
        }


    -- ---------------------------------------------------------------------------------------------------------------------
    -- Debug
    -- ---------------------------------------------------------------------------------------------------------------------

    filter "configurations:Debug"
        defines
        {
            "GAME_DEBUG"
        }

        runtime "Debug"
        symbols "On"


    -- ---------------------------------------------------------------------------------------------------------------------
    -- Release
    -- ---------------------------------------------------------------------------------------------------------------------

    filter "configurations:Release"
        defines
        {
            "GAME_RELEASE"
        }

        runtime "Release"
        optimize "On"


    -- ---------------------------------------------------------------------------------------------------------------------
    -- Dist
    -- ---------------------------------------------------------------------------------------------------------------------

    filter "configurations:Dist"
        defines
        {
            "GAME_DIST"
        }

        runtime "Release"
        optimize "Full"
        symbols "Off"


    -- ---------------------------------------------------------------------------------------------------------------------
    -- Windows Dist
    --
    -- Dieser Ordner kann direkt gezippt und weitergegeben werden.
    -- ---------------------------------------------------------------------------------------------------------------------

    filter { "system:windows", "configurations:Dist" }
        staticruntime "on"

        targetdir (
            path.join(
                RootDir,
                "dist",
                "windows-x64"
            )
        )

        debugdir("%{cfg.targetdir}")


    filter {}