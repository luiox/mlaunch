set_project("mlaunch")
set_version("0.1.0")
set_xmakever("2.8.3")

add_rules("mode.debug", "mode.release")
add_rules("plugin.compile_commands.autoupdate", {outputdir = "."})
add_requires("gtest")

-- libca / micon / duilib: 中心包仓（luiox/luiox-repo）git 直连包，submodule 全部迁出。
-- libca 0.0.8（560d263f3）：仅 C++ core 形态消费；em 形态已于 0.0.7 拆出至 luiox/libca-em。
-- micon 0.2.0（f1a5a9da）= 原 submodule 指针，零 API 变化。
-- duilib 0.1.0（fork 03c53b2）= 原 submodule 指针；包定义指名构建 fork 根 xmake
-- 的 DuiLib target，静态形态 syslinks 由包补齐，消费方自带 UNICODE/UILIB_STATIC。
add_repositories("luiox-repo https://github.com/luiox/luiox-repo.git")
add_requires("libca 0.0.8")
add_requires("micon 0.2.0")
add_requires("duilib 0.1.0")

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
        -- Debug 用运行期读盘变体（icons/ 随 after_build 拷到 targetdir）
        add_packages("micon", {configs = {dynamic = true}})
        -- 仅 Debug 构建启用控制台输出（main.cpp 据此决定是否 AllocConsole）。
        add_defines("MLAUNCH_DEV_CONSOLE")
    else
        set_optimize("faster")
        -- Release 内嵌 SVG 资产（包默认即 embed 变体）
        add_packages("micon")
    end

    add_defines("UNICODE", "_UNICODE", "WIN32", "_WINDOWS", "UILIB_STATIC")
    add_includedirs("src/ui", {public = true})

    add_files("src/ui/*.cpp")
    add_headerfiles("src/ui/*.h")
    add_packages("nlohmann_json")
    add_deps("mlaunch-core")
    add_deps("mlaunch-tools")
    add_packages("duilib")

    add_syslinks("user32", "gdi32", "comctl32", "comdlg32", "ole32", "oleaut32", "imm32", "winmm", "version", "uxtheme", "shell32", "advapi32", "dwmapi", "bcrypt")

    after_build(function (target)
        if is_mode("debug") then
            -- dynamic 变体运行期从磁盘读 SVG；资源已随包安装，从包安装目录拷出
            local pkg = target:pkg("micon")
            assert(pkg, "micon package not attached")
            os.cp(path.join(pkg:installdir(), "icons"), path.join(target:targetdir(), "icons"))
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
