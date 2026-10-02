set_project("mlaunch")
set_version("0.1.0")
set_xmakever("2.8.3")

add_rules("mode.debug", "mode.release")
add_rules("plugin.compile_commands.autoupdate", {outputdir = "."})
add_repositories("luiox-repo https://github.com/luiox/luiox-repo.git")
add_requires("gtest", "duilib 0.1.0", "uikit 0.2.0")

-- libca options: only enable C++ core, disable embedded MCU and demo
set_config("with_core", true)
set_config("with_em", false)
set_config("with_demo", false)
set_config("with_tests", false)

includes("third_party/libca")
includes("third_party/micon")

-- duilib / uikit 均改由 luiox-repo 包供给（duilib 0.1.0 = fork 静态库包，
-- uikit 0.2.0 = 主题引擎 + 控件层），vendored submodule 已随之移除。

-- 纯 CRUD 核心：数据模型、JSON 持久化、备份轮转、journal、软删除/撤销。
-- 不依赖 DuiLib / shell32 / ole32，可被 core_tests 独立链接测试。
target("mlaunch-core")
    set_kind("static")
    set_languages("cxx17")
    add_cxxflags("/utf-8")

    if is_mode("debug") then
        set_symbols("debug")
        set_optimize("none")
    else
        set_optimize("faster")
    end

    add_defines("UNICODE", "_UNICODE", "WIN32", "_WINDOWS")
    add_includedirs("src/core", {public = true})
    add_files("src/core/*.cpp", "src/core/utils/*.cpp")
    add_packages("nlohmann_json")
    add_deps("libca_json")
    -- MD5 走 CryptoAPI
    add_syslinks("advapi32")

-- DuiLib UI 层：窗口、控制器、渲染、shell 服务实现。
target("mlaunch")
    set_kind("binary")
    set_languages("cxx17")
    add_cxxflags("/utf-8")

    if is_mode("debug") then
        set_symbols("debug")
        set_optimize("none")
        add_deps("micon_dynamic")
        -- 仅 Debug 构建启用控制台输出（main.cpp 据此决定是否 AllocConsole）。
        add_defines("MLAUNCH_DEV_CONSOLE")
    else
        set_optimize("faster")
        add_deps("micon_embed")
    end

    add_defines("UNICODE", "_UNICODE", "WIN32", "_WINDOWS", "UILIB_STATIC")
    add_includedirs("src/ui", {public = true})

    add_files("src/ui/*.cpp")
    add_headerfiles("src/ui/*.h")
    add_packages("nlohmann_json", "duilib", "uikit")
    add_deps("mlaunch-core")

    add_syslinks("user32", "gdi32", "comctl32", "comdlg32", "ole32", "oleaut32", "imm32", "winmm", "version", "uxtheme", "shell32", "advapi32", "dwmapi")

    after_build(function (target)
        if is_mode("debug") then
            os.cp(path.join(os.scriptdir(), "third_party", "micon", "icons"), path.join(target:targetdir(), "icons"))
        end
    end)

-- 纯核心测试：不链接 DuiLib / shell32 / ole32，注入 fake 执行器。
target("core_tests")
    set_kind("binary")
    set_languages("cxx17")
    add_cxxflags("/utf-8")

    if is_mode("debug") then
        set_symbols("debug")
        set_optimize("none")
    else
        set_optimize("faster")
    end

    add_defines("UNICODE", "_UNICODE", "WIN32", "_WINDOWS")
    add_files("tests/core_tests.cpp")
    add_packages("gtest")
    add_deps("mlaunch-core")
    add_syslinks("advapi32")
