#ifndef PERSISTENCEFACTORY_H
#define PERSISTENCEFACTORY_H

#include "StorageConfig.h"
#include <memory>

class IModelTreeRepository;

namespace Persistence {

/** Single composition entry for ModelStore — callers must not construct repos ad hoc from INI. */
std::unique_ptr<IModelTreeRepository> createModelTreeRepository(const StorageConfig& cfg,
                                                              const QString& connectionName);

} // namespace Persistence

#endif
