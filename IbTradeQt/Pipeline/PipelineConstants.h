#pragma once
#include <QLatin1StringView>
#include <QString>

// ─────────────────────────────────────────────────────────────────────────────
// Single source of truth for pipeline-related string constants.
//
// Category  – human-readable block category names used throughout the UI and
//             registry (e.g. "Alpha", "Risk").
//
// Key       – JSON property keys used to store blocks inside a pipelineConfig
//             object (e.g. "alphas", "selection").  Always use these instead of
//             inline string literals so that the schema can be changed in one
//             place.
//
// Helper free-functions:
//   categoryKey(cat)      → JSON storage key for a category name
//   categoryIsArray(cat)  → true if the JSON value is an array (multi-block)
// ─────────────────────────────────────────────────────────────────────────────

namespace Pipeline {

namespace Category {
    inline constexpr QLatin1StringView Alpha     { "Alpha"     };
    inline constexpr QLatin1StringView Risk      { "Risk"      };
    inline constexpr QLatin1StringView Rebalance { "Rebalance" };
    inline constexpr QLatin1StringView Execution { "Execution" };
    inline constexpr QLatin1StringView Selection { "Selection" };
} // namespace Category

namespace Key {
    // Array-valued pipeline config slots
    inline constexpr QLatin1StringView Alphas    { "alphas"    };
    inline constexpr QLatin1StringView Risks     { "risks"     };
    inline constexpr QLatin1StringView Selection { "selection" };

    // Single-valued pipeline config slots
    inline constexpr QLatin1StringView Rebalance { "rebalance" };
    inline constexpr QLatin1StringView Execution { "execution" };

    // Per-block entry keys (inside every block object stored in the arrays above)
    inline constexpr QLatin1StringView BlockId   { "blockId"   };
    inline constexpr QLatin1StringView Config    { "config"    };

    // Top-level strategy node keys
    inline constexpr QLatin1StringView PipelineConfig { "pipelineConfig" };
    inline constexpr QLatin1StringView AssetList      { "assetList"      };
} // namespace Key

// Returns the JSON storage key for a category name.
// E.g. Category::Alpha → Key::Alphas,  Category::Selection → Key::Selection
inline QLatin1StringView categoryKey(const QString& category)
{
    if (category == Category::Alpha)     return Key::Alphas;
    if (category == Category::Risk)      return Key::Risks;
    if (category == Category::Selection) return Key::Selection;
    if (category == Category::Rebalance) return Key::Rebalance;
    if (category == Category::Execution) return Key::Execution;
    return {};
}

// Returns true if the JSON slot for this category holds an array of blocks.
inline bool categoryIsArray(const QString& category)
{
    return category == Category::Alpha
        || category == Category::Risk
        || category == Category::Selection;
}

} // namespace Pipeline
