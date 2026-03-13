#pragma once

#include <windows.h>

#include "AppConfig.h"

namespace ctg {
namespace ui {

    // 应用配置界面的占位实现。
    class ConfigDialog
    {
    public:
        // 以模态形式显示配置对话框。
        // inOutConfig 用于传入当前配置并在用户点击“确定”后返回修改结果。
        // firstRun 为 true 时表示首次运行：取消按钮禁用，标题栏关闭（X）移除。
        // 返回 true 表示用户点击确定并且 inOutConfig 已更新，false 表示取消。
        static bool ShowModal(HINSTANCE hInstance,
                              HWND parent,
                              config::AppConfig& inOutConfig,
                              bool firstRun = false);
    };

} // namespace ui
} // namespace ctg

