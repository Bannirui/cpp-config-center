## ADDED Requirements

### Requirement: Config value storage with versioning
The system SHALL store configuration key-value pairs with an auto-incremented version number. Every write to a config key MUST create a new version while preserving all previous versions for audit and rollback.

#### Scenario: Create a new config key
- **WHEN** a user creates a new config key `timeout` with value `30` in namespace `order-service/DEV/default/application`
- **THEN** the system stores it with version `1` and returns the version number

#### Scenario: Update an existing config key
- **WHEN** a user updates config key `timeout` from `30` to `60` in the same namespace
- **THEN** the system creates version `2` with value `60`, and version `1` with value `30` remains retrievable

#### Scenario: Retrieve a specific version
- **WHEN** a user requests version `1` of config key `timeout`
- **THEN** the system returns value `30`

### Requirement: Rollback to a previous version
The system SHALL support rolling back a config key to any previously published version. A rollback MUST create a new version whose value equals the target version's value.

#### Scenario: Rollback to version 1
- **WHEN** a user rolls back `timeout` to version `1` (current version is `3`)
- **THEN** the system creates version `4` with value `30` (same as version `1`) and records the rollback in the change history

### Requirement: Change history audit trail
The system SHALL maintain a change history for each config key, recording who made each change, when it was made, the old and new values, and the operation type (create/update/rollback).

#### Scenario: View change history
- **WHEN** a user requests the change history for config key `timeout`
- **THEN** the system returns a chronological list with entries containing: version number, operation type, operator identity, timestamp, old value, and new value

### Requirement: MySQL storage backend
The system SHALL support MySQL as a storage backend. It MUST auto-create the required tables (applications, namespaces, configs, config_versions, change_logs, releases, approvals) on first startup if they do not exist.

#### Scenario: First startup with MySQL
- **WHEN** the server starts with an empty MySQL database configured
- **THEN** all required tables are created automatically and the server becomes ready to accept requests

### Requirement: SQLite storage backend
The system SHALL support SQLite as a storage backend for development and single-node deployments. It MUST auto-create the database file and tables on first startup.

#### Scenario: First startup with SQLite
- **WHEN** the server starts with SQLite backend and no existing database file
- **THEN** the database file is created with all required tables and the server becomes ready

### Requirement: Transactional publish
The system SHALL wrap config publish operations (creating release records, updating version pointers) in a database transaction to ensure atomicity.

#### Scenario: Publish transaction succeeds
- **WHEN** a user publishes a config namespace with multiple key changes
- **THEN** all keys are updated atomically — either all changes are applied or none are
