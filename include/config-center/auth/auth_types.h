#pragma once

#include <string>
#include <memory>

namespace config_center {

class ConfigStore;

class JwtManager {
public:
    explicit JwtManager(const std::string& secret, int expiryHours = 24);

    std::string generateToken(const std::string& username, const std::string& role);
    bool validateToken(const std::string& token, std::string& username, std::string& role);
    bool isExpired(const std::string& token);

private:
    std::string secret_;
    int expiryHours_;
};

class UserManager {
public:
    explicit UserManager(std::shared_ptr<ConfigStore> store);

    bool createUser(const std::string& username, const std::string& password, const std::string& role);
    bool authenticate(const std::string& username, const std::string& password);
    std::string listUsers();

private:
    std::shared_ptr<ConfigStore> store_;
    std::string hashPassword(const std::string& password);
    bool verifyPassword(const std::string& password, const std::string& hash);
};

class AuthMiddleware {
public:
    explicit AuthMiddleware(std::shared_ptr<JwtManager> jwt);

    bool validateRequest(const std::string& authHeader, std::string& username, std::string& role);
    bool hasPermission(const std::string& role, const std::string& requiredRole);

private:
    std::shared_ptr<JwtManager> jwt_;
};

} // namespace config_center
