/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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
#include "acl.h"

#include <dlfcn.h>
#include <fcntl.h>
#include <string>
#include <sys/xattr.h>
#include <unistd.h>
#include "gtest/gtest.h"
#include "securec.h"
using namespace testing::ext;
namespace OHOS::Test {
using namespace DATABASE_UTILS;
class AclTest : public testing::Test {
public:
    static constexpr const char *PATH_ABC = "/data/test/abc";
    static constexpr const char *PATH_ABC_XIAOMING = "/data/test/abc/xiaoming";
    static constexpr const char *PATH_ABC_XIAOMING_TEST = "/data/test/abc/xiaoming/test.txt";
    static constexpr const char *DATA = "SetDefaultUserTest";
    static constexpr uint32_t UID = 2024;
    static constexpr uint32_t TESTUID = 2025;
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
    void PreOperation() const;
};

void AclTest::SetUpTestCase(void)
{
}

void AclTest::TearDownTestCase(void)
{
}

// input testcase setup step，setup invoked before each testcases
void AclTest::SetUp(void)
{
    (void)remove(PATH_ABC);
}

// input testcase teardown step，teardown invoked after each testcases
void AclTest::TearDown(void)
{
    (void)remove(PATH_ABC);
}

void AclTest::PreOperation() const
{
    mode_t mode = S_IRWXU | S_IRWXG | S_IXOTH; // 0771
    int res = mkdir(PATH_ABC, mode);
    EXPECT_EQ(res, 0) << "directory creation failed." << std::strerror(errno);

    Acl acl(PATH_ABC);
    acl.SetDefaultUser(UID, Acl::R_RIGHT | Acl::W_RIGHT);
    acl.SetDefaultGroup(UID, Acl::R_RIGHT | Acl::W_RIGHT);

    res = mkdir(PATH_ABC_XIAOMING, mode);
    EXPECT_EQ(res, 0) << "directory creation failed." << std::strerror(errno);

    int fd = open(PATH_ABC_XIAOMING_TEST, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
    EXPECT_NE(fd, -1) << "open file failed." << std::strerror(errno);
    res = write(fd, DATA, strlen(DATA));
    EXPECT_NE(res, -1) << "write failed." << std::strerror(errno);
    res = fsync(fd);
    EXPECT_NE(res, -1) << "fsync failed." << std::strerror(errno);
    close(fd);
}

/**
* @tc.name: SetDefaultGroup001
* @tc.desc: Set default extended properties for groups.
* @tc.type: FUNC
* @tc.require:
* @tc.author: Jiaxing Chang
*/
HWTEST_F(AclTest, SetDefaultGroup001, TestSize.Level0)
{
    mode_t mode = S_IRWXU | S_IRWXG | S_IXOTH; // 0771
    int res = mkdir(PATH_ABC, mode);
    EXPECT_EQ(res, 0) << "directory creation failed.";
    int rc = Acl(PATH_ABC).SetDefaultGroup(UID, Acl::R_RIGHT | Acl::W_RIGHT);
    EXPECT_EQ(rc, 0);

    Acl aclNew(PATH_ABC);
    AclXattrEntry entry(ACL_TAG::GROUP, UID, Acl::R_RIGHT | Acl::W_RIGHT);
    ASSERT_TRUE(aclNew.HasEntry(entry));
}

/**
* @tc.name: SetDefaultpUser001
* @tc.desc: Set default extended properties for user.
* @tc.type: FUNC
* @tc.require:
* @tc.author: Jiaxing Chang
*/
HWTEST_F(AclTest, SetDefaultUser001, TestSize.Level0)
{
    mode_t mode = S_IRWXU | S_IRWXG | S_IXOTH; // 0771
    int res = mkdir(PATH_ABC, mode);
    EXPECT_EQ(res, 0) << "directory creation failed.";
    int rc = Acl(PATH_ABC).SetDefaultUser(UID, Acl::R_RIGHT | Acl::W_RIGHT);
    EXPECT_EQ(rc, 0);

    Acl aclNew(PATH_ABC);
    AclXattrEntry entry(ACL_TAG::USER, UID, Acl::R_RIGHT | Acl::W_RIGHT);
    ASSERT_TRUE(aclNew.HasEntry(entry));
}

/**
* @tc.name: SetDefaultUser002
* @tc.desc: After the main process extends the uid attribute, set this uid to the uid and gid of the child process,
* and the child process can access the files created by the main process normally.
* @tc.type: FUNC
* @tc.require:
* @tc.author: Jiaxing Chang
*/
HWTEST_F(AclTest, SetDefaultUser002, TestSize.Level0)
{
    PreOperation();
    int fd[2];
    pid_t pid;
    char buf[100];
    int res = pipe(fd);
    ASSERT_TRUE(res >= 0) << "create pipe failed." << std::strerror(errno);
    pid = fork();
    ASSERT_TRUE(pid >= 0) << "fork failed." << std::strerror(errno);
    if (pid == 0) { // subprocess
        // close the read end of the pipeline.
        close(fd[0]);
        // redirect standard output to the write end of the pipeline
        dup2(fd[1], STDOUT_FILENO);
        auto exitFun = [&fd](const std::string &str, bool isErr) {
            std::cout << str << (isErr ? std::strerror(errno) : "") << std::endl;
            close(fd[1]);
            _exit(0);
        };
        if (setuid(UID) != 0) {
            exitFun("setuid failed.", true);
        }
        if (setgid(UID) != 0) {
            exitFun("setgid failed.", true);
        }
        int file = open(PATH_ABC_XIAOMING_TEST, O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
        if (file == -1) {
            exitFun("open file failed.", true);
        }
        if (read(file, buf, sizeof(buf)) == -1) {
            close(file);
            exitFun("read failed.", true);
        }
        close(file);
        exitFun(buf, false);
    } else { // main process
        // close the write end of the pipeline.
        close(fd[1]);
        int status;
        res = waitpid(pid, &status, 0);
        EXPECT_NE(res, -1) << "waitpid falied." << std::strerror(errno);
        res = memset_s(buf, sizeof(buf), 0, sizeof(buf));
        EXPECT_EQ(res, EOK) << "memset_s falied." << std::strerror(errno);
        res = read(fd[0], buf, sizeof(buf));
        EXPECT_NE(res, -1) << "read falied." << std::strerror(errno);
        EXPECT_EQ(std::string(buf, buf + strlen(buf) - 1), std::string(DATA)) << "buffer:[" << buf << "]";
        close(fd[0]);
    }
}

/**
* @tc.name: AclXattrEntry001
* @tc.desc: Test operator.
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(AclTest, AclXattrEntry001, TestSize.Level0)
{
    AclXattrEntry entryA(ACL_TAG::USER, UID, Acl::R_RIGHT | Acl::W_RIGHT);
    AclXattrEntry entryB(ACL_TAG::USER, UID, Acl::R_RIGHT | Acl::W_RIGHT);
    EXPECT_TRUE(entryA == entryB);

    AclXattrEntry entryC(ACL_TAG::USER, TESTUID, Acl::R_RIGHT | Acl::W_RIGHT);
    EXPECT_FALSE(entryA == entryC);
}

/**
* @tc.name: AclXattrEntry002
* @tc.desc: Test IsValid().
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(AclTest, AclXattrEntry002, TestSize.Level0)
{
    AclXattrEntry entryA(ACL_TAG::USER, UID, Acl::R_RIGHT | Acl::W_RIGHT);
    auto result = entryA.IsValid();
    EXPECT_TRUE(result);

    AclXattrEntry entryB(ACL_TAG::GROUP, UID, Acl::R_RIGHT | Acl::W_RIGHT);
    result = entryB.IsValid();
    EXPECT_TRUE(result);

    AclXattrEntry entryC(ACL_TAG::UNDEFINED, UID, Acl::R_RIGHT | Acl::W_RIGHT);
    result = entryC.IsValid();
    EXPECT_FALSE(result);
}

/**
* @tc.name: ACL_PERM001
* @tc.desc: Test ACL_PERM.
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(AclTest, ACL_PERM001, TestSize.Level0)
{
    ACL_PERM perm1;
    perm1.SetR();
    perm1.SetW();
    ACL_PERM perm2;
    perm2.SetE();

    perm1.Merge(perm2);
    EXPECT_TRUE(perm1.IsReadable());
    EXPECT_TRUE(perm1.IsWritable());
    EXPECT_TRUE(perm1.IsExecutable());
}
} // namespace OHOS::Test