add_rules("mode.debug", "mode.release")

if is_mode("release") then
    add_defines("NDEBUG")
elseif is_mode("debug") then
    add_undefines("NDEBUG")
end

set_toolchains("msvc")
set_languages("c++17", "c17")

add_requires("vcpkg::directx-headers")
add_includedirs("c:/src/vcpkg/installed/x64-windows-static/include")

target("D3DApp")
    set_kind("static")

    add_defines("UNICODE")
    add_defines("_UNICODE")
    add_defines("WIN32")

    add_includedirs("./")
    add_files("./*.cpp")

    add_vectorexts("avx2")
    set_fpmodels("fast")
    set_warnings("more")
