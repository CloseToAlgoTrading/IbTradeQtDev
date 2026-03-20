#pragma once

// ─────────────────────────────────────────────────────────────────────────────
// UI color tokens shared with MainSystem/style/operations-console.qss
//
// QSS cannot define variables or export values to C++. For runtime-generated
// stylesheets (badge background, per-metric color, etc.), use these literals
// so hex stays aligned with the QSS THEME PALETTE. When you change a value,
// update the matching rules in operations-console.qss.
// ─────────────────────────────────────────────────────────────────────────────

namespace UiTheme {

inline constexpr char kBgWindow[]      = "#1b1b1b";
inline constexpr char kBgElevated[]    = "#212121";
inline constexpr char kBgChrome[]      = "#161616";
inline constexpr char kBorderSubtle[]  = "#2a2a2a";
inline constexpr char kBorderField[]   = "#333333";
inline constexpr char kBorderFocus[]   = "#507dbc";
inline constexpr char kTextPrimary[]   = "#e8e8e8";
inline constexpr char kTextSecondary[] = "#b8b8b8";
inline constexpr char kTextMuted[]     = "#888888";
inline constexpr char kTextDisabled[]  = "#666666";
inline constexpr char kSelectionBg[]   = "#2a3a50";
inline constexpr char kAccent[]        = "#507dbc";

/**
 * Monochrome PNG icons on dark chrome: applied in CIconHandler::loadIconForChrome()
 * only (trees use loadIconFromResourceTheme() and keep full-colour assets).
 * QSS cannot recolor QIcon raster pixmaps — keep in sync with THEME PALETTE
 * "toolbarIcon" in operations-console.qss.
 */
inline constexpr char kToolbarIcon[] = "#c8c8c8";

/** High-contrast text on tinted / colored badges (workspace header state) */
inline constexpr char kTextOnAccent[] = "#ffffff";

/** Empty-page / placeholder hints */
inline constexpr char kTextPlaceholder[] = "#707070";
inline constexpr char kTextHintLarge[]   = "#8888a0";

/** Metrics strip cards (QFrame#metricCard) */
inline constexpr char kMetricCardBg[]       = "#252538";
inline constexpr char kMetricCardBorder[]   = "#2a2a3a";
inline constexpr char kMetricLabel[]        = "#8888a0";
inline constexpr char kMetricValueDefault[] = "#e0e0e0";

inline constexpr char kWarningAmber[] = "#eab308";

inline constexpr char kInspectorDescription[] = "#777777";
inline constexpr char kInspectorInfoValue[]    = "#aaaaaa";

/** Dynamic overview state text (Strategy workspace); matches QSS when not overridden */
inline constexpr int kFontSizeStrategyOverviewState = 14;

inline constexpr int kFontSizeMetricValue = 13;

} // namespace UiTheme
