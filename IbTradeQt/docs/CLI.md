# IbTradeQt CLI

`ibtrade-cli` is a standalone command-line client that operates directly on the same SQLite database as the GUI. It uses `ISystemBackend` / `SystemBackendImpl` — the exact same backend code — with no GUI components.

---

## Building

```bash
cd cli
/home/denis/Qt/6.9.2/gcc_64/bin/qmake cli.pro
make -j$(nproc)
```

The binary is placed at `release/ibtrade-cli`.

---

## Synopsis

```
ibtrade-cli [options] <command> [arguments]
```

**Options:**

| Option | Default | Description |
|--------|---------|-------------|
| `-d <path>`, `--db <path>` | `model_tree.sqlite` | Path to the SQLite database |
| `--help` | | Print help |
| `--version` | | Print version |

---

## Commands

### `tree`

Print the full model tree loaded from the database.

```bash
ibtrade-cli tree
ibtrade-cli --db /path/to/model_tree.sqlite tree
```

**Example output:**
```
Model Tree:
  My IB Account [type=8 uuid=a1b2c3d4... ON]
    Tech Portfolio [type=9 uuid=e5f6a7b8... ON]
      Momentum Strategy [type=13 uuid=c9d0e1f2... OFF]
    Growth Portfolio [type=9 uuid=11223344... ON]
```

- `type=8` → ACCOUNT
- `type=9` → PORTFOLIO  
- `type=13` → STRATEGY_PIPELINE
- `ON` / `OFF` → active state

---

### `add-account [name]`

Create a new account at the top level.

```bash
ibtrade-cli add-account "My IB Account"
```

**Output:**
```
Created account: My IB Account [a1b2c3d4-e5f6-7890-abcd-ef1234567890]
```

The printed UUID is used as the parent reference for `add-portfolio`.

---

### `add-portfolio <account-uuid> [name]`

Create a portfolio under an account.

```bash
ibtrade-cli add-portfolio a1b2c3d4-e5f6-7890-abcd-ef1234567890 "Tech Portfolio"
```

**Output:**
```
Created portfolio: Tech Portfolio [e5f6a7b8-1234-5678-90ab-cdef01234567]
```

---

### `add-strategy <portfolio-uuid>`

Create a new pipeline strategy under a portfolio. The strategy is created with an empty `pipelineConfig`.

```bash
ibtrade-cli add-strategy e5f6a7b8-1234-5678-90ab-cdef01234567
```

**Output:**
```
Created strategy [c9d0e1f2-3456-7890-abcd-ef0123456789]
```

---

### `remove <uuid>`

Remove a node and all its descendants.

```bash
ibtrade-cli remove c9d0e1f2-3456-7890-abcd-ef0123456789
```

**Output:**
```
Removed node c9d0e1f2-3456-7890-abcd-ef0123456789
```

> Cascades through all children. Removing an account removes its portfolios and strategies too.

---

### `rename <uuid> <new-name>`

Rename a node.

```bash
ibtrade-cli rename a1b2c3d4-e5f6-7890-abcd-ef1234567890 "Renamed Account"
```

**Output:**
```
Renamed to Renamed Account
```

---

### `info <uuid>`

Print full node information as JSON (persistent config only — no runtime state).

```bash
ibtrade-cli info c9d0e1f2-3456-7890-abcd-ef0123456789
```

**Example output:**
```json
{
    "uuid": "c9d0e1f2-3456-7890-abcd-ef0123456789",
    "name": "Momentum Strategy",
    "type": 13,
    "active": true,
    "config": {
        "parameters": {
            "BP": 10000,
            "Name": "Momentum Strategy"
        },
        "pipelineConfig": {
            "alphaBlocks": [
                {
                    "blockId": "Momentum",
                    "config": { "period": 20, "threshold": 0.02 }
                }
            ],
            "executionBlock": { "blockId": "MarketOrder", "config": {} },
            "riskBlocks": [
                {
                    "blockId": "MaxPosition",
                    "config": { "maxPositionValue": 10000 }
                }
            ],
            "selectionBlocks": [
                { "blockId": "PassAll", "config": {} }
            ]
        }
    }
}
```

---

### `export [path]`

Export the entire model tree to a JSON file (same format as `model_tree_config.json` used by the GUI).

```bash
ibtrade-cli export
ibtrade-cli export /tmp/backup.json
```

Default path: `model_tree_export.json`

**Output:**
```
Exported to model_tree_export.json
```

---

## Typical workflow

```bash
# 1. Build (first time)
cd cli && /home/denis/Qt/6.9.2/gcc_64/bin/qmake && make -j$(nproc)

# 2. Check what's in the database the GUI uses
./release/ibtrade-cli --db release/model_tree.sqlite tree

# 3. Add structure
ACCT=$(./release/ibtrade-cli add-account "Paper Trading" | grep -oP '\[\K[^\]]+')
PORT=$(./release/ibtrade-cli add-portfolio $ACCT "US Equities" | grep -oP '\[\K[^\]]+')
./release/ibtrade-cli add-strategy $PORT

# 4. Inspect a node
./release/ibtrade-cli info $PORT

# 5. Rename
./release/ibtrade-cli rename $ACCT "IB Paper DU123456"

# 6. Back up the tree
./release/ibtrade-cli export /tmp/model_backup_$(date +%Y%m%d).json

# 7. View the tree
./release/ibtrade-cli tree
```

---

## Database sharing with the GUI

The CLI and the GUI use the same `model_tree.sqlite` file. By default the GUI writes to `release/model_tree.sqlite` (in the application working directory). Point the CLI at the same file:

```bash
./release/ibtrade-cli --db release/model_tree.sqlite tree
```

> **Do not run CLI and GUI simultaneously against the same database.** SQLite does not support concurrent writes from separate processes without WAL mode enabled. In practice, use the CLI when the GUI is closed, or use it for read-only inspection (`tree`, `info`, `export`) while the GUI is running.

---

## Current limitations

| Feature | Status |
|---------|--------|
| `add-account`, `add-portfolio`, `add-strategy`, `remove`, `rename`, `info`, `export` | ✅ Working |
| `tree` — display full hierarchy | ✅ Working |
| `add-block` / `remove-block` — modify pipeline config | 🔲 Not yet exposed (use GUI or `updateNodeConfig` via the backend API in code) |
| `move` — move node to different parent | 🔲 Not yet exposed as a CLI command (`ISystemBackend::moveNode()` exists) |
| `activate` / `deactivate` — toggle active state | 🔲 Not yet exposed |
| `import` — import from JSON | 🔲 Not yet exposed |
| Broker connection | 🔲 Not applicable (CLI is headless, no IB connection) |

---

## Architecture note

The CLI is a proof that the backend is client-independent. It uses exactly the same code path as the GUI mutations:

```
CLI command
    └─> SystemBackendImpl::createAccount()   (same code as GUI)
            └─> ModelTreeRepository::insertNode()
                    └─> SQLite model_nodes table
```

No special CLI-only code exists in the backend. See [ARCHITECTURE.md](ARCHITECTURE.md) for the full backend service design.
