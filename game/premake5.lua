project "game"
    location "game"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++20"
    staticruntime "off"

    targetdir ("bin/" .. outputdir .. "/%{prj.name}")
    objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

    files
    {
        "src/**.h",
        "src/**.hpp",
        "src/**.cpp"
    }

    includedirs
    {
        "../engine/src"
    }

    links
    {
        "engine"
    }

    filter "system:linux"
        defines
        {
            "GAME_PLATFORM_LINUX"
        }

    filter "configurations:Debug"
        defines
        {
            "GAME_DEBUG"
        }
        symbols "On"

    filter "configurations:Release"
        defines
        {
            "GAME_RELEASE"
        }
        optimize "On"

    filter "configurations:Dist"
        defines
        {
            "GAME_DIST"
        }
        optimize "Full"