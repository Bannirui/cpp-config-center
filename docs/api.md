# REST API Documentation

## Base URL

```
http://<host>:8080/api/v1
```

## Authentication

Write endpoints require a JWT Bearer token:
```
Authorization: Bearer <token>
```

Obtain a token via:
```
POST /api/v1/auth/login
Content-Type: application/json

{
  "username": "admin",
  "password": "password"
}
```

Response:
```json
{
  "token": "eyJhbGciOi...",
  "expires_in": 86400
}
```

## Error Format

All errors return:
```json
{
  "error": "Description of the error",
  "code": 400
}
```

HTTP status codes: `400` (Bad Request), `401` (Unauthorized), `403` (Forbidden), `404` (Not Found), `409` (Conflict), `500` (Internal Error)

## Endpoints

### Health Check

```
GET /health
```

Response:
```json
{
  "status": "UP",
  "uptime_seconds": 3600,
  "db_connected": true
}
```

### Applications

#### Create Application
```
POST /api/v1/apps
Content-Type: application/json

{
  "app_id": "order-service",
  "name": "Order Service",
  "description": "Handles order processing"
}
```

Response: `201 Created`
```json
{
  "app_id": "order-service",
  "name": "Order Service"
}
```

#### List Applications
```
GET /api/v1/apps
```

Response:
```json
{
  "applications": [
    {
      "app_id": "order-service",
      "name": "Order Service",
      "created_at": "2024-01-01T00:00:00Z"
    }
  ]
}
```

#### Get Application
```
GET /api/v1/apps/{app_id}
```

#### Delete Application
```
DELETE /api/v1/apps/{app_id}
```

### Namespaces

#### Create Namespace
```
POST /api/v1/namespaces
Content-Type: application/json

{
  "app_id": "order-service",
  "env": "DEV",
  "cluster": "default",
  "name": "database",
  "type": "private"
}
```

Namespace types: `private`, `public`, `common`

#### List Namespaces
```
GET /api/v1/apps/{app_id}/namespaces
```

### Config Keys

#### Get Config
```
GET /api/v1/namespaces/{namespace}/keys/{key}
```

Response:
```json
{
  "key": "timeout",
  "value": "30",
  "version": 2
}
```

#### Set Config (creates draft)
```
PUT /api/v1/namespaces/{namespace}/keys/{key}
Content-Type: application/json

{
  "value": "60"
}
```

Response:
```json
{
  "status": "ok"
}
```

#### Delete Config (marks for deletion in draft)
```
DELETE /api/v1/namespaces/{namespace}/keys/{key}
```

### Publish / Releases

#### Publish Draft
```
POST /api/v1/namespaces/{namespace}/publish
Content-Type: application/json

{
  "release_note": "Updated timeouts for payment gateway"
}
```

Response:
```json
{
  "release_id": 42,
  "release_version": 1,
  "release_note": "Updated timeouts for payment gateway",
  "created_at": "2024-01-01T00:00:00Z"
}
```

#### Get Release History
```
GET /api/v1/namespaces/{namespace}/releases
```

Response:
```json
{
  "releases": [
    {
      "id": 42,
      "release_version": 1,
      "release_note": "Updated timeouts",
      "release_type": "full",
      "status": "active",
      "operator_id": "admin",
      "created_at": "2024-01-01T00:00:00Z"
    }
  ]
}
```

### Long-Polling Notifications

```
GET /api/v1/notifications?namespace={namespace}&release_id={last_known_release_id}&timeout=60
```

- `namespace`: The namespace to watch
- `release_id`: Last known release ID for catch-up
- `timeout`: Maximum wait time in seconds (default: 60, max: 120)

Response (when change detected):
```json
{
  "namespace": "order-service/DEV/default/application",
  "release_id": 43,
  "changed_keys": ["timeout", "max_connections"],
  "timestamp": "2024-01-01T00:01:00Z"
}
```

Response (timeout, no change):
```json
{
  "changed": false
}
```
