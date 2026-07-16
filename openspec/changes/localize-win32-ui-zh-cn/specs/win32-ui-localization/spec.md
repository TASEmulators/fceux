## ADDED Requirements

### Requirement: Win32 UI language selection
Win32 前端 SHALL 提供 `Auto`、`English` 和 `简体中文` 三种界面语言选择，SHALL 将显式选择持久化到现有配置，并 SHALL 在创建首个用户界面资源前解析本次进程使用的语言。

#### Scenario: Auto selects Simplified Chinese
- **WHEN** 语言配置为 `Auto` 或尚未存在，且 Windows 显示语言为简体中文区域变体
- **THEN** Win32 前端使用 `zh-CN` 资源创建菜单、对话框和动态提示

#### Scenario: Auto selects English for other display languages
- **WHEN** 语言配置为 `Auto` 或尚未存在，且 Windows 显示语言不是简体中文
- **THEN** Win32 前端使用 `en-US` 资源

#### Scenario: Explicit language overrides system language
- **WHEN** 用户选择 `English` 或 `简体中文` 并重新启动 Win32 前端
- **THEN** 前端使用该显式语言而不受 Windows 显示语言影响，且菜单正确标记当前选择

#### Scenario: Invalid configuration is safe
- **WHEN** 持久化的语言值为空、未知或损坏
- **THEN** 前端按 `Auto` 规则解析语言并保持可启动

### Requirement: Complete auditable Win32 text coverage
系统 MUST 盘点 Win32 前端所有用户可见的静态和动态文案，并 MUST 为每项记录 `翻译`、`保留` 或 `非本地化范围` 的处理结论。覆盖范围 SHALL 包括主菜单、对话框、工具窗口、TAS Editor、调试工具、消息框、状态提示、动态菜单以及文件对话框的自定义标题和筛选器。

#### Scenario: Simplified Chinese UI is selected
- **WHEN** 用户以 `zh-CN` 启动并打开清单中的任一 Win32 窗口或提示
- **THEN** 所有标为 `翻译` 的文案以简体中文显示，标为 `保留` 的文案与清单一致，且界面中不存在未经审计的英文 UI 文案

#### Scenario: User data appears inside localized text
- **WHEN** 译文包含文件名、ROM 名称、路径、脚本输出、地址、寄存器值或其他用户/运行时数据
- **THEN** 系统仅翻译固定 UI 文案并原样保留所插入的数据

### Requirement: Terminology consistency
简体中文译文 MUST 遵循版本控制中的术语表：产品名、格式名及行业通行缩写 MUST 保留；大众已有稳定理解的操作词 SHALL 使用简明中文；存在显著歧义的 TAS 专有概念 MUST 使用术语表规定的英文保留或中英并列形式。

#### Scenario: Common emulator operation is translated
- **WHEN** 文案使用 `Frame`、`Save State`、`Load State` 或 TAS 语境下的 `Movie`
- **THEN** 译文分别使用术语表规定的“帧”“即时存档”“读取即时存档”和“输入录像”及其已登记短译

#### Scenario: Established identifier is preserved
- **WHEN** 文案包含 FCEUX、NES、TAS、Lua、ROM、RAM、CPU、PPU、APU、FDS、NTSC、PAL、FPS、FM2 或 FM3 等标识
- **THEN** 标识保持原拼写，不创建可能误导用户的中文展开词

#### Scenario: Ambiguous TAS term is presented
- **WHEN** 空间允许的位置首次显示 Greenzone、Lag Frame 等术语表认定的易歧义概念
- **THEN** 系统显示统一的“中文（英文）”形式，并在紧凑位置使用同一术语登记的短译

### Requirement: Deterministic English fallback
英文资源 SHALL 始终作为完整基准随 Win32 可执行文件提供。所选语言的任一静态资源或动态字符串无法加载时，系统 MUST 对该项回退到对应英文资源，不得因翻译缺失阻止窗口创建或命令执行。

#### Scenario: Chinese translation is missing
- **WHEN** `zh-CN` 中缺少一个已请求的菜单、对话框或动态字符串资源
- **THEN** 系统显示同一资源 ID 的 `en-US` 内容，其关联命令仍可正常使用

#### Scenario: English resource is unexpectedly missing
- **WHEN** 所选语言和 `en-US` 均没有请求的动态字符串 ID
- **THEN** 系统显示包含该资源 ID 的可诊断占位文本并继续运行

### Requirement: Unicode-safe localized presentation
所有简体中文资源和动态译文 MUST 通过 Unicode 资源或显式宽字符 Win32 API 显示，且 MUST 不依赖当前 Windows ANSI code page。引入本地化不得改变内部模拟数据、文件路径、用户文本或既有非 UI 接口的编码语义。

#### Scenario: Chinese UI runs under a non-Chinese ANSI code page
- **WHEN** 用户在 ANSI code page 不是简体中文的受支持 Windows 环境中显式选择 `简体中文`
- **THEN** 菜单、对话框、窗口标题和消息框中的译文正确显示而无乱码或替换字符

#### Scenario: Localized message includes a Unicode path
- **WHEN** 动态中文提示插入包含非 ASCII 字符的文件路径
- **THEN** 提示中的路径字符保持完整，且底层文件操作接收的数据不因本地化发生变化

### Requirement: UI behavior remains equivalent across languages
本地化资源 MUST 保持英文界面的资源 ID、命令 ID、控件语义、格式参数、显示快捷键和模拟器行为。中文布局 SHALL 在受支持的 100% 与高 DPI 设置下保持关键文本可读，且菜单或同一对话框范围内的助记键 SHALL 可用并避免明显冲突。

#### Scenario: A localized command is invoked
- **WHEN** 用户从中文菜单或对话框触发任一已有命令
- **THEN** 系统执行与英文资源中同一命令 ID 完全相同的操作

#### Scenario: A formatted localized string is rendered
- **WHEN** 系统使用数值、字符串或路径参数格式化中文动态文案
- **THEN** 中文资源使用与英文资源相同数量、顺序和类型的格式参数并正确显示所有值

#### Scenario: Key windows are viewed at supported DPI
- **WHEN** 用户在 100% 或项目支持的高 DPI 设置下打开主窗口、常用配置、调试器、内存工具、Movie 工具或 TAS Editor
- **THEN** 关键标签、输入内容和操作按钮不重叠、不被裁掉且保持可操作

### Requirement: Localization validation
项目 MUST 提供可重复执行的本地化校验，至少检查语言资源 ID/类型对齐、清单覆盖状态、动态字符串 ID、格式占位符及快捷键后缀一致性，并 SHALL 在 Win32 构建验证中执行该检查。

#### Scenario: A translated resource drifts from English
- **WHEN** 中文资源缺少必需 ID、使用了错误资源类型、格式参数不匹配或修改了显示快捷键
- **THEN** 校验以非零状态失败并指出对应资源或字符串 ID

#### Scenario: A deliberately preserved term is audited
- **WHEN** 中文资源中的可见文本与英文相同且清单将其标记为 `保留`
- **THEN** 校验接受该文本且不会把它误报为漏译
