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

#include "db_common.h"

#include <climits>
#include <cstdio>
#include <queue>

#include "cloud/cloud_db_constant.h"
#include "cloud/cloud_db_types.h"
#include "db_errno.h"
#include "platform_specific.h"
#include "query_sync_object.h"
#include "hash.h"
#include "runtime_context.h"
#include "value_hash_calc.h"

namespace DistributedDB {
namespace {
    void RemoveFiles(const std::list<OS::FileAttr> &fileList, OS::FileType type)
    {
        for (const auto &item : fileList) {
            if (item.fileType != type) {
                continue;
            }
            int errCode = OS::RemoveFile(item.fileName);
            if (errCode != E_OK) {
                LOGE("Remove file failed:%d", errno);
            }
        }
    }

    void RemoveDirectories(const std::list<OS::FileAttr> &fileList, OS::FileType type)
    {
        for (auto item = fileList.rbegin(); item != fileList.rend(); ++item) {
            if (item->fileType != type) {
                continue;
            }
            int errCode = OS::RemoveDBDirectory(item->fileName);
            if (errCode != 0) {
                LOGE("Remove directory failed:%d", errno);
            }
        }
    }
    const std::string HEX_CHAR_MAP = "0123456789abcdef";
    const std::string CAP_HEX_CHAR_MAP = "0123456789ABCDEF";
}

int DBCommon::CreateDirectory(const std::string &directory)
{
    bool isExisted = OS::CheckPathExistence(directory);
    if (!isExisted) {
        int errCode = OS::MakeDBDirectory(directory);
        if (errCode != E_OK) {
            return errCode;
        }
    }
    return E_OK;
}

void DBCommon::StringToVector(const std::string &src, std::vector<uint8_t> &dst)
{
    dst.resize(src.size());
    dst.assign(src.begin(), src.end());
}

void DBCommon::VectorToString(const std::vector<uint8_t> &src, std::string &dst)
{
    dst.clear();
    dst.assign(src.begin(), src.end());
}

std::string DBCommon::VectorToHexString(const std::vector<uint8_t> &inVec, const std::string &separator)
{
    std::string outString;
    for (auto &entry : inVec) {
        outString.push_back(CAP_HEX_CHAR_MAP[entry >> 4]); // high 4 bits to one hex.
        outString.push_back(CAP_HEX_CHAR_MAP[entry & 0x0F]); // low 4 bits to one hex.
        outString += separator;
    }
    outString.erase(outString.size() - separator.size(), separator.size()); // remove needless separator at last
    return outString;
}

void DBCommon::PrintHexVector(const std::vector<uint8_t> &data, int line, const std::string &tag)
{
    const size_t maxDataLength = 1024;
    const int byteHexNum = 2;
    size_t dataLength = data.size();

    if (data.size() > maxDataLength) {
        dataLength = maxDataLength;
    }

    char *buff = new (std::nothrow) char[dataLength * byteHexNum + 1]; // dual and add one for the end;
    if (buff == nullptr) {
        return;
    }

    for (std::vector<uint8_t>::size_type i = 0; i < dataLength; ++i) {
        buff[byteHexNum * i] = CAP_HEX_CHAR_MAP[data[i] >> 4]; // high 4 bits to one hex.
        buff[byteHexNum * i + 1] = CAP_HEX_CHAR_MAP[data[i] & 0x0F]; // low 4 bits to one hex.
    }
    buff[dataLength * byteHexNum] = '\0';

    if (line == 0) {
        LOGD("[%s] size:%zu -- %s", tag.c_str(), data.size(), buff);
    } else {
        LOGD("[%s][%d] size:%zu -- %s", tag.c_str(), line, data.size(), buff);
    }

    delete []buff;
    return;
}

std::string DBCommon::TransferHashString(const std::string &devName)
{
    if (devName.empty()) {
        return "";
    }
    std::vector<uint8_t> devVect(devName.begin(), devName.end());
    std::vector<uint8_t> hashVect;
    int errCode = CalcValueHash(devVect, hashVect);
    if (errCode != E_OK) {
        return "";
    }

    return std::string(hashVect.begin(), hashVect.end());
}

std::string DBCommon::TransferStringToHex(const std::string &origStr)
{
    if (origStr.empty()) {
        return "";
    }

    std::string tmp;
    for (auto item : origStr) {
        unsigned char currentByte = static_cast<unsigned char>(item);
        tmp.push_back(HEX_CHAR_MAP[currentByte >> 4]); // high 4 bits to one hex.
        tmp.push_back(HEX_CHAR_MAP[currentByte & 0x0F]); // low 4 bits to one hex.
    }
    return tmp;
}

int DBCommon::CalcValueHash(const std::vector<uint8_t> &value, std::vector<uint8_t> &hashValue)
{
    ValueHashCalc hashCalc;
    int errCode = hashCalc.Initialize();
    if (errCode != E_OK) {
        return -E_INTERNAL_ERROR;
    }

    errCode = hashCalc.Update(value);
    if (errCode != E_OK) {
        return -E_INTERNAL_ERROR;
    }

    errCode = hashCalc.GetResult(hashValue);
    if (errCode != E_OK) {
        return -E_INTERNAL_ERROR;
    }

    return E_OK;
}

int DBCommon::CreateStoreDirectory(const std::string &directory, const std::string &identifierName,
    const std::string &subDir, bool isCreate)
{
    std::string newDir = directory;
    if (newDir.back() != '/') {
        newDir += "/";
    }

    newDir += identifierName;
    if (!isCreate) {
        if (!OS::CheckPathExistence(newDir)) {
            LOGE("Required path does not exist and won't create.");
            return -E_INVALID_ARGS;
        }
        return E_OK;
    }

    if (directory.empty()) {
        return -E_INVALID_ARGS;
    }

    int errCode = DBCommon::CreateDirectory(newDir);
    if (errCode != E_OK) {
        return errCode;
    }

    newDir += ("/" + subDir);
    return DBCommon::CreateDirectory(newDir);
}

int DBCommon::CopyFile(const std::string &srcFile, const std::string &dstFile)
{
    const int copyBlockSize = 4096;
    std::vector<uint8_t> tmpBlock(copyBlockSize, 0);
    int errCode;
    FILE *fileIn = fopen(srcFile.c_str(), "rb");
    if (fileIn == nullptr) {
        LOGE("[Common:CpFile] open the source file error:%d", errno);
        return -E_INVALID_FILE;
    }
    FILE *fileOut = fopen(dstFile.c_str(), "wb");
    if (fileOut == nullptr) {
        LOGE("[Common:CpFile] open the target file error:%d", errno);
        errCode = -E_INVALID_FILE;
        goto END;
    }
    for (;;) {
        size_t readSize = fread(static_cast<void *>(tmpBlock.data()), 1, copyBlockSize, fileIn);
        if (readSize < copyBlockSize) {
            // not end and have error.
            if (feof(fileIn) != 0 && ferror(fileIn) != 0) {
                LOGE("Copy the file error:%d", errno);
                errCode = -E_SYSTEM_API_FAIL;
                break;
            }
        }

        if (readSize != 0) {
            size_t writeSize = fwrite(static_cast<void *>(tmpBlock.data()), 1, readSize, fileOut);
            if (ferror(fileOut) != 0 || writeSize != readSize) {
                LOGE("Write the data while copy:%d", errno);
                errCode = -E_SYSTEM_API_FAIL;
                break;
            }
        }

        if (feof(fileIn) != 0) {
            errCode = E_OK;
            break;
        }
    }

END:
    if (fileIn != nullptr) {
        (void)fclose(fileIn);
    }
    if (fileOut != nullptr) {
        (void)fclose(fileOut);
    }
    return errCode;
}

int DBCommon::RemoveAllFilesOfDirectory(const std::string &dir, bool isNeedRemoveDir)
{
    std::list<OS::FileAttr> fileList;
    bool isExisted = OS::CheckPathExistence(dir);
    if (!isExisted) {
        return E_OK;
    }
    int errCode = OS::GetFileAttrFromPath(dir, fileList, true);
    if (errCode != E_OK) {
        return errCode;
    }

    RemoveFiles(fileList, OS::FileType::FILE);
    RemoveDirectories(fileList, OS::FileType::PATH);
    if (isNeedRemoveDir) {
        // Pay attention to the order of deleting the directory
        if (OS::CheckPathExistence(dir) && OS::RemoveDBDirectory(dir) != 0) {
            LOGI("Remove the directory error:%d", errno);
            errCode = -E_SYSTEM_API_FAIL;
        }
    }

    return errCode;
}

std::string DBCommon::GenerateIdentifierId(const std::string &storeId,
    const std::string &appId, const std::string &userId, int32_t instanceId)
{
    std::string id = userId + "-" + appId + "-" + storeId;
    if (instanceId != 0) {
        id += "-" + std::to_string(instanceId);
    }
    return id;
}

std::string DBCommon::GenerateDualTupleIdentifierId(const std::string &storeId, const std::string &appId)
{
    return appId + "-" + storeId;
}

void DBCommon::SetDatabaseIds(KvDBProperties &properties, const std::string &appId, const std::string &userId,
    const std::string &storeId, int32_t instanceId)
{
    properties.SetIdentifier(userId, appId, storeId, instanceId);
    std::string oriStoreDir;
    // IDENTIFIER_DIR no need cal with instanceId
    std::string identifier = GenerateIdentifierId(storeId, appId, userId);
    if (properties.GetBoolProp(KvDBProperties::CREATE_DIR_BY_STORE_ID_ONLY, false)) {
        oriStoreDir = storeId;
    } else {
        oriStoreDir = identifier;
    }
    std::string hashIdentifier = TransferHashString(identifier);
    std::string hashDir = TransferHashString(oriStoreDir);
    std::string hexHashDir = TransferStringToHex(hashDir);
    properties.SetStringProp(KvDBProperties::IDENTIFIER_DIR, hexHashDir);
}

std::string DBCommon::StringMasking(const std::string &oriStr, size_t remain)
{
#ifndef DB_DEBUG_ENV
    if (oriStr.size() > remain) {
        return oriStr.substr(0, remain);
    }
#endif
    return oriStr;
}

std::string DBCommon::GetDistributedTableName(const std::string &device, const std::string &tableName)
{
    if (!RuntimeContext::GetInstance()->ExistTranslateDevIdCallback()) {
        return GetDistributedTableNameWithHash(device, tableName);
    }
    return CalDistributedTableName(device, tableName);
}

std::string DBCommon::GetDistributedTableName(const std::string &device, const std::string &tableName,
    const StoreInfo &info)
{
    std::string newDeviceId;
    if (RuntimeContext::GetInstance()->TranslateDeviceId(device, info, newDeviceId) != E_OK) {
        return GetDistributedTableNameWithHash(device, tableName);
    }
    return CalDistributedTableName(newDeviceId, tableName);
}

std::string DBCommon::GetDistributedTableNameWithHash(const std::string &device, const std::string &tableName)
{
    std::string deviceHashHex = DBCommon::TransferStringToHex(DBCommon::TransferHashString(device));
    return CalDistributedTableName(deviceHashHex, tableName);
}

std::string DBCommon::CalDistributedTableName(const std::string &device, const std::string &tableName)
{
    return DBConstant::RELATIONAL_PREFIX + tableName + "_" + device;
}

void DBCommon::GetDeviceFromName(const std::string &deviceTableName, std::string &deviceHash, std::string &tableName)
{
    std::size_t found = deviceTableName.rfind('_');
    if (found != std::string::npos && found + 1 < deviceTableName.length() &&
        found > DBConstant::RELATIONAL_PREFIX.length()) {
        deviceHash = deviceTableName.substr(found + 1);
        tableName = deviceTableName.substr(DBConstant::RELATIONAL_PREFIX.length(),
            found - DBConstant::RELATIONAL_PREFIX.length());
    }
}

std::string DBCommon::TrimSpace(const std::string &input)
{
    std::string res;
    res.reserve(input.length());
    bool isPreSpace = true;
    for (char c : input) {
        if (std::isspace(c)) {
            isPreSpace = true;
        } else {
            if (!res.empty() && isPreSpace) {
                res += ' ';
            }
            res += c;
            isPreSpace = false;
        }
    }
    res.shrink_to_fit();
    return res;
}

void DBCommon::RTrim(std::string &oriString)
{
    if (oriString.empty()) {
        return;
    }
    oriString.erase(oriString.find_last_not_of(" ") + 1);
}

namespace {
bool CharIn(char c, const std::string &pattern)
{
    return std::any_of(pattern.begin(), pattern.end(), [c] (char p) {
        return c == p;
    });
}
}

bool DBCommon::HasConstraint(const std::string &sql, const std::string &keyWord, const std::string &prePattern,
    const std::string &nextPattern)
{
    size_t pos = 0;
    while ((pos = sql.find(keyWord, pos)) != std::string::npos) {
        if (pos >= 1 && CharIn(sql[pos - 1], prePattern) && ((pos + keyWord.length() == sql.length()) ||
            ((pos + keyWord.length() < sql.length()) && CharIn(sql[pos + keyWord.length()], nextPattern)))) {
            return true;
        }
        pos++;
    }
    return false;
}

bool DBCommon::IsSameCipher(CipherType srcType, CipherType inputType)
{
    // At present, the default type is AES-256-GCM.
    // So when src is default and input is AES-256-GCM,
    // or when src is AES-256-GCM and input is default,
    // we think they are the same type.
    if (srcType == inputType ||
        ((srcType == CipherType::DEFAULT || srcType == CipherType::AES_256_GCM) &&
        (inputType == CipherType::DEFAULT || inputType == CipherType::AES_256_GCM))) {
        return true;
    }
    return false;
}

std::string DBCommon::ToLowerCase(const std::string &str)
{
    std::string res(str.length(), ' ');
    std::transform(str.begin(), str.end(), res.begin(), ::tolower);
    return res;
}

std::string DBCommon::ToUpperCase(const std::string &str)
{
    std::string res(str.length(), ' ');
    std::transform(str.begin(), str.end(), res.begin(), ::toupper);
    return res;
}

bool DBCommon::CaseInsensitiveCompare(const std::string &first, const std::string &second)
{
    return (strcasecmp(first.c_str(), second.c_str()) == 0);
}

bool DBCommon::CheckIsAlnumOrUnderscore(const std::string &text)
{
    auto iter = std::find_if_not(text.begin(), text.end(), [](char c) {
        return (std::isalnum(c) || c == '_');
    });
    return iter == text.end();
}

bool DBCommon::CheckQueryWithoutMultiTable(const Query &query)
{
    QuerySyncObject syncObject(query);
    if (!syncObject.GetRelationTableNames().empty()) {
        LOGE("check query table names from tables failed!");
        return false;
    }
    if (!QuerySyncObject::GetQuerySyncObject(query).empty()) {
        LOGE("check query object from table failed!");
        return false;
    }
    return true;
}

/* this function us topology sorting algorithm to detect whether a ring exists in the dependency
 * the algorithm main procedure as below:
 * 1. select a point which in-degree is 0 in the graph and record it;
 * 2. delete the point and all edges starting from this point;
 * 3. repeat step 1 and 2, until the graph is empty or there is no point with a zero degree
 * */
bool DBCommon::IsCircularDependency(int size, const std::vector<std::vector<int>> &dependency)
{
    std::vector<int> inDegree(size, 0); // save in-degree of every point
    std::vector<std::vector<int>> adjacencyList(size);
    for (size_t i = 0; i < dependency.size(); i++) {
        adjacencyList[dependency[i][0]].push_back(dependency[i][1]); // update adjacencyList
        inDegree[dependency[i][1]]++;
    }
    std::queue<int> que;
    for (size_t i = 0; i < inDegree.size(); i++) {
        if (inDegree[i] == 0) {
            que.push(i); // push all point which in-degree = 0
        }
    }

    int zeroDegreeCnt = static_cast<int>(que.size());
    while (!que.empty()) {
        int index = que.front();
        que.pop();
        for (size_t i = 0; i < adjacencyList[index].size(); ++i) {
            int j = adjacencyList[index][i]; // adjacencyList[index] save the point which is connected to index
            inDegree[j]--;
            if (inDegree[j] == 0) {
                zeroDegreeCnt++;
                que.push(j);
            }
        }
    }
    return zeroDegreeCnt != size;
}

int DBCommon::SerializeWaterMark(Timestamp localMark, const std::string &cloudMark, Value &blobMeta)
{
    uint64_t length = Parcel::GetUInt64Len() + Parcel::GetStringLen(cloudMark);
    blobMeta.resize(length);
    Parcel parcel(blobMeta.data(), blobMeta.size());
    parcel.WriteUInt64(localMark);
    parcel.WriteString(cloudMark);
    if (parcel.IsError()) {
        LOGE("[DBCommon] Parcel error while serializing cloud meta data.");
        return -E_PARSE_FAIL;
    }
    return E_OK;
}

Key DBCommon::GetPrefixTableName(const TableName &tableName)
{
    TableName newName = CloudDbConstant::CLOUD_META_TABLE_PREFIX + tableName;
    Key prefixedTableName(newName.begin(), newName.end());
    return prefixedTableName;
}

void DBCommon::InsertNodesByScore(const std::map<std::string, std::map<std::string, bool>> &graph,
    const std::vector<std::string> &generateNodes, const std::map<std::string, int> &scoreGraph,
    std::list<std::string> &insertTarget)
{
    auto copyGraph = graph;
    // insert all nodes into res
    for (const auto &generateNode : generateNodes) {
        auto iterator = insertTarget.begin();
        for (; iterator != insertTarget.end(); iterator++) {
            // don't compare two no reachable node
            if (!copyGraph[*iterator][generateNode] && !copyGraph[generateNode][*iterator]) {
                continue;
            }
            if (scoreGraph.find(*iterator) == scoreGraph.end() || scoreGraph.find(generateNode) == scoreGraph.end()) {
                // should not happen
                LOGW("[DBCommon] not find score in graph");
                continue;
            }
            if (scoreGraph.at(*iterator) <= scoreGraph.at(generateNode)) {
                break;
            }
        }
        insertTarget.insert(iterator, generateNode);
    }
}

std::list<std::string> DBCommon::GenerateNodesByNodeWeight(const std::vector<std::string> &nodes,
    const std::map<std::string, std::map<std::string, bool>> &graph,
    const std::map<std::string, int> &nodeWeight)
{
    std::list<std::string> res;
    std::set<std::string> paramNodes;
    std::set<std::string> visitNodes;
    for (const auto &node : nodes) {
        res.push_back(node);
        paramNodes.insert(node);
        visitNodes.insert(node);
    }
    // find all node which can be reached by param nodes
    for (const auto &source : paramNodes) {
        if (graph.find(source) == graph.end()) {
            continue;
        }
        for (const auto &[target, reach] : graph.at(source)) {
            if (reach) {
                visitNodes.insert(target);
            }
        }
    }
    std::vector<std::string> generateNodes;
    for (const auto &node : visitNodes) {
        // ignore the node which is param
        if (paramNodes.find(node) == paramNodes.end()) {
            generateNodes.push_back(node);
        }
    }
    InsertNodesByScore(graph, generateNodes, nodeWeight, res);
    return res;
}

bool DBCommon::HasPrimaryKey(const std::vector<Field> &fields)
{
    for (const auto &field : fields) {
        if (field.primary) {
            return true;
        }
    }
    return false;
}

bool DBCommon::IsRecordError(const VBucket &record)
{
    if (record.find(CloudDbConstant::ERROR_FIELD) == record.end()) {
        return false;
    }
    return record.at(CloudDbConstant::ERROR_FIELD).index() == TYPE_INDEX<std::string>;
}

bool DBCommon::IsRecordIgnored(const VBucket &record)
{
    if (record.find(CloudDbConstant::ERROR_FIELD) == record.end()) {
        return false;
    }
    if (record.at(CloudDbConstant::ERROR_FIELD).index() != TYPE_INDEX<int64_t>) {
        return false;
    }
    auto status = std::get<int64_t>(record.at(CloudDbConstant::ERROR_FIELD));
    return status == static_cast<int64_t>(DBStatus::CLOUD_RECORD_EXIST_CONFLICT);
}

bool DBCommon::IsRecordVersionConflict(const VBucket &record)
{
    if (record.find(CloudDbConstant::ERROR_FIELD) == record.end()) {
        return false;
    }
    if (record.at(CloudDbConstant::ERROR_FIELD).index() != TYPE_INDEX<int64_t>) {
        return false;
    }
    auto status = std::get<int64_t>(record.at(CloudDbConstant::ERROR_FIELD));
    return status == static_cast<int64_t>(DBStatus::CLOUD_VERSION_CONFLICT);
}

std::string DBCommon::GenerateHashLabel(const DBInfo &dbInfo)
{
    if (dbInfo.syncDualTupleMode) {
        return DBCommon::TransferHashString(dbInfo.appId + "-" + dbInfo.storeId);
    }
    return DBCommon::TransferHashString(dbInfo.userId + "-" + dbInfo.appId + "-" + dbInfo.storeId);
}

uint64_t DBCommon::EraseBit(uint64_t origin, uint64_t eraseBit)
{
    return origin & (~eraseBit);
}
} // namespace DistributedDB
