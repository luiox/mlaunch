set_project("mlaunch")
set_version("0.1.0")
set_xmakever("2.8.3")

add_rules("mode.debug", "mode.release")
add_rules("plugin.compile_commands.autoupdate", {outputdir = "."})
add_requires("gtest")

-- libca: 中心包仓（luiox/luiox-repo）git 直连包，仅 C++ core 形态消费。
-- 0.0.4 = 原 submodule 指针 d6676b8da。
add_repositories("luiox-repo https://github.com/luiox/luiox-repo.git")
add_requires("libca 0.0.4")

includes("third_party/micon")

-- directory where the DuiLib source submodule is checked out
local duilib_dir = "third_party/DuiLib_DuiEditor/DuiLib"

target("DuiLibLite")
    set_kind("static")
    set_languages("cxx17")

    if is_mode("debug") then
        set_symbols("debug")
        set_optimize("none")
    else
        set_optimize("faster")
    end

    add_defines("WIN32", "_WIN32", "WINDOWS", "_WIN64", "UNICODE", "_UNICODE", "UILIB_EXPORTS", "UILIB_STATIC")
    -- fork 03c53b2 起 pugixml 转独立编译单元（3rd/README.md「迁移历史」/「上游合并策略」）：
    -- glob DuiLib/**.cpp 不含 pugixml.cpp，须显式加入编译。PUGIXML_WCHAR_MODE 仅注入
    -- 该 TU（不含 StdAfx.h）；DuiLib 各 TU 由 StdAfx.h 的 _UNICODE 条件宏提供同口径，
    -- 不在 target 级重复定义以免 C4005 宏重定义警告。
    add_files("third_party/DuiLib_DuiEditor/3rd/pugixml/pugixml.cpp", {defines = "PUGIXML_WCHAR_MODE"})
    add_includedirs(duilib_dir, {public = true})
    add_files(duilib_dir .. "/**.cpp")
    remove_files(
        duilib_dir .. "/Utils/unzip.cpp",
        duilib_dir .. "/Utils/UIDataExchange.cpp",
        -- pugixml 已随 PR#3 迁至 fork 3rd/（不在 DuiLib glob 内），无需排除。
        duilib_dir .. "/**/**Gtk.cpp",
        -- 新上游引入 SDL 后端（DUILIB_SDL 宏门控），其头文件在宏门外包含 SDL.h，
        -- Win32 构建必须整组排除（与 fork 根 xmake.lua 的排除清单一致）。
        duilib_dir .. "/**/*Sdl.cpp",
        duilib_dir .. "/**/*SDL.cpp",
        duilib_dir .. "/Render/UIObject_Cairo.cpp",
        duilib_dir .. "/Render/UIRender_Cairo.cpp",
        duilib_dir .. "/Render/UIRender_CairoWin32.cpp",
        duilib_dir .. "/Render/UIRenderFactory_Cairo.cpp"
    )

-- 纯 CRUD 核心：数据模型、JSON 持久化、备份轮转、journal、软删除/撤销。
-- 不依赖 DuiLib / shell32 / ole32，可被 core_tests 独立链接测试。
target("mlaunch-core")
    set_kind("static")
    set_languages("cxx17")
    set_warnings("all")
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
    add_packages("libca", {public = true})
    -- MD5 走 CryptoAPI
    add_syslinks("advapi32")

-- 小工具插件层：纯函数，mtool.exe CLI 与 mlaunch 搜索框共用同一注册表。
target("mlaunch-tools")
    set_kind("static")
    set_languages("cxx17")
    set_warnings("all")
    add_cxxflags("/utf-8")

    if is_mode("debug") then
        set_symbols("debug")
        set_optimize("none")
    else
        set_optimize("faster")
    end

    add_defines("UNICODE", "_UNICODE", "WIN32", "_WINDOWS")
    add_includedirs("src/tools", {public = true})
    add_files("src/tools/*.cpp")
    add_packages("libca", {public = true})
    -- font 工具的 GDI 渲染需要 gdi32/user32；bcrypt 给 base64/uuid 随机源
    -- （包定义已带 libca_crypto 的 syslinks，此处重复声明无害）。
    add_syslinks("gdi32", "user32", "bcrypt")

-- DuiLib UI 层：窗口、控制器、渲染、shell 服务实现。
target("mlaunch")
    set_kind("binary")
    set_languages("cxx17")
    set_warnings("all")
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
    add_packages("nlohmann_json")
    add_deps("mlaunch-core")
    add_deps("mlaunch-tools")
    add_deps("DuiLibLite")

    add_syslinks("user32", "gdi32", "comctl32", "comdlg32", "ole32", "oleaut32", "imm32", "winmm", "version", "uxtheme", "shell32", "advapi32", "dwmapi", "bcrypt")

    after_build(function (target)
        if is_mode("debug") then
            os.cp(path.join(os.scriptdir(), "third_party", "micon", "icons"), path.join(target:targetdir(), "icons"))
        end
    end)

-- 小工具 CLI：mtool <关键字> [参数]，给 AI / 脚本 / 管道用。
target("mtool")
    set_kind("binary")
    set_languages("cxx17")
    set_warnings("all")
    add_cxxflags("/utf-8")

    if is_mode("debug") then
        set_symbols("debug")
        set_optimize("none")
    else
        set_optimize("faster")
    end

    add_defines("UNICODE", "_UNICODE", "WIN32", "_WINDOWS")
    add_files("tools/mtool_main.cpp")
    add_deps("mlaunch-tools")
    add_syslinks("shell32", "gdi32", "user32", "bcrypt")

-- 纯核心测试：不链接 DuiLib / shell32 / ole32，注入 fake 执行器。
target("core_tests")
    set_kind("binary")
    set_languages("cxx17")
    set_warnings("all")
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

-- 小工具层测试：golden 值 + 往返一致性，不链接 DuiLib / shell32。
target("tools_tests")
    set_kind("binary")
    set_languages("cxx17")
    set_warnings("all")
    add_cxxflags("/utf-8")

    if is_mode("debug") then
        set_symbols("debug")
        set_optimize("none")
    else
        set_optimize("faster")
    end

    add_defines("UNICODE", "_UNICODE", "WIN32", "_WINDOWS")
    add_files("tests/tools_tests.cpp")
    add_packages("gtest")
    add_deps("mlaunch-tools")
    -- font 工具测试走 GDI 渲染
    add_syslinks("gdi32", "user32", "bcrypt")
