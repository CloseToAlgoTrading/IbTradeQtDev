#include "PersistenceFactory.h"
#include "IModelTreeRepository.h"
#include "ModelTreeRepository.h"
#include "ModelTreeRepositoryPostgres.h"

namespace Persistence {

std::unique_ptr<IModelTreeRepository> createModelTreeRepository(const StorageConfig& cfg,
                                                                  const QString& connectionName)
{
    if (cfg.modelStore.backend == StorageBackend::Postgresql) {
        const PostgreSqlConnection& pg = cfg.modelStore.postgres;
        return std::make_unique<ModelTreeRepositoryPostgres>(
            pg.host, pg.port, pg.database, pg.user, pg.password, connectionName);
    }
    return std::make_unique<ModelTreeRepository>(cfg.modelStore.sqlitePath, connectionName);
}

} // namespace Persistence
