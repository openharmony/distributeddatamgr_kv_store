/*
 * Copyright (c) 2021-2025 Huawei Device Co., Ltd.
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
#ifndef OHOS_ABILITY_BASE_WANT_PARAMS_H
#define OHOS_ABILITY_BASE_WANT_PARAMS_H

#include <iostream>
#include <map>
#include <mutex>
#include <set>
#include <unistd.h>
#include <vector>

#include "base_interfaces.h"
#include "message_parcel.h"
#include "parcel.h"
#include "refbase.h"

namespace OHOS {
namespace AAFwk {
extern const char* FD;
extern const char* REMOTE_OBJECT;
extern const char* TYPE_PROPERTY;
extern const char* VALUE_PROPERTY;

enum ScreenMode : int8_t {
    IDLE_SCREEN_MODE = -1,
    JUMP_SCREEN_MODE = 0,
    EMBEDDED_FULL_SCREEN_MODE = 1,
    EMBEDDED_HALF_SCREEN_MODE = 2
};
constexpr const char* SCREEN_MODE_KEY = "ohos.extra.param.key.showMode";

class UnsupportedData {
public:
    std::u16string key;
    int type = 0;
    int size = 0;
    uint8_t *buffer = nullptr;

    ~UnsupportedData();

    UnsupportedData();
    UnsupportedData(const UnsupportedData &other);
    UnsupportedData(UnsupportedData &&other);

    UnsupportedData &operator=(const UnsupportedData &other);
    UnsupportedData &operator=(UnsupportedData &&other);
};

class WantParams final : public Parcelable {
public:
    WantParams() = default;
    WantParams(const WantParams &wantParams);
    WantParams(WantParams &&other) noexcept;
    ~WantParams()
    {}
    WantParams &operator=(const WantParams &other);
    WantParams &operator=(WantParams &&other) noexcept;
    bool operator==(const WantParams &other);

    static sptr<IInterface> GetInterfaceByType(int typeId, const std::string &value);

    static bool CompareInterface(const sptr<IInterface> iIt1, const sptr<IInterface> iIt2, int typeId);

    static int GetDataType(const sptr<IInterface> iIt);

    static std::string GetStringByType(const sptr<IInterface> iIt, int typeId);

    void SetParam(const std::string &key, IInterface *value);

    sptr<IInterface> GetParam(const std::string &key) const;

    WantParams GetWantParams(const std::string& key) const;

    std::string GetStringParam(const std::string& key) const;

    int GetIntParam(const std::string& key, const int defaultValue) const;

    const std::map<std::string, sptr<IInterface>> &GetParams() const;

    const std::set<std::string> KeySet() const;

    void Remove(const std::string &key);

    bool HasParam(const std::string &key) const;

    int Size() const;

    bool IsEmpty() const;

    virtual bool Marshalling(Parcel &parcel) const;

    static WantParams *Unmarshalling(Parcel &parcel, int depth = 1);

    void DumpInfo(int level) const;

    void CloseAllFd();

    void RemoveAllFd();

    void DupAllFd();

    void GetCachedUnsupportedData(std::vector<UnsupportedData> &cachedUnsuppertedData) const;

    void SetCachedUnsupportedData(const std::vector<UnsupportedData> &cachedUnsuppertedData);

    std::string ToString() const;
    void SetNeedExpansion(bool flag) const;
    bool CheckNeedExpansion() const;
    bool PublicReadFromParcel(Parcel &parcel, int depth = 1);
private:
    enum {
        VALUE_TYPE_NULL = -1,
        VALUE_TYPE_BOOLEAN = 1,
        VALUE_TYPE_BYTE = 2,
        VALUE_TYPE_CHAR = 3,
        VALUE_TYPE_SHORT = 4,
        VALUE_TYPE_INT = 5,
        VALUE_TYPE_LONG = 6,
        VALUE_TYPE_FLOAT = 7,
        VALUE_TYPE_DOUBLE = 8,
        VALUE_TYPE_STRING = 9,
        VALUE_TYPE_CHARSEQUENCE = 10,
        VALUE_TYPE_BOOLEANARRAY = 11,
        VALUE_TYPE_BYTEARRAY = 12,
        VALUE_TYPE_CHARARRAY = 13,
        VALUE_TYPE_SHORTARRAY = 14,
        VALUE_TYPE_INTARRAY = 15,
        VALUE_TYPE_LONGARRAY = 16,
        VALUE_TYPE_FLOATARRAY = 17,
        VALUE_TYPE_DOUBLEARRAY = 18,
        VALUE_TYPE_STRINGARRAY = 19,
        VALUE_TYPE_CHARSEQUENCEARRAY = 20,

        VALUE_TYPE_PARCELABLE = 21,
        VALUE_TYPE_PARCELABLEARRAY = 22,
        VALUE_TYPE_SERIALIZABLE = 23,
        VALUE_TYPE_WANTPARAMSARRAY = 24,
        VALUE_TYPE_LIST = 50,

        VALUE_TYPE_WANTPARAMS = 101,
        VALUE_TYPE_ARRAY = 102,
        VALUE_TYPE_FD = 103,
        VALUE_TYPE_REMOTE_OBJECT = 104,
        VALUE_TYPE_INVALID_FD = 105,
    };

    bool WriteArrayToParcel(Parcel &parcel, IArray *ao, int depth) const;
    bool ReadArrayToParcel(Parcel &parcel, int type, sptr<IArray> &ao, int depth);
    bool ReadFromParcel(Parcel &parcel, int depth = 1);
    bool ReadFromParcelParam(Parcel &parcel, const std::string &key, int type, int depth);
    bool ReadFromParcelString(Parcel &parcel, const std::string &key);
    bool ReadFromParcelBool(Parcel &parcel, const std::string &key);
    bool ReadFromParcelInt8(Parcel &parcel, const std::string &key);
    bool ReadFromParcelChar(Parcel &parcel, const std::string &key);
    bool ReadFromParcelShort(Parcel &parcel, const std::string &key);
    bool ReadFromParcelInt(Parcel &parcel, const std::string &key);
    bool ReadFromParcelLong(Parcel &parcel, const std::string &key);
    bool ReadFromParcelFloat(Parcel &parcel, const std::string &key);
    bool ReadFromParcelDouble(Parcel &parcel, const std::string &key);

    bool ReadFromParcelArrayString(Parcel &parcel, sptr<IArray> &ao);
    bool ReadFromParcelArrayBool(Parcel &parcel, sptr<IArray> &ao);
    bool ReadFromParcelArrayByte(Parcel &parcel, sptr<IArray> &ao);
    bool ReadFromParcelArrayChar(Parcel &parcel, sptr<IArray> &ao);
    bool ReadFromParcelArrayShort(Parcel &parcel, sptr<IArray> &ao);

    bool ReadFromParcelArrayInt(Parcel &parcel, sptr<IArray> &ao);
    bool ReadFromParcelArrayLong(Parcel &parcel, sptr<IArray> &ao);
    bool ReadFromParcelArrayFloat(Parcel &parcel, sptr<IArray> &ao);
    bool ReadFromParcelArrayDouble(Parcel &parcel, sptr<IArray> &ao);
    bool ReadFromParcelArrayWantParams(Parcel &parcel, sptr<IArray> &ao, int depth);
    bool ReadFromParcelWantParamWrapper(Parcel &parcel, const std::string &key, int type, int depth);
    bool ReadFromParcelFD(Parcel &parcel, const std::string &key);
    bool ReadFromParcelRemoteObject(Parcel &parcel, const std::string &key);

    bool WriteArrayToParcelString(Parcel &parcel, IArray *ao) const;
    bool WriteArrayToParcelBool(Parcel &parcel, IArray *ao) const;
    bool WriteArrayToParcelByte(Parcel &parcel, IArray *ao) const;
    bool WriteArrayToParcelChar(Parcel &parcel, IArray *ao) const;
    bool WriteArrayToParcelShort(Parcel &parcel, IArray *ao) const;
    bool WriteArrayToParcelInt(Parcel &parcel, IArray *ao) const;
    bool WriteArrayToParcelLong(Parcel &parcel, IArray *ao) const;
    bool WriteArrayToParcelFloat(Parcel &parcel, IArray *ao) const;
    bool WriteArrayToParcelDouble(Parcel &parcel, IArray *ao) const;
    bool WriteArrayToParcelWantParams(Parcel &parcel, IArray *ao, int depth) const;

    bool WriteMarshalling(Parcel &parcel, sptr<IInterface> &o, int depth) const;
    bool WriteToParcelString(Parcel &parcel, sptr<IInterface> &o) const;
    bool WriteToParcelBool(Parcel &parcel, sptr<IInterface> &o) const;
    bool WriteToParcelByte(Parcel &parcel, sptr<IInterface> &o) const;
    bool WriteToParcelChar(Parcel &parcel, sptr<IInterface> &o) const;
    bool WriteToParcelShort(Parcel &parcel, sptr<IInterface> &o) const;
    bool WriteToParcelInt(Parcel &parcel, sptr<IInterface> &o) const;
    bool WriteToParcelLong(Parcel &parcel, sptr<IInterface> &o) const;
    bool WriteToParcelFloat(Parcel &parcel, sptr<IInterface> &o) const;
    bool WriteToParcelDouble(Parcel &parcel, sptr<IInterface> &o) const;
    bool WriteToParcelWantParams(Parcel &parcel, sptr<IInterface> &o, int depth) const;
    bool WriteToParcelFD(Parcel &parcel, const WantParams &value) const;
    bool WriteToParcelRemoteObject(Parcel &parcel, const WantParams &value) const;

    bool DoMarshalling(Parcel &parcel, int depth = 1) const;
    bool ReadUnsupportedData(Parcel &parcel, const std::string &key, int type);

    friend class WantParamWrapper;
    // inner use function
    bool NewArrayData(IArray *source, sptr<IArray> &dest);
    bool NewParams(const WantParams &source, WantParams &dest);
    bool NewFds(const WantParams &source, WantParams &dest);
    bool AddWantParamToInterfaceVector(const sptr<WantParams> &value,
        std::vector<sptr<IInterface>> &array) const;

    mutable bool needExpansion_ = false; // compatible DMS
    std::map<std::string, sptr<IInterface>> params_;
    std::map<std::string, int> fds_;
    std::vector<UnsupportedData> cachedUnsupportedData_;
};

void ParseWantParamsFromJsonString(const std::string &jsonString, WantParams &wantParams);

template<typename JsonType>
void from_json(const JsonType &jsonObject, WantParams &wantParams)
{
    ParseWantParamsFromJsonString(jsonObject.dump(), wantParams);
}

template<typename JsonType>
void to_json(JsonType &jsonObject, const WantParams &wantParams)
{
    jsonObject = JsonType::parse(wantParams.ToString(), nullptr, false);
}

}  // namespace AAFwk
}  // namespace OHOS

#endif  // OHOS_ABILITY_BASE_WANT_PARAMS_H
