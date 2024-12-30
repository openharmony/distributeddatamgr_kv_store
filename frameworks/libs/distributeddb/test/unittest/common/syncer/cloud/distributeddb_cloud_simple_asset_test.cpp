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
#ifdef RELATIONAL_STORE
#include "cloud/cloud_sync_tag_assets.h"
#include <gtest/gtest.h>

using namespace testing::ext;
using namespace DistributedDB;
using namespace std;

namespace {
class DistributedDBCloudSimpleAssetTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void DistributedDBCloudSimpleAssetTest::SetUpTestCase()
{
}

void DistributedDBCloudSimpleAssetTest::TearDownTestCase()
{
}

void DistributedDBCloudSimpleAssetTest::SetUp()
{
}

void DistributedDBCloudSimpleAssetTest::TearDown()
{
}

/*
 * @tc.name: DownloadAssetForDupDataTest001
 * @tc.desc: Test tag asset with diff hash and downloading.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zqq
 */
HWTEST_F(DistributedDBCloudSimpleAssetTest, TagAsset001, TestSize.Level0)
{
    /**
     * @tc.steps:step1. Local asset is downloading.
     */
    Asset asset;
    asset.name = "test";
    asset.hash = "insert";
    asset.status = static_cast<int64_t>(AssetStatus::DOWNLOADING);
    VBucket beCovered;
    Assets beCoveredAssets;
    beCoveredAssets.push_back(asset);
    beCovered["field"] = beCoveredAssets;
    /**
     * @tc.steps:step2. Cloud asset has diff hash.
     */
    VBucket covered;
    Assets coveredAssets;
    asset.hash = "update";
    asset.status = static_cast<int64_t>(AssetStatus::NORMAL);
    coveredAssets.push_back(asset);
    covered["field"] = coveredAssets;
    /**
     * @tc.steps:step3. Tag assets.
     * @tc.expected: step3. Local asset change hash
     */
    Field field;
    field.colName = "field";
    field.type = TYPE_INDEX<Assets>;
    int errCode = E_OK;
    Assets res = TagAssetsInSingleCol(covered, beCovered, field, false, errCode);
    for (const auto &item : res) {
        EXPECT_EQ(item.hash, asset.hash);
    }
}
}
#endif