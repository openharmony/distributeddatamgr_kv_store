/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef SAMGR_INTERFACE_INNERKITS_COMMOM_INCLUDE_PARSE_UTIL_H
#define SAMGR_INTERFACE_INNERKITS_COMMOM_INCLUDE_PARSE_UTIL_H

#include <list>
#include <map>
#include <set>
#include <string>
#include "sa_profiles.h"
#include "nlohmann/json.hpp"

namespace OHOS {
class ParseUtil {
public:
    ~ParseUtil();
    bool ParseSaProfiles(const std::string& profilePath);
    const std::list<SaProfile>& GetAllSaProfiles() const;
    bool GetProfile(int32_t saId, SaProfile& saProfile);
    const std::set<int32_t>& GetMultiInstanceSaIds() const;
    void ClearResource();
    void OpenSo(uint32_t bootPhase);
    void CloseSo(int32_t systemAbilityId);
    bool LoadSaLib(int32_t systemAbilityId);
    bool ParseTrustConfig(const std::string& profilePath, std::map<std::u16string, std::set<int32_t>>& values);
    void RemoveSaProfile(int32_t saId);
    bool CheckPathExist(const std::string& profilePath);
    std::u16string GetProcessName() const;
    static std::unordered_map<std::string, std::string> StringToMap(const std::string& eventStr);
    static nlohmann::json StringToJsonObj(const std::string& eventStr);
    static std::unordered_map<std::string, std::string> JsonObjToMap(const nlohmann::json& eventJson);
    static bool CheckLogicRelationship(const std::string& state, const std::string& profile);
private:
    void CloseSo();
    uint32_t GetBootPriorityPara(const std::string& bootPhase);
    uint32_t GetOndemandPriorityPara(const std::string& loadpriority);
    void OpenSo(SaProfile& saProfile);
    void CloseHandle(SaProfile& saProfile);
    bool ParseJsonFile(const std::string& realPath);
    bool ParseJsonObj(nlohmann::json& jsonObj, const std::string& jsonPath);
    bool ParseSystemAbilityGetSaBaseInfo(SaProfile& saProfile, nlohmann::json& systemAbilityJson);
    bool ParseSystemAbilityGetSaExtInfo(SaProfile& saProfile, nlohmann::json& systemAbilityJson);
    bool ParseSystemAbility(SaProfile& saProfile, nlohmann::json& systemAbilityJson);
    void ParseMultiInstanceSaIds(const nlohmann::json& systemAbilityJson);
    bool ParseJsonTag(const nlohmann::json& systemAbilityJson, const std::string& jsonTag,
        nlohmann::json& onDemandJson);
    void ParseOndemandTag(const nlohmann::json& onDemandJson, std::vector<OnDemandEvent>& onDemandEvents);
    void ParseStartOndemandTag(const nlohmann::json& systemAbilityJson,
        const std::string& jsonTag, StartOnDemand& startOnDemand);
    void ParseStopOndemandTag(const nlohmann::json& systemAbilityJson,
        const std::string& jsonTag, StopOnDemand& stopOnDemand);
    bool ParseSystemAbilityGetExtension(SaProfile& saProfile, nlohmann::json& systemAbilityJson);
    void GetOnDemandArrayFromJson(int32_t eventId, const nlohmann::json& obj,
        const std::string& key, std::vector<OnDemandEvent>& out);
    void GetOnDemandConditionsFromJson(const nlohmann::json& obj,
        const std::string& key, std::vector<OnDemandCondition>& out);
    void GetOnDemandExtraMessagesFromJson(const nlohmann::json& obj,
        const std::string& key, std::map<std::string, std::string>& out);
    bool CheckRecycleStrategy(const std::string& recycleStrategyStr, int32_t& recycleStrategy);

    static inline void GetBoolFromJson(const nlohmann::json& obj, const std::string& key, bool& out)
    {
        if (obj.find(key.c_str()) != obj.end() && obj[key.c_str()].is_boolean()) {
            obj[key.c_str()].get_to(out);
        }
    }

    static inline void GetStringFromJson(const nlohmann::json& obj, const std::string& key, std::string& out)
    {
        if (obj.find(key.c_str()) != obj.end() && obj[key.c_str()].is_string()) {
            obj[key.c_str()].get_to(out);
        }
    }

    static inline void GetInt32FromJson(const nlohmann::json& obj, const std::string& key, int32_t& out)
    {
        if (obj.find(key.c_str()) != obj.end() && obj[key.c_str()].is_number_integer()) {
            obj[key.c_str()].get_to(out);
        }
    }

    static inline void GetStringArrayFromJson(const nlohmann::json& obj, const std::string& key,
        std::vector<std::string>& out)
    {
        if (obj.find(key.c_str()) != obj.end() && obj[key.c_str()].is_array()) {
            for (auto& item : obj[key.c_str()]) {
                if (item.is_string()) {
                    out.emplace_back(item.get<std::string>());
                }
            }
        }
    }

    static inline void GetIntArrayFromJson(const nlohmann::json& obj, const std::string& key,
        std::set<int32_t>& out)
    {
        if (obj.find(key.c_str()) != obj.end() && obj[key.c_str()].is_array()) {
            for (auto& item : obj[key.c_str()]) {
                if (item.is_number_integer()) {
                    out.insert(item.get<int32_t>());
                }
            }
        }
    }

    static inline void GetIntArrayFromJson(const nlohmann::json& obj, const std::string& key,
        std::vector<int32_t>& out)
    {
        if (obj.find(key.c_str()) != obj.end() && obj[key.c_str()].is_array()) {
            for (auto& item : obj[key.c_str()]) {
                if (item.is_number_integer()) {
                    out.emplace_back(item.get<int32_t>());
                }
            }
        }
    }
    static bool Endswith(const std::string& src, const std::string& sub);
    std::string GetRealPath(const std::string& profilePath) const;
    std::list<SaProfile> saProfiles_;
    std::u16string procName_;
    std::vector<std::string> updateVec_;
    std::set<int32_t> multiInstanceSaIds_;
};
} // namespace OHOS

#endif // SAMGR_INTERFACE_INNERKITS_COMMOM_INCLUDE_PARSE_UTIL_H
