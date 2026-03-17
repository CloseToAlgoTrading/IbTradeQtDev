#include <QCoreApplication>
#include <QCommandLineParser>
#include <QTextStream>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include "Backend/ModelTreeRepository.h"
#include "Backend/SystemBackendImpl.h"
#include "Strategies/Generic/cbasicroot.h"

static QTextStream out(stdout);

static void printTree(ISystemBackend* backend)
{
    QJsonObject snapshot = backend->fullTreeSnapshot();
    QJsonArray children = snapshot["children"].toArray();
    if (children.isEmpty()) {
        out << "  (empty tree)\n";
        return;
    }

    std::function<void(const QJsonArray&, int)> printLevel;
    printLevel = [&](const QJsonArray& nodes, int depth) {
        for (const auto& val : nodes) {
            QJsonObject node = val.toObject();
            QString indent(depth * 2, ' ');
            QString active = node["active"].toBool() ? "ON" : "OFF";
            out << indent << node["name"].toString()
                << " [type=" << node["type"].toInt()
                << " uuid=" << node["uuid"].toString().left(8) << "..."
                << " " << active << "]\n";
            if (node.contains("children"))
                printLevel(node["children"].toArray(), depth + 1);
        }
    };
    printLevel(children, 1);
}

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    app.setApplicationName("ibtrade-cli");
    app.setApplicationVersion("1.0");

    QCommandLineParser parser;
    parser.setApplicationDescription("IbTradeQt CLI - backend proof of concept");
    parser.addHelpOption();
    parser.addVersionOption();

    parser.addOption({{"d", "db"}, "SQLite database path", "path", "model_tree.sqlite"});

    parser.addPositionalArgument("command",
        "Command: tree, add-account, add-portfolio, add-strategy, remove, rename, info, export");

    parser.process(app);

    QString dbPath = parser.value("db");
    QStringList args = parser.positionalArguments();

    if (args.isEmpty()) {
        out << "No command specified. Use --help for usage.\n";
        return 1;
    }

    ModelTreeRepository repo(dbPath, "cli_conn");
    if (!repo.initialize()) {
        out << "Failed to initialize database at " << dbPath << "\n";
        return 1;
    }

    SystemBackendImpl backend(&repo);

    if (!backend.loadFromDb()) {
        out << "No existing tree in DB (starting fresh).\n";
    }

    QString cmd = args[0];

    if (cmd == "tree") {
        out << "Model Tree:\n";
        printTree(&backend);
    }
    else if (cmd == "add-account") {
        QString name = args.size() > 1 ? args[1] : "Account";
        QString uuid = backend.createAccount(name);
        if (uuid.isEmpty()) {
            out << "Failed to create account.\n";
            return 1;
        }
        out << "Created account: " << name << " [" << uuid << "]\n";
    }
    else if (cmd == "add-portfolio") {
        if (args.size() < 2) {
            out << "Usage: add-portfolio <account-uuid> [name]\n";
            return 1;
        }
        QString parentId = args[1];
        QString name = args.size() > 2 ? args[2] : "Portfolio";
        QString uuid = backend.createPortfolio(parentId, name);
        if (uuid.isEmpty()) {
            out << "Failed to create portfolio (check account UUID).\n";
            return 1;
        }
        out << "Created portfolio: " << name << " [" << uuid << "]\n";
    }
    else if (cmd == "add-strategy") {
        if (args.size() < 2) {
            out << "Usage: add-strategy <portfolio-uuid> [name]\n";
            return 1;
        }
        QString parentId = args[1];
        QString uuid = backend.createStrategy(parentId, ModelType::STRATEGY_PIPELINE);
        if (uuid.isEmpty()) {
            out << "Failed to create strategy (check portfolio UUID).\n";
            return 1;
        }
        out << "Created strategy [" << uuid << "]\n";
    }
    else if (cmd == "remove") {
        if (args.size() < 2) {
            out << "Usage: remove <uuid>\n";
            return 1;
        }
        if (backend.removeNode(args[1])) {
            out << "Removed node " << args[1] << "\n";
        } else {
            out << "Failed to remove node.\n";
            return 1;
        }
    }
    else if (cmd == "rename") {
        if (args.size() < 3) {
            out << "Usage: rename <uuid> <new-name>\n";
            return 1;
        }
        if (backend.renameNode(args[1], args[2])) {
            out << "Renamed to " << args[2] << "\n";
        } else {
            out << "Failed to rename node.\n";
            return 1;
        }
    }
    else if (cmd == "info") {
        if (args.size() < 2) {
            out << "Usage: info <uuid>\n";
            return 1;
        }
        QJsonObject info = backend.nodeInfo(args[1]);
        if (info.isEmpty()) {
            out << "Node not found.\n";
            return 1;
        }
        out << QJsonDocument(info).toJson(QJsonDocument::Indented);
    }
    else if (cmd == "export") {
        QString path = args.size() > 1 ? args[1] : "model_tree_export.json";
        if (backend.exportToJsonFile(path)) {
            out << "Exported to " << path << "\n";
        } else {
            out << "Export failed.\n";
            return 1;
        }
    }
    else {
        out << "Unknown command: " << cmd << "\n";
        out << "Available: tree, add-account, add-portfolio, add-strategy, remove, rename, info, export\n";
        return 1;
    }

    return 0;
}
