# Application Theming (QSS)

All global Qt widget styling lives in:

**`MainSystem/style/operations-console.qss`**

It is loaded at startup in `CApplicationController::setUpApplication()` from the resource `:/style/operations-console.qss` (see `ibtradesystem.qrc`).

## How to change the look

1. Open `operations-console.qss`.
2. Edit the **THEME PALETTE** comment block at the top — it lists the semantic colors (backgrounds, text, borders, accent).
3. Update matching string literals in **`MainSystem/ThemePalette.h`** (`namespace UiTheme`) so C++ runtime styles stay in sync.
4. Search/replace hex values consistently, or adjust individual selectors (`QLineEdit`, `QLabel`, `QGroupBox`, etc.).

## Text hierarchy

| Role | Typical selectors | Default color |
|------|-------------------|---------------|
| Primary body | `QLabel` | `#e8e8e8` |
| Form field captions | `QFormLayout QLabel` | `#f0f0f0` |
| Section titles | `QGroupBox`, `QGroupBox::title` | `#888888`–`#999999` |
| Inputs | `QLineEdit`, `QComboBox`, `QSpinBox` | text `#e8e8e8` |
| Muted / hints | `QLabel#labelMuted` or property `mute="true"` | `#888888` |

For a one-off muted label in code:

```cpp
auto* hint = new QLabel(tr("Optional hint"));
hint->setObjectName(QStringLiteral("labelMuted"));
// or: hint->setProperty("mute", true);
```

## Assets

- Checkbox checkmark: `:/style/icons/checkbox-check.svg` (bundled in `ibtradesystem.qrc`).

## Dependencies

- `QT += svg` in `ibtrading.pro` so SVG resources load for stylesheet `image: url(...)`.

## Tab subtree

The main content tabs use `QTabWidget#MainTabWidget` (object name set in `CIBTradeSystemView::setupConsoleLayout()`). **Do not** call `setStyleSheet()` on that tab widget with a full replacement stylesheet — it would hide the app-level rules for trees inside the tab. Extra tab-specific rules belong in `operations-console.qss` under `QTabWidget#MainTabWidget`.

## `QTableView` and model data

Stylesheets apply to the **view widget** (e.g. `QTableView { color: #d0d0d0; }`). They **cannot** express “if status column == Finished use green” because `QStandardItem` is not a `QObject` and item views do not expose per-row property selectors tied to your domain data.

**Backtest Run History / Trade Log** tables set `objectName` to `BacktestRunHistoryTable` and `BacktestTradeLogTable` so you can add rules like `QTableView#BacktestRunHistoryTable { font-size: 11px; }` in `operations-console.qss` without affecting every table.

If you need **row or cell colours from data** (status, BUY/SELL), implement a **`QStyledItemDelegate`** (or use a HTML/rich-text delegate for one column) — that stays in C++, while fonts, grid, selection, and base colours stay in QSS.

## `QTreeView` left spacing / indentation

| What you want | Where |
|---------------|--------|
| **Indent per tree depth** (each nested level shifts right) | On `QTreeView` / `QTreeWidget`: `qproperty-indentation: 20;` (maps to `setIndentation`) |
| **Padding inside each row** (space before branch + content) | `QTreeView::item, QTreeWidget::item { padding-left: 6px; }` (you already set `padding: 1px 3px` in `operations-console.qss`) |
| **Space between icon and label** in the strategy tree | `SharedUI/StrategyTreeDelegate.cpp` — `IconLeftPad` / `TextLeftPad` / `CellPadH` (custom `paint`; QSS does not affect that layout) |
| **Branch column width** | Mostly driven by `indentation` + style; `QTreeView::branch` can use `background`/`image` but rarely replaces indent math |

**Example** (global tree, tighter nesting):

```css
QTreeView, QTreeWidget {
    qproperty-indentation: 14;
}
```

**Example** (only the main tab trees):

```css
QTabWidget#MainTabWidget QTreeView {
    qproperty-indentation: 18;
}
```

## Settings slide panel

Logging / server settings are shown in a **right-hand sheet** over the **central widget** only (tabs + global status bar). Styles: `SettingsOverlay`, `SettingsOverlayBackdrop`, `SettingsSlidePanel`, and under the panel `QTreeView#settingsTreeView` + its `QHeaderView` (`qproperty-defaultSectionSize`, `qproperty-stretchLastSection`, etc.) in `operations-console.qss`. Toggle with the toolbar **Setting** action (checkable); **Escape** or click the dimmed area closes it.

**Sheet width** is `min-width` / `max-width` on `#SettingsSlidePanel` (default 480px). Insets use **`padding`** on that frame; spacing between the title row and the tree uses **`margin-top`** on `#settingsTreeView`. **Parameter column** width is driven mainly by `qproperty-defaultSectionSize` on that header; **Value** uses `qproperty-stretchLastSection: true`.

**QLayout and QSS:** Qt style sheets do **not** control `QLayout` margins or `spacing`. The settings sheet uses a plain `QVBoxLayout` with style defaults; visual spacing should come from QSS (`padding`, widget `margin`). If your platform style adds extra vertical gap between the title row and the tree, reduce `margin-top` on `#settingsTreeView` or reintroduce `body->setSpacing(0)` in `setupSettingsSlideOverlay()` as a last resort.

**Backtest workspace tab underline:** The blue bar under **Run Configuration** (and selected tabs in Run History / Equity / …) is `border-bottom: 2px solid #507dbc` on `QTabWidget#BacktestWorkspaceConfigTabs QTabBar::tab:selected` and `#BacktestWorkspaceResultTabs` (see `operations-console.qss`). Matches THEME **accent** / **borderFocus**.

**Backtest context header:** The strip with “No strategy selected” / strategy name is `QLabel#BacktestWorkspaceContextHeader` (dark `bgElevated`-style `#212121`, readable text, subtle border). Rich-text spans when a strategy is selected use inline colors in `BacktestWorkspaceDock::updateStrategyHeader()` chosen for that background.

## Why not “constants in QSS” for C++?

Qt Style Sheets **cannot** declare variables or export values to C++. There is no `#define` or `var(--accent)` that both QSS and `QString::arg` can share.

**Pattern in this project:**

1. **Static UI** → `objectName` + `operations-console.qss`.
2. **Runtime-dependent colors** (state badge `QColor`, metric value tint) → build a small stylesheet string in C++ using **`UiTheme::k…`** from **`MainSystem/ThemePalette.h`** so hex values stay aligned with the QSS THEME PALETTE.
3. When you change a palette color, update **both** `operations-console.qss` and `ThemePalette.h` (grep the old hex).

## QSS coverage vs. remaining C++

**Centralized in QSS:** Most chrome plus named labels (`#ContextWorkspaceEmptyHint`, `#strategyOverviewMuted`, `#InspectorEmptyHint`, `#metricCard` + children, backtest selector headers, etc.).

**Remaining `setStyleSheet` in C++ (intentional):**

| Location | Why |
|----------|-----|
| `WorkspaceHeader::applyStateBadge` | Background from live `QColor`; text uses `UiTheme::kTextOnAccent`. |
| `MetricsStrip` value row | Per-card `QColor` when valid; otherwise stylesheet cleared so QSS applies. |
| `StrategyWorkspace::refreshOverview` | State line color from `ModelStateUtils`; font size from `UiTheme::kFontSizeStrategyOverviewState`. |

**Always C++:** `StrategyTreeDelegate` painting, other model-driven visuals.

**Recommendation:** Prefer **`objectName` + QSS** for fixed appearance; for dynamic rules use **`UiTheme`** literals, not ad-hoc hex strings.
