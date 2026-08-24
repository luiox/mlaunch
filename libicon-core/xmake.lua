set_project("libicon-core")
set_version("0.1.0")
set_xmakever("2.7.9")

add_rules("mode.debug", "mode.release")

task("libicon_core_codegen")
    set_menu {
        usage = "xmake libicon_core_codegen",
        description = "Manually regenerate libicon-core generated/icons.h and generated/icons.cpp"
    }
    on_run(function ()
        local script = path.join(os.scriptdir(), "scripts", "generate_cpp_assets.py")
        os.exec("python %s", script)
    end)

target("libicon_core_dynamic")
    set_kind("static")
    set_languages("cxx17")
    set_default(false)

    add_includedirs("generated", {public = true})
    add_headerfiles("generated/icons.h")
    add_files("generated/icons.cpp")
    add_defines("LIBICON_CORE_ENABLE_EMBED=0", {public = true})

target("libicon_core_embed")
    set_kind("static")
    set_languages("cxx17")
    set_default(false)

    add_includedirs("generated", {public = true})
    add_headerfiles("generated/icons.h")
    add_files("generated/icons.cpp")
    add_defines("LIBICON_CORE_ENABLE_EMBED=1", {public = true})
