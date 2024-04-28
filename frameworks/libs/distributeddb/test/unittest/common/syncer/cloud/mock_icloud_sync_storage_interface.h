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
#ifndef MOCK_ICLOUD_SYNC_STORAGE_INTERFACE_H
#define MOCK_ICLOUD_SYNC_STORAGE_INTERFACE_H

#include <gmock/gmock.h>
#include "icloud_sync_storage_interface.h"

namespace DistributedDB {
class MockICloudSyncStorageInterface : public ICloudSyncStorageInterface {
public:
    MOCK_CONST_METHOD2(GetMetaData, int(const Key &, Value &));
    MOCK_METHOD2(PutMetaData, int(const Key &, const Value &));
    MOCK_METHOD1(ChkSchema, int(const TableName &));
    MOCK_METHOD1(SetCloudDbSchema, int(const DataBaseSchema &));
    MOCK_METHOD1(GetCloudDbSchema, int(std::shared_ptr<DataBaseSchema> &));
    MOCK_METHOD2(GetCloudTableSchema, int(const TableName &, TableSchema &));
    MOCK_METHOD1(StartTransaction, int(TransactType));
    MOCK_METHOD0(Commit, int(void));
    MOCK_METHOD0(Rollback, int(void));
    MOCK_METHOD5(GetUploadCount, int(const QuerySyncObject &, const Timestamp &, bool, bool, int64_t &));
    MOCK_METHOD5(GetAllUploadCount, int(const QuerySyncObject &, const std::vector<Timestamp> &, bool,
        bool, int64_t &));
    MOCK_METHOD5(GetCloudData, int(const TableSchema &, const QuerySyncObject &, const Timestamp &, ContinueToken &,
        CloudSyncData &));
    MOCK_METHOD2(GetCloudDataNext, int(ContinueToken &, CloudSyncData &));
    MOCK_METHOD5(GetCloudGid, int(const TableSchema &, const QuerySyncObject &, bool, bool,
        std::vector<std::string> &));
    MOCK_METHOD1(ReleaseCloudDataToken, int(ContinueToken &));
    MOCK_METHOD4(GetInfoByPrimaryKeyOrGid, int(const std::string &, const VBucket &, DataInfoWithLog &, VBucket &));
    MOCK_METHOD2(PutCloudSyncData, int(const std::string &, DownloadData &));
    MOCK_METHOD3(TriggerObserverAction, void(const std::string &, ChangedData &&, bool));
    MOCK_METHOD4(CleanCloudData, int(ClearMode mode, const std::vector<std::string> &tableNameList,
        const RelationalSchemaObject &localSchema, std::vector<Asset> &assets));
    MOCK_METHOD3(FillCloudAssetForDownload, int(const std::string &, VBucket &, bool));
    MOCK_METHOD1(SetLogTriggerStatus, int(bool));
    MOCK_METHOD4(FillCloudLogAndAsset, int(OpType, const CloudSyncData &, bool, bool));
    MOCK_CONST_METHOD0(GetIdentify, std::string());
    MOCK_METHOD1(CheckQueryValid, int(const QuerySyncObject &));
    MOCK_METHOD1(IsSharedTable, bool(const std::string &));
};

}
#endif // #define MOCK_ICLOUD_SYNC_STORAGE_INTERFACE_H
