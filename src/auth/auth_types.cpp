#include "config-center/auth/auth_types.h"
#include "config-center/storage/config_store.h"
#include "config-center/storage/data_types.h"

#include <jwt-cpp/jwt.h>
#include <openssl/evp.h>
#include <sstream>

namespace config_center {

// JWT Manager
JwtManager::JwtManager(const std::string& secret, int expiryHours)
    : secret_(secret), expiryHours_(expiryHours) {
}

std::string JwtManager::generateToken(const std::string& username, const std::string& role) {
    auto now = std::chrono::system_clock::now();
    auto token = jwt::create()
                     .set_issuer("config-center")
                     .set_subject(username)
                     .set_payload_claim("role", jwt::claim(role))
                     .set_issued_at(now)
                     .set_expires_at(now + std::chrono::hours(expiryHours_))
                     .sign(jwt::algorithm::hs256{secret_});
    return token;
}

bool JwtManager::validateToken(const std::string& token, std::string& username, std::string& role) {
    try {
        auto decoded = jwt::decode(token);
        auto verifier = jwt::verify()
                            .allow_algorithm(jwt::algorithm::hs256{secret_})
                            .with_issuer("config-center");
        verifier.verify(decoded);

        username = decoded.get_subject();
        role = decoded.get_payload_claim("role").as_string();
        return true;
    } catch (...) {
        return false;
    }
}

bool JwtManager::isExpired(const std::string& token) {
    try {
        auto decoded = jwt::decode(token);
        auto exp = decoded.get_expires_at();
        auto now = std::chrono::system_clock::now();
        return now > exp;
    } catch (...) {
        return true;
    }
}

// User Manager
UserManager::UserManager(std::shared_ptr<ConfigStore> store)
    : store_(std::move(store)) {
}

std::string UserManager::hashPassword(const std::string& password) {
    std::string salt = "config-center-salt";
    std::string salted = salt + password;

    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hashLen = 0;

    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr);
    EVP_DigestUpdate(ctx, salted.c_str(), salted.size());
    EVP_DigestFinal_ex(ctx, hash, &hashLen);
    EVP_MD_CTX_free(ctx);

    std::stringstream ss;
    for (unsigned int i = 0; i < hashLen; i++) {
        ss << std::hex << static_cast<int>(hash[i]);
    }
    return ss.str();
}

bool UserManager::verifyPassword(const std::string& password, const std::string& hash) {
    return hashPassword(password) == hash;
}

bool UserManager::createUser(const std::string& username, const std::string& password, const std::string& role) {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::gmtime(&time), "%Y-%m-%dT%H:%M:%SZ");

    User user;
    user.username = username;
    user.password_hash = hashPassword(password);
    user.role = role;
    user.created_at = ss.str();
    return store_->createUser(user);
}

bool UserManager::authenticate(const std::string& username, const std::string& password) {
    auto user = store_->getUserByUsername(username);
    if (!user) return false;
    return verifyPassword(password, user->password_hash);
}

std::string UserManager::listUsers() {
    return "";
}

// Auth Middleware
AuthMiddleware::AuthMiddleware(std::shared_ptr<JwtManager> jwt)
    : jwt_(std::move(jwt)) {
}

bool AuthMiddleware::validateRequest(const std::string& authHeader, std::string& username, std::string& role) {
    if (authHeader.empty()) return false;

    const std::string bearerPrefix = "Bearer ";
    if (authHeader.length() < bearerPrefix.length()) return false;
    if (authHeader.substr(0, bearerPrefix.length()) != bearerPrefix) return false;

    std::string token = authHeader.substr(bearerPrefix.length());
    return jwt_->validateToken(token, username, role);
}

bool AuthMiddleware::hasPermission(const std::string& role, const std::string& requiredRole) {
    if (role == "admin") return true;
    if (role == "operator" && requiredRole == "viewer") return true;
    if (role == requiredRole) return true;
    return false;
}

} // namespace config_center
