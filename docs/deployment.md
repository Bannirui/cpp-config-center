# Deployment Guide

## Single-Node Deployment (SQLite)

For development, testing, or small production deployments.

### Prerequisites

- Linux/macOS with C++17 compiler
- CMake 3.20+
- System packages: Boost, OpenSSL, Protobuf, gRPC

```bash
# Ubuntu/Debian
sudo apt install cmake libboost-system-dev libssl-dev protobuf-compiler libprotobuf-dev libgrpc++-dev

# macOS
brew install cmake boost openssl protobuf grpc
```

### Build

```bash
cd config-center
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

Other dependencies (nlohmann/json, yaml-cpp, jwt-cpp, sqlite_orm) are auto-fetched by CMake FetchContent.

### Configure

Copy and edit `config-server.yaml`:

```yaml
server:
  http:
    host: "0.0.0.0"
    port: 8080
  grpc:
    host: "0.0.0.0"
    port: 9090

storage:
  backend: sqlite
  sqlite:
    path: /var/lib/config-center/config-center.db

auth:
  enabled: true
  jwt:
    secret: "use-a-strong-random-secret-here"
    expiry_hours: 24

logging:
  level: info
  file: /var/log/config-center/server.log
```

### Run

```bash
mkdir -p /var/lib/config-center
mkdir -p /var/log/config-center
./build/src/config-center-server /etc/config-center/config-server.yaml
```

### Systemd Service

```
[Unit]
Description=Config Center Server
After=network.target

[Service]
Type=simple
User=config-center
ExecStart=/usr/local/bin/config-center-server /etc/config-center/config-server.yaml
Restart=on-failure
RestartSec=5

[Install]
WantedBy=multi-user.target
```

## Production Deployment (MySQL)

For production environments requiring high availability.

### Prerequisites (additional)

- MySQL 8.0+ with replication setup
- MySQL user with CREATE TABLE permissions

### MySQL Setup

```sql
CREATE DATABASE config_center;
CREATE USER 'config_center'@'%' IDENTIFIED BY 'strong-password';
GRANT ALL PRIVILEGES ON config_center.* TO 'config_center'@'%';
FLUSH PRIVILEGES;
```

### Configuration

```yaml
storage:
  backend: mysql
  mysql:
    host: "mysql-primary.internal"
    port: 3306
    user: "config_center"
    password: "strong-password"
    database: "config_center"
    pool_size: 20
    timeout_seconds: 30
```

The server will auto-create all required tables on first startup.

### High Availability

For read-heavy workloads:

1. Set up MySQL replication (primary → replicas)
2. Deploy multiple Config Center server instances
3. Point clients to different instances for read load balancing
4. Writes go through the primary instance (MySQL primary)

### Security Considerations

1. **Change the JWT secret**: Use a strong random string (`openssl rand -hex 32`)
2. **Use HTTPS**: Place behind nginx/HAProxy with TLS termination
3. **Firewall**: Restrict ports 8080/9090 to trusted networks only
4. **Database credentials**: Use environment variables or secrets manager
5. **Regular backups**: Backup MySQL database or SQLite file

### Monitoring

- Health endpoint: `GET /health`
- Check HTTP/gRPC port availability
- Monitor database connection pool
- Log file: configured via `logging.file`
- Metrics could be exported via Prometheus endpoint (future)

### Scaling Notes

- **Write scaling**: Config write volume is inherently low (human-triggered publishes). Single MySQL primary is sufficient for most use cases.
- **Read scaling**: Read path scales horizontally. Deploy more server instances pointed at MySQL replicas.
- **Connection limits**: Each client SDK maintains 1 persistent connection (gRPC streaming or long-polling). Plan server capacity accordingly.
