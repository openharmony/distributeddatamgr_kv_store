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
#ifndef DATA_TRANSFORMER_H
#define DATA_TRANSFORMER_H
#ifdef RELATIONAL_STORE

#include <vector>

#include "cloud/cloud_store_types.h"
#include "data_value.h"
#include "db_types.h"
#include "relational_schema_object.h"

namespace DistributedDB {
using RowData = std::vector<DataValue>;
using OptRowData = std::vector<DataValue>;

struct LogInfo {
    int64_t dataKey = -1;
    std::string device;
    std::string originDev;
    Timestamp timestamp = 0;
    Timestamp wTimestamp = 0;
    uint64_t flag = 0;
    Key hashKey; // primary key hash value
    std::string cloudGid; // use for sync with cloud
    std::string sharingResource; // use for cloud share data
    std::string version; // use for conflict check
    uint32_t status = static_cast<uint32_t>(LockStatus::UNLOCK); // record lock status
    bool isNeedUpdateAsset = false;
    uint64_t cloud_flag = 0; // use for kv cloud log table
};

enum class LogInfoFlag : uint32_t {
    FLAG_CLOUD = 0x0, // same as device sync
    FLAG_DELETE = 0x1,
    FLAG_LOCAL = 0x2,
    FLAG_FORCE_PUSH_IGNORE = 0x4, // use in RDB
    FLAG_LOGIC_DELETE = 0x8, // use in RDB
    FLAG_WAIT_COMPENSATED_SYNC = 0x10,
    FLAG_DEVICE_CLOUD_INCONSISTENCY = 0x20,
    FLAG_KV_FORCE_PUSH_IGNORE = 0x40,
    FLAG_KV_LOGIC_DELETE = 0x80,
    FLAG_CLOUD_WRITE = 0x100,
    FLAG_SYSTEM_RECORD = 0x200,
    FLAG_UPLOAD_FINISHED = 0x400,
    FLAG_LOGIC_DELETE_FOR_LOGOUT = 0x800,
    FLAG_ASSET_DOWNLOADING_FOR_ASYNC = 0x1000,
    FLAG_LOGIN_USER = 0x2000, // same hash key, login user's data
    FLAG_CLOUD_UPDATE_LOCAL = 0x4000,  // 1 indicates an update on the cloud side, and 0 indicates data inserted on the
                                       // cloud side or data operated locally
    FLAG_KNOWLEDGE_INVERTED_WRITE = 0x8000, // knowledge process written to inverted table
    FLAG_KNOWLEDGE_VECTOR_WRITE = 0x10000,  // knowledge process written to vector table
};

struct RowDataWithLog {
    LogInfo logInfo;
    RowData rowData;
};

struct OptRowDataWithLog {
    LogInfo logInfo;
    OptRowData optionalData;
};

struct TableDataWithLog {
    std::string tableName;
    std::vector<RowDataWithLog> dataList;
};

struct OptTableDataWithLog {
    std::string tableName;
    std::vector<OptRowDataWithLog> dataList;
};

// use for cloud sync
struct DataInfoWithLog {
    LogInfo logInfo;
    VBucket primaryKeys;
};

struct UpdateRecordFlagStruct {
    std::string tableName;
    bool isRecordConflict;
    bool isInconsistency;
};

struct DeviceSyncSaveDataInfo {
    bool isDefeated = false; // data is defeated by local
    bool isExist = false;
    int64_t rowId = -1;
    LogInfo localLogInfo;
};

class DataTransformer {
public:
    static int TransformTableData(const TableDataWithLog &tableDataWithLog,
        const std::vector<FieldInfo> &fieldInfoList, std::vector<DataItem> &dataItems);
    static int TransformDataItem(const std::vector<DataItem> &dataItems, const std::vector<FieldInfo> &remoteFieldInfo,
        OptTableDataWithLog &tableDataWithLog);

    static int SerializeDataItem(const RowDataWithLog &data, const std::vector<FieldInfo> &fieldInfo,
        DataItem &dataItem);
    static int DeSerializeDataItem(const DataItem &dataItem, OptRowDataWithLog &data,
        const std::vector<FieldInfo> &remoteFieldInfo);

    static uint32_t CalDataValueLength(const DataValue &dataValue);
    static int DeserializeDataValue(DataValue &dataValue, Parcel &parcel);
    static int SerializeDataValue(const DataValue &dataValue, Parcel &parcel);

private:
    static int SerializeValue(Value &value, const RowData &rowData, const std::vector<FieldInfo> &fieldInfoList);
    static int DeSerializeValue(const Value &value, OptRowData &optionalData,
        const std::vector<FieldInfo> &remoteFieldInfo);
};
}

#endif
#endif // DATA_TRANSFORMER_H
