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

#ifndef CHANGE_NOTIFICATION_H
#define CHANGE_NOTIFICATION_H

#include <list>
#include "types.h"

namespace OHOS {
namespace DistributedKv {
class ChangeNotification final {
public:
    /**
     * @brief Constructor of ChangeNotification.
     * @param insertEntries Inserted entries.
     * @param updateEntries Updated entries.
     * @param deleteEntries Deleted entries.
     * @param deviceId The device ID.
     * @param icClear Indicate whether the clear function cause this change.
    */
    API_EXPORT ChangeNotification(std::vector<Entry> &&insertEntries, std::vector<Entry> &&updateEntries,
                       std::vector<Entry> &&deleteEntries, const std::string &deviceId, bool isClear);

    /**
     * @brief Destructor.
     */
    API_EXPORT ~ChangeNotification();

    /**
     * @brief Get all inserted entries in this change.
    */
    API_EXPORT const std::vector<Entry> &GetInsertEntries() const;

    /**
     * @brief Get all updated entries in this changing.
    */
    API_EXPORT const std::vector<Entry> &GetUpdateEntries() const;

    /**
     * @brief Get all deleted entries in this changing.
    */
    API_EXPORT const std::vector<Entry> &GetDeleteEntries() const;

    /**
     * @brief Get the device ID.
    */
    API_EXPORT const std::string &GetDeviceId() const;

    /**
     * @brief Check if this change is made by calling the clear function.
     * @return Return true if caused by clear function, otherwise false.
    */
    API_EXPORT bool IsClear() const;

private:
    std::vector<Entry> insertEntries_;

    std::vector<Entry> updateEntries_;

    std::vector<Entry> deleteEntries_;

    std::string deviceId_;

    bool isClear_ = false;
};
}  // namespace DistributedKv
}  // namespace OHOS
#endif  // CHANGE_NOTIFICATION_H
