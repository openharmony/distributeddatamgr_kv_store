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

#include "single_ver_utils.h"

#include "db_common.h"
#include "platform_specific.h"
#include "runtime_context.h"

namespace DistributedDB {

int GetPathSecurityOption(const std::string &filePath, SecurityOption &secOpt)
{
    return RuntimeContext::GetInstance()->GetSecurityOption(filePath, secOpt);
}

std::string GetDbDir(const std::string &subDir, DbType type)
{
    switch (type) {
        case DbType::MAIN:
            return subDir + "/" + DBConstant::MAINDB_DIR;
        case DbType::META:
            return subDir + "/" + DBConstant::METADB_DIR;
        case DbType::CACHE:
            return subDir + "/" + DBConstant::CACHEDB_DIR;
        default:
            break;
    }
    return "";
}

std::string GetDatabasePath(const KvDBProperties &kvDBProp)
{
    return GetSubDirPath(kvDBProp) + "/" + DBConstant::MAINDB_DIR + "/" + DBConstant::SINGLE_VER_DATA_STORE +
        DBConstant::DB_EXTENSION;
}

std::string GetSubDirPath(const KvDBProperties &kvDBProp)
{
    std::string dataDir = kvDBProp.GetStringProp(KvDBProperties::DATA_DIR, "");
    std::string identifierDir = kvDBProp.GetStringProp(KvDBProperties::IDENTIFIER_DIR, "");
    return dataDir + "/" + identifierDir + "/" + DBConstant::SINGLE_SUB_DIR;
}

int ClearIncompleteDatabase(const KvDBProperties &kvDBPro)
{
    std::string dbSubDir = GetSubDirPath(kvDBPro);
    if (OS::CheckPathExistence(dbSubDir + DBConstant::PATH_POSTFIX_DB_INCOMPLETE)) {
        int errCode = DBCommon::RemoveAllFilesOfDirectory(dbSubDir);
        if (errCode != E_OK) {
            LOGE("Remove the incomplete database dir failed!");
            return -E_REMOVE_FILE;
        }
    }
    return E_OK;
}

int CreateNewDirsAndSetSecOption(const OpenDbProperties &option)
{
    std::vector<std::string> dbDir { DBConstant::MAINDB_DIR, DBConstant::METADB_DIR, DBConstant::CACHEDB_DIR };
    for (const auto &item : dbDir) {
        if (OS::CheckPathExistence(option.subdir + "/" + item)) {
            continue;
        }

        // Dir and old db file not existed, it means that the database is newly created
        // need create flag of database not incomplete
        if (!OS::CheckPathExistence(option.subdir + DBConstant::PATH_POSTFIX_DB_INCOMPLETE) &&
            !OS::CheckPathExistence(option.subdir + "/" + DBConstant::SINGLE_VER_DATA_STORE +
            DBConstant::DB_EXTENSION) &&
            OS::CreateFileByFileName(option.subdir + DBConstant::PATH_POSTFIX_DB_INCOMPLETE) != E_OK) {
            LOGE("Fail to create the token of database incompleted! errCode = [E_SYSTEM_API_FAIL]");
            return -E_SYSTEM_API_FAIL;
        }

        if (DBCommon::CreateDirectory(option.subdir + "/" + item) != E_OK) {
            LOGE("Create sub-directory for single ver failed, errno:%d", errno);
            return -E_SYSTEM_API_FAIL;
        }

        if (option.securityOpt.securityLabel == NOT_SET) {
            continue;
        }

        SecurityOption secOption = option.securityOpt;
        if (item == DBConstant::METADB_DIR) {
            secOption.securityLabel = ((option.securityOpt.securityLabel >= SecurityLabel::S2) ?
                SecurityLabel::S2 : option.securityOpt.securityLabel);
            secOption.securityFlag = SecurityFlag::ECE;
        }

        int errCode = RuntimeContext::GetInstance()->SetSecurityOption(option.subdir + "/" + item, secOption);
        if (errCode != E_OK && errCode != -E_NOT_SUPPORT) {
            LOGE("Set the security option of sub-directory failed[%d]", errCode);
            return errCode;
        }
    }
    return E_OK;
}

int GetExistedSecOpt(const OpenDbProperties &opt, SecurityOption &secOption)
{
    // Check the existence of the database, include the origin database and the database in the 'main' directory.
    auto mainDbDir = GetDbDir(opt.subdir, DbType::MAIN);
    auto mainDbFilePath = mainDbDir + "/" + DBConstant::SINGLE_VER_DATA_STORE + DBConstant::DB_EXTENSION;
    auto origDbFilePath = opt.subdir + "/" + DBConstant::SINGLE_VER_DATA_STORE + DBConstant::DB_EXTENSION;
    if (!OS::CheckPathExistence(origDbFilePath) && !OS::CheckPathExistence(mainDbFilePath)) {
        secOption = opt.securityOpt;
        return E_OK;
    }

    // the main database file has high priority of the security option.
    int errCode;
    if (OS::CheckPathExistence(mainDbFilePath)) {
        errCode = GetPathSecurityOption(mainDbFilePath, secOption);
    } else {
        errCode = GetPathSecurityOption(origDbFilePath, secOption);
    }
    if (errCode == E_OK) {
        return E_OK;
    }
    secOption = SecurityOption();
    if (errCode == -E_NOT_SUPPORT) {
        return E_OK;
    }
    LOGE("Get the security option of the existed database failed.");
    return errCode;
}

void InitCommitNotifyDataKeyStatus(SingleVerNaturalStoreCommitNotifyData *committedData, const Key &hashKey,
    const DataOperStatus &dataStatus)
{
    if (committedData == nullptr) {
        return;
    }

    ExistStatus existedStatus = ExistStatus::NONE;
    if (dataStatus.preStatus == DataStatus::DELETED) {
        existedStatus = ExistStatus::DELETED;
    } else if (dataStatus.preStatus == DataStatus::EXISTED) {
        existedStatus = ExistStatus::EXIST;
    }

    committedData->InitKeyPropRecord(hashKey, existedStatus);
}
} // namespace DistributedDB