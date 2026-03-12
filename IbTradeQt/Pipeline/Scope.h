#ifndef PIPELINE_SCOPE_H
#define PIPELINE_SCOPE_H

#include <vector>

namespace Pipeline {

enum class Scope {
    Strategy,
    Portfolio,
    Account
};

inline const std::vector<Scope>& riskScopeOrder() {
    static const std::vector<Scope> order = {
        Scope::Strategy,
        Scope::Portfolio,
        Scope::Account
    };
    return order;
}

} // namespace Pipeline

#endif // PIPELINE_SCOPE_H
