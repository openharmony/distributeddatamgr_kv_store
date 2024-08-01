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
#ifndef DB_PROPERTIES_H
#define DB_PROPERTIES_H

#include <map>
#include <string>

namespace DistributedDB {
class DBProperties {
public:
    DBProperties() = default;
    virtual ~DBProperties() = default;

    // Get the string property according the name
    std::string GetStringProp(const std::string &name, const std::string &defaultValue) const;

    // Set the string property for the name
    void SetStringProp(const std::string &name, const std::string &value);

    // Get the bool property according the name
    bool GetBoolProp(const std::string &name, bool defaultValue) const;

    // Set the bool property for the name
    void SetBoolProp(const std::string &name, bool value);

    // Get the integer property according the name
    int GetIntProp(const std::string &name, int defaultValue) const;

    // Set the integer property for the name
    void SetIntProp(const std::string &name, int value);

    // Get the unsigned integer property according the name
    uint32_t GetUIntProp(const std::string &name, uint32_t defaultValue) const;

    // Set the unsigned integer property for the name
    void SetUIntProp(const std::string &name, uint32_t value);

    // Set all indentifers
    void SetIdentifier(const std::string &userId, const std::string &appId, const std::string &storeId,
        const std::string &subUser = "", int32_t instanceId = 0);

    static constexpr const char *CREATE_IF_NECESSARY = "createIfNecessary";
    static constexpr const char *DATA_DIR = "dataDir";
    static constexpr const char *USER_ID = "userId";
    static constexpr const char *APP_ID = "appId";
    static constexpr const char *STORE_ID = "storeId";
    static constexpr const char *INSTANCE_ID = "instanceId";
    static constexpr const char *SUB_USER = "subUser";
    static constexpr const char *IDENTIFIER_DATA = "identifier";
    static constexpr const char *IDENTIFIER_DIR = "identifierDir";
    static constexpr const char *DUAL_TUPLE_IDENTIFIER_DATA = "dualTupleIdentifier";
    static constexpr const char *SYNC_DUAL_TUPLE_MODE = "syncDualTuple";
    static constexpr const char *AUTO_LAUNCH_ID = "autoLaunchID";

    static constexpr const char *DATABASE_TYPE = "databaseType";

protected:

    std::map<std::string, std::string> stringProperties_;
    std::map<std::string, bool> boolProperties_;
    std::map<std::string, int> intProperties_;
    std::map<std::string, uint32_t> uintProperties_;
};
}
#endif