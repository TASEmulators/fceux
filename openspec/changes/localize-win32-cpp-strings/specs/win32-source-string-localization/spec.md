## ADDED Requirements

### Requirement: Complete Win32 source-string inventory
系统 MUST 扫描 `src/drivers/win` 中 C/C++ 源码产生的用户可见字符串，并 MUST 为每个候选记录 `translate`、`preserve` 或 `exclude` 结论及可复核的理由。覆盖范围 SHALL 包括消息框、窗口和控件标题、状态文本、工具提示、动态菜单，以及其他直接或间接传入 Win32 显示 API 的固定文案。文件对话框标题与筛选器 SHALL 作为本变更的明确例外记录，保持现有实现。

#### Scenario: A source literal reaches a Win32 UI sink
- **WHEN** 固定字符串字面量或由固定片段拼接的字符串被传给已登记的 Win32 UI 显示入口
- **THEN** 审计清单包含其源码位置、显示场景和处理结论，且不存在未登记的用户可见英文候选

#### Scenario: A string is not interface copy
- **WHEN** 候选仅表示配置键、Win32 类名、文件扩展名、内部标识、日志/命令行输出、反汇编或其他运行数据
- **THEN** 清单将其标为 `preserve` 或 `exclude` 并记录理由，系统不对其做界面翻译

### Requirement: Resource-backed dynamic UI text
所有标记为 `translate` 的固定源码文案 SHALL 使用稳定资源 ID 从当前 Win32 UI 语言的字符串资源加载，不得依赖调用点中的中文字符串字面量。英文资源 MUST 作为完整基准存在；简体中文条目缺失时 MUST 对单项回退到对应英文文本。

#### Scenario: Chinese dynamic text is requested
- **WHEN** Win32 UI 语言为 `zh-CN` 且代码显示一个标记为 `translate` 的源码动态文案
- **THEN** 系统按资源 ID 显示对应简体中文文本，不显示原调用点英文

#### Scenario: Chinese entry is missing
- **WHEN** `zh-CN` 字符串表缺少请求的动态文案资源 ID
- **THEN** 系统显示同一资源 ID 的 `en-US` 文本，窗口和命令继续正常工作

#### Scenario: Both language entries are missing
- **WHEN** 当前语言和 `en-US` 均无法加载请求的动态文案资源 ID
- **THEN** 系统显示含资源 ID 的可诊断占位文本且不崩溃

### Requirement: Unicode-safe Win32 presentation
迁移后的动态译文 MUST 通过显式宽字符 Win32 API 显示，且 MUST 不依赖当前 ANSI code page。实现 MUST 保持项目未迁移接口的现有字符类型，不得通过全局启用 `UNICODE` 扩大变更范围。

#### Scenario: Chinese UI runs under a non-Chinese ANSI code page
- **WHEN** 用户在非简体中文 ANSI code page 的 Windows 环境中选择 `zh-CN`
- **THEN** 源码动态窗口标题、提示和状态文本正确显示简体中文且无乱码；文件对话框保持既有显示和路径处理

#### Scenario: An untouched Win32 interface is compiled
- **WHEN** 项目使用本变更后的头文件和 Visual Studio 2022 `v143` 编译
- **THEN** 未迁移的窄字符接口继续使用原有类型和调用约定，不因全局字符集切换产生编译或行为变化

### Requirement: Runtime data and formatting preservation
本地化 SHALL 只翻译动态文案中的固定模板，并 MUST 原样保留插入的设备名、文件名、路径、ROM 名称、地址、寄存器、数值、快捷键信息及脚本/调试数据。英文和简体中文模板 MUST 使用相同数量、顺序和类型的格式参数。

#### Scenario: A localized prompt includes runtime values
- **WHEN** 消息框、标题或状态文本将路径、名称、地址或数值插入固定文案
- **THEN** 固定文案按当前语言显示，所有插入值与本地化前的值一致且完整

#### Scenario: A translated format template is validated
- **WHEN** 自动化检查比较一个英文和简体中文格式化资源
- **THEN** 两个模板的格式参数数量、顺序和类型完全一致，否则检查失败

### Requirement: Existing file-dialog behavior preserved
Win32 源码创建的文件对话框 SHALL 保持现有标题、筛选器、`OPENFILENAME` 结构和文件路径处理语义。本变更 SHALL NOT 将文件对话框标题或筛选器迁移到宽字符资源，也 SHALL NOT 改变筛选模式、扩展名、条目顺序、双 NUL 终止结构或文件 I/O 边界。

#### Scenario: Chinese UI opens a file dialog
- **WHEN** 用户在 `zh-CN` 下打开源码配置的 ROM、Movie、Lua、金手指、输入预设或其他 Win32 文件对话框
- **THEN** 文件对话框使用与本变更前相同的标题、筛选器和路径处理行为

#### Scenario: A Unicode path is selected
- **WHEN** 文件对话框返回包含非 ASCII 字符的文件路径
- **THEN** 本地化逻辑不介入或改变既有文件操作的路径内容或语义

### Requirement: Consistent emulator and TAS terminology
简体中文源码动态文案 MUST 执行保守、可审计的术语规则。产品、项目、人物或组织名称，官方工具/模块名称，文件格式、协议、扩展名、品牌词、缩写及独有 TAS/NES 术语 MUST 默认保留原拼写和大小写。只有普通界面词或在简体中文模拟器/TAS 用户中具有单一、稳定、大众通行译法的概念 SHALL 翻译；采用译法时 MUST 在术语表记录类别和依据。无法确认、存在多种竞争译法或直译可能改变技术含义的术语 MUST 保留英文，不得逐字硬译或创造新的中文名称或缩写。

#### Scenario: Dynamic text contains a proper or unique technical term
- **WHEN** 动态标题或提示包含 FCEUX、NES、TAS、ROM、RAM、CPU、PPU、APU、Lua、FM2/FM3，或其他被分类为产品名、工具/模块名、格式名、缩写及独有 TAS/NES 术语的 token
- **THEN** 简体中文文案完整保留该 token 的原拼写和大小写，不为其创建中文替代名

#### Scenario: A proper term appears inside otherwise translatable text
- **WHEN** 动态文案由普通界面动作和专有名词组成，例如 `Open ROM` 或包含格式名的错误提示
- **THEN** 系统可翻译普通动作和说明，但 MUST 原样保留其中每个专有名词 token

#### Scenario: A common UI concept has a stable Chinese equivalent
- **WHEN** 普通界面动作、状态或模拟器常用概念具有术语表所记录的单一、稳定且大众易懂的简体中文译法
- **THEN** 动态文案使用该已审查译法，并与其他 Win32 文案保持一致

#### Scenario: A domain term is ambiguous or disputed
- **WHEN** 某个 TAS/NES 领域词无法证明具有单一主流中文译法、存在多种竞争译法或直译可能误导用户
- **THEN** 动态文案保留英文，审计清单将其标为 `preserve` 并记录理由

### Requirement: Source-string regression gate
项目 MUST 提供可重复、无新增第三方运行时依赖的检查，验证所有已识别源码 UI 候选均有审计结论、双语资源 ID 成对存在、格式参数兼容，并在新增未登记候选时失败。

#### Scenario: New untranslated UI literal is introduced
- **WHEN** `src/drivers/win` 中新增一个匹配已知 UI sink 且未进入审计清单的固定英文文案
- **THEN** 本地化检查返回失败并报告源码位置

#### Scenario: A registered exception is encountered
- **WHEN** 扫描命中一个带明确理由且源码位置仍匹配的 `preserve` 或 `exclude` 项
- **THEN** 检查接受该项并继续验证其他候选

#### Scenario: A preserved terminology token is altered
- **WHEN** 简体中文资源删除、改写或改变审计清单中 `preserve` 术语的拼写或大小写
- **THEN** 本地化检查返回失败并报告资源 ID 与被改变的术语

### Requirement: Visual Studio 2022 v143 Win32 compatibility
完成源码动态文案迁移后，项目 MUST 使用 Visual Studio 2022 MSBuild、`PlatformToolset=v143`、`Configuration=Release` 和 `Platform=Win32` 成功生成 x86 Windows GUI 应用，并 MUST 保持英语和简体中文动态资源可加载。

#### Scenario: Release Win32 is built with v143
- **WHEN** 执行项目规定的 `v143` Release Win32 构建入口
- **THEN** 构建成功生成 `output/fceux.exe`，且不存在本变更引入的编译或资源错误

#### Scenario: Localized executable is smoke tested
- **WHEN** 分别以 `en-US` 和 `zh-CN` 打开主窗口、配置、调试/内存、Movie/Lua 和 TAS Editor 的代表性动态界面
- **THEN** 固定文案使用所选语言，运行参数完整，命令行为等价，且没有未经登记的可见英文源码文案

### Requirement: Win32-only scope
本变更 SHALL 仅改变 Win32 前端的用户可见源码文案和支撑其验证的资源、清单与脚本，不得改变其他前端、模拟核心、文件格式、配置兼容性或命令行输出。

#### Scenario: Non-Win32 behavior is compared
- **WHEN** 本变更完成后检查 Qt/SDL 前端、核心模拟路径和持久化文件格式
- **THEN** 这些区域没有因源码文案本地化产生功能或格式变化
