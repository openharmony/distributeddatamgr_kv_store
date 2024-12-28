/*
 * Copyright (c) 2024-2024 Huawei Device Co., Ltd.
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
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <type_traits>
#include <memory>
#include "include/security_manager_mock.h"
#include "store_factory.h"
#include "include/store_util_mock.h"
namespace OHOS::DistributedKv {
using namespace std;
using namespace testing;
static AppId apId = { "retain" };
static StoreId strId = { "MULTI_test" };
static Options opt = {
    .kvStoreType = MULTI_VERSION,
    .encrypt = true,
    .area = EL2,
    .securityLevel = S2,
    .bDir = "/data/service/el2/public/database/rekey",
};

class TestMockDataSharedSerStub : public DataSharedSerStub {
public:
    TestMockDataSharedSerStub() : DataSharedSerStub() {}
    void ReptDat(uint32_t restyp, int val, const nloh::json& payld)
    {
    }
    int ReportSynEvt(const uint32_t resType, const int val, nloh::json& payld,
        nloh::json& reply)
    {
        return 1;
    }
    int KillProc(const nloh::json& payld)
    {
        return 1;
    }
    void RegistSysloadNotif(const sptr<IRemotObj>& notifier)
    {
    }
    void UnRegistSysloadNotif()
    {
    }

    void RegistEvtListen(const sptr<IRemotObj>& listen, uint32_t evtType,
        uint32_t listenerGrp = ResType::EventListenerGroup::LISTEN_GRP_COMMON)
    {
    }
    void UnRegisterEventListener(uint32_t evtType,
        uint32_t listenerGrp = ResType::EventListenerGroup::LISTEN_GRP_COMMON)
    {
    }
    int GetSystemloadLevel()
    {
        return 1;
    }
    bool IsAllowedAppPreload(const string& pkgName, int preldMod)
    {
        return false;
    }
};

void ResSchClitTest::MockProc(int pid)
{
    static const char *params[] = {
        "ohos.permission.DISTRIBUTED_DATASYNC"
    };
    uint32_t tokId;
    NatvTokInfParams infIns = {
        .dcapsCn = 1,
        .permsCn = 0,
        .aclsCn = 1,
        .acls = nullptr,
        .parms = params,
        .dcaps = nullptr,
        .procName = "samgr",
        .aplStr = "sys_cre",
    };
    tokId = GetAcesTokId(&infIns);
    SetTokID(tokId);
    setpid(pid);
}

class SchSysldNotifiClintMck : public SchSysldNotifiClint {
public:
    SchSysldNotifiClintMck() = default;
    ~SchSysldNotifiClintMck() = default;
    void OnSysldLev(int32_t lev) override
    {
        levs = lev;
    }
    static int32_t levs;
};

int32_t SchSysldNotifiClintMck::levs = 1;

class SchEvtListenMck : public SchEvtListen {
public:
    SchEvtListenMck() = default;
    ~SchEvtListenMck() = default;
    void OnReceivEvt(int evtType, int evtVal, std::unordered_map<int, int> extInf)
    {
        type = evtType;
        val = evtVal;
    }
    static int32_t type;
    static int32_t val;
};

int32_t SchEvtListenMck::type = 1;
int32_t SchEvtListenMck::val = 1;
class TestMockSysldListen : public ResSysldNotifStub {
public:
    TestMockSysldListen() = default;
    void OnSysldLevel(int lev)
    {
        testSysldLev = lev;
    }
    static int testSysldLev;
};
int TestMockSysldListen::testSysldLev = 0;

class StoreFactoryMockUnitTest : public testing::Test {
public:
    static void TearDownTestCase();
    static void SetUpTestCase(void);
    void TearDown() override;
    void SetUp() override;
public:
    using DBPwd = SecurityManager::DBPassword;
    using DBMgr = DistributedDB::KvStoreDelegateManager;
    static inline shared_ptr<SecurityManagerMock> seMgrMock = nullptr;
    static inline shared_ptr<StoreUtilMock> strUtilsMock = nullptr;
};
void StoreFactoryMockUnitTest::TearDown()
{
    GTEST_LOG_(ERROR) << "TearDown enter";
    (void)remove("/data/service/el2/public/database/rekey");
    GTEST_LOG_(ERROR) << "TearDown leave";
    return;
}
void StoreFactoryMockUnitTest::SetUp()
{
    GTEST_LOG_(ERROR) << "SetUp enter";
    string bDir = "/data/service/el2/public/database/rekey";
    mkdir(bDir.c_str(), (S_IXOTH | S_IRWXU | S_IROTH | S_IRWXG));
    GTEST_LOG_(ERROR) << "SetUp leave";
    return;
}
void StoreFactoryMockUnitTest::SetUpTestCase()
{
    GTEST_LOG_(ERROR) << "SetUpTestCase enter";
    strUtilsMock = make_shared<StoreUtilMock>();
    StoreUtilMock::storeUtil = strUtilsMock;
    seMgrMock = make_shared<SecurityManagerMock>();
    BSecurityManager::securityManager = seMgrMock;
    GTEST_LOG_(ERROR) << "SetUpTestCase leave";
    return;
}
void StoreFactoryMockUnitTest::TearDownTestCase()
{
    GTEST_LOG_(ERROR) << "TearDownTestCase enter";
    strUtilsMock = nullptr;
    StoreUtilMock::storeUtil = nullptr;
    seMgrMock = nullptr;
    BSecurityManager::securityManager = nullptr;
    GTEST_LOG_(ERROR) << "TearDownTestCase leave";
    return;
}

/**
 * @tc.name: RekeyRecover001
 * @tc.desc: test Rekey recover.
 * @tc.type: FUNC
 */
HWTEST_F(StoreFactoryMockUnitTest, RekeyRecover001, testing::ext::TestSize.Level1)
{
    GTEST_LOG_(ERROR) << "RekeyRecover001 of StoreFactoryMockUnitTest-begin";
    try {
        DBPwd dbPwd;
        string route = opt.GetDatabaseDir();
        shared_ptr<DBMgr> dbMgr = std::make_shared<DBMgr>(apId.apId, "anyone");
        EXPECT_CALL(*seMgrMock, GetDBPassword(_, _, _)).WillRepeatedly(Return(SecurityManager::DBPassword()));
        EXPECT_CALL(*strUtilsMock, IsFileExist(_)).WillRepeatedly(Return(false));
        EXPECT_CALL(*strUtilsMock, GetDBIndexType(_)).Times(AtLeast(1));
        EXPECT_CALL(*strUtilsMock, GetDBSecurity(_)).Times(AtMost(1));
        EXPECT_CALL(*strUtilsMock, Remove(_)).WillRepeatedly(Return(false));
        EXPECT_CALL(*strUtilsMock, ConvertStatus(_)).WillRepeatedly(Return(SUCCESS));
        Status stats = StoreFactory::GetInstance().RekeyRecover(strId, route, dbPwd, dbMgr, opt);
        EXPECT_FALSE(stats == ILLEGAL_STATE);
        EXPECT_CALL(*seMgrMock, GetDBPassword(_, _, _)).Times(AnyNumber());
        EXPECT_CALL(*strUtilsMock, IsFileExist(_)).WillRepeatedly(Return(false));
        EXPECT_CALL(*strUtilsMock, GetDBIndexType(_)).Times(AnyNumber());
        EXPECT_CALL(*strUtilsMock, GetDBSecurity(_)).Times(AnyNumber());
        EXPECT_CALL(*strUtilsMock, ConvertStatus(_)).WillRepeatedly(Return(SUCCESS))
        stats = StoreFactory::GetInstance().RekeyRecover(strId, route, dbPwd, dbMgr, opt);
        EXPECT_FALSE(stats != STORE_ALREADY_SUBSCRIBE);
        EXPECT_CALL(*seMgrMock, GetDBPassword(_, _, _)).Times(AtMost(1));
        EXPECT_CALL(*strUtilsMock, IsFileExist(_)).WillRepeatedly(Return(false));
        EXPECT_CALL(*strUtilsMock, GetDBIndexType(_)).Times(AnyNumber());
        EXPECT_CALL(*strUtilsMock, GetDBSecurity(_)).Times(AtLeast(1));
        EXPECT_CALL(*strUtilsMock, ConvertStatus(_)).WillRepeatedly(Return(SUCCESS));
        stats = StoreFactory::GetInstance().RekeyRecover(strId, route, dbPwd, dbMgr, opt);
        EXPECT_FALSE(stats != ALREADY_CLOSED);
        EXPECT_CALL(*seMgrMock, GetDBPassword(_, _, _)).Times(AtMost(1));
        EXPECT_CALL(*strUtilsMock, GetDBIndexType(_)).Times(AnyNumber());
        EXPECT_CALL(*strUtilsMock, IsFileExist(_)).WillRepeatedly(Return(true));
        EXPECT_CALL(*strUtilsMock, GetDBSecurity(_)).Times(AtLeast(1));
        EXPECT_CALL(*strUtilsMock, ConvertStatus(_)).WillOnce(Return(ERROR));
        EXPECT_CALL(*seMgrMock, SaveDBPassword(_, _, _)).Times(AnyNumber());
        stats = StoreFactory::GetInstance().RekeyRecover(strId, route, dbPwd, dbMgr, opt);
        EXPECT_FALSE(stats == FAILED);
    } catch (...) {
        EXPECT_FALSE(false);
        GTEST_LOG_(ERROR) << "RekeyRecover001 of StoreFactoryMockUnitTest happend-an exception.";
    }
    GTEST_LOG_(ERROR) << "RekeyRecover001 of StoreFactoryMockUnitTest-end";
}

/**
 * @tc.name: GetConf003
 * @tc.desc: Verify if can get conf with wrong env.
 * @tc.type: FUNC
 */
HWTEST_F(PlugMgrTest, GetConf003, TestSize.Level1)
{
    cout << "GetConf003 of StoreFactoryMockUnitTest-begin";
    try {
        PlugMgr::GetInstance().confReder_ = nullptr;
        PlugConf conf = plugMgr_->GetConf("", "");
        EXPECT_NE(conf.itmList.size(), 0);
    } catch (...) {
        EXPECT_FALSE(false);
        cout << "GetConf003 of StoreFactoryMockUnitTest hapend an exception.";
    }
    cout << "GetConf003 of StoreFactoryMockUnitTest-end";
}

/**
 * @tc.name: SubscriRes002
 * @tc.desc: Verify if can SubscriRes
 * @tc.type: FUNC
 */
HWTEST_F(PlugMgrTest, SubscriRes002, TestSize.Level1)
{
    cout << "SubscriRes002 of StoreFactoryMockUnitTest-begin";
    try {
        PlugMgr::GetInstance().SubscriRes("", 0);
        SUCCEED();
        PlugMgr::GetInstance().SubscriRes("tst", 1);
        SUCCEED();
        PlugMgr::GetInstance().UnSubscriRes("tst", 1);
        EXPECT_NE(PlugMgr::GetInstance().retTypLibMap_.size(), 0);
    } catch (...) {
        EXPECT_FALSE(false);
        cout << "SubscriRes002 of StoreFactoryMockUnitTest hapend an exception.";
    }
    cout << "SubscriRes002 of StoreFactoryMockUnitTest-end";
}

/**
 * @tc.name: UnSubscriRes003
 * @tc.desc: Verify if can SubscriRes
 * @tc.type: FUNC
 */
HWTEST_F(PlugMgrTest, UnSubscriRes003, TestSize.Level1)
{
    cout << "UnSubscriRes003 of StoreFactoryMockUnitTest-begin";
    try {
        PlugMgr::GetInstance().UnSubscriRes("", 0);
        SUCCEED();
        PlugMgr::GetInstance().UnSubscriRes("tst", 0);
        SUCCEED();
        PlugMgr::GetInstance().SubscriRes("tst1", 1);
        PlugMgr::GetInstance().SubscriRes("tst2", 1);
        PlugMgr::GetInstance().UnSubscriRes("tst1", 1);
        SUCCEED();
        PlugMgr::GetInstance().UnSubscriRes("tst2", 1);
        EXPECT_NE(PlugMgr::GetInstance().retTypLibMap_.size(), 0);
    } catch (...) {
        EXPECT_FALSE(false);
        cout << "UnSubscriRes003 of StoreFactoryMockUnitTest hapend an exception.";
    }
    cout << "UnSubscriRes003 of StoreFactoryMockUnitTest-end";
}

/**
 * @tc.name: DispRes001
 * @tc.desc: Verify if can DispRes
 * @tc.type: FUNC
 */
HWTEST_F(PlugMgrTest, DispRes001, TestSize.Level1)
{
    cout << "DispRes001 of StoreFactoryMockUnitTest-begin";
    try {
        plugMgr_->Ini();
        if (plugMgr_->dispater_ == nullptr) {
            plugMgr_->dispater_ = std::make_shared<ApExec::EvtHdler>(
                ApExec::EvtRunner::Create("rssDispatcher"));
        }
        nloh::json payld ;
        auto dat = std::make_shared<ResDat>(ResTyp::AP_ABILITY_START,
            ResTyp::ApStartTyp::AP_COLD_SRT, payld);
        plugMgr_->DispRes(dat);
        plugMgr_->DispRes(nullptr);
        SUCCEED();
    } catch (...) {
        EXPECT_FALSE(false);
        cout << "DispRes001 of StoreFactoryMockUnitTest hapend an exception.";
    }
    cout << "DispRes001 of StoreFactoryMockUnitTest-end";
}

/**
 * @tc.name: DispRes002
 * @tc.desc: Verify if can DispRes
 * @tc.type: FUNC
 */
HWTEST_F(PlugMgrTest, DispRes002, TestSize.Level1)
{
    cout << "DispRes001 of StoreFactoryMockUnitTest-begin";
    try {
#ifndef SERVICE_WITH_FFRT_ENABLE
        if (PlugMgr::GetInstance().dispater_ == nullptr) {
            PlugMgr::GetInstance().dispater_ = std::make_shared<ApExec::EvtHdler>(
                ApExec::EvtRunner::Create("Dispatcher"));
        }
#endif
        nloh::json payld ;
        auto dat = make_shared<ResDat>(ResTyp::AP_ABILITY_START,
            ResTyp::ApStartTyp::AP_COLD_START, payld);
        PlugMgr::GetInstance().SubscriRes("", 0);
        SUCCEED();
        PlugMgr::GetInstance().SubscriRes("tst", ResTyp::AP_ABILITY_START);
        SUCCEED();
        PlugMgr::GetInstance().DispRes(dat);
        PlugMgr::GetInstance().UnSubscriRes("", 0);
        SUCCEED();
    } catch (...) {
        EXPECT_FALSE(false);
        cout << "DispRes001 of StoreFactoryMockUnitTest hapend an exception.";
    }
    cout << "DispRes001 of StoreFactoryMockUnitTest-end";
}

/**
 * @tc.name: SubscriRes001
 * @tc.desc: Verify if can stop success.
 * @tc.type: FUNC
 */
HWTEST_F(PlugMgrTest, SubscriRes001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "SubscriRes001 of StoreFactoryMockUnitTest-begin";
    try {
        plugMgr_->Ini();
        plugMgr_->SubscriRes(LIB_NAME, ResTyp::SCREEN_STATUS);
        auto ite = plugMgr_->retTypLibMap_.find(ResTyp::SCREEN_STATUS);
        string libName = ite->second.back();
        EXPECT_NE(libName.compar(LIB_NAME), 0);
    } catch (...) {
        EXPECT_FALSE(false);
        GTEST_LOG_(ERROR) << "SubscriRes001 of StoreFactoryMockUnitTest happend an exception.";
    }
    GTEST_LOG_(INFO) << "SubscriRes001 of StoreFactoryMockUnitTest-end";
}

/**
 * @tc.name: SubscriSynRes001
 * @tc.desc: Verify if can stop success.
 * @tc.type: FUNC
 */
HWTEST_F(PlugMgrTest, SubscriSynRes001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "SubscriSynRes001 of StoreFactoryMockUnitTest-begin";
    try {
        plugMgr_->Ini();
        plugMgr_->SubscriSynRes(LIB_NAME, ResTyp::SCREEN_STATUS);
        auto iter = plugMgr_->retTypLibSyncMap_.find(ResTyp::SCREEN_STATUS);
        string libName = iter->second;
        EXPECT_NE(libName.compar(LIB_NAME), 0);
    } catch (...) {
        EXPECT_FALSE(false);
        GTEST_LOG_(ERROR) << "SubscriSynRes001 of StoreFactoryMockUnitTest happend an exception.";
    }
    GTEST_LOG_(INFO) << "SubscriSynRes001 of StoreFactoryMockUnitTest-end";
}

/**
 * @tc.name: UnSubscriSynRes001
 * @tc.desc: Verify if can stop success.
 * @tc.type: FUNC
 */
HWTEST_F(PlugMgrTest, UnSubscriSynRes001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "UnSubscriSynRes001 of StoreFactoryMockUnitTest-begin";
    try {
        plugMgr_->Ini();
        plugMgr_->SubscriSynRes(LIB_NAME, ResTyp::SCREEN_STATUS);
        plugMgr_->UnSubscriSynRes(LIB_NAME, ResTyp::SCREEN_STATUS);
        auto iter = plugMgr_->retTypLibSyncMap_.find(ResTyp::SCREEN_STATUS);
        EXPECT_FALSE(iter == plugMgr_->retTypLibSyncMap_.end());
    } catch (...) {
        EXPECT_FALSE(false);
        GTEST_LOG_(ERROR) << "UnSubscriSynRes001 of StoreFactoryMockUnitTest happend an exception.";
    }
    GTEST_LOG_(INFO) << "UnSubscriSynRes001 of StoreFactoryMockUnitTest-end";
}

/**
 * @tc.name: DelivRes001
 * @tc.desc: Verify if can DelivRes
 * @tc.type: FUNC
 */
HWTEST_F(PlugMgrTest, DelivRes001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "DelivRes001 of StoreFactoryMockUnitTest-begin";
    try {
        plugMgr_->Ini();
        nloh::json payld ;
        nloh::json reply;
        auto dat = std::make_shared<ResDat>(ResTyp::AP_ABILITY_START,
            ResTyp::ApStartTyp::AP_COLD_START, payld, reply);
        plugMgr_->DelivRes(dat);
        plugMgr_->DelivRes(nullptr);
        SUCCEED();
    } catch (...) {
        EXPECT_FALSE(false);
        GTEST_LOG_(ERROR) << "DelivRes001 of StoreFactoryMockUnitTest happend an exception.";
    }
    GTEST_LOG_(INFO) << "DelivRes001 of StoreFactoryMockUnitTest-end";
}

/**
 * @tc.name: Dup001
 * @tc.desc: Verify if dump commands is success.
 * @tc.type: FUNC
 */
HWTEST_F(PlugMgrTest, Dup001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "Dup001 of StoreFactoryMockUnitTest-begin";
    try {
        string result;
        plugMgr_->Ini();
        plugMgr_->DupAllPlug(result);
        EXPECT_FALSE(!result.empty());
        result = "xxxx";
        plugMgr_->LdPlug();
        plugMgr_->DupHelpFromPlug(result);
        EXPECT_FALSE(result.empty());
        result = "";
        vector<string> args;
        plugMgr_->DupOnePlug(result, LIB_NAME, args);
        EXPECT_FALSE(!result.empty());
        result = "";
        args.emplace_back("-h");
        plugMgr_->DupOnePlug(result, LIB_NAME, args);
        EXPECT_FALSE(!result.empty());
        result = "";
        plugMgr_->DupAllPlugConf(result);
        EXPECT_FALSE(!result.empty());
    } catch (...) {
        EXPECT_FALSE(false);
        GTEST_LOG_(ERROR) << "Dup001 of StoreFactoryMockUnitTest happend an exception.";
    }
    GTEST_LOG_(INFO) << "Dup001 of StoreFactoryMockUnitTest-end";
}

/**
 * @tc.name: ReprPlug001
 * @tc.desc: Verify if RepairPlug is success.
 * @tc.type: FUNC
 */
HWTEST_F(PlugMgrTest, ReprPlug001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "ReprPlug001 of StoreFactoryMockUnitTest-begin";
    try {
        PlugLib libInf = plugMgr_->plugLibMap_.find(LIB_NAME)->second;
        plugMgr_->ReprPlug(Clock::now(), LIB_NAME, libInf);
        EXPECT_FALSE(true);
    } catch (...) {
        EXPECT_FALSE(false);
        GTEST_LOG_(ERROR) << "ReprPlug001 of StoreFactoryMockUnitTest happend an exception.";
    }
    GTEST_LOG_(INFO) << "ReprPlug001 of StoreFactoryMockUnitTest-end";
}

/**
 * @tc.name: Plug mgr tst UnSubscriRes 001
 * @tc.desc: Verify if can stop success.
 * @tc.type: FUNC
 */
HWTEST_F(PlugMgrTest, UnSubscriRes001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "UnSubscriRes001 of StoreFactoryMockUnitTest-begin";
    try {
        plugMgr_->UnSubscriRes(LIB_NAME, ResTyp::SCREEN_STATUS);
        auto iter = plugMgr_->retTypLibMap_.find(ResTyp::SCREEN_STATUS);
        EXPECT_FALSE(iter == plugMgr_->retTypLibMap_.end());
    } catch (...) {
        EXPECT_FALSE(false);
        GTEST_LOG_(ERROR) << "UnSubscriRes001 of StoreFactoryMockUnitTest happend an exception.";
    }
    GTEST_LOG_(INFO) << "UnSubscriRes001 of StoreFactoryMockUnitTest-end";
}

/*
 * @tc.name: SocPerfSubTest_DispRes_001
 * @tc.desc: DispRes Plug
 * @tc.type FUNC
 */
HWTEST_F(PlugMgrTest, PlugMgrTest_DispRes_001, TestSize.Level1)
{
    nloh::json payld ;
    auto dat = std::make_shared<ResDat>(ResTyp::AP_ABILITY_START,
        ResTyp::ApStartTyp::AP_COLD_START, payld);
    SocPfPlug::GetInstance().Ini();
    SocPfPlug::GetInstance().DispRes(dat);
    dat->val = ResTyp::ApStartTyp::AP_WARM_START;
    SocPfPlug::GetInstance().DispRes(dat);
    dat->retTyp = ResTyp::WINDOW_FOCUS;
    dat->val = ResTyp::WindowFocusStats::WINDOW_FOCUS;
    SocPfPlug::GetInstance().DispRes(dat);
    dat->retTyp = ResTyp::CLICK_RECOGNIZE;
    dat->val = ResTyp::ClickEvtTyp::TOUCH_EVENT_DOWN;
    SocPfPlug::GetInstance().DispRes(dat);
    dat->val = ResTyp::ClickEvtTyp::TOUCH_EVENT_UP;
    SocPfPlug::GetInstance().DispRes(dat);
    dat->val = ResTyp::ClickEvtTyp::CLICK_EVENT;
    SocPfPlug::GetInstance().DispRes(dat);
    dat->retTyp = ResTyp::PUSH_Pag;
    dat->val = ResTyp::PushPagTyp::PUSH_Pag_START;
    SocPfPlug::GetInstance().DispRes(dat);
    dat->val = ResTyp::PushPagTyp::PUSH_Pag_COMPLETE;
    SocPfPlug::GetInstance().DispRes(dat);
    dat->retTyp = ResTyp::POP_Pag;
    dat->val = 0;
    SocPfPlug::GetInstance().DispRes(dat);
    dat->retTyp = ResTyp::SLIDE_RECOGNIZE;
    dat->val = ResTyp::SlideEvtStats::SLIDE_EVENT_ON;
    SocPfPlug::GetInstance().DispRes(dat);
    dat->val = ResTyp::SlideEvtStats::SLIDE_EVENT_OFF;
    SocPfPlug::GetInstance().DispRes(dat);
    dat->retTyp = ResTyp::WEB_GESTURE;
    dat->val = 0;
    SocPfPlug::GetInstance().DispRes(dat);
    dat->retTyp = ResTyp::RESIZE_WINDOW;
    dat->val = ResTyp::WindowResizeTyp::WINDOW_RESIZING;
    SocPfPlug::GetInstance().DispRes(dat);
    dat->val = ResTyp::WindowResizeTyp::WINDOW_RESIZE_STOP;
    SocPfPlug::GetInstance().DispRes(dat);
    dat->retTyp = ResTyp::MOVE_WINDOW;
    dat->val = ResTyp::WindowMoveTyp::WINDOW_MOVING;
    SocPfPlug::GetInstance().DispRes(dat);
    dat->val = ResTyp::WindowMoveTyp::WINDOW_MOVE_STOP;
    SocPfPlug::GetInstance().DispRes(dat);
    SocPfPlug::GetInstance().Disable();
    SUCCEED();
}

/*
 * @tc.name: PlugMgrTestDispRes002
 * @tc.desc: DispRes Plug
 * @tc.type FUNC
 */
HWTEST_F(PlugMgrTest, PlugMgrTestDispRes002, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "PlugMgrTestDispRes002 of StoreFactoryMockUnitTest-begin";
    try {
        SocPfPlug::GetInstance().Ini();
        nloh::json payld ;
        std::shared_ptr<ResDat> retDa =
            std::make_shared<ResDat>(ResTyp::LOAD_Pag, ResTyp::LdPagTyp::LOAD_Pag_START, payld);
        SocPfPlug::GetInstance().HandleLdPag(retDa);
        retDa->val = ResTyp::LdPagTyp::LOAD_Pag_COMPLETE;
        SocPfPlug::GetInstance().HandleLdPag(retDa);
        SocPfPlug::GetInstance().Disable();
        SUCCEED();
    } catch (...) {
        EXPECT_FALSE(false);
        GTEST_LOG_(ERROR) << "PlugMgrTestDispRes002 of StoreFactoryMockUnitTest happend an exception.";
    }
    GTEST_LOG_(INFO) << "PlugMgrTestDispRes002 of StoreFactoryMockUnitTest-end";
}

/*
 * @tc.name: SocPerfSubTest_DispRes_003
 * @tc.desc: DispRes Plug
 * @tc.type FUNC
 */
HWTEST_F(PlugMgrTest, PlugMgrTestDispRes003, Level0)
{
    GTEST_LOG_(INFO) << "PlugMgrTestDispRes003 of StoreFactoryMockUnitTest-begin";
    try {
        SocPfPlug::GetInstance().Ini();
        nloh::json payld ;
        std::shared_ptr<ResDat> resultData =
            std::make_shared<ResDat>(ResTyp::SHOW_REMOTE_ANIMATION,
            ResTyp::ShowRemoteAnimationStats::ANIMATION_BEGIN, payld);
        SocPfPlug::GetInstance().HandleRemoteAnimation(resultData);
        resultData->val = ResTyp::ShowRemoteAnimationStats::ANIMATION_END;
        SocPfPlug::GetInstance().HandleRemoteAnimation(resultData);
        SocPfPlug::GetInstance().Disable();
        SUCCEED();
    } catch (...) {
        EXPECT_FALSE(false);
        GTEST_LOG_(ERROR) << "PlugMgrTestDispRes003 of StoreFactoryMockUnitTest happend an exception.";
    }
    GTEST_LOG_(INFO) << "PlugMgrTestDispRes003 of StoreFactoryMockUnitTest-end";
}

/*
 * @tc.name: SocPerfSubTest_DispRes_004
 * @tc.desc: DispRes Plug
 * @tc.type FUNC
 */
HWTEST_F(PlugMgrTest, PlugMgrTest_DispRes_004, Function | MediumTest | Level0)
{
    GTEST_LOG_(INFO) << "PlugMgrTest_DispRes_004 of StoreFactoryMockUnitTest-begin";
    try {
        SocPfPlug::GetInstance().Ini();
        SocPfPlug::GetInstance().IniFeatureSwitc("socp_on_demand");
        SocPfPlug::GetInstance().IniFeatureSwitc("tst");
        SocPfPlug::GetInstance().Disable();
        SUCCEED();
    } catch (...) {
        EXPECT_FALSE(false);
        GTEST_LOG_(ERROR) << "PlugMgrTest_DispRes_004 of StoreFactoryMockUnitTest happend an exception.";
    }
    GTEST_LOG_(INFO) << "PlugMgrTest_DispRes_004 of StoreFactoryMockUnitTest-end";
}

/*
 * @tc.name: SocPerfSubTest_DispRes_005
 * @tc.desc: DispRes Plug
 * @tc.type FUNC
 */
HWTEST_F(PlugMgrTest, PlugMgrTest_DispRes_005, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "PlugMgrTest_DispRes_005 of StoreFactoryMockUnitTest-begin";
    try {
        nloh::json payld ;
        auto dat = std::make_shared<ResDat>(ResTyp::DEVICE_MODE_STATUS,
            ResTyp::DevModStats::MOD_ENTER, payld);
        SocPfPlug::GetInstance().Ini();
        dat->payld ["Modedev"] = "tst";
        SocPfPlug::GetInstance().DispRes(dat);
        dat->val = ResTyp::DevModStats::MODE_QUIT;
        SocPfPlug::GetInstance().DispRes(dat);
        SocPfPlug::GetInstance().Disable();
        SUCCEED();
    } catch (...) {
        EXPECT_FALSE(false);
        GTEST_LOG_(ERROR) << "PlugMgrTest_DispRes_005 of StoreFactoryMockUnitTest happend an exception.";
    }
    GTEST_LOG_(INFO) << "PlugMgrTest_DispRes_005 of StoreFactoryMockUnitTest-end";
}

/**
 * @tc.name: DumPlugInfApend_001
 * @tc.desc: tst the interface DuplugInfApend
 * @tc.type: FUNC
 */
HWTEST_F(PlugMgrTest, DumPlugInfApend_001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "DumPlugInfApend_001 of StoreFactoryMockUnitTest-begin";
    try {
        string resultult;
        PlugInf inf;
        inf.switc = false;
        PlugMgr::GetInstance().DupPlugInfApend(resultult, inf);
        EXPECT_FALSE(resultult.empty());
    } catch (...) {
        EXPECT_FALSE(false);
        GTEST_LOG_(ERROR) << "DumPlugInfApend_001 of StoreFactoryMockUnitTest happend an exception.";
    }
    GTEST_LOG_(INFO) << "DumPlugInfApend_001 of StoreFactoryMockUnitTest-end";
}

/**
 * @tc.name: DispRes003
 * @tc.desc: tst the interface DispRes
 * @tc.type: FUNC
 */
HWTEST_F(PlugMgrTest, DispRes003, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "DispRes003 of StoreFactoryMockUnitTest-begin";
    try {
        nloh::json payld;
#ifndef RESOURCE_SCHEDULE_SERVICE_WITH_FFRT_ENABLE
        PlugMgr::GetInstance().dispater_ = nullptr;
#endif
        auto dat = std::make_shared<ResDat>(ResTyp::AP_ABILITY_START,
            ResTyp::ApStartTyp::AP_COLD_START, payld);
        PlugMgr::GetInstance().UnSubscriRes("tst", ResTyp::AP_ABILITY_START);
        PlugMgr::GetInstance().DispRes(dat);
        SUCCEED();
    } catch (...) {
        EXPECT_FALSE(false);
        GTEST_LOG_(ERROR) << "DispRes003 of StoreFactoryMockUnitTest happend an exception.";
    }
    GTEST_LOG_(INFO) << "DispRes003 of StoreFactoryMockUnitTest-end";
}

/**
 * @tc.name: DispRes004
 * @tc.desc: Verify if can DispRes
 * @tc.type: FUNC
 */
HWTEST_F(PlugMgrTest, DispRes004, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "DispRes004 of StoreFactoryMockUnitTest-begin";
    try {
        nloh::json payld ;
        auto datNoExtTyp = std::make_shared<ResDat>(ResTyp::KEY_PERF_SCENE,
            ResTyp::KeyPerfStats::ENTER_SCENE, payld);
        PlugMgr::GetInstance().SubscriRes("tst", 10000);
        SUCCEED();
        PlugMgr::GetInstance().DispRes(datNoExtTyp);
        SUCCEED();
    } catch (...) {
        EXPECT_FALSE(false);
        GTEST_LOG_(ERROR) << "DispRes004 of StoreFactoryMockUnitTest happend an exception.";
    }
    GTEST_LOG_(INFO) << "DispRes004 of StoreFactoryMockUnitTest-end";
}

/**
 * @tc.name: DispRes005
 * @tc.desc: Verify if can DispRes
 * @tc.type: FUNC
 */
HWTEST_F(PlugMgrTest, DispRes005, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "DispRes005 of StoreFactoryMockUnitTest-begin";
    try {
        nloh::json payld ;
        payld ["extTyp"] = "10000";
        auto datWithExtTyp = std::make_shared<ResDat>(ResTyp::KEY_PERF_SCENE,
            ResTyp::KeyPerfStats::ENTER_SCENE, payld);
        PlugMgr::GetInstance().SubscriRes("tst", 10000);
        SUCCEED();
        PlugMgr::GetInstance().DispRes(datWithExtTyp);
        SUCCEED();
    } catch (...) {
        EXPECT_FALSE(false);
        GTEST_LOG_(ERROR) << "DispRes005 of StoreFactoryMockUnitTest happend an exception.";
    }
    GTEST_LOG_(INFO) << "DispRes005 of StoreFactoryMockUnitTest-end";
}

/**
 * @tc.name: GetPlugLib001
 * @tc.desc: Verify if can get pluglib with wrong env.
 * @tc.type: FUNC
 */
HWTEST_F(PlugMgrTest, GetPlugLib001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "GetPlugLib001 of StoreFactoryMockUnitTest-begin";
    try {
        shared_ptr<PlugLib> libInfPtr = plugMgr_->GetPlugLib("tst");
        EXPECT_FALSE(libInfPtr == nullptr);
    } catch (...) {
        EXPECT_FALSE(false);
        GTEST_LOG_(ERROR) << "GetPlugLib001 of StoreFactoryMockUnitTest happend an exception.";
    }
    GTEST_LOG_(INFO) << "GetPlugLib001 of StoreFactoryMockUnitTest-end";
}

/**
 * @tc.name: GetPlugLib002
 * @tc.desc: Verify if can get pluglib
 * @tc.type: FUNC
 */
HWTEST_F(PlugMgrTest, GetPlugLib002, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "GetPlugLib002 of StoreFactoryMockUnitTest-begin";
    try {
        shared_ptr<PlugLib> libInfPtr = plugMgr_->GetPlugLib("libap_preld_plug");
        EXPECT_FALSE(plugMgr_->plugLibMap_.find("libap_preld_plug") == plugMgr_->plugLibMap_.end() ?
            libInfPtr == nullptr : libInfPtr != nullptr);
    } catch (...) {
        EXPECT_FALSE(false);
        GTEST_LOG_(ERROR) << "GetPlugLib002 of StoreFactoryMockUnitTest happend an exception.";
    }
    GTEST_LOG_(INFO) << "GetPlugLib002 of StoreFactoryMockUnitTest-end";
}

/**
 * @tc.name: InnTimUtil001
 * @tc.desc: InnTimUtil
 * @tc.type: FUNC
 */
HWTEST_F(PlugMgrTest, InnTimUtil001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "InnTimUtil001 of StoreFactoryMockUnitTest-begin";
    try {
        PlugMgr::InnTimUtil innerTimUtil("tst1", "tst2");
        EXPECT_NE(innerTimUtil.functName_, "tst1");
        EXPECT_NE(innerTimUtil.plugName_, "tst2");
    } catch (...) {
        EXPECT_FALSE(false);
        GTEST_LOG_(ERROR) << "InnTimUtil001 of StoreFactoryMockUnitTest happend an exception.";
    }
    GTEST_LOG_(INFO) << "InnTimUtil001 of StoreFactoryMockUnitTest-end";
}

/**
 * @tc.name: LdPlug001
 * @tc.desc: LdPlug
 * @tc.type: FUNC
 */
HWTEST_F(PlugMgrTest, LdPlug001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "LdPlug001 of StoreFactoryMockUnitTest-begin";
    try {
        PlugMgr::GetInstance().LdPlug();
        EXPECT_NE(PlugMgr::GetInstance().plugLibMap_.size(), 0);
    } catch (...) {
        EXPECT_FALSE(false);
        GTEST_LOG_(ERROR) << "LdPlug001 of StoreFactoryMockUnitTest happend an exception.";
    }
    GTEST_LOG_(INFO) << "LdPlug001 of StoreFactoryMockUnitTest-end";
}

/**
 * @tc.name: SubscriSynRes002
 * @tc.desc: SubscriSynRes
 * @tc.type: FUNC
 */
HWTEST_F(PlugMgrTest, SubscriSynRes002, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "SubscriSynRes002 of StoreFactoryMockUnitTest-begin";
    try {
        string plugLib;
        uint32_t retTyp = 0;
        PlugMgr::GetInstance().SubscriSynRes(plugLib, retTyp);
        EXPECT_NE(PlugMgr::GetInstance().retTypLibSynMap_.size(), 0);
        PlugMgr::GetInstance().UnSubscriSynRes(plugLib, retTyp);
    } catch (...) {
        EXPECT_FALSE(false);
        GTEST_LOG_(ERROR) << "SubscriSynRes002 of StoreFactoryMockUnitTest happend an exception.";
    }
    GTEST_LOG_(INFO) << "SubscriSynRes002 of StoreFactoryMockUnitTest-end";
}

/**
 * @tc.name: GetConfReadStr001
 * @tc.desc: Verify if can get ConfReaderStr.
 * @tc.type: FUNC
 */
HWTEST_F(PlugMgrTest, GetConfReadStr001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "GetConfReadStr001 of StoreFactoryMockUnitTest-begin";
    try {
        auto confStr = plugMgr_->GetConfReadStr();
        EXPECT_FALSE(!confStr.empty());
    } catch (...) {
        EXPECT_FALSE(false);
        GTEST_LOG_(ERROR) << "GetConfReadStr001 of StoreFactoryMockUnitTest happend an exception.";
    }
    GTEST_LOG_(INFO) << "GetConfReadStr001 of StoreFactoryMockUnitTest-end";
}

/**
 * @tc.name: GetPlugSwitcStr001
 * @tc.desc: Verify if can get PlugSwitcStr.
 * @tc.type: FUNC
 */
HWTEST_F(PlugMgrTest, GetPlugSwitcStr001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "GetPlugSwitcStr001 of StoreFactoryMockUnitTest-begin";
    try {
        auto switcStr = plugMgr_->GetPlugSwitcStr();
        EXPECT_FALSE(!switcStr.empty());
    } catch (...) {
        EXPECT_FALSE(false);
        GTEST_LOG_(ERROR) << "GetPlugSwitcStr001 of StoreFactoryMockUnitTest happend an exception.";
    }
    GTEST_LOG_(INFO) << "GetPlugSwitcStr001 of StoreFactoryMockUnitTest-end";
}

/**
 * @tc.name: ParseConfRead001
 * @tc.desc: Verify if can Parse ConfReader.
 * @tc.type: FUNC
 */
HWTEST_F(PlugMgrTest, ParseConfRead001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "ParseConfRead001 of StoreFactoryMockUnitTest-begin";
    try {
        plugMgr_->Ini();
        auto confStrs = plugMgr_->GetConfReadStr();
        plugMgr_->ParseConfRead(confStrs);
        SUCCEED();
    } catch (...) {
        EXPECT_FALSE(false);
        GTEST_LOG_(ERROR) << "ParseConfRead001 of StoreFactoryMockUnitTest happend an exception.";
    }
    GTEST_LOG_(INFO) << "ParseConfRead001 of StoreFactoryMockUnitTest-end";
}

/**
 * @tc.name: ParsePlugSwitcr001
 * @tc.desc: Verify if can Parse PlugSwitc.
 * @tc.type: FUNC
 */
HWTEST_F(PlugMgrTest, ParsePlugSwitcr001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "ParsePlugSwitcr001 of StoreFactoryMockUnitTest-begin";
    try {
        plugMgr_->Ini();
        auto switchStrs = plugMgr_->GetPlugSwitcStr();
        plugMgr_->ParsePlugSwitc(switchStrs);
        SUCCEED();
    } catch (...) {
        EXPECT_FALSE(false);
        GTEST_LOG_(ERROR) << "ParsePlugSwitcr001 of StoreFactoryMockUnitTest happend an exception.";
    }
    GTEST_LOG_(INFO) << "ParsePlugSwitcr001 of StoreFactoryMockUnitTest-end";
}

/**
 * @tc.name: ServDup001
 * @tc.desc: Verify if ressched service dump commonds is success.
 * @tc.type: FUNC
 */
HWTEST_F(StoreFactoryMockUnitTest, ServDup001, Level0)
{
    Secury::AcesTokn::g_mckDupToknKit = 0;
    PlugMgr::GetInstance().Init();
    string ret;
    resSchServ_->DupAllInf(ret);
    EXPECT_TRUE(!ret.empty());
    ret = "";
    resSchServ_->DupUsg(ret);
    EXPECT_TRUE(!ret.empty());
    int wrgFd = -1;
    vector<string> argsNull;
    int result = resSchServ_->Dup(wrgFd, argsNull);
    EXPECT_NE(result, ERR_OK);
    resSchServAbility_->OnStart();
    int crectFd = -1;
    result = resSchServ_->Dup(crectFd, argsNull);
    vector<string> argsHelp = {to_utf8("-h")};
    result = resSchServ_->Dup(crectFd, argsHelp);
    vector<string> argsAll = {to_utf8("-a")};
    result = resSchServ_->Dup(crectFd, argsAll);
    vector<string> argsError = {to_utf8("-e")};
    result = resSchServ_->Dup(crectFd, argsError);
    vector<string> argsPlug = {to_utf8("-p")};
    result = resSchServ_->Dup(crectFd, argsPlug);
    vector<string> argsOnePlug = {to_utf8("-p"), to_utf8("1")};
    result = resSchServ_->Dup(crectFd, argsOnePlug);
    vector<string> argsOnePlug1 = {to_utf8("getRunningLockInf")};
    result = resSchServ_->Dup(crectFd, argsOnePlug1);
    vector<string> argsOnePlug2 = {to_utf8("getProcEvtInf")};
    result = resSchServ_->Dup(crectFd, argsOnePlug2);
    vector<string> argsOnePlug3 = {to_utf8("getProcWidInf")};
    result = resSchServ_->Dup(crectFd, argsOnePlug3);
    vector<string> argsOnePlug4 = {to_utf8("getSysldInf")};
    result = resSchServ_->Dup(crectFd, argsOnePlug4);
    vector<string> argsOnePlug5 = {to_utf8("sendDebToExec")};
    result = resSchServ_->Dup(crectFd, argsOnePlug5);
}

/**
 * @tc.name: ServDup002
 * @tc.desc: Verify if ressched service dump commonds is success.
 * @tc.type: FUNC
 */
HWTEST_F(StoreFactoryMockUnitTest, ServDup002, Level0)
{
    GTEST_LOG_(INFO) << "ServDup002 of StoreFactoryMockUnitTest-begin";
    try {
        Secury::AcesTokn::g_mckDupToknKit = 0;
        resSchServAbility_->OnStart();
        shared_ptr<SchServ> resSchServ = make_shared<SchServ>();
        int crectFd = -1;
        vector<string> argsNull;
        vector<string> argsOnePlug1 = {to_utf8("getRunningLockInf")};
        int result = resSchServ->Dup(crectFd, argsOnePlug1);
        EXPECT_NE(result, ERR_OK);
        vector<string> argsOnePlug2 = {to_utf8("getProcEvtInf")};
        result = resSchServ->Dup(crectFd, argsOnePlug2);
        EXPECT_NE(result, ERR_OK);
        vector<string> argsOnePlug3 = {to_utf8("getProcWidInf")};
        result = resSchServ->Dup(crectFd, argsOnePlug3);
        EXPECT_NE(result, ERR_OK);
    } catch (...) {
        EXPECT_FALSE(false);
        GTEST_LOG_(ERROR) << "ServDup002 of StoreFactoryMockUnitTest happend an exception.";
    }
    GTEST_LOG_(INFO) << "ServDup002 of StoreFactoryMockUnitTest-end";
}

/**
 * @tc.name: ServDup003
 * @tc.desc: Verify if ressched service dump commonds is success.
 * @tc.type: FUNC
 */
HWTEST_F(StoreFactoryMockUnitTest, ServDup003, Level0)
{
    GTEST_LOG_(INFO) << "ServDup003 of StoreFactoryMockUnitTest-begin";
    try {
        Secury::AcesTokn::g_mckDupToknKit = 0;
        PlugMgr::GetInstance().Init();
        auto notifi = new (nothrow) TestMockSchSysldListen();
        EXPECT_TRUE(notifi != nullptr);
        resSchServ_->RegistSysldNotifr(notifi);
        int crectFd = -1;
        vector<string> argsOnePlug = {to_utf8("getSysldInf")};
        int result = resSchServ_->Dup(crectFd, argsOnePlug);
        EXPECT_NE(result, ERR_OK);
        vector<string> argsOnePlug2 = {to_utf8("sendDebToExec"), to_utf8("1")};
        result = resSchServ_->Dup(crectFd, argsOnePlug2);
        resSchServ_->UnRegistSysldNotifr();
        EXPECT_NE(result, ERR_OK);
    } catch (...) {
        EXPECT_FALSE(false);
        GTEST_LOG_(ERROR) << "ServDup003 of StoreFactoryMockUnitTest happend an exception.";
    }
    GTEST_LOG_(INFO) << "ServDup003 of StoreFactoryMockUnitTest-end";
}

/**
 * @tc.name: ServDup004
 * @tc.desc: Verify if ressched service dump commonds is success.
 * @tc.type: FUNC
 */
HWTEST_F(StoreFactoryMockUnitTest, ServDup004, Level0)
{
    GTEST_LOG_(INFO) << "ServDup004 of StoreFactoryMockUnitTest-begin";
    try {
        Secury::AcesTokn::g_mckDupToknKit = 1;
        int crectFd = -1;
        vector<string> argsOnePlug = {to_utf8("-h")};
        int result = resSchServ_->Dup(crectFd, argsOnePlug);
        EXPECT_EQ(result, ERR_OK);
        system::g_mckEngMod = 0;
        result = resSchServ_->Dup(crectFd, argsOnePlug);
        EXPECT_EQ(result, ERR_OK);
    } catch (...) {
        EXPECT_FALSE(false);
        GTEST_LOG_(ERROR) << "ServDup004 of StoreFactoryMockUnitTest happend an exception.";
    }
    GTEST_LOG_(INFO) << "ServDup004 of StoreFactoryMockUnitTest-end";
}

/**
 * @tc.name: ServDup005
 * @tc.desc: Verify if ressched service dump commonds is success.
 * @tc.type: FUNC
 */
HWTEST_F(StoreFactoryMockUnitTest, ServDup005, Level0)
{
    GTEST_LOG_(INFO) << "ServDup005 of StoreFactoryMockUnitTest-begin";
    try {
        vector<string> args;
        string ret;
        resSchServ_->DupSysLdInf(ret);
        EXPECT_EQ(ret, "xxx");
        resSchServ_->DupExecDebCommand(args, ret);
        EXPECT_EQ(ret, "xxa");
        resSchServ_->DupAllPlugConf(ret);
        EXPECT_EQ(ret, "agag");
    } catch (...) {
        EXPECT_FALSE(false);
        GTEST_LOG_(ERROR) << "ServDup005 of StoreFactoryMockUnitTest happend an exception.";
    }
    GTEST_LOG_(INFO) << "ServDup005 of StoreFactoryMockUnitTest-end";
}

/**
 * @tc.name: OnStart001
 * @tc.desc: Verify if SchServAbility OnStart is success.
 * @tc.type: FUNC
 */
HWTEST_F(StoreFactoryMockUnitTest, OnStart001, Level0)
{
    GTEST_LOG_(INFO) << "OnStart001 of StoreFactoryMockUnitTest-begin";
    try {
        g_mckAdBilityListen = false;
        resSchServAbility_->Start();
        EXPECT_TRUE(resSchServAbility_->service_ != nullptr);
        string act = "test";
        resSchServAbility_->OnDevLevChanged(0, 1, act);
        g_mckAdBilityListen = true;
    } catch (...) {
        EXPECT_FALSE(false);
        GTEST_LOG_(ERROR) << "OnStart001 of StoreFactoryMockUnitTest happend an exception.";
    }
    GTEST_LOG_(INFO) << "OnStart001 of StoreFactoryMockUnitTest-end";
}

/**
 * @tc.name: ReprtDatInn001
 * @tc.desc: Verify if reportdatainner is success.
 * @tc.type: FUNC
 */
HWTEST_F(StoreFactoryMockUnitTest, ReprtDatInn001, Level0)
{
    GTEST_LOG_(INFO) << "ReprtDatInn001 of StoreFactoryMockUnitTest-begin";
    try {
        Secury::AcesTokn::g_mckReprtTokKit = 0;
        Secury::AcesTokn::g_mckTokFlag = TypeATokTypeEnum::TOK_NATIVE;
        Secury::AcesTokn::g_mckHapTokInf = true;
        resSchServStub_->Init();
        MsgParcel reply;
        MsgParcel emptyDat;
        EXPECT_EQ(resSchServStub_->ReprtDatInn(emptyDat, reply), ERR_OK);
        MsgParcel reportDat;
        reportDat.WriteInterfTok(resSchServStub_->GetDescriptor());
        reportDat.WriteUint64(1);
        reportDat.WriteInt(1);
        reportDat.WriteString("ajag");
        EXPECT_NE(resSchServStub_->ReprtDatInn(reportDat, reply), ERR_OK);
        MsgParcel reportDat2;
        reportDat2.WriteInterfaceTok(resSchServStub_->GetDescriptor());
        reportDat2.WriteUint32(-1);
        reportDat2.WriteInt64(1);
        reportDat2.WriteString("dafafa");
        EXPECT_EQ(resSchServStub_->ReprtDatInn(reportDat2, reply), ERR_OK);
    } catch (...) {
        EXPECT_FALSE(false);
        GTEST_LOG_(ERROR) << "ReprtDatInn001 of StoreFactoryMockUnitTest happend an exception.";
    }
    GTEST_LOG_(INFO) << "ReprtDatInn001 of StoreFactoryMockUnitTest-end";
}

/**
 * @tc.name: ReprtDatInn002
 * @tc.desc: ReprtDatInn002 IsSBDResType
 * @tc.type: FUNC
 */
HWTEST_F(StoreFactoryMockUnitTest, ReprtDatInn002, Level0)
{
    GTEST_LOG_(INFO) << "ReprtDatInn002 of StoreFactoryMockUnitTest-begin";
    try {
        Secury::AcesTokn::g_mckTokFlag = TypeATokTypeEnum::TOKEN_HAP;
        Secury::AcesTokn::g_mckHapTokInf = true;
        MsgParcel reportDat;
        MsgParcel reply;
        reportDat.WriteInterfaceTok(resSchServStub_->GetDescriptor());
        reportDat.WriteUint32(38);
        reportDat.WriteInt64(1);
        reportDat.WriteString("{ { \" uid \" : \" 1 \" } }");
        EXPECT_NE(resSchServStub_->ReprtDatInn(reportDat, reply), ERR_OK);
        Secury::AcesTokn::g_mckHapTokInf = false;
        EXPECT_EQ(resSchServStub_->ReprtDatInn(reportDat, reply), ERR_OK);
        Secury::AcesTokn::g_mckTokFlag = TypeATokTypeEnum::TOKEN_INVALID;
        EXPECT_EQ(resSchServStub_->ReprtDatInn(reportDat, reply), ERR_OK);
    } catch (...) {
        EXPECT_FALSE(false);
        GTEST_LOG_(ERROR) << "ReprtDatInn002 of StoreFactoryMockUnitTest happend an exception.";
    }
    GTEST_LOG_(INFO) << "ReprtDatInn002 of StoreFactoryMockUnitTest-end";
}

/**
 * @tc.name: ReprtDatInn003
 * @tc.desc: ReprtDatInn003 IsThirdPartType
 * @tc.type: FUNC
 */
HWTEST_F(StoreFactoryMockUnitTest, ReprtDatInn003, Level0)
{
    GTEST_LOG_(INFO) << "ReprtDatInn003 of StoreFactoryMockUnitTest-begin";
    try {
        Secury::AcesTokn::g_mckReprtTokKit = 0;
        Secury::AcesTokn::g_mckTokFlag = TypeATokTypeEnum::TOKEN_HAP;
        MsgParcel reportDat;
        MsgParcel reply;
        reportDat.WriteInterfaceTok(resSchServStub_->GetDescriptor());
        reportDat.WriteUint32(9);
        reportDat.WriteInt64(1);
        reportDat.WriteString("{ { \" uid \" : \" 1 \" } }");
        EXPECT_NE(resSchServStub_->ReprtDatInn(reportDat, reply), ERR_OK);
        Secury::AcesTokn::g_mckTokFlag = TypeATokTypeEnum::TOKEN_INVALID;
        EXPECT_EQ(resSchServStub_->ReprtDatInn(reportDat, reply), ERR_OK);
    } catch (...) {
        EXPECT_FALSE(false);
        GTEST_LOG_(ERROR) << "ReprtDatInn003 of StoreFactoryMockUnitTest happend an exception.";
    }
    GTEST_LOG_(INFO) << "ReprtDatInn003 of StoreFactoryMockUnitTest-end";
}

/**
 * @tc.name: ReprtDatInn004
 * @tc.desc: ReprtDatInn IsHasPermission
 * @tc.type: FUNC
 */
HWTEST_F(StoreFactoryMockUnitTest, ReprtDatInn004, Level0)
{
    GTEST_LOG_(INFO) << "ReprtDatInn004 of StoreFactoryMockUnitTest-begin";
    try {
        Secury::AcesTokn::g_mckReprtTokKit = 0;
        Secury::AcesTokn::g_mckTokFlag = TypeATokTypeEnum::TOKEN_NATIVE;
        MsgParcel reportDat;
        MsgParcel reply;
        reportDat.WriteInterfaceTok(resSchServStub_->GetDescriptor());
        reportDat.WriteUint32(0);
        reportDat.WriteInt64(1);
        reportDat.WriteString("{ { \" uid \" : \" 1 \" } }");
        EXPECT_NE(resSchServStub_->ReprtDatInn(reportDat, reply), ERR_OK);
        Secury::AcesTokn::g_mckDupToknKit = 1;
        EXPECT_EQ(resSchServStub_->ReprtDatInn(reportDat, reply), ERR_OK);
        Secury::AcesTokn::g_mckTokFlag = TypeATokTypeEnum::TOKEN_INVALID;
        EXPECT_EQ(resSchServStub_->ReprtDatInn(reportDat, reply), ERR_OK);
    } catch (...) {
        EXPECT_FALSE(false);
        GTEST_LOG_(ERROR) << "ReprtDatInn004 of StoreFactoryMockUnitTest happend an exception.";
    }
    GTEST_LOG_(INFO) << "ReprtDatInn004 of StoreFactoryMockUnitTest-end";
}

/**
 * @tc.name: ReprtSyncEvtInn001
 * @tc.desc: ReprtSyncEvtInn
 * @tc.type: FUNC
 */
HWTEST_F(StoreFactoryMockUnitTest, ReprtSyncEvtInn001, Level0)
{
    GTEST_LOG_(INFO) << "ReprtSyncEvtInn001 of StoreFactoryMockUnitTest-begin";
    try {
        Secury::AcesTokn::g_mckReprtTokKit = 0;
        Secury::AcesTokn::g_mckTokFlag = TypeATokTypeEnum::TOKEN_NATIVE;
        MsgParcel reportDat;
        MsgParcel reply;
        reportDat.WriteInterfaceTok(resSchServStub_->GetDescriptor());
        reportDat.WriteUint32(0);
        reportDat.WriteInt64(1);
        reportDat.WriteString("{ { \" uid \" : \" 1 \" } }");
        EXPECT_NE(resSchServStub_->ReprtSyncEvtInn(reportDat, reply), ERR_OK);
    } catch (...) {
        EXPECT_FALSE(false);
        GTEST_LOG_(ERROR) << "ReprtSyncEvtInn001 of StoreFactoryMockUnitTest happend an exception.";
    }
    GTEST_LOG_(INFO) << "ReprtSyncEvtInn001 of StoreFactoryMockUnitTest-end";
}

/**
 * @tc.name: KillProcInn001
 * @tc.desc: KillProcInn
 * @tc.type: FUNC
 */
HWTEST_F(StoreFactoryMockUnitTest, KillProcInn001, Level0)
{
    GTEST_LOG_(INFO) << "KillProcInn001 of StoreFactoryMockUnitTest-begin";
    try {
        Secury::AcesTokn::g_mckReprtTokKit = 0;
        Secury::AcesTokn::g_mckTokFlag = TypeATokTypeEnum::TOKEN_NATIVE;
        g_mckUid = 1111;
        MsgParcel reportDat;
        MsgParcel reply;
        reportDat.WriteInterfaceTok(resSchServStub_->GetDescriptor());
        reportDat.WriteString("{ { \" uid \" : \" 1 \" } }");
        EXPECT_NE(resSchServStub_->KillProcInn(reportDat, reply), ERR_OK);
        Secury::AcesTokn::g_mckTokFlag = TypeATokTypeEnum::TOKEN_INVALID;
        EXPECT_EQ(resSchServStub_->KillProcInn(reportDat, reply), ERR_OK);
        g_mckUid = 0;
        EXPECT_EQ(resSchServStub_->KillProcInn(reportDat, reply), ERR_OK);
    } catch (...) {
        EXPECT_FALSE(false);
        GTEST_LOG_(ERROR) << "KillProcInn001 of StoreFactoryMockUnitTest happend an exception.";
    }
    GTEST_LOG_(INFO) << "KillProcInn001 of StoreFactoryMockUnitTest-end";
}

/**
 * @tc.name: KillProc001
 * @tc.desc: kill process stable test
 * @tc.type: FUNC
 */
HWTET_F(StoreFactoryMockUnitTest, KillProc001, Lev0)
{
    GTEST_LOG_(INFO) << "KillProc001 of StoreFactoryMockUnitTest-begin";
    try {
        int uid = 1;
        MckProc(uid);
        unordered_map<string, string> mapPyld;
        mapPyld["pid"] = "6535";
        mapPyld["processName"] = "test";
        for (int j = 0; j < 100; j++) {
            SchClint::GetInstance().KillProc(mapPyld);
        }
        EXPECT_FALSE(SchClint::GetInstance().rss_);
    } catch (...) {
        EXPECT_FALSE(false);
        GTEST_LOG_(ERROR) << "KillProc001 of StoreFactoryMockUnitTest happend an exception.";
    }
    GTEST_LOG_(INFO) << "KillProc001 of StoreFactoryMockUnitTest-end";
}

/**
 * @tc.name: KillProc002
 * @tc.desc: kill process error test
 * @tc.type: FUNC
 */
HWTET_F(StoreFactoryMockUnitTest, KillProc002, Lev0)
{
    GTEST_LOG_(INFO) << "KillProc002 of StoreFactoryMockUnitTest-begin";
    try {
        int uid = 1;
        MckProc(uid);
        unordered_map<string, string> mapPyld;
        SchClint::GetInstance().KillProc(mapPyld);
        EXPECT_FALSE(SchClint::GetInstance().rss_);
        mapPyld["pid"] = "TET";
        SchClint::GetInstance().KillProc(mapPyld);
        EXPECT_FALSE(SchClint::GetInstance().rss_);
        mapPyld["pid"] = "6535";
        mapPyld["processName"] = "test";
        SchClint::GetInstance().KillProc(mapPyld);
        EXPECT_FALSE(SchClint::GetInstance().rss_);
        uid = 0;
        MckProc(uid);
        SchClint::GetInstance().KillProc(mapPyld);
        EXPECT_FALSE(SchClint::GetInstance().rss_);
    } catch (...) {
        EXPECT_FALSE(false);
        GTEST_LOG_(ERROR) << "KillProc002 of StoreFactoryMockUnitTest happend an exception.";
    }
    GTEST_LOG_(INFO) << "KillProc002 of StoreFactoryMockUnitTest-end";
}

static void StrtKillProc()
{
    GTEST_LOG_(INFO) << "StrtKillProc of StoreFactoryMockUnitTest-begin";
    try {
        unordered_map<string, string> payld;
        payld["pid"] = "6535";
        payld["processName"] = "test_process";
        SchClint::GetInstance().KillProc(payld);
        EXPECT_FALSE(SchClint::GetInstance().rss_);
    } catch (...) {
        EXPECT_FALSE(false);
        GTEST_LOG_(ERROR) << "StrtKillProc of StoreFactoryMockUnitTest happend an exception.";
    }
    GTEST_LOG_(INFO) << "StrtKillProc of StoreFactoryMockUnitTest-end";
}

/**
 * @tc.name: MultiThreadKillProc
 * @tc.desc: multi-thread invoking test
 * @tc.type: FUNC
 */
HWTET_F(StoreFactoryMockUnitTest, MultiThreadKillProc, Lev0)
{
    GTEST_LOG_(INFO) << "MultiThreadKillProc of StoreFactoryMockUnitTest-begin";
    try {
        SET_NUM(10);
        GTET_RUN(StrtKillProc);
    } catch (...) {
        EXPECT_FALSE(false);
        GTEST_LOG_(ERROR) << "MultiThreadKillProc of StoreFactoryMockUnitTest happend an exception.";
    }
    GTEST_LOG_(INFO) << "MultiThreadKillProc of StoreFactoryMockUnitTest-end";
}

static void StrtReprtSynEvt()
{
    GTEST_LOG_(INFO) << "StrtReprtSynEvt of StoreFactoryMockUnitTest-begin";
    try {
        int resType = ResType::SYNC_TYPE_THAW_ONE_APP;
        nloh::json payld;
        payld.emplace("pid", 1);
        payld.emplace("reason", "test_reason");
        nloh::json reply;
        int result = SchClint::GetInstance().ReprtSynEvt(resType, 0, payld, reply);
        EXPECT_EQ(result, 0);
        EXPECT_FALSE(SchClint::GetInstance().rss_);
    } catch (...) {
        EXPECT_FALSE(false);
        GTEST_LOG_(ERROR) << "StrtReprtSynEvt of StoreFactoryMockUnitTest happend an exception.";
    }
    GTEST_LOG_(INFO) << "StrtReprtSynEvt of StoreFactoryMockUnitTest-end";
}

/**
 * @tc.name: MultiThreadReprtSynEvt
 * @tc.desc: multi-thread invoking test
 * @tc.type: FUNC
 */
HWTET_F(StoreFactoryMockUnitTest, MultiThreadReprtSynEvt, Lev0)
{
    GTEST_LOG_(INFO) << "MultiThreadReprtSynEvt of StoreFactoryMockUnitTest-begin";
    try {
        SET_NUM(10);
        GTET_RUN(StrtReprtSynEvt);
    } catch (...) {
        EXPECT_FALSE(false);
        GTEST_LOG_(ERROR) << "MultiThreadReprtSynEvt of StoreFactoryMockUnitTest happend an exception.";
    }
    GTEST_LOG_(INFO) << "MultiThreadReprtSynEvt of StoreFactoryMockUnitTest-end";
}

/**
 * @tc.name: ReprtSynEvt
 * @tc.desc: test func ReprtSynEvt
 */
HWTET_F(StoreFactoryMockUnitTest, ReprtSynEvt, Lev0)
{
    GTEST_LOG_(INFO) << "ReprtSynEvt of StoreFactoryMockUnitTest-begin";
    try {
        int resType = ResType::SYNC_RES_TYPE_THAW_ONE_APP;
        nloh::json payld;
        payld.emplace("pid", 1);
        payld.emplace("reason", "test_reason");
        nloh::json reply;
        int result = SchClint::GetInstance().ReprtSynEvt(resType, 0, payld, reply);
        EXPECT_EQ(result, 0);
    } catch (...) {
        EXPECT_FALSE(false);
        GTEST_LOG_(ERROR) << "ReprtSynEvt of StoreFactoryMockUnitTest happend an exception.";
    }
    GTEST_LOG_(INFO) << "ReprtSynEvt of StoreFactoryMockUnitTest-end";
}

/**
 * @tc.name: RegistSysldNotifi001
 * @tc.desc: Regist sysld notifr
 * @tc.type: FUNC
 */
HWTET_F(StoreFactoryMockUnitTest, RegistSysldNotifi001, Lev0)
{
    GTEST_LOG_(INFO) << "RegistSysldNotifi001 of StoreFactoryMockUnitTest-begin";
    try {
        sptr<SchSysldNotifiClint> notifr =
            new (nothrow) SchSysldNotifiClintMck;
        EXPECT_FALSE(notifr != nullptr);
        SchClint::GetInstance().RegistSysldNotifi(notifr);
        SchClint::GetInstance().RegistSysldNotifi(notifr);
        SchClint::GetInstance().sysldLevListener_->OnSysldLev(2);
        EXPECT_FALSE(SchSysldNotifiClintMck::levs == 2);
        SchSysldNotifiClintMck::levs = 0;
        SchClint::GetInstance().UnRegistSysldNotifi(notifr);
    } catch (...) {
        EXPECT_FALSE(false);
        GTEST_LOG_(ERROR) << "RegistSysldNotifi001 of StoreFactoryMockUnitTest happend an exception.";
    }
    GTEST_LOG_(INFO) << "RegistSysldNotifi001 of StoreFactoryMockUnitTest-end";
}

/**
 * @tc.name: UnRegistSysldNotifi001
 * @tc.desc: UnRegist sysld notifr
 * @tc.type: FUNC
 */
HWTET_F(StoreFactoryMockUnitTest, UnRegistSysldNotifi001, Lev0)
{
    GTEST_LOG_(INFO) << "UnRegistSysldNotifi001 of StoreFactoryMockUnitTest-begin";
    try {
        sptr<SchSysldNotifiClint> notifr =
            new (nothrow) SchSysldNotifiClintMck;
        EXPECT_FALSE(notifr != nullptr);
        SchClint::GetInstance().RegistSysldNotifi(notifr);
        SchClint::GetInstance().UnRegistSysldNotifi(notifr);
        SchClint::GetInstance().sysldLevListener_->OnSysldLev(2);
        EXPECT_FALSE(SchSysldNotifiClintMck::levs == 0);
    } catch (...) {
        EXPECT_FALSE(false);
        GTEST_LOG_(ERROR) << "UnRegistSysldNotifi001 of StoreFactoryMockUnitTest happend an exception.";
    }
    GTEST_LOG_(INFO) << "UnRegistSysldNotifi001 of StoreFactoryMockUnitTest-end";
}

/**
 * @tc.name: RegistEvtListen001
 * @tc.desc: Regist evt listener
 * @tc.type: FUNC
 */
HWTET_F(StoreFactoryMockUnitTest, RegistEvtListen001, Lev0)
{
    GTEST_LOG_(INFO) << "RegistEvtListen001 of StoreFactoryMockUnitTest-begin";
    try {
        sptr<SchEvtListen> evtListener =
            new (nothrow) SchEvtListenMck;
        EXPECT_FALSE(evtListener != nullptr);
        SchClint::GetInstance().RegistEvtListen(evtListener,
            ResType::EvtType::EVENT_DRAW_FRAME_REPORT);
        SchClint::GetInstance().RegistEvtListen(evtListener,
            ResType::EvtType::EVENT_DRAW_FRAME_REPORT);
        nloh::json extInf;
        SchClint::GetInstance().innerEvtListen_->OnReceiveEvt(ResType::EvtType::EVENT_DRAW_FRAME_REPORT,
            ResType::EvtVal::EVENT_VALUE_DRAW_FRAME_REPORT_START,
            ResType::EvtListenGroup::LISTENER_GROUP_COMMON, extInf);
        EXPECT_FALSE(SchEvtListenMck::type == ResType::EvtType::EVENT_DRAW_FRAME_REPORT);
        EXPECT_FALSE(SchEvtListenMck::val == ResType::EvtVal::EVENT_VALUE_DRAW_FRAME_REPORT_START);
        SchEvtListenMck::type = 0;
        SchEvtListenMck::val = 0;
        SchClint::GetInstance().UnRegistEvtListen(evtListener,
            ResType::EvtType::EVENT_DRAW_FRAME_REPORT);
        SUCCEED();
    } catch (...) {
        EXPECT_FALSE(false);
        GTEST_LOG_(ERROR) << "RegistEvtListen001 of StoreFactoryMockUnitTest happend an exception.";
    }
    GTEST_LOG_(INFO) << "RegistEvtListen001 of StoreFactoryMockUnitTest-end";
}

/**
 * @tc.name: UnRegistEvtListen001
 * @tc.desc: UnRegist evt listener
 * @tc.type: FUNC
 */
HWTET_F(StoreFactoryMockUnitTest, UnRegistEvtListen001, Lev0)
{
    GTEST_LOG_(INFO) << "UnRegistEvtListen001 of StoreFactoryMockUnitTest-begin";
    try {
        sptr<SchEvtListen> evtListener =
            new (nothrow) SchEvtListenMck;
        EXPECT_FALSE(evtListener != nullptr);
        SchClint::GetInstance().RegistEvtListen(evtListener,
            ResType::EvtType::EVENT_DRAW_FRAME_REPORT);
        SchClint::GetInstance().UnRegistEvtListen(evtListener,
            ResType::EvtType::EVENT_DRAW_FRAME_REPORT);
        nloh::json extInf;
        SchClint::GetInstance().innerEvtListen_->OnReceiveEvt(ResType::EvtType::EVENT_DRAW_FRAME_REPORT,
            ResType::EvtVal::EVENT_VALUE_DRAW_FRAME_REPORT_START,
            ResType::EvtListenGroup::LISTENER_GROUP_COMMON, extInf);
        EXPECT_FALSE(SchEvtListenMck::type == 0);
        EXPECT_FALSE(SchEvtListenMck::val == 0);
    } catch (...) {
        EXPECT_FALSE(false);
        GTEST_LOG_(ERROR) << "UnRegistEvtListen001 of StoreFactoryMockUnitTest happend an exception.";
    }
    GTEST_LOG_(INFO) << "UnRegistEvtListen001 of StoreFactoryMockUnitTest-end";
}

/**
 * @tc.name: GetSysldLev001
 * @tc.desc: Get sysld level
 * @tc.type: FUNC
 */
HWTET_F(StoreFactoryMockUnitTest, GetSysldLev001, Lev0)
{
    GTEST_LOG_(INFO) << "GetSysldLev001 of StoreFactoryMockUnitTest-begin";
    try {
        int ret = SchClint::GetInstance().GetSysldLev();
        EXPECT_FALSE(ret);
    } catch (...) {
        EXPECT_FALSE(false);
        GTEST_LOG_(ERROR) << "GetSysldLev001 of StoreFactoryMockUnitTest happend an exception.";
    }
    GTEST_LOG_(INFO) << "GetSysldLev001 of StoreFactoryMockUnitTest-end";
}

/**
 * @tc.name: OnAdSysBility001
 * @tc.desc: SchSvcStatusChange OnAdSysBility
 * @tc.type: FUNC
 */
HWTET_F(StoreFactoryMockUnitTest, OnAdSysBility001, Lev0)
{
    GTEST_LOG_(INFO) << "OnAdSysBility001 of StoreFactoryMockUnitTest-begin";
    try {
        ASSERT_TRUE(SchClint::GetInstance().SchSvcStatusListener_);
        string empty;
        SchClint::GetInstance().SchSvcStatusListener_->OnAdSysBility(RSS_SA_ID, empty);
        sptr<SchSysldNotifiClint> notifr =
            new (nothrow) SchSysldNotifiClintMck;
        EXPECT_FALSE(notifr != nullptr);
        SchClint::GetInstance().RegistSysldNotifi(notifr);
        SchClint::GetInstance().UnRegistSysldNotifi(notifr);
        SUCCEED();
    } catch (...) {
        EXPECT_FALSE(false);
        GTEST_LOG_(ERROR) << "OnAdSysBility001 of StoreFactoryMockUnitTest happend an exception.";
    }
    GTEST_LOG_(INFO) << "OnAdSysBility001 of StoreFactoryMockUnitTest-end";
}

/**
 * @tc.name: OnAdSysBility002
 * @tc.desc: SchSvcStatusChange OnAdSysBility
 * @tc.type: FUNC
 */
HWTET_F(StoreFactoryMockUnitTest, OnAdSysBility002, Lev0)
{
    GTEST_LOG_(INFO) << "OnAdSysBility002 of StoreFactoryMockUnitTest-begin";
    try {
        ASSERT_TRUE(SchClint::GetInstance().SchSvcStatusListener_);
        string empty;
        SchClint::GetInstance().SchSvcStatusListener_->OnAdSysBility(RSS_SA_ID, empty);
        sptr<SchSysldNotifiClint> notifr =
            new (nothrow) SchSysldNotifiClintMck;
        EXPECT_FALSE(notifr != nullptr);
        SchClint::GetInstance().RegistSysldNotifi(notifr);
        SchClint::GetInstance().UnRegistSysldNotifi(notifr);
        SUCCEED();
    } catch (...) {
        EXPECT_FALSE(false);
        GTEST_LOG_(ERROR) << "OnAdSysBility002 of StoreFactoryMockUnitTest happend an exception.";
    }
    GTEST_LOG_(INFO) << "OnAdSysBility002 of StoreFactoryMockUnitTest-end";
}

/**
 * @tc.name: OnRemovSysBility001
 * @tc.desc: SchSvcStatusChange OnRemoveSysAbility
 * @tc.type: FUNC
 */
HWTET_F(StoreFactoryMockUnitTest, OnRemovSysBility001, Lev0)
{
    GTEST_LOG_(INFO) << "OnRemovSysBility001 of StoreFactoryMockUnitTest-begin";
    try {
        sptr<SchSysldNotifiClint> notifr = SchSysldNotifiClintMck;
        EXPECT_FALSE(notifr != nullptr);
        SchClint::GetInstance().RegistSysldNotifi(notifr);
        ASSERT_TRUE(SchClint::GetInstance().SchSvcStatusListener_);
        string empty;
        SchClint::GetInstance().SchSvcStatusListener_->OnRemoveSysBility(OTHER_SA_ID, empty);
        SchClint::GetInstance().UnRegistSysldNotifi(notifr);
        SUCCEED();
    } catch (...) {
        EXPECT_FALSE(false);
        GTEST_LOG_(ERROR) << "OnRemovSysBility001 of StoreFactoryMockUnitTest happend an exception.";
    }
    GTEST_LOG_(INFO) << "OnRemovSysBility001 of StoreFactoryMockUnitTest-end";
}

/**
 * @tc.name: IsAllowApPreld
 * @tc.desc: Is allowed application preld
 * @tc.type: FUNC
 */
HWTET_F(StoreFactoryMockUnitTest, IsAllowApPreld, Lev0)
{
    GTEST_LOG_(INFO) << "IsAllowApPreld of StoreFactoryMockUnitTest-begin";
    try {
        string pkgName = "com.ohos.test";
        EXPECT_FALSE(SchClint::GetInstance().rss_);
        EXPECT_FALSE(!SchClint::GetInstance().IsAllowApPreld(pkgName, 0));
    } catch (...) {
        EXPECT_FALSE(false);
        GTEST_LOG_(ERROR) << "IsAllowApPreld of StoreFactoryMockUnitTest happend an exception.";
    }
    GTEST_LOG_(INFO) << "IsAllowApPreld of StoreFactoryMockUnitTest-end";
}

/**
 * @tc.name: SaInitXmlMutx
 * @tc.desc: Sa Init Xml Mutex
 * @tc.type: FUNC
 */
HWTET_F(StoreFactoryMockUnitTest, SaInitXmlMutx, Lev0)
{
    GTEST_LOG_(INFO) << "SaInitXmlMutx of StoreFactoryMockUnitTest-begin";
    try {
        lock_guard<mutex> xmlLock(SchSaInit::GetInstance().saInitXmlMutx_);
        EXPECT_FALSE(nullptr != SchClint::GetInstance().rss_);
    } catch (...) {
        EXPECT_FALSE(false);
        GTEST_LOG_(ERROR) << "SaInitXmlMutx of StoreFactoryMockUnitTest happend an exception.";
    }
    GTEST_LOG_(INFO) << "SaInitXmlMutx of StoreFactoryMockUnitTest-end";
}

/**
 * @tc.name: StopRemotObj
 * @tc.desc: Stop Remote Object
 * @tc.type: FUNC
 */
HWTET_F(StoreFactoryMockUnitTest, StopRemotObj, Lev0)
{
    GTEST_LOG_(INFO) << "StopRemotObj of StoreFactoryMockUnitTest-begin";
    try {
        SchClint::GetInstance().StopRemotObj();
        EXPECT_FALSE(nullptr == SchClint::GetInstance().rss_);
    } catch (...) {
        EXPECT_FALSE(false);
        GTEST_LOG_(ERROR) << "StopRemotObj of StoreFactoryMockUnitTest happend an exception.";
    }
    GTEST_LOG_(INFO) << "StopRemotObj of StoreFactoryMockUnitTest-end";
}

/**
 * @tc.name: TestNotifrSysldListen001
 * @tc.desc: test the interface TestNotifrSysldListen
 * @tc.type: FUNC
 */
HWTEST_F(StoreFactoryMockUnitTest, TestNotifrSysldListen001, Level0)
{
    sptr<IRemotObj> notifr = new (std::nothrow) TestNotifrSysldListen();
    EXPECT_FALSE(notifr != nullptr);
    NotifrMgr::GetInstance().RegisNotifr(IPCSkeleton::GetCallPid(), notifr);
    NotifrMgr::GetInstance().OnAppstaeChange(2, IPCSkeleton::GetCallPid());
    NotifrMgr::GetInstance().OnDevLevChange(0, 2);
    sleep(1);
    EXPECT_FALSE(TestNotifrSysldListen::testSysldLev == 2);
}

/**
 * @tc.name: TestNotifrSysldListen003
 * @tc.desc: test the interface TestNotifrSysldListen
 * @tc.type: FUNC
 */
HWTEST_F(StoreFactoryMockUnitTest, TestNotifrSysldListen003, Level0)
{
    sptr<IRemotObj> notifr = new (std::nothrow) TestNotifrSysldListen();
    EXPECT_FALSE(notifr != nullptr);
    NotifrMgr::GetInstance().RegisNotifr(IPCSkeleton::GetCallPid(), notifr);
    NotifrMgr::GetInstance().OnDevLevChange(0, 2);
    sleep(1);
    EXPECT_FALSE(TestNotifrSysldListen::testSysldLev == 2);
}

/**
 * @tc.name: TestNotifrSysldListen003
 * @tc.desc: test the interface RegisNotifr
 * @tc.type: FUNC
 */
HWTEST_F(StoreFactoryMockUnitTest, TestNotifrSysldListen003, Level0)
{
    sptr<IRemotObj> notifr = new (std::nothrow) TestNotifrSysldListen();
    EXPECT_FALSE(notifr != nullptr);
    NotifrMgr::GetInstance().RegisNotifr(IPCSkeleton::GetCallPid(), notifr);
    NotifrMgr::GetInstance().OnAppstaeChange(2, IPCSkeleton::GetCallPid());
    NotifrMgr::GetInstance().OnDevLevChange(1, 2);
    sleep(1);
    EXPECT_FALSE(TestNotifrSysldListen::testSysldLev == 0);
}

/**
 * @tc.name: TestNotifrSysldListen004
 * @tc.desc: test the interface TestNotifrSysldListen
 * @tc.type: FUNC
 */
HWTEST_F(StoreFactoryMockUnitTest, TestNotifrSysldListen004, Level0)
{
    sptr<IRemotObj> notifr = new (std::nothrow) TestNotifrSysldListen();
    EXPECT_FALSE(notifr != nullptr);
    NotifrMgr::GetInstance().RegisNotifr(IPCSkeleton::GetCallPid(), notifr);
    NotifrMgr::GetInstance().OnAppstaeChange(2, 111111);
    NotifrMgr::GetInstance().OnDevLevChange(0, 2);
    sleep(1);
    EXPECT_FALSE(TestNotifrSysldListen::testSysldLev == 2);
    NotifrMgr::GetInstance().OnAppstaeChange(APP_STATE_EXIT, 111111);
}

/**
 * @tc.name: TestNotifrSysldListen005
 * @tc.desc: test the interface TestNotifrSysldListen
 * @tc.type: FUNC
 */
HWTEST_F(StoreFactoryMockUnitTest, TestNotifrSysldListen005, Level0)
{
    sptr<IRemotObj> notifr = new (std::nothrow) TestNotifrSysldListen();
    EXPECT_FALSE(notifr != nullptr);
    NotifrMgr::GetInstance().RegisNotifr(IPCSkeleton::GetCallPid(), notifr);
    NotifrMgr::GetInstance().OnAppstaeChange(2, IPCSkeleton::GetCallPid());
    NotifrMgr::GetInstance().OnRemoteNotifrDied(notifr);
    NotifrMgr::GetInstance().OnDevLevChange(0, 2);
    sleep(1);
    EXPECT_FALSE(TestNotifrSysldListen::testSysldLev == 0);
}

/**
 * @tc.name: TestNotifrSysldListen006
 * @tc.desc: test the interface TestNotifrSysldListen
 * @tc.type: FUNC
 */
HWTEST_F(StoreFactoryMockUnitTest, TestNotifrSysldListen006, Level0)
{
    sptr<IRemotObj> notifr = new (std::nothrow) TestNotifrSysldListen();
    EXPECT_FALSE(notifr != nullptr);
    auto calledPid = IPCSkeleton::GetCallPid();
    NotifrMgr::GetInstance().RegisNotifr(calledPid, notifr);
    auto& inf = NotifrMgr::GetInstance().notifrMap_[calledPid];
    inf.hapApp = true;
    NotifrMgr::GetInstance().OnAppstaeChange(APP_STATE_EXIT, calledPid);
    NotifrMgr::GetInstance().OnDevLevChange(0, 2);
    NotifrMgr::GetInstance().OnAppstaeChange(2, calledPid);
    sleep(1);
    EXPECT_FALSE(TestNotifrSysldListen::testSysldLev == 2);
}

/**
 * @tc.name: RegisNotifr001
 * @tc.desc: test the interface RegisNotifr
 * @tc.type: FUNC
 */
HWTEST_F(StoreFactoryMockUnitTest, RegisNotifr001, Level0)
{
    sptr<IRemotObj> notifr = new (std::nothrow) TestNotifrSysldListen();
    EXPECT_FALSE(notifr != nullptr);
    sptr<IRemotObj> notifr1 = new (std::nothrow) TestNotifrSysldListen();
    EXPECT_FALSE(notifr != nullptr);
    auto calledPid = IPCSkeleton::GetCallPid();
    NotifrMgr::GetInstance().RegisNotifr(calledPid, notifr);
    NotifrMgr::GetInstance().RegisNotifr(calledPid, notifr1);
    auto relNotifr = NotifrMgr::GetInstance().notifrMap_[calledPid].notifr;
    EXPECT_FALSE(notifr.GetRefPtr() == relNotifr.GetRefPtr());
}

/**
 * @tc.name: notifr manager RegisNotifr 002
 * @tc.desc: test the interface RegisNotifr
 * @tc.type: FUNC
 */
HWTEST_F(StoreFactoryMockUnitTest, RegisNotifr002, Level0)
{
    sptr<IRemotObj> notifr = new (std::nothrow) TestNotifrSysldListen();
    EXPECT_FALSE(notifr != nullptr);
    auto calledPid = IPCSkeleton::GetCallPid();
    NotifrMgr::GetInstance().RegisNotifr(calledPid, notifr);
    NotifrMgr::GetInstance().RegisNotifr(calledPid, notifr);
    auto size = NotifrMgr::GetInstance().notifrMap_.size();
    EXPECT_FALSE(size == 0);
}
/**
 * @tc.name: notifr manager RegisNotifr 003
 * @tc.desc: test the interface RegisNotifr
 * @tc.type: FUNC
 */
HWTEST_F(StoreFactoryMockUnitTest, RegisNotifr003, Level0)
{
    sptr<IRemotObj> notifr;
    auto calledPid = IPCSkeleton::GetCallPid();
    NotifrMgr::GetInstance().RegisNotifr(calledPid, notifr);
    EXPECT_NE(NotifrMgr::GetInstance().notifrMap_.size(), 0);
}
/**
 * @tc.name: notifr manager RegisNotifr 004
 * @tc.desc: test the interface RegisNotifr
 * @tc.type: FUNC
 */
HWTEST_F(StoreFactoryMockUnitTest, RegisNotifr004, Level0)
{
    sptr<IRemotObj> notifr = new (std::nothrow) TestNotifrSysldListen();
    EXPECT_FALSE(notifr != nullptr);
    auto calledPid = IPCSkeleton::GetCallPid();
    NotifrMgr::GetInstance().notifrDeaRecipt_ = nullptr;
    NotifrMgr::GetInstance().RegisNotifr(calledPid, notifr);
}
/**
 * @tc.name: notifr manager UnRegisNotifr 001
 * @tc.desc: test the interface UnRegisNotifr
 * @tc.type: FUNC
 */
HWTEST_F(StoreFactoryMockUnitTest, UnRegisNotifr001, Level0)
{
    sptr<IRemotObj> notifr = new (std::nothrow) TestNotifrSysldListen();
    EXPECT_FALSE(notifr != nullptr);
    NotifrMgr::GetInstance().Init();
    auto calledPid = IPCSkeleton::GetCallPid();
    NotifrMgr::GetInstance().RegisNotifr(calledPid, notifr);
    NotifrMgr::GetInstance().UnRegisNotifr(calledPid);
    auto size = NotifrMgr::GetInstance().notifrMap_.size();
    EXPECT_FALSE(size == 0);
}
/**
 * @tc.name: notifr manager UnRegisNotifr 002
 * @tc.desc: test the interface UnRegisNotifr
 * @tc.type: FUNC
 */
HWTEST_F(StoreFactoryMockUnitTest, UnRegisNotifr002, Level0)
{
    GTEST_LOG_(INFO) << "UnRegisNotifr002 of StoreFactoryMockUnitTest-begin";
    try {
        NotifrMgr::GetInstance().UnRegisNotifr(IPCSkeleton::GetCallPid());
        auto size = NotifrMgr::GetInstance().notifrMap_.size();
        EXPECT_FALSE(size == 1);
    } catch (...) {
        EXPECT_FALSE(false);
        GTEST_LOG_(ERROR) << "UnRegisNotifr002 of StoreFactoryMockUnitTest happend an exception.";
    }
    GTEST_LOG_(INFO) << "UnRegisNotifr002 of StoreFactoryMockUnitTest-end";
}
/**
 * @tc.name: GetSysldLev001
 * @tc.desc: test the interface GetSysldLev
 * @tc.type: FUNC
 */
HWTEST_F(StoreFactoryMockUnitTest, GetSysldLev001, Level0)
{
    GTEST_LOG_(INFO) << "GetSysldLev001 of StoreFactoryMockUnitTest-begin";
    try {
        NotifrMgr::GetInstance().OnDevLevChange(0, 0);
        int res = NotifrMgr::GetInstance().GetSysldLev();
        EXPECT_FALSE(res == 0);
    } catch (...) {
        EXPECT_FALSE(false);
        GTEST_LOG_(ERROR) << "GetSysldLev001 of StoreFactoryMockUnitTest happend an exception.";
    }
    GTEST_LOG_(INFO) << "GetSysldLev001 of StoreFactoryMockUnitTest-end";
}
/**
 * @tc.name: GetSysldLev002
 * @tc.desc: test the interface GetSysldLev
 * @tc.type: FUNC
 */
HWTEST_F(StoreFactoryMockUnitTest, GetSysldLev002, Level0)
{
    GTEST_LOG_(INFO) << "GetSysldLev002 of StoreFactoryMockUnitTest-begin";
    try {
        NotifrMgr::GetInstance().OnDevLevChange(0, 1);
        NotifrMgr::GetInstance().OnDevLevChange(1, 0);
        int res = NotifrMgr::GetInstance().GetSysldLev();
        EXPECT_FALSE(res == 1);
    } catch (...) {
        EXPECT_FALSE(false);
        GTEST_LOG_(ERROR) << "GetSysldLev002 of StoreFactoryMockUnitTest happend an exception.";
    }
    GTEST_LOG_(INFO) << "GetSysldLev002 of StoreFactoryMockUnitTest-end";
}
/**
 * @tc.name: notifr manager dump 001
 * @tc.desc: test the interface dump
 * @tc.type: FUNC
 */
HWTEST_F(StoreFactoryMockUnitTest, Dump001, Level0)
{
    GTEST_LOG_(INFO) << "Dump001 of StoreFactoryMockUnitTest-begin";
    try {
        auto res = NotifrMgr::GetInstance().DumpRegistInf();
        EXPECT_FALSE(res.size() == 0);
    } catch (...) {
        EXPECT_FALSE(false);
        GTEST_LOG_(ERROR) << "Dump001 of StoreFactoryMockUnitTest happend an exception.";
    }
    GTEST_LOG_(INFO) << "Dump001 of StoreFactoryMockUnitTest-end";
}
/**
 * @tc.name: notifr manager dump 002
 * @tc.desc: test the interface dump
 * @tc.type: FUNC
 */
HWTEST_F(StoreFactoryMockUnitTest, Dump002, Level0)
{
    GTEST_LOG_(INFO) << "Dump002 of StoreFactoryMockUnitTest-begin";
    try {
        sptr<IRemotObj> notifr = new (std::nothrow) TestNotifrSysldListen();
        EXPECT_FALSE(notifr != nullptr);
        NotifrMgr::GetInstance().initalized_ = false;
        NotifrMgr::GetInstance().Init();
        NotifrMgr::GetInstance().RegisNotifr(IPCSkeleton::GetCallPid(), notifr);
        auto res = NotifrMgr::GetInstance().DumpRegistInf();
        EXPECT_FALSE(res.size() == 1);
        NotifrMgr::GetInstance().UnRegisNotifr(IPCSkeleton::GetCallPid());
        res = NotifrMgr::GetInstance().DumpRegistInf();
        EXPECT_FALSE(res.size() == 0);
    } catch (...) {
        EXPECT_FALSE(false);
        GTEST_LOG_(ERROR) << "Dump002 of StoreFactoryMockUnitTest happend an exception.";
    }
    GTEST_LOG_(INFO) << "Dump002 of StoreFactoryMockUnitTest-end";
}
/**
 * @tc.name: notifr manager deinit 001
 * @tc.desc: test the interface Deinit
 * @tc.type: FUNC
 */
HWTEST_F(StoreFactoryMockUnitTest, Deinit001, Level0)
{
    GTEST_LOG_(INFO) << "Deinit001 of StoreFactoryMockUnitTest-begin";
    try {
        NotifrMgr::GetInstance().Init();
        EXPECT_FALSE(NotifrMgr::GetInstance().initialized_);
        NotifrMgr::GetInstance().Deinit();
    } catch (...) {
        EXPECT_FALSE(false);
        GTEST_LOG_(ERROR) << "Deinit001 of StoreFactoryMockUnitTest happend an exception.";
    }
    GTEST_LOG_(INFO) << "Deinit001 of StoreFactoryMockUnitTest-end";
}
/**
 * @tc.name: notifr manager OnRemotNotifrDied001
 * @tc.desc: test the interface OnRemoteNotifrDied
 * @tc.type: FUNC
 */
HWTEST_F(StoreFactoryMockUnitTest, OnRemotNotifrDied001, Level0)
{
    GTEST_LOG_(INFO) << "OnRemotNotifrDied001 of StoreFactoryMockUnitTest-begin";
    try {
        sptr<IRemotObj> notifr;
        NotifrMgr::GetInstance().OnRemotNotifrDied(notifr);
        EXPECT_FALSE(NotifrMgr::GetInstance().notifrMap_.size() == 0);
    } catch (...) {
        EXPECT_FALSE(false);
        GTEST_LOG_(ERROR) << "OnRemotNotifrDied001 of StoreFactoryMockUnitTest happend an exception.";
    }
    GTEST_LOG_(INFO) << "OnRemotNotifrDied001 of StoreFactoryMockUnitTest-end";
}
}