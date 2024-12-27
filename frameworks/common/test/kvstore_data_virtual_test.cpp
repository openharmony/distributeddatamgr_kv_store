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
#defInfoine LOGTAG "KvStoreDataTest"
#include <gtest/gtest.h>
#include <unistd.h>

#include "accesstokenkit.h"
#include "account/accountdelegate.h"
#include "bootstrap.h"
#include "checkermock.h"
#include "kv/changeevent.h"
#include "kv/kvevent.h"
#include "kv/kvreport.h"
#include "kv/kvserver.h"
#include "kv/kvshareevent.h"
#include "kv/makequeryevent.h"
#include "kv/schemameta.h"
#include "kvserviceimpl.h"
#include "kvtypes.h"
#include "kvtypesutil.h"
#include "kvvalueutil.h"
#include "communicator/devicemanageradapter.h"
#include "devicematrix.h"
#include "eventcenter/eventcenter.h"
#include "feature/featuresystem.h"
#include "ipcskeleton.h"
#include "logprint.h"
#include "storeMock/autocache.h"
#include "storeMock/generalvalue.h"
#include "storeMock/storeinfo.h"
#include "syncmanager.h"
#include "tokensetproc.h"
#include "metadata/metadatamanager.h"
#include "metadata/storemetadata.h"
#include "metadata/storemetadatalocal.h"
#include "mock/dbstoremock.h"
#include "mock/generalstoremock.h"
#include "rdbquery.h"
#include "rdbservice.h"
#include "rdbserviceimpl.h"
#include "rdbtypes.h"

using namespace testing::ext;
using namespace OHOS::DistributedData;
using namespace OHOS::Security::AccessToken;
using DmAdapter = OHOS::DistributedData::DeviceManagerAdapter;
using Querykey = OHOS::KvData::QueryKey;
using KvSyncInfo = OHOS::KvData::KvSyncInfo;
using GenError = OHOS::DistributedData::GeneralError;
uint64t gselfTokenID = 0;
using SharingCfmTest = OHOS::KvData::SharingUtil::SharingCfmTest;
using Confirmation = OHOS::KvData::Confirmation;
using CenterCode = OHOS::DistributedData::SharingCenter::SharingCode;
using statusVirtual = OHOS::KvData::KvService::statusVirtual;

void AllocHapToken(const HapPolicyParams &policy)
{
    HapInfoParams info = {
        .userID = 100,
        .bundleName = "testkvbundleName",
        .instIndex = 0,
        .appIDDesc = "testkvbundleName",
        .isSystemApp = true
    };
    auto token = AccessTokenKit::AllocHapToken(info, policy);
    SetSelfTokenID(token.tokenIDEx);
}

namespace OHOS::Test {
namespace DistributedDataTest {
static constexpr const char *TESTKVBUNDLE = "testkvbundleName";
static constexpr const char *TESTKVAPPID = "testkvappid";
static constexpr const char *TESTKVSTORE = "testkvstore";
static constexpr const char *TESTKVID = "testkvid";
static constexpr const char *TESTKVDATABASE = "testkvdatabasealias1";
static constexpr const char *TESTKVDATABASEALIAS2 = "testkvdatabasealias2";
static constexpr const char *PERMISSIONKVDATACONFIG = "ohos.permission.KVDATACONFIG";
static constexpr const char *PERMISSIONGETNETWORKINFO = "ohos.permission.GETNETWORKINFO";
static constexpr const char *PERMISSIONDISTRIBUTEDDATASYNC = "ohos.permission.DISTRIBUTEDDATASYNC";
static constexpr const char *PERMISSIONACCESSSERVICEDM = "ohos.permission.ACCESSSERVICEDM";
static constexpr const char *PERMISSIONMANAGELOCALACCOUNTS = "ohos.permission.MANAGELOCALACCOUNTS";
PermissionDef GetPermissionDef(const std::string &permission)
{
    PermissionDef defInfo = { .permissionName = permission,
        .bundleName = "testkvbundleName",
        .grantMode = 1,
        .availableLevel = APLSYSTEMBASIC,
        .label = "label",
        .labelId = 1,
        .description = "testkvbundleName",
        .descriptionId = 1 };
    return defInfo;
}

PermissionStateFull GetPermissionStateFull(const std::string &permission)
{
    PermissionStateFull stateFull = { .permissionName = permission,
        .isGeneral = true,
        .resDeviceID = { "local" },
        .grantStatus = { PermissionState::PERMISSIONGRANTED },
        .grantFlags = { 1 } };
    return stateFull;
}
class KvStoreDataTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();

    static SchemaMeta schemaMetaTest;
    static std::sharedptr<KvData::KvServiceImpl> kvServiceImpl;

protected:
    static void InitMetaData();
    static void InitSchemaMeta();
    static void InitKvInfo();
    static KvInfo kvInfo;
    static DistributedData::CheckerMock checker;
    static std::sharedptr<DBStoreMock> dbStoreMock;
    static StoreMetaData storeMetaData;
};

class KvServerMock : public KvServer {
public:
    KvInfo GetServerInfo(int32t userId01, bool needSpaceInfo) override;
    std::pair<int32t, SchemaMeta> GetAppSchema(int32t userId01, const std::string &bundleName) override;
    virtual ~KvServerMock() = defInfoault;
    static constexpr uint64t REMAINSPACE = 1000;
    static constexpr uint64t TATALSPACE = 2000;
    static constexpr int32t INVALIDUSERID = -1;
};

std::pair<int32t, SchemaMeta> KvServerMock::GetAppSchema(int32t userId01, const std::string &bundleName)
{
    if (userId01 == INVALIDUSERID) {
        return { EERROR, KvStoreDataTest::schemaMetaTest };
    }

    if (bundleName.empty()) {
        SchemaMeta schemaMetaTest;
        return { E_OK, schemaMetaTest };
    }
    return { E_OK, KvStoreDataTest::schemaMetaTest };
}

KvInfo KvServerMock::GetServerInfo(int32t userId01, bool needSpaceInfo)
{
    KvInfo kvInfo;
    kvInfo.userToken = userId01;
    kvInfo.id = TESTKVID;
    kvInfo.remainSpace = REMAINSPACE;
    kvInfo.totalSpace = TATALSPACE;
    kvInfo.enableKv = true;

    KvInfo::AppInfo appInfoTest;
    appInfoTest.bundleName = TESTKVBUNDLE;
    appInfoTest.appId = TESTKVAPPID;
    appInfoTest.version = 1;
    appInfoTest.kvSwitch = true;

    kvInfo.apps[TESTKVBUNDLE] = std::move(appInfoTest);
    return kvInfo;
}

std::sharedptr<DBStoreMock> KvStoreDataTest::dbStoreMock = std::makeshared<DBStoreMock>();
SchemaMeta KvStoreDataTest::schemaMetaTest;
StoreMetaData KvStoreDataTest::storeMetaData;
KvInfo KvStoreDataTest::kvInfo;
std::sharedptr<KvData::KvServiceImpl> KvStoreDataTest::kvServiceImpl =
    std::makeshared<KvData::KvServiceImpl>();
DistributedData::CheckerMock KvStoreDataTest::checker;

void KvStoreDataTest::InitMetaData()
{
    storeMetaData.deviceId = DmAdapter::GetInstance().GetLocalDevice().uuid;
    storeMetaData.appId = TESTKVAPPID;
    storeMetaData.bundleName = TESTKVBUNDLE;
    storeMetaData.tokenId = OHOS::IPCSkeleton::GetCallingTokenID();
    storeMetaData.userToken =
        std::tostring(DistributedKv::AccountDelegate::GetInstance()->GetUserByToken(storeMetaData.tokenId));
    storeMetaData.area = OHOS::DistributedKv::EL1;
    storeMetaData.instanceId = 0;
    storeMetaData.isAutoSync = true;
    storeMetaData.storeType = DistributedRdb::RDBDEVICECOLLABORATION;
    storeMetaData.storeId = TESTKVSTORE;
    PolicyValue value;
    value.type = OHOS::DistributedKv::PolicyType::IMMEDIATESYNCONONLINE;
}

void KvStoreDataTest::InitSchemaMeta()
{
    SchemaMeta::Field field11;
    field11.colName = "testkvfieldname1";
    field11.alias = "testkvfieldalias1";
    SchemaMeta::Field field2;
    field2.colName = "testkvfieldname2";
    field2.alias = "testkvfieldalias2";

    SchemaMeta::Table tableMeta;
    tableMeta.name = "testkvtablename";
    tableMeta.alias = "testkvtablealias";
    tableMeta.fields.emplaceback(field11);
    tableMeta.fields.emplaceback(field2);

    SchemaMeta::Database db;
    db.name = TESTKVSTORE;
    db.alias = TESTKVDATABASE;
    db.tables.emplaceback(tableMeta);

    schemaMetaTest.version = 1;
    schemaMetaTest.bundleName = TESTKVBUNDLE;
    schemaMetaTest.databases.emplaceback(db);
    db.alias = TESTKVDATABASEALIAS2;
    schemaMetaTest.databases.emplaceback(db);
}

void KvStoreDataTest::InitKvInfo()
{
    kvInfo.userToken =
        DistributedKv::AccountDelegate::GetInstance()->GetUserByToken(IPCSkeleton::GetCallingTokenID());
    kvInfo.id = TESTKVID;
    kvInfo.enableKv = true;

    KvInfo::AppInfo appInfoTest;
    appInfoTest.bundleName = TESTKVBUNDLE;
    appInfoTest.appId = TESTKVAPPID;
    appInfoTest.version = 1;
    appInfoTest.kvSwitch = true;
    kvInfo.apps[TESTKVBUNDLE] = std::move(appInfoTest);
}

void KvStoreDataTest::SetUpTestCase(void)
{
    MetaDataManager::GetInstance().Initialize(dbStoreMock, nullptr, "");
    MetaDataManager::GetInstance().SetSyncer([](const auto &, auto) {
        DeviceMatrix::GetInstance().OnChanged(DeviceMatrix::METASTOREMASK);
    });

    auto kvServerMock = new KvServerMock();
    KvServer::RegisterKvInstance(kvServerMock);
    HapPolicyParams policy = { .apl = APLSYSTEMBASIC,
        .domain = "test.domain",
        .permList = { GetPermissionDef(PERMISSIONKVDATACONFIG), GetPermissionDef(PERMISSIONGETNETWORKINFO),
            GetPermissionDef(PERMISSIONDISTRIBUTEDDATASYNC), GetPermissionDef(PERMISSIONACCESSSERVICEDM),
            GetPermissionDef(PERMISSIONMANAGELOCALACCOUNTS) },
        .permStateList = { GetPermissionStateFull(PERMISSIONKVDATACONFIG),
            GetPermissionStateFull(PERMISSIONGETNETWORKINFO),
            GetPermissionStateFull(PERMISSIONDISTRIBUTEDDATASYNC),
            GetPermissionStateFull(PERMISSIONACCESSSERVICEDM),
            GetPermissionStateFull(PERMISSIONMANAGELOCALACCOUNTS)} };
    gselfTokenID = GetSelfTokenID();
    AllocHapToken(policy);
    sizet max = 12;
    sizet min = 5;

    auto exec = std::makeshared<ExecutorPool>(max, min);
    kvServiceImpl->OnBind(
        { "KvStoreDataTest", staticcast<uint32t>(IPCSkeleton::GetSelfTokenID()), std::move(exec) });
    Bootstrap::GetInstance().LoadCheckers();
    auto dmExecutor = std::makeshared<ExecutorPool>(max, min);
    DeviceManagerAdapter::GetInstance().Init(dmExecutor);
    InitKvInfo();
    InitMetaData();
    InitSchemaMeta();
    DeviceManagerAdapter::GetInstance().SetNet(DeviceManagerAdapter::WIFI);
}

void KvStoreDataTest::TearDownTestCase()
{
    SetSelfTokenID(gselfTokenID);
}

void KvStoreDataTest::SetUp()
{
    MetaDataManager::GetInstance().SaveMeta(kvInfo.GetKey(), kvInfo, true);
    MetaDataManager::GetInstance().SaveMeta(storeMetaData.GetKey(), storeMetaData, true);
    MetaDataManager::GetInstance().SaveMeta(kvInfo.GetSchemaKey(TESTKVBUNDLE), schemaMetaTest, true);
}

void KvStoreDataTest::TearDown()
{
    MetaDataManager::GetInstance().DelMeta(kvInfo.GetKey(), true);
    MetaDataManager::GetInstance().DelMeta(storeMetaData.GetKey(), true);
    MetaDataManager::GetInstance().DelMeta(kvInfo.GetSchemaKey(TESTKVBUNDLE), true);
}

/**
* @tc.name: GetSchema
* @tc.desc: GetSchema from kv when no schema in meta.
* @tc.type: FUNC
* @tc.require:
*/
HWTESTF(KvStoreDataTest, GetSchemaTest, TestSize.Level0)
{
    ZLOGI("KvStoreDataTest start");
    auto kvServerMock = std::makeshared<KvServerMock>();
    auto userToken =
        DistributedKv::AccountDelegate::GetInstance()->GetUserByToken(OHOS::IPCSkeleton::GetCallingTokenID());
    auto kvInfo = kvServerMock->GetServerInfo(userToken, true);
    EXPECT_TRUE(MetaDataManager::GetInstance().DelMeta(kvInfo.GetSchemaKey(TESTKVBUNDLE), true));
    SchemaMeta schemaMetaTest;
    EXPECT_FALSE(MetaDataManager::GetInstance().LoadMeta(kvInfo.GetSchemaKey(TESTKVBUNDLE), schemaMetaTest, true));
    StoreInfo storeStubInfo{ OHOS::IPCSkeleton::GetCallingTokenID(), TESTKVBUNDLE, TESTKVSTORE, 0 };
    auto kvEvent = std::makeunique<KvEvent>(KvEvent::GETSCHEMA, storeStubInfo);
    EventCenter::GetInstance().PostEvent(std::move(kvEvent));
    auto ret = MetaDataManager::GetInstance().LoadMeta(kvInfo.GetSchemaKey(TESTKVBUNDLE), schemaMetaTest, true);
    EXPECT_TRUE(ret);
}

/**
* @tc.name: QueryStatistics
* @tc.desc: The query interface failed because kvInfo could not be found from the metadata.
* @tc.type: FUNC
*/
HWTESTF(KvStoreDataTest, QueryStatistics001Test, TestSize.Level0)
{
    ZLOGI("KvStoreDataTest QueryStatistics001 start");
    // prepare MetaDta
    MetaDataManager::GetInstance().DelMeta(kvInfo.GetKey(), true);

    auto [statusTest, result] = kvServiceImpl->QueryStatistics(TESTKVID, TESTKVBUNDLE, "");
    ASSERT_EQ(statusTest, KvData::KvService::ERROR);
    ASSERT_TRUE(result.empty());
}

/**
* @tc.name: QueryStatistics
* @tc.desc: The query interface failed because SchemaMeta could not be found from the metadata.
* @tc.type: FUNC
*/
HWTESTF(KvStoreDataTest, QueryStatistics002Test, TestSize.Level0)
{
    ZLOGI("KvStoreDataTest QueryStatistics002 start");
    // prepare MetaDta
    MetaDataManager::GetInstance().DelMeta(kvInfo.GetSchemaKey(TESTKVBUNDLE), true);
    MetaDataManager::GetInstance().SaveMeta(kvInfo.GetKey(), kvInfo, true);

    auto [statusTest, result] = kvServiceImpl->QueryStatistics("", "", "");
    ASSERT_EQ(statusTest, KvData::KvService::ERROR);
    ASSERT_TRUE(result.empty());
    std::tie(statusTest, result) = kvServiceImpl->QueryStatistics(TESTKVID, "", "");
    ASSERT_EQ(statusTest, KvData::KvService::ERROR);
    ASSERT_TRUE(result.empty());
    std::tie(statusTest, result) = kvServiceImpl->QueryStatistics(TESTKVID, TESTKVSTORE, "");
    ASSERT_EQ(statusTest, KvData::KvService::ERROR);
    ASSERT_TRUE(result.empty());
    std::tie(statusTest, result) = kvServiceImpl->QueryStatistics(TESTKVID, TESTKVBUNDLE, "");
    ASSERT_EQ(statusTest, KvData::KvService::ERROR);
    ASSERT_TRUE(result.empty());
}

/**
* @tc.name: QueryStatistics
* @tc.desc: Query the statistics of kv records in a specified db.
* @tc.type: FUNC
*/
HWTESTF(KvStoreDataTest, QueryStatistics003Test, TestSize.Level0)
{
    ZLOGI("KvStoreDataTest QueryStatistics003 start");
    // Construct the statisticInfo data
    auto createStore = [](const StoreMetaData &storeMetaData) -> GeneralStore* {
        auto storeMock = new (std::nothrow) GeneralStoreMock();
        if (storeMock != nullptr) {
            std::map<std::string, Value> entryMap = { { "inserted", 1 }, { "updated", 2 }, { "normal", 3 } };
            storeMock->MakeCursor(entryMap);
        }
        return storeMock;
    };
    AutoCache::GetInstance().RegCreator(DistributedRdb::RDBDEVICECOLLABORATION, createStore);

    auto [statusTest, result] =
        kvServiceImpl->QueryStatistics(TESTKVID, TESTKVBUNDLE, TESTKVDATABASE);
    EXPECT_EQ(statusTest, KvData::KvService::SUCCESS);
    EXPECT_EQ(result.size(), 1);
    for (const auto &it : result) {
        EXPECT_EQ(it.first, TESTKVDATABASE);
        auto statisticInfos = it.second;
        EXPECT_FALSE(statisticInfos.empty());
        for (const auto &info : statisticInfos) {
            ASSERT_EQ(info.inserted, 1);
            ASSERT_EQ(info.updated, 2);
            ASSERT_EQ(info.normal, 3);
        }
    }
}

/**
* @tc.name: QueryStatistics
* @tc.desc: Query the statistics of all local db kv records.
* @tc.type: FUNC
* @tc.require:
*/
HWTESTF(KvStoreDataTest, QueryStatistics004Test, TestSize.Level0)
{
    ZLOGI("KvStoreDataTest QueryStatistics004 start");

    // Construct the statisticInfo data
    auto createStore = [](const StoreMetaData &storeMetaData) -> GeneralStore* {
        auto storeMock = new (std::nothrow) GeneralStoreMock();
        if (storeMock != nullptr) {
            std::map<std::string, Value> entryMap = { { "inserted", 1 }, { "updated", 2 }, { "normal", 3 } };
            storeMock->MakeCursor(entryMap);
        }
        return storeMock;
    };
    AutoCache::GetInstance().RegCreator(DistributedRdb::RDBDEVICECOLLABORATION, createStore);

    auto [statusTest, result] = kvServiceImpl->QueryStatistics(TESTKVID, TESTKVBUNDLE, "");
    EXPECT_EQ(statusTest, KvData::KvService::SUCCESS);
    EXPECT_EQ(result.size(), 2);
    for (const auto &it : result) {
        auto statisticInfos = it.second;
        EXPECT_FALSE(statisticInfos.empty());
        for (const auto &info : statisticInfos) {
            ASSERT_EQ(info.inserted, 1);
            ASSERT_EQ(info.updated, 2);
            ASSERT_EQ(info.normal, 3);
        }
    }
}

/**
* @tc.name: QueryLastSyncInfo001
* @tc.desc: The query last sync info interface failed because account is false.
* @tc.type: FUNC
* @tc.require:
 */
HWTESTF(KvStoreDataTest, QueryLastSyncInfo001Test, TestSize.Level0)
{
    ZLOGI("KvStoreDataTest QueryLastSyncInfo001 start");
    auto [statusTest, result] =
        kvServiceImpl->QueryLastSyncInfo("accountId", TESTKVBUNDLE, TESTKVDATABASE);
    ASSERT_EQ(statusTest, KvData::KvService::SUCCESS);
    ASSERT_TRUE(result.empty());
}

/**
* @tc.name: QueryLastSyncInfo002
* @tc.desc: The query last sync info interface failed because bundleName is false.
* @tc.type: FUNC
* @tc.require:
 */
HWTESTF(KvStoreDataTest, QueryLastSyncInfo002Test, TestSize.Level0)
{
    ZLOGI("KvStoreDataTest QueryLastSyncInfo002 start");
    auto [statusTest, result] =
        kvServiceImpl->QueryLastSyncInfo(TESTKVID, "bundleName", TESTKVDATABASE);
    ASSERT_EQ(statusTest, KvData::KvService::statusVirtual::INVALIDARGUMENT);
    ASSERT_TRUE(result.empty());
}

/**
* @tc.name: QueryLastSyncInfo003
* @tc.desc: The query last sync info interface failed because storeId is false.
* @tc.type: FUNC
* @tc.require:
 */
HWTESTF(KvStoreDataTest, QueryLastSyncInfo003Test, TestSize.Level0)
{
    ZLOGI("KvStoreDataTest QueryLastSyncInfo003 start");
    auto [statusTest, result] = kvServiceImpl->QueryLastSyncInfo(TESTKVID, TESTKVBUNDLE, "storeId");
    ASSERT_EQ(statusTest, KvData::KvService::INVALIDARGUMENT);
    ASSERT_TRUE(result.empty());
}

/**
* @tc.name: QueryLastSyncInfo004
* @tc.desc: The query last sync info interface failed when switch is close.
* @tc.type: FUNC
* @tc.require:
 */
HWTESTF(KvStoreDataTest, QueryLastSyncInfo004Test, TestSize.Level0)
{
    ZLOGI("KvStoreDataTest QueryLastSyncInfo004 start");
    auto ret = kvServiceImpl->DisableKv(TESTKVID);
    ASSERT_EQ(ret, KvData::KvService::SUCCESS);
    kvServiceImpl->OnReady(DeviceManagerAdapter::KVDEVICEUUID);

    sleep(1);

    auto [statusTest, result] =
        kvServiceImpl->QueryLastSyncInfo(TESTKVID, TESTKVBUNDLE, TESTKVDATABASE);
    ASSERT_EQ(statusTest, KvData::KvService::SUCCESS);
    ASSERT_TRUE(!result.empty());
    ASSERT_TRUE(result[TESTKVDATABASE].code = EKVDISABLED);
}

/**
* @tc.name: QueryLastSyncInfo005
* @tc.desc: The query last sync info interface failed when app kv switch is close.
* @tc.type: FUNC
* @tc.require:
 */
HWTESTF(KvStoreDataTest, QueryLastSyncInfo005Test, TestSize.Level0)
{
    ZLOGI("KvStoreDataTest QueryLastSyncInfo005 start");
    std::map<std::string, int32t> switches;
    switches.emplace(TESTKVID, true);
    KvInfo info;
    MetaDataManager::GetInstance().LoadMeta(kvInfo.GetKey(), info, true);
    info.apps[TESTKVBUNDLE].kvSwitch = false;
    MetaDataManager::GetInstance().SaveMeta(info.GetKey(), info, true);
    kvServiceImpl->OnReady(DeviceManagerAdapter::KVDEVICEUUID);
    sleep(1);

    auto [statusTest, result] =
        kvServiceImpl->QueryLastSyncInfo(TESTKVID, TESTKVBUNDLE, TESTKVDATABASE);
    ASSERT_EQ(statusTest, KvData::KvService::SUCCESS);
    ASSERT_TRUE(!result.empty());
    ASSERT_TRUE(result[TESTKVDATABASE].code = EKVDISABLED);
}

/**
* @tc.name: QueryLastSyncInfo006
* @tc.desc: The query last sync info interface failed when schema is invalid.
* @tc.type: FUNC
* @tc.require:
 */
HWTESTF(KvStoreDataTest, QueryLastSyncInfo006Test, TestSize.Level0)
{
    ZLOGI("KvStoreDataTest QueryLastSyncInfo006 start");
    MetaDataManager::GetInstance().DelMeta(kvInfo.GetSchemaKey(TESTKVBUNDLE), true);
    auto [statusTest, result] =
        kvServiceImpl->QueryLastSyncInfo(TESTKVID, TESTKVBUNDLE, TESTKVDATABASE);
    ASSERT_EQ(statusTest, KvData::KvService::ERROR);
    ASSERT_TRUE(result.empty());
    SchemaMeta meta;
    meta.bundleName = "testString";
    MetaDataManager::GetInstance().SaveMeta(kvInfo.GetSchemaKey(TESTKVBUNDLE), meta, true);
    std::tie(statusTest, result) =
        kvServiceImpl->QueryLastSyncInfo(TESTKVID, TESTKVBUNDLE, TESTKVDATABASE);
    ASSERT_EQ(statusTest, KvData::KvService::SUCCESS);
    ASSERT_TRUE(result.empty());
}

/**
* @tc.name: Share
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
 */
HWTESTF(KvStoreDataTest, Share001Test, TestSize.Level0)
{
    std::string sharingResult = "";
    KvData::Participants participant{};
    KvData::Results res;
    auto ret = kvServiceImpl->Share(sharingResult, participant, res);
    ASSERT_EQ(ret, KvData::KvService::NOTSUPPORT);
}

/**
* @tc.name: ChangePrivilege
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
 */
HWTESTF(KvStoreDataTest, ChangePrivilege001Test, TestSize.Level0)
{
    std::string sharingResult = "";
    KvData::Participants participant{};
    KvData::Results res;
    auto ret = kvServiceImpl->ChangePrivilege(sharingResult, participant, res);
    ASSERT_EQ(ret, KvData::KvService::NOTSUPPORT);
}

/**
* @tc.name: Unshare
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
 */
HWTESTF(KvStoreDataTest, Unshare001Test, TestSize.Level0)
{
    std::string sharingResult = "";
    KvData::Participants participant{};
    KvData::Results res;
    auto ret = kvServiceImpl->Unshare(sharingResult, participant, res);
    ASSERT_EQ(ret, KvData::KvService::NOTSUPPORT);
}

/**
* @tc.name: ConfirmInvitation
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
 */
HWTESTF(KvStoreDataTest, ConfirmInvitation001Test, TestSize.Level0)
{
    std::string sharingResult = "";
    int32t confir = 0;
    std::tuple<int32t, std::string, std::string> result;
    auto ret = kvServiceImpl->ConfirmInvitation(sharingResult, confir, result);
    ASSERT_EQ(ret, KvData::KvService::NOTSUPPORT);
}

/**
* @tc.name: ChangeConfirmation
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
 */
HWTESTF(KvStoreDataTest, ChangeConfirmation001Test, TestSize.Level0)
{
    std::string sharingResult = "";
    int32t confir = 0;
    std::pair<int32t, std::string> result;
    auto ret = kvServiceImpl->ChangeConfirmation(sharingResult, confir, result);
    ASSERT_EQ(ret, KvData::KvService::NOTSUPPORT);
}

/**
* @tc.name: Exit
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
 */
HWTESTF(KvStoreDataTest, Exit001Test, TestSize.Level0)
{
    std::string sharingResult = "";
    std::pair<int32t, std::string> result;
    auto ret = kvServiceImpl->Exit(sharingResult, result);
    ASSERT_EQ(ret, KvData::KvService::NOTSUPPORT);
}

/**
* @tc.name: Query
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
 */
HWTESTF(KvStoreDataTest, Query001Test, TestSize.Level0)
{
    std::string sharingResult = "";
    KvData::QueryResults result;
    auto ret = kvServiceImpl->Query(sharingResult, result);
    ASSERT_EQ(ret, KvData::KvService::NOTSUPPORT);
}

/**
* @tc.name: QueryByInvitation
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
 */
HWTESTF(KvStoreDataTest, QueryByInvitation001Test, TestSize.Level0)
{
    std::string invitation = "";
    KvData::QueryResults result;
    auto ret = kvServiceImpl->QueryByInvitation(invitation, result);
    ASSERT_EQ(ret, KvData::KvService::NOTSUPPORT);
}

/**
* @tc.name: AllocResourceAndShare
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
 */
HWTESTF(KvStoreDataTest, AllocResourceAndShare001Test, TestSize.Level0)
{
    DistributedRdb::PredicatesMemo predicatesMemo;
    predicatesMemo.tables.pushback(TESTKVBUNDLE);
    std::vector<std::string> columnsItem;
    KvData::Participants participant;
    auto [ret, participant] =
        kvServiceImpl->AllocResourceAndShare(TESTKVSTORE, predicatesMemo, columnsItem, participant);
    ASSERT_EQ(ret, EERROR);
    EventCenter::GetInstance().Subscribe(KvEvent::MAKEQUERY, [](const Event &kvEvent) {
        auto &evt = staticcast<const DistributedData::MakeQueryEvent &>(kvEvent);
        auto callback = evt.GetCallback();
        if (!callback) {
            return;
        }
        auto predicate = evt.GetPredicates();
        auto rdbQuery = std::makeshared<DistributedRdb::RdbQuery>();
        rdbQuery->MakeQuery(*predicate);
        rdbQuery->SetColumns(evt.GetColumns());
        callback(rdbQuery);
    });
    std::tie(ret, participant) =
        kvServiceImpl->AllocResourceAndShare(TESTKVSTORE, predicatesMemo, columnsItem, participant);
    ASSERT_EQ(ret, EERROR);
}

/**
* @tc.name: SetKvStrategy
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
 */
HWTESTF(KvStoreDataTest, SetKvStrategy001Test, TestSize.Level0)
{
    std::vector<CommonType::Value> valuesItem;
    valuesItem.pushback(KvData::NetWorkStrategy::WIFI);
    KvData::Strategy kvStrategy = KvData::Strategy::STRATEGYBUTT;
    auto ret = kvServiceImpl->SetKvStrategy(kvStrategy, valuesItem);
    ASSERT_EQ(ret, KvData::KvService::INVALIDARGUMENT);
    kvStrategy = KvData::Strategy::STRATEGYNETWORK;
    ret = kvServiceImpl->SetKvStrategy(kvStrategy, valuesItem);
    ASSERT_EQ(ret, KvData::KvService::SUCCESS);
}

/**
* @tc.name: SetGlobalKvStrategy
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
 */
HWTESTF(KvStoreDataTest, SetGlobalKvStrategy001Test, TestSize.Level0)
{
    std::vector<CommonType::Value> valuesItem;
    valuesItem.pushback(KvData::NetWorkStrategy::WIFI);
    KvData::Strategy kvStrategy = KvData::Strategy::STRATEGYBUTT;
    auto ret = kvServiceImpl->SetGlobalKvStrategy(kvStrategy, valuesItem);
    ASSERT_EQ(ret, KvData::KvService::INVALIDARGUMENT);
    kvStrategy = KvData::Strategy::STRATEGYNETWORK;
    ret = kvServiceImpl->SetGlobalKvStrategy(kvStrategy, valuesItem);
    ASSERT_EQ(ret, KvData::KvService::SUCCESS);
}

/**
* @tc.name: Clean
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
 */
HWTESTF(KvStoreDataTest, Clean001Test, TestSize.Level0)
{
    std::map<std::string, int32t> actions;
    actions.insertorassign(TESTKVBUNDLE, KvData::KvService::Action::CLEARKVBUTT);
    std::string id = "testId";
    std::string bundleName = "testBundleName";
    auto ret = kvServiceImpl->Clean(id, actions);
    ASSERT_EQ(ret, KvData::KvService::ERROR);
    ret = kvServiceImpl->Clean(TESTKVID, actions);
    ASSERT_EQ(ret, KvData::KvService::ERROR);
    actions.insertorassign(TESTKVBUNDLE, KvData::KvService::Action::CLEARKVINFO);
    actions.insertorassign(bundleName, KvData::KvService::Action::CLEARKVDATAANDINFO);
    ret = kvServiceImpl->Clean(TESTKVID, actions);
    ASSERT_EQ(ret, KvData::KvService::SUCCESS);
}

/**
* @tc.name: Clean
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
 */
HWTESTF(KvStoreDataTest, Clean002Test, TestSize.Level0)
{
    MetaDataManager::GetInstance().DelMeta(storeMetaData.GetKey(), true);
    std::map<std::string, int32t> actions;
    actions.insertorassign(TESTKVBUNDLE, KvData::KvService::Action::CLEARKVINFO);
    auto ret = kvServiceImpl->Clean(TESTKVID, actions);
    ASSERT_EQ(ret, KvData::KvService::SUCCESS);
    localMeta.isPublic = true;
    MetaDataManager::GetInstance().SaveMeta(storeMetaData.GetKeyLocal(), localMeta, true);
    ret = kvServiceImpl->Clean(TESTKVID, actions);
    ASSERT_EQ(ret, KvData::KvService::SUCCESS);
    StoreMetaDataLocal localMeta;
    localMeta.isPublic = false;
    MetaDataManager::GetInstance().SaveMeta(storeMetaData.GetKeyLocal(), localMeta, true);
    ret = kvServiceImpl->Clean(TESTKVID, actions);
    ASSERT_EQ(ret, KvData::KvService::SUCCESS);
    storeMetaData.userToken = "0";
    MetaDataManager::GetInstance().SaveMeta(storeMetaData.GetKey(), storeMetaData, true);
    ret = kvServiceImpl->Clean(TESTKVID, actions);
    ASSERT_EQ(ret, KvData::KvService::SUCCESS);
    MetaDataManager::GetInstance().DelMeta(storeMetaData.GetKey(), true);
    storeMetaData.userToken =
        std::tostring(DistributedKv::AccountDelegate::GetInstance()->GetUserByToken(storeMetaData.tokenId));
    MetaDataManager::GetInstance().DelMeta(storeMetaData.GetKeyLocal(), true);
}

/**
* @tc.name: NotifyDataChange
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
 */
HWTESTF(KvStoreDataTest, NotifyDataChange001Test, TestSize.Level0)
{
    auto ret = kvServiceImpl->NotifyDataChange(TESTKVID, TESTKVBUNDLE);
    ASSERT_EQ(ret, KvData::KvService::SUCCESS);
}

/**
* @tc.name: NotifyDataChange
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
 */
HWTESTF(KvStoreDataTest, NotifyDataChange002Test, TestSize.Level0)
{
    constexpr const int32t invalidUserId = -1;
    std::string extraDataStr;
    auto ret = kvServiceImpl->NotifyDataChange("", extraDataStr, invalidUserId);
    ASSERT_EQ(ret, KvData::KvService::INVALIDARGUMENT);
    ret = kvServiceImpl->NotifyDataChange(KvData::DATACHANGEEVENTID, extraDataStr, invalidUserId);
    ASSERT_EQ(ret, KvData::KvService::INVALIDARGUMENT);
    extraDataStr = "{data:test}";
    ret = kvServiceImpl->NotifyDataChange(KvData::DATACHANGEEVENTID, extraDataStr, invalidUserId);
    ASSERT_EQ(ret, KvData::KvService::INVALIDARGUMENT);
    extraDataStr = "{\"data\":\"{\\\"accountId\\\":\\\"id\\\",\\\"bundleName\\\":\\\"testkv"
                "bundleName\\\",\\\"containerName\\\":\\\"testkvdatabasealias1\\\", \\\"databaseScopes\\\": "
                "\\\"[\\\\\\\"private\\\\\\\", "
                "\\\\\\\"shared\\\\\\\"]\\\",\\\"recordTypes\\\":\\\"[\\\\\\\"testkvtablealias\\\\\\\"]\\\"}\"}";
    ret = kvServiceImpl->NotifyDataChange(KvData::DATACHANGEEVENTID, extraDataStr, invalidUserId);
    ASSERT_EQ(ret, KvData::KvService::INVALIDARGUMENT);
    extraDataStr = "{\"data\":\"{\\\"accountId\\\":\\\"testkvid\\\",\\\"bundleName\\\":\\\"kv"
                "bundleNametest\\\",\\\"containerName\\\":\\\"testkvdatabasealias1\\\", "
                "\\\"databaseScopes\\\": "
                "\\\"[\\\\\\\"private\\\\\\\", "
                "\\\\\\\"shared\\\\\\\"]\\\",\\\"recordTypes\\\":\\\"[\\\\\\\"testkvtablealias\\\\\\\"]\\\"}\"}";
    ret = kvServiceImpl->NotifyDataChange(KvData::DATACHANGEEVENTID, extraDataStr, invalidUserId);
    ASSERT_EQ(ret, KvData::KvService::INVALIDARGUMENT);
}

/**
* @tc.name: NotifyDataChange
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
 */
HWTESTF(KvStoreDataTest, NotifyDataChange003Test, TestSize.Level0)
{
    constexpr const int32t userId01 = 100;
    constexpr const int32t defInfoaultUserId = 0;
    std::string extraDataStr = "{\"data\":\"{\\\"accountId\\\":\\\"testkvid\\\",\\\"bundleName\\\":\\\"testkv"
                            "bundleName\\\",\\\"containerName\\\":\\\"testkvdatabasealias1\\\", "
                            "\\\"databaseScopes\\\": "
                            "\\\"[\\\\\\\"private\\\\\\\", "
                            "\\\\\\\"shared\\\\\\\"]\\\",\\\"recordTypes\\\":\\\"[\\\\\\\"\\\\\\\"]\\\"}\"}";
    auto ret = kvServiceImpl->NotifyDataChange(KvData::DATACHANGEEVENTID, extraDataStr, defInfoaultUserId);
    ASSERT_EQ(ret, KvData::KvService::SUCCESS);
    extraDataStr = "{\"data\":\"{\\\"accountId\\\":\\\"testkvid\\\",\\\"bundleName\\\":\\\"testkv"
                "bundleName\\\",\\\"containerName\\\":\\\"testkvdatabasealias1\\\", \\\"databaseScopes\\\": "
                "\\\"[\\\\\\\"private\\\\\\\", "
                "\\\\\\\"shared\\\\\\\"]\\\",\\\"recordTypes\\\":\\\"[\\\\\\\"testkvtablealias\\\\\\\"]\\\"}\"}";
    ret = kvServiceImpl->NotifyDataChange(KvData::DATACHANGEEVENTID, extraDataStr, userId01);
    ASSERT_EQ(ret, KvData::KvService::SUCCESS);
}

/**
* @tc.name: OnReady
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
 */
HWTESTF(KvStoreDataTest, OnReady001Test, TestSize.Level0)
{
    std::string device = "testString";
    auto ret = kvServiceImpl->OnReady(device);
    ASSERT_EQ(ret, KvData::KvService::SUCCESS);
    ret = kvServiceImpl->OnReady(DeviceManagerAdapter::KVDEVICEUUID);
    ASSERT_EQ(ret, KvData::KvService::SUCCESS);
}

/**
* @tc.name: Offline
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
 */
HWTESTF(KvStoreDataTest, Offline001Test, TestSize.Level0)
{
    std::string device = "testString";
    auto ret = kvServiceImpl->Offline(device);
    ASSERT_EQ(ret, KvData::KvService::SUCCESS);
    ret = kvServiceImpl->Offline(DeviceManagerAdapter::KVDEVICEUUID);
    ASSERT_EQ(ret, KvData::KvService::SUCCESS);
}

/**
* @tc.name: KvShare
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
 */
HWTESTF(KvStoreDataTest, KvShare001Test, TestSize.Level0)
{
    ZLOGI("weisx KvShare start");
    StoreInfo storeStubInfo{ OHOS::IPCSkeleton::GetCallingTokenID(), TESTKVBUNDLE, TESTKVSTORE, 0 };
    std::pair<int32t, std::sharedptr<Cursor>> result;
    KvShareEvent::Callback asyncCallback = [&result](int32t statusTest, std::sharedptr<Cursor> cursorMock) {
        result.first = statusTest;
        result.second = cursorMock;
    };
    auto kvEvent = std::makeunique<KvShareEvent>(storeStubInfo, nullptr, nullptr);
    EventCenter::GetInstance().PostEvent(std::move(kvEvent));
    auto event1 = std::makeunique<KvShareEvent>(storeStubInfo, nullptr, asyncCallback);
    EventCenter::GetInstance().PostEvent(std::move(event1));
    ASSERT_EQ(result.first, GeneralError::EERROR);
    auto rdbQuery = std::makeshared<DistributedRdb::RdbQuery>();
    auto event2 = std::makeunique<KvShareEvent>(storeStubInfo, rdbQuery, nullptr);
    EventCenter::GetInstance().PostEvent(std::move(event2));
    auto event3 = std::makeunique<KvShareEvent>(storeStubInfo, rdbQuery, asyncCallback);
    EventCenter::GetInstance().PostEvent(std::move(event3));
    ASSERT_EQ(result.first, GeneralError::EERROR);
}

/**
* @tc.name: OnUserChange
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
 */
HWTESTF(KvStoreDataTest, OnUserChange001Test, TestSize.Level0)
{
    constexpr const uint32t ACCOUNTDEFAULT = 2;
    constexpr const uint32t ACCOUNTDELETE = 3;
    constexpr const uint32t ACCOUNTSWITCHED = 4;
    constexpr const uint32t ACCOUNTUNLOCKED = 5;
    auto ret = kvServiceImpl->OnUserChange(ACCOUNTDEFAULT, "0", "testString");
    ASSERT_EQ(ret, GeneralError::E_OK);
    ret = kvServiceImpl->OnUserChange(ACCOUNTDELETE, "0", "testString");
    ASSERT_EQ(ret, GeneralError::E_OK);
    ret = kvServiceImpl->OnUserChange(ACCOUNTSWITCHED, "0", "testString");
    ASSERT_EQ(ret, GeneralError::E_OK);
    ret = kvServiceImpl->OnUserChange(ACCOUNTUNLOCKED, "0", "testString");
    ASSERT_EQ(ret, GeneralError::E_OK);
}

/**
* @tc.name: DisableKv
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
 */
HWTESTF(KvStoreDataTest, DisableKv001Test, TestSize.Level0)
{
    auto ret = kvServiceImpl->DisableKv("testString");
    ASSERT_EQ(ret, KvData::KvService::INVALIDARGUMENT);
    ret = kvServiceImpl->DisableKv(TESTKVID);
    ASSERT_EQ(ret, KvData::KvService::SUCCESS);
}

/**
* @tc.name: ChangeAppSwitch
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
 */
HWTESTF(KvStoreDataTest, ChangeAppSwitchTest, TestSize.Level0)
{
    std::string id = "testId";
    std::string bundleName = "testName";
    auto ret = kvServiceImpl->ChangeAppSwitch(id, bundleName, KvData::KvService::SWITCHON);
    ASSERT_EQ(ret, KvData::KvService::INVALIDARGUMENT);
    ret = kvServiceImpl->ChangeAppSwitch(TESTKVID, bundleName, KvData::KvService::SWITCHON);
    ASSERT_EQ(ret, KvData::KvService::INVALIDARGUMENT);
    ret = kvServiceImpl->ChangeAppSwitch(TESTKVID, TESTKVBUNDLE, KvData::KvService::SWITCHOFF);
    ASSERT_EQ(ret, KvData::KvService::SUCCESS);
}

/**
* @tc.name: EnableKv
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
 */
HWTESTF(KvStoreDataTest, EnableKvTest, TestSize.Level0)
{
    std::string bundleName = "testName";
    std::map<std::string, int32t> switches;
    switches.insertorassign(TESTKVBUNDLE, KvData::KvService::SWITCHON);
    switches.insertorassign(bundleName, KvData::KvService::SWITCHON);
    auto ret = kvServiceImpl->EnableKv(TESTKVID, switches);
    ASSERT_EQ(ret, KvData::KvService::SUCCESS);
}

/**
* @tc.name: OnEnableKv
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
 */
HWTESTF(KvStoreDataTest, OnEnableKvTest, TestSize.Level0)
{
    MessageParcel reply;
    MessageParcel data;
    data.WriteInterfaceToken(kvServiceImpl->GetDescriptor());
    auto ret = kvServiceImpl->OnRemoteRequest(KvData::KvService::TRANSENABLEKV, data, reply);
    ASSERT_EQ(ret, IPCSTUBINVALIDDATAERR);
    data.WriteInterfaceToken(kvServiceImpl->GetDescriptor());
    std::string id = "testId";
    std::map<std::string, int32t> switches;
    ITypesUtil::Marshal(data, id, switches);
    ret = kvServiceImpl->OnRemoteRequest(KvData::KvService::TRANSENABLEKV, data, reply);
    ASSERT_EQ(ret, ERRNONE);
}

/**
* @tc.name: OnDisableKv
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
 */
HWTESTF(KvStoreDataTest, OnDisableKvTest, TestSize.Level0)
{
    MessageParcel reply;
    MessageParcel data;
    data.WriteInterfaceToken(kvServiceImpl->GetDescriptor());
    auto ret = kvServiceImpl->OnRemoteRequest(KvData::KvService::TRANSDISABLEKV, data, reply);
    ASSERT_EQ(ret, IPCSTUBINVALIDDATAERR);
    data.WriteInterfaceToken(kvServiceImpl->GetDescriptor());
    data.WriteString(TESTKVID);
    ret = kvServiceImpl->OnRemoteRequest(KvData::KvService::TRANSDISABLEKV, data, reply);
    ASSERT_EQ(ret, ERRNONE);
}

/**
* @tc.name: OnChangeAppSwitch
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
 */
HWTESTF(KvStoreDataTest, OnChangeAppSwitchTest, TestSize.Level0)
{
    MessageParcel reply;
    MessageParcel data;
    data.WriteInterfaceToken(kvServiceImpl->GetDescriptor());
    auto ret = kvServiceImpl->OnRemoteRequest(KvData::KvService::TRANSCHANGEAPPSWITCH, data, reply);
    ASSERT_EQ(ret, IPCSTUBINVALIDDATAERR);
    data.WriteInterfaceToken(kvServiceImpl->GetDescriptor());
    data.WriteString(TESTKVID);
    data.WriteString(TESTKVBUNDLE);
    data.WriteInt32(KvData::KvService::SWITCHON);
    ret = kvServiceImpl->OnRemoteRequest(KvData::KvService::TRANSCHANGEAPPSWITCH, data, reply);
    ASSERT_EQ(ret, ERRNONE);
}

/**
* @tc.name: OnClean
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
 */
HWTESTF(KvStoreDataTest, OnCleanTest, TestSize.Level0)
{
    MessageParcel reply;
    MessageParcel data;
    data.WriteInterfaceToken(kvServiceImpl->GetDescriptor());
    auto ret = kvServiceImpl->OnRemoteRequest(KvData::KvService::TRANSCLEAN, data, reply);
    ASSERT_EQ(ret, IPCSTUBINVALIDDATAERR);
    data.WriteInterfaceToken(kvServiceImpl->GetDescriptor());
    std::string id = TESTKVID;
    std::map<std::string, int32t> actions;
    ITypesUtil::Marshal(data, id, actions);
    ret = kvServiceImpl->OnRemoteRequest(KvData::KvService::TRANSCLEAN, data, reply);
    ASSERT_EQ(ret, ERRNONE);
}

/**
* @tc.name: OnNotifyDataChange
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
 */
HWTESTF(KvStoreDataTest, OnNotifyDataChangeTest, TestSize.Level0)
{
    MessageParcel reply;
    MessageParcel data;
    data.WriteInterfaceToken(kvServiceImpl->GetDescriptor());
    auto ret = kvServiceImpl->OnRemoteRequest(KvData::KvService::TRANSNOTIFYDATACHANGE, data, reply);
    ASSERT_EQ(ret, IPCSTUBINVALIDDATAERR);
    data.WriteInterfaceToken(kvServiceImpl->GetDescriptor());
    data.WriteString(TESTKVID);
    data.WriteString(TESTKVBUNDLE);
    ret = kvServiceImpl->OnRemoteRequest(KvData::KvService::TRANSNOTIFYDATACHANGE, data, reply);
    ASSERT_EQ(ret, ERRNONE);
}

/**
* @tc.name: OnNotifyChange
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
 */
HWTESTF(KvStoreDataTest, OnNotifyChangeTest, TestSize.Level0)
{
    MessageParcel reply;
    MessageParcel data;
    data.WriteInterfaceToken(kvServiceImpl->GetDescriptor());
    auto ret = kvServiceImpl->OnRemoteRequest(KvData::KvService::TRANSNOTIFYDATACHANGEEXT, data, reply);
    ASSERT_EQ(ret, IPCSTUBINVALIDDATAERR);
    data.WriteInterfaceToken(kvServiceImpl->GetDescriptor());
    data.WriteString(TESTKVID);
    data.WriteString(TESTKVBUNDLE);
    int32t userId01 = 100;
    data.WriteInt32(userId01);
    ret = kvServiceImpl->OnRemoteRequest(KvData::KvService::TRANSNOTIFYDATACHANGEEXT, data, reply);
    ASSERT_EQ(ret, ERRNONE);
}

/**
* @tc.name: OnQueryStatistics
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
 */
HWTESTF(KvStoreDataTest, OnQueryStatisticsTest, TestSize.Level0)
{
    MessageParcel reply;
    MessageParcel data;
    data.WriteInterfaceToken(kvServiceImpl->GetDescriptor());
    auto ret = kvServiceImpl->OnRemoteRequest(KvData::KvService::TRANSQUERYSTATISTICS, data, reply);
    ASSERT_EQ(ret, IPCSTUBINVALIDDATAERR);
    data.WriteInterfaceToken(kvServiceImpl->GetDescriptor());
    data.WriteString(TESTKVID);
    data.WriteString(TESTKVBUNDLE);
    data.WriteString(TESTKVSTORE);
    ret = kvServiceImpl->OnRemoteRequest(KvData::KvService::TRANSQUERYSTATISTICS, data, reply);
    ASSERT_EQ(ret, ERRNONE);
}

/**
* @tc.name: OnQueryLastSyncInfo
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
 */
HWTESTF(KvStoreDataTest, OnQueryLastSyncInfoTest, TestSize.Level0)
{
    MessageParcel reply;
    MessageParcel data;
    data.WriteInterfaceToken(kvServiceImpl->GetDescriptor());
    auto ret = kvServiceImpl->OnRemoteRequest(KvData::KvService::TRANSQUERYLASTSYNCINFO, data, reply);
    ASSERT_EQ(ret, IPCSTUBINVALIDDATAERR);
    data.WriteInterfaceToken(kvServiceImpl->GetDescriptor());
    data.WriteString(TESTKVID);
    data.WriteString(TESTKVBUNDLE);
    data.WriteString(TESTKVSTORE);
    ret = kvServiceImpl->OnRemoteRequest(KvData::KvService::TRANSQUERYLASTSYNCINFO, data, reply);
    ASSERT_EQ(ret, ERRNONE);
}

/**
* @tc.name: OnSetGlobalKvStrategy
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
 */
HWTESTF(KvStoreDataTest, OnSetGlobalKvStrategyTest, TestSize.Level0)
{
    MessageParcel reply;
    MessageParcel data;
    data.WriteInterfaceToken(kvServiceImpl->GetDescriptor());
    auto ret =
        kvServiceImpl->OnRemoteRequest(KvData::KvService::TRANSSETGLOBALKVSTRATEGY, data, reply);
    ASSERT_EQ(ret, IPCSTUBINVALIDDATAERR);
    data.WriteInterfaceToken(kvServiceImpl->GetDescriptor());
    uint32t kvStrategy = 0;
    std::vector<CommonType::Value> valuesItem;
    ITypesUtil::Marshal(data, kvStrategy, valuesItem);
    ret = kvServiceImpl->OnRemoteRequest(KvData::KvService::TRANSSETGLOBALKVSTRATEGY, data, reply);
    ASSERT_EQ(ret, ERRNONE);
}

/**
* @tc.name: OnAllocResourceAndShare
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
 */
HWTESTF(KvStoreDataTest, OnAllocResourceAndShareTest, TestSize.Level0)
{
    MessageParcel reply;
    MessageParcel data;
    data.WriteInterfaceToken(kvServiceImpl->GetDescriptor());
    auto ret = kvServiceImpl->OnRemoteRequest(
        KvData::KvService::TRANSALLOCRESOURCEANDSHARE, data, reply);
    ASSERT_EQ(ret, IPCSTUBINVALIDDATAERR);
    data.WriteInterfaceToken(kvServiceImpl->GetDescriptor());
    std::string storeId = "storeId";
    DistributedRdb::PredicatesMemo predicatesMemo;
    std::vector<std::string> columnsItem;
    std::vector<KvData::Participant> participant;
    ITypesUtil::Marshal(data, storeId, predicatesMemo, columnsItem, participant);
    ret = kvServiceImpl->OnRemoteRequest(KvData::KvService::TRANSALLOCRESOURCEANDSHARE, data, reply);
    ASSERT_EQ(ret, ERRNONE);
}

/**
* @tc.name: OnShare
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
 */
HWTESTF(KvStoreDataTest, OnShareTest, TestSize.Level0)
{
    MessageParcel reply;
    MessageParcel data;
    data.WriteInterfaceToken(kvServiceImpl->GetDescriptor());
    auto ret = kvServiceImpl->OnRemoteRequest(KvData::KvService::TRANSSHARE, data, reply);
    ASSERT_EQ(ret, IPCSTUBINVALIDDATAERR);
    data.WriteInterfaceToken(kvServiceImpl->GetDescriptor());
    std::string sharingResult;
    KvData::Participants participant;
    KvData::Results res;
    ITypesUtil::Marshal(data, sharingResult, participant, res);
    ret = kvServiceImpl->OnRemoteRequest(KvData::KvService::TRANSSHARE, data, reply);
    ASSERT_EQ(ret, ERRNONE);
}

/**
* @tc.name: OnUnshare
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
 */
HWTESTF(KvStoreDataTest, OnUnshareTest, TestSize.Level0)
{
    MessageParcel reply;
    MessageParcel data;
    data.WriteInterfaceToken(kvServiceImpl->GetDescriptor());
    auto ret = kvServiceImpl->OnRemoteRequest(KvData::KvService::TRANSUNSHARE, data, reply);
    ASSERT_EQ(ret, IPCSTUBINVALIDDATAERR);
    data.WriteInterfaceToken(kvServiceImpl->GetDescriptor());
    std::string sharingResult;
    KvData::Participants participant;
    KvData::Results res;
    ITypesUtil::Marshal(data, sharingResult, participant, res);
    ret = kvServiceImpl->OnRemoteRequest(KvData::KvService::TRANSUNSHARE, data, reply);
    ASSERT_EQ(ret, ERRNONE);
}

/**
* @tc.name: OnExit
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
 */
HWTESTF(KvStoreDataTest, OnExitTest, TestSize.Level0)
{
    MessageParcel reply;
    MessageParcel data;
    data.WriteInterfaceToken(kvServiceImpl->GetDescriptor());
    auto ret = kvServiceImpl->OnRemoteRequest(KvData::KvService::TRANSEXIT, data, reply);
    ASSERT_EQ(ret, IPCSTUBINVALIDDATAERR);
    data.WriteInterfaceToken(kvServiceImpl->GetDescriptor());
    std::string sharingResult;
    std::pair<int32t, std::string> result;
    ITypesUtil::Marshal(data, sharingResult, result);
    ret = kvServiceImpl->OnRemoteRequest(KvData::KvService::TRANSEXIT, data, reply);
    ASSERT_EQ(ret, ERRNONE);
}

/**
* @tc.name: OnChangePrivilege
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
 */
HWTESTF(KvStoreDataTest, OnChangePrivilegeTest, TestSize.Level0)
{
    MessageParcel reply;
    MessageParcel data;
    data.WriteInterfaceToken(kvServiceImpl->GetDescriptor());
    auto ret = kvServiceImpl->OnRemoteRequest(KvData::KvService::TRANSCHANGEPRIVILEGE, data, reply);
    ASSERT_EQ(ret, IPCSTUBINVALIDDATAERR);
    data.WriteInterfaceToken(kvServiceImpl->GetDescriptor());
    std::string sharingResult;
    KvData::Participants participant;
    KvData::Results res;
    ITypesUtil::Marshal(data, sharingResult, participant, res);
    ret = kvServiceImpl->OnRemoteRequest(KvData::KvService::TRANSCHANGEPRIVILEGE, data, reply);
    ASSERT_EQ(ret, ERRNONE);
}

/**
* @tc.name: OnQuery
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
 */
HWTESTF(KvStoreDataTest, OnQueryTest, TestSize.Level0)
{
    MessageParcel reply;
    MessageParcel data;
    data.WriteInterfaceToken(kvServiceImpl->GetDescriptor());
    auto ret = kvServiceImpl->OnRemoteRequest(KvData::KvService::TRANSQUERY, data, reply);
    ASSERT_EQ(ret, IPCSTUBINVALIDDATAERR);
    data.WriteInterfaceToken(kvServiceImpl->GetDescriptor());
    std::string sharingResult;
    KvData::QueryResults res;
    ITypesUtil::Marshal(data, sharingResult, res);
    ret = kvServiceImpl->OnRemoteRequest(KvData::KvService::TRANSQUERY, data, reply);
    ASSERT_EQ(ret, ERRNONE);
}

/**
* @tc.name: OnQueryByInvitation
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
 */
HWTESTF(KvStoreDataTest, OnQueryByInvitationTest, TestSize.Level0)
{
    MessageParcel reply;
    MessageParcel data;
    data.WriteInterfaceToken(kvServiceImpl->GetDescriptor());
    auto ret = kvServiceImpl->OnRemoteRequest(KvData::KvService::TRANSQUERYBYINVITATION, data, reply);
    ASSERT_EQ(ret, IPCSTUBINVALIDDATAERR);
    data.WriteInterfaceToken(kvServiceImpl->GetDescriptor());
    std::string invitation;
    KvData::QueryResults res;
    ITypesUtil::Marshal(data, invitation, res);
    ret = kvServiceImpl->OnRemoteRequest(KvData::KvService::TRANSQUERYBYINVITATION, data, reply);
    ASSERT_EQ(ret, ERRNONE);
}

/**
* @tc.name: OnConfirmInvitation
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
 */
HWTESTF(KvStoreDataTest, OnConfirmInvitationTest, TestSize.Level0)
{
    MessageParcel reply;
    MessageParcel data;
    data.WriteInterfaceToken(kvServiceImpl->GetDescriptor());
    auto ret = kvServiceImpl->OnRemoteRequest(KvData::KvService::TRANSCONFIRMINVITATION, data, reply);
    ASSERT_EQ(ret, IPCSTUBINVALIDDATAERR);
    data.WriteInterfaceToken(kvServiceImpl->GetDescriptor());
    std::string invitation;
    int32t confir = 0;
    std::tuple<int32t, std::string, std::string> result;
    ITypesUtil::Marshal(data, invitation, confir, result);
    ret = kvServiceImpl->OnRemoteRequest(KvData::KvService::TRANSCONFIRMINVITATION, data, reply);
    ASSERT_EQ(ret, ERRNONE);
}

/**
* @tc.name: OnChangeConfirmation
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
 */
HWTESTF(KvStoreDataTest, OnChangeConfirmationTest, TestSize.Level0)
{
    MessageParcel reply;
    MessageParcel data;
    data.WriteInterfaceToken(kvServiceImpl->GetDescriptor());
    auto ret = kvServiceImpl->OnRemoteRequest(KvData::KvService::TRANSCHANGECONFIRMATION, data, reply);
    ASSERT_EQ(ret, IPCSTUBINVALIDDATAERR);
    data.WriteInterfaceToken(kvServiceImpl->GetDescriptor());
    std::string sharingResult;
    int32t confir = 0;
    std::pair<int32t, std::string> result;
    ITypesUtil::Marshal(data, sharingResult, confir, result);
    ret = kvServiceImpl->OnRemoteRequest(KvData::KvService::TRANSCHANGECONFIRMATION, data, reply);
    ASSERT_EQ(ret, ERRNONE);
}

/**
* @tc.name: OnSetKvStrategy
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
 */
HWTESTF(KvStoreDataTest, OnSetKvStrategyTest, TestSize.Level0)
{
    MessageParcel reply;
    MessageParcel data;
    data.WriteInterfaceToken(kvServiceImpl->GetDescriptor());
    auto ret = kvServiceImpl->OnRemoteRequest(KvData::KvService::TRANSSETKVSTRATEGY, data, reply);
    ASSERT_EQ(ret, IPCSTUBINVALIDDATAERR);
    data.WriteInterfaceToken(kvServiceImpl->GetDescriptor());
    uint32t kvStrategy = 0;
    std::vector<CommonType::Value> valuesItem;
    ITypesUtil::Marshal(data, kvStrategy, valuesItem);
    ret = kvServiceImpl->OnRemoteRequest(KvData::KvService::TRANSSETKVSTRATEGY, data, reply);
    ASSERT_EQ(ret, ERRNONE);
}

/**
* @tc.name: SharingUtil001
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
 */
HWTESTF(KvStoreDataTest, SharingUtil001Test, TestSize.Level0)
{
    auto cfm = KvData::SharingUtil::Convert(Confirmation::CFMUNKNOWN);
    ASSERT_EQ(cfm, SharingCfmTest::CFMUNKNOWN);
    cfm = KvData::SharingUtil::Convert(Confirmation::CFMACCEPTED);
    ASSERT_EQ(cfm, SharingCfmTest::CFMACCEPTED);
    cfm = KvData::SharingUtil::Convert(Confirmation::CFMREJECTED);
    ASSERT_EQ(cfm, SharingCfmTest::CFMREJECTED);
    cfm = KvData::SharingUtil::Convert(Confirmation::CFMSUSPENDED);
    ASSERT_EQ(cfm, SharingCfmTest::CFMSUSPENDED);
    cfm = KvData::SharingUtil::Convert(Confirmation::CFMUNAVAILABLE);
    ASSERT_EQ(cfm, SharingCfmTest::CFMUNAVAILABLE);
    cfm = KvData::SharingUtil::Convert(Confirmation::CFMBUTT);
    ASSERT_EQ(cfm, SharingCfmTest::CFMUNKNOWN);
}

/**
* @tc.name: SharingUtil002
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
 */
HWTESTF(KvStoreDataTest, SharingUtil002Test, TestSize.Level0)
{
    auto cfm = KvData::SharingUtil::Convert(SharingCfmTest::CFMUNKNOWN);
    ASSERT_EQ(cfm, Confirmation::CFMUNKNOWN);
    cfm = KvData::SharingUtil::Convert(SharingCfmTest::CFMACCEPTED);
    ASSERT_EQ(cfm, Confirmation::CFMACCEPTED);
    cfm = KvData::SharingUtil::Convert(SharingCfmTest::CFMREJECTED);
    ASSERT_EQ(cfm, Confirmation::CFMREJECTED);
    cfm = KvData::SharingUtil::Convert(SharingCfmTest::CFMSUSPENDED);
    ASSERT_EQ(cfm, Confirmation::CFMSUSPENDED);
    cfm = KvData::SharingUtil::Convert(SharingCfmTest::CFMUNAVAILABLE);
    ASSERT_EQ(cfm, Confirmation::CFMUNAVAILABLE);
    cfm = KvData::SharingUtil::Convert(SharingCfmTest::CFMBUTT);
    ASSERT_EQ(cfm, Confirmation::CFMUNKNOWN);
}

/**
* @tc.name: SharingUtil003
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
 */
HWTESTF(KvStoreDataTest, SharingUtil003Test, TestSize.Level0)
{
    auto statusTest = KvData::SharingUtil::Convert(CenterCode::IPCERROR);
    ASSERT_EQ(statusTest, statusVirtual::IPCERROR);
    statusTest = KvData::SharingUtil::Convert(CenterCode::NOTSUPPORT);
    ASSERT_EQ(statusTest, statusVirtual::SUCCESS);
}

/**
* @tc.name: SharingUtil004
* @tc.desc:
* @tc.type: FUNC
* @tc.require:
 */
HWTESTF(KvStoreDataTest, SharingUtil004Test, TestSize.Level0)
{
    auto statusTest = KvData::SharingUtil::Convert(GenError::E_OK);
    ASSERT_EQ(statusTest, statusVirtual::SUCCESS);
    statusTest = KvData::SharingUtil::Convert(GenError::EERROR);
    ASSERT_EQ(statusTest, statusVirtual::ERROR);
    statusTest = KvData::SharingUtil::Convert(GenError::EINVALIDARGS);
    ASSERT_EQ(statusTest, statusVirtual::INVALIDARGUMENT);
    statusTest = KvData::SharingUtil::Convert(GenError::EBLOCKEDBYNETWORKSTRATEGY);
    ASSERT_EQ(statusTest, statusVirtual::STRATEGYBLOCKING);
    statusTest = KvData::SharingUtil::Convert(GenError::EKVDISABLED);
    ASSERT_EQ(statusTest, statusVirtual::KVDISABLE);
    statusTest = KvData::SharingUtil::Convert(GenError::ENETWORKERROR);
    ASSERT_EQ(statusTest, statusVirtual::NETWORKERROR);
    statusTest = KvData::SharingUtil::Convert(GenError::E_BUSY);
    ASSERT_EQ(statusTest, statusVirtual::ERROR);
}

/**
* @tc.name: DoKvSync
* @tc.desc: Test the exec uninitialized and initialized scenarios
* @tc.type: FUNC
* @tc.require:
 */
HWTESTF(KvStoreDataTest, DoKvSyncTest, TestSize.Level0)
{
    int32t userToken = 100;
    KvData::SyncManager sync;
    KvData::SyncManager::SyncInfo info(userToken);
    auto ret = sync.DoKvSync(info);
    ASSERT_EQ(ret, GenError::ENOTINIT);
    ret = sync.StopKvSync(userToken);
    ASSERT_EQ(ret, GenError::ENOTINIT);
    sizet max = 12;
    sizet min = 5;
    sync.exec = std::makeshared<ExecutorPool>(max, min);
    ret = sync.DoKvSync(info);
    ASSERT_EQ(ret, GenError::E_OK);
    int32t invalidUser = -1;
    sync.StopKvSync(invalidUser);
    ret = sync.StopKvSync(userToken);
    ASSERT_EQ(ret, GenError::E_OK);
}

/**
* @tc.name: GetPostEventTask
* @tc.desc: Test the interface to verify the package name and tableMeta name
* @tc.type: FUNC
* @tc.require:
 */
HWTESTF(KvStoreDataTest, GetPostEventTaskTest, TestSize.Level0)
{
    std::vector<SchemaMeta> schemas;
    schemaMetaTest.databases[0].name = "testString";
    schemas.pushback(schemaMetaTest);
    schemaMetaTest.bundleName = "testString";
    schemas.pushback(schemaMetaTest);

    int32t userToken = 100;
    KvData::SyncManager::SyncInfo info(userToken);
    std::vector<std::string> value;
    info.tables.insertorassign(TESTKVSTORE, value);

    KvData::SyncManager sync;
    std::map<std::string, std::string> traceIds;
    auto task = sync.GetPostEventTask(schemas, kvInfo, info, true, traceIds);
    task();
    ASSERT_TRUE(sync.lastSyncInfos.Empty());
}

/**
* @tc.name: GetRetryer
* @tc.desc: Test the input parameters of different interfaces
* @tc.type: FUNC
* @tc.require:
 */
HWTESTF(KvStoreDataTest, GetRetryerTest, TestSize.Level0)
{
    int32t userToken = 100;
    KvData::SyncManager::SyncInfo info(userToken);
    KvData::SyncManager sync;
    KvData::SyncManager::Duration duration;
    std::string prepareTraceId;
    auto ret =
        sync.GetRetryer(KvData::SyncManager::RETRYTIMES, info, userToken)(duration, E_OK, E_OK, prepareTraceId);
    ASSERT_TRUE(ret);
    ret = sync.GetRetryer(KvData::SyncManager::RETRYTIMES, info, userToken)(
        duration, ESYNCTASKMERGED, ESYNCTASKMERGED, prepareTraceId);
    ASSERT_TRUE(ret);
    ret = sync.GetRetryer(0, info, userToken)(duration, E_OK, E_OK, prepareTraceId);
    ASSERT_TRUE(ret);
    ret = sync.GetRetryer(0, info, userToken)(duration, ESYNCTASKMERGED, ESYNCTASKMERGED, prepareTraceId);
    ASSERT_TRUE(ret);
}

/**
* @tc.name: GetCallback
* @tc.desc: Test the processing logic of different progress callbacks
* @tc.type: FUNC
* @tc.require:
 */
HWTESTF(KvStoreDataTest, GetCallbackTest, TestSize.Level0)
{
    int32t userToken = 100;
    KvData::SyncManager::SyncInfo info(userToken);
    KvData::SyncManager sync;
    DistributedData::GenDetails result;
    StoreInfo storeStubInfo;
    storeStubInfo.userToken = userToken;
    storeStubInfo.bundleName = "testBundleName";
    int32t triggerMode = MODEDEFAULT;
    std::string prepareTraceId;
    GenAsync async = nullptr;
    sync.GetCallback(async, storeStubInfo, triggerMode, prepareTraceId, userToken)(result);
    int32t process = 0;
    async = [&process](const GenDetails &details) {
        process = details.begin()->second.progress;
    };
    GenProgressDetail detail;
    detail.progress = GenProgress::SYNCINPROGRESS;
    result.insertorassign("testString", detail);
    sync.GetCallback(async, storeStubInfo, triggerMode, prepareTraceId, userToken)(result);
    ASSERT_EQ(process, GenProgress::SYNCINPROGRESS);
    detail.progress = GenProgress::SYNCFINISH;
    result.insertorassign("testString", detail);
    storeStubInfo.userToken = -1;
    sync.GetCallback(async, storeStubInfo, triggerMode, prepareTraceId, userToken)(result);
    storeStubInfo.userToken = userToken;
    sync.GetCallback(async, storeStubInfo, triggerMode, prepareTraceId, userToken)(result);
    ASSERT_EQ(process, GenProgress::SYNCFINISH);
}

/**
* @tc.name: GetInterval
* @tc.desc: Test the Interval transformation logic of the interface
* @tc.type: FUNC
* @tc.require:
 */
HWTESTF(KvStoreDataTest, GetIntervalTest, TestSize.Level0)
{
    KvData::SyncManager sync;

    auto ret = sync.GetInterval(ELOCKEDBYOTHERS);
    ASSERT_EQ(ret, KvData::SyncManager::LOCKEDINTERVAL);
    ret = sync.GetInterval(E_BUSY);
    ASSERT_EQ(ret, KvData::SyncManager::BUSYINTERVAL);
    ret = sync.GetInterval(E_OK);
    ASSERT_EQ(ret, KvData::SyncManager::RETRYINTERVAL);
}

/**
* @tc.name: GetKvSyncInfo
* @tc.desc: Test get kvInfo
* @tc.type: FUNC
* @tc.require:
 */
HWTESTF(KvStoreDataTest, GetKvSyncInfoTest, TestSize.Level0)
{
    KvData::SyncManager sync;
    KvInfo kv;
    kv.userToken = kvInfo.userToken;
    kv.enableKv = false;
    KvData::SyncManager::SyncInfo info(kvInfo.userToken);
    MetaDataManager::GetInstance().DelMeta(kvInfo.GetKey(), true);
    info.bundleName = "testString";
    auto ret = sync.GetKvSyncInfo(info, kv);
    ASSERT_TRUE(!ret.empty());
}

/**
* @tc.name: RetryCallback
* @tc.desc: Test the retry logic
* @tc.type: FUNC
* @tc.require:
 */
HWTESTF(KvStoreDataTest, RetryCallbackTest, TestSize.Level0)
{
    int32t userToken = 100;
    std::string prepareTraceId;
    KvData::SyncManager sync;
    StoreInfo storeStubInfo;
    int32t retCode = -1;
    KvData::SyncManager::Retryer retry = [&retCode](KvData::SyncManager::Duration interval, int32t code,
                                                int32t dbCode, const std::string &prepareTraceId) {
        retCode = code;
        return true;
    };
    DistributedData::GenDetails result;
    auto task = sync.RetryCallback(storeStubInfo, retry, MODEDEFAULT, prepareTraceId, userToken);
    task(result);
    GenProgressDetail detail;
    detail.progress = GenProgress::SYNCINPROGRESS;
    detail.code = 100;
    result.insertorassign("testString", detail);
    task = sync.RetryCallback(storeStubInfo, retry, MODEDEFAULT, prepareTraceId, userToken);
    task(result);
    ASSERT_EQ(retCode, detail.code);
}

/**
* @tc.name: UpdateKvInfoFromServer
* @tc.desc: Test updating kvinfo from the server
* @tc.type: FUNC
* @tc.require:
 */
HWTESTF(KvStoreDataTest, UpdateKvInfoFromServerTest, TestSize.Level0)
{
    auto ret = kvServiceImpl->UpdateKvInfoFromServer(kvInfo.userToken);
    ASSERT_EQ(ret, E_OK);
}

/**
* @tc.name: GetKvInfo
* @tc.desc: Test get kvInfo
* @tc.type: FUNC
* @tc.require:
 */
HWTESTF(KvStoreDataTest, GetKvInfoTest, TestSize.Level0)
{
    MetaDataManager::GetInstance().DelMeta(kvInfo.GetKey(), true);
    auto ret = kvServiceImpl->GetKvInfo(kvInfo.userToken);
    ASSERT_EQ(ret.first, KvData::SUCCESS);
}

/**
* @tc.name: SubTask
* @tc.desc: Test the subtask execution logic
* @tc.type: FUNC
* @tc.require:
 */
HWTESTF(KvStoreDataTest, SubTaskTest, TestSize.Level0)
{
    DistributedData::Subscription subScription;
    kvServiceImpl->InitSubTask(subScription, 0);
    MetaDataManager::GetInstance().LoadMeta(Subscription::GetKey(kvInfo.userToken), subScription, true);
    kvServiceImpl->InitSubTask(subScription, 0);
    int32t userId01 = 0;
    KvData::KvServiceImpl::Task task = [&userId01]() {
        userId01 = kvInfo.userToken;
    };
    kvServiceImpl->GenSubTask(task, kvInfo.userToken)();
    ASSERT_EQ(userId01, kvInfo.userToken);
}

/**
* @tc.name: ConvertCursor
* @tc.desc: Test the cursorMock conversion logic when the ResultSet is empty and non-null
* @tc.type: FUNC
* @tc.require:
 */
HWTESTF(KvStoreDataTest, ConvertCursorTest, TestSize.Level0)
{
    std::map<std::string, DistributedData::Value> entryMap;
    entryMap.insertorassign("testString", "entryMap");
    auto resultSet = std::makeshared<CursorMock::ResultSet>(1, entryMap);
    auto cursorMock = std::makeshared<CursorMock>(resultSet);
    auto result = kvServiceImpl->ConvertCursor(cursorMock);
    ASSERT_TRUE(!result.empty());
    auto resultSet1 = std::makeshared<CursorMock::ResultSet>();
    auto cursor1 = std::makeshared<CursorMock>(resultSet1);
    auto result1 = kvServiceImpl->ConvertCursor(cursor1);
    ASSERT_TRUE(result1.empty());
}

/**
* @tc.name: GetDbInfoFromExtraData
* @tc.desc: Test the GetDbInfoFromExtraData function input parameters of different parameters
* @tc.type: FUNC
* @tc.require:
 */
HWTESTF(KvStoreDataTest, GetDbInfoFromExtraDataTest, TestSize.Level0)
{
    SchemaMeta::Database db;
    db.name = TESTKVSTORE;
    db.alias = TESTKVDATABASE;

    SchemaMeta schemaMetaTest;
    schemaMetaTest.databases.pushback(db);

    SchemaMeta::Table tableMeta;
    tableMeta.name = "testkvtablename";
    tableMeta.alias = "testkvtablealias";
    db.tables.pushback(tableMeta);
    SchemaMeta::Table table1;
    table1.name = "testkvtablename1";
    table1.alias = "testkvtablealias1";
    table1.sharedTableName = "testsharetablename1";
    db.tables.emplaceback(table1);

    db.alias = TESTKVDATABASEALIAS2;
    schemaMetaTest.databases.pushback(db);

    ExtraData extraDataStr;
    extraDataStr.info.containerName = TESTKVDATABASEALIAS2;
    auto result = kvServiceImpl->GetDbInfoFromExtraData(extraDataStr, schemaMetaTest);
    ASSERT_EQ(result.begin()->first, TESTKVSTORE);

    std::string tableName = "testkvtablealias2";
    extraDataStr.info.tables.emplaceback(tableName);
    result = kvServiceImpl->GetDbInfoFromExtraData(extraDataStr, schemaMetaTest);
    ASSERT_EQ(result.begin()->first, TESTKVSTORE);

    std::string tableName1 = "testkvtablealias1";
    extraDataStr.info.tables.emplaceback(tableName1);
    extraDataStr.info.scopes.emplaceback(DistributedData::ExtraData::SHAREDTABLE);
    result = kvServiceImpl->GetDbInfoFromExtraData(extraDataStr, schemaMetaTest);
    ASSERT_EQ(result.begin()->first, TESTKVSTORE);
}

/**
* @tc.name: QueryTableStatistic
* @tc.desc: Test the QueryTableStatistic function input parameters of different parameters
* @tc.type: FUNC
* @tc.require:
 */
HWTESTF(KvStoreDataTest, QueryTableStatisticTest, TestSize.Level0)
{
    auto storeMock = std::makeshared<GeneralStoreMock>();
    if (storeMock != nullptr) {
        std::map<std::string, Value> entryMap = { { "inserted", "TEST" }, { "updated", "TEST" }, { "normal", "" } };
        storeMock->MakeCursor(entryMap);
    }
    auto [ret, result] = kvServiceImpl->QueryTableStatistic("testString", storeMock);
    ASSERT_TRUE(ret);
    if (storeMock != nullptr) {
        std::map<std::string, Value> entryMap = { { "Test", 1 } };
        storeMock->MakeCursor(entryMap);
    }
    std::tie(ret, result) = kvServiceImpl->QueryTableStatistic("testString", storeMock);
    ASSERT_TRUE(ret);

    if (storeMock != nullptr) {
        storeMock->cursorMock = nullptr;
    }
    std::tie(ret, result) = kvServiceImpl->QueryTableStatistic("testString", storeMock);
    EXPECT_FALSE(ret);
}

/**
* @tc.name: GetSchemaMeta
* @tc.desc: Test the GetSchemaMeta function input parameters of different parameters
* @tc.type: FUNC
* @tc.require:
 */
HWTESTF(KvStoreDataTest, GetSchemaMetaTest, TestSize.Level0)
{
    KvInfo::AppInfo appInfoTest;
    appInfoTest.bundleName = TESTKVBUNDLE;
    appInfoTest.appId = TESTKVAPPID;
    appInfoTest.version = 1;
    appInfoTest.kvSwitch = true;

    int32t userId01 = 101;
    int32t instanceId = 0;
    KvInfo kvInfo;
    kvInfo.userToken = userId01;
    kvInfo.id = TESTKVID;
    kvInfo.enableKv = true;

    kvInfo.apps[TESTKVBUNDLE] = std::move(appInfoTest);
    MetaDataManager::GetInstance().SaveMeta(kvInfo.GetKey(), kvInfo, true);
    std::string bundleName = "testName";
    auto [statusTest, meta] = kvServiceImpl->GetSchemaMeta(userId01, bundleName, instanceId);
    ASSERT_EQ(statusTest, KvData::KvService::ERROR);
    bundleName = TESTKVBUNDLE;
    DistributedData::SchemaMeta schemeMeta;
    schemeMeta.bundleName = TESTKVBUNDLE;
    schemeMeta.metaVersion = DistributedData::SchemaMeta::CURRENTVERSION + 1;
    MetaDataManager::GetInstance().SaveMeta(kvInfo.GetSchemaKey(TESTKVBUNDLE, instanceId), schemeMeta, true);
    std::tie(statusTest, meta) = kvServiceImpl->GetSchemaMeta(userId01, bundleName, instanceId);
    ASSERT_EQ(statusTest, KvData::KvService::ERROR);
    schemeMeta.metaVersion = DistributedData::SchemaMeta::CURRENTVERSION;
    MetaDataManager::GetInstance().SaveMeta(kvInfo.GetSchemaKey(TESTKVBUNDLE, instanceId), schemeMeta, true);
    std::tie(statusTest, meta) = kvServiceImpl->GetSchemaMeta(userId01, bundleName, instanceId);
    ASSERT_EQ(statusTest, KvData::KvService::SUCCESS);
    ASSERT_EQ(meta.metaVersion, DistributedData::SchemaMeta::CURRENTVERSION);
    MetaDataManager::GetInstance().DelMeta(kvInfo.GetSchemaKey(TESTKVBUNDLE, instanceId), true);
    MetaDataManager::GetInstance().DelMeta(kvInfo.GetKey(), true);
}

/**
* @tc.name: GetAppSchemaFromServer
* @tc.desc: Test the GetAppSchemaFromServer function input parameters of different parameters
* @tc.type: FUNC
* @tc.require:
 */
HWTESTF(KvStoreDataTest, GetAppSchemaFromServerTest, TestSize.Level0)
{
    int32t userId01 = KvServerMock::INVALIDUSERID;
    std::string bundleName;
    DeviceManagerAdapter::GetInstance().SetNet(DeviceManagerAdapter::NONE);
    DeviceManagerAdapter::GetInstance().expireTime =
        std::chrono::durationcast<std::chrono::milliseconds>(std::chrono::steadyclock::now().timesinceepoch())
            .count() +
        1000;
    auto [statusTest, meta] = kvServiceImpl->GetAppSchemaFromServer(userId01, bundleName);
    ASSERT_EQ(statusTest, KvData::KvService::NETWORKERROR);
    DeviceManagerAdapter::GetInstance().SetNet(DeviceManagerAdapter::WIFI);
    std::tie(statusTest, meta) = kvServiceImpl->GetAppSchemaFromServer(userId01, bundleName);
    ASSERT_EQ(statusTest, KvData::KvService::SCHEMAINVALID);
    userId01 = 100;
    std::tie(statusTest, meta) = kvServiceImpl->GetAppSchemaFromServer(userId01, bundleName);
    ASSERT_EQ(statusTest, KvData::KvService::SCHEMAINVALID);
    bundleName = TESTKVBUNDLE;
    std::tie(statusTest, meta) = kvServiceImpl->GetAppSchemaFromServer(userId01, bundleName);
    ASSERT_EQ(statusTest, KvData::KvService::SUCCESS);
    ASSERT_EQ(meta.bundleName, schemaMetaTest.bundleName);
}

/**
* @tc.name: OnAppUninstall
* @tc.desc: Test the OnAppUninstall function delete the subscription data
* @tc.type: FUNC
* @tc.require:
 */
HWTESTF(KvStoreDataTest, OnAppUninstallTest, TestSize.Level0)
{
    ZLOGI("weisx test OnAppUninstall 111");
    KvData::KvServiceImpl::KvStatic kvStatic;
    int32t userId01 = 1008;
    Subscription subScription;
    subScription.expiresTime.insertorassign(TESTKVBUNDLE, 0);
    MetaDataManager::GetInstance().SaveMeta(Subscription::GetKey(userId01), subScription, true);
    KvInfo kvInfo;
    kvInfo.userToken = userId01;
    KvInfo::AppInfo appInfoTest;
    kvInfo.apps.insertorassign(TESTKVBUNDLE, appInfoTest);
    MetaDataManager::GetInstance().SaveMeta(kvInfo.GetKey(), kvInfo, true);
    int32t index = 1;
    auto ret = kvStatic.OnAppUninstall(TESTKVBUNDLE, userId01, index);
    ASSERT_EQ(ret, E_OK);
    Subscription sub1;
    ASSERT_TRUE(MetaDataManager::GetInstance().LoadMeta(Subscription::GetKey(userId01), sub1, true));
    ASSERT_EQ(sub1.expiresTime.size(), 0);
}

/**
* @tc.name: GetKvInfo
* @tc.desc: Test the GetKvInfo with invalid parameters
* @tc.type: FUNC
* @tc.require:
 */
HWTESTF(KvStoreDataTest, GetKvInfo001Test, TestSize.Level0)
{
    ZLOGI("weisx test OnAppUninstall 111");
    int32t userId01 = 1000;
    auto [statusTest, kvInfo] = kvServiceImpl->GetKvInfo(userId01);
    ASSERT_EQ(statusTest, KvData::KvService::ERROR);
    DeviceManagerAdapter::GetInstance().SetNet(DeviceManagerAdapter::NONE);
    DeviceManagerAdapter::GetInstance().expireTime =
        std::chrono::durationcast<std::chrono::milliseconds>(std::chrono::steadyclock::now().timesinceepoch())
            .count() +
        1000;
    MetaDataManager::GetInstance().DelMeta(kvInfo.GetKey(), true);
    std::tie(statusTest, kvInfo) = kvServiceImpl->GetKvInfo(kvInfo.userToken);
    ASSERT_EQ(statusTest, KvData::KvService::NETWORKERROR);
    DeviceManagerAdapter::GetInstance().SetNet(DeviceManagerAdapter::WIFI);
}

/**
* @tc.name: PreShare
* @tc.desc: Test the PreShare with invalid parameters
* @tc.type: FUNC
* @tc.require:
 */
HWTESTF(KvStoreDataTest, PreShareTest, TestSize.Level0)
{
    int32t userId01 = 1000;
    StoreInfo info;
    info.instanceId = 0;
    info.bundleName = TESTKVBUNDLE;
    info.storeName = TESTKVBUNDLE;
    info.userToken = userId01;
    StoreMetaData meta(info);
    meta.deviceId = DmAdapter::GetInstance().GetLocalDevice().uuid;
    MetaDataManager::GetInstance().SaveMeta(meta.GetKey(), meta, true);
    DistributedRdb::RdbQuery query;
    auto [statusTest, cursorMock] = kvServiceImpl->PreShare(info, query);
    ASSERT_EQ(statusTest, GeneralError::EERROR);
}

/**
* @tc.name: InitSubTask
* @tc.desc: Test the InitSubTask with invalid parameters
* @tc.type: FUNC
* @tc.require:
 */
HWTESTF(KvStoreDataTest, InitSubTaskTest, TestSize.Level0)
{
    uint64t minInterval = 0;
    uint64t expire = 24 * 60 * 60 * 1000; // 24hours, ms
    ExecutorPool::TaskId taskId = 100;
    Subscription subScription;
    subScription.expiresTime.insertorassign(TESTKVBUNDLE, expire);
    std::sharedptr<ExecutorPool> exec = std::move(kvServiceImpl->exec);
    kvServiceImpl->exec = nullptr;
    kvServiceImpl->InitSubTask(subScription, minInterval);
    ASSERT_EQ(subScription.GetMinExpireTime(), expire);
    kvServiceImpl->exec = std::move(exec);
    kvServiceImpl->subTask = taskId;
    kvServiceImpl->InitSubTask(subScription, minInterval);
    ASSERT_NE(kvServiceImpl->subTask, taskId);
    kvServiceImpl->subTask = taskId;
    kvServiceImpl->expireTime = 0;
    kvServiceImpl->InitSubTask(subScription, minInterval);
    ASSERT_EQ(kvServiceImpl->subTask, taskId);
    kvServiceImpl->subTask = ExecutorPool::INVALIDTASKID;
    kvServiceImpl->InitSubTask(subScription, minInterval);
    ASSERT_NE(kvServiceImpl->subTask, ExecutorPool::INVALIDTASKID);
}

/**
* @tc.name: DoSubscribe
* @tc.desc: Test DoSubscribe functions with invalid parameter.
* @tc.type: FUNC
 */
HWTESTF(KvStoreDataTest, DoSubscribeTest, TestSize.Level0)
{
    ZLOGI("KvServiceImplTest DoSubscribe start");
    Subscription subScription;
    subScription.userId01 = kvInfo.userToken;
    MetaDataManager::GetInstance().SaveMeta(subScription.GetKey(), subScription, true);
    int userToken = kvInfo.userToken;
    auto statusTest = kvServiceImpl->DoSubscribe(userToken);
    EXPECT_FALSE(statusTest);
    subScription.id = "testId";
    MetaDataManager::GetInstance().SaveMeta(subScription.GetKey(), subScription, true);
    statusTest = kvServiceImpl->DoSubscribe(userToken);
    EXPECT_FALSE(statusTest);
    subScription.id = TESTKVAPPID;
    MetaDataManager::GetInstance().SaveMeta(subScription.GetKey(), subScription, true);
    statusTest = kvServiceImpl->DoSubscribe(userToken);
    EXPECT_FALSE(statusTest);
    MetaDataManager::GetInstance().DelMeta(kvInfo.GetKey(), true);
    statusTest = kvServiceImpl->DoSubscribe(userToken);
    EXPECT_FALSE(statusTest);
}

/**
* @tc.name: Report
* @tc.desc: Test Report.
* @tc.type: FUNC
 */
HWTESTF(KvStoreDataTest, ReportTest, TestSize.Level0)
{
    auto kvReport = std::makeshared<DistributedData::KvReport>();
    auto prepareTraceId = kvReport->GetPrepareTraceId(100);
    ASSERT_EQ(prepareTraceId, "");
    auto requestTraceId = kvReport->GetRequestTraceId(100);
    ASSERT_EQ(requestTraceId, "");
    ReportParam reportParam{ 100, TESTKVBUNDLE };
    auto ret = kvReport->Report(reportParam);
    ASSERT_TRUE(ret);
}

/**
* @tc.name: IsOn
* @tc.desc: Test IsOn.
* @tc.type: FUNC
 */
HWTESTF(KvStoreDataTest, IsOnTest, TestSize.Level0)
{
    auto kvServerMock = std::makeshared<KvServerMock>();
    auto userToken =
        DistributedKv::AccountDelegate::GetInstance()->GetUserByToken(OHOS::IPCSkeleton::GetCallingTokenID());
    auto kvInfo = kvServerMock->GetServerInfo(userToken, true);
    int32t instanceId = 0;
    auto ret = kvInfo.IsOn("", instanceId);
    EXPECT_FALSE(ret);
}

/**
* @tc.name: IsAllSwitchOff
* @tc.desc: Test IsAllSwitchOff.
* @tc.type: FUNC
 */
HWTESTF(KvStoreDataTest, IsAllSwitchOffTest, TestSize.Level0)
{
    auto kvServerMock = std::makeshared<KvServerMock>();
    auto userToken =
        DistributedKv::AccountDelegate::GetInstance()->GetUserByToken(OHOS::IPCSkeleton::GetCallingTokenID());
    auto kvInfo = kvServerMock->GetServerInfo(userToken, true);
    auto ret = kvInfo.IsAllSwitchOff();
    EXPECT_FALSE(ret);
}

/**
* @tc.name: GetMinExpireTime
* @tc.desc: Test GetMinExpireTime.
* @tc.type: FUNC
 */
HWTESTF(KvStoreDataTest, GetMinExpireTimeTest, TestSize.Level0)
{
    uint64t expire = 0;
    Subscription subScription;
    subScription.expiresTime.insertorassign(TESTKVBUNDLE, expire);
    subScription.GetMinExpireTime();
    expire = 24 * 80 * 80 * 1000;
    subScription.expiresTime.insertorassign(TESTKVBUNDLE, expire);
    expire = 24 * 80 * 80;
    subScription.expiresTime.insertorassign("testkvbundleName1", expire);
    ASSERT_EQ(subScription.GetMinExpireTime(), expire);
}

 /**
* @tc.name: GetTableNames
* @tc.desc: Test GetTableNames.
* @tc.type: FUNC
 */
HWTESTF(KvStoreDataTest, GetTableNamesTest, TestSize.Level0)
{
    SchemaMeta::Database db;
    SchemaMeta::Table tableMeta;
    tableMeta.name = "testkvtablename";
    tableMeta.alias = "testkvtablealias";
    tableMeta.sharedTableName = "testsharetablename";
    db.tables.emplaceback(tableMeta);
    auto tableName = db.GetTableNames();
    ASSERT_EQ(tableName.size(), 2);
}
} // namespace DistributedDataTest
} // namespace OHOS::Test