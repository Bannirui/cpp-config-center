## ADDED Requirements

### Requirement: REST API for config CRUD
The system SHALL expose RESTful HTTP endpoints for all configuration operations. All endpoints MUST accept and return JSON.

#### Scenario: Get a config value
- **WHEN** a client sends `GET /api/v1/namespaces/{namespace}/keys/{key}`
- **THEN** the server returns `200 OK` with JSON body `{"key": "timeout", "value": "30", "version": 2}`

#### Scenario: Set a config value (draft)
- **WHEN** a client sends `PUT /api/v1/namespaces/{namespace}/keys/{key}` with body `{"value": "60"}`
- **THEN** the server saves the change as a draft and returns `200 OK` with the new draft status

#### Scenario: Delete a config key
- **WHEN** a client sends `DELETE /api/v1/namespaces/{namespace}/keys/{key}`
- **THEN** the server marks the key for deletion in the draft and returns `200 OK`

### Requirement: REST API for namespace management
The system SHALL expose RESTful HTTP endpoints for managing applications, environments, clusters, and namespaces.

#### Scenario: Create an application
- **WHEN** a client sends `POST /api/v1/apps` with body `{"app_id": "order-service", "name": "Order Service"}`
- **THEN** the server creates the application and returns `201 Created`

#### Scenario: List namespaces for an app
- **WHEN** a client sends `GET /api/v1/apps/order-service/namespaces`
- **THEN** the server returns all namespaces under that application across all environments and clusters

### Requirement: REST API for publish and release
The system SHALL expose endpoints for publishing drafts and querying release history.

#### Scenario: Publish a draft
- **WHEN** a client sends `POST /api/v1/namespaces/{namespace}/publish` with body `{"release_note": "Updated timeouts"}`
- **THEN** the server publishes all draft changes, creates a release, and returns `200 OK` with release details

#### Scenario: Get release history
- **WHEN** a client sends `GET /api/v1/namespaces/{namespace}/releases`
- **THEN** the server returns a paginated list of past releases with version, timestamp, operator, and release note

### Requirement: REST API for long-polling
The system SHALL expose an HTTP endpoint for long-polling config change notifications.

#### Scenario: Long-poll for changes
- **WHEN** a client sends `GET /api/v1/notifications?namespace={namespace}&release_id={last_id}&timeout=60`
- **THEN** the server holds the connection open until a change occurs or timeout expires, then responds with notification or empty result

### Requirement: gRPC service definition
The system SHALL provide a gRPC service definition (`ConfigService`) with methods for all configuration operations: `GetConfig`, `SetConfig`, `DeleteConfig`, `WatchConfig` (server streaming), `PublishRelease`, and `GetReleaseHistory`.

#### Scenario: gRPC GetConfig call
- **WHEN** a gRPC client calls `ConfigService.GetConfig` with namespace and key
- **THEN** the server returns the current config value with version

#### Scenario: gRPC WatchConfig stream
- **WHEN** a gRPC client calls `ConfigService.WatchConfig` with subscription details
- **THEN** the server opens a stream and pushes notifications whenever watched namespaces change

### Requirement: Authentication for API access
The system SHALL require JWT Bearer token authentication for all write operations (create, update, delete, publish). Read operations MAY be configured to require authentication. The system MUST provide a login endpoint that returns a JWT token.

#### Scenario: Authenticated write request
- **WHEN** a client sends a PUT request with a valid `Authorization: Bearer <token>` header
- **THEN** the request is processed normally

#### Scenario: Unauthenticated write request rejected
- **WHEN** a client sends a PUT request without an Authorization header
- **THEN** the server returns `401 Unauthorized`

#### Scenario: Login and obtain token
- **WHEN** a client sends `POST /api/v1/auth/login` with valid credentials
- **THEN** the server returns a JWT token with an expiration time

### Requirement: Role-based access control
The system SHALL support three roles: `admin` (full access), `operator` (can edit and publish configs), and `viewer` (read-only access).

#### Scenario: Viewer cannot publish
- **WHEN** a user with `viewer` role attempts to publish a namespace
- **THEN** the server returns `403 Forbidden`

### Requirement: Health check endpoint
The system SHALL expose a health check endpoint at `GET /health` that returns `200 OK` with server status, uptime, and database connectivity status.

#### Scenario: Health check
- **WHEN** a client sends `GET /health`
- **THEN** the server returns `200 OK` with JSON body `{"status": "UP", "uptime_seconds": 3600, "db_connected": true}`
