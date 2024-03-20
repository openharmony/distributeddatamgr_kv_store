/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#ifndef OHOS_DISTRIBUTED_DATA_DATABASE_UTILS_ACL_H
#define OHOS_DISTRIBUTED_DATA_DATABASE_UTILS_ACL_H

#include "visibility.h"
#include <set>
#include <stddef.h>
#include <stdint.h>
#include <string>
namespace OHOS {
namespace DATABASE_UTILS {
/*
 * ACL tag values
 */
enum class ACL_TAG : uint16_t {
    UNDEFINED = 0x00,
    USER_OBJ = 0x01,
    USER = 0x02,
    GROUP_OBJ = 0x04,
    GROUP = 0x08,
    MASK = 0x10,
    OTHER = 0x20,
};

/*
 * ACL perm values
 */
class ACL_PERM {
public:
    uint16_t value_ = 0;
    enum Value : uint16_t {
        READ = 0x04,
        WRITE = 0x02,
        EXECUTE = 0x01,
    };

public:
    ACL_PERM() = default;
    ACL_PERM(const uint16_t x)
    {
        value_ = (x & READ) | (x & WRITE) | (x & EXECUTE);
    }
    void SetR() { value_ |= READ; }
    void SetW() { value_ |= WRITE; }
    void SetE() { value_ |= EXECUTE; }
    bool IsReadable() const { return (value_ & READ) == READ; }
    bool IsWritable() const { return (value_ & WRITE) == WRITE; }
    bool IsExecutable() const { return (value_ & EXECUTE) == EXECUTE; }
    void Merge(const ACL_PERM &acl_perm) { value_ |= acl_perm.value_; }
};


/*
 * ACL data structure
 */
struct AclXattrHeader {
    static constexpr uint32_t ACL_EA_VERSION = 0x0002;
    uint32_t version = ACL_EA_VERSION;
};

struct AclXattrEntry {
    static constexpr uint32_t ACL_UNDEFINED_ID = static_cast<uint32_t>(-1);
    ACL_TAG tag_ = ACL_TAG::UNDEFINED;
    ACL_PERM perm_ = {};
    uint32_t id_ = ACL_UNDEFINED_ID;

    AclXattrEntry(const ACL_TAG tag, const uint32_t id, const ACL_PERM mode) : tag_(tag), perm_(mode), id_(id)
    {
    }

    bool IsValid() const
    {
        if (tag_ == ACL_TAG::USER || tag_ == ACL_TAG::GROUP) {
            return id_ != ACL_UNDEFINED_ID;
        }
        return tag_ != ACL_TAG::UNDEFINED;
    }

    bool operator<(const AclXattrEntry &rhs) const
    {
        if (tag_ == rhs.tag_) {
            return id_ < rhs.id_;
        }
        return tag_ < rhs.tag_;
    }

    bool operator==(const AclXattrEntry &rhs) const
    {
        return tag_ == rhs.tag_ && perm_.value_ == rhs.perm_.value_ && id_ == rhs.id_;
    }

    friend inline bool operator<(const AclXattrEntry &lhs, const ACL_TAG &rhs)
    {
        return lhs.tag_ < rhs;
    }

    friend inline bool operator<(const ACL_TAG &lhs, const AclXattrEntry &rhs)
    {
        return lhs < rhs.tag_;
    }
};

class Acl {
public:
    static constexpr uint16_t R_RIGHT = 4;
    static constexpr uint16_t W_RIGHT = 2;
    static constexpr uint16_t E_RIGHT = 1;

    API_EXPORT Acl(const std::string &path);
    Acl();
    API_EXPORT ~Acl();
    API_EXPORT int32_t SetDefaultGroup(const uint32_t gid, const uint16_t mode);
    API_EXPORT int32_t SetDefaultUser(const uint32_t uid, const uint16_t mode);
    // just for Acl Test
    bool HasEntry(const AclXattrEntry &entry);

private:
    /*
     * ACL extended attributes (xattr) names
    */
    static constexpr const char *ACL_XATTR_DEFAULT = "system.posix_acl_default";
    static constexpr int32_t E_OK = 0;
    static constexpr int32_t E_ERROR = -1;
    static constexpr int32_t USER_OFFSET = 6;
    static constexpr int32_t GROUP_OFFSET = 3;
    static constexpr int32_t BUF_SIZE = 400;
    static constexpr size_t ENTRIES_MAX_NUM = 100; // just heuristic
    static constexpr size_t BUF_MAX_SIZE = sizeof(AclXattrHeader) + sizeof(AclXattrEntry) * ENTRIES_MAX_NUM;
    bool IsEmpty();
    int32_t SetDefault();
    void AclFromMode();
    void AclFromDefault();
    void CompareInsertEntry(const AclXattrEntry &entry);
    ACL_PERM ReCalcMaskPerm();
    std::unique_ptr<char[]> Serialize(uint32_t &bufSize);
    int DeSerialize(const char *p, int32_t bufSize);
    int InsertEntry(const AclXattrEntry &entry);
    AclXattrHeader header_;
    /*
    * Only one entry should exist for the following types:
    *     ACL_USER_OBJ
    *     ACL_GROUP_OBJ
    *     ACL_MASK
    *     ACL_OTHER
    * While for these types, multiple entries could exist, but one entry
    * for each id (i.e. uid/gid):
    *     ACL_USER
    *     ACL_GROUP
    */
    std::set<AclXattrEntry, std::less<>> entries_;
    unsigned maskDemand_ = 0;
    std::string path_;
    bool hasError_ = false;
};
} // DATABASE_UTILS
} // namespace OHOS

#endif // OHOS_DISTRIBUTED_DATA_DATABASE_UTILS_ACL_H