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

import {describe, beforeAll, beforeEach, afterEach, afterAll, it, expect} from 'deccjsunit/index'
import factory from '@ohos.data.distributedData';
import abilityFeatureAbility from '@ohos.ability.featureAbility';

var context = abilityFeatureAbility.getContext();
const TEST_BUNDLE_NAME = 'com.example.myapplication';
const TEST_STORE_ID = 'storeId';
var kvManager = null;
var kvStore = null;
var kvStoreNew = null;

describe('kvStoreTest', function () {
    const config = {
        bundleName: TEST_BUNDLE_NAME,
        userInfo: {
            userId: '0',
            userType: factory.UserType.SAME_USER_ID
        },
        context: context
    }

    const options = {
        createIfMissing: true,
        encrypt: false,
        backup: false,
        autoSync: true,
        kvStoreType: factory.KVStoreType.SINGLE_VERSION,
        schema: '',
        securityLevel: factory.SecurityLevel.S2,
    }

    beforeAll(async function (done) {
        console.info('beforeAll');
        await factory.createKVManager(config).then((manager) => {
            kvManager = manager;
            console.info('beforeAll createKVManager success');
            kvManager.getKVStore(TEST_STORE_ID, options).then((store) => {
                console.info("beforeAll getKVStore success");
                kvStoreNew = store;
            }).catch((err) => {
                console.info("beforeAll getKVStore err: " + JSON.stringify(err));
            });
        }).catch((err) => {
            console.info('beforeAll createKVManager err ' + err);
        });
        console.info('beforeAll end');
        done();
    })

    afterAll(async function (done) {
        console.info('afterAll');
        done();
    })

    beforeEach(async function (done) {
        console.info('beforeEach');
        done();
    })

    afterEach(async function (done) {
        console.info('afterEach');
        await kvManager.closeKVStore(TEST_BUNDLE_NAME, TEST_STORE_ID, kvStore).then(async () => {
            console.info('afterEach closeKVStore success');
            await kvManager.deleteKVStore(TEST_BUNDLE_NAME, TEST_STORE_ID).then(() => {
                console.info('afterEach deleteKVStore success');
            }).catch((err) => {
                console.info('afterEach deleteKVStore err ' + err);
            });
        }).catch((err) => {
            console.info('afterEach closeKVStore err ' + err);
        });
        kvStore = null;
        done();
    })

    /*
     * @tc.name:KVJsTestDemo
     * @tc.desc:verify app info is not null
     * @tc.type: FUNC
     * @tc.require: issueNumber.
     */
    it('KVJsTestDemo', 0, async function (done) {
        console.info('KVJsTestDemo');
        const optionsInfo = {
            createIfMissing: true,
            encrypt: false,
            backup: false,
            autoSync: true,
            kvStoreType: factory.KVStoreType.SINGLE_VERSION,
            schema: '',
            securityLevel: factory.SecurityLevel.NO_LEVEL,
        }
        await kvManager.getKVStore(TEST_STORE_ID, optionsInfo).then((store) => {
            console.info('KVJsTestDemo getKVStore success');
            kvStore = store;
            expect(store != null).assertTrue();
        }).catch((err) => {
            console.info('KVJsTestDemo getKVStore err ' + err);
            expect(null).assertFail();
        });
        done();
    })
})