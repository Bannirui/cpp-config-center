#include <gtest/gtest.h>

#include "config-center/storage/sqlite_config_store.h"
#include "config-center/storage/data_types.h"

using namespace config_center;

class SqliteStorageTest : public ::testing::Test {
protected:
    void SetUp() override {
        store_ = std::make_unique<SqliteConfigStore>();
        store_->initialize(":memory:");
        setupTestData();
    }

    void setupTestData() {
        Application app{"test-app", "Test App", std::nullopt, "2024-01-01T00:00:00Z"};
        store_->createApplication(app);

        Namespace ns{"test-app/DEV/default/application", "test-app", "DEV", "default", "application",
                      "private", std::nullopt, false, "2024-01-01T00:00:00Z"};
        store_->createNamespace(ns);

        store_->createConfig("test-app/DEV/default/application", "timeout", "30", "test-user");
    }

    std::unique_ptr<SqliteConfigStore> store_;
};

TEST_F(SqliteStorageTest, CreateAndGetConfig) {
    auto config = store_->getConfig("test-app/DEV/default/application", "timeout", false);
    ASSERT_TRUE(config.has_value());
    EXPECT_EQ(config->key, "timeout");
    EXPECT_EQ(config->value, "30");
    EXPECT_EQ(config->version, 1);
}

TEST_F(SqliteStorageTest, UpdateConfigCreatesNewVersion) {
    store_->updateConfig("test-app/DEV/default/application", "timeout", "60", "test-user", true);
    auto config = store_->getConfig("test-app/DEV/default/application", "timeout", false);
    ASSERT_TRUE(config.has_value());
    EXPECT_EQ(config->version, 2);
    EXPECT_EQ(config->value, "60");
}

TEST_F(SqliteStorageTest, GetConfigVersions) {
    store_->updateConfig("test-app/DEV/default/application", "timeout", "60", "test-user", true);
    auto versions = store_->getConfigVersions("test-app/DEV/default/application", "timeout");
    EXPECT_EQ(versions.size(), 2);
}

TEST_F(SqliteStorageTest, RollbackConfig) {
    store_->updateConfig("test-app/DEV/default/application", "timeout", "60", "test-user", true);
    store_->rollbackConfig("test-app/DEV/default/application", "timeout", 1, "test-user");

    auto config = store_->getConfig("test-app/DEV/default/application", "timeout", false);
    ASSERT_TRUE(config.has_value());
    EXPECT_EQ(config->value, "30");
}

TEST_F(SqliteStorageTest, ChangeHistory) {
    ChangeLogQuery query;
    query.namespace_id = "test-app/DEV/default/application";
    auto logs = store_->getChangeHistory(query);
    EXPECT_EQ(logs.size(), 1);
    EXPECT_EQ(logs[0].operation, "create");
}

TEST_F(SqliteStorageTest, ListApplications) {
    auto apps = store_->listApplications();
    EXPECT_EQ(apps.size(), 1);
}

TEST_F(SqliteStorageTest, CreateUser) {
    User user{0, "admin", "hash123", "admin", "2024-01-01T00:00:00Z"};
    store_->createUser(user);
    auto fetched = store_->getUserByUsername("admin");
    ASSERT_TRUE(fetched.has_value());
    EXPECT_EQ(fetched->role, "admin");
}

TEST_F(SqliteStorageTest, TransactionalPublish) {
    Release release;
    bool ok = store_->publishRelease("test-app/DEV/default/application", "Test release", "test-user", release);
    EXPECT_TRUE(ok);
    EXPECT_EQ(release.release_version, 1);
}
