# DuiLib + nanosvg 兼容规范

这份规范用于保证 SVG 在 DuiLib 的 nanosvg 加载链路下稳定可用。

## 允许使用（安全）
- `<svg>`、`<g>`、`<path>`、`<rect>`、`<circle>`、`<ellipse>`、`<line>`、`<polyline>`、`<polygon>`
- 简单 `fill` / `stroke`
- 明确定义 `viewBox`
- 固定单色或简单双色 icon

## 尽量避免（nanosvg 高风险）
- 嵌入位图资源（`<image>`）
- 复杂滤镜（`<filter>`、blur、drop-shadow）
- 高级合成的 mask/clip（`<mask>`、复杂 `<clipPath>`）
- 外部 CSS、script、以及不受支持的 animation
- stop 很多、结构复杂的 gradient

## 本项目样式约束
- 推荐尺寸：`24x24`（或保持一致 aspect ratio）
- 优先使用 path-based icon
- `stroke-width` 视觉保持一致（通常 1.5~2）
- 最终文件保持精简（建议做 SVGO 类优化）

## DuiLib 加载约定
- Debug：`file='assets/ui/xxx.svg'`
- Release（内嵌）：`file='RESOURCE_ID' restype='10'`（`RCDATA`）

## 导入前检查清单
- 浏览器可正常打开
- 不含重度 filter/mask 特性
- 在应用中 Debug 和 Release 都能正常显示
