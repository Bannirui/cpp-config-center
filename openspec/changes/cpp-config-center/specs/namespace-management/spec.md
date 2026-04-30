## ADDED Requirements

### Requirement: Application registration
The system SHALL support registering applications with a unique application ID. An application represents a logical service or microservice.

#### Scenario: Register a new application
- **WHEN** an admin registers application `order-service` with display name "Order Service"
- **THEN** the application is created and can have namespaces associated with it

#### Scenario: Duplicate application ID rejected
- **WHEN** an admin tries to register an application with an ID that already exists
- **THEN** the system returns an error indicating the application ID is already taken

### Requirement: Environment management
The system SHALL support environments as a dimension of namespace isolation. Predefined environments MUST include DEV, FAT, UAT, and PROD. Custom environments MAY be added.

#### Scenario: List available environments
- **WHEN** a user requests the list of environments for application `order-service`
- **THEN** the system returns the default environments (DEV, FAT, UAT, PROD) plus any custom environments

### Requirement: Cluster management
The system SHALL support clusters as a dimension of namespace isolation within an environment. Each application+environment combination MUST have at least a `default` cluster. Additional clusters MAY be created for regional or logical isolation.

#### Scenario: Create a regional cluster
- **WHEN** an admin creates cluster `shanghai` under application `order-service` and environment `PROD`
- **THEN** the cluster is created and configs can be scoped to `order-service/PROD/shanghai`

### Requirement: Namespace creation
The system SHALL support namespaces as the unit of configuration grouping. A namespace is scoped to an application + environment + cluster combination. Namespace types SHALL include `private` (app-specific), `public` (shared across apps), and `common` (system-level defaults).

#### Scenario: Create a private namespace
- **WHEN** a user creates namespace `database` of type `private` under `order-service/DEV/default`
- **THEN** the namespace is created and config keys can be added to it

#### Scenario: Create a public namespace
- **WHEN** a user creates namespace `common-logging` of type `public`
- **THEN** other applications can reference and inherit configs from this namespace

### Requirement: Namespace inheritance
The system SHALL support namespace inheritance where a namespace can reference a public namespace as its parent. Config lookups in the child namespace MUST fall back to the parent namespace for keys not defined in the child.

#### Scenario: Config fallback to parent namespace
- **WHEN** namespace `order-service/DEV/default/application` inherits from `common-app-defaults`, and key `log_level` is not defined in the child but is defined as `INFO` in the parent
- **THEN** querying `log_level` from the child namespace returns `INFO` from the parent

### Requirement: Config key format validation
The system SHALL validate config key names against the pattern `[a-zA-Z0-9._-]+` (max 128 characters) and reject keys that don't match.

#### Scenario: Invalid key name rejected
- **WHEN** a user tries to create a config key with name `timeout!@#`
- **THEN** the system returns an error indicating the key name contains invalid characters

### Requirement: Config value size limit
The system SHALL enforce a config value size limit of 1 MB per key.

#### Scenario: Oversized value rejected
- **WHEN** a user tries to set a config value exceeding 1 MB
- **THEN** the system returns an error indicating the value is too large
