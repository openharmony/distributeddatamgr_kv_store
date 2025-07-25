/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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
#include "rd_single_ver_storage_engine.h"

#include "db_constant.h"
#include "grd_base/grd_error.h"
#include "grd_base/grd_type_export.h"
#include "rd_single_ver_storage_executor.h"
#include "rd_utils.h"
#include "single_ver_utils.h"
#include "sqlite_single_ver_storage_executor_sql.h"

namespace DistributedDB {

RdSingleVerStorageEngine::RdSingleVerStorageEngine()
{
    LOGD("[RdSingleVerStorageEngine] RdSingleVerStorageEngine Created");
}

RdSingleVerStorageEngine::~RdSingleVerStorageEngine()
{
}

inline std::string GetTableMode(bool isHash)
{
    return isHash ? DBConstant::RD_KV_HASH_COLLECTION_MODE : DBConstant::RD_KV_COLLECTION_MODE;
}

int RdSingleVerStorageEngine::CreateNewExecutor(bool isWrite, StorageExecutor *&handle)
{
    int ret = PreCreateExecutor(isWrite);
    if (ret != E_OK) {
        LOGE("[RdSingleVerStorageEngine][CreateNewExecutor] PreCreateExecutor unscuccess");
        return ret;
    }
    GRD_DB *db = nullptr;
    auto option = GetOption();
    ret = TryToOpenMainDatabase(isWrite, db, option);
    if (ret != E_OK) {
        LOGE("[RdSingleVerStorageEngine] GRD_DBOPEN FAILED:%d", ret);
        return ret;
    }
    if (!option.readOnly) {
        std::string tableMode = GetTableMode(option.isHashTable);
        ret = RdCreateCollection(db, SYNC_COLLECTION_NAME, tableMode.c_str(), 0);
        if (ret != E_OK) {
            LOGE("[RdSingleVerStorageEngine] GRD_CreateCollection SYNC_COLLECTION_NAME FAILED %d", ret);
            (void)RdDBClose(db, GRD_DB_CLOSE_IGNORE_ERROR);
            return ret;
        }
    }
    ret = IndexPreLoad(db, SYNC_COLLECTION_NAME);
    if (ret != E_OK) {
        LOGE("[RdSingleVerStorageEngine] GRD_IndexPreload FAILED %d", ret);
        (void)RdDBClose(db, GRD_DB_CLOSE_IGNORE_ERROR);
        return ret;
    }
    handle = new (std::nothrow) RdSingleVerStorageExecutor(db, isWrite);
    if (handle == nullptr) {
        (void)RdDBClose(db, GRD_DB_CLOSE_IGNORE_ERROR);
        return -E_OUT_OF_MEMORY;
    }
    if (OS::CheckPathExistence(option.subdir + DBConstant::PATH_POSTFIX_DB_INCOMPLETE) &&
        OS::RemoveFile(option.subdir + DBConstant::PATH_POSTFIX_DB_INCOMPLETE) != E_OK) {
        LOGE("Finish to create the complete database, but delete token fail! errCode = [E_SYSTEM_API_FAIL]");
        delete handle;
        handle = nullptr;
        return -E_SYSTEM_API_FAIL;
    }
    return E_OK;
}

int RdSingleVerStorageEngine::InitRdStorageEngine(const StorageEngineAttr &poolSize, const OpenDbProperties &option,
    const std::string &identifier)
{
    if (StorageEngine::CheckEngineAttr(poolSize)) {
        LOGE("Invalid storage engine attributes!");
        return -E_INVALID_ARGS;
    }
    engineAttr_ = poolSize;
    option_ = option;
    identifier_ = identifier;
    hashIdentifier_ = DBCommon::TransferStringToHex(identifier_);
    int errCode = Init(true);
    if (errCode != E_OK) {
        LOGI("Storage engine init fail! errCode = [%d]", errCode);
    }
    return errCode;
}

int RdSingleVerStorageEngine::GetExistedSecOption(SecurityOption &secOption) const
{
    LOGD("[RdSingleVerStorageEngine] Try to get existed sec option");
    return GetExistedSecOpt(option_, secOption);
}

int RdSingleVerStorageEngine::CheckDatabaseSecOpt(const SecurityOption &secOption) const
{
    if (!(secOption == option_.securityOpt) && (secOption.securityLabel > option_.securityOpt.securityLabel) &&
        secOption.securityLabel != SecurityLabel::NOT_SET &&
        option_.securityOpt.securityLabel != SecurityLabel::NOT_SET) {
        LOGE("[RdSingleVerStorageEngine] SecurityOption mismatch, existed:[%d-%d] vs input:[%d-%d]",
            secOption.securityLabel, secOption.securityFlag, option_.securityOpt.securityLabel,
            option_.securityOpt.securityFlag);
        return -E_SECURITY_OPTION_CHECK_ERROR;
    }
    return E_OK;
}

int RdSingleVerStorageEngine::CreateNewDirsAndSetSecOpt() const
{
    LOGD("[RdSingleVerStorageEngine] Begin to create new dirs and set security option");
    return CreateNewDirsAndSetSecOption(option_);
}

int RdSingleVerStorageEngine::TryToOpenMainDatabase(bool isWrite, GRD_DB *&db, OpenDbProperties &optionTemp)
{
    // Only could get the main database handle in the uninitialized and the main status.
    if (GetEngineState() != EngineState::INVALID && GetEngineState() != EngineState::MAINDB) {
        LOGE("[RdSinStoreEng][GetMainHandle] Can only create new handle for state[%d]", GetEngineState());
        return -E_EKEYREVOKED;
    }

    optionTemp.uri = GetDbDir(optionTemp.subdir, DbType::MAIN) + "/" + DBConstant::SINGLE_VER_DATA_STORE +
        DBConstant::DB_EXTENSION;
    SetUri(optionTemp.uri);

    if (!isWrite) {
        optionTemp.createIfNecessary = false;
    }
    int errCode = OpenGrdDb(optionTemp, db);
    if (errCode != E_OK) {
        LOGE("Failed to open the main database [%d]", errCode);
        return errCode;
    }

    // Set the engine state to main status for that the main database is valid.
    SetEngineState(EngineState::MAINDB);
    return errCode;
}

int RdSingleVerStorageEngine::PreCreateExecutor(bool isWrite)
{
    if (!isWrite) {
        return E_OK;
    }
    // Get the existed database secure option.
    SecurityOption existedSecOpt;
    int ret = GetExistedSecOption(existedSecOpt);
    if (ret != E_OK) {
        LOGD("[RdSingleVerStorageEngine][PreCreateExecutor]Something unexpected happened");
        return ret;
    }
    ret = CheckDatabaseSecOpt(existedSecOpt);
    if (ret != E_OK) {
        LOGD("[RdSingleVerStorageEngine][CheckDatabaseSecOpt]Something unexpected happened");
        return ret;
    }
    ret = CreateNewDirsAndSetSecOpt();
    if (ret != E_OK) {
        LOGD("[RdSingleVerStorageEngine][CreateNewDirsAndSetSecOpt]Something unexpected happened");
    }
    return ret;
}

int RdSingleVerStorageEngine::OpenGrdDb(const OpenDbProperties &option, GRD_DB *&db)
{
    uint32_t flag = GRD_DB_OPEN_ONLY;
    if (option.createIfNecessary && !option.readOnly) {
        flag |= GRD_DB_OPEN_CREATE;
    }
    if (option.readOnly) {
        flag |= GRD_DB_OPEN_SHARED_READ_ONLY;
    }
    int errCode = RdDbOpen(option.uri.c_str(), option.rdConfig.c_str(), flag, db);
    if (errCode == -E_REBUILD_DATABASE) {
        if (option.isNeedRmCorruptedDb) {
            LOGD("[RdSingleVerStorageEngine] rebuild database successfully");
            return E_OK;
        } else {
            LOGE("[RdSingleVerStorageEngine] database is corrupted");
            return -E_INVALID_PASSWD_OR_CORRUPTED_DB;
        }
    }
    return errCode;
}

int RdSingleVerStorageEngine::IndexPreLoad(GRD_DB *&db, const char *collectionName)
{
    if (isFirstTimeOpenDb_) {
        int ret = RdIndexPreload(db, collectionName);
        if (ret != E_OK) {
            LOGE("Unable to RdIndexPreload %d", ret);
            return ret;
        }
        isFirstTimeOpenDb_ = false;
    }
    return E_OK;
}
} // namespace DistributedDB