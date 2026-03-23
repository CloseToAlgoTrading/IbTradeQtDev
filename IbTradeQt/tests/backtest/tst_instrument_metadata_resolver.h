#ifndef TST_INSTRUMENT_METADATA_RESOLVER_H
#define TST_INSTRUMENT_METADATA_RESOLVER_H

#include <QObject>

class TestInstrumentMetadataResolver : public QObject {
    Q_OBJECT
private slots:
    void resolve_classificationOverrideWins();
    void resolve_infersWhenNoRow();
    void resolve_usesProviderMetadataRow();
    void resolve_sessionPolicyPassesThrough();
};

#endif
