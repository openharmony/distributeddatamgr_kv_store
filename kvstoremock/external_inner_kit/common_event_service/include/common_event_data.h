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

#ifndef FOUNDATION_EVENT_CESFWK_KITS_NATIVE_INCLUDE_COMMON_EVENT_DATA_H
#define FOUNDATION_EVENT_CESFWK_KITS_NATIVE_INCLUDE_COMMON_EVENT_DATA_H

#include "parcel.h"
#include "want.h"

namespace OHOS {
namespace EventFwk {
using Want = OHOS::AAFwk::Want;

class CommonEventData : public Parcelable {
public:
    CommonEventData();

    /**
     * Creates a CommonEventData instance based on the want parameter to set
     * the content of a common want.
     *
     * @param want Indicates the want of a common event.
     */
    explicit CommonEventData(const Want &want);

    /**
     * Creates a CommonEventData instance based on the parameters want, code,
     * and data to set the content of the common event.
     *
     * @param want Indicates the want of a common event.
     * @param code Indicates the code of a common event.
     * @param data Indicates the data of a common event.
     */
    CommonEventData(const Want &want, const int32_t &code, const std::string &data);

    ~CommonEventData();

    /**
     * Sets the want attribute of a common event.
     *
     * @param want Indicates the want of a common event.
     */
    void SetWant(const Want &want);

    /**
     * Obtains the Want attribute of a common event.
     *
     * @return Returns the want of a common event.
     */
    const Want &GetWant() const;

    /**
     * Sets the result code of the common event.
     *
     * @param code Indicates the code of a common event.
     */
    void SetCode(const int32_t &code);

    /**
     * Obtains the result code of a common event.
     *
     * @return Returns the code of a common event.
     */
    int32_t GetCode() const;

    /**
     * Sets the result data of a common event.
     *
     * @param data Indicates the data of a common event.
     */
    void SetData(const std::string &data);

    /**
     * Obtains the result data of a common event
     *
     * @return Returns the data of a common event.
     */
    std::string GetData() const;

    /**
     * Marshals a common event data object into a Parcel.
     *
     * @param parcel Indicates specified Parcel object.
     * @return Returns true if success; false otherwise.
     */
    virtual bool Marshalling(Parcel &parcel) const override;

    /**
     * Unmarshals this common event data object from a Parcel.
     * @return Returns the common event data.
     */
    static CommonEventData *Unmarshalling(Parcel &parcel);

private:
    /**
     * Reads a CommonEventData object from a Parcel.
     *
     * @param parcel Indicates specified Parcel object.
     * @return Returns true if success; false otherwise.
     */
    bool ReadFromParcel(Parcel &parcel);

private:
    Want want_;
    int32_t code_;
    std::string data_;
    static constexpr int VALUE_NULL = -1;
    static constexpr int VALUE_OBJECT = 1;
};
}  // namespace EventFwk
}  // namespace OHOS

#endif  // FOUNDATION_EVENT_CESFWK_KITS_NATIVE_INCLUDE_COMMON_EVENT_DATA_H