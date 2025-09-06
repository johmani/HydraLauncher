-------------------------------------------------------------------------------------
-- Args
-------------------------------------------------------------------------------------
newoption {
    trigger = "enginePath",
    description = "enginePath",
}

newoption {
    trigger = "includSourceCode",
    description = "includSourceCode",
}

HE = _OPTIONS["enginePath"]
includSourceCode = _OPTIONS["includSourceCode"]  == "true"

print("Engine : " .. HE)
include (HE .. "/build.lua")

-------------------------------------------------------------------------------------
-- Setup
-------------------------------------------------------------------------------------
CloneLibs(
    {
        windows = "https://github.com/johmani/HydraLauncherLibs_Windows_x64",
    },
    "ThirdParty"
)

-------------------------------------------------------------------------------------
-- global variables
-------------------------------------------------------------------------------------
projectLocation = "%{wks.location}/Build/IDE"


-------------------------------------------------------------------------------------
-- workspace
-------------------------------------------------------------------------------------
workspace "HydraLauncher"
    architecture "x86_64"
    configurations { "Debug", "Release", "Profile", "Dist" }
    flags {  "MultiProcessorCompile" }
    startproject "HydraLauncher"

    IncludeHydraProject(includSourceCode)
    AddPlugins()

    group "HydraLauncher"
        project "HydraLauncher"
            kind "ConsoleApp"
            language "C++"
            cppdialect "C++latest"
            staticruntime "off"
            targetdir (binOutputDir)
            objdir (IntermediatesOutputDir)

            LinkHydraApp(includSourceCode)
            SetHydraFilters()

            files {
            
                "Source/**.h",
                "Source/**.cpp",
                "Source/**.cppm",
                "*.lua",

                "Resources/Icons/HydraLauncher.aps",
                "Resources/Icons/HydraLauncher.rc",
                "Resources/Icons/resource.h",
            }
            
            includedirs {
            
                "Source",
                "ThirdParty/libgit2/include",
            }
            
            links {
            
                "ImGui",
                "ThirdParty/libgit2/lib/git2",
            }

            buildoptions {
            
                AddCppm("imgui"),
            }

            filter "system:windows"
                links {
                    -- for libgit2
                    "winhttp",
                    "Crypt32",
                    "Rpcrt4"
                }
            filter {}
    group ""
