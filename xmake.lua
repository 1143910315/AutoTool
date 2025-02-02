add_rules("mode.debug", "mode.release", "mode.releasedbg")
add_rules("plugin.vsxmake.autoupdate")

if is_mode("debug") then
    set_runtimes("MDd")
    add_requires("pcre2", {debug = true})
    add_requires("raw_pdb main", {debug = true})
else
    set_runtimes("MD")
    add_requires("pcre2")
    add_requires("raw_pdb main")
end

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
        set_symbols("debug", "edit")
    end
    set_kind("binary")
    add_includedirs("src")
    add_headerfiles("src**/*.h")
    add_headerfiles("src**/*.hpp")
    add_files("src**/*.cpp")
    set_languages("clatest", "cxxlatest")
