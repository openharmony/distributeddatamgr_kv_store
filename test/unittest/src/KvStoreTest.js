/*
 * Copyright (C) 2021 XXXX Device Co., Ltd.
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

import app from '@system.app'
import distributedData from '@ohos.data.distributedData'
import {describe, beforeAll, beforeEach, afterEach, afterAll, it, expect} from 'deccjsunit/index'

const TEST_BUNDLE_NAME = 'com.example.myapplication';
const TEST_STORE_ID = 'storeId';
var kvManager = null;
var kvStore = null;

describe("KvStoreTest", function () {
    const config = {
        bundleName: TEST_BUNDLE_NAME,
        userInfo: {
            userId: '0',
            userType: distributedData.UserType.SAME_USER_ID
        }
    }

    const options = {
        createIfMissing: true,
        encrypt: false,
        backup: false,
        autoSync: true,
        kvStoreType: distributedData.KVStoreType.SINGLE_VERSION,
        schema: '',
        securityLevel: distributedData.SecurityLevel.S2,
    }


    beforeAll(async function (done) {
        console.info('beforeAll');
        await distributedData.createKVManager(config, function (err, manager) {
            kvManager = manager;
            done();
        });
        console.info('beforeAll end');
    })

    afterAll(function () {
        console.info('afterAll called')
    })

    beforeEach(function () {
        console.info('beforeEach called')
    })

    afterEach(async function (done) {
        console.info('afterEach');
        await kvManager.closeKVStore(TEST_BUNDLE_NAME, TEST_STORE_ID, kvStore, async function () {
            console.info('afterEach closeKVStore success');
            await kvManager.deleteKVStore(TEST_BUNDLE_NAME, TEST_STORE_ID, function () {
                console.info('afterEach deleteKVStore success');
                done();
            });
        });
        kvStore = null;
    })

    /*
     * @tc.name:appInfoTest001
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: issueNumber.
     */
    it("KvStoreInfoTest001", 0, async function (done) {
        console.info('testKVManagerGetKVStore101');
        try {
            await kvManager.getKVStore(TEST_STORE_ID, options, function (err, store) {
                console.info('testKVManagerGetKVStore101 getKVStore success');
                kvStore = store;
                done();
            });
        } catch (e) {
            console.info('testKVManagerGetKVStore101 getKVStore e ' + e);
            expect(null).assertFail();
            done();
        }
    })
})