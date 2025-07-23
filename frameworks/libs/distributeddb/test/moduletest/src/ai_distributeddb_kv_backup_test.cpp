/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "gtest/gtest.h"
#include "backup_manager.h"
#include "mock_dbstore.h"
#include "mock_kvdb_service.h"
#include "store_util.h"
#include <fstream>
#include <filesystem>
#include <vector>
#include <string>

using namespace testing;
using namespace OHOS::DistributedKv;
namespace fs = std::filesystem;

// 测试临时目录管理
class TempDirManager {
public:
    TempDirManager() {
        baseDir = fs::temp_directory_path() / "backup_test";
        fs::create_directories(baseDir);
    }
    
    ~TempDirManager() {
        if (fs::exists(baseDir)) {
            fs::remove_all(baseDir);
        }
    }
    
    std::string GetBaseDir() const {
        return baseDir.string();
    }
    
    void CreateFile(const std::string &relativePath, const std::string &content = "") {
        std::string fullPath = baseDir.string() + relativePath;
        fs::create_directories(fs::path(fullPath).parent_path());
        std::ofstream ofs(fullPath);
        ofs << content;
    }
    
    bool FileExists(const std::string &relativePath) const {
        return fs::exists(baseDir.string() + relativePath);
    }
    
    void RemoveFile(const std::string &relativePath) {
        std::string fullPath = baseDir.string() + relativePath;
        if (fs::exists(fullPath)) {
            fs::remove(fullPath);
        }
    }

private:
    fs::path baseDir;
};

// 测试用例基类
class BackupManagerTest : public Test {
protected:
    void SetUp() override {
        tempDir = std::make_unique<TempDirManager>();
        BackupManager::GetInstance().Init(tempDir->GetBaseDir());
        mockDBStore = std::make_shared<MockDBStore>();
    }
    
    void TearDown() override {
        mockDBStore.reset();
        tempDir.reset();
    }
    
    std::unique_ptr<TempDirManager> tempDir;
    std::shared_ptr<MockDBStore> mockDBStore;
};

// 测试单例模式
TEST_F(BackupManagerTest, GetInstance) {
    BackupManager &instance1 = BackupManager::GetInstance();
    BackupManager &instance2 = BackupManager::GetInstance();
    EXPECT_EQ(&instance1, &instance2);
}

// 测试初始化函数
TEST_F(BackupManagerTest, Init) {
    // 创建残留文件
    std::string storeId = "test_store";
    std::string backupPath = "/kvdb/backup/" + storeId + "/test.bak.bk";
    tempDir->CreateFile(backupPath);
    
    // 执行初始化
    BackupManager::GetInstance().Init(tempDir->GetBaseDir());
    
    // 验证残留文件被清理
    EXPECT_FALSE(tempDir->FileExists(backupPath));
}

// 测试Prepare函数
TEST_F(BackupManagerTest, Prepare) {
    std::string storeId = "prepare_test";
    BackupManager::GetInstance().Prepare(tempDir->GetBaseDir(), storeId);
    
    // 验证目录和文件创建
    EXPECT_TRUE(tempDir->FileExists("/kvdb/backup/" + storeId + "/autoBackup.bak"));
}

// 测试KeepData函数（创建新文件）
TEST_F(BackupManagerTest, KeepData_Create) {
    std::string testFile = tempDir->GetBaseDir() + "/test.keep";
    BackupManager::GetInstance().KeepData(testFile, true);
    EXPECT_TRUE(fs::exists(testFile + ".bk"));
}

// 测试KeepData函数（重命名现有文件）
TEST_F(BackupManagerTest, KeepData_Rename) {
    std::string testFile = tempDir->GetBaseDir() + "/test.keep";
    std::ofstream ofs(testFile);
    ofs << "test";
    ofs.close();
    
    BackupManager::GetInstance().KeepData(testFile, false);
    EXPECT_FALSE(fs::exists(testFile));
    EXPECT_TRUE(fs::exists(testFile + ".bk"));
}

// 测试RollBackData函数（创建场景）
TEST_F(BackupManagerTest, RollBackData_Create) {
    std::string testFile = tempDir->GetBaseDir() + "/test.rollback";
    BackupManager::GetInstance().KeepData(testFile, true);
    BackupManager::GetInstance().RollBackData(testFile, true);
    
    EXPECT_FALSE(fs::exists(testFile));
    EXPECT_FALSE(fs::exists(testFile + ".bk"));
}

// 测试RollBackData函数（重命名场景）
TEST_F(BackupManagerTest, RollBackData_Rename) {
    std::string testFile = tempDir->GetBaseDir() + "/test.rollback";
    std::ofstream ofs(testFile);
    ofs << "test";
    ofs.close();
    
    BackupManager::GetInstance().KeepData(testFile, false);
    BackupManager::GetInstance().RollBackData(testFile, false);
    
    EXPECT_TRUE(fs::exists(testFile));
    EXPECT_FALSE(fs::exists(testFile + ".bk"));
}

// 测试CleanTmpData函数
TEST_F(BackupManagerTest, CleanTmpData) {
    std::string testFile = tempDir->GetBaseDir() + "/test.clean";
    std::ofstream ofs(testFile + ".bk");
    ofs << "tmp";
    ofs.close();
    
    BackupManager::GetInstance().CleanTmpData(testFile);
    EXPECT_FALSE(fs::exists(testFile + ".bk"));
}

// 测试Backup函数（无效参数）
TEST_F(BackupManagerTest, Backup_InvalidArgs) {
    BackupInfo info;
    info.name = "";  // 无效名称
    info.baseDir = tempDir->GetBaseDir();
    info.storeId = "invalid_arg";
    
    Status status = BackupManager::GetInstance().Backup(info, mockDBStore);
    EXPECT_EQ(status, INVALID_ARGUMENT);
}

// 测试Backup函数（备份成功）
TEST_F(BackupManagerTest, Backup_Success) {
    BackupInfo info;
    info.name = "success_test";
    info.baseDir = tempDir->GetBaseDir();
    info.storeId = "backup_success";
    info.isCheckIntegrity = false;
    
    EXPECT_CALL(*mockDBStore, Export(_, _)).WillOnce(Return(DistributedDB::DBStatus::OK));
    
    Status status = BackupManager::GetInstance().Backup(info, mockDBStore);
    EXPECT_EQ(status, SUCCESS);
    EXPECT_TRUE(tempDir->FileExists("/kvdb/backup/" + info.storeId + "/" + info.name + ".bak"));
}

// 测试Backup函数（备份失败）
TEST_F(BackupManagerTest, Backup_Fail) {
    BackupInfo info;
    info.name = "fail_test";
    info.baseDir = tempDir->GetBaseDir();
    info.storeId = "backup_fail";
    
    EXPECT_CALL(*mockDBStore, Export(_, _)).WillOnce(Return(DistributedDB::DBStatus::ERROR));
    
    Status status = BackupManager::GetInstance().Backup(info, mockDBStore);
    EXPECT_NE(status, SUCCESS);
}

// 测试Backup函数（超过最大备份数）
TEST_F(BackupManagerTest, Backup_ExceedMax) {
    std::string storeId = "exceed_max";
    BackupInfo info;
    info.baseDir = tempDir->GetBaseDir();
    info.storeId = storeId;
    info.isCheckIntegrity = false;
    
    // 创建最大数量的备份文件
    for (int i = 0; i < 5; ++i) {  // 假设MAX_BACKUP_NUM为5
        info.name = "backup_" + std::to_string(i);
        EXPECT_CALL(*mockDBStore, Export(_, _)).WillRepeatedly(Return(DistributedDB::DBStatus::OK));
        BackupManager::GetInstance().Backup(info, mockDBStore);
    }
    
    // 尝试创建第6个备份
    info.name = "backup_6";
    Status status = BackupManager::GetInstance().Backup(info, mockDBStore);
    EXPECT_EQ(status, ERROR);
}

// 测试GetBackupFileInfo函数（存在指定文件）
TEST_F(BackupManagerTest, GetBackupFileInfo_Specific) {
    std::string storeId = "file_info";
    std::string fileName = "specific.bak";
    tempDir->CreateFile("/kvdb/backup/" + storeId + "/" + fileName);
    
    auto fileInfo = BackupManager::GetInstance().GetBackupFileInfo(
        "specific", tempDir->GetBaseDir(), storeId);
    EXPECT_EQ(fileInfo.name, fileName);
}

// 测试GetBackupFileInfo函数（返回最新文件）
TEST_F(BackupManagerTest, GetBackupFileInfo_Latest) {
    std::string storeId = "latest_test";
    std::string basePath = "/kvdb/backup/" + storeId + "/";
    
    // 创建两个不同时间的文件
    tempDir->CreateFile(basePath + "old.bak");
    std::this_thread::sleep_for(std::chrono::seconds(1));
    tempDir->CreateFile(basePath + "new.bak");
    
    auto fileInfo = BackupManager::GetInstance().GetBackupFileInfo(
        "", tempDir->GetBaseDir(), storeId);
    EXPECT_EQ(fileInfo.name, "new.bak");
}

// 测试Restore函数（无效参数）
TEST_F(BackupManagerTest, Restore_InvalidArgs) {
    BackupInfo info;
    info.storeId = "";  // 无效storeId
    
    Status status = BackupManager::GetInstance().Restore(info, mockDBStore);
    EXPECT_EQ(status, INVALID_ARGUMENT);
}

// 测试Restore函数（文件不存在）
TEST_F(BackupManagerTest, Restore_FileNotFound) {
    BackupInfo info;
    info.storeId = "not_found";
    info.baseDir = tempDir->GetBaseDir();
    info.name = "nonexistent";
    
    Status status = BackupManager::GetInstance().Restore(info, mockDBStore);
    EXPECT_EQ(status, INVALID_ARGUMENT);
}

// 测试Restore函数（恢复成功）
TEST_F(BackupManagerTest, Restore_Success) {
    std::string storeId = "restore_success";
    std::string backupName = "restore_test";
    tempDir->CreateFile("/kvdb/backup/" + storeId + "/" + backupName + ".bak");
    
    BackupInfo info;
    info.storeId = storeId;
    info.baseDir = tempDir->GetBaseDir();
    info.name = backupName;
    
    EXPECT_CALL(*mockDBStore, Import(_, _, _)).WillOnce(Return(DistributedDB::DBStatus::OK));
    
    Status status = BackupManager::GetInstance().Restore(info, mockDBStore);
    EXPECT_EQ(status, SUCCESS);
}

// 测试GetRestorePassword函数（自动备份）
TEST_F(BackupManagerTest, GetRestorePassword_Auto) {
    BackupInfo info;
    info.storeId = "auto_pwd";
    info.baseDir = tempDir->GetBaseDir();
    
    // 模拟服务端返回密码
    auto mockService = std::make_shared<MockKVDBService>();
    KVDBServiceClient::SetInstance(mockService);
    EXPECT_CALL(*mockService, GetBackupPassword(_, _, _, _, _)).WillOnce(DoAll(
        SetArgReferee<3>(std::vector<std::vector<uint8_t>>{{1,2,3,4}}),
        Return(Status::SUCCESS)
    ));
    
    auto password = BackupManager::GetInstance().GetRestorePassword("autoBackup.bak", info);
    EXPECT_TRUE(password.IsValid());
    
    KVDBServiceClient::SetInstance(nullptr);
}

// 测试GetRestorePassword函数（手动备份）
TEST_F(BackupManagerTest, GetRestorePassword_Manual) {
    std::string storeId = "manual_pwd";
    std::string keyName = "Prefix_backup_" + storeId + "_manual";
    tempDir->CreateFile("/key/" + keyName + ".key");
    
    BackupInfo info;
    info.storeId = storeId;
    info.baseDir = tempDir->GetBaseDir();
    
    auto password = BackupManager::GetInstance().GetRestorePassword("manual.bak", info);
    EXPECT_TRUE(password.IsValid());
}

// 测试GetSecretKeyFromService函数（成功）
TEST_F(BackupManagerTest, GetSecretKeyFromService_Success) {
    auto mockService = std::make_shared<MockKVDBService>();
    KVDBServiceClient::SetInstance(mockService);
    
    EXPECT_CALL(*mockService, GetBackupPassword(_, _, _, _, _)).WillOnce(DoAll(
        SetArgReferee<3>(std::vector<std::vector<uint8_t>>{{5,6,7,8}}),
        Return(Status::SUCCESS)
    ));
    
    std::vector<std::vector<uint8_t>> keys;
    Status status = BackupManager::GetInstance().GetSecretKeyFromService(
        {"app1"}, {"store1"}, keys, 0);
    
    EXPECT_EQ(status, SUCCESS);
    EXPECT_EQ(keys.size(), 1);
    
    KVDBServiceClient::SetInstance(nullptr);
}

// 测试GetSecretKeyFromService函数（失败）
TEST_F(BackupManagerTest, GetSecretKeyFromService_Fail) {
    auto mockService = std::make_shared<MockKVDBService>();
    KVDBServiceClient::SetInstance(mockService);
    
    EXPECT_CALL(*mockService, GetBackupPassword(_, _, _, _, _)).WillOnce(Return(Status::ERROR));
    
    std::vector<std::vector<uint8_t>> keys;
    Status status = BackupManager::GetInstance().GetSecretKeyFromService(
        {"app1"}, {"store1"}, keys, 0);
    
    EXPECT_NE(status, SUCCESS);
    EXPECT_TRUE(keys.empty());
    
    KVDBServiceClient::SetInstance(nullptr);
}

// 测试ImportWithSecretKeyFromService函数（成功）
TEST_F(BackupManagerTest, ImportWithSecretKeyFromService_Success) {
    auto mockService = std::make_shared<MockKVDBService>();
    KVDBServiceClient::SetInstance(mockService);
    
    EXPECT_CALL(*mockService, GetBackupPassword(_, _, _, _, _)).WillOnce(DoAll(
        SetArgReferee<3>(std::vector<std::vector<uint8_t>>{{9,10,11,12}}),
        Return(Status::SUCCESS)
    ));
    
    EXPECT_CALL(*mockDBStore, Import(_, _, _)).WillOnce(Return(DistributedDB::DBStatus::OK));
    
    BackupInfo info;
    info.appId = "import_app";
    info.storeId = "import_store";
    std::string fullName = tempDir->GetBaseDir() + "/test.import";
    
    Status status = BackupManager::GetInstance().ImportWithSecretKeyFromService(
        info, mockDBStore, fullName, false);
    EXPECT_EQ(status, SUCCESS);
    
    KVDBServiceClient::SetInstance(nullptr);
}

// 测试ImportWithSecretKeyFromService函数（失败）
TEST_F(BackupManagerTest, ImportWithSecretKeyFromService_Fail) {
    auto mockService = std::make_shared<MockKVDBService>();
    KVDBServiceClient::SetInstance(mockService);
    
    EXPECT_CALL(*mockService, GetBackupPassword(_, _, _, _, _)).WillOnce(Return(Status::ERROR));
    
    BackupInfo info;
    info.appId = "import_app";
    info.storeId = "import_store";
    std::string fullName = tempDir->GetBaseDir() + "/test.import";
    
    Status status = BackupManager::GetInstance().ImportWithSecretKeyFromService(
        info, mockDBStore, fullName, false);
    EXPECT_NE(status, SUCCESS);
    
    KVDBServiceClient::SetInstance(nullptr);
}

// 测试DeleteBackup函数（无效参数）
TEST_F(BackupManagerTest, DeleteBackup_InvalidArgs) {
    std::map<std::string, Status> deleteList;
    Status status = BackupManager::GetInstance().DeleteBackup(deleteList, "", "");
    EXPECT_EQ(status, INVALID_ARGUMENT);
}

// 测试DeleteBackup函数（成功删除）
TEST_F(BackupManagerTest, DeleteBackup_Success) {
    std::string storeId = "delete_test";
    std::string backupName = "to_delete";
    tempDir->CreateFile("/kvdb/backup/" + storeId + "/" + backupName + ".bak");
    tempDir->CreateFile("/key/Prefix_backup_" + storeId + "_" + backupName + ".key");
    
    std::map<std::string, Status> deleteList{{backupName, Status::SUCCESS}};
    Status status = BackupManager::GetInstance().DeleteBackup(deleteList, tempDir->GetBaseDir(), storeId);
    
    EXPECT_EQ(status, SUCCESS);
    EXPECT_EQ(deleteList[backupName], SUCCESS);
    EXPECT_FALSE(tempDir->FileExists("/kvdb/backup/" + storeId + "/" + backupName + ".bak"));
}


// 测试GetRestorePassword函数（成功）   
TEST_F(BackupManagerTest, GetRestorePassword_Success) {
    std::string storeId = "restore_test";
    std::string backupName = "to_restore";
    tempDir->CreateFile("/kvdb/backup/" + storeId + "/" + backupName + ".bak"); 

    std::string password = "123456";
    std::string keyName = "Prefix_backup_" + storeId + "_" + backupName + ".key";
    tempDir->CreateFile("/key/" + keyName, password);

    std::string result = BackupManager::GetInstance().GetRestorePassword(tempDir->GetBaseDir(), storeId, backupName);
    EXPECT_EQ(result, password);
}

// 测试GetRestorePassword函数（失败） 
TEST_F(BackupManagerTest, GetRestorePassword_Fail) {
    std::string storeId = "restore_test";
    std::string backupName = "to_restore";
    tempDir->CreateFile("/kvdb/backup/" + storeId + "/" + backupName + ".bak"); 

    std::string password = "123456";
    std::string keyName = "Prefix_backup_" + storeId + "_" + backupName + ".key";
    tempDir->CreateFile("/key/" + keyName, password);

    std::string result = BackupManager::GetInstance().GetRestorePassword(tempDir->GetBaseDir(), storeId, "not_exist");
    EXPECT_EQ(result, "");
    
}

/**
 * @tc.name: BackupManagerInitTest001
 * @tc.desc: Verify BackupManager initialization with residue files
 * @tc.type: FUNC
 * @tc.require: AR000D4876
 * @tc.author: zhangqiquan
 */
HWTEST_F(BackupManagerTest, BackupManagerInitTest001, TestSize.Level1)
{
    // Setup residue files
    StoreUtil::AddMockFile("/data/test/kvdb/backup/store1/backup1.bak.bk", 100);
    StoreUtil::AddMockFile("/data/test/kvdb/backup/store1/backup1.bak", 200);
    StoreUtil::AddMockFile("/data/test/key/Prefix_backup_store1_backup1.key.bk", 50);
    StoreUtil::AddMockFile("/data/test/key/Prefix_backup_store1_backup1.key", 100);

    BackupManager::GetInstance().Init("/data/test");

    // Verify residue files are cleaned up
    EXPECT_FALSE(StoreUtil::FileExists("/data/test/kvdb/backup/store1/backup1.bak.bk"));
    EXPECT_FALSE(StoreUtil::FileExists("/data/test/key/Prefix_backup_store1_backup1.key.bk"));
    EXPECT_TRUE(StoreUtil::FileExists("/data/test/kvdb/backup/store1/backup1.bak"));
    EXPECT_TRUE(StoreUtil::FileExists("/data/test/key/Prefix_backup_store1_backup1.key"));
}

/**
 * @tc.name: BackupManagerInitTest002
 * @tc.desc: Verify BackupManager initialization with auto backup files
 * @tc.type: FUNC
 * @tc.require: AR000D4876
 * @tc.author: zhangqiquan
 */
HWTEST_F(BackupManagerTest, BackupManagerInitTest002, TestSize.Level1)
{
    // Setup auto backup files
    StoreUtil::AddMockFile("/data/test/kvdb/backup/store1/autoBackup.bak.bk", 100);
    StoreUtil::AddMockFile("/data/test/kvdb/backup/store1/autoBackup.bak", 200);

    BackupManager::GetInstance().Init("/data/test");

    // Verify auto backup tmp file is not cleaned
    EXPECT_TRUE(StoreUtil::FileExists("/data/test/kvdb/backup/store1/autoBackup.bak.bk"));
    EXPECT_TRUE(StoreUtil::FileExists("/data/test/kvdb/backup/store1/autoBackup.bak"));
}

/**
 * @tc.name: BackupManagerPrepareTest001
 * @tc.desc: Verify BackupManager prepare creates necessary directories and files
 * @tc.type: FUNC
 * @tc.require: AR000D4876
 * @tc.author: zhangqiquan
 */
HWTEST_F(BackupManagerTest, BackupManagerPrepareTest001, TestSize.Level1)
{
    BackupManager::GetInstance().Prepare("/data/test", "store1");

    EXPECT_TRUE(StoreUtil::DirExists("/data/test/kvdb/backup/store1"));
    EXPECT_TRUE(StoreUtil::FileExists("/data/test/kvdb/backup/store1/autoBackup.bak"));
}

/**
 * @tc.name: BackupManagerBackupTest001
 * @tc.desc: Verify successful backup operation
 * @tc.type: FUNC
 * @tc.require: AR000D4876
 * @tc.author: zhangqiquan
 */
HWTEST_F(BackupManagerTest, BackupManagerBackupTest001, TestSize.Level1)
{
    auto mockDB = std::make_shared<MockDBStore>();
    EXPECT_CALL(*mockDB, Export(_, _)).WillOnce(Return(DistributedDB::DBStatus::OK));

    BackupInfo info;
    info.name = "testBackup";
    info.baseDir = "/data/test";
    info.storeId = "store1";
    info.isCheckIntegrity = false;

    auto status = BackupManager::GetInstance().Backup(info, mockDB);
    EXPECT_EQ(status, Status::SUCCESS);

    EXPECT_TRUE(StoreUtil::FileExists("/data/test/kvdb/backup/store1/testBackup.bak"));
    EXPECT_FALSE(StoreUtil::FileExists("/data/test/kvdb/backup/store1/testBackup.bak.bk"));
}

/**
 * @tc.name: BackupManagerBackupTest002
 * @tc.desc: Verify backup operation with integrity check
 * @tc.type: FUNC
 * @tc.require: AR000D4876
 * @tc.author: zhangqiquan
 */
HWTEST_F(BackupManagerTest, BackupManagerBackupTest002, TestSize.Level1)
{
    auto mockDB = std::make_shared<MockDBStore>();
    EXPECT_CALL(*mockDB, CheckIntegrity()).WillOnce(Return(DistributedDB::DBStatus::OK));
    EXPECT_CALL(*mockDB, Export(_, _)).WillOnce(Return(DistributedDB::DBStatus::OK));

    BackupInfo info;
    info.name = "testBackup";
    info.baseDir = "/data/test";
    info.storeId = "store1";
    info.isCheckIntegrity = true;

    auto status = BackupManager::GetInstance().Backup(info, mockDB);
    EXPECT_EQ(status, Status::SUCCESS);
}

/**
 * @tc.name: BackupManagerBackupTest003
 * @tc.desc: Verify backup operation fails when DB integrity check fails
 * @tc.type: FUNC
 * @tc.require: AR000D4876
 * @tc.author: zhangqiquan
 */
HWTEST_F(BackupManagerTest, BackupManagerBackupTest003, TestSize.Level1)
{
    auto mockDB = std::make_shared<MockDBStore>();
    EXPECT_CALL(*mockDB, CheckIntegrity()).WillOnce(Return(DistributedDB::DBStatus::DB_ERROR));

    BackupInfo info;
    info.name = "testBackup";
    info.baseDir = "/data/test";
    info.storeId = "store1";
    info.isCheckIntegrity = true;

    auto status = BackupManager::GetInstance().Backup(info, mockDB);
    EXPECT_EQ(status, Status::ERROR);
}

/**
 * @tc.name: BackupManagerBackupTest004
 * @tc.desc: Verify backup operation with encryption
 * @tc.type: FUNC
 * @tc.require: AR000D4876
 * @tc.author: zhangqiquan
 */
HWTEST_F(BackupManagerTest, BackupManagerBackupTest004, TestSize.Level1)
{
    auto mockDB = std::make_shared<MockDBStore>();
    EXPECT_CALL(*mockDB, Export(_, _)).WillOnce(Return(DistributedDB::DBStatus::OK));

    // Setup password
    std::vector<uint8_t> password = {1, 2, 3, 4};
    SecurityManager::GetInstance().SaveDBPassword("store1", "/data/test", password);

    BackupInfo info;
    info.name = "testBackup";
    info.baseDir = "/data/test";
    info.storeId = "store1";
    info.isCheckIntegrity = false;

    auto status = BackupManager::GetInstance().Backup(info, mockDB);
    EXPECT_EQ(status, Status::SUCCESS);

    EXPECT_TRUE(StoreUtil::FileExists("/data/test/kvdb/backup/store1/testBackup.bak"));
    EXPECT_TRUE(StoreUtil::FileExists("/data/test/key/Prefix_backup_store1_testBackup.key"));
}

/**
 * @tc.name: BackupManagerBackupTest005
 * @tc.desc: Verify backup operation rollback when export fails
 * @tc.type: FUNC
 * @tc.require: AR000D4876
 * @tc.author: zhangqiquan
 */
HWTEST_F(BackupManagerTest, BackupManagerBackupTest005, TestSize.Level1)
{
    auto mockDB = std::make_shared<MockDBStore>();
    EXPECT_CALL(*mockDB, Export(_, _)).WillOnce(Return(DistributedDB::DBStatus::DB_ERROR));

    BackupInfo info;
    info.name = "testBackup";
    info.baseDir = "/data/test";
    info.storeId = "store1";
    info.isCheckIntegrity = false;

    auto status = BackupManager::GetInstance().Backup(info, mockDB);
    EXPECT_EQ(status, Status::ERROR);

    EXPECT_FALSE(StoreUtil::FileExists("/data/test/kvdb/backup/store1/testBackup.bak"));
    EXPECT_FALSE(StoreUtil::FileExists("/data/test/kvdb/backup/store1/testBackup.bak.bk"));
}

/**
 * @tc.name: BackupManagerRestoreTest001
 * @tc.desc: Verify successful restore operation
 * @tc.type: FUNC
 * @tc.require: AR000D4876
 * @tc.author: zhangqiquan
 */
HWTEST_F(BackupManagerTest, BackupManagerRestoreTest001, TestSize.Level1)
{
    auto mockDB = std::make_shared<MockDBStore>();
    EXPECT_CALL(*mockDB, Import(_, _, _)).WillOnce(Return(DistributedDB::DBStatus::OK));

    // Setup backup file
    StoreUtil::AddMockFile("/data/test/kvdb/backup/store1/testBackup.bak", 100);

    BackupInfo info;
    info.name = "testBackup";
    info.baseDir = "/data/test";
    info.storeId = "store1";
    info.isCheckIntegrity = false;

    auto status = BackupManager::GetInstance().Restore(info, mockDB);
    EXPECT_EQ(status, Status::SUCCESS);
}

/**
 * @tc.name: BackupManagerRestoreTest002
 * @tc.desc: Verify restore operation with integrity check
 * @tc.type: FUNC
 * @tc.require: AR000D4876
 * @tc.author: zhangqiquan
 */
HWTEST_F(BackupManagerTest, BackupManagerRestoreTest002, TestSize.Level1)
{
    auto mockDB = std::make_shared<MockDBStore>();
    EXPECT_CALL(*mockDB, Import(_, _, true)).WillOnce(Return(DistributedDB::DBStatus::OK));

    // Setup backup file
    StoreUtil::AddMockFile("/data/test/kvdb/backup/store1/testBackup.bak", 100);

    BackupInfo info;
    info.name = "testBackup";
    info.baseDir = "/data/test";
    info.storeId = "store1";
    info.isCheckIntegrity = true;

    auto status = BackupManager::GetInstance().Restore(info, mockDB);
    EXPECT_EQ(status, Status::SUCCESS);
}

/**
 * @tc.name: BackupManagerRestoreTest003
 * @tc.desc: Verify restore operation with encryption
 * @tc.type: FUNC
 * @tc.require: AR000D4876
 * @tc.author: zhangqiquan
 */
HWTEST_F(BackupManagerTest, BackupManagerRestoreTest003, TestSize.Level1)
{
    auto mockDB = std::make_shared<MockDBStore>();
    EXPECT_CALL(*mockDB, Import(_, _, _)).WillOnce(Return(DistributedDB::DBStatus::OK));

    // Setup backup file and key
    StoreUtil::AddMockFile("/data/test/kvdb/backup/store1/testBackup.bak", 100);
    StoreUtil::AddMockFile("/data/test/key/Prefix_backup_store1_testBackup.key", 100);

    // Setup password
    std::vector<uint8_t> password = {1, 2, 3, 4};
    SecurityManager::GetInstance().SaveDBPassword("Prefix_backup_store1_testBackup", "/data/test", password);

    BackupInfo info;
    info.name = "testBackup";
    info.baseDir = "/data/test";
    info.storeId = "store1";
    info.isCheckIntegrity = false;

    auto status = BackupManager::GetInstance().Restore(info, mockDB);
    EXPECT_EQ(status, Status::SUCCESS);
}

/**
 * @tc.name: BackupManagerRestoreTest004
 * @tc.desc: Verify restore operation fails when backup file doesn't exist
 * @tc.type: FUNC
 * @tc.require: AR000D4876
 * @tc.author: zhangqiquan
 */
HWTEST_F(BackupManagerTest, BackupManagerRestoreTest004, TestSize.Level1)
{
    auto mockDB = std::make_shared<MockDBStore>();

    BackupInfo info;
    info.name = "nonexistent";
    info.baseDir = "/data/test";
    info.storeId = "store1";
    info.isCheckIntegrity = false;

    auto status = BackupManager::GetInstance().Restore(info, mockDB);
    EXPECT_EQ(status, Status::INVALID_ARGUMENT);
}

/**
 * @tc.name: BackupManagerRestoreTest005
 * @tc.desc: Verify restore operation with auto backup
 * @tc.type: FUNC
 * @tc.require: AR000D4876
 * @tc.author: zhangqiquan
 */
HWTEST_F(BackupManagerTest, BackupManagerRestoreTest005, TestSize.Level1)
{
    auto mockDB = std::make_shared<MockDBStore>();
    EXPECT_CALL(*mockDB, Import(_, _, _)).WillOnce(Return(DistributedDB::DBStatus::OK));

    // Setup auto backup file
    StoreUtil::AddMockFile("/data/test/kvdb/backup/store1/autoBackup.bak", 100);

    // Setup mock service to return password
    std::vector<std::vector<uint8_t>> passwords = {{1, 2, 3, 4}};
    MockKVDBService::SetBackupPasswords(passwords);

    BackupInfo info;
    info.name = "";
    info.baseDir = "/data/test";
    info.storeId = "store1";
    info.isCheckIntegrity = false;
    info.appId = "testApp";
    info.subUser = 0;

    auto status = BackupManager::GetInstance().Restore(info, mockDB);
    EXPECT_EQ(status, Status::SUCCESS);
}

/**
 * @tc.name: BackupManagerRestoreTest006
 * @tc.desc: Verify restore operation with service secret key when password fails
 * @tc.type: FUNC
 * @tc.require: AR000D4876
 * @tc.author: zhangqiquan
 */
HWTEST_F(BackupManagerTest, BackupManagerRestoreTest006, TestSize.Level1)
{
    auto mockDB = std::make_shared<MockDBStore>();
    EXPECT_CALL(*mockDB, Import(_, _, _))
        .WillOnce(Return(DistributedDB::DBStatus::INVALID_FILE))
        .WillOnce(Return(DistributedDB::DBStatus::OK));

    // Setup backup file
    StoreUtil::AddMockFile("/data/test/kvdb/backup/store1/testBackup.bak", 100);

    // Setup mock service to return secret key
    std::vector<std::vector<uint8_t>> secretKeys = {{5, 6, 7, 8}};
    MockKVDBService::SetSecretKeys(secretKeys);

    BackupInfo info;
    info.name = "testBackup";
    info.baseDir = "/data/test";
    info.storeId = "store1";
    info.isCheckIntegrity = false;
    info.appId = "testApp";
    info.subUser = 0;
    info.encrypt = true;

    auto status = BackupManager::GetInstance().Restore(info, mockDB);
    EXPECT_EQ(status, Status::SUCCESS);
}

/**
 * @tc.name: BackupManagerDeleteBackupTest001
 * @tc.desc: Verify delete backup operation
 * @tc.type: FUNC
 * @tc.require: AR000D4876
 * @tc.author: zhangqiquan
 */
HWTEST_F(BackupManagerTest, BackupManagerDeleteBackupTest001, TestSize.Level1)
{
    // Setup backup files
    StoreUtil::AddMockFile("/data/test/kvdb/backup/store1/backup1.bak", 100);
    StoreUtil::AddMockFile("/data/test/kvdb/backup/store1/backup2.bak", 200);
    StoreUtil::AddMockFile("/data/test/key/Prefix_backup_store1_backup1.key", 50);

    std::map<std::string, Status> deleteList;
    deleteList["backup1"] = Status::ERROR;
    deleteList["backup2"] = Status::ERROR;

    auto status = BackupManager::GetInstance().DeleteBackup(deleteList, "/data/test", "store1");
    EXPECT_EQ(status, Status::SUCCESS);

    EXPECT_EQ(deleteList["backup1"], Status::SUCCESS);
    EXPECT_EQ(deleteList["backup2"], Status::SUCCESS);
    EXPECT_FALSE(StoreUtil::FileExists("/data/test/kvdb/backup/store1/backup1.bak"));
    EXPECT_FALSE(StoreUtil::FileExists("/data/test/kvdb/backup/store1/backup2.bak"));
    EXPECT_FALSE(StoreUtil::FileExists("/data/test/key/Prefix_backup_store1_backup1.key"));
}

/**
 * @tc.name: BackupManagerDeleteBackupTest002
 * @tc.desc: Verify delete auto backup is not allowed
 * @tc.type: FUNC
 * @tc.require: AR000D4876
 * @tc.author: zhangqiquan
 */
HWTEST_F(BackupManagerTest, BackupManagerDeleteBackupTest002, TestSize.Level1)
{
    // Setup backup files
    StoreUtil::AddMockFile("/data/test/kvdb/backup/store1/autoBackup.bak", 100);

    std::map<std::string, Status> deleteList;
    deleteList["autoBackup"] = Status::ERROR;

    auto status = BackupManager::GetInstance().DeleteBackup(deleteList, "/data/test", "store1");
    EXPECT_EQ(status, Status::SUCCESS);

    EXPECT_EQ(deleteList["autoBackup"], Status::INVALID_ARGUMENT);
    EXPECT_TRUE(StoreUtil::FileExists("/data/test/kvdb/backup/store1/autoBackup.bak"));
}

/**
 * @tc.name: BackupManagerDeleteBackupTest003
 * @tc.desc: Verify delete non-existent backup
 * @tc.type: FUNC
 * @tc.require: AR000D4876
 * @tc.author: zhangqiquan
 */
HWTEST_F(BackupManagerTest, BackupManagerDeleteBackupTest003, TestSize.Level1)
{
    std::map<std::string, Status> deleteList;
    deleteList["nonexistent"] = Status::ERROR;

    auto status = BackupManager::GetInstance().DeleteBackup(deleteList, "/data/test", "store1");
    EXPECT_EQ(status, Status::SUCCESS);

    EXPECT_EQ(deleteList["nonexistent"], Status::ERROR);
}

/**
 * @tc.name: BackupManagerGetBackupFileInfoTest001
 * @tc.desc: Verify get backup file info by name
 * @tc.type: FUNC
 * @tc.require: AR000D4876
 * @tc.author: zhangqiquan
 */
HWTEST_F(BackupManagerTest, BackupManagerGetBackupFileInfoTest001, TestSize.Level1)
{
    // Setup backup files
    StoreUtil::AddMockFile("/data/test/kvdb/backup/store1/backup1.bak", 100);
    StoreUtil::AddMockFile("/data/test/kvdb/backup/store1/backup2.bak", 200);

    auto fileInfo = BackupManager::GetInstance().GetBackupFileInfo("backup1", "/data/test", "store1");
    EXPECT_EQ(fileInfo.name, "backup1.bak");
    EXPECT_EQ(fileInfo.size, 100);
}

/**
 * @tc.name: BackupManagerGetBackupFileInfoTest002
 * @tc.desc: Verify get latest backup file info when name is empty
 * @tc.type: FUNC
 * @tc.require: AR000D4876
 * @tc.author: zhangqiquan
 */
HWTEST_F(BackupManagerTest, BackupManagerGetBackupFileInfoTest002, TestSize.Level1)
{
    // Setup backup files with different modify times
    StoreUtil::AddMockFile("/data/test/kvdb/backup/store1/backup1.bak", 100, 1000);
    StoreUtil::AddMockFile("/data/test/kvdb/backup/store1/backup2.bak", 200, 2000);

    auto fileInfo = BackupManager::GetInstance().GetBackupFileInfo("", "/data/test", "store1");
    EXPECT_EQ(fileInfo.name, "backup2.bak");
    EXPECT_EQ(fileInfo.size, 200);
}

/**
 * @tc.name: BackupManagerGetBackupFileInfoTest003
 * @tc.desc: Verify get backup file info when file doesn't exist
 * @tc.type: FUNC
 * @tc.require: AR000D4876
 * @tc.author: zhangqiquan
 */
HWTEST_F(BackupManagerTest, BackupManagerGetBackupFileInfoTest003, TestSize.Level1)
{
    auto fileInfo = BackupManager::GetInstance().GetBackupFileInfo("nonexistent", "/data/test", "store1");
    EXPECT_TRUE(fileInfo.name.empty());
}

/**
 * @tc.name: BackupManagerGetSecretKeyFromServiceTest001
 * @tc.desc: Verify get secret key from service successfully
 * @tc.type: FUNC
 * @tc.require: AR000D4876
 * @tc.author: zhangqiquan
 */
HWTEST_F(BackupManagerTest, BackupManagerGetSecretKeyFromServiceTest001, TestSize.Level1)
{
    // Setup mock service to return secret key
    std::vector<std::vector<uint8_t>> secretKeys = {{1, 2, 3, 4}};
    MockKVDBService::SetSecretKeys(secretKeys);

    std::vector<std::vector<uint8_t>> keys;
    auto status = BackupManager::GetInstance().GetSecretKeyFromService(
        {"testApp"}, {"store1"}, keys, 0);
    
    EXPECT_EQ(status, Status::SUCCESS);
    EXPECT_EQ(keys.size(), 1);
    EXPECT_EQ(keys[0], std::vector<uint8_t>({1, 2, 3, 4}));
}

/**
 * @tc.name: BackupManagerGetSecretKeyFromServiceTest002
 * @tc.desc: Verify get secret key from service fails when service unavailable
 * @tc.type: FUNC
 * @tc.require: AR000D4876
 * @tc.author: zhangqiquan
 */
HWTEST_F(BackupManagerTest, BackupManagerGetSecretKeyFromServiceTest002, TestSize.Level1)
{
    MockKVDBService::SetAvailable(false);

    std::vector<std::vector<uint8_t>> keys;
    auto status = BackupManager::GetInstance().GetSecretKeyFromService(
        {"testApp"}, {"store1"}, keys, 0);
    
    EXPECT_EQ(status, Status::SERVER_UNAVAILABLE);
    EXPECT_TRUE(keys.empty());
}

/**
 * @tc.name: BackupManagerGetSecretKeyFromServiceTest003
 * @tc.desc: Verify get secret key from service fails when no key returned
 * @tc.type: FUNC
 * @tc.require: AR000D4876
 * @tc.author: zhangqiquan
 */
HWTEST_F(BackupManagerTest, BackupManagerGetSecretKeyFromServiceTest003, TestSize.Level1)
{
    // Setup mock service to return empty keys
    MockKVDBService::SetSecretKeys({});

    std::vector<std::vector<uint8_t>> keys;
    auto status = BackupManager::GetInstance().GetSecretKeyFromService(
        {"testApp"}, {"store1"}, keys, 0);
    
    EXPECT_EQ(status, Status::ERROR);
    EXPECT_TRUE(keys.empty());
}

/**
 * @tc.name: BackupManagerImportWithSecretKeyFromServiceTest001
 * @tc.desc: Verify import with secret key from service successfully
 * @tc.type: FUNC
 * @tc.require: AR000D4876
 * @tc.author: zhangqiquan
 */
HWTEST_F(BackupManagerTest, BackupManagerImportWithSecretKeyFromServiceTest001, TestSize.Level1)
{
    auto mockDB = std::make_shared<MockDBStore>();
    EXPECT_CALL(*mockDB, Import(_, _, _))
        .WillOnce(Return(DistributedDB::DBStatus::INVALID_FILE))
        .WillOnce(Return(DistributedDB::DBStatus::OK));

    // Setup mock service to return secret key
    std::vector<std::vector<uint8_t>> secretKeys = {{1, 2, 3, 4}};
    MockKVDBService::SetSecretKeys(secretKeys);

    BackupInfo info;
    info.appId = "testApp";
    info.storeId = "store1";
    info.subUser = 0;
    info.encrypt = true;

    std::string fullName = "/data/test/kvdb/backup/store1/backup1.bak";
    auto status = BackupManager::GetInstance().ImportWithSecretKeyFromService(
        info, mockDB, fullName, false);
    
    EXPECT_EQ(status, Status::SUCCESS);
}

/**
 * @tc.name: BackupManagerImportWithSecretKeyFromServiceTest002
 * @tc.desc: Verify import with secret key from service fails when no valid key
 * @tc.type: FUNC
 * @tc.require: AR000D4876
 * @tc.author: zhangqiquan
 */
HWTEST_F(BackupManagerTest, BackupManagerImportWithSecretKeyFromServiceTest002, TestSize.Level1)
{
    auto mockDB = std::make_shared<MockDBStore>();
    EXPECT_CALL(*mockDB, Import(_, _, _))
        .WillRepeatedly(Return(DistributedDB::DBStatus::INVALID_FILE));

    // Setup mock service to return secret key
    std::vector<std::vector<uint8_t>> secretKeys = {{1, 2, 3, 4}};
    MockKVDBService::SetSecretKeys(secretKeys);

    BackupInfo info;
    info.appId = "testApp";
    info.storeId = "store1";
    info.subUser = 0;
    info.encrypt = true;

    std::string fullName = "/data/test/kvdb/backup/store1/backup1.bak";
    auto status = BackupManager::GetInstance().ImportWithSecretKeyFromService(
        info, mockDB, fullName, false);
    
    EXPECT_EQ(status, Status::CRYPT_ERROR);
}

/**
 * @tc.name: BackupManagerGetRestorePasswordTest001
 * @tc.desc: Verify get restore password for normal backup
 * @tc.type: FUNC
 * @tc.require: AR000D4876
 * @tc.author: zhangqiquan
 */
HWTEST_F(BackupManagerTest, BackupManagerGetRestorePasswordTest001, TestSize.Level1)
{
    // Setup key file
    StoreUtil::AddMockFile("/data/test/key/Prefix_backup_store1_backup1.key", 100);

    // Setup password
    std::vector<uint8_t> password = {1, 2, 3, 4};
    SecurityManager::GetInstance().SaveDBPassword("Prefix_backup_store1_backup1", "/data/test", password);

    BackupInfo info;
    info.name = "backup1";
    info.baseDir = "/data/test";
    info.storeId = "store1";

    auto dbPassword = BackupManager::GetInstance().GetRestorePassword("backup1.bak", info);
    EXPECT_EQ(dbPassword.password, password);
}

/**
 * @tc.name: BackupManagerGetRestorePasswordTest002
 * @tc.desc: Verify get restore password for auto backup
 * @tc.type: FUNC
 * @tc.require: AR000D4876
 * @tc.author: zhangqiquan
 */
HWTEST_F(BackupManagerTest, BackupManagerGetRestorePasswordTest002, TestSize.Level1)
{
    // Setup mock service to return password
    std::vector<std::vector<uint8_t>> passwords = {{1, 2, 3, 4}};
    MockKVDBService::SetBackupPasswords(passwords);

    BackupInfo info;
    info.name = "";
    info.baseDir = "/data/test";
    info.storeId = "store1";
    info.appId = "testApp";
    info.subUser = 0;

    auto dbPassword = BackupManager::GetInstance().GetRestorePassword("autoBackup.bak", info);
    EXPECT_EQ(dbPassword.password, std::vector<uint8_t>({1, 2, 3, 4}));
}

/**
 * @tc.name: BackupManagerGetRestorePasswordTest003
 * @tc.desc: Verify get restore password returns empty when no password available
 * @tc.type: FUNC
 * @tc.require: AR000D4876
 * @tc.author: zhangqiquan
 */
HWTEST_F(BackupManagerTest, BackupManagerGetRestorePasswordTest003, TestSize.Level1)
{
    BackupInfo info;
    info.name = "backup1";
    info.baseDir = "/data/test";
    info.storeId = "store1";

    auto dbPassword = BackupManager::GetInstance().GetRestorePassword("backup1.bak", info);
    EXPECT_TRUE(dbPassword.password.empty());
}

/**
 * @tc.name: BackupManagerHaveResidueFileTest001
 * @tc.desc: Verify have residue file detection
 * @tc.type: FUNC
 * @tc.require: AR000D4876
 * @tc.author: zhangqiquan
 */
HWTEST_F(BackupManagerTest, BackupManagerHaveResidueFileTest001, TestSize.Level1)
{
    std::vector<StoreUtil::FileInfo> files;
    files.push_back({"file1.bak", 100, 1000});
    files.push_back({"file2.bak.bk", 200, 2000});

    bool hasResidue = BackupManager::GetInstance().HaveResidueFile(files);
    EXPECT_TRUE(hasResidue);
}

/**
 * @tc.name: BackupManagerHaveResidueFileTest002
 * @tc.desc: Verify no residue file detection
 * @tc.type: FUNC
 * @tc.require: AR000D4876
 * @tc.author: zhangqiquan
 */
HWTEST_F(BackupManagerTest, BackupManagerHaveResidueFileTest002, TestSize.Level1)
{
    std::vector<StoreUtil::FileInfo> files;
    files.push_back({"file1.bak", 100, 1000});
    files.push_back({"file2.bak", 200, 2000});

    bool hasResidue = BackupManager::GetInstance().HaveResidueFile(files);
    EXPECT_FALSE(hasResidue);
}

/**
 * @tc.name: BackupManagerHaveResidueKeyTest001
 * @tc.desc: Verify have residue key detection
 * @tc.type: FUNC
 * @tc.require: AR000D4876
 * @tc.author: zhangqiquan
 */
HWTEST_F(BackupManagerTest, BackupManagerHaveResidueKeyTest001, TestSize.Level1)
{
    std::vector<StoreUtil::FileInfo> files;
    files.push_back({"Prefix_backup_store1_backup1.key", 100, 1000});
    files.push_back({"Prefix_backup_store1_backup1.key.bk", 200, 2000});

    bool hasResidue = BackupManager::GetInstance().HaveResidueKey(files, "store1");
    EXPECT_TRUE(hasResidue);
}

/**
 * @tc.name: BackupManagerHaveResidueKeyTest002
 * @tc.desc: Verify no residue key detection
 * @tc.type: FUNC
 * @tc.require: AR000D4876
 * @tc.author: zhangqiquan
 */
HWTEST_F(BackupManagerTest, BackupManagerHaveResidueKeyTest002, TestSize.Level1)
{
    std::vector<StoreUtil::FileInfo> files;
    files.push_back({"Prefix_backup_store1_backup1.key", 100, 1000});
    files.push_back({"Prefix_backup_store2_backup1.key.bk", 200, 2000});

    bool hasResidue = BackupManager::GetInstance().HaveResidueKey(files, "store1");
    EXPECT_FALSE(hasResidue);
}

/**
 * @tc.name: BackupManagerBuildResidueInfoTest001
 * @tc.desc: Verify build residue info for backup files
 * @tc.type: FUNC
 * @tc.require: AR000D4876
 * @tc.author: zhangqiquan
 */
HWTEST_F(BackupManagerTest, BackupManagerBuildResidueInfoTest001, TestSize.Level1)
{
    std::vector<StoreUtil::FileInfo> files;
    files.push_back({"backup1.bak", 100, 1000});
    files.push_back({"backup1.bak.bk", 200, 2000});
    files.push_back({"backup2.bak", 300, 3000});

    std::vector<StoreUtil::FileInfo> keys;
    keys.push_back({"Prefix_backup_store1_backup1.key", 50, 1000});
    keys.push_back({"Prefix_backup_store1_backup1.key.bk", 60, 2000});

    auto residueInfo = BackupManager::GetInstance().BuildResidueInfo(files, keys, "store1");
    
    EXPECT_EQ(residueInfo.size(), 1);
    EXPECT_TRUE(residueInfo["backup1"].hasRawBackup);
    EXPECT_TRUE(residueInfo["backup1"].hasTmpBackup);
    EXPECT_TRUE(residueInfo["backup1"].hasRawKey);
    EXPECT_TRUE(residueInfo["backup1"].hasTmpKey);
    EXPECT_EQ(residueInfo["backup1"].tmpBackupSize, 200);
    EXPECT_EQ(residueInfo["backup1"].tmpKeySize, 60);
}

/**
 * @tc.name: BackupManagerGetClearTypeTest001
 * @tc.desc: Verify get clear type for rollback data
 * @tc.type: FUNC
 * @tc.require: AR000D4876
 * @tc.author: zhangqiquan
 */
HWTEST_F(BackupManagerTest, BackupManagerGetClearTypeTest001, TestSize.Level1)
{
    BackupManager::ResidueInfo info = {100, 0, true, false, false, false};
    auto clearType = BackupManager::GetInstance().GetClearType(info);
    EXPECT_EQ(clearType, BackupManager::ROLLBACK_DATA);
}

/**
 * @tc.name: BackupManagerGetClearTypeTest002
 * @tc.desc: Verify get clear type for backup data
 * @tc.type: FUNC
 * @tc.require: AR000D4876
 * @tc.author: zhangqiquan
 */
HWTEST_F(BackupManagerTest, BackupManagerGetClearTypeTest002, TestSize.Level1) 
{
    BackupManager::ResidueInfo info = {0, 100, false, true, false, false};
    auto clearType = BackupManager::GetInstance().GetClearType(info);
    EXPECT_EQ(clearType, BackupManager::BACKUP_DATA);
}

/**
 * @tc.name: BackupManagerGetClearTypeTest003
 * @tc.desc: Verify get clear type for all data
 * @tc.type: FUNC
 * @tc.require: AR000D4876
 * @tc.author: zhangqiquan
 */
HWTEST_F(BackupManagerTest, BackupManagerGetClearTypeTest003, TestSize.Level1)
{
    
    BackupManager::ResidueInfo info = {100, 100, true, true, false, false};
    auto clearType = BackupManager::GetInstance().GetClearType(info);
    EXPECT_EQ(clearType, BackupManager::ALL_DATA);

}

/**
 * @tc.name: BackupManagerGetClearTypeTest004
 * @tc.desc: Verify get clear type for no data
 * @tc.type: FUNC
 * @tc.require: AR000D4876
 * @tc.author: zhangqiquan
 */
HWTEST_F(BackupManagerTest, BackupManagerGetClearTypeTest004, TestSize.Level1)
{
    
    BackupManager::ResidueInfo info = {0, 0, false, false, false, false};
    auto clearType = BackupManager::GetInstance().GetClearType(info);
    EXPECT_EQ(clearType, BackupManager::NO_DATA);   
}

/**
 * @tc.name: BackupManagerGetClearTypeTest005
 * @tc.desc: Verify get clear type for unknown data
 * @tc.type: FUNC
 * @tc.require: AR000D4876
 * @tc.author: zhangqiquan
 */
HWTEST_F(BackupManagerTest, BackupManagerGetClearTypeTest005, TestSize.Level1)
{
    
    BackupManager::ResidueInfo info = {100, 100, true, true, true, true};
    auto clearType = BackupManager::GetInstance().GetClearType(info);
    EXPECT_EQ(clearType, BackupManager::UNKNOWN_DATA);  
}

/**
 * @tc.name: BackupManagerHaveResidueKeyTest001
 * @tc.desc: Verify residue key detection
 * @tc.type: FUNC
 * @tc.require: AR000D4876
 * @tc.author: zhangqiquan
 */
HWTEST_F(BackupManagerTest, BackupManagerHaveResidueKeyTest001, TestSize.Level1)
{
    
    std::vector<StoreUtil::FileInfo> files;
    files.push_back({"Prefix_backup_store1_backup1.key", 100, 1000});
    files.push_back({"Prefix_backup_store2_backup1.key.bk", 200, 2000});

    bool hasResidue = BackupManager::GetInstance().HaveResidueKey(files, "store1");
    EXPECT_FALSE(hasResidue);
}
