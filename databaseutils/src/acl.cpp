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

#define LOG_TAG "DataBaseUtilsAcl"
#include "acl.h"

#include "securec.h"
#include <cerrno>
#include <dlfcn.h>
#include <functional>
#include <iosfwd>
#include <memory>
#include <new>
#include <sys/stat.h>
#include <sys/xattr.h>
#include <type_traits>
#include "log_print.h"


namespace OHOS {
namespace DATABASE_UTILS {
using namespace DistributedKv;
Acl::Acl(const std::string &path) : path_(path), hasError_(false)
{
    /* init acl from file's defaule or mode*/
    AclFromDefault();
}

Acl::Acl()
{
}

ACL_PERM Acl::ReCalcMaskPerm()
{
    ACL_PERM perm;
    for (const auto &e : entries_) {
        if (e.tag_ == ACL_TAG::USER || e.tag_ == ACL_TAG::GROUP_OBJ || e.tag_ == ACL_TAG::GROUP) {
            perm.Merge(e.perm_);
        }
    }
    return perm;
}

bool Acl::IsEmpty()
{
    return entries_.empty();
}

void Acl::CompareInsertEntry(const AclXattrEntry &entry)
{
    if (entries_.count(entry)) {
        auto it = entries_.find(entry);
        entries_.erase(it);
    }
    if (entry.perm_.IsReadable() || entry.perm_.IsWritable() ||
        entry.perm_.IsExecutable()) {
        entries_.insert(entry);
    }
}

int Acl::InsertEntry(const AclXattrEntry &entry)
{
    if (entries_.size() >= ENTRIES_MAX_NUM) {
        return E_ERROR;
    }
    CompareInsertEntry(entry); // must before ReCalcMaskPerm()

    maskDemand_++;
    /*
    * In either case there's no or already one ACL_MASK entry in the set,
    * we need to re-calculate MASK's permission and *insert* it (to replace
    * the old one in latter case since we can't change std::set's element
    * in-place). So do the following unconditionally.
    *
    * Be warned: do _NOT_ combine the following into one line, otherwise
    * you can't pass the !!genius!! CI coding style check.
    */
    CompareInsertEntry(AclXattrEntry(ACL_TAG::MASK, AclXattrEntry::ACL_UNDEFINED_ID, ReCalcMaskPerm()));
    return E_OK;
}

std::unique_ptr<char[]> Acl::Serialize(uint32_t &bufSize)
{
    bufSize = sizeof(AclXattrHeader) + sizeof(AclXattrEntry) * entries_.size();
    if (bufSize > BUF_MAX_SIZE) {
        bufSize = 0;
        return nullptr;
    }
    auto buf = std::make_unique<char[]>(bufSize);
    auto err = memcpy_s(buf.get(), bufSize, &header_, sizeof(AclXattrHeader));
    if (err != EOK) {
        bufSize = 0;
        return nullptr;
    }

    int32_t restSize = static_cast<int32_t>(bufSize - sizeof(AclXattrHeader));
    AclXattrEntry *ptr = reinterpret_cast<AclXattrEntry *>(buf.get() + sizeof(AclXattrHeader));
    for (const auto &e : entries_) {
        auto err = memcpy_s(ptr++, restSize, &e, sizeof(AclXattrEntry));
        if (err != EOK) {
            bufSize = 0;
            return nullptr;
        }
        restSize -= sizeof(AclXattrEntry);
    }
    return buf;
}

int Acl::DeSerialize(const char *p, int32_t bufSize)
{
    header_ = *reinterpret_cast<const AclXattrHeader *>(p);
    bufSize -= sizeof(AclXattrHeader);
    p += sizeof(AclXattrHeader);

    /*
     * `e->tag != ACL_TAG::UNDEFINED` is unreliable outside the buffer, so check
     * it after checking the size of remaining buffer.
     */
    for (const AclXattrEntry *e = reinterpret_cast<const AclXattrEntry *>(p);
            bufSize >= static_cast<int32_t>(sizeof(AclXattrEntry)) && e->tag_ != ACL_TAG::UNDEFINED;
            e++) {
        InsertEntry(*e);
        bufSize -= sizeof(AclXattrEntry);
    }
    if (bufSize < 0) {
        entries_.clear();
        header_ = { 0 };
        return -1;
    }

    return 0;
}

void Acl::AclFromDefault()
{
    char buf[BUF_SIZE] = { 0 };
    ssize_t len = getxattr(path_.c_str(), ACL_XATTR_DEFAULT, buf, BUF_SIZE);
    if (len != -1) {
        DeSerialize(buf, BUF_SIZE);
    } else if (errno == ENODATA) {
        AclFromMode();
    } else {
        hasError_ = true;
        ZLOGW("getxattr failed. error %{public}s path %{public}s", std::strerror(errno), path_.c_str());
    }
}

void Acl::AclFromMode()
{
    struct stat st;
    if (stat(path_.c_str(), &st) == -1) {
        return;
    }

    InsertEntry(AclXattrEntry(ACL_TAG::USER_OBJ, AclXattrEntry::ACL_UNDEFINED_ID,
        (st.st_mode & S_IRWXU) >> USER_OFFSET));
    InsertEntry(AclXattrEntry(ACL_TAG::GROUP_OBJ, AclXattrEntry::ACL_UNDEFINED_ID,
        (st.st_mode & S_IRWXG) >> GROUP_OFFSET));
    InsertEntry(AclXattrEntry(ACL_TAG::OTHER, AclXattrEntry::ACL_UNDEFINED_ID,
        (st.st_mode & S_IRWXO)));
}

int32_t Acl::SetDefault()
{
    if (IsEmpty()) {
        ZLOGE("Failed to generate ACL from file's mode: %{public}s", std::strerror(errno));
        return E_ERROR;
    }

    /* transform to binary and write to file */
    uint32_t bufSize;
    auto buf = Serialize(bufSize);
    if (buf == nullptr) {
        ZLOGE("Failed to serialize ACL into binary: %{public}s", std::strerror(errno));
        return E_ERROR;
    }
    if (setxattr(path_.c_str(), ACL_XATTR_DEFAULT, buf.get(), bufSize, 0) == -1) {
        ZLOGE("Failed to write into file's xattr: %{public}s", std::strerror(errno));
        return E_ERROR;
    }
    return E_OK;
}

int32_t Acl::SetDefaultGroup(const uint32_t gid, const uint16_t mode)
{
    return InsertEntry(AclXattrEntry(ACL_TAG::GROUP, gid, mode));
}

int32_t Acl::SetDefaultUser(const uint32_t uid, const uint16_t mode)
{
    return InsertEntry(AclXattrEntry(ACL_TAG::USER, uid, mode));
}

Acl::~Acl()
{
    if (!hasError_) {
        SetDefault();
    }
}

bool Acl::HasEntry(const AclXattrEntry &Acl)
{
    return entries_.find(Acl) != entries_.end();
}
}
}