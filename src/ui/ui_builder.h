#pragma once

#include <UIlib.h>

class AppWindow;

class UiBuilder {
public:
    explicit UiBuilder(AppWindow& owner);

    DuiLib::CControlUI* BuildRootUi() const;

private:
    AppWindow& owner_;
};
