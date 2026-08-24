# 数据格式说明

## 1. launcher.v2.json

位置：`%LOCALAPPDATA%/nassistant/launcher.v2.json`

```json
{
  "version": 2,
  "groups": [
    {
      "id": "group_xxx",
      "name": "Common",
      "order": 0,
      "items": [
        {
          "id": "item_xxx",
          "itemType": "app",
          "name": "Notepad",
          "targetPath": "C:\\Windows\\notepad.exe",
          "iconLocation": "C:\\Windows\\notepad.exe,0",
          "arguments": "",
          "launchCount": 0,
          "enabled": true
        }
      ]
    }
  ]
}
```

字段说明：

- `version`：数据版本，当前支持 `2`。
- `groups[].order`：分组显示顺序。
- `items[].itemType`：`app` 或 `separator`。
- `items[].launchCount`：启动次数统计。

### 兼容与恢复策略

- 若版本不为 `2`，视为不兼容：
  - 原文件备份为 `launcher.v2.json.bad.<timestamp>.bak`
  - 生成默认数据并落盘。
- 若 JSON 损坏/解析失败：
  - 同样执行备份并回退默认数据。

## 2. nassistant.settings.json

位置：`%LOCALAPPDATA%/nassistant/nassistant.settings.json`

```json
{
  "hotkey": "Alt+1",
  "executeHide": true,
  "currentGroup": "group_xxx",
  "groupPanelWidth": 220.0,
  "mainWindowWidth": 1040.0,
  "mainWindowHeight": 700.0
}
```

字段说明：

- `hotkey`：快捷键字符串（保留字段）。
- `executeHide`：执行后是否隐藏主窗口。
- `currentGroup`：当前分组 ID（可为空）。
- `groupPanelWidth`：左侧分栏宽度。
- `mainWindowWidth/mainWindowHeight`：主窗口尺寸。

## 3. ui_state.ini

位置：`%LOCALAPPDATA%/nassistant/ui_state.ini`

- `layout.splitter_width`：分栏宽度。
- `window.left/top/right/bottom`：窗口矩形。
- `window.width/height`：窗口尺寸。
- `window.maximized`：是否最大化。

写入策略：

- 通过防抖定时器延迟写入。
- 先写临时文件后原子替换，避免中断造成半写入文件。