/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#define LOG_TAG "ITypesUtil"

#include "itypes_util.h"
#include "iremote_object.h"
#include "log_print.h"

namespace OHOS {
namespace DistributedKv {
bool ITypesUtil::Marshal(MessageParcel &data)
{
    return true;
}

bool ITypesUtil::Unmarshal(MessageParcel &data)
{
    return true;
}

bool ITypesUtil::Marshalling(uint32_t input, MessageParcel &data)
{
    return data.WriteUint32(input);
}

bool ITypesUtil::Unmarshalling(uint32_t &output, MessageParcel &data)
{
    return data.ReadUint32(output);
}

bool ITypesUtil::Marshalling(int32_t input, MessageParcel &data)
{
    return data.WriteInt32(input);
}

bool ITypesUtil::Unmarshalling(int32_t &output, MessageParcel &data)
{
    return data.ReadInt32(output);
}

bool ITypesUtil::Marshalling(uint64_t input, MessageParcel &data)
{
    return data.WriteUint64(input);
}

bool ITypesUtil::Unmarshalling(uint64_t &output, MessageParcel &data)
{
    return data.ReadUint64(output);
}

bool ITypesUtil::Marshalling(IRemoteObject* input, MessageParcel &data)
{
    return Marshalling(sptr<IRemoteObject>(input), data);
}

bool ITypesUtil::Marshalling(const std::monostate &input, MessageParcel &data)
{
    return true;
}

bool ITypesUtil::Unmarshalling(std::monostate &output, MessageParcel &data)
{
    return true;
}

bool ITypesUtil::Marshalling(const std::string &input, MessageParcel &data)
{
    return data.WriteString(input);
}

bool ITypesUtil::Unmarshalling(std::string &output, MessageParcel &data)
{
    return data.ReadString(output);
}

bool ITypesUtil::Marshalling(const std::vector<uint8_t> &input, MessageParcel &data)
{
    return data.WriteUInt8Vector(input);
}

bool ITypesUtil::Unmarshalling(std::vector<uint8_t> &output, MessageParcel &data)
{
    return data.ReadUInt8Vector(&output);
}

bool ITypesUtil::Marshalling(const Blob &blob, MessageParcel &data)
{
    return data.WriteUInt8Vector(blob.Data());
}

bool ITypesUtil::Unmarshalling(Blob &output, MessageParcel &data)
{
    std::vector<uint8_t> blob;
    bool result = data.ReadUInt8Vector(&blob);
    output = blob;
    return result;
}

bool ITypesUtil::Marshalling(const Entry &entry, MessageParcel &data)
{
    if (!Marshalling(entry.key, data)) {
        return false;
    }
    return Marshalling(entry.value, data);
}

bool ITypesUtil::Unmarshalling(Entry &output, MessageParcel &data)
{
    if (!Unmarshalling(output.key, data)) {
        return false;
    }
    return Unmarshalling(output.value, data);
}

bool ITypesUtil::Marshalling(const DeviceInfo &entry, MessageParcel &data)
{
    if (!data.WriteString(entry.deviceId)) {
        return false;
    }
    if (!data.WriteString(entry.deviceName)) {
        return false;
    }
    return data.WriteString(entry.deviceType);
}

bool ITypesUtil::Unmarshalling(DeviceInfo &output, MessageParcel &data)
{
    if (!data.ReadString(output.deviceId)) {
        return false;
    }
    if (!data.ReadString(output.deviceName)) {
        return false;
    }
    return data.ReadString(output.deviceType);
}

bool ITypesUtil::Marshalling(const ChangeNotification &notification, MessageParcel &parcel)
{
    if (!Marshalling(notification.GetInsertEntries(), parcel)) {
        return false;
    }

    if (!Marshalling(notification.GetUpdateEntries(), parcel)) {
        return false;
    }

    if (!Marshalling(notification.GetDeleteEntries(), parcel)) {
        return false;
    }
    if (!parcel.WriteString(notification.GetDeviceId())) {
        ZLOGE("WriteString deviceId_ failed.");
        return false;
    }

    return parcel.WriteBool(notification.IsClear());
}

bool ITypesUtil::Unmarshalling(ChangeNotification &output, MessageParcel &parcel)
{
    std::vector<Entry> insertEntries;
    if (!Unmarshalling(insertEntries, parcel)) {
        return false;
    }
    std::vector<Entry> updateEntries;
    if (!Unmarshalling(updateEntries, parcel)) {
        return false;
    }
    std::vector<Entry> deleteEntries;
    if (!Unmarshalling(deleteEntries, parcel)) {
        return false;
    }
    std::string deviceId;
    if (!parcel.ReadString(deviceId)) {
        ZLOGE("WriteString deviceId_ failed.");
        return false;
    }
    bool isClear = false;
    if (!parcel.ReadBool(isClear)) {
        ZLOGE("WriteString deviceId_ failed.");
        return false;
    }
    output = ChangeNotification(
        std::move(insertEntries), std::move(updateEntries), std::move(deleteEntries), deviceId, isClear);
    return true;
}

bool ITypesUtil::Marshalling(const DistributedRdb::RdbSyncerParam &param, MessageParcel &parcel)
{
    if (!parcel.WriteString(param.bundleName_)) {
        ZLOGE("RdbStoreParam write bundle name failed");
        return false;
    }
    if (!parcel.WriteString(param.hapName_)) {
        ZLOGE("RdbStoreParam write directory failed");
        return false;
    }
    if (!parcel.WriteString(param.storeName_)) {
        ZLOGE("RdbStoreParam write store name failed");
        return false;
    }
    if (!parcel.WriteInt32(param.area_)) {
        ZLOGE("RdbStoreParam write security level failed");
        return false;
    }
    if (!parcel.WriteInt32(param.level_)) {
        ZLOGE("RdbStoreParam write security level failed");
        return false;
    }
    if (!parcel.WriteInt32(param.type_)) {
        ZLOGE("RdbStoreParam write type failed");
        return false;
    }
    if (!parcel.WriteUInt8Vector(param.password_)) {
        ZLOGE("RdbStoreParam write password failed");
        return false;
    }
    if (!parcel.WriteBool(param.isAutoSync_)) {
        ZLOGE("RdbStoreParam write auto sync failed");
        return false;
    }
    if (!parcel.WriteBool(param.isEncrypt_)) {
        ZLOGE("RdbStoreParam write encrypt sync failed");
        return false;
    }
    return true;
}

bool ITypesUtil::Unmarshalling(DistributedRdb::RdbSyncerParam &param, MessageParcel &parcel)
{
    if (!parcel.ReadString(param.bundleName_)) {
        ZLOGE("RdbStoreParam read bundle name failed");
        return false;
    }
    if (!parcel.ReadString(param.hapName_)) {
        ZLOGE("RdbStoreParam read directory failed");
        return false;
    }
    if (!parcel.ReadString(param.storeName_)) {
        ZLOGE("RdbStoreParam read store name failed");
        return false;
    }
    if (!parcel.ReadInt32(param.area_)) {
        ZLOGE("RdbStoreParam read security level failed");
        return false;
    }
    if (!parcel.ReadInt32(param.level_)) {
        ZLOGE("RdbStoreParam read security level failed");
        return false;
    }
    if (!parcel.ReadInt32(param.type_)) {
        ZLOGE("RdbStoreParam read type failed");
        return false;
    }
    if (!parcel.ReadUInt8Vector(&(param.password_))) {
        ZLOGE("RdbStoreParam read password failed");
        return false;
    }
    if (!parcel.ReadBool(param.isAutoSync_)) {
        ZLOGE("RdbStoreParam read auto sync failed");
        return false;
    }
    if (!parcel.ReadBool(param.isEncrypt_)) {
        ZLOGE("RdbStoreParam read encrypt sync failed");
        return false;
    }
    return true;
}

bool ITypesUtil::Marshalling(const DistributedRdb::SyncOption &option, MessageParcel &parcel)
{
    if (!parcel.WriteInt32(option.mode)) {
        ZLOGE("SyncOption write mode failed");
        return false;
    }
    if (!parcel.WriteBool(option.isBlock)) {
        ZLOGE("SyncOption write isBlock failed");
        return false;
    }
    return true;
}

bool ITypesUtil::Unmarshalling(DistributedRdb::SyncOption &option, MessageParcel &parcel)
{
    int32_t mode;
    if (!parcel.ReadInt32(mode)) {
        ZLOGE("SyncOption read mode failed");
        return false;
    }
    option.mode = static_cast<DistributedRdb::SyncMode>(mode);
    if (!parcel.ReadBool(option.isBlock)) {
        ZLOGE("SyncOption read isBlock failed");
        return false;
    }
    return true;
}

bool ITypesUtil::Marshalling(const DistributedRdb::RdbPredicates &predicates, MessageParcel &parcel)
{
    if (!parcel.WriteString(predicates.table_)) {
        ZLOGE("predicate write table failed");
        return false;
    }
    if (!parcel.WriteStringVector(predicates.devices_)) {
        ZLOGE("predicate write devices failed");
        return false;
    }
    if (!parcel.WriteUint32(predicates.operations_.size())) {
        ZLOGE("predicate write operation size failed");
        return false;
    }
    for (const auto &operation : predicates.operations_) {
        if (!parcel.WriteInt32(operation.operator_)) {
            ZLOGE("predicate write operator failed");
            return false;
        }
        if (!parcel.WriteString(operation.field_)) {
            ZLOGE("predicate write field failed");
            return false;
        }
        if (!parcel.WriteStringVector(operation.values_)) {
            ZLOGE("predicate write values failed");
            return false;
        }
    }
    return true;
}

bool ITypesUtil::Unmarshalling(DistributedRdb::RdbPredicates &predicates, MessageParcel &parcel)
{
    if (!parcel.ReadString(predicates.table_)) {
        ZLOGE("predicate read table failed");
        return false;
    }
    if (!parcel.ReadStringVector(&predicates.devices_)) {
        ZLOGE("predicate read devices failed");
        return false;
    }
    uint32_t size = 0;
    if (!parcel.ReadUint32(size)) {
        ZLOGE("predicate read operation size failed");
        return false;
    }
    for (uint32_t i = 0; i < size; i++) {
        int32_t op;
        if (!parcel.ReadInt32(op)) {
            ZLOGE("predicate read operator failed");
            return false;
        }
        DistributedRdb::RdbPredicateOperation operation;
        operation.operator_ = static_cast<DistributedRdb::RdbPredicateOperator>(op);
        if (!parcel.ReadString(operation.field_)) {
            ZLOGE("predicate read field failed");
            return false;
        }
        if (!parcel.ReadStringVector(&operation.values_)) {
            ZLOGE("predicate read values failed");
            return false;
        }
        predicates.operations_.push_back(std::move(operation));
    }
    return true;
}

bool ITypesUtil::Marshalling(const Options &input, MessageParcel &data)
{
    if (!data.WriteString(input.schema)) {
        ZLOGE("schema is failed");
        return false;
    }

    if (!data.WriteString(input.hapName)) {
        ZLOGE("hapName is failed");
        return false;
    }

    if (!Marshalling(input.policies, data)) {
        ZLOGE("write policies failed");
        return false;
    }

    std::unique_ptr<uint8_t[]> buffer = std::make_unique<uint8_t[]>(sizeof(input));
    Options *target = reinterpret_cast<Options *>(buffer.get());
    target->createIfMissing = input.createIfMissing;
    target->encrypt = input.encrypt;
    target->persistent = input.persistent;
    target->backup = input.backup;
    target->autoSync = input.autoSync;
    target->syncable = input.syncable;
    target->securityLevel = input.securityLevel;
    target->area = input.area;
    target->kvStoreType = input.kvStoreType;
    return data.WriteRawData(buffer.get(), sizeof(input));
}

bool ITypesUtil::Unmarshalling(Options &output, MessageParcel &data)
{
    if (!data.ReadString(output.schema)) {
        ZLOGE("read schema failed");
        return false;
    }

    if (!data.ReadString(output.hapName)) {
        ZLOGE("read hapName failed");
        return false;
    }

    if (!Unmarshalling(output.policies, data)) {
        ZLOGE("read policies failed");
        return false;
    }

    const Options *source = reinterpret_cast<const Options *>(data.ReadRawData(sizeof(output)));
    if (source == nullptr) {
        return false;
    }
    output.createIfMissing = source->createIfMissing;
    output.encrypt = source->encrypt;
    output.persistent = source->persistent;
    output.backup = source->backup;
    output.autoSync = source->autoSync;
    output.securityLevel = source->securityLevel;
    output.area = source->area;
    output.kvStoreType = source->kvStoreType;
    output.syncable = source->syncable;
    return true;
}

bool ITypesUtil::Marshalling(const SyncPolicy &input, MessageParcel &data)
{
    if (!data.WriteUint32(input.type)) {
        ZLOGE("write policy type failed");
        return false;
    }
    if (!Marshalling(input.value, data)) {
        ZLOGE("write policy value failed");
        return false;
    }
    return true;
}

bool ITypesUtil::Unmarshalling(SyncPolicy &output, MessageParcel &data)
{
    if (!data.ReadUint32(output.type)) {
        ZLOGE("read policy type failed");
        return false;
    }
    if (!Unmarshalling(output.value, data)) {
        ZLOGE("read policy value failed");
        return false;
    }
    return true;
}

bool ITypesUtil::Marshalling(const KVDBService::DevBrief &input, MessageParcel &data)
{
    return ITypesUtil::Marshal(data, input.uuid, input.networkId);
}

bool ITypesUtil::Unmarshalling(KVDBService::DevBrief &output, MessageParcel &data)
{
    return ITypesUtil::Unmarshal(data, output.uuid, output.networkId);
}

bool ITypesUtil::Marshalling(const sptr<IRemoteObject> &input, MessageParcel &data)
{
    return data.WriteRemoteObject(input);
}

bool ITypesUtil::Unmarshalling(sptr<IRemoteObject> &output, MessageParcel &data)
{
    output = data.ReadRemoteObject();
    return true;
}

bool ITypesUtil::Unmarshalling(DataShare::DataSharePredicates &predicates, MessageParcel &parcel)
{
    ZLOGD("Unmarshalling DataSharePredicates Start");
    std::list<DataShare::OperationItem> operations{};
    std::string whereClause = "";
    std::vector<std::string> whereArgs;
    std::string order = "";
    int64_t mode = DataShare::INVALID_MODE;
    size_t size = static_cast<size_t>(parcel.ReadInt32());
    if (static_cast<int32_t>(size) < 0) {
        ZLOGE("predicate read listSize failed");
        return false;
    }
    if ((size > parcel.GetReadableBytes()) || (operations.max_size() < size)) {
        ZLOGE("Read operations failed, size : %{public}zu", size);
        return false;
    }
    operations.clear();
    for (size_t i = 0; i < size; i++) {
        DataShare::OperationItem listitem{};
        if (!Unmarshalling(listitem, parcel)) {
            ZLOGE("operations read OperationItem failed");
            return false;
        }
        operations.push_back(listitem);
    }
    if (!parcel.ReadString(whereClause)) {
        ZLOGE("predicate read whereClause failed");
        return false;
    }
    if (!parcel.ReadStringVector(&whereArgs)) {
        ZLOGE("predicate read whereArgs failed");
        return false;
    }
    if (!parcel.ReadString(order)) {
        ZLOGE("predicate read order failed");
        return false;
    }
    if (!parcel.ReadInt64(mode)) {
        ZLOGE("predicate read mode failed");
        return false;
    }
    DataShare::DataSharePredicates tmpPredicates(operations);
    tmpPredicates.SetWhereClause(whereClause);
    tmpPredicates.SetWhereArgs(whereArgs);
    tmpPredicates.SetOrder(order);
    tmpPredicates.SetSettingMode(static_cast<DataShare::SettingMode>(mode));
    predicates = tmpPredicates;
    return true;
}

bool ITypesUtil::Unmarshalling(DataShare::DataShareValuesBucket &valuesBucket, MessageParcel &parcel)
{
    int len = parcel.ReadInt32();
    if (len < 0) {
        ZLOGE("valuesBucket read mapSize failed");
        return false;
    }
    size_t size = static_cast<size_t>(len);
    if ((size > parcel.GetReadableBytes()) || (valuesBucket.valuesMap.max_size() < size)) {
        ZLOGE("Read valuesMap failed, size : %{public}zu", size);
        return false;
    }
    valuesBucket.valuesMap.clear();
    for (size_t i = 0; i < size; i++) {
        std::string key = parcel.ReadString();
        DataShare::DataShareValueObject value{};
        if (!Unmarshalling(value, parcel)) {
            ZLOGE("valuesBucket read value failed");
            return false;
        }
        valuesBucket.valuesMap.insert(std::make_pair(key, value));
    }
    return true;
}

bool ITypesUtil::Unmarshalling(DataShare::OperationItem &operationItem, MessageParcel &parcel)
{
    operationItem.operation = static_cast<DataShare::OperationType>(parcel.ReadInt64());
    if (operationItem.operation <DataShare::OperationType::INVALID_OPERATION) {
        ZLOGE("operationItem read operation failed");
        return false;
    }
    if (!Unmarshalling(operationItem.singleParams, parcel)) {
        ZLOGE("Unmarshalling singleParams failed");
        return false;
    }
    if (!Unmarshalling(operationItem.multiParams, parcel)) {
        ZLOGE("Unmarshalling multiParams failed");
        return false;
    }
    return true;
}

bool ITypesUtil::Unmarshalling(DataShare::DataSharePredicatesObject &predicatesObject, MessageParcel &parcel)
{
    int16_t type = parcel.ReadInt16();
    if (type < (int16_t)DataShare::DataSharePredicatesObjectType::TYPE_NULL) {
        ZLOGE("predicatesObject read type failed");
        return false;
    }
    predicatesObject.type = static_cast<DataShare::DataSharePredicatesObjectType>(type);
    switch (predicatesObject.type) {
        case DataShare::DataSharePredicatesObjectType::TYPE_INT: {
            predicatesObject.value = parcel.ReadInt32();
            break;
        }
        case DataShare::DataSharePredicatesObjectType::TYPE_LONG: {
            predicatesObject.value = parcel.ReadInt64();
            break;
        }
        case DataShare::DataSharePredicatesObjectType::TYPE_DOUBLE: {
            predicatesObject.value = parcel.ReadDouble();
            break;
        }
        case DataShare::DataSharePredicatesObjectType::TYPE_STRING: {
            predicatesObject.value = parcel.ReadString();
            break;
        }
        case DataShare::DataSharePredicatesObjectType::TYPE_BOOL: {
            predicatesObject.value = parcel.ReadBool();
            break;
        }
        default:
            break;
    }
    return true;
}

bool ITypesUtil::Unmarshalling(DataShare::DataSharePredicatesObjects &predicatesObject, MessageParcel &parcel)
{
    int16_t type = parcel.ReadInt16();
    if (type < (int16_t)DataShare::DataSharePredicatesObjectsType::TYPE_NULL) {
        ZLOGE("predicatesObject read type failed");
        return false;
    }
    predicatesObject.type = static_cast<DataShare::DataSharePredicatesObjectsType>(type);
    switch (predicatesObject.type) {
        case DataShare::DataSharePredicatesObjectsType::TYPE_INT_VECTOR: {
            std::vector<int> intval{};
            if (!parcel.ReadInt32Vector(&intval)) {
                ZLOGE("predicatesObject ReadInt32Vector value failed");
                return false;
            }
            predicatesObject.value = intval;
            break;
        }
        case DataShare::DataSharePredicatesObjectsType::TYPE_LONG_VECTOR: {
            std::vector<int64_t> int64val{};
            if (!parcel.ReadInt64Vector(&int64val)) {
                ZLOGE("predicatesObject ReadInt64Vector value failed");
                return false;
            }
            predicatesObject.value = int64val;
            break;
        }
        case DataShare::DataSharePredicatesObjectsType::TYPE_DOUBLE_VECTOR: {
            std::vector<double> doubleval{};
            if (!parcel.ReadDoubleVector(&doubleval)) {
                ZLOGE("predicatesObject ReadDoubleVector value failed");
                return false;
            }
            predicatesObject.value = doubleval;
            break;
        }
        case DataShare::DataSharePredicatesObjectsType::TYPE_STRING_VECTOR: {
            std::vector<std::string> stringval{};
            if (!parcel.ReadStringVector(&stringval)) {
                ZLOGE("predicatesObject ReadDoubReadStringVectorleVector value failed");
                return false;
            }
            predicatesObject.value = stringval;
            break;
        }
        default:
            break;
    }
    return true;
}

bool ITypesUtil::Unmarshalling(DataShare::DataShareValueObject &valueObject, MessageParcel &parcel)
{
    int16_t type = parcel.ReadInt16();
    if (type < (int16_t)DataShare::DataShareValueObjectType::TYPE_NULL) {
        ZLOGE("valueObject read type failed");
        return false;
    }
    valueObject.type = static_cast<DataShare::DataShareValueObjectType>(type);
    switch (valueObject.type) {
        case DataShare::DataShareValueObjectType::TYPE_INT: {
            valueObject.value = parcel.ReadInt64();
            break;
        }
        case DataShare::DataShareValueObjectType::TYPE_DOUBLE: {
            valueObject.value = parcel.ReadDouble();
            break;
        }
        case DataShare::DataShareValueObjectType::TYPE_STRING: {
            valueObject.value = parcel.ReadString();
            break;
        }
        case DataShare::DataShareValueObjectType::TYPE_BLOB: {
            std::vector<uint8_t> val;
            if (!parcel.ReadUInt8Vector(&val)) {
                ZLOGE("valueObject ReadUInt8Vector value failed");
                return false;
            }
            valueObject.value = val;
            break;
        }
        case DataShare::DataShareValueObjectType::TYPE_BOOL: {
            valueObject.value = parcel.ReadBool();
            break;
        }
        default:
            break;
    }
    return true;
}

int64_t ITypesUtil::GetTotalSize(const std::vector<Entry> &entries)
{
    int64_t bufferSize = 1;
    for (const auto &item : entries) {
        if (item.key.Size() > Entry::MAX_KEY_LENGTH || item.value.Size() > Entry::MAX_VALUE_LENGTH) {
            return -bufferSize;
        }
        bufferSize += item.key.RawSize() + item.value.RawSize();
    }
    return bufferSize - 1;
}

int64_t ITypesUtil::GetTotalSize(const std::vector<Key> &entries)
{
    int64_t bufferSize = 1;
    for (const auto &item : entries) {
        if (item.Size() > Entry::MAX_KEY_LENGTH) {
            return -bufferSize;
        }
        bufferSize += item.RawSize();
    }
    return bufferSize - 1;
}
} // namespace DistributedKv
} // namespace OHOS