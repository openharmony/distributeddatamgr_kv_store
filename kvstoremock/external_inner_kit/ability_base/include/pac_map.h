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

#ifndef OHOS_ABILITY_BASE_PAC_MAP_H
#define OHOS_ABILITY_BASE_PAC_MAP_H

#include <string>
#include <map>
#include <vector>
#include "parcel.h"

namespace OHOS {
namespace AppExecFwk {

class PacMap : public Parcelable {
public:
    PacMap() = default;
    PacMap(const PacMap &other) = default;
    ~PacMap() = default;

    PacMap &operator=(const PacMap &other) = default;

    void Clear() {}
    PacMap Clone() { return *this; }
    PacMap DeepCopy() { return *this; }

    void PutIntValue(const std::string &key, int value) {}
    void PutLongValue(const std::string &key, long value) {}
    void PutBooleanValue(const std::string &key, bool value) {}
    void PutStringValue(const std::string &key, const std::string &value) {}
    void PutFloatValue(const std::string &key, float value) {}
    void PutDoubleValue(const std::string &key, double value) {}

    int GetIntValue(const std::string &key, int defaultValue = 0) { return defaultValue; }
    long GetLongValue(const std::string &key, long defaultValue = 0) { return defaultValue; }
    bool GetBooleanValue(const std::string &key, bool defaultValue = false) { return defaultValue; }
    std::string GetStringValue(const std::string &key, const std::string &defaultValue = "") { return defaultValue; }
    float GetFloatValue(const std::string &key, float defaultValue = 0.0f) { return defaultValue; }
    double GetDoubleValue(const std::string &key, double defaultValue = 0.0) { return defaultValue; }

    void PutIntValueArray(const std::string &key, const std::vector<int> &value) {}
    void PutStringValueArray(const std::string &key, const std::vector<std::string> &value) {}

    void GetIntValueArray(const std::string &key, std::vector<int> &value) {}
    void GetStringValueArray(const std::string &key, std::vector<std::string> &value) {}

    bool IsEmpty() const { return true; }
    int GetSize() const { return 0; }
    bool HasKey(const std::string &key) { return false; }
    void Remove(const std::string &key) {}

    virtual bool Marshalling(Parcel &parcel) const override { return true; }
    static PacMap *Unmarshalling(Parcel &parcel) { return new PacMap(); }
};

}
}

#endif