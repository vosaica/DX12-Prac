add_rules("mode.debug", "mode.release")

if is_mode("release") then
    add_defines("NDEBUG")
elseif is_mode("debug") then
    add_undefines("NDEBUG")
end

set_toolchains("clang")
set_languages("c++17", "c17")

add_defines("WIN32", "UNICODE", "_UNICODE", "_XM_NO_XMVECTOR_OVERLOADS_")
add_cxxflags("-march=x86-64-v3")

add_vectorexts("avx2")
set_fpmodels("fast")
set_warnings("more")

add_requires("vcpkg::directx-headers",
             "vcpkg::directxtk12",
             "vcpkg::imgui",
             "vcpkg::imgui[dx12-binding]",
             "vcpkg::imgui[win32-binding]")
add_includedirs("c:/src/vcpkg/installed/x64-windows-static/include")


target("Chapter_1")
    set_kind("binary")

    add_files("Chapter_1/*.cpp")


target("Chapter_2")
    set_kind("binary")

    add_files("Chapter_2/*.cpp")


target("Chapter_3")
    set_kind("binary")

    add_files("Chapter_3/*.cpp")


target("Chapter_4")
    set_kind("binary")
    add_deps("D3DApp")

    add_includedirs("Chapter_4/")
    add_files("Chapter_4/*.cpp")

    add_ldflags("/SUBSYSTEM:WINDOWS")
    add_syslinks("User32", "Gdi32")


target("Chapter_6")
    set_kind("binary")
    add_deps("D3DApp")

    add_packages("vcpkg::directxtk12")

    add_includedirs("Chapter_6/")
    add_files("Chapter_6/*.cpp")

    add_ldflags("/SUBSYSTEM:WINDOWS")
    add_syslinks("User32", "Gdi32", "dxguid")

    after_build(function (target)
        import("core.project.task")
        task.run("CopyAssets_Chapter_6")
    end)


target("Chapter_7")
    set_kind("binary")
    add_deps("D3DApp")

    add_packages("vcpkg::directxtk12")

    add_includedirs("Chapter_7/")
    add_files("Chapter_7/*.cpp", "Shared/GeometryGenerator.cpp", "Shared/PlatformHelpers.cpp")

    add_ldflags("/SUBSYSTEM:WINDOWS")
    add_syslinks("User32", "Gdi32", "dxguid")

    after_build(function (target)
        import("core.project.task")
        task.run("CopyAssets_Chapter_7")
    end)


target("D3DApp")
    set_kind("static")

    add_includedirs("D3DApp/", {public = true})
    add_files("D3DApp/*.cpp", "Shared/PlatformHelpers.cpp", "Shared/Timer.cpp")


target("D3DApp_imgui")
    set_kind("binary")
    add_deps("D3DApp")

    add_packages("vcpkg::directxtk12",
                 "vcpkg::imgui",
                 "vcpkg::imgui[dx12-binding]",
                 "vcpkg::imgui[win32-binding]")

    add_includedirs("D3DApp_imgui/")
    add_files("D3DApp_imgui/*.cpp")

    add_ldflags("/SUBSYSTEM:WINDOWS")
    add_syslinks("User32", "Gdi32", "dxguid")

    after_build(function (target)
        import("core.project.task")
        task.run("CopyAssets_D3DApp_imgui")
    end)


target("DXTK")
    set_kind("binary")

    add_packages("vcpkg::directxtk12")

    add_includedirs("DXTK/")
    add_files("DXTK/*.cpp")


target("Ch7_Test")
    set_kind("binary")
    add_deps("D3DApp")

    add_packages("vcpkg::directxtk12",
    "vcpkg::imgui",
    "vcpkg::imgui[dx12-binding]",
    "vcpkg::imgui[win32-binding]")

    add_includedirs("Ch7_Test/")
    add_files("Ch7_Test/*.cpp")

    add_ldflags("/SUBSYSTEM:WINDOWS")
    add_syslinks("User32", "Gdi32", "dxguid")


task("CopyAssets_Chapter_6")
    on_run(function()
        if is_mode("debug") then
            os.cp("$(projectdir)/Chapter_6/Shaders/Chapter_6.hlsl", "$(buildir)/windows/x64/debug/Shaders/")
        elseif is_mode("release") then
            os.cp("$(projectdir)/Chapter_6/Shaders/Chapter_6.hlsl", "$(buildir)/windows/x64/release/Shaders/")
        end
        print("Chapter_6.hlsl copied")
    end)


task("CopyAssets_Chapter_7")
    on_run(function()
        if is_mode("debug") then
            os.cp("$(projectdir)/Chapter_7/Shaders/Chapter_7.hlsl", "$(buildir)/windows/x64/debug/Shaders/")
        elseif is_mode("release") then
            os.cp("$(projectdir)/Chapter_7/Shaders/Chapter_7.hlsl", "$(buildir)/windows/x64/release/Shaders/")
        end
        print("Chapter_7.hlsl copied")
    end)


task("CopyAssets_D3DApp_imgui")
    on_run(function()
        if is_mode("debug") then
            os.cp("$(projectdir)/D3DApp_imgui/Shaders/D3DApp_imgui.hlsl", "$(buildir)/windows/x64/debug/Shaders/")
        elseif is_mode("release") then
            os.cp("$(projectdir)/D3DApp_imgui/Shaders/D3DApp_imgui.hlsl", "$(buildir)/windows/x64/release/Shaders/")
        end
        print("D3DApp_imgui.hlsl copied")
    end)
