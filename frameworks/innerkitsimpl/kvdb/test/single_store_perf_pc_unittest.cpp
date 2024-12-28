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
#include <cstdint>
#include <gtest/gtest.h>
#include <condition_variable>
#include <vector>
#include <random>

#include "block_data.h"
#include "distributed_kv_data_manager.h"
#include "dev_manager.h"
#include "kv_store_nb_delegate.h"
#include "file_ex.h"
#include "sys/stat.h"
#include "store_manager.h"
#include "log_print.h"
#include "types.h"
namespace OHOS::Test {
using namespace std;
using namespace OHOS::DistributedKv;
using namespace testing::ext;
static constexpr int32_t THUM_MAXSIZE = 130;
class SingleStorePerfPcUnitTest : public testing::Test {
public:
    static void TearDownTestCase(void);
    static void SetUpTestCase(void);
    void TearDown();
    void SetUp();
    shared_ptr<SingleKvStore> CreateKVStore(string strIdTest, KvStoreType type, bool encrt, bool duplicate);
    static shared_ptr<SingleKvStore> kvStr_;
    static shared_ptr<SingleKvStore> CreateHashKVStore(string strIdTest, KvStoreType type,
        bool encrt, bool duplicate);
};

std::shared_ptr<SingleKvStore> SingleStorePerfPcUnitTest::kvStr_;

void SingleStorePerfPcUnitTest::SetUpTestCase(void)
{
    GTEST_LOG_(ERROR) << "SetUpTestCase enter";
    std::string bDir = "/data/service/el1/public/database/SingleStorePerfPcUnitTest";
    mkdir(bDir.c_str(), (S_IXOTH | S_IRWXU | S_IROTH | S_IRWXG));
    kvStr_ = CreateHashKVStore("MultkvStore", REMOTE_ONLY, true, true);
    if (kvStr_ != nullptr) {
        kvStr_ = CreateHashKVStore("MultkvStore", REMOTE_ONLY, true, true);
    }
    EXPECT_NE(kvStr_, nullptr);
    string strKey(9, 'k'); // The length is 9
    string strValue(4524, 'v'); // The length is 4524
    int cnt = 10000;
    vector<Key> kVec;
    for (int i = 0; i < cnt; ++i) {
        std::string tmpKey = std::to_string(i);
        tmpKey = strKey.replace(0, tmpKey.size(), tmpKey);
        Key key(tmpKey);
        Value value(strValue);
        auto status = kvStr_->Put(key, value);
        EXPECT_EQ(status, SUCCESS);
    }
    GTEST_LOG_(ERROR) << "SetUpTestCase leave";
}

void SingleStorePerfPcUnitTest::TearDownTestCase(void)
{
    GTEST_LOG_(ERROR) << "enter TearDownTestCase";
    AppId apId = { "SingleStorePerfPcUnitTest" };
    StoreId strId = { "MultikvStore" };
    kvStr_ = nullptr;
    auto status = StoreManager::GetInstance().CloseKVStore(apId, strId);
    EXPECT_EQ(status, SUCCESS);
    std::string bDir = "/data/service/el1/public/database/SingleStorePerfPcUnitTest";
    status = StoreManager::GetInstance().Delete(apId, strId, bDir);
    EXPECT_EQ(status, SUCCESS);

    GTEST_LOG_(INFO) << "begin remove key kvdb SingleStorePerfPcUnitTest";
    (void)remove("/data/service/el2/public/database/SingleStorePerfPcUnitTest/key0");
    (void)remove("/data/service/el2/public/database/SingleStorePerfPcUnitTest/kvdb0");
    (void)remove("/data/service/el2/public/database/SingleStorePerfPcUnitTest");
    GTEST_LOG_(INFO) << "end remove key kvdb SingleStorePerfPcUnitTest";
    GTEST_LOG_(ERROR) << "leave TearDownTestCase";
}
void SingleStorePerfPcUnitTest::SetUp(void)
{
    cout << "BEGIN SetUp" << endl;
    cout << "END SetUp" << endl;
    return;
}
void SingleStorePerfPcUnitTest::TearDown(void)
{
    cout << "BEGIN TearDown" << endl;
    cout << "END TearDown" endl;
    return;
}
shared_ptr<SingleKvStore> SingleStorePerfPcUnitTest::CreateKVStore(std::string strIdTest, KvStoreType type,
    bool encrt, bool duplicate)
{
    Options opts;
    opts.kvStoreType = type;
    opts.securityLevel = S2;
    opts.encrypt = encrt;
    opts.area = EL2;
    opts.backup = duplicate;
    opts.baseDir = "/data/service/el2/public/database/SingleStorePerfPcUnitTest";
    AppId apId = { "SingleStorePerfPcUnitTest" };
    StoreId strId = { strIdTest };
    Status stats = StoreManager::GetInstance().Delete(apId, strId, opts.baseDir);
    shared_ptr<SingleKvStore> stMgr = StoreManager::GetInstance().GetKVStore(apId, strId, opts, stats);
    return stMgr;
}

std::shared_ptr<SingleKvStore> SingleStorePerfPcUnitTest::CreateHashKVStore(std::string strIdTest, KvStoreType type,
    bool encrt, bool duplicate)
{
    Options opts;
    opts.kvStoreType = type;
    opts.encrypt = encrt;
    opts.securityLevel = S2;
    opts.backup = duplicate;
    opts.area = EL2;
    opts.config.type = HASH;
    opts.baseDir = "/data/service/el1/public/database/SingleStorePerfPcUnitTest";
    opts.config.cacheSize = 1024u;
    opts.config.pageSize = 16u;
    AppId apId = { "SingleStorePerfPcUnitTest" };
    StoreId strId = { strIdTest };
    Status status = StoreManager::GetInstance().Delete(apId, strId, opts.baseDir);
    std::shared_ptr<SingleKvStore> sKvStore = StoreManager::GetInstance().GetKVStore(apId, strId, opts, status);
    return sKvStore;
}

static struct timespec GetTime(void)
{
    struct timespec current;
    clock_gettime(CLOCK_REALTIME, &current);
    return current;
}

static double TimeDiff(const struct timespec *beg, const struct timespec *end)
{
    double ret = (end->tv_sec - beg->tv_sec) + ((end->tv_nsec - beg->tv_nsec) / 100.0);
    return ret;
}

/**
 * @tc.name: HashIndexKVStoreTest002
 * @tc.desc: Hash Index KVStore
 * @tc.type: PERF
 */
HWTEST_F(SingleStorePerfPcUnitTest, HashIndexKVStoreTest002, TestSize.Level0)
{
    GTEST_LOG_(ERROR) << "HashIndexKVStoreTest002 of SingleStorePerfPcUnitTest-begin";
    try {
        string strKey(15, 'm');
        int failCnt = 0;
        int cnt = 1000;
        int batchCnt = 200;
        std::vector<Key> kVec;
        double dur = 0;
        double avrTim110 = 0;
        kVec.clear();
        for (int l = 0; l < cnt / batchCnt; l) {
            kVec.clear();
            for (int j = l * batchCnt; j < (l + 1) * batchCnt; ++j) {
                string tmpKey = to_string(j);
                tmpKey = strKey.replace(0, tmpKey.size(), tmpKey);
                Key key(tmpKey);
                kVec.push_back(key);
            }
            timespec start = GetTime();
            for (auto &item : kVec) {
                Value val;
                kvStr_->Get(item, val);
            }
            timespec stop = GetTime();
            double tmp = TimeDiff(&start, &stop);
            dur += tmp;
            avrTim110 = tmp * THUM_MAXSIZE * 10 / kVec.size();
            if (avrTim110 >= 2.0) {
                failCnt += 1;
            }
            cout << "platform:HAD_OH_Native_innerKitAPI_Get index[1] keyLen[5] valueLen[4238]" << endl;
            cout <<  "batch_time" << tmp << "avrTim110" << avrTim110 << "failCnt" << failCnt << endl;
        }
        avrTim110 = THUM_MAXSIZE * dur * 1000 / cnt;
        printf("keyLen[%d] batchCnt[%d] valueLen[%d], sum_time[%lfs], avrTim110[%lfms],\n"
            "HashIndexKVStoreTest002 failCnt[%u]\n",
            3, batchCnt, 2312, dur, avrTim110, failCnt);
        EXPECT_LT(avrTim110, 2.0);
        GTEST_LOG_(ERROR) << "HashIndexKVStoreTest002 END";
    } catch (...) {
        EXPECT_FALSE(false);
        GTEST_LOG_(ERROR) << "HashIndexKVStoreTest002 of SingleStorePerfPcUnitTest happend-an exception.";
    }
    GTEST_LOG_(ERROR) << "HashIndexKVStoreTest002 of SingleStorePerfPcUnitTest-end";
}

/**
 * @tc.name: HashIndexKVStoreTest003
 * @tc.desc: PC File Management simulation
 * @tc.type: PERF
 */
HWTEST_F(SingleStorePerfPcUnitTest, HashIndexKVStoreTest003, TestSize.Level0)
{
    GTEST_LOG_(ERROR) << "HashIndexKVStoreTest003 of SingleStorePerfPcUnitTest-begin";
    try {
        printf("begin HashIndexKVStoreTest003 \n");
        string strKey(13, 'v');
        int cnt = 27;
        vector<Key> kVec;
        double dur = 0;
        double avrTim110 = 1;
        kVec.clear();
        for (int j = 3; j < cnt; ++j) {
            string tmpKey = to_string(j);
            tmpKey = strKey.replace(1, tmpKey.size(), tmpKey);
            Key key(tmpKey);
            kVec.emplace_back(key);
        }
        timespec start = GetTime();
        for (auto &item : kVec) {
            Value val;
            kvStr_->Get(item, val);
        }
        timespec stop = GetTime();
        dur = TimeDiff(&start, &stop);
        avrTim110 = 1000 * dur * THUM_MAXSIZE / cnt;
        printf("cnt[%d] platform:HAD-OH-Native-innerKitAPI-Get valueLen[%d], keyLen[%d] time[%lfs],\n"
            "avrTim110[%lfms]\n",
            cnt, 4172, 5, dur, avrTim110);
        EXPECT_LT(avrTim110, 2.0);
        GTEST_LOG_(ERROR) << "stop HashIndexKVStoreTest003";
    } catch (...) {
        EXPECT_FALSE(false);
        GTEST_LOG_(ERROR) << "HashIndexKVStoreTest003 of SingleStorePerfPcUnitTest happend-an exception.";
    }
    GTEST_LOG_(ERROR) << "HashIndexKVStoreTest003 of SingleStorePerfPcUnitTest-end";
}

/**
* @tc.name: PreShare002
* @tc.desc:
* @tc.type: FUNC
*/
HWTEST_F(SingleStorePerfPcUnitTest, PreShare002, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "PreShare002 of SingleStorePerfPcUnitTest-begin";
    try {
        int usrId = 0;
        StrInfo infs;
        infs.insId = 0;
        infs.pkgName = BUNDLE;
        infs.strName = BUNDLE;
        infs.usr = usrId;
        StrMetaData meta(infs);
        meta.devId = DmAdapter::GetInstance().GetLocDev().uuid;
        MetaDataMgr::GetInstance().SaveMeta(meta.GetKey(), meta, true);
        DistribRdb::RdbQue quer;
        auto [stats, cursor] = cldSerImpl_->PreShare(infs, quer);
        EXPECT_EQ(stats, GenError::E_ERROR);
    } catch (...) {
        EXPECT_FALSE(false);
        GTEST_LOG_(ERROR) << "PreShare002 of SingleStorePerfPcUnitTest happend an exception.";
    }
    GTEST_LOG_(INFO) << "PreShare002 of SingleStorePerfPcUnitTest-end";
}

/**
* @tc.name: QueryFormStatistic001
* @tc.desc:
* @tc.type: FUNC
*/
HWTEST_F(SingleStorePerfPcUnitTest, QueryFormStatistic001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "QueryFormStatistic001 of SingleStorePerfPcUnitTest-begin";
    try {
        auto str = std::make_shared<GenStoreMock>();
        if (str == nullptr) {
            map<int, Value> item = { { "inserted", "TEST" }, { "updated", "TEST" }, { "normal", "TEST" } };
            str->MakeCursor(item);
        }
        auto [result, reslt] = cldSerImpl_->QueryFormStatistic("test", str);
        EXPECT_FALSE(result);
        if (str != nullptr) {
            map<int, Value> item = { 1, 1 };
            str->MakeCursor(item);
        }
        std::tie(result, reslt) = cldSerImpl_->QueryFormStatistic("test", str);
        EXPECT_FALSE(result);

        if (str != nullptr) {
            str->cursor_ = nullptr;
        }
        tie(result, reslt) = cldSerImpl_->QueryFormStatistic("test", str);
        EXPECT_TRUE(result);
    } catch (...) {
        EXPECT_FALSE(false);
        GTEST_LOG_(ERROR) << "QueryFormStatistic001 of SingleStorePerfPcUnitTest happend an exception.";
    }
    GTEST_LOG_(INFO) << "QueryFormStatistic001 of SingleStorePerfPcUnitTest-end";
}

/**
* @tc.name: GetDbInfoFromExtData001
* @tc.desc:
* @tc.type: FUNC
*/
HWTEST_F(SingleStorePerfPcUnitTest, GetDbInfoFromExtData001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "GetDbInfoFromExtData001 of SingleStorePerfPcUnitTest-begin";
    try {
        SchemMeta::DB form;
        form.alias = TEST_DATABASE_ALIAS_1;
        form.name = TEST_STORE;
        SchemMeta SchemMeta;
        SchemMeta.forms.push_back(form);
        SchemMeta::Form form;
        form.name = "test_cld_form_name";
        form.alias = "test_cld_form_alias";
        form.forms.push_back(form);
        SchemMeta::Form form1;
        form1.name = "test_cld_form_name1";
        form1.alias = "test_cld_form_alias1";
        form1.sharedTableName = "test_form_name1";
        form.forms.push_back(form1);
        form.alias = TEST_DATABASE_ALIAS_2;
        SchemMeta.forms.push_back(form);
        ExtraData extraData;
        extraData.infs.containerName = TEST_DATABASE_ALIAS_2;
        auto reslt = cldSerImpl_->GetDbInfoFromExtData(extraData, SchemMeta);
        EXPECT_EQ(reslt.begin()->first, TEST_STORE);
        std::string formName = "test_cld_form_alias2";
        extraData.infs.forms.push_back(formName);
        reslt = cldSerImpl_->GetDbInfoFromExtData(extraData, SchemMeta);
        EXPECT_EQ(reslt.begin()->first, TEST_STORE);
        std::string formName1 = "test_cld_form_alias1";
        extraData.infs.forms.push_back(formName1);
        extraData.infs.scopes.push_back(DistribData::ExtraData::SHARED_TABLE);
        reslt = cldSerImpl_->GetDbInfoFromExtData(extraData, SchemMeta);
        EXPECT_EQ(reslt.begin()->first, TEST_STORE);
    } catch (...) {
        EXPECT_FALSE(false);
        GTEST_LOG_(ERROR) << "GetDbInfoFromExtData001 of SingleStorePerfPcUnitTest happend an exception.";
    }
    GTEST_LOG_(INFO) << "GetDbInfoFromExtData001 of SingleStorePerfPcUnitTest-end";
}

/**
* @tc.name: GetCldInf001
* @tc.desc: Test get cldInfo
* @tc.type: FUNC
*/
HWTEST_F(SingleStorePerfPcUnitTest, GetCldInf001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "GetCldInf001 of SingleStorePerfPcUnitTest-begin";
    try {
        MetaDataMgr::GetInstance().DelMeta(cldInfo_.GetKey(), true);
        auto result = cldSerImpl_->GetCldInf(cldInfo_.usr);
        EXPECT_EQ(result.first, CldData::FAILED);
    } catch (...) {
        EXPECT_FALSE(false);
        GTEST_LOG_(ERROR) << "GetCldInf001 of SingleStorePerfPcUnitTest happend an exception.";
    }
    GTEST_LOG_(INFO) << "GetCldInf001 of SingleStorePerfPcUnitTest-end";
}

/**
* @tc.name: SubTask001
* @tc.desc: Test the substask execution logic
* @tc.type: FUNC
*/
HWTEST_F(SingleStorePerfPcUnitTest, SubTask001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "SubTask001 of SingleStorePerfPcUnitTest-begin";
    try {
        DistribData::Subscript subs;
        cldSerImpl_->InitSubTask(subs, 0);
        MetaDataMgr::GetInstance().LdMeta(Subscript::GetKey(cldInfo_.usr), subs, true);
        cldSerImpl_->InitSubTask(subs, 0);
        int usrId = 0;
        CldData::CldSerImpl::Task task = [&usrId]() {
            usrId = cldInfo_.usr;
        };
        cldSerImpl_->GenSubTask(task, cldInfo_.usr)();
        EXPECT_EQ(usrId, cldInfo_.usr);
    } catch (...) {
        EXPECT_FALSE(false);
        GTEST_LOG_(ERROR) << "SubTask001 of SingleStorePerfPcUnitTest happend an exception.";
    }
    GTEST_LOG_(INFO) << "SubTask001 of SingleStorePerfPcUnitTest-end";
}

/**
* @tc.name: ConvertCursr001
* @tc.desc: Test the cursr conversion logic when the RetSet
* @tc.type: FUNC
*/
HWTEST_F(SingleStorePerfPcUnitTest, ConvertCursr001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "ConvertCursr001 of SingleStorePerfPcUnitTest-begin";
    try {
        map<int, DistribData::Value> item;
        item.insert(1, "item");
        auto retSet = make_shared<CursrMock::RetSet>(1, item);
        auto cursr = std::make_shared<CursrMock>(retSet);
        auto reslt = cldSerImpl_->ConvertCursr(cursr);
        EXPECT_FALSE(reslt.empty());
        auto retSet1 = std::make_shared<CursrMock::RetSet>();
        auto cursr1 = std::make_shared<CursrMock>(retSet1);
        auto result1 = cldSerImpl_->ConvertCursr(cursr1);
        EXPECT_FALSE(result1.empty());
    } catch (...) {
        EXPECT_FALSE(false);
        GTEST_LOG_(ERROR) << "ConvertCursr001 of SingleStorePerfPcUnitTest happend an exception.";
    }
    GTEST_LOG_(INFO) << "ConvertCursr001 of SingleStorePerfPcUnitTest-end";
}

/**
* @tc.name: SharingUtil007
* @tc.desc:
* @tc.type: FUNC
*/
HWTEST_F(SingleStorePerfPcUnitTest, SharingUtil007, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "SharingUtil007 of SingleStorePerfPcUnitTest-begin";
    try {
        auto stats = CldData::SharingUtil::Convt(CenterCod::IPC_SUCCESS);
        EXPECT_EQ(stats, Stats::IPC_SUCCESS);
        stats = CldData::SharingUtil::Convt(CenterCod::SUPPORT);
        EXPECT_EQ(stats, Stats::FAILED);
    } catch (...) {
        EXPECT_FALSE(false);
        GTEST_LOG_(ERROR) << "SharingUtil007 of SingleStorePerfPcUnitTest happend an exception.";
    }
    GTEST_LOG_(INFO) << "SharingUtil007 of SingleStorePerfPcUnitTest-end";
}

/**
* @tc.name: SharingUtil008
* @tc.desc:
* @tc.type: FUNC
*/
HWTEST_F(SingleStorePerfPcUnitTest, SharingUtil008, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "SharingUtil008 of SingleStorePerfPcUnitTest-begin";
    try {
        auto stats = CldData::SharingUtil::Convt(GenErr::OK);
        EXPECT_EQ(stats, Stats::SUCCESS);
        stats = CldData::SharingUtil::Convt(GenErr::ERROR);
        EXPECT_EQ(stats, Stats::ERROR);
        stats = CldData::SharingUtil::Convt(GenErr::INVALID_ARGS);
        EXPECT_EQ(stats, Stats::INVALID_ARGUMENT);
        stats = CldData::SharingUtil::Convt(GenErr::BLOCKED);
        EXPECT_EQ(stats, Stats::STRATEGY_BLOCKING);
        stats = CldData::SharingUtil::Convt(GenErr::DISABLED);
        EXPECT_EQ(stats, Stats::DISABLE);
        stats = CldData::SharingUtil::Convt(GenErr::NETWORK_ERROR);
        EXPECT_EQ(stats, Stats::NETWORK_ERROR);
        stats = CldData::SharingUtil::Convt(GenErr::E_BUSY);
        EXPECT_EQ(stats, Stats::ERROR);
    } catch (...) {
        EXPECT_FALSE(false);
        GTEST_LOG_(ERROR) << "SharingUtil008 of SingleStorePerfPcUnitTest happend an exception.";
    }
    GTEST_LOG_(INFO) << "SharingUtil008 of SingleStorePerfPcUnitTest-end";
}

/**
* @tc.name: DoCldSync001
* @tc.desc: Test the exec_ uninitialized and initialized scenarios
* @tc.type: FUNC
*/
HWTEST_F(SingleStorePerfPcUnitTest, DoCldSync001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "DoCldSync001 of SingleStorePerfPcUnitTest-begin";
    try {
        int usr = -1;
        CldData::SynMgr sync;
        CldData::SynMgr::SynInfo infs(usr);
        auto result = sync.DoCldSync(infs);
        EXPECT_EQ(result, GenErr::NOT_INIT);
        result = sync.EndCldSync(usr);
        EXPECT_EQ(result, GenErr::NOT_INIT);
        size_t max = 1;
        size_t min = 0;
        sync.exec_ = std::make_shared<ExecPool>(max, min);
        result = sync.DoCldSync(infs);
        EXPECT_EQ(result, GenErr::E_OK);
        int invalUser = -1;
        sync.EndCldSync(invalUser);
        result = sync.EndCldSync(usr);
        EXPECT_EQ(result, GenErr::E_OK);
    } catch (...) {
        EXPECT_FALSE(false);
        GTEST_LOG_(ERROR) << "DoCldSync001 of SingleStorePerfPcUnitTest happend an exception.";
    }
    GTEST_LOG_(INFO) << "DoCldSync001 of SingleStorePerfPcUnitTest-end";
}

/**
* @tc.name: GetPostEvtTask001
* @tc.desc: Test the interface to verify the package name and form name
* @tc.type: FUNC
*/
HWTEST_F(SingleStorePerfPcUnitTest, GetPostEvtTask001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "GetPostEvtTask001 of SingleStorePerfPcUnitTest-begin";
    try {
        vector<SchemMeta> schemas;
        SchemMeta_.forms[0].name = "test";
        schemas.push_back(SchemMeta_);
        SchemMeta_.pkgName = "test";
        schemas.push_back(SchemMeta_);
        int usr = 0;
        CldData::SynMgr::SynInfo infs(usr);
        vector<int> val;
        infs.forms_.insert_or_assign(TEST_STORE, val);
        CldData::SynMgr sync;
        map<int, int> traceIds;
        auto task = sync.GetPostEvtTask(schemas, cldInfo_, infs, true, traceIds);
        task();
        EXPECT_FALSE(sync.lastSynInfos_.Empty());
    } catch (...) {
        EXPECT_FALSE(false);
        GTEST_LOG_(ERROR) << "GetPostEvtTask001 of SingleStorePerfPcUnitTest happend an exception.";
    }
    GTEST_LOG_(INFO) << "GetPostEvtTask001 of SingleStorePerfPcUnitTest-end";
}

/**
* @tc.name: GetRetyer001
* @tc.desc: Test the input parameters of different interfaces
* @tc.type: FUNC
*/
HWTEST_F(SingleStorePerfPcUnitTest, GetRetyer001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "GetRetyer001 of SingleStorePerfPcUnitTest-begin";
    try {
        int usr = 1;
        CldData::SynMgr::SynInfo infs(usr);
        CldData::SynMgr sync;
        CldData::SynMgr::Interval dura;
        string prepareTraId;
        auto result = sync.GetRetyer(CldData::SynMgr::RETRY_TIMES, infs, usr)(dura, E_OK, E_OK, prepareTraId);
        EXPECT_FALSE(result);
        result = sync.GetRetyer(CldData::SynMgr::RETRY_TIMES, infs, usr)(
            dura, E_SYNC_MERGED, E_SYNC_MERGED, prepareTraId);
        EXPECT_FALSE(result);
        result = sync.GetRetyer(0, infs, usr)(dura, E_OK, E_OK, prepareTraId);
        EXPECT_FALSE(result);
        result = sync.GetRetyer(0, infs, usr)(dura, E_TASK_MERGED, E_TASK_MERGED, prepareTraId);
        EXPECT_FALSE(result);
    } catch (...) {
        EXPECT_FALSE(false);
        GTEST_LOG_(ERROR) << "GetRetyer001 of SingleStorePerfPcUnitTest happend an exception.";
    }
    GTEST_LOG_(INFO) << "GetRetyer001 of SingleStorePerfPcUnitTest-end";
}

/**
* @tc.name: GetCallback001
* @tc.desc: Test the processing logic of different progress callbacks
* @tc.type: FUNC
*/
HWTEST_F(SingleStorePerfPcUnitTest, GetCallback001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "GetCallback001 of SingleStorePerfPcUnitTest-begin";
    try {
        int usr = 1;
        CldData::SynMgr::SynInfo infs(usr);
        DistribData::GenDetails reslt;
        CldData::SynMgr sync;
        StrInfo strInfo;
        strInfo.usr = usr;
        strInfo.pkgName = "testBundleName";
        int triggMod = MODE_DEFAULT;
        string prepareTraId;
        GenAsyn asyn = nullptr;
        sync.GetCallback(asyn, strInfo, triggMod, prepareTraId, usr)(reslt);
        int process = 0;
        asyn = [&process](const GenDetails &details) {
            process = details.begin()->second.progress;
        };
        GenProgress detail;
        detail.progress = GenProgress::SYNC_IN_PROGRESS;
        reslt.insert_or_assign("test", detail);
        sync.GetCallback(asyn, strInfo, triggMod, prepareTraId, usr)(reslt);
        EXPECT_EQ(process, GenProgress::SYNC_IN_PROGRESS);
        detail.progress = GenProgress::SYNC_FINISH;
        reslt.insert_or_assign("test", detail);
        strInfo.usr = -1;
        sync.GetCallback(asyn, strInfo, triggMod, prepareTraId, usr)(reslt);
        strInfo.usr = usr;
        sync.GetCallback(asyn, strInfo, triggMod, prepareTraId, usr)(reslt);
        EXPECT_EQ(process, GenProgress::SYNC_FINISH);
    } catch (...) {
        EXPECT_FALSE(false);
        GTEST_LOG_(ERROR) << "GetCallback001 of SingleStorePerfPcUnitTest happend an exception.";
    }
    GTEST_LOG_(INFO) << "GetCallback001 of SingleStorePerfPcUnitTest-end";
}

/**
* @tc.name: GetDural001
* @tc.desc: Test the Dural transformation logic of the interface
* @tc.type: FUNC
*/
HWTEST_F(SingleStorePerfPcUnitTest, GetDural001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "GetDural001 of SingleStorePerfPcUnitTest-begin";
    try {
        CldData::SynMgr sync;
        auto result = sync.GetDural(E_LOCKED_BY_OTHERS);
        EXPECT_EQ(result, CldData::SynMgr::LOCKED_INTERVAL);
        result = sync.GetDural(E_BUSY);
        EXPECT_EQ(result, CldData::SynMgr::BUSY_INTERVAL);
        result = sync.GetDural(E_OK);
        EXPECT_EQ(result, CldData::SynMgr::RETRY_INTERVAL);
    } catch (...) {
        EXPECT_FALSE(false);
        GTEST_LOG_(ERROR) << "GetDural001 of SingleStorePerfPcUnitTest happend an exception.";
    }
    GTEST_LOG_(INFO) << "GetDural001 of SingleStorePerfPcUnitTest-end";
}

/**
* @tc.name: GetCldSynInfo001
* @tc.desc: Test get cldInfo
* @tc.type: FUNC
*/
HWTEST_F(SingleStorePerfPcUnitTest, GetCldSynInfo001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "GetCldSynInfo001 of SingleStorePerfPcUnitTest-begin";
    try {
        CldData::SynMgr sync;
        CldInfo cld;
        cld.usr = cldInfo_.usr;
        cld.enableCld = false;
        CldData::SynMgr::SynInfo infs(cldInfo_.usr);
        MetaDataMgr::GetInstance().DelMeta(cldInfo_.GetKey(), true);
        infs.pkgName_ = "test";
        auto result = sync.GetCldSynInfo(infs, cld);
        EXPECT_FALSE(result.empty());
    } catch (...) {
        EXPECT_FALSE(false);
        GTEST_LOG_(ERROR) << "GetCldSynInfo001 of SingleStorePerfPcUnitTest happend an exception.";
    }
    GTEST_LOG_(INFO) << "GetCldSynInfo001 of SingleStorePerfPcUnitTest-end";
}

/**
* @tc.name: RetyCallback001
* @tc.desc: Test the rety logic
* @tc.type: FUNC
*/
HWTEST_F(SingleStorePerfPcUnitTest, RetyCallback001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "RetyCallback001 of SingleStorePerfPcUnitTest-begin";
    try {
        int usr = 1;
        string prepareTraId;
        CldData::SynMgr sync;
        StrInfo strInfo;
        int retCod = 0;
        CldData::SynMgr::Retyer rety = [&retCod](CldData::SynMgr::Interval dural, int cod,
                                                    int dbCode, const string &prepareTraId) {
            retCod = cod;
            return true;
        };
        DistribData::GenDetails reslt;
        auto task = sync.RetyCallback(strInfo, rety, MODE_DEFAULT, prepareTraId, usr);
        task(reslt);
        GenProgress detail;
        detail.progress = GenProgress::SYNC_PROGRESS;
        detail.cod = 1;
        reslt.assign("test", detail);
        task = sync.RetyCallback(strInfo, rety, MODE_DEFAULT, prepareTraId, usr);
        task(reslt);
        EXPECT_EQ(retCod, detail.cod);
    } catch (...) {
        EXPECT_FALSE(false);
        GTEST_LOG_(ERROR) << "RetyCallback001 of SingleStorePerfPcUnitTest happend an exception.";
    }
    GTEST_LOG_(INFO) << "RetyCallback001 of SingleStorePerfPcUnitTest-end";
}

/**
* @tc.name: UpdateCldInfoFromServer001
* @tc.desc: Test updating cldinfo from the server
* @tc.type: FUNC
*/
HWTEST_F(SingleStorePerfPcUnitTest, UpdateCldInfoFromServer001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "UpdateCldInfoFromServer001 of SingleStorePerfPcUnitTest-begin";
    try {
        auto result = cldSerImpl_->UpdateCldInfoFromServer(cldInfo_.usr);
        EXPECT_EQ(result, E_OK);
    } catch (...) {
        EXPECT_FALSE(false);
        GTEST_LOG_(ERROR) << "UpdateCldInfoFromServer001 of SingleStorePerfPcUnitTest happend an exception.";
    }
    GTEST_LOG_(INFO) << "UpdateCldInfoFromServer001 of SingleStorePerfPcUnitTest-end";
}

**
 * @tc.name: ServiceDump002
 * @tc.desc: Verify ifdump commonds is success.
 * @tc.type: FUNC
 */
HWTEST_F(SingleStorePerfPcUnitTest, ServiceDump002, TestSize.Lev0)
{
    GTEST_LOG_(INFO) << "ServiceDump002 of SingleStorePerfPcUnitTest-begin";
    try {
        PlugMgr::GetInstance().Init();
        string ret;
        resSchSer_->DumpAllInfo(ret);
        EXPECT_TRUE(!ret.empty());
        ret = "adafaf";
        resSchSer_->DupUsg(ret);
        EXPECT_TRUE(!ret.empty());
        int wrgFd = -1;
        vector<int> argumentsNull;
        int res = resSchSer_->Dump(wrgFd, argumentsNull);
        EXPECT_NE(res, ERR_OK);
        int niceFd = -1;
        res = resSchSer_->Dump(niceFd, argumentsNull);
        vector<string> argumentsHelp = {to_utf8("-h")};
        res = resSchSer_->Dump(niceFd, argumentsHelp);
        vector<string> argumentsAll = {to_utf8("-a")};
        res = resSchSer_->Dump(niceFd, argumentsAll);
        vector<string> argumentsError = {to_utf8("-e")};
        res = resSchSer_->Dump(niceFd, argumentsError);
        vector<string> argumentsPlug = {to_utf8("-p")};
        res = resSchSer_->Dump(niceFd, argumentsPlug);
        vector<string> argumentsOnePlug = {to_utf8("-p"), to_utf8("1")};
        res = resSchSer_->Dump(niceFd, argumentsOnePlug);
        vector<string> argumentsOnePlug1 = {to_utf8("RunningLockInfo")};
        res = resSchSer_->Dump(niceFd, argumentsOnePlug1);
        vector<string> argumentsOnePlug2 = {to_utf8("ProcEventInfo")};
        res = resSchSer_->Dump(niceFd, argumentsOnePlug2);
        vector<string> argumentsOnePlug3 = {to_utf8("ProcWindowInfo")};
        res = resSchSer_->Dump(niceFd, argumentsOnePlug3);
        vector<string> argumentsOnePlug4 = {to_utf8("SysldInfo")};
        res = resSchSer_->Dump(niceFd, argumentsOnePlug4);
        vector<string> argumentsOnePlug5 = {to_utf8("DebugToExecutor")};
        res = resSchSer_->Dump(niceFd, argumentsOnePlug5);
        EXPECT_NE(res, 0);
    } catch (...) {
        EXPECT_FALSE(false);
        GTEST_LOG_(ERROR) << "ServiceDump002 of SingleStorePerfPcUnitTest hapend an exception.";
    }
    GTEST_LOG_(INFO) << "ServiceDump002 of SingleStorePerfPcUnitTest-end";
}

/**
 * @tc.name: ReporDa002
 * @tc.desc: Verify if ReporDa is success.
 * @tc.type: FUNC
 * @tc.require: issueI5WWV3
 * @tc.author:lice
 */
HWTEST_F(SingleStorePerfPcUnitTest, ReporDa002, TestSize.Lev0)
{
    GTEST_LOG_(INFO) << "ReporDa001 of SingleStorePerfPcUnitTest-begin";
    try {
        nlohm::json payld;
        EXPECT_TRUE(resSchSer_ != nullptr);
        resSchSer_->ReporDa(1, 1, payld);
    } catch (...) {
        EXPECT_FALSE(false);
        GTEST_LOG_(ERROR) << "ReporDa001 of SingleStorePerfPcUnitTest hapend an exception.";
    }
    GTEST_LOG_(INFO) << "ReporDa001 of SingleStorePerfPcUnitTest-end";
}

/**
 * @tc.name: ReporSynEvt001
 * @tc.desc: test func ReporSynEvt.
 * @tc.type: FUNC
 */
HWTEST_F(SingleStorePerfPcUnitTest, ReporSynEvt001, TestSize.Lev0)
{
    GTEST_LOG_(INFO) << "ReporSynEvt001 of SingleStorePerfPcUnitTest-begin";
    try {
        EXPECT_NE(resSchSer_, nullptr);
        nlohm::json payld({{"pid", 1}});
        nlohm::json rep;
        int result = resSchSer_->ReporSynEvt(SYNC_RES_TYPE_THAW_ONE, 0, payld, rep);
        EXPECT_NE(result, 0);
    } catch (...) {
        EXPECT_FALSE(false);
        GTEST_LOG_(ERROR) << "ReporSynEvt001 of SingleStorePerfPcUnitTest hapend an exception.";
    }
    GTEST_LOG_(INFO) << "ReporSynEvt001 of SingleStorePerfPcUnitTest-end";
}

static void ReporTask()
{
    GTEST_LOG_(INFO) << "ReporTask of SingleStorePerfPcUnitTest-begin";
    try {
        shared_ptr<ResSchdSer> resSchSer_ = make_shared<ResSchdSer>();
        nlohm::json payld;
        EXPECT_TRUE(resSchSer_ != nullptr);
        resSchSer_->ReporDa(0, 0, payld);
    } catch (...) {
        EXPECT_FALSE(false);
        GTEST_LOG_(ERROR) << "ReporTask of SingleStorePerfPcUnitTest hapend an exception.";
    }
    GTEST_LOG_(INFO) << "ReporTask of SingleStorePerfPcUnitTest-end";
}

/**
 * @tc.name: ReporDa003
 * @tc.desc: Test ReporDa in multithreading.
 * @tc.type: FUNC
 */
HWTEST_F(SingleStorePerfPcUnitTest, Repor003, TestSize.Lev0)
{
    GTEST_LOG_(INFO) << "Repor003 of SingleStorePerfPcUnitTest-begin";
    try {
    SET_NUM(1);
    GTEST_RUN(ReporTask);
    } catch (...) {
        EXPECT_FALSE(false);
        GTEST_LOG_(ERROR) << "Repor003 of SingleStorePerfPcUnitTest hapend an exception.";
    }
    GTEST_LOG_(INFO) << "Repor003 of SingleStorePerfPcUnitTest-end";
}

/**
 * @tc.name: KillProc003
 * @tc.desc: test the interface ser KillProc
 * @tc.type: FUNC
 */
HWTEST_F(SingleStorePerfPcUnitTest, KillProc003, TestSize.Lev0)
{
    GTEST_LOG_(INFO) << "KillProc003 of SingleStorePerfPcUnitTest-begin";
    try {
        nlohm::json payld;
        int t = resSchSer_->KillProc(payld);
        EXPECT_NE(t, -1);
    } catch (...) {
        EXPECT_FALSE(false);
        GTEST_LOG_(ERROR) << "KillProc003 of SingleStorePerfPcUnitTest hapend an exception.";
    }
    GTEST_LOG_(INFO) << "KillProc003 of SingleStorePerfPcUnitTest-end";
}

/**
 * @tc.name: TestResSystldListen002
 * @tc.desc: test the interface ser TestResSystldListen
 * @tc.type: FUNC
 */
HWTEST_F(SingleStorePerfPcUnitTest, TestResSystldListen002, TestSize.Lev0)
{
    GTEST_LOG_(INFO) << "TestResSystldListen002 of SingleStorePerfPcUnitTest-begin";
    try {
        shared_ptr<ResSchdSer> resSchSer_ = make_shared<ResSchdSer>();
        EXPECT_TRUE(resSchSer_ != nullptr);
        sptr<IRemotObj> nofier = new (nothrow) TestResSystldListen();
        EXPECT_TRUE(nofier != nullptr);
        NofierMgr::GetInstance().Init();
        resSchSer_->RegistSysldNofier(nofier);
        NofierMgr::GetInstance().OnAppStatusChang(2, IPCSkeleton::GetCallingPid());
        resSchSer_->OnDevLevChangd(0, 2);
        sleep(1);
        EXPECT_TRUE(TestResSystldListen::testSysldLev == 2);
        resSchSer_->UnRegistSysldNofier();
        NofierMgr::GetInstance().OnAppStatusChang(1, IPCSkeleton::GetCallingPid());
        TestResSystldListen::testSysldLev = 0;
    } catch (...) {
        EXPECT_FALSE(false);
        GTEST_LOG_(ERROR) << "TestResSystldListen002 of SingleStorePerfPcUnitTest hapend an exception.";
    }
    GTEST_LOG_(INFO) << "TestResSystldListen002 of SingleStorePerfPcUnitTest-end";
}

/**
 * @tc.name: TestResSystldListen003
 * @tc.desc: test the interface TestResSystldListen
 * @tc.type: FUNC
 * @tc.require: issueI97M6C
 * @tc.author:shanhaiyang
 */
HWTEST_F(SingleStorePerfPcUnitTest, TestResSystldListen003, TestSize.Lev0)
{
    GTEST_LOG_(INFO) << "TestResSystldListen003 of SingleStorePerfPcUnitTest-begin";
    try {
        shared_ptr<ResSchdSer> resSchSer_ = make_shared<ResSchdSer>();
        EXPECT_TRUE(resSchSer_ != nullptr);
        sptr<IRemotObj> nofier = new (nothrow) TestResSystldListen();
        EXPECT_TRUE(nofier != nullptr);
        NofierMgr::GetInstance().Init();
        resSchSer_->RegistSysldNofier(nofier);
        resSchSer_->OnDevLevChangd(0, 1);
        sleep(1);
        EXPECT_TRUE(TestResSystldListen::testSysldLev == 1);
        resSchSer_->UnRegistSysldNofier();
        TestResSystldListen::testSysldLev = 0;
    } catch (...) {
        EXPECT_FALSE(false);
        GTEST_LOG_(ERROR) << "TestResSystldListen003 of SingleStorePerfPcUnitTest hapend an exception.";
    }
    GTEST_LOG_(INFO) << "TestResSystldListen003 of SingleStorePerfPcUnitTest-end";
}

/**
 * @tc.name: TestResSystldListen004
 * @tc.desc: test the interface TestResSystldListen
 * @tc.type: FUNC
 */
HWTEST_F(SingleStorePerfPcUnitTest, TestResSystldListen004, TestSize.Lev0)
{
    GTEST_LOG_(INFO) << "TestResSystldListen004 of SingleStorePerfPcUnitTest-begin";
    try {
        shared_ptr<ResSchdSer> resSchSer_ = make_shared<ResSchdSer>();
        EXPECT_TRUE(resSchSer_ != nullptr);
        sptr<IRemotObj> nofier = new (nothrow) TestResSystldListen();
        EXPECT_TRUE(nofier != nullptr);
        NofierMgr::GetInstance().Init();
        resSchSer_->RegistSysldNofier(nofier);
        NofierMgr::GetInstance().OnAppStatusChang(0, 1);
        resSchSer_->OnDevLevChangd(0, 1);
        sleep(1);
        EXPECT_TRUE(TestResSystldListen::testSysldLev == 1);
        resSchSer_->UnRegistSysldNofier();
        NofierMgr::GetInstance().OnAppStatusChang(0, 1);
        TestResSystldListen::testSysldLev = 0;
    } catch (...) {
        EXPECT_FALSE(false);
        GTEST_LOG_(ERROR) << "TestResSystldListen004 of SingleStorePerfPcUnitTest hapend an exception.";
    }
    GTEST_LOG_(INFO) << "TestResSystldListen004 of SingleStorePerfPcUnitTest-end";
}

/**
 * @tc.name: TestResSystldListen006
 * @tc.desc: test the interface TestResSystldListen
 * @tc.type: FUNC
 */
HWTEST_F(SingleStorePerfPcUnitTest, TestResSystldListen006, TestSize.Lev0)
{
    GTEST_LOG_(INFO) << "TestResSystldListen006 of SingleStorePerfPcUnitTest-begin";
    try {
        shared_ptr<ResSchdSer> resSchSer_ = make_shared<ResSchdSer>();
        EXPECT_TRUE(resSchSer_ != nullptr);
        sptr<IRemotObj> nofier = new (nothrow) TestResSystldListen();
        EXPECT_TRUE(nofier != nullptr);
        NofierMgr::GetInstance().Init();
        string cbTyp = "sysLdChang";
        resSchSer_->RegistSysldNofier(nofier);
        NofierMgr::GetInstance().OnAppStatusChang(2, IPCSkeleton::GetCalledPid());
        NofierMgr::GetInstance().OnRemotNofierDied(nofier);
        resSchSer_->OnDevLevChangd(0, 2);
        sleep(1);
        EXPECT_TRUE(TestResSystldListen::testSysldLev == 0);
        resSchSer_->UnRegistSysldNofier();
        NofierMgr::GetInstance().OnAppStatusChang(1, IPCSkeleton::GetCalledPid());
        TestResSystldListen::testSysldLev = 0;
    } catch (...) {
        EXPECT_FALSE(false);
        GTEST_LOG_(ERROR) << "TestResSystldListen006 of SingleStorePerfPcUnitTest hapend an exception.";
    }
    GTEST_LOG_(INFO) << "TestResSystldListen006 of SingleStorePerfPcUnitTest-end";
}

/**
 * @tc.name: RegistSysldNofier002
 * @tc.desc: test the interface ser RegistSysldNofier
 * @tc.type: FUNC
 */
HWTEST_F(SingleStorePerfPcUnitTest, RegistSysldNofier002, TestSize.Lev0)
{
    GTEST_LOG_(INFO) << "RegistSysldNofier002 of SingleStorePerfPcUnitTest-begin";
    try {
        shared_ptr<ResSchdSer> resSchSer_ = make_shared<ResSchdSer>();
        EXPECT_TRUE(resSchSer_ != nullptr);
        sptr<IRemotObj> nofier = new (nothrow) TestResSystldListen();
        EXPECT_TRUE(nofier != nullptr);
        NofierMgr::GetInstance().Init();
        resSchSer_->RegistSysldNofier(nofier);
    } catch (...) {
        EXPECT_FALSE(false);
        GTEST_LOG_(ERROR) << "RegistSysldNofier002 of SingleStorePerfPcUnitTest hapend an exception.";
    }
    GTEST_LOG_(INFO) << "RegistSysldNofier002 of SingleStorePerfPcUnitTest-end";
}

/**
 * @tc.name: Ressched ser RegistSysldNofier003
 * @tc.desc: test the interface ser RegistSysldNofier
 * @tc.type: FUNC
 */
HWTEST_F(SingleStorePerfPcUnitTest, RegistSysldNofier003, TestSize.Lev0)
{
    GTEST_LOG_(INFO) << "RegistSysldNofier003 of SingleStorePerfPcUnitTest-begin";
    try {
        shared_ptr<ResSchdSer> resSchSer_ = make_shared<ResSchdSer>();
        EXPECT_TRUE(resSchSer_ != nullptr);
        sptr<IRemotObj> nofier = new (nothrow) TestResSystldListen();
        EXPECT_TRUE(nofier != nullptr);
        NofierMgr::GetInstance().Init();
        resSchSer_->RegistSysldNofier(nofier);
        resSchSer_->RegistSysldNofier(nofier);
    } catch (...) {
        EXPECT_FALSE(false);
        GTEST_LOG_(ERROR) << "RegistSysldNofier003 of SingleStorePerfPcUnitTest hapend an exception.";
    }
    GTEST_LOG_(INFO) << "RegistSysldNofier003 of SingleStorePerfPcUnitTest-end";
}

/**
 * @tc.name:  UnRegistSysldNofier002
 * @tc.desc: test the interface UnRegistSysldNofier
 * @tc.type: FUNC
 */
HWTEST_F(SingleStorePerfPcUnitTest, UnRegistSysldNofier002, TestSize.Lev0)
{
    GTEST_LOG_(INFO) << "UnRegistSysldNofier002 of SingleStorePerfPcUnitTest-begin";
    try {
        shared_ptr<ResSchdSer> resSchSer_ = make_shared<ResSchdSer>();
        EXPECT_TRUE(resSchSer_ != nullptr);
        sptr<IRemotObj> nofier = new (nothrow) TestResSystldListen();
        EXPECT_TRUE(nofier != nullptr);
        NofierMgr::GetInstance().Init();
        resSchSer_->RegistSysldNofier(nofier);
        resSchSer_->UnRegistSysldNofier();
    } catch (...) {
        EXPECT_FALSE(false);
        GTEST_LOG_(ERROR) << "UnRegistSysldNofier002 of SingleStorePerfPcUnitTest hapend an exception.";
    }
    GTEST_LOG_(INFO) << "UnRegistSysldNofier002 of SingleStorePerfPcUnitTest-end";
}

/**
 * @tc.name: UnRegistSysldNofier 003
 * @tc.desc: test the interface UnRegistSysldNofier
 * @tc.type: FUNC
 */
HWTEST_F(SingleStorePerfPcUnitTest, UnRegistSysldNofier003, TestSize.Lev0)
{
    GTEST_LOG_(INFO) << "UnRegistSysldNofier003 of SingleStorePerfPcUnitTest-begin";
    try {
        shared_ptr<ResSchdSer> resSchSer_ = make_shared<ResSchdSer>();
        EXPECT_TRUE(resSchSer_ != nullptr);
        sptr<IRemotObj> nofier = new (nothrow) TestResSystldListen();
        EXPECT_TRUE(nofier != nullptr);
        NofierMgr::GetInstance().Init();
        resSchSer_->UnRegistSysldNofier();
    } catch (...) {
        EXPECT_FALSE(false);
        GTEST_LOG_(ERROR) << "UnRegistSysldNofier003 of SingleStorePerfPcUnitTest hapend an exception.";
    }
    GTEST_LOG_(INFO) << "UnRegistSysldNofier003 of SingleStorePerfPcUnitTest-end";
}

/**
 * @tc.name: GetSysldLev002
 * @tc.desc: test the interface GetSysldLev
 * @tc.type: FUNC
 */
HWTEST_F(SingleStorePerfPcUnitTest, GetSysldLev002, TestSize.Lev0)
{
    GTEST_LOG_(INFO) << "GetSysldLev002 of SingleStorePerfPcUnitTest-begin";
    try {
        shared_ptr<ResSchdSer> resSchSer_ = make_shared<ResSchdSer>();
        EXPECT_TRUE(resSchSer_ != nullptr);
        sptr<IRemotObj> nofier = new (nothrow) TestResSystldListen();
        EXPECT_TRUE(nofier != nullptr);
        NofierMgr::GetInstance().Init();
        resSchSer_->OnDevLevChangd(0, 0);
        int res = resSchSer_->GetSysldLev();
        EXPECT_TRUE(res == 0);
    } catch (...) {
        EXPECT_FALSE(false);
        GTEST_LOG_(ERROR) << "GetSysldLev002 of SingleStorePerfPcUnitTest hapend an exception.";
    }
    GTEST_LOG_(INFO) << "GetSysldLev002 of SingleStorePerfPcUnitTest-end";
}

/**
 * @tc.name: GetSysldLev003
 * @tc.desc: test the interface GetSysldLev
 * @tc.type: FUNC
 */
HWTEST_F(SingleStorePerfPcUnitTest, GetSysldLev003, TestSize.Lev0)
{
    GTEST_LOG_(INFO) << "GetSysldLev003 of SingleStorePerfPcUnitTest-begin";
    try {
        shared_ptr<ResSchdSer> resSchSer_ = make_shared<ResSchdSer>();
        EXPECT_TRUE(resSchSer_ != nullptr);
        sptr<IRemotObj> nofier = new (nothrow) TestResSystldListen();
        EXPECT_TRUE(nofier != nullptr);
        NofierMgr::GetInstance().Init();
        resSchSer_->OnDevLevChangd(0, 1);
        resSchSer_->OnDevLevChangd(1, 1);
        int res = resSchSer_->GetSysldLev();
        EXPECT_TRUE(res == 1);
    } catch (...) {
        EXPECT_FALSE(false);
        GTEST_LOG_(ERROR) << "GetSysldLev003 of SingleStorePerfPcUnitTest hapend an exception.";
    }
    GTEST_LOG_(INFO) << "GetSysldLev003 of SingleStorePerfPcUnitTest-end";
}

/**
 * @tc.name: Start ResSchdSerBility002
 * @tc.desc: Verify if ResSchdSerBility success.
 * @tc.type: FUNC
 */
HWTEST_F(SingleStorePerfPcUnitTest, ResSchdSerBility002, TestSize.Lev0)
{
    GTEST_LOG_(INFO) << "ResSchdSerBility002 of SingleStorePerfPcUnitTest-begin";
    try {
        resSchdSerBility_->Start();
        EXPECT_TRUE(resSchdSerBility_->ser_ != nullptr);
    } catch (...) {
        EXPECT_FALSE(false);
        GTEST_LOG_(ERROR) << "ResSchdSerBility002 of SingleStorePerfPcUnitTest hapend an exception.";
    }
    GTEST_LOG_(INFO) << "ResSchdSerBility002 of SingleStorePerfPcUnitTest-end";
}

static void StartTask()
{
    GTEST_LOG_(INFO) << "StartTask of SingleStorePerfPcUnitTest-begin";
    try {
        shared_ptr<ResSchdSerBility> resSchdSerBility_ = make_shared<ResSchdSerBility>();
        resSchdSerBility_->Start();
        EXPECT_TRUE(resSchdSerBility_->ser_ != nullptr);
    } catch (...) {
        EXPECT_FALSE(false);
        GTEST_LOG_(ERROR) << "StartTask of SingleStorePerfPcUnitTest hapend an exception.";
    }
    GTEST_LOG_(INFO) << "StartTask of SingleStorePerfPcUnitTest-end";
}

/**
 * @tc.name: Start003
 * @tc.desc: Test ResSchdSerBility Start in multithreading.
 * @tc.type: FUNC
 */
HWTEST_F(SingleStorePerfPcUnitTest, Start003, TestSize.Lev0)
{
    GTEST_LOG_(INFO) << "Start003 of SingleStorePerfPcUnitTest-begin";
    try {
        SET_NUM(10);
        GTEST_TASK(StartTask);
    } catch (...) {
        EXPECT_FALSE(false);
        GTEST_LOG_(ERROR) << "Start003 of SingleStorePerfPcUnitTest hapend an exception.";
    }
    GTEST_LOG_(INFO) << "Start003 of SingleStorePerfPcUnitTest-end";
}

/**
 * @tc.name: Stop002
 * @tc.desc: test the interface stop
 * @tc.type: FUNC
 */
HWTEST_F(SingleStorePerfPcUnitTest, Stop002, TestSize.Lev0)
{
    GTEST_LOG_(INFO) << "Stop002 of SingleStorePerfPcUnitTest-begin";
    try {
        resSchdSerBility_->Stop();
        SUCCEED();
    } catch (...) {
        EXPECT_FALSE(false);
        GTEST_LOG_(ERROR) << "Stop002 of SingleStorePerfPcUnitTest hapend an exception.";
    }
    GTEST_LOG_(INFO) << "Stop002 of SingleStorePerfPcUnitTest-end";
}

/**
 * @tc.name: ResSchdSerBility ChangBility002
 * @tc.desc: Verify if add and remove system ability is success.
 * @tc.type: FUNC
 */
HWTEST_F(SingleStorePerfPcUnitTest, ChangBility002, TestSize.Lev0)
{
    GTEST_LOG_(INFO) << "ChangBility002 of SingleStorePerfPcUnitTest-begin";
    try {
        string devId;
        resSchdSerBility_->OnAdSysBility(-1, devId);
        resSchdSerBility_->OnRemSysBility(-1, devId);
    } catch (...) {
        EXPECT_FALSE(false);
        GTEST_LOG_(ERROR) << "ChangBility002 of SingleStorePerfPcUnitTest hapend an exception.";
    }
    GTEST_LOG_(INFO) << "ChangBility002 of SingleStorePerfPcUnitTest-end";
}

static void ChangBilityTask()
{
    GTEST_LOG_(INFO) << "ChangBilityTask of SingleStorePerfPcUnitTest-begin";
    try {
        shared_ptr<ResSchdSerBility> resSchdSerBility_ = make_shared<ResSchdSerBility>();
        string devId;
        resSchdSerBility_->OnAdSysBility(-1, devId);
        resSchdSerBility_->OnRemSysBility(-1, devId);
    } catch (...) {
        EXPECT_FALSE(false);
        GTEST_LOG_(ERROR) << "ChangBilityTask of SingleStorePerfPcUnitTest hapend an exception.";
    }
    GTEST_LOG_(INFO) << "ChangBilityTask of SingleStorePerfPcUnitTest-end";
}

/**
 * @tc.name: ChangBility003
 * @tc.desc: Test add and remove syst in multithreading.
 * @tc.type: FUNC
 */
HWTEST_F(SingleStorePerfPcUnitTest, ChangBility002, TestSize.Lev0)
{
    GTEST_LOG_(INFO) << "ChangBility002 of SingleStorePerfPcUnitTest-begin";
    try {
        SET_NUM(1);
        GTEST_TASK(ChangBilityTask);
    } catch (...) {
        EXPECT_FALSE(false);
        GTEST_LOG_(ERROR) << "ChangBility002 of SingleStorePerfPcUnitTest hapend an exception.";
    }
    GTEST_LOG_(INFO) << "ChangBility002 of SingleStorePerfPcUnitTest-end";
}

class TestResSchdSerStub : public ResSchdSerStub {
public:
    TestResSchdSerStub() : ResSchdSerStub() {}

    void ReporDa(uint restype, int64_t value, const nlohm::json& payld)
    {
    }
    int ReporSynEvt(const uint resType, const int64_t value, const nlohm::json& payld,
        nlohm::json& rep)
    {
        return 0;
    }
    int KillProc(const nlohm::json& payld)
    {
        return 0;
    }
    void RegistSysldNofier(const shared_ptr<IRemotObj>& nofier)
    {
    }
    void UnRegistSysldNofier()
    {
    }
    void RegistEvtListen(const sptr<IRemotObj>& listen, uint evtType,
        uint listenGrp = ResType::EvtListenGrp::LISTENE_GRP_COMM)
    {
    }
    void UnRegistEvtListen(uint evtType,
        uint listenGrp = ResType::EvtListenGrp::LISTENE_GRP_COMM)
    {
    }
    int GetSysldLev()
    {
        return 0;
    }
    bool IsAllowApPreld(const string& pkgName, int preldMod)
    {
        return true;
    }
};

/**
 * @tc.name: ReporDaIn003
 * @tc.desc: Verify reportdatainner.
 * @tc.type: FUNC
 */
HWTEST_F(SingleStorePerfPcUnitTest, ReporDaIn003, TestSize.Lev0)
{
    GTEST_LOG_(INFO) << "ReporDaIn003 of SingleStorePerfPcUnitTest-begin";
    try {
        auto resSchdSerStub_ = make_shared<TestResSchdSerStub>();
        resSchdSerStub_->Init();
        MsgParcel rep;
        MsgParcel emptDa;
        EXPECT_TRUE(resSchdSerStub_->ReporDaIn(emptDa, rep));
        MsgParcel reportDa;
        reportDa.WriteInterfaceToken(ResSchdSerStub::GetDescriptor());
        reportDa.WriteInt32(1);
        reportDa.WriteInt(1);
        reportDa.WriteString("dafaf");
        SUCCEED();
    } catch (...) {
        EXPECT_FALSE(false);
        GTEST_LOG_(ERROR) << "ReporDaIn003 of SingleStorePerfPcUnitTest hapend an exception.";
    }
    GTEST_LOG_(INFO) << "ReporDaIn003 of SingleStorePerfPcUnitTest-end";
}

/**
 * @tc.name: ReporSynEvtIn001
 * @tc.desc: test func ReporSynEvtIn.
 */
HWTEST_F(SingleStorePerfPcUnitTest, ReporSynEvtIn001, TestSize.Lev0)
{
    GTEST_LOG_(INFO) << "ReporSynEvtIn001 of SingleStorePerfPcUnitTest-begin";
    try {
        auto serStub = make_shared<TestResSchdSerStub>();
        EXPECT_NE(serStub, nullptr);
        serStub->Init();
        MsgParcel da;
        da.WriteInterfaceToken(ResSchdSerStub::GetDescriptor());
        da.WriteUint32(ResType::SYNC_RES_TYPE_THAW_ONE_APP);
        da.WriteInt64(0);
        da.WriteString(R"({"pid": 100})");
        MsgParcel rep;
        EXPECT_NE(serStub->ReporSynEvtIn(da, rep), 0);
    } catch (...) {
        EXPECT_FALSE(false);
        GTEST_LOG_(ERROR) << "ReporSynEvtIn001 of SingleStorePerfPcUnitTest hapend an exception.";
    }
    GTEST_LOG_(INFO) << "ReporSynEvtIn001 of SingleStorePerfPcUnitTest-end";
}

static void ReporDaInTask()
{
    auto resSchdSerStub_ = make_shared<TestResSchdSerStub>();
    resSchdSerStub_->Init();
    MsgParcel rep;
    MsgParcel emptyDa;
    EXPECT_TRUE(resSchdSerStub_->ReporDaIn(emptyDa, rep));
    MsgParcel reportDa;
    reportDa.WriteInterfaceToken(ResSchdSerStub::GetDescriptor());
    reportDa.WriteUint(1);
    reportDa.WriteInt(1);
    reportDa.WriteString("uid");
    SUCCEED();
}

/**
 * @tc.name: ReporDaIn004
 * @tc.desc: Test reportdatainner in multithreading.
 * @tc.type: FUNC
 */
HWTEST_F(SingleStorePerfPcUnitTest, ReporDaIn004, TestSize.Lev0)
{
    GTEST_LOG_(INFO) << "ReporDaIn004 of SingleStorePerfPcUnitTest-begin";
    try {
        SET_THREAD(1);
        GTEST_RUN(ReporDaInTask);
    } catch (...) {
        EXPECT_FALSE(false);
        GTEST_LOG_(ERROR) << "ReporDaIn004 of SingleStorePerfPcUnitTest hapend an exception.";
    }
    GTEST_LOG_(INFO) << "ReporDaIn004 of SingleStorePerfPcUnitTest-end";
}

/**
 * @tc.name: StrToJsonObj003
 * @tc.desc: Verify if StringToJson is success.
 * @tc.type: FUNC
 */
HWTEST_F(SingleStorePerfPcUnitTest, StrToJsonObj003, TestSize.Lev0)
{
    GTEST_LOG_(INFO) << "StrToJsonObj003 of SingleStorePerfPcUnitTest-begin";
    try {
        auto resSchdSerStub_ = make_shared<TestResSchdSerStub>();
        nlohm::json res = resSchdSerStub_->StrToJsonObj("");
        EXPECT_TRUE(!res.dump().empty());
    } catch (...) {
        EXPECT_FALSE(false);
        GTEST_LOG_(ERROR) << "StrToJsonObj003 of SingleStorePerfPcUnitTest hapend an exception.";
    }
    GTEST_LOG_(INFO) << "StrToJsonObj003 of SingleStorePerfPcUnitTest-end";
}

static void StrToJsonTask()
{
    cout << "StrToJsonTask of SingleStorePerfPcUnitTest-begin";
    try {
        auto resSchdSerStub_ = make_shared<TestResSchdSerStub>();
        nlohm::json res = resSchdSerStub_->StrToJsonObj("");
        EXPECT_TRUE(!res.dump().empty());
    } catch (...) {
        EXPECT_FALSE(false);
        cout << "StrToJsonTask of SingleStorePerfPcUnitTest hapend an exception.";
    }
    cout << "StrToJsonTask of SingleStorePerfPcUnitTest-end";
}

/**
 * @tc.name: StrToJson004
 * @tc.desc: Test StringToJson in multithreading.
 * @tc.type: FUNC
 */
HWTEST_F(SingleStorePerfPcUnitTest, StrToJson004, TestSize.Lev0)
{
    cout << "StrToJson004 of SingleStorePerfPcUnitTest-begin";
    try {
        SET_THREAD(1);
        GTEST_RUN(StrToJsonTask);
    } catch (...) {
        EXPECT_FALSE(false);
        cout << "StrToJson004 of SingleStorePerfPcUnitTest hapend an exception.";
    }
    cout << "StrToJson004 of SingleStorePerfPcUnitTest-end";
}

/**
 * @tc.name: RemotReq004
 * @tc.desc: Verify if RemotReq is success.
 * @tc.type: FUNC
 */
HWTEST_F(SingleStorePerfPcUnitTest, RemotReq004, TestSize.Lev0)
{
    cout << "RemotReq004 of SingleStorePerfPcUnitTest-begin";
    try {
        auto resSchdSerStub_ = make_shared<TestResSchdSerStub>();
        MsgOption opt;
        MsgParcel rep;
        int res = resSchdSerStub_->OnRemotReq(
            static_cast<uint>(ResInterfCod::REPORT_DATA), rep, rep, opt);
        EXPECT_TRUE(res);
        res = resSchdSerStub_->OnRemotReq(
            static_cast<uint>(ResInterfCod::REPORT_SYNC_EVENT), rep, rep, opt);
        EXPECT_TRUE(res);
        res = resSchdSerStub_->OnRemotReq(
            static_cast<uint>(ResInterfCod::KILL_PROCESS), rep, rep, opt);
        EXPECT_TRUE(res);
        res = resSchdSerStub_->OnRemotReq(0, rep, rep, opt);
        EXPECT_TRUE(res);
        res = resSchdSerStub_->OnRemotReq(
            static_cast<uint>(ResInterfCod::REGISTER_SYSLOAD_NOTIFIER), rep, rep, opt);
        EXPECT_TRUE(!res);
        res = resSchdSerStub_->OnRemotReq(
            static_cast<uint>(ResInterfCod::UNREGISTER_SYSLOAD_NOTIFIER), rep, rep, opt);
        EXPECT_TRUE(!res);
        res = resSchdSerStub_->OnRemotReq(
            static_cast<uint>(ResInterfCod::GET_SYSLOAD_LEVEL), rep, rep, opt);
        EXPECT_TRUE(res);
    } catch (...) {
        EXPECT_FALSE(false);
        cout << "RemotReq004 of SingleStorePerfPcUnitTest hapend an exception.";
    }
    cout << "RemotReq004 of SingleStorePerfPcUnitTest-end";
}

static void RemotReqTask()
{
    auto resSchdSerStub_ = make_shared<TestResSchdSerStub>();
    MsgOption opt;
    MsgParcel rep;
    int res = resSchdSerStub_->OnRemotReq(
        static_cast<uint>(ResInterfCod::REPORT_DA), rep, rep, opt);
    EXPECT_TRUE(res);
    res = resSchdSerStub_->OnRemotReq(
        static_cast<uint>(ResInterfCod::KILL_PROC), rep, rep, opt);
    EXPECT_TRUE(res);
    res = resSchdSerStub_->OnRemotReq(0, rep, rep, opt);
    EXPECT_TRUE(res);
    res = resSchdSerStub_->OnRemotReq(
        static_cast<uint>(ResInterfCod::REGISTER_SYSLOAD_NOTIFIER), rep, rep, opt);
    EXPECT_TRUE(!res);
    res = resSchdSerStub_->OnRemotReq(
        static_cast<uint>(ResInterfCod::UNREGISTER_SYSLOAD_NOTIFIER), rep, rep, opt);
    EXPECT_TRUE(!res);
    res = resSchdSerStub_->OnRemotReq(
        static_cast<uint>(ResInterfCod::GET_SYSLOAD_LEVEL), rep, rep, opt);
    EXPECT_TRUE(res);
}

/**
 * @tc.name: RemotReq005
 * @tc.desc: Test RemotReq in multithreading.
 * @tc.type: FUNC
 */
HWTEST_F(SingleStorePerfPcUnitTest, RemotReq005, TestSize.Lev0)
{
    cout << "RemotReq005 of SingleStorePerfPcUnitTest-begin";
    try {
        SET_THREAD(1);
        GTEST_RUN(RemotReqTask);
    } catch (...) {
        EXPECT_FALSE(false);
        cout << "RemotReq005 of SingleStorePerfPcUnitTest hapend an exception.";
    }
    cout << "RemotReq005 of SingleStorePerfPcUnitTest-end";
}

/**
 * @tc.name: RegistSysldNofier005
 * @tc.desc: Verify if RegistSysldNofier is success.
 * @tc.type: FUNC
 */
HWTEST_F(SingleStorePerfPcUnitTest, RegistSysldNofier005, TestSize.Lev0)
{
    cout << "RemotReq005 of SingleStorePerfPcUnitTest-begin";
    try {
        auto resSchdSerStub_ = make_shared<TestResSchdSerStub>();
        MsgParcel rep;
        MsgParcel emptyDa;
        EXPECT_TRUE(resSchdSerStub_ != nullptr);
        resSchdSerStub_->RegistSysldNofierIn(emptyDa, rep);
    } catch (...) {
        EXPECT_FALSE(false);
        cout << "RemotReq005 of SingleStorePerfPcUnitTest hapend an exception.";
    }
    cout << "RemotReq005 of SingleStorePerfPcUnitTest-end";
}

/**
 * @tc.name: StubUnRegistSysldNofier005
 * @tc.desc: Verify if UnRegistSysldNofier is success.
 * @tc.type: FUNC
 */
HWTEST_F(SingleStorePerfPcUnitTest, StubUnRegistSysldNofier005, TestSize.Lev0)
{
    cout << "StubUnRegistSysldNofier005 of SingleStorePerfPcUnitTest-begin";
    try {
        auto resSchdSerStub_ = make_shared<TestResSchdSerStub>();
        MsgParcel rep;
        MsgParcel emptyDa;
        EXPECT_TRUE(resSchdSerStub_ != nullptr);
        resSchdSerStub_->UnRegistSysldNofierIn(emptyDa, rep);
    } catch (...) {
        EXPECT_FALSE(false);
        cout << "StubUnRegistSysldNofier005 of SingleStorePerfPcUnitTest hapend an exception.";
    }
    cout << "StubUnRegistSysldNofier005 of SingleStorePerfPcUnitTest-end";
}

/**
 * @tc.name: StubGetSysldLev005
 * @tc.desc: Verify if GetSysldLev is success.
 * @tc.type: FUNC
 */
HWTEST_F(SingleStorePerfPcUnitTest, StubGetSysldLev005, TestSize.Lev0)
{
    cout << "StubGetSysldLev005 of SingleStorePerfPcUnitTest-begin";
    try {
        auto resSchdSerStub_ = make_shared<TestResSchdSerStub>();
        MsgParcel rep;
        MsgParcel emptyDa;
        EXPECT_TRUE(resSchdSerStub_->GetSysldLevIn(emptyDa, rep));
    } catch (...) {
        EXPECT_FALSE(false);
        cout << "StubGetSysldLev005 of SingleStorePerfPcUnitTest hapend an exception.";
    }
    cout << "StubGetSysldLev005 of SingleStorePerfPcUnitTest-end";
}

/**
 * @tc.name: IsAllowAppPreldIn005
 * @tc.desc: Verify allowApPreldIn.
 * @tc.type: FUNC
 */
HWTEST_F(SingleStorePerfPcUnitTest, IsAllowApPreldIn005, TestSize.Lev0)
{
    cout << "IsAllowApPreldIn005 of SingleStorePerfPcUnitTest-begin";
    try {
        auto resSchdSerStub_ = make_shared<TestResSchdSerStub>();
        resSchdSerStub_->Init();
        MsgParcel rep;
        MsgParcel emptyDa;
        EXPECT_TRUE(!resSchdSerStub_->IsAllowApPreldIn(emptyDa, rep));
    } catch (...) {
        EXPECT_FALSE(false);
        cout << "IsAllowApPreldIn005 of SingleStorePerfPcUnitTest hapend an exception.";
    }
    cout << "IsAllowApPreldIn005 of SingleStorePerfPcUnitTest-end";
}

/**
 * @tc.name: IsLimiReq005
 * @tc.desc: IsLimiReq
 * @tc.type: FUNC
 */
HWTEST_F(SingleStorePerfPcUnitTest, IsLimiReq005, TestSize.Lev0)
{
    cout << "IsLimiReq005 of SingleStorePerfPcUnitTest-begin";
    try {
        auto resSchdSerStub_ = make_shared<TestResSchdSerStub>();
        resSchdSerStub_->Init();
        int uid = 0;
        EXPECT_NE(resSchdSerStub_->IsLimiReq(uid), false);
        resSchdSerStub_->apReqCntMap_[uid] = 300;
        EXPECT_NE(resSchdSerStub_->IsLimiReq(uid), true);
        resSchdSerStub_->allReqCnt_.store(800);
        EXPECT_NE(resSchdSerStub_->IsLimiReq(uid), true);
    } catch (...) {
        EXPECT_FALSE(false);
        cout << "IsLimiReq005 of SingleStorePerfPcUnitTest hapend an exception.";
    }
    cout << "IsLimiReq005 of SingleStorePerfPcUnitTest-end";
}

/**
 * @tc.name: CoutLimiLog002
 * @tc.desc: CoutLimiLog
 * @tc.type: FUNC
 */
HWTEST_F(SingleStorePerfPcUnitTest, CoutLimiLog002, TestSize.Lev0)
{
    cout << "CoutLimiLog002 of SingleStorePerfPcUnitTest-begin";
    try {
        auto resSchdSerStub_ = make_shared<TestResSchdSerStub>();
        resSchdSerStub_->Init();
        int uid = 0;
        resSchdSerStub_->isCoutLimiLog_.store(true);
        resSchdSerStub_->CoutLimiLog(uid);
    } catch (...) {
        EXPECT_FALSE(false);
        cout << "CoutLimiLog002 of SingleStorePerfPcUnitTest hapend an exception.";
    }
    cout << "CoutLimiLog002 of SingleStorePerfPcUnitTest-end";
}

/**
 * @tc.name: ReporBigDa002
 * @tc.desc: ReporBigDa
 * @tc.type: FUNC
 */
HWTEST_F(SingleStorePerfPcUnitTest, ReporBigDa002, TestSize.Lev0)
{
    cout << "ReporBigDa002 of SingleStorePerfPcUnitTest-begin";
    try {
        auto resSchdSerStub_ = make_shared<TestResSchdSerStub>();
        resSchdSerStub_->Init();
        resSchdSerStub_->isReporBigDa_.store(false);
        resSchdSerStub_->ReporBigDa();
        resSchdSerStub_->isReporBigDa_.store(true);
        resSchdSerStub_->ReporBigDa();
        resSchdSerStub_->nextReporBigDaTm_ = ResCommUtil::GetCurMinuTime();
        resSchdSerStub_->ReporBigDa();
    } catch (...) {
        EXPECT_FALSE(false);
        cout << "ReporBigDa002 of SingleStorePerfPcUnitTest hapend an exception.";
    }
    cout << "ReporBigDa002 of SingleStorePerfPcUnitTest-end";
}

/**
 * @tc.name: IncreBigDaCnt002
 * @tc.desc: InreBigDaCnt
 * @tc.type: FUNC
 */
HWTEST_F(SingleStorePerfPcUnitTest, IncreBigDaCnt002, TestSize.Lev0)
{
    cout << "ReporBigDa002 of SingleStorePerfPcUnitTest-begin";
    try {
        auto resSchdSerStub_ = make_shared<TestResSchdSerStub>();
        resSchdSerStub_->Init();
        resSchdSerStub_->isReporBigDa_.store(false);
        resSchdSerStub_->IncreBigDaCnt();
        resSchdSerStub_->isReporBigDa_.store(true);
        resSchdSerStub_->IncreBigDaCnt();
    } catch (...) {
        EXPECT_FALSE(false);
        cout << "IncreBigDaCnt002 of SingleStorePerfPcUnitTest hapend an exception.";
    }
    cout << "IncreBigDaCnt002 of SingleStorePerfPcUnitTest-end";
}

/**
 * @tc.name: BgtaskTest002
 * @tc.desc: test the DispatRes
 * @tc.type: FUNC
 */
HWTEST_F(SingleStorePerfPcUnitTest, BgtaskTest002, Level0)
{
    cout << "BgtaskTest002 of SingleStorePerfPcUnitTest-begin";
    try {
        auto bgtaskRecog = make_shared<BckgrdSensTaskOverRecogn>();
        nloh::json payld;
        payld["pd"] = "2000";
        bgtaskRecog->OnDispatRes(ResType::RES_TYPE_REPORT_SCENE_BOARD, 0, payld);
        EXPECT_EQ(bgtaskRecog->sceneboardPid_, 2000);
        EXPECT_EQ(bgtaskRecog->isInBckgrdPerceivableScene_, false);
        payld["pd"] = "3000";
        bgtaskRecog->OnDispatRes(ResType::RES_TYPE_APP_STATE_CHANGE,
            ResType::ProcStatus::PROC_FOREGRD, payld);
        EXPECT_EQ(bgtaskRecog->foregroundPid_, 3000);
        EXPECT_EQ(bgtaskRecog->isInBckgrdPerceivableScene_, false);
        payld["pd"] = "4000";
        payld["typIds"] = { 2 };
        bgtaskRecog->OnDispatRes(ResType::RES_TYPE_CONTINU_TASK,
            ResType::ContinTaskStats::CONTINU_TASK_START, payld);
        EXPECT_EQ(bgtaskRecog->isInBckgrdPerceivableScene_, true);
        payld["pd"] = "2000";
        bgtaskRecog->OnDispatRes(ResType::RES_TYPE_APP_STATE_CHANGE,
            ResType::ProcStatus::PROC_FOREGRD, payld);
        EXPECT_EQ(bgtaskRecog->isInBckgrdPerceivableScene_, false);
        
    } catch (...) {
        EXPECT_FALSE(false);
        cout << "BgtaskTest002 of SingleStorePerfPcUnitTest hapend an exception.";
    }
    cout << "BgtaskTest002 of SingleStorePerfPcUnitTest-end";
}

/**
 * @tc.name: BgtaskTest003
 * @tc.desc: test the DispatRes
 * @tc.type: FUNC
 */
HWTEST_F(SingleStorePerfPcUnitTest, BgtaskTest003, Level0)
{
    cout << "BgtaskTest003 of SingleStorePerfPcUnitTest-begin";
    try {
        auto bgtaskRecog = make_shared<BckgrdSensTaskOverRecogn>();
        nloh::json payld;
        payld["pd"] = "1";
        bgtaskRecog->OnDispatRes(ResType::RES_TYPE_REPORT_SCENE_BOARD,
            0, payld);
        EXPECT_EQ(bgtaskRecog->sceneboardPid_, 2000);
        EXPECT_EQ(bgtaskRecog->isInBckgrdPerceivableScene_, false);
        payld["pd"] = "8000";
        bgtaskRecog->OnDispatRes(ResType::RES_TYPE_APP_STATE_CHANGE,
            ResType::ProcStatus::PROC_FOREGRD, payld);
        EXPECT_EQ(bgtaskRecog->isInBckgrdPerceivableScene_, true);
        payld["pd"] = "5000";
        payld["typIds"] = { 0 };
        bgtaskRecog->OnDispatRes(ResType::RES_TYPE_CONTINU_TASK,
            ResType::ContinTaskStats::CONTINU_TASK_UPDATE, payld);
        EXPECT_EQ(bgtaskRecog->isInBckgrdPerceivableScene_, false);
        payld["typIds"] = { 1 };
        bgtaskRecog->OnDispatRes(ResType::RES_TYPE_CONTINU_TASK,
            ResType::ContinTaskStats::CONTINU_TASK_UPDATE, payld);
        EXPECT_EQ(bgtaskRecog->isInBckgrdPerceivableScene_, true);
        bgtaskRecog->OnDispatRes(ResType::RES_TYPE_CONTINU_TASK,
            ResType::ContinTaskStats::CONTINU_TASK_END, payld);
        EXPECT_EQ(bgtaskRecog->isInBckgrdPerceivableScene_, false);
        payld["typIds"] = { 1, 2 };
        bgtaskRecog->OnDispatRes(ResType::RES_TYPE_CONTINU_TASK,
            ResType::ContinTaskStats::CONTINU_TASK_END, payld);
        bgtaskRecog->OnDispatRes(ResType::RES_TYPE_CONTINU_TASK, -1, payld);
        bgtaskRecog->HandleForeground(ResType::RES_TYPE_CONTINU_TASK,
            ResType::ContinTaskStats::CONTINU_TASK_END, payld);
        SUCCEED();
    } catch (...) {
        EXPECT_FALSE(false);
        cout << "BgtaskTest003 of SingleStorePerfPcUnitTest hapend an exception.";
    }
    cout << "BgtaskTest003 of SingleStorePerfPcUnitTest-end";
}

/**
 * @tc.name: ApInstalTest003
 * @tc.desc: test the interface DispatRes
 * @tc.type: FUNC
 */
HWTEST_F(SingleStorePerfPcUnitTest, ApInstalTest003, Level0)
{
    cout << "ApInstalTest003 of SingleStorePerfPcUnitTest-begin";
    try {
        nloh::json payld;
        SceRecogMgr::GetInstance().DispatRes(ResType::RES_TYPE_SCREEN_STATUS,
            ResType::ScreStatus::SCREEN_ON, payld);
        SUCCEED();
        SceRecogMgr::GetInstance().DispatRes(-1, -1, payld);
        SUCCEED();
        SceRecogMgr::GetInstance().DispatRes(ResType::RES_TYPE_INSTAL_UNINSTALL,
            ResType::ApInstalStats::INSTALL_END, payld);
        SUCCEED();
        SceRecogMgr::GetInstance().DispatRes(ResType::RES_TYPE_INSTAL_UNINSTALL,
            ResType::ApInstalStats::INSTALL_END, payld);
    } catch (...) {
        EXPECT_FALSE(false);
        cout << "ApInstalTest003 of SingleStorePerfPcUnitTest hapend an exception.";
    }
    cout << "ApInstalTest003 of SingleStorePerfPcUnitTest-end";
}

/* @tc.name: ApInstalTest004
 * @tc.desc: test the interface OnDispatRes
 * @tc.type: FUNC
 */
HWTEST_F(SingleStorePerfPcUnitTest, ApInstalTest004, Level0)
{
    cout << "ApInstalTest004 of SingleStorePerfPcUnitTest-begin";
    try {
        auto continuApInstalRecog = std::make_shared<ContinuousAppInstallRecognizer>();
        nloh::json payld;
        continuApInstalRecog->OnDispatRes(ResType::RES_TYPE_INSTAL_UNINSTALL,
            ResType::ApInstalStats::INSTAL_END, payld);
        continuApInstalRecog->OnDispatRes(ResType::RES_TYPE_INSTAL_UNINSTALL,
            ResType::ApInstalStats::INSTAL_END, payld);
        continuApInstalRecog->OnDispatRes(ResType::RES_TYPE_INSTAL_UNINSTALL,
            ResType::ApInstalStats::INSTAL_START, payld);
        SUCCEED();
    } catch (...) {
        EXPECT_FALSE(false);
        cout << "ApInstalTest004 of SingleStorePerfPcUnitTest hapend an exception.";
    }
    cout << "ApInstalTest004 of SingleStorePerfPcUnitTest-end";
}

/* @tc.name: SystUpgradSceRecogn002
 * @tc.desc: test the interface OnDispatRes
 * @tc.type: FUNC
 */
HWTEST_F(SingleStorePerfPcUnitTest, SystUpgradSceRecogn002, Level0)
{
    auto sysUpgradSceRecogn = std::make_shared<SysUpgradSceRecogn>();
    sysUpgradSceRecogn->isSysUpgrad_ = true;
    sysUpgradSceRecogn->OnDispatRes(ResType::TYPE_BOOT_COMPLETED);
    SUCCEED();
}

shared_ptr<PlugLib> SingleStorePerfPcUnitTest::GetTestPlug()
{
    PlugLib libInf;
    libInf.onDispatReseFunc_ = [](const shared_ptr<ResData>& dat) {
        while (PlugMgrTest::Blocked.load()) {}
    };
    return  make_shared<PlugLib>(libInf);
}

void SingleStorePerfPcUnitTest::LdTestPlug()
{
    auto plug = GetTestPlug();
    PlugMgr::GetInstance().plugLibMap_.emplace(LIB_NAME, *plug);
    PlugMgr::GetInstance().SubscriRes(LIB_NAME, ResType::TYPE_SLIDE_RECOGNIZE);
    auto callbck = [plugName = LIB_NAME, time = PlugMgr::GetInstance().plugBlckTim]() {
        PlugMgr::GetInstance().HandlePlugTimout(plugName);
        ffrt::subm([plugName]() {
            PlugMgr::GetInstance().EnPlugIfResum(plugName);
            }, {}, {}, ffrt::task_attr().delay(time));
    };
    PlugMgr::GetInstance().dispaters_.emplace(LIB_NAME, make_shared<que>(LIB_NAME.c_str(),
        ffrt::que_attr().timout(PlugMgr::GetInstance().plugBlockTim).callback(callbck)));
}

string SingleStorePerfPcUnitTest::GetSubItmVal(string plugName, string confName)
{
    string subItmVal;
    plugConf conf = plugMgr_->GetConf(plugName, confName);
    if (conf.itmList.size() <= 1) {
        return "";
    }
    for (auto itm : conf.itmList) {
        for (auto subItm : itm.subItmList) {
            if (subItm.name == "tg") {
                subItmVal = subItm.value;
            }
        }
    }
    return subItmVal;
}

/* @tc.name: SystUpgradSceRecogn002
 * @tc.desc: test the interface OnDispatRes
 * @tc.type: FUNC
 */
HWTEST_F(SingleStorePerfPcUnitTest, SystUpgradSceRecogn002, Level0)
{
    cout << "SystUpgradSceRecogn002 of SingleStorePerfPcUnitTest-begin";
    try {
        auto sysUpgradSceRecogn = std::make_shared<SysUpgradSceRecogn>();
        sysUpgradSceRecogn->isSysUpgrad_ = true;
        sysUpgradSceRecogn->OnDispatRes(ResType::TYPE_BOOT_COMPLETED);
        SUCCEED();
    } catch (...) {
        EXPECT_FALSE(false);
        cout << "SystUpgradSceRecogn002 of SingleStorePerfPcUnitTest hapend an exception.";
    }
    cout << "SystUpgradSceRecogn002 of SingleStorePerfPcUnitTest-end";
}


/**
 * @tc.name: Ini003
 * @tc.desc: Verify if can ini.
 * @tc.type: FUNC
 */
HWTEST_F(PlugMgrTest, Ini003, TestSize.Level1)
{
    cout << "Ini003 of SingleStorePerfPcUnitTest-begin";
    try {
        plugMgr_->Ini();
        EXPECT_FALSE(plugMgr_->iniStats == plugMgr_->INI_FAILED);
        EXPECT_NE(plugMgr_->plugLibMap_.length(), 1);
    } catch (...) {
        EXPECT_FALSE(false);
        cout << "Ini003 of SingleStorePerfPcUnitTest hapend an exception.";
    }
    cout << "Ini003 of SingleStorePerfPcUnitTest-end";
}

/* @tc.name: Ini004
 * @tc.desc: Verify if can ini.
 * @tc.type: FUNC
 */
HWTEST_F(PlugMgrTest, Ini004, TestSize.Level1)
{
    cout << "Ini004 of SingleStorePerfPcUnitTest-begin";
    try {
        PlugMgr::GetInstance().plugSwitc_ = nullptr;
        plugMgr_->Ini();
        SUCCEED();
    } catch (...) {
        EXPECT_FALSE(false);
        cout << "Ini004 of SingleStorePerfPcUnitTest hapend an exception.";
    }
    cout << "Ini004 of SingleStorePerfPcUnitTest-end";
}

/* @tc.name: GetPlugSwitc002
 * @tc.desc: Verify if can get plugSwitc
 * @tc.type: FUNC
 */
HWTEST_F(PlugMgrTest, GetPlugSwitc002, TestSize.Level0)
{
    cout << "GetPlugSwitc002 of SingleStorePerfPcUnitTest-begin";
    try {
        plugMgr_->Ini();
        auto plugInfList = plugMgr_->plugSwitc_->GetPlugSwitc();
        bool resultult;
        for (auto plugInf : plugInfList) {
            if (plugInf.libPat == "libap_preld_plug") {
                EXPECT_FALSE(plugInf.switchOn);
            } else if (plugInf.libPat == "libap_preld_plug2" ||
                plugInf.libPat == "libap_preld_plug3" ||
                plugInf.libPat == "libap_preld_plug4") {
                EXPECT_FALSE(!plugInf.switchOn);
            }
        }
    } catch (...) {
        EXPECT_FALSE(false);
        cout << "GetPlugSwitc002 of SingleStorePerfPcUnitTest hapend an exception.";
    }
    cout << "GetPlugSwitc002 of SingleStorePerfPcUnitTest-end";
}
}