project "engine"
    location "engine"
    kind "StaticLib"
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
        "src"
    }

    filter "system:linux"
        defines
        {
            "ENGINE_PLATFORM_LINUX"
        }

    filter "configurations:Debug"
        defines
        {
            "ENGINE_DEBUG"
        }
        symbols "On"

    filter "configurations:Release"
        defines
        {
            "ENGINE_RELEASE"
        }
        optimize "On"

    filter "configurations:Dist"
        defines
        {
            "ENGINE_DIST"
        }
        optimize "Full"