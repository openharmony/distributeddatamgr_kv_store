/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
import factory from '@ohos.data.distributedKVStore'
import abilityFeatureAbility from '@ohos.ability.featureAbility';

let context = abilityFeatureAbility.getContext();

const TEST_BUNDLE_NAME = 'com.example.myapplication';
const TEST_STORE_ID = 'storeId';
let kvManager = null;
let kvStore = null;

describe('KVManagerCustomizeDirTest', function () {
    const config = {
        bundleName: TEST_BUNDLE_NAME,
        context: context
    }

    const options = {
        createIfMissing: true,
        encrypt: false,
        backup: false,
        autoSync: false,
        kvStoreType: factory.KVStoreType.SINGLE_VERSION,
        schema: '',
        rootDir: '/data/storage/el2/database/entry',
        securityLevel: factory.SecurityLevel.S2,
    }

    const backupConfig = {
      fileName: 'BK001',
      filePath: '/data/storage/el2/database/entry',
    };

    beforeAll(async function (done) {
        console.info('beforeAll');
        kvManager = factory.createKVManager(config);
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
        await kvManager.closeKVStore(TEST_BUNDLE_NAME, TEST_STORE_ID, options).then(async () => {
            console.info('afterEach closeKVStore success');
            await kvManager.deleteKVStore(TEST_BUNDLE_NAME, TEST_STORE_ID, options).then(() => {
                console.info('afterEach deleteKVStore success');
            }).catch((err) => {
                console.error('afterEach deleteKVStore err ' + `, error code is ${err.code}, message is ${err.message}`);
            });
        }).catch((err) => {
            console.error('afterEach closeKVStore err ' + `, error code is ${err.code}, message is ${err.message}`);
        });

        kvStore = null;
        done();
    })

    /**
     * @tc.name KVManagerCustomizeDirGetKVStorePromiseSucTest
     * @tc.desc Test Js Api KVManager.GetKVStore() successfully
     * @tc.type: FUNC
     */
    it('KVManagerCustomizeDirGetKVStorePromiseSucTest', 0, async function (done) {
        console.info('KVManagerCustomizeDirGetKVStorePromiseSucTest');
        try {
            await kvManager.getKVStore(TEST_STORE_ID, options).then((store) => {
                console.info('KVManagerCustomizeDirGetKVStorePromiseSucTest getKVStore success');
                expect(store != undefined && store != null).assertTrue();
            }).catch((err) => {
                console.error('KVManagerCustomizeDirGetKVStorePromiseSucTest getKVStore err ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('KVManagerCustomizeDirGetKVStorePromiseSucTest getKVStore e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name KVManagerCustomizeDirCloseKVStorePromiseSucTest
     * @tc.desc Test Js Api KVManager.CloseKVStore() successfully
     * @tc.type: FUNC
     */
    it('KVManagerCustomizeDirCloseKVStorePromiseSucTest', 0, async function (done) {
        console.info('KVManagerCustomizeDirCloseKVStorePromiseSucTest');
        await kvManager.getKVStore(TEST_STORE_ID, options).then(async () => {
            console.info('KVManagerCustomizeDirCloseKVStorePromiseSucTest getKVStore success');
            await kvManager.closeKVStore(TEST_BUNDLE_NAME, TEST_STORE_ID, options).then(() => {
                console.info('KVManagerCustomizeDirCloseKVStorePromiseSucTest closeKVStore success');
                expect(true).assertTrue();
            }).catch((err) => {
                console.error('KVManagerCustomizeDirCloseKVStorePromiseSucTest closeKVStore err ' + `, error code is ${err.code}, message is ${err.message}`);
            });
        });
        done();
    })

    /**
     * @tc.name KVManagerCustomizeDirDeleteKVStorePromiseSucTest
     * @tc.desc Test Js Api KVManager.DeleteKVStore() successfully
     * @tc.type: FUNC
     */
    it('KVManagerCustomizeDirDeleteKVStorePromiseSucTest', 0, async function (done) {
        console.info('KVManagerCustomizeDirDeleteKVStorePromiseSucTest');
        try {
            kvStore = await kvManager.getKVStore(TEST_STORE_ID, options);
            await kvManager.closeKVStore(TEST_BUNDLE_NAME, TEST_STORE_ID, options);
            await kvManager.deleteKVStore(TEST_BUNDLE_NAME, TEST_STORE_ID, options).then(() => {
                expect(true).assertTrue();
            }).catch((err) => {
                console.error('KVManagerCustomizeDirDeleteKVStorePromiseSucTest deleteKVStore err ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('KVManagerCustomizeDirDeleteKVStorePromiseSucTest getKVStore err ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name KVManagerCustomizeDirBrdExPromiseSucTest
     * @tc.desc Test Js Api KVManager.backupEx(),restoreEx(), deleteBackupEx() successfully
     * @tc.type: FUNC
     */
    it('KVManagerCustomizeDirBrdExPromiseSucTest', 0, async function (done) {
        console.info('KVManagerCustomizeDirBrdExPromiseSucTest');
        try {
            kvStore = await kvManager.getKVStore(TEST_STORE_ID, options);

            await kvStore.backupEx(backupConfig).then(() => {
                expect(true).assertTrue();
            }).catch((err) => {
                console.error('KVManagerCustomizeDirBrdExPromiseSucTest backupEx err ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });

            await kvStore.restoreEx(backupConfig).then(() => {
                expect(true).assertTrue();
            }).catch((err) => {
                console.error('KVManagerCustomizeDirBrdExPromiseSucTest restoreEx err ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
            await kvManager.closeKVStore(TEST_BUNDLE_NAME, TEST_STORE_ID, options);

            await kvStore.deleteBackupEx(backupConfig).then(() => {
                expect(true).assertTrue();
            }).catch((err) => {
                console.error('KVManagerCustomizeDirBrdExPromiseSucTest deleteBackupEx err ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('KVManagerCustomizeDirBrdExPromiseSucTest getKVStore err ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

})
