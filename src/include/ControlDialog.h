#pragma once

#include <windows.h>

#include "ConfigManager.h"
#include "TimeController.h"

namespace ctg {
namespace ui {

    // User choice when clicking OK on the control dialog.
    enum class ControlDialogAction
    {
        Cancel = 0,
        OpenConfig,
        TempDisable,
        ExitApp
    };

    struct ControlDialogResult
    {
        ControlDialogAction action{ ControlDialogAction::Cancel };
        int tempDisableMinutes{ 0 };  // Valid when action == TempDisable
    };

    // Ctrl+Shift+K 弹出的控制对话框实现（使用资源对话框）。
    class ControlDialog
    {
    public:
        // 以模态形式显示控制对话框。
        // configManager: 用于校验家长密码；可为 nullptr（则不校验）。
        // timeController: 用于获取临时停用剩余分钟数、执行临时停用；可为 nullptr。
        // outResult: 用户点击确定时写入选项与临时停用分钟数；取消时 outResult->action = Cancel。
        // 返回 true 表示用户点击了“确定”且密码通过，false 表示取消或密码错误。
        static bool ShowModal(
            HINSTANCE hInstance,
            HWND parent,
            ctg::config::ConfigManager* configManager,
            ctg::core::TimeController* timeController,
            ControlDialogResult* outResult);
    };

} // namespace ui
} // namespace ctg
