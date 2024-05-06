/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#include "distributeddb_tools_unit_test.h"

#include <codecvt>
#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <fstream>
#include <gtest/gtest.h>
#include <locale>
#include <openssl/rand.h>
#include <random>
#include <set>
#include <sys/types.h>

#include "cloud/cloud_db_constant.h"
#include "cloud/cloud_db_types.h"
#include "db_common.h"
#include "db_constant.h"
#include "generic_single_ver_kv_entry.h"
#include "platform_specific.h"
#include "runtime_config.h"
#include "single_ver_data_packet.h"
#include "sqlite_relational_utils.h"
#include "store_observer.h"
#include "time_helper.h"
#include "value_hash_calc.h"

using namespace DistributedDB;

namespace DistributedDBUnitTest {
namespace {
    const std::string CREATE_LOCAL_TABLE_SQL =
        "CREATE TABLE IF NOT EXISTS local_data(" \
        "key BLOB PRIMARY KEY," \
        "value BLOB," \
        "timestamp INT," \
        "hash_key BLOB);";

    const std::string CREATE_META_TABLE_SQL =
        "CREATE TABLE IF NOT EXISTS meta_data("  \
        "key    BLOB PRIMARY KEY  NOT NULL," \
        "value  BLOB);";

    const std::string CREATE_SYNC_TABLE_SQL =
        "CREATE TABLE IF NOT EXISTS sync_data(" \
        "key         BLOB NOT NULL," \
        "value       BLOB," \
        "timestamp   INT  NOT NULL," \
        "flag        INT  NOT NULL," \
        "device      BLOB," \
        "ori_device  BLOB," \
        "hash_key    BLOB PRIMARY KEY NOT NULL," \
        "w_timestamp INT);";

    const std::string CREATE_SYNC_TABLE_INDEX_SQL =
        "CREATE INDEX IF NOT EXISTS key_index ON sync_data (key);";

    const std::string CREATE_TABLE_SQL =
        "CREATE TABLE IF NOT EXISTS version_data(key BLOB, value BLOB, oper_flag INTEGER, version INTEGER, " \
        "timestamp INTEGER, ori_timestamp INTEGER, hash_key BLOB, " \
        "PRIMARY key(hash_key, version));";

    const std::string CREATE_SQL =
        "CREATE TABLE IF NOT EXISTS data(key BLOB PRIMARY key, value BLOB);";

    bool CompareEntry(const DistributedDB::Entry &a, const DistributedDB::Entry &b)
    {
        return (a.key < b.key);
    }
}

// OpenDbProperties.uri do not need
int DistributedDBToolsUnitTest::CreateMockSingleDb(DatabaseInfo &dbInfo, OpenDbProperties &properties)
{
    std::string identifier = dbInfo.userId + "-" + dbInfo.appId + "-" + dbInfo.storeId;
    std::string hashIdentifier = DBCommon::TransferHashString(identifier);
    std::string identifierName = DBCommon::TransferStringToHex(hashIdentifier);

    if (OS::GetRealPath(dbInfo.dir, properties.uri) != E_OK) {
        LOGE("Failed to canonicalize the path.");
        return -E_INVALID_ARGS;
    }

    int errCode = DBCommon::CreateStoreDirectory(dbInfo.dir, identifierName, DBConstant::SINGLE_SUB_DIR, true);
    if (errCode != E_OK) {
        return errCode;
    }

    properties.uri = dbInfo.dir + "/" + identifierName + "/" +
        DBConstant::SINGLE_SUB_DIR + "/" + DBConstant::SINGLE_VER_DATA_STORE + DBConstant::DB_EXTENSION;
    if (properties.sqls.empty()) {
        std::vector<std::string> defaultCreateTableSqls = {
            CREATE_LOCAL_TABLE_SQL,
            CREATE_META_TABLE_SQL,
            CREATE_SYNC_TABLE_SQL,
            CREATE_SYNC_TABLE_INDEX_SQL
        };
        properties.sqls = defaultCreateTableSqls;
    }

    sqlite3 *db = nullptr;
    errCode = SQLiteUtils::OpenDatabase(properties, db);
    if (errCode != E_OK) {
        return errCode;
    }
    errCode = SQLiteUtils::SetUserVer(properties, dbInfo.dbUserVersion);
    if (errCode != E_OK) {
        return errCode;
    }

    (void)sqlite3_close_v2(db);
    db = nullptr;
    return errCode;
}

static int CreatMockMultiDb(OpenDbProperties &properties, DatabaseInfo &dbInfo)
{
    sqlite3 *db = nullptr;
    (void)SQLiteUtils::OpenDatabase(properties, db);
    int errCode = SQLiteUtils::SetUserVer(properties, dbInfo.dbUserVersion);
    (void)sqlite3_close_v2(db);
    db = nullptr;
    if (errCode != E_OK) {
        return errCode;
    }
    return errCode;
}

int DistributedDBToolsUnitTest::OpenMockMultiDb(DatabaseInfo &dbInfo, OpenDbProperties &properties)
{
    std::string identifier = dbInfo.userId + "-" + dbInfo.appId + "-" + dbInfo.storeId;
    std::string hashIdentifier = DBCommon::TransferHashString(identifier);
    std::string identifierName = DBCommon::TransferStringToHex(hashIdentifier);

    OpenDbProperties commitProperties = properties;
    commitProperties.uri = dbInfo.dir + "/" + identifierName + "/" + DBConstant::MULTI_SUB_DIR +
        "/commit_logs" + DBConstant::DB_EXTENSION;

    commitProperties.sqls = {CREATE_SQL};

    OpenDbProperties kvStorageProperties = commitProperties;
    kvStorageProperties.uri = dbInfo.dir + "/" + identifierName + "/" +
        DBConstant::MULTI_SUB_DIR + "/value_storage" + DBConstant::DB_EXTENSION;
    OpenDbProperties metaStorageProperties = commitProperties;
    metaStorageProperties.uri = dbInfo.dir + "/" + identifierName + "/" +
        DBConstant::MULTI_SUB_DIR + "/meta_storage" + DBConstant::DB_EXTENSION;

    // test code, Don't needpay too much attention to exception handling
    int errCode = CreatMockMultiDb(properties, dbInfo);
    if (errCode != E_OK) {
        return errCode;
    }

    errCode = CreatMockMultiDb(kvStorageProperties, dbInfo);
    if (errCode != E_OK) {
        return errCode;
    }

    errCode = CreatMockMultiDb(metaStorageProperties, dbInfo);
    if (errCode != E_OK) {
        return errCode;
    }

    return errCode;
}

// OpenDbProperties.uri do not need
int DistributedDBToolsUnitTest::CreateMockMultiDb(DatabaseInfo &dbInfo, OpenDbProperties &properties)
{
    std::string identifier = dbInfo.userId + "-" + dbInfo.appId + "-" + dbInfo.storeId;
    std::string hashIdentifier = DBCommon::TransferHashString(identifier);
    std::string identifierName = DBCommon::TransferStringToHex(hashIdentifier);

    if (OS::GetRealPath(dbInfo.dir, properties.uri) != E_OK) {
        LOGE("Failed to canonicalize the path.");
        return -E_INVALID_ARGS;
    }

    int errCode = DBCommon::CreateStoreDirectory(dbInfo.dir, identifierName, DBConstant::MULTI_SUB_DIR, true);
    if (errCode != E_OK) {
        return errCode;
    }

    properties.uri = dbInfo.dir + "/" + identifierName + "/" + DBConstant::MULTI_SUB_DIR +
        "/" + DBConstant::MULTI_VER_DATA_STORE + DBConstant::DB_EXTENSION;

    if (properties.sqls.empty()) {
        properties.sqls = {CREATE_TABLE_SQL};
    }

    OpenMockMultiDb(dbInfo, properties);

    return errCode;
}

int DistributedDBToolsUnitTest::GetResourceDir(std::string& dir)
{
    int errCode = GetCurrentDir(dir);
    if (errCode != E_OK) {
        return -E_INVALID_PATH;
    }
#ifdef DB_DEBUG_ENV
    dir = dir + "/resource/";
#endif
    return E_OK;
}

int DistributedDBToolsUnitTest::GetCurrentDir(std::string &dir)
{
    static const int maxFileLength = 1024;
    dir = "";
    char buffer[maxFileLength] = {0};
    int length = readlink("/proc/self/exe", buffer, maxFileLength);
    if (length < 0 || length >= maxFileLength) {
        LOGE("read directory err length:%d", length);
        return -E_LENGTH_ERROR;
    }
    LOGD("DIR = %s", buffer);
    dir = buffer;
    if (dir.rfind("/") == std::string::npos && dir.rfind("\\") == std::string::npos) {
        LOGE("current patch format err");
        return -E_INVALID_PATH;
    }

    if (dir.rfind("/") != std::string::npos) {
        dir.erase(dir.rfind("/") + 1);
    }
    return E_OK;
}

void DistributedDBToolsUnitTest::TestDirInit(std::string &dir)
{
    if (GetCurrentDir(dir) != E_OK) {
        dir = "/";
    }

    dir.append("testDbDir");
    DIR *dirTmp = opendir(dir.c_str());
    if (dirTmp == nullptr) {
        if (OS::MakeDBDirectory(dir) != 0) {
            LOGI("MakeDirectory err!");
            dir = "/";
            return;
        }
    } else {
        closedir(dirTmp);
    }
}

int DistributedDBToolsUnitTest::RemoveTestDbFiles(const std::string &dir)
{
    bool isExisted = OS::CheckPathExistence(dir);
    if (!isExisted) {
        return E_OK;
    }

    int nFile = 0;
    std::string dirName;
    struct dirent *direntPtr = nullptr;
    DIR *dirPtr = opendir(dir.c_str());
    if (dirPtr == nullptr) {
        LOGE("opendir error!");
        return -E_INVALID_PATH;
    }
    while (true) {
        direntPtr = readdir(dirPtr);
        // condition to exit the loop
        if (direntPtr == nullptr) {
            break;
        }
        // only remove all *.db files
        std::string str(direntPtr->d_name);
        if (str == "." || str == "..") {
            continue;
        }
        dirName.clear();
        dirName.append(dir).append("/").append(str);
        if (direntPtr->d_type == DT_DIR) {
            RemoveTestDbFiles(dirName);
            rmdir(dirName.c_str());
        } else if (remove(dirName.c_str()) != 0) {
            LOGI("remove file: %s failed!", dirName.c_str());
            continue;
        }
        nFile++;
    }
    closedir(dirPtr);
    LOGI("Total %d test db files are removed!", nFile);
    return 0;
}

#ifndef OMIT_MULTI_VER
void DistributedDBToolsUnitTest::KvStoreDelegateCallback(
    DBStatus statusSrc, KvStoreDelegate *kvStoreSrc, DBStatus &statusDst, KvStoreDelegate *&kvStoreDst)
{
    statusDst = statusSrc;
    kvStoreDst = kvStoreSrc;
}

void DistributedDBToolsUnitTest::SnapshotDelegateCallback(
    DBStatus statusSrc, KvStoreSnapshotDelegate* snapshot, DBStatus &statusDst, KvStoreSnapshotDelegate *&snapshotDst)
{
    statusDst = statusSrc;
    snapshotDst = snapshot;
}
#endif

void DistributedDBToolsUnitTest::KvStoreNbDelegateCallback(
    DBStatus statusSrc, KvStoreNbDelegate* kvStoreSrc, DBStatus &statusDst, KvStoreNbDelegate *&kvStoreDst)
{
    statusDst = statusSrc;
    kvStoreDst = kvStoreSrc;
}

void DistributedDBToolsUnitTest::ValueCallback(
    DBStatus statusSrc, const Value &valueSrc, DBStatus &statusDst, Value &valueDst)
{
    statusDst = statusSrc;
    valueDst = valueSrc;
}

void DistributedDBToolsUnitTest::EntryVectorCallback(DBStatus statusSrc, const std::vector<Entry> &entrySrc,
    DBStatus &statusDst, unsigned long &matchSize, std::vector<Entry> &entryDst)
{
    statusDst = statusSrc;
    matchSize = static_cast<unsigned long>(entrySrc.size());
    entryDst = entrySrc;
}

// size need bigger than prefixkey length
std::vector<uint8_t> DistributedDBToolsUnitTest::GetRandPrefixKey(const std::vector<uint8_t> &prefixKey, uint32_t size)
{
    std::vector<uint8_t> value;
    if (size <= prefixKey.size()) {
        return value;
    }
    DistributedDBToolsUnitTest::GetRandomKeyValue(value, size - prefixKey.size());
    std::vector<uint8_t> res(prefixKey);
    res.insert(res.end(), value.begin(), value.end());
    return res;
}

void DistributedDBToolsUnitTest::GetRandomKeyValue(std::vector<uint8_t> &value, uint32_t defaultSize)
{
    uint32_t randSize = 0;
    if (defaultSize == 0) {
        uint8_t simSize = 0;
        RAND_bytes(&simSize, 1);
        randSize = (simSize == 0) ? 1 : simSize;
    } else {
        randSize = defaultSize;
    }

    value.resize(randSize);
    RAND_bytes(value.data(), randSize);
}

bool DistributedDBToolsUnitTest::IsValueEqual(const DistributedDB::Value &read, const DistributedDB::Value &origin)
{
    if (read != origin) {
        DBCommon::PrintHexVector(read, __LINE__, "read");
        DBCommon::PrintHexVector(origin, __LINE__, "origin");
        return false;
    }

    return true;
}

bool DistributedDBToolsUnitTest::IsEntryEqual(const DistributedDB::Entry &entryOrg,
    const DistributedDB::Entry &entryRet)
{
    if (entryOrg.key != entryRet.key) {
        LOGD("key not equal, entryOrg key size is [%zu], entryRet key size is [%zu]", entryOrg.key.size(),
            entryRet.key.size());
        return false;
    }

    if (entryOrg.value != entryRet.value) {
        LOGD("value not equal, entryOrg value size is [%zu], entryRet value size is [%zu]", entryOrg.value.size(),
            entryRet.value.size());
        return false;
    }

    return true;
}

bool DistributedDBToolsUnitTest::IsEntriesEqual(const std::vector<DistributedDB::Entry> &entriesOrg,
    const std::vector<DistributedDB::Entry> &entriesRet, bool needSort)
{
    LOGD("entriesOrg size is [%zu], entriesRet size is [%zu]", entriesOrg.size(),
        entriesRet.size());

    if (entriesOrg.size() != entriesRet.size()) {
        return false;
    }
    std::vector<DistributedDB::Entry> entries1 = entriesOrg;
    std::vector<DistributedDB::Entry> entries2 = entriesRet;

    if (needSort) {
        sort(entries1.begin(), entries1.end(), CompareEntry);
        sort(entries2.begin(), entries2.end(), CompareEntry);
    }

    for (size_t i = 0; i < entries1.size(); i++) {
        if (entries1[i].key != entries2[i].key) {
            LOGE("IsEntriesEqual failed, key of index[%zu] not match", i);
            return false;
        }
        if (entries1[i].value != entries2[i].value) {
            LOGE("IsEntriesEqual failed, value of index[%zu] not match", i);
            return false;
        }
    }

    return true;
}

bool DistributedDBToolsUnitTest::CheckObserverResult(const std::vector<DistributedDB::Entry> &orgEntries,
    const std::list<DistributedDB::Entry> &resultLst)
{
    LOGD("orgEntries.size() is [%zu], resultLst.size() is [%zu]", orgEntries.size(),
        resultLst.size());

    if (orgEntries.size() != resultLst.size()) {
        return false;
    }

    int index = 0;
    for (const auto &entry : resultLst) {
        if (entry.key != orgEntries[index].key) {
            LOGE("CheckObserverResult failed, key of index[%d] not match", index);
            return false;
        }
        if (entry.value != orgEntries[index].value) {
            LOGE("CheckObserverResult failed, value of index[%d] not match", index);
            return false;
        }
        index++;
    }

    return true;
}

bool DistributedDBToolsUnitTest::IsEntryExist(const DistributedDB::Entry &entry,
    const std::vector<DistributedDB::Entry> &entries)
{
    std::set<std::vector<uint8_t>> sets;
    for (const auto &iter : entries) {
        sets.insert(iter.key);
    }

    if (entries.size() != sets.size()) {
        return false;
    }
    sets.clear();
    bool isFound = false;
    for (const auto &iter : entries) {
        if (entry.key == iter.key) {
            if (entry.value == iter.value) {
                isFound = true;
            }
            break;
        }
    }
    return isFound;
}

bool DistributedDBToolsUnitTest::IsItemValueExist(const DistributedDB::DataItem &item,
    const std::vector<DistributedDB::DataItem> &items)
{
    std::set<Key> sets;
    for (const auto &iter : items) {
        sets.insert(iter.key);
    }

    if (items.size() != sets.size()) {
        return false;
    }
    sets.clear();
    bool isFound = false;
    for (const auto &iter : items) {
        if (item.key == iter.key) {
            if (item.value == iter.value) {
                isFound = true;
            }
            break;
        }
    }
    return isFound;
}

bool DistributedDBToolsUnitTest::IsKvEntryExist(const DistributedDB::Entry &entry,
    const std::vector<DistributedDB::Entry> &entries)
{
    std::set<std::vector<uint8_t>> sets;
    for (const auto &iter : entries) {
        sets.insert(iter.key);
    }

    if (entries.size() != sets.size()) {
        return false;
    }
    sets.clear();
    bool isFound = false;
    for (const auto &iter : entries) {
        if (entry.key == iter.key) {
            if (entry.value == iter.value) {
                isFound = true;
            }
            break;
        }
    }

    return isFound;
}

int DistributedDBToolsUnitTest::ModifyDatabaseFile(const std::string &fileDir, uint64_t modifyPos,
    uint32_t modifyCnt, uint32_t value)
{
    LOGI("Modify database file:%s", fileDir.c_str());
    std::fstream dataFile(fileDir, std::fstream::binary | std::fstream::out | std::fstream::in);
    if (!dataFile.is_open()) {
        LOGD("Open the database file failed");
        return -E_UNEXPECTED_DATA;
    }

    if (!dataFile.seekg(0, std::fstream::end)) {
        return -E_UNEXPECTED_DATA;
    }

    uint64_t fileSize;
    std::ios::pos_type pos = dataFile.tellg();
    if (pos < 0) {
        return -E_UNEXPECTED_DATA;
    } else {
        fileSize = static_cast<uint64_t>(pos);
        if (fileSize < 1024) { // the least page size is 1024 bytes.
            LOGE("Invalid database file:%" PRIu64 ".", fileSize);
            return -E_UNEXPECTED_DATA;
        }
    }

    if (fileSize <= modifyPos) {
        return E_OK;
    }

    if (!dataFile.seekp(modifyPos)) {
        return -E_UNEXPECTED_DATA;
    }
    for (uint32_t i = 0; i < modifyCnt; i++) {
        if (!dataFile.write(reinterpret_cast<char *>(&value), sizeof(uint32_t))) {
            return -E_UNEXPECTED_DATA;
        }
    }

    dataFile.flush();
    return E_OK;
}

int DistributedDBToolsUnitTest::GetSyncDataTest(const SyncInputArg &syncInputArg, SQLiteSingleVerNaturalStore *store,
    std::vector<DataItem> &dataItems, ContinueToken &continueStmtToken)
{
    std::vector<SingleVerKvEntry *> entries;
    DataSizeSpecInfo syncDataSizeInfo = {syncInputArg.blockSize_, DBConstant::MAX_HPMODE_PACK_ITEM_SIZE};
    int errCode = store->GetSyncData(syncInputArg.begin_, syncInputArg.end_, entries,
        continueStmtToken, syncDataSizeInfo);

    ConvertSingleVerEntryToItems(entries, dataItems);
    return errCode;
}

int DistributedDBToolsUnitTest::GetSyncDataNextTest(SQLiteSingleVerNaturalStore *store, uint32_t blockSize,
    std::vector<DataItem> &dataItems, ContinueToken &continueStmtToken)
{
    std::vector<SingleVerKvEntry *> entries;
    DataSizeSpecInfo syncDataSizeInfo = {blockSize, DBConstant::MAX_HPMODE_PACK_ITEM_SIZE};
    int errCode = store->GetSyncDataNext(entries, continueStmtToken, syncDataSizeInfo);

    ConvertSingleVerEntryToItems(entries, dataItems);
    return errCode;
}

int DistributedDBToolsUnitTest::PutSyncDataTest(SQLiteSingleVerNaturalStore *store,
    const std::vector<DataItem> &dataItems, const std::string &deviceName)
{
    QueryObject query(Query::Select());
    return PutSyncDataTest(store, dataItems, deviceName, query);
}

int DistributedDBToolsUnitTest::PutSyncDataTest(SQLiteSingleVerNaturalStore *store,
    const std::vector<DataItem> &dataItems, const std::string &deviceName, const QueryObject &query)
{
    std::vector<SingleVerKvEntry *> entries;
    std::vector<DistributedDB::DataItem> items = dataItems;
    for (auto &item : items) {
        auto *entry = new (std::nothrow) GenericSingleVerKvEntry();
        if (entry == nullptr) {
            ReleaseSingleVerEntry(entries);
            return -E_OUT_OF_MEMORY;
        }
        entry->SetEntryData(std::move(item));
        entry->SetWriteTimestamp(entry->GetTimestamp());
        entries.push_back(entry);
    }

    int errCode = store->PutSyncDataWithQuery(query, entries, deviceName);
    ReleaseSingleVerEntry(entries);
    return errCode;
}

int DistributedDBToolsUnitTest::ConvertItemsToSingleVerEntry(const std::vector<DistributedDB::DataItem> &dataItems,
    std::vector<DistributedDB::SingleVerKvEntry *> &entries)
{
    std::vector<DistributedDB::DataItem> items = dataItems;
    for (auto &item : items) {
        GenericSingleVerKvEntry *entry = new (std::nothrow) GenericSingleVerKvEntry();
        if (entry == nullptr) {
            ReleaseSingleVerEntry(entries);
            return -E_OUT_OF_MEMORY;
        }
        entry->SetEntryData(std::move(item));
        entries.push_back(entry);
    }
    return E_OK;
}

void DistributedDBToolsUnitTest::ConvertSingleVerEntryToItems(std::vector<DistributedDB::SingleVerKvEntry *> &entries,
    std::vector<DistributedDB::DataItem> &dataItems)
{
    for (auto &itemEntry : entries) {
        GenericSingleVerKvEntry *entry = reinterpret_cast<GenericSingleVerKvEntry *>(itemEntry);
        if (entry != nullptr) {
            DataItem item;
            item.origDev = entry->GetOrigDevice();
            item.flag = entry->GetFlag();
            item.timestamp = entry->GetTimestamp();
            entry->GetKey(item.key);
            entry->GetValue(item.value);
            dataItems.push_back(item);
            // clear vector entry
            delete itemEntry;
            itemEntry = nullptr;
        }
    }
    entries.clear();
}

void DistributedDBToolsUnitTest::ReleaseSingleVerEntry(std::vector<DistributedDB::SingleVerKvEntry *> &entries)
{
    for (auto &item : entries) {
        delete item;
        item = nullptr;
    }
    entries.clear();
}

void DistributedDBToolsUnitTest::CalcHash(const std::vector<uint8_t> &value, std::vector<uint8_t> &hashValue)
{
    ValueHashCalc hashCalc;
    hashCalc.Initialize();
    hashCalc.Update(value);
    hashCalc.GetResult(hashValue);
}

void DistributedDBToolsUnitTest::Dump()
{
    constexpr const char *rightDumpParam = "--database";
    constexpr const char *ignoreDumpParam = "ignore-param";
    const std::u16string u16DumpRightParam =
        std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> {}.from_bytes(rightDumpParam);
    const std::u16string u16DumpIgnoreParam =
        std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> {}.from_bytes(ignoreDumpParam);
    std::vector<std::u16string> params = {
        u16DumpRightParam,
        u16DumpIgnoreParam
    };
    // print to std::cout
    RuntimeConfig::Dump(0, params);
}

std::string DistributedDBToolsUnitTest::GetKvNbStoreDirectory(const std::string &identifier,
    const std::string &dbFilePath, const std::string &dbDir)
{
    std::string identifierName = DBCommon::TransferStringToHex(identifier);
    return dbDir + "/" + identifierName + "/" + dbFilePath;
}

KvStoreObserverUnitTest::KvStoreObserverUnitTest() : callCount_(0), isCleared_(false)
{}

void KvStoreObserverUnitTest::OnChange(const KvStoreChangedData& data)
{
    callCount_++;
    inserted_ = data.GetEntriesInserted();
    updated_ = data.GetEntriesUpdated();
    deleted_ = data.GetEntriesDeleted();
    isCleared_ = data.IsCleared();
    LOGD("Onchangedata :%zu -- %zu -- %zu -- %d", inserted_.size(), updated_.size(), deleted_.size(), isCleared_);
    LOGD("Onchange() called success!");
}

void KvStoreObserverUnitTest::OnChange(const DistributedDB::StoreChangedData &data)
{
    (void)data;
    KvStoreObserver::OnChange(data);
}

void KvStoreObserverUnitTest::OnChange(DistributedDB::StoreObserver::StoreChangedInfo &&data)
{
    (void)data;
    KvStoreObserver::OnChange(std::move(data));
}

void KvStoreObserverUnitTest::OnChange(DistributedDB::Origin origin, const std::string &originalId,
    DistributedDB::ChangedData &&data)
{
    (void)origin;
    (void)originalId;
    (void)data;
    KvStoreObserver::OnChange(origin, originalId, std::move(data));
}

void KvStoreObserverUnitTest::ResetToZero()
{
    callCount_ = 0;
    isCleared_ = false;
    inserted_.clear();
    updated_.clear();
    deleted_.clear();
}

unsigned long KvStoreObserverUnitTest::GetCallCount() const
{
    return callCount_;
}

const std::list<Entry> &KvStoreObserverUnitTest::GetEntriesInserted() const
{
    return inserted_;
}

const std::list<Entry> &KvStoreObserverUnitTest::GetEntriesUpdated() const
{
    return updated_;
}

const std::list<Entry> &KvStoreObserverUnitTest::GetEntriesDeleted() const
{
    return deleted_;
}

bool KvStoreObserverUnitTest::IsCleared() const
{
    return isCleared_;
}

RelationalStoreObserverUnitTest::RelationalStoreObserverUnitTest() : callCount_(0)
{
}

unsigned long RelationalStoreObserverUnitTest::GetCallCount() const
{
    return callCount_;
}

unsigned long RelationalStoreObserverUnitTest::GetCloudCallCount() const
{
    return cloudCallCount_;
}

void RelationalStoreObserverUnitTest::OnChange(const StoreChangedData &data)
{
    callCount_++;
    changeDevice_ = data.GetDataChangeDevice();
    data.GetStoreProperty(storeProperty_);
    LOGD("Onchangedata : %s", changeDevice_.c_str());
    LOGD("Onchange() called success!");
}

void RelationalStoreObserverUnitTest::OnChange(
    DistributedDB::Origin origin, const std::string &originalId, DistributedDB::ChangedData &&data)
{
    cloudCallCount_++;
    savedChangedData_[data.tableName] = data;
    LOGD("cloud sync Onchangedata, tableName = %s", data.tableName.c_str());
}

uint32_t RelationalStoreObserverUnitTest::GetCallbackDetailsType() const
{
    return detailsType_;
}

void RelationalStoreObserverUnitTest::SetCallbackDetailsType(uint32_t type)
{
    detailsType_ = type;
}

void RelationalStoreObserverUnitTest::SetExpectedResult(const DistributedDB::ChangedData &changedData)
{
    expectedChangedData_[changedData.tableName] = changedData;
}

static bool IsPrimaryKeyEq(DistributedDB::Type &input, DistributedDB::Type &expected)
{
    if (input.index() != expected.index()) {
        return false;
    }
    switch (expected.index()) {
        case TYPE_INDEX<int64_t>:
            if (std::get<int64_t>(input) != std::get<int64_t>(expected)) {
                return false;
            }
            break;
        case TYPE_INDEX<std::string>:
            if (std::get<std::string>(input) != std::get<std::string>(expected)) {
                return false;
            }
            break;
        case TYPE_INDEX<bool>:
            if (std::get<bool>(input) != std::get<bool>(expected)) {
                return false;
            }
            break;
        case TYPE_INDEX<double>:
        case TYPE_INDEX<Bytes>:
        case TYPE_INDEX<Asset>:
        case TYPE_INDEX<Assets>:
            LOGE("NOT HANDLE THIS SITUATION");
            return false;
        default: {
            break;
        }
    }
    return true;
}

static bool IsPrimaryDataEq(
    uint64_t type, DistributedDB::ChangedData &input, DistributedDB::ChangedData &expected)
{
    for (size_t m = 0; m < input.primaryData[type].size(); m++) {
        if (m >= expected.primaryData[type].size()) {
            LOGE("Actual primary data's size is more than the expected!");
            return false;
        }
        if (input.primaryData[type][m].size() != expected.primaryData[type][m].size()) {
            LOGE("Primary data fields' size is not equal!");
            return false;
        }
        for (size_t k = 0; k < input.primaryData[type][m].size(); k++) {
            if (!IsPrimaryKeyEq(input.primaryData[type][m][k], expected.primaryData[type][m][k])) {
                return false;
            }
        }
    }
    return true;
}

static bool IsAllTypePrimaryDataEq(DistributedDB::ChangedData &input, DistributedDB::ChangedData &expected)
{
    for (uint64_t type = ChangeType::OP_INSERT; type < ChangeType::OP_BUTT; ++type) {
        if (!IsPrimaryDataEq(type, input, expected)) {
            return false;
        }
    }
    return true;
}

static bool isChangedDataEq(DistributedDB::ChangedData &input, DistributedDB::ChangedData &expected)
{
    if (input.tableName != expected.tableName) {
        return false;
    }
    if (input.properties.isTrackedDataChange != expected.properties.isTrackedDataChange) {
        return false;
    }
    if (input.field.size() != expected.field.size()) {
        return false;
    }
    for (size_t i = 0; i < input.field.size(); i++) {
        if (!DBCommon::CaseInsensitiveCompare(input.field[i], expected.field[i])) {
            return false;
        }
    }
    return IsAllTypePrimaryDataEq(input, expected);
}

bool RelationalStoreObserverUnitTest::IsAllChangedDataEq()
{
    for (auto iter = expectedChangedData_.begin(); iter != expectedChangedData_.end(); ++iter) {
        auto iterInSavedChangedData = savedChangedData_.find(iter->first);
        if (iterInSavedChangedData == savedChangedData_.end()) {
            return false;
        }
        if (!isChangedDataEq(iterInSavedChangedData->second, iter->second)) {
            return false;
        }
    }
    return true;
}

void RelationalStoreObserverUnitTest::ClearChangedData()
{
    expectedChangedData_.clear();
    savedChangedData_.clear();
}

void RelationalStoreObserverUnitTest::ResetToZero()
{
    callCount_ = 0;
    changeDevice_.clear();
    storeProperty_ = {};
}

void RelationalStoreObserverUnitTest::ResetCloudSyncToZero()
{
    cloudCallCount_ = 0u;
    savedChangedData_.clear();
}

const std::string RelationalStoreObserverUnitTest::GetDataChangeDevice() const
{
    return changeDevice_;
}

DistributedDB::StoreProperty RelationalStoreObserverUnitTest::GetStoreProperty() const
{
    return storeProperty_;
}

DBStatus DistributedDBToolsUnitTest::SyncTest(KvStoreNbDelegate* delegate,
    const std::vector<std::string>& devices, SyncMode mode,
    std::map<std::string, DBStatus>& statuses, const Query &query)
{
    statuses.clear();
    DBStatus callStatus = delegate->Sync(devices, mode,
        [&statuses, this](const std::map<std::string, DBStatus>& statusMap) {
            statuses = statusMap;
            std::unique_lock<std::mutex> innerlock(this->syncLock_);
            this->syncCondVar_.notify_one();
        }, query, false);

    std::unique_lock<std::mutex> lock(syncLock_);
    syncCondVar_.wait(lock, [callStatus, &statuses]() {
            if (callStatus != OK) {
                return true;
            }
            return !statuses.empty();
        });
    return callStatus;
}

DBStatus DistributedDBToolsUnitTest::SyncTest(KvStoreNbDelegate* delegate,
    const std::vector<std::string>& devices, SyncMode mode,
    std::map<std::string, DBStatus>& statuses, bool wait)
{
    statuses.clear();
    DBStatus callStatus = delegate->Sync(devices, mode,
        [&statuses, this](const std::map<std::string, DBStatus>& statusMap) {
            statuses = statusMap;
            std::unique_lock<std::mutex> innerlock(this->syncLock_);
            this->syncCondVar_.notify_one();
        }, wait);
    if (!wait) {
        std::unique_lock<std::mutex> lock(syncLock_);
        syncCondVar_.wait(lock, [callStatus, &statuses]() {
                if (callStatus != OK) {
                    return true;
                }
                if (statuses.size() != 0) {
                    return true;
                }
                return false;
            });
        }
    return callStatus;
}

void KvStoreCorruptInfo::CorruptCallBack(const std::string &appId, const std::string &userId,
    const std::string &storeId)
{
    DatabaseInfo databaseInfo;
    databaseInfo.appId = appId;
    databaseInfo.userId = userId;
    databaseInfo.storeId = storeId;
    LOGD("appId :%s, userId:%s, storeId:%s", appId.c_str(), userId.c_str(), storeId.c_str());
    databaseInfoVect_.push_back(databaseInfo);
}

size_t KvStoreCorruptInfo::GetDatabaseInfoSize() const
{
    return databaseInfoVect_.size();
}

bool KvStoreCorruptInfo::IsDataBaseCorrupted(const std::string &appId, const std::string &userId,
    const std::string &storeId) const
{
    for (const auto &item : databaseInfoVect_) {
        if (item.appId == appId &&
            item.userId == userId &&
            item.storeId == storeId) {
            return true;
        }
    }
    return false;
}

void KvStoreCorruptInfo::Reset()
{
    databaseInfoVect_.clear();
}

int DistributedDBToolsUnitTest::GetRandInt(const int randMin, const int randMax)
{
    std::random_device randDev;
    std::mt19937 genRand(randDev());
    std::uniform_int_distribution<int> disRand(randMin, randMax);
    return disRand(genRand);
}

int64_t DistributedDBToolsUnitTest::GetRandInt64(const int64_t randMin, const int64_t randMax)
{
    std::random_device randDev;
    std::mt19937_64 genRand(randDev());
    std::uniform_int_distribution<int64_t> disRand(randMin, randMax);
    return disRand(genRand);
}

void DistributedDBToolsUnitTest::PrintTestCaseInfo()
{
    testing::UnitTest *test = testing::UnitTest::GetInstance();
    ASSERT_NE(test, nullptr);
    const testing::TestInfo *testInfo = test->current_test_info();
    ASSERT_NE(testInfo, nullptr);
    LOGI("Start unit test: %s.%s", testInfo->test_case_name(), testInfo->name());
}

int DistributedDBToolsUnitTest::BuildMessage(const DataSyncMessageInfo &messageInfo,
    DistributedDB::Message *&message)
{
    auto packet = new (std::nothrow) DataRequestPacket;
    if (packet == nullptr) {
        return -E_OUT_OF_MEMORY;
    }
    message = new (std::nothrow) Message(messageInfo.messageId_);
    if (message == nullptr) {
        delete packet;
        packet = nullptr;
        return -E_OUT_OF_MEMORY;
    }
    packet->SetBasicInfo(messageInfo.sendCode_, messageInfo.version_, messageInfo.mode_);
    packet->SetWaterMark(messageInfo.localMark_, messageInfo.peerMark_, messageInfo.deleteMark_);
    std::vector<uint64_t> reserved {messageInfo.packetId_};
    packet->SetReserved(reserved);
    message->SetMessageType(messageInfo.messageType_);
    message->SetSessionId(messageInfo.sessionId_);
    message->SetSequenceId(messageInfo.sequenceId_);
    message->SetExternalObject(packet);
    return E_OK;
}

sqlite3 *RelationalTestUtils::CreateDataBase(const std::string &dbUri)
{
    LOGD("Create database: %s", dbUri.c_str());
    sqlite3 *db = nullptr;
    if (int r = sqlite3_open_v2(dbUri.c_str(), &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr) != SQLITE_OK) {
        LOGE("Open database [%s] failed. %d", dbUri.c_str(), r);
        if (db != nullptr) {
            (void)sqlite3_close_v2(db);
            db = nullptr;
        }
    }
    return db;
}

int RelationalTestUtils::ExecSql(sqlite3 *db, const std::string &sql)
{
    if (db == nullptr || sql.empty()) {
        return -E_INVALID_ARGS;
    }
    char *errMsg = nullptr;
    int errCode = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &errMsg);
    if (errCode != SQLITE_OK && errMsg != nullptr) {
        LOGE("Execute sql failed. %d err: %s", errCode, errMsg);
    }
    sqlite3_free(errMsg);
    return errCode;
}

int RelationalTestUtils::ExecSql(sqlite3 *db, const std::string &sql,
    const std::function<int (sqlite3_stmt *)> &bindCallback, const std::function<int (sqlite3_stmt *)> &resultCallback)
{
    if (db == nullptr || sql.empty()) {
        return -E_INVALID_ARGS;
    }

    bool bindFinish = true;
    sqlite3_stmt *stmt = nullptr;
    int errCode = SQLiteUtils::GetStatement(db, sql, stmt);
    if (errCode != E_OK) {
        goto END;
    }

    do {
        if (bindCallback) {
            errCode = bindCallback(stmt);
            if (errCode != E_OK && errCode != -E_UNFINISHED) {
                goto END;
            }
            bindFinish = (errCode != -E_UNFINISHED); // continue bind if unfinished
        }

        while (true) {
            errCode = SQLiteUtils::StepWithRetry(stmt);
            if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_DONE)) {
                errCode = E_OK; // Step finished
                break;
            } else if (errCode != SQLiteUtils::MapSQLiteErrno(SQLITE_ROW)) {
                goto END; // Step return error
            }
            if (resultCallback == nullptr) {
                continue;
            }
            errCode = resultCallback(stmt);
            if (errCode != E_OK) {
                goto END;
            }
        }
        SQLiteUtils::ResetStatement(stmt, false, errCode);
    } while (!bindFinish);

END:
    SQLiteUtils::ResetStatement(stmt, true, errCode);
    return errCode;
}

void RelationalTestUtils::CreateDeviceTable(sqlite3 *db, const std::string &table, const std::string &device)
{
    ASSERT_NE(db, nullptr);
    std::string deviceTable = DBCommon::GetDistributedTableName(device, table);
    TableInfo baseTbl;
    ASSERT_EQ(SQLiteUtils::AnalysisSchema(db, table, baseTbl), E_OK);
    EXPECT_EQ(SQLiteUtils::CreateSameStuTable(db, baseTbl, deviceTable), E_OK);
    EXPECT_EQ(SQLiteUtils::CloneIndexes(db, table, deviceTable), E_OK);
}

int RelationalTestUtils::CheckSqlResult(sqlite3 *db, const std::string &sql, bool &result)
{
    if (db == nullptr || sql.empty()) {
        return -E_INVALID_ARGS;
    }
    sqlite3_stmt *stmt = nullptr;
    int errCode = SQLiteUtils::GetStatement(db, sql, stmt);
    if (errCode != E_OK) {
        goto END;
    }

    errCode = SQLiteUtils::StepWithRetry(stmt);
    if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_ROW)) {
        result = true;
        errCode = E_OK;
    } else if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_DONE)) {
        result = false;
        errCode = E_OK;
    }
END:
    SQLiteUtils::ResetStatement(stmt, true, errCode);
    return errCode;
}

int RelationalTestUtils::CheckTableRecords(sqlite3 *db, const std::string &table)
{
    if (db == nullptr || table.empty()) {
        return -E_INVALID_ARGS;
    }
    int count = -1;
    std::string sql = "select count(1) from " + table + ";";

    sqlite3_stmt *stmt = nullptr;
    int errCode = SQLiteUtils::GetStatement(db, sql, stmt);
    if (errCode != E_OK) {
        goto END;
    }

    errCode = SQLiteUtils::StepWithRetry(stmt);
    if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_ROW)) {
        count = sqlite3_column_int(stmt, 0);
    }
END:
    SQLiteUtils::ResetStatement(stmt, true, errCode);
    return count;
}

int RelationalTestUtils::GetMetaData(sqlite3 *db, const DistributedDB::Key &key, DistributedDB::Value &value)
{
    if (db == nullptr) {
        return -E_INVALID_ARGS;
    }

    std::string sql = "SELECT value FROM " + DBConstant::RELATIONAL_PREFIX + "metadata WHERE key = ?;";
    sqlite3_stmt *stmt = nullptr;
    int errCode = SQLiteUtils::GetStatement(db, sql, stmt);
    if (errCode != E_OK) {
        goto END;
    }
    errCode = SQLiteUtils::BindBlobToStatement(stmt, 1, key);
    if (errCode != E_OK) {
        goto END;
    }

    errCode = SQLiteUtils::StepWithRetry(stmt);
    if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_ROW)) {
        errCode = SQLiteUtils::GetColumnBlobValue(stmt, 0, value);
    }
END:
    SQLiteUtils::ResetStatement(stmt, true, errCode);
    return errCode;
}

int RelationalTestUtils::SetMetaData(sqlite3 *db, const DistributedDB::Key &key, const DistributedDB::Value &value)
{
    if (db == nullptr) {
        return -E_INVALID_ARGS;
    }

    std::string sql = "INSERT OR REPLACE INTO " + DBConstant::RELATIONAL_PREFIX + "metadata VALUES (?, ?);";
    sqlite3_stmt *stmt = nullptr;
    int errCode = SQLiteUtils::GetStatement(db, sql, stmt);
    if (errCode != E_OK) {
        goto END;
    }
    errCode = SQLiteUtils::BindBlobToStatement(stmt, 1, key);
    if (errCode != E_OK) {
        goto END;
    }
    errCode = SQLiteUtils::BindBlobToStatement(stmt, 2, value); // 2: bind index
    if (errCode != E_OK) {
        goto END;
    }

    errCode = SQLiteUtils::StepWithRetry(stmt);
    if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_DONE)) {
        errCode = E_OK;
    }
END:
    SQLiteUtils::ResetStatement(stmt, true, errCode);
    return SQLiteUtils::MapSQLiteErrno(errCode);
}

void RelationalTestUtils::CloudBlockSync(const DistributedDB::Query &query,
    DistributedDB::RelationalStoreDelegate *delegate, DistributedDB::DBStatus expect,
    DistributedDB::DBStatus callbackExpect)
{
    ASSERT_NE(delegate, nullptr);
    std::mutex dataMutex;
    std::condition_variable cv;
    bool finish = false;
    auto callback = [callbackExpect, &cv, &dataMutex, &finish](const std::map<std::string, SyncProcess> &process) {
        for (const auto &item: process) {
            if (item.second.process == DistributedDB::FINISHED) {
                {
                    std::lock_guard<std::mutex> autoLock(dataMutex);
                    finish = true;
                }
                EXPECT_EQ(item.second.errCode, callbackExpect);
                cv.notify_one();
            }
        }
    };
    ASSERT_EQ(delegate->Sync({ "CLOUD" }, SYNC_MODE_CLOUD_MERGE, query, callback, DBConstant::MAX_TIMEOUT), expect);
    if (expect != DistributedDB::DBStatus::OK) {
        return;
    }
    std::unique_lock<std::mutex> uniqueLock(dataMutex);
    cv.wait(uniqueLock, [&finish]() {
        return finish;
    });
}

int RelationalTestUtils::SelectData(sqlite3 *db, const DistributedDB::TableSchema &schema,
    std::vector<DistributedDB::VBucket> &data)
{
    LOGD("[RelationalTestUtils] Begin select data");
    int errCode = E_OK;
    std::string selectSql = "SELECT ";
    for (const auto &field : schema.fields) {
        selectSql += field.colName + ",";
    }
    selectSql.pop_back();
    selectSql += " FROM " + schema.name;
    sqlite3_stmt *statement = nullptr;
    errCode = SQLiteUtils::GetStatement(db, selectSql, statement);
    if (errCode != E_OK) {
        LOGE("[RelationalTestUtils] Prepare statement failed %d", errCode);
        return errCode;
    }
    do {
        errCode = SQLiteUtils::StepWithRetry(statement, false);
        errCode = (errCode == -SQLITE_ROW) ? E_OK :
            (errCode == -SQLITE_DONE) ? -E_FINISHED : errCode;
        if (errCode != E_OK) {
            break;
        }
        VBucket rowData;
        for (size_t index = 0; index < schema.fields.size(); ++index) {
            Type colValue;
            int ret = SQLiteRelationalUtils::GetCloudValueByType(statement, schema.fields[index].type, index, colValue);
            if (ret != E_OK) {
                LOGE("[RelationalTestUtils] Get col value failed %d", ret);
                break;
            }
            rowData[schema.fields[index].colName] = colValue;
        }
        data.push_back(rowData);
    } while (errCode == E_OK);
    if (errCode == -E_FINISHED) {
        errCode = E_OK;
    }
    int err = E_OK;
    SQLiteUtils::ResetStatement(statement, true, err);
    LOGW("[RelationalTestUtils] Select data finished errCode %d", errCode);
    return errCode != E_OK ? errCode : err;
}

DistributedDB::Assets RelationalTestUtils::GetAssets(const DistributedDB::Type &value,
    const std::shared_ptr<DistributedDB::ICloudDataTranslate> &translate, bool isAsset)
{
    DistributedDB::Assets assets;
    if (value.index() == TYPE_INDEX<Assets>) {
        auto tmp = std::get<Assets>(value);
        assets.insert(assets.end(), tmp.begin(), tmp.end());
    } else if (value.index() == TYPE_INDEX<Asset>) {
        assets.push_back(std::get<Asset>(value));
    } else if (value.index() == TYPE_INDEX<Bytes> && translate != nullptr) {
        if (isAsset) {
            auto tmpAsset = translate->BlobToAsset(std::get<Bytes>(value));
            assets.push_back(tmpAsset);
        } else {
            auto tmpAssets = translate->BlobToAssets(std::get<Bytes>(value));
            assets.insert(assets.end(), tmpAssets.begin(), tmpAssets.end());
        }
    }
    return assets;
}

DistributedDB::DBStatus RelationalTestUtils::InsertCloudRecord(int64_t begin, int64_t count,
    const std::string &tableName, const std::shared_ptr<DistributedDB::VirtualCloudDb> &cloudDbPtr, int32_t assetCount)
{
    if (cloudDbPtr == nullptr) {
        LOGE("[RelationalTestUtils] Not support insert cloud with null");
        return DistributedDB::DBStatus::DB_ERROR;
    }
    std::vector<VBucket> record;
    std::vector<VBucket> extend;
    Timestamp now = DistributedDB::TimeHelper::GetSysCurrentTime();
    for (int64_t i = begin; i < (begin + count); ++i) {
        VBucket data;
        data.insert_or_assign("id", std::to_string(i));
        data.insert_or_assign("name", "Cloud" + std::to_string(i));
        Assets assets;
        std::string assetNameBegin = "Phone" + std::to_string(i);
        for (int j = 1; j <= assetCount; ++j) {
            Asset asset;
            asset.name = assetNameBegin + "_" + std::to_string(j);
            asset.status = AssetStatus::INSERT;
            asset.hash = "DEC";
            assets.push_back(asset);
        }
        data.insert_or_assign("assets", assets);
        record.push_back(data);
        VBucket log;
        log.insert_or_assign(DistributedDB::CloudDbConstant::CREATE_FIELD, static_cast<int64_t>(
            now / DistributedDB::CloudDbConstant::TEN_THOUSAND));
        log.insert_or_assign(DistributedDB::CloudDbConstant::MODIFY_FIELD, static_cast<int64_t>(
            now / DistributedDB::CloudDbConstant::TEN_THOUSAND));
        log.insert_or_assign(DistributedDB::CloudDbConstant::DELETE_FIELD, false);
        extend.push_back(log);
    }
    return cloudDbPtr->BatchInsert(tableName, std::move(record), extend);
}

std::vector<DistributedDB::Assets> RelationalTestUtils::GetAllAssets(sqlite3 *db,
    const DistributedDB::TableSchema &schema, const std::shared_ptr<DistributedDB::ICloudDataTranslate> &translate)
{
    std::vector<DistributedDB::Assets> res;
    if (db == nullptr || translate == nullptr) {
        LOGW("[RelationalTestUtils] DB or translate is null");
        return res;
    }
    std::vector<VBucket> allData;
    EXPECT_EQ(RelationalTestUtils::SelectData(db, schema, allData), E_OK);
    std::map<std::string, int32_t> assetFields;
    for (const auto &field : schema.fields) {
        if (field.type != TYPE_INDEX<Asset> && field.type != TYPE_INDEX<Assets>) {
            continue;
        }
        assetFields[field.colName] = field.type;
    }
    for (const auto &oneRow : allData) {
        Assets assets;
        for (const auto &[col, data] : oneRow) {
            if (assetFields.find(col) == assetFields.end()) {
                continue;
            }
            auto tmpAssets = GetAssets(data, translate, (assetFields[col] == TYPE_INDEX<Asset>));
            assets.insert(assets.end(), tmpAssets.begin(), tmpAssets.end());
        }
        res.push_back(assets);
    }
    return res;
}

int RelationalTestUtils::GetRecordLog(sqlite3 *db, const std::string &tableName,
    std::vector<DistributedDB::VBucket> &records)
{
    DistributedDB::TableSchema schema;
    schema.name = DBCommon::GetLogTableName(tableName);
    Field field;
    field.type = TYPE_INDEX<int64_t>;
    field.colName = "data_key";
    schema.fields.push_back(field);
    field.colName = "flag";
    schema.fields.push_back(field);
    field.colName = "cursor";
    schema.fields.push_back(field);
    field.colName = "cloud_gid";
    field.type = TYPE_INDEX<std::string>;
    schema.fields.push_back(field);
    return SelectData(db, schema, records);
}

int RelationalTestUtils::DeleteRecord(sqlite3 *db, const std::string &tableName,
    const std::vector<std::map<std::string, std::string>> &conditions)
{
    if (db == nullptr || tableName.empty()) {
        LOGE("[RelationalTestUtils] db is null or table is empty");
        return -E_INVALID_ARGS;
    }
    int errCode = E_OK;
    for (const auto &condition : conditions) {
        std::string deleteSql = "DELETE FROM " + tableName + " WHERE ";
        int count = 0;
        for (const auto &[col, value] : condition) {
            if (count > 0) {
                deleteSql += " AND ";
            }
            deleteSql += col + "=" + value;
            count++;
        }
        LOGD("[RelationalTestUtils] Sql is %s", deleteSql.c_str());
        errCode = ExecSql(db, deleteSql);
        if (errCode != E_OK) {
            return errCode;
        }
    }
    return errCode;
}

bool DBInfoHandleTest::IsSupport()
{
    std::lock_guard<std::mutex> autoLock(supportMutex_);
    return localIsSupport_;
}

bool DBInfoHandleTest::IsNeedAutoSync(const std::string &userId, const std::string &appId, const std::string &storeId,
    const DeviceInfos &devInfo)
{
    std::lock_guard<std::mutex> autoLock(autoSyncMutex_);
    return isNeedAutoSync_;
}

void DBInfoHandleTest::SetLocalIsSupport(bool isSupport)
{
    std::lock_guard<std::mutex> autoLock(supportMutex_);
    localIsSupport_ = isSupport;
}

void DBInfoHandleTest::SetNeedAutoSync(bool needAutoSync)
{
    std::lock_guard<std::mutex> autoLock(autoSyncMutex_);
    isNeedAutoSync_ = needAutoSync;
}
} // namespace DistributedDBUnitTest
