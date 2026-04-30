# gRPC Service Documentation

## Service Definition

The gRPC service is defined in `proto/config_service.proto`.

Server listens on port `9090` by default.

## Service: ConfigService

### RPC Methods

| Method | Type | Description |
|--------|------|-------------|
| `GetConfig` | Unary | Get a single config value |
| `SetConfig` | Unary | Set/update a config value (draft) |
| `DeleteConfig` | Unary | Delete a config key (draft) |
| `WatchConfig` | Server Streaming | Subscribe to config changes |
| `PublishRelease` | Unary | Publish draft changes |
| `GetReleaseHistory` | Unary | Query release history |

### GetConfig

```protobuf
rpc GetConfig(GetConfigRequest) returns (GetConfigResponse);

message GetConfigRequest {
    string namespace_id = 1;
    string key = 2;
}

message GetConfigResponse {
    Config config = 1;
}
```

Usage example:
```cpp
grpc::ClientContext context;
config_center::GetConfigRequest request;
request.set_namespace_id("order-service/DEV/default/application");
request.set_key("timeout");

config_center::GetConfigResponse response;
grpc::Status status = stub->GetConfig(&context, request, &response);
if (status.ok()) {
    std::cout << "Value: " << response.config().value() << std::endl;
    std::cout << "Version: " << response.config().version() << std::endl;
}
```

### SetConfig

```protobuf
rpc SetConfig(SetConfigRequest) returns (SetConfigResponse);

message SetConfigRequest {
    string namespace_id = 1;
    string key = 2;
    string value = 3;
    string operator_id = 4;
}
```

### DeleteConfig

```protobuf
rpc DeleteConfig(DeleteConfigRequest) returns (DeleteConfigResponse);

message DeleteConfigRequest {
    string namespace_id = 1;
    string key = 2;
    string operator_id = 3;
}
```

### WatchConfig (Server Streaming)

```protobuf
rpc WatchConfig(WatchConfigRequest) returns (stream ConfigChangeNotification);

message WatchConfigRequest {
    repeated string namespace_ids = 1;
    int64 last_release_id = 2;
    string instance_id = 3;
    string ip_address = 4;
}

message ConfigChangeNotification {
    string namespace_id = 1;
    int64 release_id = 2;
    repeated string changed_keys = 3;
    int64 timestamp = 4;
}
```

Usage example:
```cpp
grpc::ClientContext context;
config_center::WatchConfigRequest request;
request.add_namespace_ids("order-service/DEV/default/application");
request.set_instance_id("my-instance-1");

auto reader = stub->WatchConfig(&context, request);
config_center::ConfigChangeNotification notification;
while (reader->Read(&notification)) {
    std::cout << "Config changed in namespace: " << notification.namespace_id()
              << ", release: " << notification.release_id() << std::endl;
}
```

### PublishRelease

```protobuf
rpc PublishRelease(PublishReleaseRequest) returns (PublishReleaseResponse);

message PublishReleaseRequest {
    string namespace_id = 1;
    string release_note = 2;
    string operator_id = 3;
}
```

### GetReleaseHistory

```protobuf
rpc GetReleaseHistory(GetReleaseHistoryRequest) returns (GetReleaseHistoryResponse);

message GetReleaseHistoryRequest {
    string namespace_id = 1;
    int32 page = 2;
    int32 page_size = 3;
}

message GetReleaseHistoryResponse {
    repeated Release releases = 1;
    int32 total = 2;
}
```

## Messages

### Config
```protobuf
message Config {
    string namespace_id = 1;
    string key = 2;
    string value = 3;
    int32 version = 4;
    string status = 5;  // "live" or "draft"
}
```

### Release
```protobuf
message Release {
    int64 id = 1;
    string namespace_id = 2;
    int32 release_version = 3;
    string release_note = 4;
    string release_type = 5;  // "full" or "grayscale"
    string status = 6;        // "active" or "retired"
    string operator_id = 7;
    string created_at = 8;
}
```

### Authentication

gRPC calls can include JWT tokens via metadata:

```cpp
context.AddMetadata("authorization", "Bearer <jwt-token>");
```

## Error Handling

gRPC status codes map to error conditions:
- `NOT_FOUND`: Config key or namespace not found
- `INVALID_ARGUMENT`: Invalid request parameters
- `UNAUTHENTICATED`: Missing or invalid JWT token
- `PERMISSION_DENIED`: Insufficient role permissions
- `INTERNAL`: Server-side error
