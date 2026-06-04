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

#ifndef FOUNDATION_APPEXECFWK_INTERFACES_INNERKITS_APPEXECFWK_BASE_INCLUDE_SKILL_H
#define FOUNDATION_APPEXECFWK_INTERFACES_INNERKITS_APPEXECFWK_BASE_INCLUDE_SKILL_H

#include <string>

#include "parcel.h"
#include "application_info.h"
#include "nlohmann/json.hpp"
#include "want.h"

namespace OHOS {
namespace AppExecFwk {

struct SkillUri {
    int32_t maxFileSupported = 0;
    std::string scheme;
    std::string host;
    std::string port;
    std::string path;
    std::string pathStartWith;
    std::string pathRegex;
    std::string type;
    std::string utd;
    std::string linkFeature;
};

struct Skill : public Parcelable {
public:
    bool domainVerify = false;
    std::vector<std::string> actions;
    std::vector<std::string> entities;
    std::vector<SkillUri> uris;
    std::vector<std::string> permissions;
    static std::string GetOptParamUri(const std::string &uriString);
    bool Match(const OHOS::AAFwk::Want &want) const;
    bool Match(const OHOS::AAFwk::Want &want, size_t &matchUriIndex) const;
    bool MatchLauncher(const OHOS::AAFwk::Want &want) const;
    bool MatchType(const std::string &type, const std::string &skillUriType) const;
    bool ReadFromParcel(Parcel &parcel);
    virtual bool Marshalling(Parcel &parcel) const override;
    static Skill *Unmarshalling(Parcel &parcel);
    void Dump(std::string prefix, int fd);
    void from_json(const nlohmann::json &jsonObject, SkillUri &uri);
    void from_json(const nlohmann::json &jsonObject, Skill &skill);
    void to_json(nlohmann::json &jsonObject, const SkillUri &uri);
    void to_json(nlohmann::json &jsonObject, const Skill &skill);
    std::string ConvertToLower(const std::string &str) const;
    std::string ConvertUriToLower(const std::string& uri) const;
private:
    bool MatchAction(const std::string &action) const;
    bool MatchEntities(const std::vector<std::string> &paramEntities) const;
    bool MatchUriAndType(const std::string &rawUriString, const std::string &type) const;
    bool MatchUriAndType(const std::string &rawUriString, const std::string &type, size_t &matchUriIndex) const;
    bool MatchUri(const std::string &uriString, const SkillUri &skillUri) const;
    bool StartsWith(const std::string &sourceString, const std::string &targetPrefix) const;
    bool MatchMimeType(const std::string &uriString) const;
    bool MatchMimeType(const std::string &uriString, size_t &matchUriIndex) const;
    bool MatchLinkFeature(const std::string &linkFeature, const OHOS::AAFwk::Want &want, size_t &matchUriIndex) const;
    bool MatchUtd(const std::string &paramType, const std::string &skillUriType, bool &containsUtd) const;
    bool IsUtdMatch(const std::string &paramUtd, const std::string &skillUtd) const;
    bool IsUtd(const std::string &param) const;
};
}  // namespace AppExecFwk
}  // namespace OHOS
#endif  // FOUNDATION_APPEXECFWK_INTERFACES_INNERKITS_APPEXECFWK_BASE_INCLUDE_ABILITY_INFO_H
