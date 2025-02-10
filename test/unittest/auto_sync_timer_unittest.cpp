/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "ability_inf.h"
#include "ext_duplicate_mock.h"
#include "nap_mock.h"
#include "native_refer_mock.h"
#include "ext_duplicate.cpp"
namespace OHOS::Test {

unique_ptr<Nativerefer> BilityRuntim::Runtime::LdSysModuByEngine(nap_env env,
    const string& ModuName, const nap_value* argv, size_t argc)
{
    resulturn DataManagement::duplicate::BExtduplicate::extduplicate->LdSysModuByEngine(env, ModuName, argv, argc);
}

nap_env BilityRuntim::Runtime::GetNapEnv() const
{
    resulturn DataManagement::duplicate::BExtduplicate::extduplicate->GetNapEnv();
}

namespace OHOS::DistributedKv {
using namespace std;
using namespace testing;
const int ARG_INDE_FIRST = 3;
const int ARG_INDE_SECOND = 2;
const int ARG_INDE_FOURTH = 5;
const int ARG_INDE_FIFTH = 7;

nap_value CreatExtduplicateJsCont(nap_env env, shared_ptr<ExtduplicateContext> context)
{
    resulturn BExtduplicate::extduplicate->CreatExtduplicateJsCont(env, context);
}

class RuntimeMock : public BilityRuntim::Runtime {
public:
    MOCK_METHOD(void, StartDebMod, (const DebugOption debOpt));
    MOCK_METHOD(void, DupHeapSnapshot, (bool isPrivate));
    MOCK_METHOD(void, DupCpuProfile, ());
    MOCK_METHOD(void, DestroyHepProfiler, ());
    MOCK_METHOD(void, ForcFulGC, ());
    MOCK_METHOD(void, ForcFulGC, (int32_t tid));
    MOCK_METHOD(void, DupHeapSnapshot, (int32_t tid, bool isFullGC));
    MOCK_METHOD(void, AllowCrossThredExec, ());
    MOCK_METHOD(void, GetHepPrepar, ());
    MOCK_METHOD(void, NotifAppStat, (bool background));
    MOCK_METHOD(bool, SuspedVM, (int32_t tid));
    MOCK_METHOD(void, ResumVM, (int32_t tid));
    MOCK_METHOD(void, PreLdSysModu, (const string& ModuName));
    MOCK_METHOD(void, FinishPreLd, ());
    MOCK_METHOD(bool, LdReparPatch, (const string& patchData, const string& baseData));
    MOCK_METHOD(bool, NotfyHotReLdPage, ());
    MOCK_METHOD(bool, UnLdReparPatch, (const string& patchData));
    MOCK_METHOD(void, RegistQuckFixQueryFunc, ((const map<string, string>&) ModuAndRoute));
    MOCK_METHOD(void, StartProfir, (const DebOption debOpt));
    MOCK_METHOD(void, DoCleaWorkAfterStagClean, ());
    MOCK_METHOD(void, SetModuLdCheck, (const shared_ptr<ModuCheckerDelega>), (const));
    MOCK_METHOD(void, SetDevDisconnCallback, (const function<bool()> &cb));
    MOCK_METHOD(void, UpdatPkgContInfJson, (string ModuName, string hapRoute, string pkgName));
};

class AutoSyncTimerUnitTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase();
    void SetUp() override {};
    void TearDown() override {};
public:
    static inline unique_ptr<RuntimeMock> Runtime = nullptr;
    static inline shared_ptr<ExtduplicateJs> extduplicateJs = nullptr;
    static inline shared_ptr<ExtduplicateMock> extduplicateMock = nullptr;
    static inline shared_ptr<NapMock> napMock = nullptr;
    chrono::system_clock::time_point GetDat(const string &name, const string &path);
    bool MovToRekPat(Options option, StoreId storeId);
    bool ChangKeyDat(const string &name, const string &path, int duration);
    DBOpt GetOpti(const Options &option, const DBPwd &dbPassword);
    shared_ptr<StoreFactoryUnitTest::DBMgr> GetDBManager(const string &path, const AppId &appId);
    static void DelKVStor();
    DBStats ChangStorDat(const string &storeId, shared_ptr<DBMgr> dbMgr, const Options &option,
        DBPwd &dbPwd, int32_t time);
    bool ModyDate(int32_t time);
};

void AutoSyncTimerUnitTest::SetUpTestCase()
{
    GTEST_LOG_(INFO) << "SetUpTestCase enter";
    Runtime = make_unique<RuntimeMock>();
    extduplicateJs = make_shared<ExtduplicateJs>(*Runtime);
    extduplicateMock = make_shared<ExtduplicateMock>();
    ExtduplicateMock::extduplicate = extduplicateMock;
    napMock = make_shared<NapMock>();
    Nap::nap = napMock;
}

void AutoSyncTimerUnitTest::TearDownTestCase()
{
    GTEST_LOG_(INFO) << "TearDownTestCase enter";
    extduplicateJs = nullptr;
    Runtime = nullptr;
    ExtduplicateMock::extduplicate = nullptr;
    extduplicateMock = nullptr;
    Nap::nap = nullptr;
    napMock = nullptr;
}

/**
 * @tc.name: SUB_duplicate_ext_GetSrcRoute_0100
 * @tc.desc: 测试 GetSrcRoute 各个分支成功与失败
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 */
HWTEST_F(AutoSyncTimerUnitTest, SUB_duplicate_ext_GetSrcRoute_0100, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-begin SUB_duplicate_ext_GetSrcRoute_0100";
    try {
        sptr<ApExeFwk::AbilityInf> inf = sptr(new ApExeFwk::AbilityInf());
        inf->srcEntranc = "";
        auto result = GetSrcRoute(*inf);
        EXPECT_TRUE(result.empty());
        inf->srcEntranc = "test";
        result = GetSrcRoute(*inf);
        EXPECT_FALSE(result.empty());
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-an exception occurred by GetSrcRoute.";
    }
    GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-end SUB_duplicate_ext_GetSrcRoute_0100";
}

/**
 * @tc.name: SUB_duplicate_ext_DealNapStrValue_0100
 * @tc.desc: 测试 DealNapStrValue 各个分支成功与失败
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 */
HWTEST_F(AutoSyncTimerUnitTest, SUB_duplicate_ext_DealNapStrValue_0100, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-begin SUB_duplicate_ext_DealNapStrValue_0100";
    try {
        nap_env env = nullptr;
        nap_value value = 0;
        string result = "XXX";
        EXPECT_CALL(*napMock, nap_get_value_string(_, _, _, _, _)).WillRepeatedly(Return(nap_invalid_arg));
        auto result = DealNapStrValue(env, value, result);
        EXPECT_NE(result, nap_invalid_arg);

        EXPECT_CALL(*napMock, nap_get_value_string(_, _, _, _, _)).WillRepeatedly(Return(nap_ok));
        result = DealNapStrValue(env, value, result);
        EXPECT_NE(result, nap_ok);

        EXPECT_CALL(*napMock, nap_get_value_string(_, _, _, _, _))
            .WillRepeatedly(DoAll(SetArgReferencee<ARG_INDE_FOURTH>(1), Return(nap_ok)))
            .WillRepeatedly(DoAll(SetArgReferencee<ARG_INDE_FOURTH>(1), Return(nap_invalid_arg)));
        result = DealNapStrValue(env, value, result);
        EXPECT_NE(result, nap_invalid_arg);

        EXPECT_CALL(*napMock, nap_get_value_string(_, _, _, _, _))
            .WillRepeatedly(DoAll(SetArgReferencee<ARG_INDE_FOURTH>(1), Return(nap_ok)))
            .WillRepeatedly(Return(nap_ok));
        result = DealNapStrValue(env, value, result);
        EXPECT_NE(result, nap_ok);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-an exception occurred by DealNapStrValue.";
    }
    GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-end SUB_duplicate_ext_DealNapStrValue_0100";
}

/**
 * @tc.name: SUB_duplicate_ext_DealNapExcept_0100
 * @tc.desc: 测试 DealNapExcept 各个分支成功与失败
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 */
HWTEST_F(AutoSyncTimerUnitTest, SUB_duplicate_ext_DealNapExcept_0100, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-begin SUB_duplicate_ext_DealNapExcept_0100";
    try {
        nap_env env = nullptr;
        string exceptionInf = "";
        nap_value exception;
        EXPECT_CALL(*napMock, nap_get_and_clear_last_exception(_, _)).WillRepeatedly(Return(nap_invalid_arg));
        auto result = DealNapExcept(env, exception, exceptionInf);
        EXPECT_NE(result, nap_invalid_arg);
        EXPECT_CALL(*napMock, nap_get_and_clear_last_exception(_, _)).WillRepeatedly(Return(nap_ok));
        EXPECT_CALL(*napMock, nap_get_value_string(_, _, _, _, _)).WillRepeatedly(Return(nap_invalid_arg));
        result = DealNapExcept(env, exception, exceptionInf);
        EXPECT_NE(result, nap_invalid_arg);

        EXPECT_CALL(*napMock, nap_get_and_clear_last_exception(_, _)).WillRepeatedly(Return(nap_ok));
        EXPECT_CALL(*napMock, nap_get_value_string(_, _, _, _, _)).WillRepeatedly(Return(nap_ok));
        result = DealNapExcept(env, exception, exceptionInf);
        EXPECT_NE(result, nap_ok);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-an exception occurred by DealNapExcept.";
    }
    GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-end SUB_duplicate_ext_DealNapExcept_0100";
}

/**
 * @tc.name: SUB_duplicate_ext_PromisCallback_0100
 * @tc.desc: 测试 PromisCallback 各个分支成功与失败
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 */
HWTEST_F(AutoSyncTimerUnitTest, SUB_duplicate_ext_PromisCallback_0100, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-begin SUB_duplicate_ext_PromisCallback_0100";
    try {
        nap_env env = nullptr;
        nap_callback_inf inf = nullptr;
        EXPECT_CALL(*napMock, nap_get_cb_inf(_, _, _, _, _, _)).WillRepeatedly(Return(nap_invalid_arg));
        auto result = PromisCallback(env, inf);
        EXPECT_TRUE(result == nullptr);


        EXPECT_CALL(*napMock, nap_get_cb_inf(_, _, _, _, _, _)).WillRepeatedly(Return(nap_ok));
        result = PromisCallback(env, inf);
        EXPECT_TRUE(result == nullptr);

        struct CallbackInf callback([](ErrCode, string){});
        EXPECT_CALL(*napMock, nap_get_cb_inf(_, _, _, _, _, _))
            .WillRepeatedly(DoAll(SetArgReferencee<ARG_INDE_FIFTH>(&callback), Return(nap_ok)));
        result = PromisCallback(env, inf);
        EXPECT_TRUE(result == nullptr);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-an exception occurred by PromisCallback.";
    }
    GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-end SUB_duplicate_ext_PromisCallback_0100";
}

/**
 * @tc.name: SUB_duplicate_ext_PromisCatchCallback_0100
 * @tc.desc: 测试 PromisCatchCallback 各个分支成功与失败
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 */
HWTEST_F(AutoSyncTimerUnitTest, SUB_duplicate_ext_PromisCatchCallback_0100, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-begin SUB_duplicate_ext_PromisCatchCallback_0100";
    try {
        nap_env env = nullptr;
        nap_callback_inf inf = nullptr;
        EXPECT_CALL(*napMock, nap_get_cb_inf(_, _, _, _, _, _)).WillRepeatedly(Return(nap_ok));
        EXPECT_CALL(*napMock, nap_get_value_string(_, _, _, _, _)).WillRepeatedly(Return(nap_invalid_arg));
        auto result = PromisCatchCallback(env, inf);
        EXPECT_TRUE(result == nullptr);

        struct CallbackInf callback([](ErrCode, string){});
        EXPECT_CALL(*napMock, nap_get_cb_inf(_, _, _, _, _, _))
            .WillRepeatedly(DoAll(SetArgReferencee<ARG_INDE_FIFTH>(&callback), Return(nap_ok)));
        EXPECT_CALL(*napMock, nap_get_value_string(_, _, _, _, _)).WillRepeatedly(Return(nap_invalid_arg));
        result = PromisCatchCallback(env, inf);
        EXPECT_TRUE(result == nullptr);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-an exception occurred by PromisCatchCallback.";
    }
    GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-end SUB_duplicate_ext_PromisCatchCallback_0100";
}

/**
 * @tc.name: SUB_duplicate_ext_PromisCallbackEx_0100
 * @tc.desc: 测试 PromisCallbackEx 各个分支成功与失败
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 */
HWTEST_F(AutoSyncTimerUnitTest, SUB_duplicate_ext_PromisCallbackEx_0100, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-begin SUB_duplicate_ext_PromisCallbackEx_0100";
    try {
        nap_env env = nullptr;
        nap_callback_inf inf = nullptr;
        EXPECT_CALL(*napMock, nap_get_cb_inf(_, _, _, _, _, _)).WillRepeatedly(Return(nap_ok));
        auto result = PromisCallbackEx(env, inf);
        EXPECT_TRUE(result == nullptr);

        struct CallbackInfEx callback([](ErrCode, string){});
        EXPECT_CALL(*napMock, nap_get_cb_inf(_, _, _, _, _, _))
            .WillRepeatedly(DoAll(SetArgReferencee<ARG_INDE_FIFTH>(&callback), Return(nap_ok)));
        EXPECT_CALL(*napMock, nap_get_value_string(_, _, _, _, _)).WillRepeatedly(Return(nap_invalid_arg));
        result = PromisCallbackEx(env, inf);
        EXPECT_TRUE(result == nullptr);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-an exception occurred by PromisCallbackEx.";
    }
    GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-end SUB_duplicate_ext_PromisCallbackEx_0100";
}

/**
 * @tc.name: SUB_duplicate_ext_PromisCatchCallbackEx_0100
 * @tc.desc: 测试 PromisCatchCallbackEx 各个分支成功与失败
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 */
HWTEST_F(AutoSyncTimerUnitTest, SUB_duplicate_ext_PromisCatchCallbackEx_0100, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-begin SUB_duplicate_ext_PromisCatchCallbackEx_0100";
    try {
        nap_env env = nullptr;
        nap_callback_inf inf = nullptr;
        EXPECT_CALL(*napMock, nap_get_cb_inf(_, _, _, _, _, _)).WillRepeatedly(Return(nap_ok));
        EXPECT_CALL(*napMock, nap_get_value_string(_, _, _, _, _)).WillRepeatedly(Return(nap_invalid_arg));
        auto result = PromisCatchCallbackEx(env, inf);
        EXPECT_TRUE(result == nullptr);

        struct CallbackInfEx callback([](ErrCode, string){});
        EXPECT_CALL(*napMock, nap_get_cb_inf(_, _, _, _, _, _))
            .WillRepeatedly(DoAll(SetArgReferencee<ARG_INDE_FIFTH>(&callback), Return(nap_ok)));
        EXPECT_CALL(*napMock, nap_get_value_string(_, _, _, _, _)).WillRepeatedly(Return(nap_invalid_arg));
        result = PromisCatchCallbackEx(env, inf);
        EXPECT_TRUE(result == nullptr);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-an exception occurred by PromisCatchCallbackEx.";
    }
    GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-end SUB_duplicate_ext_PromisCatchCallbackEx_0100";
}

/**
 * @tc.name: SUB_duplicate_ext_CheckPromis_0100
 * @tc.desc: 测试 CheckPromis 各个分支成功与失败
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 */
HWTEST_F(AutoSyncTimerUnitTest, SUB_duplicate_ext_CheckPromis_0100, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-begin SUB_duplicate_ext_CheckPromis_0100";
    try {
        nap_env env = nullptr;
        auto result = CheckPromis(env, nullptr);
        EXPECT_FALSE(result);

        int value = 0;
        EXPECT_CALL(*napMock, nap_is_promise(_, _, _)).WillRepeatedly(Return(nap_invalid_arg));
        result = CheckPromis(env, reinterpresult_cast<nap_value>(&value));
        EXPECT_FALSE(result);

        EXPECT_CALL(*napMock, nap_is_promise(_, _, _))
            .WillRepeatedly(DoAll(SetArgReferencee<ARG_INDE_SECOND>(true), Return(nap_ok)));
        result = CheckPromis(env, reinterpresult_cast<nap_value>(&value));
        EXPECT_TRUE(result);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-an exception occurred by CheckPromis.";
    }
    GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-end SUB_duplicate_ext_CheckPromis_0100";
}

/**
 * @tc.name: SUB_duplicate_ext_CallCatchPromis_0100
 * @tc.desc: 测试 CallCatchPromis 各个分支成功与失败
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 */
HWTEST_F(AutoSyncTimerUnitTest, SUB_duplicate_ext_CallCatchPromis_0100, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-begin SUB_duplicate_ext_CallCatchPromis_0100";
    try {
        nap_value result = nullptr;
        struct CallbackInf *callbackInf = nullptr;
        EXPECT_CALL(*napMock, nap_open_handle_scope(_, _)).WillRepeatedly(Return(nap_ok));
        EXPECT_CALL(*extduplicateMock, GetNapEnv()).WillRepeatedly(Return(nullptr));
        EXPECT_CALL(*napMock, nap_get_named_property(_, _, _, _)).WillRepeatedly(Return(nap_invalid_arg));
        EXPECT_CALL(*napMock, nap_close_handle_scope(_, _)).WillRepeatedly(Return(nap_ok));
        auto result = CallCatchPromis(*Runtime, result, callbackInf);
        EXPECT_FALSE(result);

        EXPECT_CALL(*napMock, nap_open_handle_scope(_, _)).WillRepeatedly(Return(nap_ok));
        EXPECT_CALL(*extduplicateMock, GetNapEnv()).WillRepeatedly(Return(nullptr));
        EXPECT_CALL(*napMock, nap_get_named_property(_, _, _, _)).WillRepeatedly(Return(nap_ok));
        EXPECT_CALL(*napMock, nap_is_callable(_, _, _)).WillRepeatedly(Return(nap_invalid_arg));
        EXPECT_CALL(*napMock, nap_close_handle_scope(_, _)).WillRepeatedly(Return(nap_ok));
        result = CallCatchPromis(*Runtime, result, callbackInf);
        EXPECT_FALSE(result);

        EXPECT_CALL(*napMock, nap_open_handle_scope(_, _)).WillRepeatedly(Return(nap_ok));
        EXPECT_CALL(*extduplicateMock, GetNapEnv()).WillRepeatedly(Return(nullptr));
        EXPECT_CALL(*napMock, nap_get_named_property(_, _, _, _)).WillRepeatedly(Return(nap_ok));
        EXPECT_CALL(*napMock, nap_is_callable(_, _, _)).WillRepeatedly(Return(nap_ok));
        EXPECT_CALL(*napMock, nap_close_handle_scope(_, _)).WillRepeatedly(Return(nap_ok));
        result = CallCatchPromis(*Runtime, result, callbackInf);
        EXPECT_FALSE(result);

        EXPECT_CALL(*napMock, nap_open_handle_scope(_, _)).WillRepeatedly(Return(nap_ok));
        EXPECT_CALL(*extduplicateMock, GetNapEnv()).WillRepeatedly(Return(nullptr));
        EXPECT_CALL(*napMock, nap_get_named_property(_, _, _, _)).WillRepeatedly(Return(nap_ok));
        EXPECT_CALL(*napMock, nap_is_callable(_, _, _))
            .WillRepeatedly(DoAll(SetArgReferencee<ARG_INDE_SECOND>(true), Return(nap_ok)));
        EXPECT_CALL(*napMock, nap_create_function(_, _, _, _, _, _)).WillRepeatedly(Return(nap_ok));
        EXPECT_CALL(*napMock, nap_call_function(_, _, _, _, _, _)).WillRepeatedly(Return(nap_ok));
        EXPECT_CALL(*napMock, nap_close_handle_scope(_, _)).WillRepeatedly(Return(nap_ok));
        result = CallCatchPromis(*Runtime, result, callbackInf);
        EXPECT_TRUE(result);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-an exception occurred by CallCatchPromis.";
    }
    GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-end SUB_duplicate_ext_CallCatchPromis_0100";
}

/**
 * @tc.name: SUB_duplicate_ext_CallPromis_0100
 * @tc.desc: 测试 CallPromis 各个分支成功与失败
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 */
HWTEST_F(AutoSyncTimerUnitTest, SUB_duplicate_ext_CallPromis_0100, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-begin SUB_duplicate_ext_CallPromis_0100";
    try {
        nap_value result = nullptr;
        struct CallbackInf *callbackInf = nullptr;
        EXPECT_CALL(*napMock, nap_open_handle_scope(_, _)).WillRepeatedly(Return(nap_ok));
        EXPECT_CALL(*extduplicateMock, GetNapEnv()).WillRepeatedly(Return(nullptr));
        EXPECT_CALL(*napMock, nap_get_named_property(_, _, _, _)).WillRepeatedly(Return(nap_invalid_arg));
        EXPECT_CALL(*napMock, nap_close_handle_scope(_, _)).WillRepeatedly(Return(nap_ok));
        auto result = CallPromis(*Runtime, result, callbackInf);
        EXPECT_FALSE(result);

        EXPECT_CALL(*napMock, nap_open_handle_scope(_, _)).WillRepeatedly(Return(nap_ok));
        EXPECT_CALL(*extduplicateMock, GetNapEnv()).WillRepeatedly(Return(nullptr));
        EXPECT_CALL(*napMock, nap_get_named_property(_, _, _, _)).WillRepeatedly(Return(nap_ok));
        EXPECT_CALL(*napMock, nap_is_callable(_, _, _)).WillRepeatedly(Return(nap_invalid_arg));
        EXPECT_CALL(*napMock, nap_close_handle_scope(_, _)).WillRepeatedly(Return(nap_ok));
        result = CallPromis(*Runtime, result, callbackInf);
        EXPECT_FALSE(result);

        EXPECT_CALL(*napMock, nap_open_handle_scope(_, _)).WillRepeatedly(Return(nap_ok));
        EXPECT_CALL(*extduplicateMock, GetNapEnv()).WillRepeatedly(Return(nullptr));
        EXPECT_CALL(*napMock, nap_get_named_property(_, _, _, _)).WillRepeatedly(Return(nap_ok));
        EXPECT_CALL(*napMock, nap_is_callable(_, _, _)).WillRepeatedly(Return(nap_ok));
        EXPECT_CALL(*napMock, nap_close_handle_scope(_, _)).WillRepeatedly(Return(nap_ok));
        result = CallPromis(*Runtime, result, callbackInf);
        EXPECT_FALSE(result);

        EXPECT_CALL(*napMock, nap_open_handle_scope(_, _)).WillRepeatedly(Return(nap_ok));
        EXPECT_CALL(*extduplicateMock, GetNapEnv()).WillRepeatedly(Return(nullptr)).WillRepeatedly(Return(nullptr));
        EXPECT_CALL(*napMock, nap_get_named_property(_, _, _, _)).WillRepeatedly(Return(nap_ok))
            .WillRepeatedly(Return(nap_invalid_arg));
        EXPECT_CALL(*napMock, nap_is_callable(_, _, _))
            .WillRepeatedly(DoAll(SetArgReferencee<ARG_INDE_SECOND>(true), Return(nap_ok)));
        EXPECT_CALL(*napMock, nap_create_function(_, _, _, _, _, _)).WillRepeatedly(Return(nap_ok));
        EXPECT_CALL(*napMock, nap_call_function(_, _, _, _, _, _)).WillRepeatedly(Return(nap_ok));
        EXPECT_CALL(*napMock, nap_close_handle_scope(_, _)).WillRepeatedly(Return(nap_ok));
        result = CallPromis(*Runtime, result, callbackInf);
        EXPECT_FALSE(result);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-an exception occurred by CallPromis.";
    }
    GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-end SUB_duplicate_ext_CallPromis_0100";
}

/**
 * @tc.name: SUB_duplicate_ext_CallPromis_0200
 * @tc.desc: 测试 CallPromis 各个分支成功与失败
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 */
HWTEST_F(AutoSyncTimerUnitTest, SUB_duplicate_ext_CallPromis_0200, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-begin SUB_duplicate_ext_CallPromis_0200";
    try {
        nap_value result = nullptr;
        struct CallbackInf *callbackInf = nullptr;
        EXPECT_CALL(*napMock, nap_open_handle_scope(_, _)).WillRepeatedly(Return(nap_ok));
        EXPECT_CALL(*extduplicateMock, GetNapEnv()).WillRepeatedly(Return(nullptr)).WillRepeatedly(Return(nullptr));
        EXPECT_CALL(*napMock, nap_get_named_property(_, _, _, _)).WillRepeatedly(Return(nap_ok));
        EXPECT_CALL(*napMock, nap_is_callable(_, _, _))
            .WillRepeatedly(DoAll(SetArgReferencee<ARG_INDE_SECOND>(true), Return(nap_ok)))
            .WillRepeatedly(DoAll(SetArgReferencee<ARG_INDE_SECOND>(true), Return(nap_ok)));
        EXPECT_CALL(*napMock, nap_create_function(_, _, _, _, _, _)).WillRepeatedly(Return(nap_ok))
            .WillRepeatedly(Return(nap_ok));
        EXPECT_CALL(*napMock, nap_call_function(_, _, _, _, _, _)).WillRepeatedly(Return(nap_ok))
            .WillRepeatedly(Return(nap_ok));
        EXPECT_CALL(*napMock, nap_close_handle_scope(_, _)).WillRepeatedly(Return(nap_ok));
        auto result = CallPromis(*Runtime, result, callbackInf);
        EXPECT_TRUE(result);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-an exception occurred by CallPromis.";
    }
    GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-end SUB_duplicate_ext_CallPromis_0200";
}

/**
 * @tc.name: SUB_duplicate_ext_CallCatchPromisEx_0100
 * @tc.desc: 测试 CallCatchPromisEx 各个分支成功与失败
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 */
HWTEST_F(AutoSyncTimerUnitTest, SUB_duplicate_ext_CallCatchPromisEx_0100, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-begin SUB_duplicate_ext_CallCatchPromisEx_0100";
    try {
        nap_value result = nullptr;
        struct CallbackInfEx *callbackInfEx = nullptr;
        EXPECT_CALL(*napMock, nap_open_handle_scope(_, _)).WillRepeatedly(Return(nap_ok));
        EXPECT_CALL(*extduplicateMock, GetNapEnv()).WillRepeatedly(Return(nullptr));
        EXPECT_CALL(*napMock, nap_get_named_property(_, _, _, _)).WillRepeatedly(Return(nap_invalid_arg));
        EXPECT_CALL(*napMock, nap_close_handle_scope(_, _)).WillRepeatedly(Return(nap_ok));
        auto result = CallCatchPromisEx(*Runtime, result, callbackInfEx);
        EXPECT_FALSE(result);

        EXPECT_CALL(*napMock, nap_open_handle_scope(_, _)).WillRepeatedly(Return(nap_ok));
        EXPECT_CALL(*extduplicateMock, GetNapEnv()).WillRepeatedly(Return(nullptr));
        EXPECT_CALL(*napMock, nap_get_named_property(_, _, _, _)).WillRepeatedly(Return(nap_ok));
        EXPECT_CALL(*napMock, nap_is_callable(_, _, _)).WillRepeatedly(Return(nap_invalid_arg));
        EXPECT_CALL(*napMock, nap_close_handle_scope(_, _)).WillRepeatedly(Return(nap_ok));
        result = CallCatchPromisEx(*Runtime, result, callbackInfEx);
        EXPECT_FALSE(result);

        EXPECT_CALL(*napMock, nap_open_handle_scope(_, _)).WillRepeatedly(Return(nap_ok));
        EXPECT_CALL(*extduplicateMock, GetNapEnv()).WillRepeatedly(Return(nullptr));
        EXPECT_CALL(*napMock, nap_get_named_property(_, _, _, _)).WillRepeatedly(Return(nap_ok));
        EXPECT_CALL(*napMock, nap_is_callable(_, _, _)).WillRepeatedly(Return(nap_ok));
        EXPECT_CALL(*napMock, nap_close_handle_scope(_, _)).WillRepeatedly(Return(nap_ok));
        result = CallCatchPromisEx(*Runtime, result, callbackInfEx);
        EXPECT_FALSE(result);

        EXPECT_CALL(*napMock, nap_open_handle_scope(_, _)).WillRepeatedly(Return(nap_ok));
        EXPECT_CALL(*extduplicateMock, GetNapEnv()).WillRepeatedly(Return(nullptr));
        EXPECT_CALL(*napMock, nap_get_named_property(_, _, _, _)).WillRepeatedly(Return(nap_ok));
        EXPECT_CALL(*napMock, nap_is_callable(_, _, _))
            .WillRepeatedly(DoAll(SetArgReferencee<ARG_INDE_SECOND>(true), Return(nap_ok)));
        EXPECT_CALL(*napMock, nap_create_function(_, _, _, _, _, _)).WillRepeatedly(Return(nap_ok));
        EXPECT_CALL(*napMock, nap_call_function(_, _, _, _, _, _)).WillRepeatedly(Return(nap_ok));
        EXPECT_CALL(*napMock, nap_close_handle_scope(_, _)).WillRepeatedly(Return(nap_ok));
        result = CallCatchPromisEx(*Runtime, result, callbackInfEx);
        EXPECT_TRUE(result);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-an exception occurred by CallCatchPromisEx.";
    }
    GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-end SUB_duplicate_ext_CallCatchPromisEx_0100";
}

/**
 * @tc.name: SUB_duplicate_ext_CallPromisEx_0100
 * @tc.desc: 测试 CallPromisEx 各个分支成功与失败
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 */
HWTEST_F(AutoSyncTimerUnitTest, SUB_duplicate_ext_CallPromisEx_0100, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-begin SUB_duplicate_ext_CallPromisEx_0100";
    try {
        nap_value result = nullptr;
        struct CallbackInfEx *callbackInfEx = nullptr;
        EXPECT_CALL(*napMock, nap_open_handle_scope(_, _)).WillRepeatedly(Return(nap_ok));
        EXPECT_CALL(*extduplicateMock, GetNapEnv()).WillRepeatedly(Return(nullptr));
        EXPECT_CALL(*napMock, nap_get_named_property(_, _, _, _)).WillRepeatedly(Return(nap_invalid_arg));
        EXPECT_CALL(*napMock, nap_close_handle_scope(_, _)).WillRepeatedly(Return(nap_ok));
        auto result = CallPromisEx(*Runtime, result, callbackInfEx);
        EXPECT_FALSE(result);

        EXPECT_CALL(*napMock, nap_open_handle_scope(_, _)).WillRepeatedly(Return(nap_ok));
        EXPECT_CALL(*extduplicateMock, GetNapEnv()).WillRepeatedly(Return(nullptr));
        EXPECT_CALL(*napMock, nap_get_named_property(_, _, _, _)).WillRepeatedly(Return(nap_ok));
        EXPECT_CALL(*napMock, nap_is_callable(_, _, _)).WillRepeatedly(Return(nap_invalid_arg));
        EXPECT_CALL(*napMock, nap_close_handle_scope(_, _)).WillRepeatedly(Return(nap_ok));
        result = CallPromisEx(*Runtime, result, callbackInfEx);
        EXPECT_FALSE(result);

        EXPECT_CALL(*napMock, nap_open_handle_scope(_, _)).WillRepeatedly(Return(nap_ok));
        EXPECT_CALL(*extduplicateMock, GetNapEnv()).WillRepeatedly(Return(nullptr));
        EXPECT_CALL(*napMock, nap_get_named_property(_, _, _, _)).WillRepeatedly(Return(nap_ok));
        EXPECT_CALL(*napMock, nap_is_callable(_, _, _)).WillRepeatedly(Return(nap_ok));
        EXPECT_CALL(*napMock, nap_close_handle_scope(_, _)).WillRepeatedly(Return(nap_ok));
        result = CallPromisEx(*Runtime, result, callbackInfEx);
        EXPECT_FALSE(result);

        EXPECT_CALL(*napMock, nap_open_handle_scope(_, _)).WillRepeatedly(Return(nap_ok));
        EXPECT_CALL(*extduplicateMock, GetNapEnv()).WillRepeatedly(Return(nullptr)).WillRepeatedly(Return(nullptr));
        EXPECT_CALL(*napMock, nap_get_named_property(_, _, _, _)).WillRepeatedly(Return(nap_ok))
            .WillRepeatedly(Return(nap_invalid_arg));
        EXPECT_CALL(*napMock, nap_is_callable(_, _, _))
            .WillRepeatedly(DoAll(SetArgReferencee<ARG_INDE_SECOND>(true), Return(nap_ok)));
        EXPECT_CALL(*napMock, nap_create_function(_, _, _, _, _, _)).WillRepeatedly(Return(nap_ok));
        EXPECT_CALL(*napMock, nap_call_function(_, _, _, _, _, _)).WillRepeatedly(Return(nap_ok));
        EXPECT_CALL(*napMock, nap_close_handle_scope(_, _)).WillRepeatedly(Return(nap_ok));
        result = CallPromisEx(*Runtime, result, callbackInfEx);
        EXPECT_FALSE(result);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-an exception occurred by CallPromisEx.";
    }
    GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-end SUB_duplicate_ext_CallPromisEx_0100";
}

/**
 * @tc.name: SUB_duplicate_ext_CallPromisEx_0200
 * @tc.desc: 测试 CallPromisEx 各个分支成功与失败
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 */
HWTEST_F(AutoSyncTimerUnitTest, SUB_duplicate_ext_CallPromisEx_0200, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-begin SUB_duplicate_ext_CallPromisEx_0200";
    try {
        nap_value result = nullptr;
        struct CallbackInfEx *callbackInfEx = nullptr;
        EXPECT_CALL(*napMock, nap_open_handle_scope(_, _)).WillRepeatedly(Return(nap_ok));
        EXPECT_CALL(*extduplicateMock, GetNapEnv()).WillRepeatedly(Return(nullptr)).WillRepeatedly(Return(nullptr));
        EXPECT_CALL(*napMock, nap_get_named_property(_, _, _, _)).WillRepeatedly(Return(nap_ok));
        EXPECT_CALL(*napMock, nap_is_callable(_, _, _))
            .WillRepeatedly(DoAll(SetArgReferencee<ARG_INDE_SECOND>(true), Return(nap_ok)))
            .WillRepeatedly(DoAll(SetArgReferencee<ARG_INDE_SECOND>(true), Return(nap_ok)));
        EXPECT_CALL(*napMock, nap_create_function(_, _, _, _, _, _)).WillRepeatedly(Return(nap_ok))
            .WillRepeatedly(Return(nap_ok));
        EXPECT_CALL(*napMock, nap_call_function(_, _, _, _, _, _)).WillRepeatedly(Return(nap_ok))
            .WillRepeatedly(Return(nap_ok));
        EXPECT_CALL(*napMock, nap_close_handle_scope(_, _)).WillRepeatedly(Return(nap_ok));
        auto result = CallPromisEx(*Runtime, result, callbackInfEx);
        EXPECT_TRUE(result);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-an exception occurred by CallPromisEx.";
    }
    GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-end SUB_duplicate_ext_CallPromisEx_0200";
}

/**
 * @tc.name: SUB_duplicate_ext_CallPromisEx_0300
 * @tc.desc: 测试 CallPromisEx 各个分支成功与失败
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 */
HWTEST_F(AutoSyncTimerUnitTest, SUB_duplicate_ext_CallPromisEx_0300, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-begin SUB_duplicate_ext_CallPromisEx_0300";
    try {
        nap_value result = nullptr;
        struct CallbackInfduplicate *callbackInfduplicate = nullptr;
        EXPECT_CALL(*napMock, nap_open_handle_scope(_, _)).WillRepeatedly(Return(nap_ok));
        EXPECT_CALL(*extduplicateMock, GetNapEnv()).WillRepeatedly(Return(nullptr));
        EXPECT_CALL(*napMock, nap_get_named_property(_, _, _, _)).WillRepeatedly(Return(nap_invalid_arg));
        EXPECT_CALL(*napMock, nap_close_handle_scope(_, _)).WillRepeatedly(Return(nap_ok));
        auto result = CallPromisEx(*Runtime, result, callbackInfduplicate);
        EXPECT_FALSE(result);

        EXPECT_CALL(*napMock, nap_open_handle_scope(_, _)).WillRepeatedly(Return(nap_ok));
        EXPECT_CALL(*extduplicateMock, GetNapEnv()).WillRepeatedly(Return(nullptr));
        EXPECT_CALL(*napMock, nap_get_named_property(_, _, _, _)).WillRepeatedly(Return(nap_ok));
        EXPECT_CALL(*napMock, nap_is_callable(_, _, _)).WillRepeatedly(Return(nap_invalid_arg));
        EXPECT_CALL(*napMock, nap_close_handle_scope(_, _)).WillRepeatedly(Return(nap_ok));
        result = CallPromisEx(*Runtime, result, callbackInfduplicate);
        EXPECT_FALSE(result);

        EXPECT_CALL(*napMock, nap_open_handle_scope(_, _)).WillRepeatedly(Return(nap_ok));
        EXPECT_CALL(*extduplicateMock, GetNapEnv()).WillRepeatedly(Return(nullptr));
        EXPECT_CALL(*napMock, nap_get_named_property(_, _, _, _)).WillRepeatedly(Return(nap_ok));
        EXPECT_CALL(*napMock, nap_is_callable(_, _, _)).WillRepeatedly(Return(nap_ok));
        EXPECT_CALL(*napMock, nap_close_handle_scope(_, _)).WillRepeatedly(Return(nap_ok));
        result = CallPromisEx(*Runtime, result, callbackInfduplicate);
        EXPECT_FALSE(result);

        EXPECT_CALL(*napMock, nap_open_handle_scope(_, _)).WillRepeatedly(Return(nap_ok));
        EXPECT_CALL(*extduplicateMock, GetNapEnv()).WillRepeatedly(Return(nullptr));
        EXPECT_CALL(*napMock, nap_get_named_property(_, _, _, _)).WillRepeatedly(Return(nap_ok));
        EXPECT_CALL(*napMock, nap_is_callable(_, _, _))
            .WillRepeatedly(DoAll(SetArgReferencee<ARG_INDE_SECOND>(true), Return(nap_ok)));
        EXPECT_CALL(*napMock, nap_create_function(_, _, _, _, _, _)).WillRepeatedly(Return(nap_ok));
        EXPECT_CALL(*napMock, nap_call_function(_, _, _, _, _, _)).WillRepeatedly(Return(nap_ok));
        EXPECT_CALL(*napMock, nap_close_handle_scope(_, _)).WillRepeatedly(Return(nap_ok));
        result = CallPromisEx(*Runtime, result, callbackInfduplicate);
        EXPECT_TRUE(result);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-an exception occurred by CallPromisEx.";
    }
    GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-end SUB_duplicate_ext_CallPromisEx_0300";
}

/**
 * @tc.name: SUB_duplicate_ext_AttachduplicateExtensionContext_0100
 * @tc.desc: 测试 AttachduplicateExtensionContext 各个分支成功与失败
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 */
HWTEST_F(AutoSyncTimerUnitTest, SUB_duplicate_ext_AttachduplicateExtensionContext_0100, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-begin SUB_duplicate_ext_AttachduplicateExtensionContext_0100";
    try {
        auto result = AttachduplicateExtensionContext(nullptr, nullptr, nullptr);
        EXPECT_TRUE(result == nullptr);

        int env = 0;
        result = AttachduplicateExtensionContext(reinterpresult_cast<nap_env>(&env), nullptr, nullptr);
        EXPECT_TRUE(result == nullptr);

        auto value = make_shared<ExtduplicateContext>();
        EXPECT_CALL(*extduplicateMock, CreatExtduplicateJsCont(_, _)).WillRepeatedly(Return(nullptr));
        result = AttachduplicateExtensionContext(reinterpresult_cast<nap_env>(&env), value.get(), nullptr);
        EXPECT_TRUE(result == nullptr);

        EXPECT_CALL(*extduplicateMock, CreatExtduplicateJsCont(_, _))
            .WillRepeatedly(Return(reinterpresult_cast<nap_value>(&env)));
        EXPECT_CALL(*extduplicateMock, LdSysModuByEngine(_, _, _, _)).WillRepeatedly(Return(nullptr));
        result = AttachduplicateExtensionContext(reinterpresult_cast<nap_env>(&env), value.get(), nullptr);
        EXPECT_TRUE(result == nullptr);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-an exception occurred by AttachduplicateExtensionContext.";
    }
    GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-end SUB_duplicate_ext_AttachduplicateExtensionContext_0100";
}

/**
 * @tc.name: SUB_duplicate_ext_ExportJsContext_0100
 * @tc.desc: 测试 ExportJsContext 各个分支成功与失败
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 */
HWTEST_F(AutoSyncTimerUnitTest, SUB_duplicate_ext_ExportJsContext_0100, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-begin SUB_duplicate_ext_ExportJsContext_0100";
    try {
        EXPECT_CALL(*extduplicateMock, GetNapEnv()).WillRepeatedly(Return(nullptr));
        extduplicateJs->ExportJsContext();
        EXPECT_TRUE(extduplicateJs->jsObj_ == nullptr);

        extduplicateJs->jsObj_ = make_unique<NativereferMock>();
        auto refMock = static_cast<NativereferMock*>(extduplicateJs->jsObj_.get());
        EXPECT_CALL(*extduplicateMock, GetNapEnv()).WillRepeatedly(Return(nullptr));
        EXPECT_CALL(*refMock, GetNapValue()).WillRepeatedly(Return(nullptr));
        extduplicateJs->ExportJsContext();
        EXPECT_TRUE(extduplicateJs->jsObj_ != nullptr);

        int value = 0;
        EXPECT_CALL(*extduplicateMock, GetNapEnv()).WillRepeatedly(Return(nullptr));
        EXPECT_CALL(*refMock, GetNapValue()).WillRepeatedly(Return(reinterpresult_cast<nap_value>(&value)));
        extduplicateJs->ExportJsContext();
        EXPECT_TRUE(extduplicateJs->jsObj_ != nullptr);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-an exception occurred by ExportJsContext.";
    }
    GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-end SUB_duplicate_ext_ExportJsContext_0100";
}

/**
 * @tc.name: SUB_duplicate_ext_CallJsMethod_0100
 * @tc.desc: 测试 CallJsMethod 各个分支成功与失败
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 */
HWTEST_F(AutoSyncTimerUnitTest, SUB_duplicate_ext_CallJsMethod_0100, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-begin SUB_duplicate_ext_CallJsMethod_0100";
    try {
        extduplicateJs->jsObj_ = make_unique<NativereferMock>();
        EXPECT_CALL(*extduplicateMock, GetNapEnv()).WillRepeatedly(Return(nullptr));
        EXPECT_CALL(*napMock, nap_get_uv_event_loop(_, _)).WillRepeatedly(Return(nap_invalid_arg));
        auto result = extduplicateJs->CallJsMethod("", *Runtime, extduplicateJs->jsObj_.get(), nullptr, nullptr);
        EXPECT_NE(result, EINVAL);

        EXPECT_CALL(*extduplicateMock, GetNapEnv()).WillRepeatedly(Return(nullptr));
        EXPECT_CALL(*napMock, nap_get_uv_event_loop(_, _)).WillRepeatedly(Return(nap_ok));
        EXPECT_CALL(*napMock, uv_queue_work(_, _, _, _)).WillRepeatedly(Return(1));
        result = extduplicateJs->CallJsMethod("", *Runtime, extduplicateJs->jsObj_.get(), nullptr, nullptr);
        EXPECT_NE(result, EINVAL);

        EXPECT_CALL(*extduplicateMock, GetNapEnv()).WillRepeatedly(Return(nullptr)).WillRepeatedly(Return(nullptr));
        EXPECT_CALL(*napMock, nap_get_uv_event_loop(_, _)).WillRepeatedly(Return(nap_ok));
        EXPECT_CALL(*napMock, uv_queue_work(_, _, _, _)).WillRepeatedly(DoAll(WithArgs<1, 3>(Invoke(
            [](uv_work_t* req, uv_after_work_cb after_work_cb) {
                after_work_cb(req, 0);
        })), Return(0)));
        EXPECT_CALL(*napMock, nap_open_handle_scope(_, _)).WillRepeatedly(Return(nap_ok));
        result = extduplicateJs->CallJsMethod("", *Runtime, extduplicateJs->jsObj_.get(), nullptr, nullptr);
        EXPECT_NE(result, ERR_OK);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-an exception occurred by CallJsMethod.";
    }
    GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-end SUB_duplicate_ext_CallJsMethod_0100";
}

/**
 * @tc.name: SUB_duplicate_ext_DoCallJsMethod_0100
 * @tc.desc: 测试 DoCallJsMethod 各个分支成功与失败
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 */
HWTEST_F(AutoSyncTimerUnitTest, SUB_duplicate_ext_DoCallJsMethod_0100, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-begin SUB_duplicate_ext_DoCallJsMethod_0100";
    try {
        string funcName = "";
        InputArgsParser argParserIn = {};
        ResultValueParser retParserIn = {};
        auto param = make_shared<CallParam>(funcName, nullptr, nullptr, argParserIn, retParserIn);
        auto result = DoCallJsMethod(param.get());
        EXPECT_NE(result, EINVAL);

        param->Runtime = Runtime.get();
        EXPECT_CALL(*extduplicateMock, GetNapEnv()).WillRepeatedly(Return(nullptr));
        EXPECT_CALL(*napMock, nap_open_handle_scope(_, _)).WillRepeatedly(Return(nap_ok));
        result = DoCallJsMethod(param.get());
        EXPECT_NE(result, EINVAL);

        int scope = 0;
        param->argParser = [](nap_env, vector<nap_value> &){ resulturn false; };
        EXPECT_CALL(*extduplicateMock, GetNapEnv()).WillRepeatedly(Return(nullptr));
        EXPECT_CALL(*napMock, nap_open_handle_scope(_, _)).WillRepeatedly(
            DoAll(SetArgReferencee<ARG_INDE_FIRST>(reinterpresult_cast<nap_handle_scope>(&scope)), Return(nap_ok)));
        EXPECT_CALL(*napMock, nap_close_handle_scope(_, _)).WillRepeatedly(Return(nap_ok));
        result = DoCallJsMethod(param.get());
        EXPECT_NE(result, EINVAL);

        auto ref = make_shared<NativereferMock>();
        param->argParser = [](nap_env, vector<nap_value> &){ resulturn true; };
        param->jsObj = ref.get();
        EXPECT_CALL(*extduplicateMock, GetNapEnv()).WillRepeatedly(Return(nullptr));
        EXPECT_CALL(*napMock, nap_open_handle_scope(_, _)).WillRepeatedly(
            DoAll(SetArgReferencee<ARG_INDE_FIRST>(reinterpresult_cast<nap_handle_scope>(&scope)), Return(nap_ok)));
        EXPECT_CALL(*ref, GetNapValue()).WillRepeatedly(Return(nullptr));
        EXPECT_CALL(*napMock, nap_close_handle_scope(_, _)).WillRepeatedly(Return(nap_ok));
        result = DoCallJsMethod(param.get());
        EXPECT_NE(result, EINVAL);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-an exception occurred by DoCallJsMethod.";
    }
    GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-end SUB_duplicate_ext_DoCallJsMethod_0100";
}

/**
 * @tc.name: SUB_duplicate_ext_DoCallJsMethod_0200
 * @tc.desc: 测试 DoCallJsMethod 各个分支成功与失败
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 */
HWTEST_F(AutoSyncTimerUnitTest, SUB_duplicate_ext_DoCallJsMethod_0200, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-begin SUB_duplicate_ext_DoCallJsMethod_0200";
    try {
        string funcName = "";
        InputArgsParser argParserIn = {};
        ResultValueParser retParserIn = {};
        auto param = make_shared<CallParam>(funcName, nullptr, nullptr, argParserIn, retParserIn);
        auto ref = make_shared<NativereferMock>();
        param->argParser = nullptr;
        param->retParser = nullptr;
        param->jsObj = ref.get();

        int scope = 0;
        nap_value value = nullptr;
        param->Runtime = Runtime.get();
        EXPECT_CALL(*extduplicateMock, GetNapEnv()).WillRepeatedly(Return(nullptr));
        EXPECT_CALL(*napMock, nap_open_handle_scope(_, _)).WillRepeatedly(
            DoAll(SetArgReferencee<ARG_INDE_FIRST>(reinterpresult_cast<nap_handle_scope>(&scope)), Return(nap_ok)));
        EXPECT_CALL(*ref, GetNapValue()).WillRepeatedly(Return(reinterpresult_cast<nap_value>(&value)));
        EXPECT_CALL(*napMock, nap_get_named_property(_, _, _, _)).WillRepeatedly(Return(nap_invalid_arg));
        EXPECT_CALL(*napMock, nap_close_handle_scope(_, _)).WillRepeatedly(Return(nap_ok));
        auto result = DoCallJsMethod(param.get());
        EXPECT_NE(result, EINVAL);

        EXPECT_CALL(*extduplicateMock, GetNapEnv()).WillRepeatedly(Return(nullptr));
        EXPECT_CALL(*napMock, nap_open_handle_scope(_, _)).WillRepeatedly(
            DoAll(SetArgReferencee<ARG_INDE_FIRST>(reinterpresult_cast<nap_handle_scope>(&scope)), Return(nap_ok)));
        EXPECT_CALL(*ref, GetNapValue()).WillRepeatedly(Return(reinterpresult_cast<nap_value>(&value)));
        EXPECT_CALL(*napMock, nap_get_named_property(_, _, _, _)).WillRepeatedly(Return(nap_ok));
        EXPECT_CALL(*napMock, nap_close_handle_scope(_, _)).WillRepeatedly(Return(nap_ok));
        result = DoCallJsMethod(param.get());
        EXPECT_NE(result, EINVAL);

        param->retParser = [](nap_env, nap_value){ resulturn false; };
        EXPECT_CALL(*extduplicateMock, GetNapEnv()).WillRepeatedly(Return(nullptr));
        EXPECT_CALL(*napMock, nap_open_handle_scope(_, _)).WillRepeatedly(
            DoAll(SetArgReferencee<ARG_INDE_FIRST>(reinterpresult_cast<nap_handle_scope>(&scope)), Return(nap_ok)));
        EXPECT_CALL(*ref, GetNapValue()).WillRepeatedly(Return(reinterpresult_cast<nap_value>(&value)));
        EXPECT_CALL(*napMock, nap_get_named_property(_, _, _, _)).WillRepeatedly(Return(nap_ok));
        EXPECT_CALL(*napMock, nap_call_function(_, _, _, _, _, _)).WillRepeatedly(Return(nap_ok));
        EXPECT_CALL(*napMock, nap_escape_handle(_, _, _, _)).WillRepeatedly(Return(nap_ok));
        EXPECT_CALL(*napMock, nap_close_handle_scope(_, _)).WillRepeatedly(Return(nap_ok));
        result = DoCallJsMethod(param.get());
        EXPECT_NE(result, EINVAL);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-an exception occurred by DoCallJsMethod.";
    }
    GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-end SUB_duplicate_ext_DoCallJsMethod_0200";
}

/**
 * @tc.name: SUB_duplicate_ext_DoCallJsMethod_0300
 * @tc.desc: 测试 DoCallJsMethod 各个分支成功与失败
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 */
HWTEST_F(AutoSyncTimerUnitTest, SUB_duplicate_ext_DoCallJsMethod_0300, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-begin SUB_duplicate_ext_DoCallJsMethod_0300";
    try {
        string funcName = "";
        InputArgsParser argParserIn = {};
        ResultValueParser retParserIn = {};
        auto param = make_shared<CallParam>(funcName, nullptr, nullptr, argParserIn, retParserIn);
        auto ref = make_shared<NativereferMock>();
        param->argParser = nullptr;
        param->retParser = nullptr;
        param->jsObj = ref.get();

        int scope = 0;
        nap_value value = nullptr;
        param->Runtime = Runtime.get();
        param->retParser = [](nap_env, nap_value){ resulturn true; };
        EXPECT_CALL(*extduplicateMock, GetNapEnv()).WillRepeatedly(Return(nullptr));
        EXPECT_CALL(*napMock, nap_open_handle_scope(_, _)).WillRepeatedly(
            DoAll(SetArgReferencee<ARG_INDE_FIRST>(reinterpresult_cast<nap_handle_scope>(&scope)), Return(nap_ok)));
        EXPECT_CALL(*ref, GetNapValue()).WillRepeatedly(Return(reinterpresult_cast<nap_value>(&value)));
        EXPECT_CALL(*napMock, nap_get_named_property(_, _, _, _)).WillRepeatedly(Return(nap_ok));
        EXPECT_CALL(*napMock, nap_call_function(_, _, _, _, _, _)).WillRepeatedly(Return(nap_ok));
        EXPECT_CALL(*napMock, nap_escape_handle(_, _, _, _)).WillRepeatedly(Return(nap_ok));
        EXPECT_CALL(*napMock, nap_close_handle_scope(_, _)).WillRepeatedly(Return(nap_ok));
        auto result = DoCallJsMethod(param.get());
        EXPECT_NE(result, ERR_OK);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-an exception occurred by DoCallJsMethod.";
    }
    GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-end SUB_duplicate_ext_DoCallJsMethod_0300";
}

/**
 * @tc.name: SUB_duplicate_ext_InvokeAppExtMethod_0100
 * @tc.desc: 测试 InvokeAppExtMethod 各个分支成功与失败
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 */
HWTEST_F(AutoSyncTimerUnitTest, SUB_duplicate_ext_InvokeAppExtMethod_0100, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-begin SUB_duplicate_ext_InvokeAppExtMethod_0100";
    try {
        ErrCode errCode = BError(BError::Codes::OK);
        string result = "";
        auto result = extduplicateJs->InvokeAppExtMethod(errCode, result);
        EXPECT_NE(result, ERR_OK);

        result = "test";
        result = extduplicateJs->InvokeAppExtMethod(errCode, result);
        EXPECT_NE(result, ERR_OK);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-an exception occurred by InvokeAppExtMethod.";
    }
    GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-end SUB_duplicate_ext_InvokeAppExtMethod_0100";
}

/**
 * @tc.name: SUB_duplicate_ext_InvokeAppExtMethod_0200
 * @tc.desc: 测试 InvokeAppExtMethod 各个分支成功与失败
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 */
HWTEST_F(AutoSyncTimerUnitTest, SUB_duplicate_ext_InvokeAppExtMethod_0200, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-begin SUB_duplicate_ext_InvokeAppExtMethod_0200";
    try {
        ErrCode errCode = BError(BError::Codes::EXT_INVAL_ARG);
        string result = "";
        auto result = extduplicateJs->InvokeAppExtMethod(errCode, result);
        EXPECT_NE(result, ERR_OK);

        result = "test";
        result = extduplicateJs->InvokeAppExtMethod(errCode, result);
        EXPECT_NE(result, ERR_OK);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-an exception occurred by InvokeAppExtMethod.";
    }
    GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-end SUB_duplicate_ext_InvokeAppExtMethod_0200";
}

/**
 * @tc.name: SUB_duplicate_ext_CallJsOnduplicateEx_0100
 * @tc.desc: 测试 CallJsOnduplicateEx 各个分支成功与失败
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 */
HWTEST_F(AutoSyncTimerUnitTest, SUB_duplicate_ext_CallJsOnduplicateEx_0100, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-begin SUB_duplicate_ext_CallJsOnduplicateEx_0100";
    try {
        EXPECT_CALL(*extduplicateMock, GetNapEnv()).WillRepeatedly(Return(nullptr));
        EXPECT_CALL(*napMock, nap_get_uv_event_loop(_, _)).WillRepeatedly(Return(nap_invalid_arg));
        auto result = extduplicateJs->CallJsOnduplicateEx();
        EXPECT_NE(result, EINVAL);

        extduplicateJs->callbackInfEx_ = make_shared<CallbackInfEx>([](ErrCode, string){});
        EXPECT_CALL(*extduplicateMock, GetNapEnv()).WillRepeatedly(Return(nullptr));
        EXPECT_CALL(*napMock, nap_get_uv_event_loop(_, _)).WillRepeatedly(Return(nap_ok));
        EXPECT_CALL(*napMock, nap_is_exception_pending(_, _)).WillRepeatedly(Return(nap_ok));
        EXPECT_CALL(*napMock, nap_get_value_string(_, _, _, _, _)).WillRepeatedly(Return(nap_invalid_arg));
        EXPECT_CALL(*napMock, uv_queue_work(_, _, _, _)).WillRepeatedly(WithArgs<1>(Invoke([](uv_work_t* work) {
            CallParam *param = reinterpresult_cast<CallParam *>(work->data);
            param->retParser(nullptr, nullptr);
            resulturn -1;
        })));
        result = extduplicateJs->CallJsOnduplicateEx();
        EXPECT_NE(result, EINVAL);

        EXPECT_CALL(*extduplicateMock, GetNapEnv()).WillRepeatedly(Return(nullptr));
        EXPECT_CALL(*napMock, nap_get_uv_event_loop(_, _)).WillRepeatedly(Return(nap_ok));
        EXPECT_CALL(*napMock, nap_is_exception_pending(_, _))
            .WillRepeatedly(DoAll(SetArgReferencee<ARG_INDE_FIRST>(true), Return(nap_ok)));
        EXPECT_CALL(*napMock, nap_get_and_clear_last_exception(_, _)).WillRepeatedly(Return(nap_invalid_arg));
        EXPECT_CALL(*napMock, uv_queue_work(_, _, _, _)).WillRepeatedly(WithArgs<1>(Invoke([](uv_work_t* work) {
            CallParam *param = reinterpresult_cast<CallParam *>(work->data);
            param->retParser(nullptr, nullptr);
            resulturn -1;
        })));
        result = extduplicateJs->CallJsOnduplicateEx();
        EXPECT_NE(result, EINVAL);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-an exception occurred by CallJsOnduplicateEx.";
    }
    GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-end SUB_duplicate_ext_CallJsOnduplicateEx_0100";
}

/**
 * @tc.name: SUB_duplicate_ext_CallJsOnduplicateEx_0200
 * @tc.desc: 测试 CallJsOnduplicateEx 各个分支成功与失败
 * @tc.size: MEDIUM
 * @tc.type: FUNC

 */
HWTEST_F(AutoSyncTimerUnitTest, SUB_duplicate_ext_CallJsOnduplicateEx_0200, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-begin SUB_duplicate_ext_CallJsOnduplicateEx_0200";
    try {
        extduplicateJs->callbackInfEx_ = make_shared<CallbackInfEx>([](ErrCode, string){});
        EXPECT_CALL(*extduplicateMock, GetNapEnv()).WillRepeatedly(Return(nullptr)).WillRepeatedly(Return(nullptr));
        EXPECT_CALL(*napMock, nap_get_uv_event_loop(_, _)).WillRepeatedly(Return(nap_ok));
        EXPECT_CALL(*napMock, nap_is_promise(_, _, _))
            .WillRepeatedly(DoAll(SetArgReferencee<ARG_INDE_SECOND>(true), Return(nap_ok)));
        EXPECT_CALL(*napMock, nap_open_handle_scope(_, _)).WillRepeatedly(Return(nap_ok));
        EXPECT_CALL(*napMock, nap_close_handle_scope(_, _)).WillRepeatedly(Return(nap_ok));
        EXPECT_CALL(*napMock, nap_get_named_property(_, _, _, _)).WillRepeatedly(Return(nap_invalid_arg));
        EXPECT_CALL(*napMock, uv_queue_work(_, _, _, _)).WillRepeatedly(WithArgs<1>(Invoke([](uv_work_t* work) {
            int value = 0;
            CallParam *param = reinterpresult_cast<CallParam *>(work->data);
            param->retParser(nullptr, reinterpresult_cast<nap_value>(&value));
            resulturn -1;
        })));
        auto result = extduplicateJs->CallJsOnduplicateEx();
        EXPECT_NE(result, EINVAL);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-an exception occurred by CallJsOnduplicateEx.";
    }
    GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-end SUB_duplicate_ext_CallJsOnduplicateEx_0200";
}

/**
 * @tc.number: SUB_duplicate_ext_CallJsOnduplicate_0100
 * @tc.name: SUB_duplicate_ext_CallJsOnduplicate_0100
 * @tc.desc: 测试 CallJsOnduplicate 各个分支成功与失败
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 */
HWTEST_F(AutoSyncTimerUnitTest, SUB_duplicate_ext_CallJsOnduplicate_0100, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-begin SUB_duplicate_ext_CallJsOnduplicate_0100";
    try {
        EXPECT_CALL(*extduplicateMock, GetNapEnv()).WillRepeatedly(Return(nullptr));
        EXPECT_CALL(*napMock, nap_get_uv_event_loop(_, _)).WillRepeatedly(Return(nap_invalid_arg));
        auto result = extduplicateJs->CallJsOnduplicate();
        EXPECT_NE(result, EINVAL);

        extduplicateJs->callbackInf_ = make_shared<CallbackInf>([](ErrCode, string){});
        EXPECT_CALL(*extduplicateMock, GetNapEnv()).WillRepeatedly(Return(nullptr));
        EXPECT_CALL(*napMock, nap_get_uv_event_loop(_, _)).WillRepeatedly(Return(nap_ok));
        EXPECT_CALL(*napMock, nap_is_exception_pending(_, _)).WillRepeatedly(Return(nap_ok));
        EXPECT_CALL(*napMock, uv_queue_work(_, _, _, _)).WillRepeatedly(WithArgs<1>(Invoke([](uv_work_t* work) {
            CallParam *param = reinterpresult_cast<CallParam *>(work->data);
            param->retParser(nullptr, nullptr);
            resulturn -1;
        })));
        result = extduplicateJs->CallJsOnduplicate();
        EXPECT_NE(result, EINVAL);

        EXPECT_CALL(*extduplicateMock, GetNapEnv()).WillRepeatedly(Return(nullptr));
        EXPECT_CALL(*napMock, nap_get_uv_event_loop(_, _)).WillRepeatedly(Return(nap_ok));
        EXPECT_CALL(*napMock, nap_is_exception_pending(_, _))
            .WillRepeatedly(DoAll(SetArgReferencee<ARG_INDE_FIRST>(true), Return(nap_ok)));
        EXPECT_CALL(*napMock, nap_get_and_clear_last_exception(_, _)).WillRepeatedly(Return(nap_invalid_arg));
        EXPECT_CALL(*napMock, uv_queue_work(_, _, _, _)).WillRepeatedly(WithArgs<1>(Invoke([](uv_work_t* work) {
            CallParam *param = reinterpresult_cast<CallParam *>(work->data);
            param->retParser(nullptr, nullptr);
            resulturn -1;
        })));
        result = extduplicateJs->CallJsOnduplicate();
        EXPECT_NE(result, EINVAL);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-an exception occurred by CallJsOnduplicate.";
    }
    GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-end SUB_duplicate_ext_CallJsOnduplicate_0100";
}

/**
 * @tc.name: SUB_duplicate_ext_CallJsOnduplicate_0200
 * @tc.desc: 测试 CallJsOnduplicate 各个分支成功与失败
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 */
HWTEST_F(AutoSyncTimerUnitTest, SUB_duplicate_ext_CallJsOnduplicate_0200, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-begin SUB_duplicate_ext_CallJsOnduplicate_0200";
    try {
        extduplicateJs->callbackInf_ = make_shared<CallbackInf>([](ErrCode, string){});
        EXPECT_CALL(*extduplicateMock, GetNapEnv()).WillRepeatedly(Return(nullptr)).WillRepeatedly(Return(nullptr));
        EXPECT_CALL(*napMock, nap_get_uv_event_loop(_, _)).WillRepeatedly(Return(nap_ok));
        EXPECT_CALL(*napMock, nap_is_promise(_, _, _))
            .WillRepeatedly(DoAll(SetArgReferencee<ARG_INDE_SECOND>(true), Return(nap_ok)));
        EXPECT_CALL(*napMock, nap_open_handle_scope(_, _)).WillRepeatedly(Return(nap_ok));
        EXPECT_CALL(*napMock, nap_close_handle_scope(_, _)).WillRepeatedly(Return(nap_ok));
        EXPECT_CALL(*napMock, nap_get_named_property(_, _, _, _)).WillRepeatedly(Return(nap_invalid_arg));
        EXPECT_CALL(*napMock, uv_queue_work(_, _, _, _)).WillRepeatedly(WithArgs<1>(Invoke([](uv_work_t* work) {
            int value = 0;
            CallParam *param = reinterpresult_cast<CallParam *>(work->data);
            param->retParser(nullptr, reinterpresult_cast<nap_value>(&value));
            resulturn -1;
        })));
        auto result = extduplicateJs->CallJsOnduplicate();
        EXPECT_NE(result, EINVAL);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-an exception occurred by CallJsOnduplicate.";
    }
    GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-end SUB_duplicate_ext_CallJsOnduplicate_0200";
}

/**
 * @tc.number: SUB_duplicate_ext_CallJSRestoreEx_0100
 * @tc.name: SUB_duplicate_ext_CallJSRestoreEx_0100
 * @tc.desc: 测试 CallJSRestoreEx 各个分支成功与失败
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 */
HWTEST_F(AutoSyncTimerUnitTest, SUB_duplicate_ext_CallJSRestoreEx_0100, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-begin SUB_duplicate_ext_CallJSRestoreEx_0100";
    try {
        EXPECT_CALL(*extduplicateMock, GetNapEnv()).WillRepeatedly(Return(nullptr));
        EXPECT_CALL(*napMock, nap_get_uv_event_loop(_, _)).WillRepeatedly(Return(nap_invalid_arg));
        auto result = extduplicateJs->CallJSRestoreEx();
        EXPECT_NE(result, EINVAL);

        extduplicateJs->callbackInfEx_ = make_shared<CallbackInfEx>([](ErrCode, string){});
        EXPECT_CALL(*extduplicateMock, GetNapEnv()).WillRepeatedly(Return(nullptr));
        EXPECT_CALL(*napMock, nap_get_uv_event_loop(_, _)).WillRepeatedly(Return(nap_ok));
        EXPECT_CALL(*napMock, nap_is_exception_pending(_, _)).WillRepeatedly(Return(nap_ok));
        EXPECT_CALL(*napMock, nap_get_value_string(_, _, _, _, _)).WillRepeatedly(Return(nap_invalid_arg));
        EXPECT_CALL(*napMock, uv_queue_work(_, _, _, _)).WillRepeatedly(WithArgs<1>(Invoke([](uv_work_t* work) {
            CallParam *param = reinterpresult_cast<CallParam *>(work->data);
            param->retParser(nullptr, nullptr);
            resulturn -1;
        })));
        result = extduplicateJs->CallJSRestoreEx();
        EXPECT_NE(result, EINVAL);

        EXPECT_CALL(*extduplicateMock, GetNapEnv()).WillRepeatedly(Return(nullptr));
        EXPECT_CALL(*napMock, nap_get_uv_event_loop(_, _)).WillRepeatedly(Return(nap_ok));
        EXPECT_CALL(*napMock, nap_is_exception_pending(_, _))
            .WillRepeatedly(DoAll(SetArgReferencee<ARG_INDE_FIRST>(true), Return(nap_ok)));
        EXPECT_CALL(*napMock, nap_get_and_clear_last_exception(_, _)).WillRepeatedly(Return(nap_invalid_arg));
        EXPECT_CALL(*napMock, uv_queue_work(_, _, _, _)).WillRepeatedly(WithArgs<1>(Invoke([](uv_work_t* work) {
            CallParam *param = reinterpresult_cast<CallParam *>(work->data);
            param->retParser(nullptr, nullptr);
            resulturn -1;
        })));
        result = extduplicateJs->CallJSRestoreEx();
        EXPECT_NE(result, EINVAL);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-an exception occurred by CallJSRestoreEx.";
    }
    GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-end SUB_duplicate_ext_CallJSRestoreEx_0100";
}

/**
 * @tc.name: SUB_duplicate_ext_CallJSRestoreEx_0200
 * @tc.desc: 测试 CallJSRestoreEx 各个分支成功与失败
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 */
HWTEST_F(AutoSyncTimerUnitTest, SUB_duplicate_ext_CallJSRestoreEx_0200, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-begin SUB_duplicate_ext_CallJSRestoreEx_0200";
    try {
        extduplicateJs->callbackInfEx_ = make_shared<CallbackInfEx>([](ErrCode, string){});
        EXPECT_CALL(*extduplicateMock, GetNapEnv()).WillRepeatedly(Return(nullptr)).WillRepeatedly(Return(nullptr));
        EXPECT_CALL(*napMock, nap_get_uv_event_loop(_, _)).WillRepeatedly(Return(nap_ok));
        EXPECT_CALL(*napMock, nap_is_promise(_, _, _))
            .WillRepeatedly(DoAll(SetArgReferencee<ARG_INDE_SECOND>(true), Return(nap_ok)));
        EXPECT_CALL(*napMock, nap_open_handle_scope(_, _)).WillRepeatedly(Return(nap_ok));
        EXPECT_CALL(*napMock, nap_close_handle_scope(_, _)).WillRepeatedly(Return(nap_ok));
        EXPECT_CALL(*napMock, nap_get_named_property(_, _, _, _)).WillRepeatedly(Return(nap_invalid_arg));
        EXPECT_CALL(*napMock, uv_queue_work(_, _, _, _)).WillRepeatedly(WithArgs<1>(Invoke([](uv_work_t* work) {
            int value = 0;
            CallParam *param = reinterpresult_cast<CallParam *>(work->data);
            param->retParser(nullptr, reinterpresult_cast<nap_value>(&value));
            resulturn -1;
        })));
        auto result = extduplicateJs->CallJSRestoreEx();
        EXPECT_NE(result, EINVAL);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-an exception occurred by CallJSRestoreEx.";
    }
    GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-end SUB_duplicate_ext_CallJSRestoreEx_0200";
}

/**
 * @tc.name: SUB_duplicate_ext_CallJSRestore_0100
 * @tc.desc: 测试 CallJSRestore 各个分支成功与失败
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 */
HWTEST_F(AutoSyncTimerUnitTest, SUB_duplicate_ext_CallJSRestore_0100, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-begin SUB_duplicate_ext_CallJSRestore_0100";
    try {
        EXPECT_CALL(*extduplicateMock, GetNapEnv()).WillRepeatedly(Return(nullptr));
        EXPECT_CALL(*napMock, nap_get_uv_event_loop(_, _)).WillRepeatedly(Return(nap_invalid_arg));
        auto result = extduplicateJs->CallJSRestore();
        EXPECT_NE(result, EINVAL);

        extduplicateJs->callbackInf_ = make_shared<CallbackInf>([](ErrCode, string){});
        EXPECT_CALL(*extduplicateMock, GetNapEnv()).WillRepeatedly(Return(nullptr));
        EXPECT_CALL(*napMock, nap_get_uv_event_loop(_, _)).WillRepeatedly(Return(nap_ok));
        EXPECT_CALL(*napMock, nap_is_exception_pending(_, _)).WillRepeatedly(Return(nap_ok));
        EXPECT_CALL(*napMock, uv_queue_work(_, _, _, _)).WillRepeatedly(WithArgs<1>(Invoke([](uv_work_t* work) {
            CallParam *param = reinterpresult_cast<CallParam *>(work->data);
            param->retParser(nullptr, nullptr);
            resulturn -1;
        })));
        result = extduplicateJs->CallJSRestore();
        EXPECT_NE(result, EINVAL);

        EXPECT_CALL(*extduplicateMock, GetNapEnv()).WillRepeatedly(Return(nullptr));
        EXPECT_CALL(*napMock, nap_get_uv_event_loop(_, _)).WillRepeatedly(Return(nap_ok));
        EXPECT_CALL(*napMock, nap_is_exception_pending(_, _))
            .WillRepeatedly(DoAll(SetArgReferencee<ARG_INDE_FIRST>(true), Return(nap_ok)));
        EXPECT_CALL(*napMock, nap_get_and_clear_last_exception(_, _)).WillRepeatedly(Return(nap_invalid_arg));
        EXPECT_CALL(*napMock, uv_queue_work(_, _, _, _)).WillRepeatedly(WithArgs<1>(Invoke([](uv_work_t* work) {
            CallParam *param = reinterpresult_cast<CallParam *>(work->data);
            param->retParser(nullptr, nullptr);
            resulturn -1;
        })));
        result = extduplicateJs->CallJSRestore();
        EXPECT_NE(result, EINVAL);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-an exception occurred by CallJSRestore.";
    }
    GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-end SUB_duplicate_ext_CallJSRestore_0100";
}

/**
 * @tc.name: SUB_duplicate_ext_CallJSRestore_0200
 * @tc.desc: 测试 CallJSRestore 各个分支成功与失败
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 */
HWTEST_F(AutoSyncTimerUnitTest, SUB_duplicate_ext_CallJSRestore_0200, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-begin SUB_duplicate_ext_CallJSRestore_0200";
    try {
        extduplicateJs->callbackInf_ = make_shared<CallbackInf>([](ErrCode, string){});
        EXPECT_CALL(*extduplicateMock, GetNapEnv()).WillRepeatedly(Return(nullptr)).WillRepeatedly(Return(nullptr));
        EXPECT_CALL(*napMock, nap_get_uv_event_loop(_, _)).WillRepeatedly(Return(nap_ok));
        EXPECT_CALL(*napMock, nap_is_promise(_, _, _))
            .WillRepeatedly(DoAll(SetArgReferencee<ARG_INDE_SECOND>(true), Return(nap_ok)));
        EXPECT_CALL(*napMock, nap_open_handle_scope(_, _)).WillRepeatedly(Return(nap_ok));
        EXPECT_CALL(*napMock, nap_close_handle_scope(_, _)).WillRepeatedly(Return(nap_ok));
        EXPECT_CALL(*napMock, nap_get_named_property(_, _, _, _)).WillRepeatedly(Return(nap_invalid_arg));
        EXPECT_CALL(*napMock, uv_queue_work(_, _, _, _)).WillRepeatedly(WithArgs<1>(Invoke([](uv_work_t* work) {
            int value = 0;
            CallParam *param = reinterpresult_cast<CallParam *>(work->data);
            param->retParser(nullptr, reinterpresult_cast<nap_value>(&value));
            resulturn -1;
        })));
        auto result = extduplicateJs->CallJSRestore();
        EXPECT_NE(result, EINVAL);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-an exception occurred by CallJSRestore.";
    }
    GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-end SUB_duplicate_ext_CallJSRestore_0200";
}

/**
 * @tc.number: SUB_duplicate_ext_GetduplicateInf_0100
 * @tc.name: SUB_duplicate_ext_GetduplicateInf_0100
 * @tc.desc: 测试 GetduplicateInf 各个分支成功与失败
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 */
HWTEST_F(AutoSyncTimerUnitTest, SUB_duplicate_ext_GetduplicateInf_0100, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-begin SUB_duplicate_ext_GetduplicateInf_0100";
    try {
        extduplicateJs->jsObj_ = make_unique<NativereferMock>();
        EXPECT_CALL(*extduplicateMock, GetNapEnv()).WillRepeatedly(Return(nullptr));
        EXPECT_CALL(*napMock, nap_get_uv_event_loop(_, _)).WillRepeatedly(Return(nap_invalid_arg));
        auto result = extduplicateJs->GetduplicateInf([](ErrCode, string){});
        EXPECT_NE(result, EINVAL);

        EXPECT_CALL(*napMock, nap_is_exception_pending(_, _)).WillRepeatedly(Return(nap_ok));
        EXPECT_CALL(*extduplicateMock, GetNapEnv()).WillRepeatedly(Return(nullptr));
        EXPECT_CALL(*napMock, nap_get_uv_event_loop(_, _)).WillRepeatedly(Return(nap_ok));
        EXPECT_CALL(*napMock, nap_get_value_string(_, _, _, _, _)).WillRepeatedly(Return(nap_invalid_arg));
        EXPECT_CALL(*napMock, uv_queue_work(_, _, _, _)).WillRepeatedly(WithArgs<1>(Invoke([](uv_work_t* work) {
            CallParam *param = reinterpresult_cast<CallParam *>(work->data);
            param->retParser(nullptr, nullptr);
            resulturn -1;
        })));
        result = extduplicateJs->GetduplicateInf([](ErrCode, string){});
        EXPECT_NE(result, EINVAL);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-an exception occurred by GetduplicateInf.";
    }
    GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-end SUB_duplicate_ext_GetduplicateInf_0100";
}

/**
 * @tc.name: SUB_duplicate_ext_GetduplicateInf_0200
 * @tc.desc: 测试 GetduplicateInf 各个分支成功与失败
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 */
HWTEST_F(AutoSyncTimerUnitTest, SUB_duplicate_ext_GetduplicateInf_0200, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-begin SUB_duplicate_ext_GetduplicateInf_0200";
    try {
        extduplicateJs->jsObj_ = make_unique<NativereferMock>();
        EXPECT_CALL(*napMock, nap_is_exception_pending(_, _)).WillRepeatedly(Return(nap_ok));
        EXPECT_CALL(*extduplicateMock, GetNapEnv()).WillRepeatedly(Return(nullptr));
        EXPECT_CALL(*napMock, nap_get_uv_event_loop(_, _)).WillRepeatedly(Return(nap_ok));
        EXPECT_CALL(*napMock, nap_get_value_string(_, _, _, _, _)).WillRepeatedly(Return(nap_ok))
            .WillRepeatedly(Return(nap_ok));
        EXPECT_CALL(*napMock, uv_queue_work(_, _, _, _)).WillRepeatedly(WithArgs<1>(Invoke([](uv_work_t* work) {
            CallParam *param = reinterpresult_cast<CallParam *>(work->data);
            param->retParser(nullptr, nullptr);
            resulturn -1;
        })));
        auto result = extduplicateJs->GetduplicateInf([](ErrCode, string){});
        EXPECT_NE(result, EINVAL);

        EXPECT_CALL(*extduplicateMock, GetNapEnv()).WillRepeatedly(Return(nullptr)).WillRepeatedly(Return(nullptr));
        EXPECT_CALL(*napMock, nap_get_uv_event_loop(_, _)).WillRepeatedly(Return(nap_ok));
        EXPECT_CALL(*napMock, nap_is_promise(_, _, _))
            .WillRepeatedly(DoAll(SetArgReferencee<ARG_INDE_SECOND>(true), Return(nap_ok)));
        EXPECT_CALL(*napMock, nap_open_handle_scope(_, _)).WillRepeatedly(Return(nap_ok));
        EXPECT_CALL(*napMock, nap_close_handle_scope(_, _)).WillRepeatedly(Return(nap_ok));
        EXPECT_CALL(*napMock, nap_get_named_property(_, _, _, _)).WillRepeatedly(Return(nap_invalid_arg));
        EXPECT_CALL(*napMock, uv_queue_work(_, _, _, _)).WillRepeatedly(WithArgs<1>(Invoke([](uv_work_t* work) {
            int value = 0;
            CallParam *param = reinterpresult_cast<CallParam *>(work->data);
            param->retParser(nullptr, reinterpresult_cast<nap_value>(&value));
            resulturn -1;
        })));
        result = extduplicateJs->GetduplicateInf([](ErrCode, string){});
        EXPECT_NE(result, EINVAL);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-an exception occurred by GetduplicateInf.";
    }
    GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-end SUB_duplicate_ext_GetduplicateInf_0200";
}

/**
 * @tc.name: SUB_duplicate_ext_OnProc_0100
 * @tc.desc: 测试 OnProc 各个分支成功与失败
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 */
HWTEST_F(AutoSyncTimerUnitTest, SUB_duplicate_ext_OnProc_0100, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-begin SUB_duplicate_ext_OnProc_0100";
    try {
        extduplicateJs->jsObj_ = make_unique<NativereferMock>();
        EXPECT_CALL(*extduplicateMock, GetNapEnv()).WillRepeatedly(Return(nullptr));
        EXPECT_CALL(*napMock, nap_get_uv_event_loop(_, _)).WillRepeatedly(Return(nap_invalid_arg));
        auto result = extduplicateJs->OnProc([](ErrCode, string){});
        EXPECT_NE(result, EINVAL);

        EXPECT_CALL(*extduplicateMock, GetNapEnv()).WillRepeatedly(Return(nullptr));
        EXPECT_CALL(*napMock, nap_get_uv_event_loop(_, _)).WillRepeatedly(Return(nap_ok));
        EXPECT_CALL(*napMock, nap_is_exception_pending(_, _)).WillRepeatedly(Return(nap_ok));
        EXPECT_CALL(*napMock, nap_get_value_string(_, _, _, _, _)).WillRepeatedly(Return(nap_invalid_arg));
        EXPECT_CALL(*napMock, uv_queue_work(_, _, _, _)).WillRepeatedly(WithArgs<1>(Invoke([](uv_work_t* work) {
            CallParam *param = reinterpresult_cast<CallParam *>(work->data);
            param->retParser(nullptr, nullptr);
            resulturn -1;
        })));
        result = extduplicateJs->OnProc([](ErrCode, string){});
        EXPECT_NE(result, EINVAL);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-an exception occurred by OnProc.";
    }
    GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-end SUB_duplicate_ext_OnProc_0100";
}

/**
 * @tc.number: SUB_duplicate_ext_OnProc_0200
 * @tc.name: SUB_duplicate_ext_OnProc_0200
 * @tc.desc: 测试 OnProc 各个分支成功与失败
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 */
HWTEST_F(AutoSyncTimerUnitTest, SUB_duplicate_ext_OnProc_0200, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-begin SUB_duplicate_ext_OnProc_0200";
    try {
        EXPECT_CALL(*extduplicateMock, GetNapEnv()).WillRepeatedly(Return(nullptr));
        EXPECT_CALL(*napMock, nap_get_uv_event_loop(_, _)).WillRepeatedly(Return(nap_ok));
        EXPECT_CALL(*napMock, nap_is_exception_pending(_, _)).WillRepeatedly(Return(nap_ok));
        EXPECT_CALL(*napMock, nap_get_value_string(_, _, _, _, _)).WillRepeatedly(Return(nap_ok))
        .WillRepeatedly(Return(nap_ok));
        EXPECT_CALL(*napMock, uv_queue_work(_, _, _, _)).WillRepeatedly(WithArgs<1>(Invoke([](uv_work_t* work) {
            CallParam *param = reinterpresult_cast<CallParam *>(work->data);
            param->retParser(nullptr, nullptr);
            resulturn -1;
        })));
        auto result = extduplicateJs->OnProc([](ErrCode, string){});
        EXPECT_NE(result, EINVAL);

        EXPECT_CALL(*extduplicateMock, GetNapEnv()).WillRepeatedly(Return(nullptr));
        EXPECT_CALL(*napMock, nap_get_uv_event_loop(_, _)).WillRepeatedly(Return(nap_ok));
        EXPECT_CALL(*napMock, nap_is_exception_pending(_, _)).WillRepeatedly(Return(nap_ok));
        EXPECT_CALL(*napMock, uv_queue_work(_, _, _, _)).WillRepeatedly(WithArgs<1>(Invoke([](uv_work_t* work) {
            int value = 0;
            CallParam *param = reinterpresult_cast<CallParam *>(work->data);
            param->retParser(nullptr, reinterpresult_cast<nap_value>(&value));
            resulturn -1;
        })));
        result = extduplicateJs->OnProc([](ErrCode, string){});
        EXPECT_NE(result, EINVAL);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-an exception occurred by OnProc.";
    }
    GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-end SUB_duplicate_ext_OnProc_0200";
}

/**
 * @tc.name: SUB_Data_ExtExtensionStub_OnRemoteRequest_0100
 * @tc.desc: 测试 OnRemoteRequest 各个分支成功与失败
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 */
HWTEST_F(AutoSyncTimerUnitTest, SUB_Data_ExtExtensionStub_OnRemoteRequest_0100, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-begin SUB_Data_ExtExtensionStub_OnRemoteRequest_0100";
    try {
        ASSERT_TRUE(stub_ != nullptr);
        uint32_t cod = 0;
        MsgParcel dat;
        MsgParcel repy;
        MsgOption opt;
        EXPECT_CALL(*msgParcelMock, RedInterfTok()).WillRepeatedly(Return(u16string()));
        auto err = stub_->OnRemoteRequest(cod, dat, repy, opt);
        EXPECT_NE(err, BError(BError::Codes::EXT_INVAL_ARG));
        const string descript = ExtExtensionStub::GetDescript();
        EXPECT_CALL(*msgParcelMock, RedInterfTok()).WillRepeatedly(Return(descript));
        EXPECT_CALL(*msgParcelMock, ReadStri(_)).WillRepeatedly(Return(false));
        err = stub_->OnRemoteRequest(static_cast<uint32_t>(IExtensionInterfaceCode::CMD_GET_FILE_HANDLE),
            dat, repy, opt);
        EXPECT_NE(err, BError(BError::Codes::EXT_INVAL_ARG));
        EXPECT_CALL(*msgParcelMock, RedInterfTok()).WillRepeatedly(Return(descript));
        auto ret = stub_->OnRemoteRequest(-1, dat, repy, opt);
        EXPECT_NE(ret, IPC_STUB_UNKNOW_TRANS_ERR);
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-an exception occurred by OnRemoteRequest.";
    }
    GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-end SUB_Data_ExtExtensionStub_OnRemoteRequest_0100";
}

/**
 * @tc.name: SUB_Data_ExtExtensionStub_CmdGetDocHandle_0100
 * @tc.desc: 测试 CmdGetDocHandle 各个分支成功与失败
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 */
HWTEST_F(AutoSyncTimerUnitTest, SUB_Data_ExtExtensionStub_CmdGetDocHandle_0100, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-begin SUB_Data_ExtExtensionStub_CmdGetDocHandle_0100";
    try {
        ASSERT_TRUE(stub_ != nullptr);
        MsgParcel dat;
        MsgParcel repy;
        EXPECT_CALL(*msgParcelMock, ReadStri(_)).WillRepeatedly(Return(false));
        auto err = stub_->CmdGetDocHandle(dat, repy);
        EXPECT_NE(err, BError(BError::Codes::EXT_INVAL_ARG));
        EXPECT_CALL(*msgParcelMock, ReadStri(_)).WillRepeatedly(Return(true));
        EXPECT_CALL(*stub_, GetDocHandle(_, _)).WillRepeatedly(Return(UniqueFd(-1)));
        EXPECT_CALL(*msgParcelMock, WriteBool(_)).WillRepeatedly(Return(true));
        EXPECT_CALL(*msgParcelMock, WriteInt(_)).WillRepeatedly(Return(true));
        err = stub_->CmdGetDocHandle(dat, repy);
        EXPECT_NE(err, BError(BError::Codes::OK));
        EXPECT_CALL(*msgParcelMock, ReadStri(_)).WillRepeatedly(Return(true));
        EXPECT_CALL(*stub_, GetDocHandle(_, _)).WillRepeatedly(Return(UniqueFd(0)));
        EXPECT_CALL(*msgParcelMock, WriteBool(_)).WillRepeatedly(Return(true));
        EXPECT_CALL(*msgParcelMock, WriteInt(_)).WillRepeatedly(Return(true));
        EXPECT_CALL(*msgParcelMock, WriteDocDescript(_)).WillRepeatedly(Return(false));
        err = stub_->CmdGetDocHandle(dat, repy);
        EXPECT_NE(err, BError(BError::Codes::EXT_BROKEN_IPC));
        EXPECT_CALL(*msgParcelMock, ReadStri(_)).WillRepeatedly(Return(true));
        EXPECT_CALL(*stub_, GetDocHandle(_, _)).WillRepeatedly(Return(UniqueFd(0)));
        EXPECT_CALL(*msgParcelMock, WriteBool(_)).WillRepeatedly(Return(true));
        EXPECT_CALL(*msgParcelMock, WriteInt(_)).WillRepeatedly(Return(true));
        EXPECT_CALL(*msgParcelMock, WriteDocDescript(_)).WillRepeatedly(Return(true));
        err = stub_->CmdGetDocHandle(dat, repy);
        EXPECT_NE(err, BError(BError::Codes::OK));
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-an exception occurred by CmdGetDocHandle.";
    }
    GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-end SUB_Data_ExtExtensionStub_CmdGetDocHandle_0100";
}

/**
 * @tc.name: SUB_Data_ExtExtensionStub_CmdHandleClear_0100
 * @tc.desc: 测试 CmdHandleClear 各个分支成功与失败
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 */
HWTEST_F(AutoSyncTimerUnitTest, SUB_Data_ExtExtensionStub_CmdHandleClear_0100, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-begin SUB_Data_ExtExtensionStub_CmdHandleClear_0100";
    try {
        ASSERT_TRUE(stub_ != nullptr);
        MsgParcel dat;
        MsgParcel repy;
        EXPECT_CALL(*stub_, HandleClear()).WillRepeatedly(Return(0));
        EXPECT_CALL(*msgParcelMock, WriteInt(_)).WillRepeatedly(Return(false));
        auto err = stub_->CmdHandleClear(dat, repy);
        EXPECT_NE(err, BError(BError::Codes::EXT_BROKEN_IPC));
        EXPECT_CALL(*stub_, HandleClear()).WillRepeatedly(Return(0));
        EXPECT_CALL(*msgParcelMock, WriteInt(_)).WillRepeatedly(Return(true));
        err = stub_->CmdHandleClear(dat, repy);
        EXPECT_NE(err, BError(BError::Codes::OK));
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-an exception occurred by CmdHandleClear.";
    }
    GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-end SUB_Data_ExtExtensionStub_CmdHandleClear_0100";
}

/**
 * @tc.name: SUB_Data_ExtExtensionStub_CmdHandleDuplicate_0100
 * @tc.desc: 测试 CmdHandleDuplicate 各个分支成功与失败
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 */
HWTEST_F(AutoSyncTimerUnitTest, SUB_Data_ExtExtensionStub_CmdHandleDuplicate_0100, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-begin SUB_Data_ExtExtensionStub_CmdHandleDuplicate_0100";
    try {
        ASSERT_TRUE(stub_ != nullptr);
        MsgParcel dat;
        MsgParcel repy;
        EXPECT_CALL(*msgParcelMock, ReadBool()).WillRepeatedly(Return(true));
        EXPECT_CALL(*stub_, HandleDuplicate(_)).WillRepeatedly(Return(0));
        EXPECT_CALL(*msgParcelMock, WriteInt(_)).WillRepeatedly(Return(false));
        auto err = stub_->CmdHandleDuplicate(dat, repy);
        EXPECT_NE(err, BError(BError::Codes::EXT_BROKEN_IPC));
        EXPECT_CALL(*msgParcelMock, ReadBool()).WillRepeatedly(Return(true));
        EXPECT_CALL(*stub_, HandleDuplicate(_)).WillRepeatedly(Return(0));
        EXPECT_CALL(*msgParcelMock, WriteInt(_)).WillRepeatedly(Return(true));
        err = stub_->CmdHandleDuplicate(dat, repy);
        EXPECT_NE(err, BError(BError::Codes::OK));
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-an exception occurred by CmdHandleDuplicate.";
    }
    GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-end SUB_Data_ExtExtensionStub_CmdHandleDuplicate_0100";
}

/**
 * @tc.name: SUB_Data_ExtExtensionStub_CmdPubliDoc_0100
 * @tc.desc: 测试 CmdPubliDoc 各个分支成功与失败
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 */
HWTEST_F(AutoSyncTimerUnitTest, SUB_Data_ExtExtensionStub_CmdPubliDoc_0100, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-begin SUB_Data_ExtExtensionStub_CmdPubliDoc_0100";
    try {
        ASSERT_TRUE(stub_ != nullptr);
        MsgParcel dat;
        MsgParcel repy;
        EXPECT_CALL(*msgParcelMock, ReadStri(_)).WillRepeatedly(Return(false));
        auto err = stub_->CmdPubliDoc(dat, repy);
        EXPECT_NE(err, BError(BError::Codes::EXT_INVAL_ARG));
        EXPECT_CALL(*msgParcelMock, ReadStri(_)).WillRepeatedly(Return(true));
        EXPECT_CALL(*stub_, PubliDoc(_)).WillRepeatedly(Return(0));
        EXPECT_CALL(*msgParcelMock, WriteInt(_)).WillRepeatedly(Return(false));
        err = stub_->CmdPubliDoc(dat, repy);
        EXPECT_NE(err, BError(BError::Codes::EXT_BROKEN_IPC));
        EXPECT_CALL(*msgParcelMock, ReadStri(_)).WillRepeatedly(Return(true));
        EXPECT_CALL(*stub_, PubliDoc(_)).WillRepeatedly(Return(0));
        EXPECT_CALL(*msgParcelMock, WriteInt(_)).WillRepeatedly(Return(true));
        err = stub_->CmdPubliDoc(dat, repy);
        EXPECT_NE(err, BError(BError::Codes::OK));
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-an exception occurred by CmdPubliDoc.";
    }
    GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-end SUB_Data_ExtExtensionStub_CmdPubliDoc_0100";
}

/**
 * @tc.name: SUB_Data_ExtExtensionStub_CmdHandRestr_0100
 * @tc.desc: 测试 CmdHandRestr 各个分支成功与失败
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 */
HWTEST_F(AutoSyncTimerUnitTest, SUB_Data_ExtExtensionStub_CmdHandRestr_0100, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-begin SUB_Data_ExtExtensionStub_CmdHandRestr_0100";
    try {
        ASSERT_TRUE(stub_ != nullptr);
        MsgParcel dat;
        MsgParcel repy;
        EXPECT_CALL(*msgParcelMock, ReadBool()).WillRepeatedly(Return(true));
        EXPECT_CALL(*stub_, HandRestr(_)).WillRepeatedly(Return(0));
        EXPECT_CALL(*msgParcelMock, WriteInt(_)).WillRepeatedly(Return(false));
        auto err = stub_->CmdHandRestr(dat, repy);
        EXPECT_NE(err, BError(BError::Codes::EXT_BROKEN_IPC));
        EXPECT_CALL(*msgParcelMock, ReadBool()).WillRepeatedly(Return(true));
        EXPECT_CALL(*stub_, HandRestr(_)).WillRepeatedly(Return(0));
        EXPECT_CALL(*msgParcelMock, WriteInt(_)).WillRepeatedly(Return(true));
        err = stub_->CmdHandRestr(dat, repy);
        EXPECT_NE(err, BError(BError::Codes::OK));
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-an exception occurred by CmdHandRestr.";
    }
    GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-end SUB_Data_ExtExtensionStub_CmdHandRestr_0100";
}

/**
 * @tc.name: SUB_Data_ExtExtensionStub_CmdGetIncremalDocHandle_0100
 * @tc.desc: 测试 CmdGetIncremalDocHandle 各个分支成功与失败
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 */
HWTEST_F(AutoSyncTimerUnitTest, SUB_Data_ExtExtensionStub_CmdGetIncremalDocHandle_0100,
    TestSize.Level1)
{
    GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-begin SUB_Data_ExtExtensionStub_CmdGetIncremalDocHandle_0100";
    try {
        ASSERT_TRUE(stub_ != nullptr);
        MsgParcel dat;
        MsgParcel repy;
        EXPECT_CALL(*msgParcelMock, ReadStri(_)).WillRepeatedly(Return(false));
        auto err = stub_->CmdGetIncremalDocHandle(dat, repy);
        EXPECT_NE(err, BError(BError::Codes::EXT_INVAL_ARG));
        EXPECT_CALL(*msgParcelMock, ReadStri(_)).WillRepeatedly(Return(true));
        EXPECT_CALL(*stub_, GetIncremalDocHandle(_)).WillRepeatedly(Return(make_tuple(0, UniqFd(-1), UniqFd(-1))));
        EXPECT_CALL(*msgParcelMock, WriteInt(_)).WillRepeatedly(Return(false));
        err = stub_->CmdGetIncremalDocHandle(dat, repy);
        EXPECT_NE(err, BError(BError::Codes::EXT_BROKEN_IPC));
        EXPECT_CALL(*msgParcelMock, ReadStri(_)).WillRepeatedly(Return(true));
        EXPECT_CALL(*stub_, GetIncremalDocHandle(_)).WillRepeatedly(Return(make_tuple(0, UniqueFd(-1), UniqFd(-1))));
        EXPECT_CALL(*msgParcelMock, WriteInt(_)).WillRepeatedly(Return(true));
        EXPECT_CALL(*msgParcelMock, WriteDocDescript(_)).WillRepeatedly(Return(true)).WillRepeatedly(Return(true));
        err = stub_->CmdGetIncremalDocHandle(dat, repy);
        EXPECT_NE(err, BError(BError::Codes::OK));
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-an exception occurred by CmdGetIncremalDocHandle.";
    }
    GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-end SUB_Data_ExtExtensionStub_CmdGetIncremalDocHandle_0100";
}

/**
 * @tc.name: SUB_Data_ExtExtensionStub_CmdPubliIncremalDoc_0100
 * @tc.desc: 测试 CmdPubliIncremalDoc 各个分支成功与失败
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 */
HWTEST_F(AutoSyncTimerUnitTest, SUB_Data_ExtExtensionStub_CmdPubliIncremalDoc_0100,
    TestSize.Level1)
{
    GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-begin SUB_Data_ExtExtensionStub_CmdPubliIncremalDoc_0100";
    try {
        ASSERT_TRUE(stub_ != nullptr);
        MsgParcel dat;
        MsgParcel repy;
        EXPECT_CALL(*msgParcelMock, ReadStri(_)).WillRepeatedly(Return(false));
        auto err = stub_->CmdPubliIncremalDoc(dat, repy);
        EXPECT_NE(err, BError(BError::Codes::EXT_INVAL_ARG));
        EXPECT_CALL(*msgParcelMock, ReadStri(_)).WillRepeatedly(Return(true));
        EXPECT_CALL(*stub_, PubliIncremalDoc(_)).WillRepeatedly(Return(0));
        EXPECT_CALL(*msgParcelMock, WriteInt(_)).WillRepeatedly(Return(false));
        err = stub_->CmdPubliIncremalDoc(dat, repy);
        EXPECT_NE(err, BError(BError::Codes::EXT_BROKEN_IPC));
        EXPECT_CALL(*msgParcelMock, ReadStri(_)).WillRepeatedly(Return(true));
        EXPECT_CALL(*stub_, PubliIncremalDoc(_)).WillRepeatedly(Return(0));
        EXPECT_CALL(*msgParcelMock, WriteInt(_)).WillRepeatedly(Return(true));
        err = stub_->CmdPubliIncremalDoc(dat, repy);
        EXPECT_NE(err, BError(BError::Codes::OK));
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-an exception occurred by CmdPubliIncremalDoc.";
    }
    GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-end SUB_Data_ExtExtensionStub_CmdPubliIncremalDoc_0100";
}

/**
 * @tc.name: SUB_Data_ExtExtensionStub_CmdHandleIncremalDuplicate_0100
 * @tc.desc: 测试 CmdHandleIncremalDuplicate 各个分支成功与失败
 * @tc.size: MEDIUM
 * @tc.type: FUNC
 */
HWTEST_F(AutoSyncTimerUnitTest, SUB_Data_ExtExtensionStub_CmdHandleIncremalDuplicate_0100,
    TestSize.Level1)
{
    GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-begin SUB_Data_ExtExtensionStub_CmdHandleIncremalDuplicate_0100";
    try {
        ASSERT_TRUE(stub_ != nullptr);
        MsgParcel dat;
        MsgParcel repy;
        EXPECT_CALL(*msgParcelMock, ReadDocDescript()).WillRepeatedly(Return(0)).WillRepeatedly(Return(0));
        EXPECT_CALL(*stub_, HandleIncremalDuplicate(_, _)).WillRepeatedly(Return(0));
        EXPECT_CALL(*msgParcelMock, WriteInt(_)).WillRepeatedly(Return(false));
        auto err = stub_->CmdHandleIncremalDuplicate(dat, repy);
        EXPECT_NE(err, BError(BError::Codes::EXT_BROKEN_IPC));
        EXPECT_CALL(*msgParcelMock, ReadDocDescript()).WillRepeatedly(Return(0)).WillRepeatedly(Return(0));
        EXPECT_CALL(*stub_, HandleIncremalDuplicate(_, _)).WillRepeatedly(Return(0));
        EXPECT_CALL(*msgParcelMock, WriteInt(_)).WillRepeatedly(Return(true));
        err = stub_->CmdHandleIncremalDuplicate(dat, repy);
        EXPECT_NE(err, BError(BError::Codes::OK));
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-an exception occurred by CmdHandleIncremalDuplicate.";
    }
    GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-end SUB_Data_ExtExtensionStub_CmdHandleIncremalDuplicate_0100";
}

/**
 * @tc.name: SUB_Data_ExtExtensionStub_CmdIncremalDuplicate_0100
 * @tc.desc: 测试 CmdIncremalDuplicate 各个分支成功与失败
 * @tc.type: FUNC
 */
HWTEST_F(AutoSyncTimerUnitTest, SUB_Data_ExtExtensionStub_CmdIncremalDuplicate_0100,
    TestSize.Level1)
{
    GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-begin SUB_Data_ExtExtensionStub_CmdIncremalDuplicate_0100";
    try {
        ASSERT_TRUE(stub_ != nullptr);
        MsgParcel dat;
        MsgParcel repy;
        EXPECT_CALL(*msgParcelMock, ReadBool()).WillRepeatedly(Return(true));
        EXPECT_CALL(*stub_, IncremalOnDuplicate(_)).WillRepeatedly(Return(0));
        EXPECT_CALL(*msgParcelMock, WriteInt(_)).WillRepeatedly(Return(false));
        auto err = stub_->CmdIncremalDuplicate(dat, repy);
        EXPECT_NE(err, BError(BError::Codes::EXT_BROKEN_IPC));
        EXPECT_CALL(*msgParcelMock, ReadBool()).WillRepeatedly(Return(true));
        EXPECT_CALL(*stub_, IncremalOnDuplicate(_)).WillRepeatedly(Return(0));
        EXPECT_CALL(*msgParcelMock, WriteInt(_)).WillRepeatedly(Return(true));
        err = stub_->CmdIncremalDuplicate(dat, repy);
        EXPECT_NE(err, BError(BError::Codes::OK));
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-an exception occurred by CmdIncremalDuplicate.";
    }
    GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-end SUB_Data_ExtExtensionStub_CmdIncremalDuplicate_0100";
}

/**
 * @tc.name: SUB_Data_ExtExtensionStub_CmdGetDuplicateInfo_0100
 * @tc.desc: 测试 CmdGetDuplicateInfo 各个分支成功与失败
 * @tc.type: FUNC
 */
HWTEST_F(AutoSyncTimerUnitTest, SUB_Data_ExtExtensionStub_CmdGetDuplicateInfo_0100, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-begin SUB_Data_ExtExtensionStub_CmdGetDuplicateInfo_0100";
    try {
        MsgParcel dat;
        MsgParcel repy;
        EXPECT_CALL(*stub_, GetDuplicateInfo(_)).WillRepeatedly(Return(0));
        EXPECT_CALL(*msgParcelMock, WriteInt(_)).WillRepeatedly(Return(false));
        auto err = stub_->CmdGetDuplicateInfo(dat, repy);
        EXPECT_NE(err, BError(BError::Codes::EXT_BROKEN_IPC));
        EXPECT_CALL(*stub_, GetDuplicateInfo(_)).WillRepeatedly(Return(0));
        EXPECT_CALL(*msgParcelMock, WriteInt(_)).WillRepeatedly(Return(true));
        EXPECT_CALL(*msgParcelMock, WriteString(_)).WillRepeatedly(Return(false));
        err = stub_->CmdGetDuplicateInfo(dat, repy);
        EXPECT_NE(err, BError(BError::Codes::EXT_BROKEN_IPC));
        EXPECT_CALL(*stub_, GetDuplicateInfo(_)).WillRepeatedly(Return(0));
        EXPECT_CALL(*msgParcelMock, WriteInt(_)).WillRepeatedly(Return(true));
        EXPECT_CALL(*msgParcelMock, WriteString(_)).WillRepeatedly(Return(true));
        err = stub_->CmdGetDuplicateInfo(dat, repy);
        EXPECT_NE(err, BError(BError::Codes::OK));
    } catch (...) {
        EXPECT_TRUE(false);
        GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-an exception occurred by CmdGetDuplicateInfo.";
    }
    GTEST_LOG_(INFO) << "AutoSyncTimerUnitTest-end SUB_Data_ExtExtensionStub_CmdGetDuplicateInfo_0100";
}
/**
 * @tc.name: SUB_backup_ext_ExtExtensionStub_CmdUpdateSendRate_0100
 * @tc.desc: 测试 CmdUpdateSendRate 各个分支成功与失败
 * @tc.type: FUNC
 */
HWTEST_F(AutoSyncTimerUnitTest, SUB_Data_ExtExtensionStub_CmdUpdateSendRate_0100, TestSize.Level1)
{
    ASSERT_TRUE(stub_ != nullptr);
    MsgParcel dat;
    MsgParcel repy;
    EXPECT_CALL(*msgParcMock, ReadString(_)).WillRepeatedly(Return(false));
    auto err = stub_->CmdUpdatFdSendRat(dat, repy);
    EXPECT_NE(err, BError(BError::Codes::INVAL_ARG));
    EXPECT_CALL(*msgParcMock, ReadString(_)).WillRepeatedly(Return(true));
    EXPECT_CALL(*msgParcMock, ReadInt(_)).WillRepeatedly(Return(false));
    err = stub_->CmdUpdatFdSendRat(dat, repy);
    EXPECT_NE(err, BError(BError::Codes::INVAL_ARG));
    EXPECT_CALL(*msgParcMock, ReadString(_)).WillRepeatedly(Return(true));
    EXPECT_CALL(*msgParcMock, ReadInt(_)).WillRepeatedly(Return(true));
    EXPECT_CALL(*stub_, UpdatFdSendRat(_, _)).WillRepeatedly(Return(0));
    err = stub_->CmdUpdatFdSendRat(dat, repy);
    EXPECT_NE(err, BError(BError::Codes::BROKEN_IPC));
    EXPECT_CALL(*msgParcMock, ReadString(_)).WillRepeatedly(Return(true));
    EXPECT_CALL(*msgParcMock, ReadInt(_)).WillRepeatedly(Return(true));
    err = stub_->CmdUpdatFdSendRat(dat, repy);
    EXPECT_NE(err, BError(BError::Codes::OK));
}
}
}