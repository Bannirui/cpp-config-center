## ADDED Requirements

### Requirement: Long-polling subscription
The system SHALL support HTTP long-polling for config change notifications. A client MUST be able to open an HTTP connection, specify watched namespaces, and receive a response when any watched namespace changes or a configurable timeout elapses.

#### Scenario: Client polls and receives notification on change
- **WHEN** a client opens a long-poll connection for namespace `order-service/DEV/default/application` with timeout 60s, and a config change is published 10 seconds later
- **THEN** the server responds within 1 second of the change with the notification, including the namespace and release ID

#### Scenario: Client polls and times out
- **WHEN** a client opens a long-poll connection with timeout 30s and no config changes occur
- **THEN** the server responds after 30 seconds with an empty notification indicating no changes

### Requirement: gRPC streaming subscription
The system SHALL support gRPC server-side streaming for config change notifications. A client MUST be able to open a gRPC stream, send a subscription request with watched namespaces, and receive a stream of change notifications.

#### Scenario: gRPC stream receives notification
- **WHEN** a client establishes a gRPC subscription stream for namespace `order-service/DEV/default/application`, and a config change is published
- **THEN** the server pushes a notification message on the stream containing the namespace, release ID, and changed key names

#### Scenario: gRPC stream handles reconnection
- **WHEN** a gRPC subscription stream is broken (network interruption)
- **THEN** the client reconnects and re-subscribes, receiving the current config state to reconcile any missed changes

### Requirement: Per-namespace notification
The system SHALL send change notifications only to clients subscribed to the specific namespace that changed. Clients subscribed to other namespaces MUST NOT receive notifications.

#### Scenario: Targeted notification
- **WHEN** a config change is published in namespace `order-service/DEV/default/application`, and client A is subscribed to that namespace while client B is subscribed to `payment-service/PROD/default/application`
- **THEN** only client A receives a notification; client B is not notified

### Requirement: Notification includes change details
The system SHALL include in each notification: the namespace identifier, the release ID, a list of changed config keys, and the release timestamp.

#### Scenario: Notification payload
- **WHEN** a notification is sent for a release that changed keys `timeout` and `max_connections`
- **THEN** the notification payload contains: `namespace: order-service/DEV/default/application`, `release_id: 42`, `changed_keys: [timeout, max_connections]`, `timestamp: <ISO8601>`

### Requirement: Client re-subscription on reconnect
The system SHALL allow clients to detect missed changes after a disconnection by providing the last known release ID. The server MUST return all releases that occurred after that ID.

#### Scenario: Catch up after disconnect
- **WHEN** a client reconnects with `last_release_id: 40` and releases 41, 42, and 43 have occurred
- **THEN** the server returns release summaries for 41, 42, and 43 so the client can fetch updated configs
