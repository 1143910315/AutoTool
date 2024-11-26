add_rules("mode.debug", "mode.release", "mode.releasedbg")
add_rules("plugin.vsxmake.autoupdate")

if is_mode("debug") then
    set_runtimes("MDd")
else
    set_runtimes("MD")
end
add_requires("pcre2")
add_requires("raw_pdb main")

target("AutoTool")
    add_cxflags(
        "cl::/utf-8",
        "cl::/W4",
        "gcc::-Wall",
        "cl::/INCREMENTAL"
    )
    add_packages(
        "pcre2",
        "raw_pdb"
    )
    if is_mode("debug") then
        add_defines("_DEBUG")
    end
    set_kind("binary")
    add_includedirs("src")
    add_headerfiles("src**/*.h")
    add_headerfiles("src**/*.hpp")
    add_files("src**/*.cpp")
    set_languages("clatest", "cxxlatest")
    set_symbols("debug", "debug", "edit")