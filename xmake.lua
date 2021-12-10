add_rules("mode.debug", "mode.release")

if is_mode("release") then
    add_defines("NDEBUG")
elseif is_mode("debug") then
    add_undefines("NDEBUG")
end

set_toolchains("msvc")
set_languages("c++17", "c17")

add_defines("WIN32")
add_defines("UNICODE")
add_defines("_UNICODE")

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


target("D3DApp")
    set_kind("static")

    add_includedirs("D3DApp/", {public = true})
    add_files("D3DApp/*.cpp")


target("D3DApp_imgui")
    set_kind("binary")
    add_deps("D3DApp")

    add_packages("vcpkg::directxtk12", "vcpkg::imgui", "vcpkg::imgui[dx12-binding]", "vcpkg::imgui[win32-binding]")

    add_includedirs("D3DApp_imgui/")
    add_files("D3DApp_imgui/*.cpp")

    add_ldflags("/SUBSYSTEM:WINDOWS")
    add_syslinks("User32", "Gdi32", "dxguid")


target("DXTK")
    set_kind("binary")

    add_packages("vcpkg::directxtk12")

    add_includedirs("DXTK/")
    add_files("DXTK/*.cpp")
