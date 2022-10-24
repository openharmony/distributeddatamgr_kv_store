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

import { describe, beforeAll, beforeEach, afterEach, afterAll, it, expect } from 'deccjsunit/index'
import factory from '@ohos.data.distributedKVStore'
import abilityFeatureAbility from '@ohos.ability.featureAbility';

var context = abilityFeatureAbility.getContext();
var contextApplication = context.getApplicationContext()

const TEST_BUNDLE_NAME = 'com.example.myapplication';
const TEST_STORE_ID = 'storeId';
var kvManager = null;
var kvStore = null;
const STORE_KEY = 'key_test_string';
const STORE_VALUE = 'value-test-string';
var kvStoreNew = null;

describe('KVManagerPromiseTest', function () {
    const config = {
        bundleName: TEST_BUNDLE_NAME,
        context:context
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
                console.info("beforeAll getKVStore err: "  + JSON.stringify(err));
            });
        }).catch((err) => {
            console.error('beforeAll createKVManager err ' + `, error code is ${err.code}, message is ${err.message}`);
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
                console.error('afterEach deleteKVStore err ' + `, error code is ${err.code}, message is ${err.message}`);
            });
        }).catch((err) => {
            console.error('afterEach closeKVStore err ' + `, error code is ${err.code}, message is ${err.message}`);
        });
        kvStore = null;
        done();
    })

    /**
     * @tc.name KVManagerGetKVStorePromiseTest001
     * @tc.desc Test Js Api KVManager.GetKVStore() testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KVManagerGetKVStorePromiseTest001', 0, async function (done) {
        console.info('KVManagerGetKVStorePromiseTest001');
        try {
            await kvManager.getKVStore(TEST_STORE_ID).then((store) => {
                console.info('KVManagerGetKVStorePromiseTest001 getKVStore success');
                expect(null).assertFail();
            }).catch((err) => {
                console.error('KVManagerGetKVStorePromiseTest001 getKVStore err ' + `, error code is ${err.code}, message is ${err.message}`);
            });
        } catch (e) {
            console.error('KVManagerGetKVStorePromiseTest001 getKVStore e ' + `, error code is ${e.code}, message is ${e.message}`);
        }
        done();
    })

    /**
     * @tc.name KVManagerGetKVStorePromiseTest002
     * @tc.desc Test Js Api KVManager.GetKVStore() testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KVManagerGetKVStorePromiseTest002', 0, async function (done) {
        console.info('KVManagerGetKVStorePromiseTest002');
        try {
            await kvManager.getKVStore(options).then((store) => {
                console.info('KVManagerGetKVStorePromiseTest002 getKVStore success');
                expect(null).assertFail();
            }).catch((err) => {
                console.error('KVManagerGetKVStorePromiseTest002 getKVStore err ' + `, error code is ${err.code}, message is ${err.message}`);
            });
        } catch (e) {
            console.error('KVManagerGetKVStorePromiseTest002 getKVStore e ' + `, error code is ${e.code}, message is ${e.message}`);
        }
        done();
    })

    /**
     * @tc.name KVManagerGetKVStorePromiseTest003
     * @tc.desc Test Js Api KVManager.GetKVStore() testcase 004
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KVManagerGetKVStorePromiseTest003', 0, async function (done) {
        console.info('KVManagerGetKVStorePromiseTest003');
        const optionsInfo = {
            createIfMissing: false,
            encrypt: false,
            backup: false,
            autoSync: true,
            kvStoreType: factory.KVStoreType.SINGLE_VERSION,
            schema: '',
            securityLevel: factory.SecurityLevel.S1,
        }
        await kvManager.getKVStore(TEST_STORE_ID, optionsInfo).then((store) => {
            console.info('KVManagerGetKVStorePromiseTest003 getKVStore success');
            expect(null).assertFail();
        }).catch((err) => {
            console.error('KVManagerGetKVStorePromiseTest003 getKVStore err ' + `, error code is ${err.code}, message is ${err.message}`);
        });
        done();
    })

    /**
     * @tc.name KVManagerCloseKVStorePromiseTest001
     * @tc.desc Test Js Api KVManager.CloseKVStore() testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KVManagerCloseKVStorePromiseTest001', 0, async function (done) {
        console.info('KVManagerCloseKVStorePromiseTest004');
        await kvManager.getKVStore(TEST_STORE_ID, options).then( async () => {
            console.info('KVManagerCloseKVStorePromiseTest001 getKVStore success');
            await kvManager.closeKVStore(TEST_BUNDLE_NAME, TEST_STORE_ID).then(() => {
                console.info('KVManagerCloseKVStorePromiseTest001 closeKVStore success');
                expect(true).assertTrue();
            }).catch((err) => {
                console.error('KVManagerCloseKVStorePromiseTest001 closeKVStore twice err ' + `, error code is ${err.code}, message is ${err.message}`);
            });
        });
        done();
    })

    /**
     * @tc.name KVManagerGetAllKVStoreIdPromiseTest001
     * @tc.desc Test Js Api KVManager.GetAllKVStoreId() testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KVManagerGetAllKVStoreIdPromiseTest001', 0, async function (done) {
        console.info('KVManagerGetAllKVStoreIdPromiseTest001');
        await kvManager.getAllKVStoreId(TEST_BUNDLE_NAME).then((data) => {
            console.info('KVManagerGetAllKVStoreIdPromiseTest001 getAllKVStoreId success');
            console.info('KVManagerGetAllKVStoreIdPromiseTest001 size = ' + data.length);
            expect(0).assertEqual(data.length);
        }).catch((err) => {
            console.error('KVManagerGetAllKVStoreIdPromiseTest001 getAllKVStoreId err ' + `, error code is ${err.code}, message is ${err.message}`);
            expect(null).assertFail();
        });
        done();
    })
    /**
     * @tc.name KVManagerOnPromiseTest001
     * @tc.desc Test Js Api KVManager.On() testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KVManagerOnPromiseTest001', 0, function (done) {
        console.info('KVManagerOnPromiseTest001');
        var deathCallback = function () {
            console.info('death callback call');
        }
        kvManager.on('distributedDataServiceDie', deathCallback);
        kvManager.off('distributedDataServiceDie', deathCallback);
        done();
    })

    /**
     * @tc.name KVManagerOnPromiseTest002
     * @tc.desc Test Js Api KVManager.On() testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KVManagerOnPromiseTest002', 0, function (done) {
        console.info('KVManagerOnPromiseTest002');
        var deathCallback1 = function () {
            console.info('death callback call');
        }
        var deathCallback2 = function () {
            console.info('death callback call');
        }
        kvManager.on('distributedDataServiceDie', deathCallback1);
        kvManager.on('distributedDataServiceDie', deathCallback2);
        kvManager.off('distributedDataServiceDie', deathCallback1);
        kvManager.off('distributedDataServiceDie', deathCallback2);
        done();
    })

    /**
     * @tc.name KVManagerOnPromiseTest003
     * @tc.desc Test Js Api KVManager.On() testcase 003
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KVManagerOnPromiseTest003', 0, function (done) {
        console.info('KVManagerOnPromiseTest003');
        var deathCallback = function () {
            console.info('death callback call');
        }
        kvManager.on('distributedDataServiceDie', deathCallback);
        kvManager.on('distributedDataServiceDie', deathCallback);
        kvManager.off('distributedDataServiceDie', deathCallback);
        done();
    })

    /**
     * @tc.name KVManagerOffPromiseTest001
     * @tc.desc Test Js Api KVManager.Off() testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KVManagerOffPromiseTest001', 0, function (done) {
        console.info('KVManagerOffPromiseTest001');
        var deathCallback = function () {
            console.info('death callback call');
        }
        kvManager.off('distributedDataServiceDie', deathCallback);
        done();
    })

    /**
     * @tc.name KVManagerOffPromiseTest002
     * @tc.desc Test Js Api KVManager.Off() testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KVManagerOffPromiseTest002', 0, function (done) {
        console.info('KVManagerOffPromiseTest002');
        var deathCallback = function () {
            console.info('death callback call');
        }
        kvManager.on('distributedDataServiceDie', deathCallback);
        kvManager.off('distributedDataServiceDie', deathCallback);
        done();
    })

    /**
     * @tc.name KVManagerOffPromiseTest003
     * @tc.desc Test Js Api KVManager.Off() testcase 003
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KVManagerOffPromiseTest003', 0, function (done) {
        console.info('KVManagerOffPromiseTest003');
        var deathCallback1 = function () {
            console.info('death callback call');
        }
        var deathCallback2 = function () {
            console.info('death callback call');
        }
        kvManager.on('distributedDataServiceDie', deathCallback1);
        kvManager.on('distributedDataServiceDie', deathCallback2);
        kvManager.off('distributedDataServiceDie', deathCallback1);
        done();
    })

    /**
     * @tc.name KVManagerOffPromiseTest004
     * @tc.desc Test Js Api KVManager.Off() testcase 004
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KVManagerOffPromiseTest004', 0, function (done) {
        console.info('KVManagerOffPromiseTest004');
        var deathCallback = function () {
            console.info('death callback call');
        }
        kvManager.on('distributedDataServiceDie', deathCallback);
        kvManager.off('distributedDataServiceDie', deathCallback);
        kvManager.off('distributedDataServiceDie', deathCallback);
        done();
    })

    /**
     * @tc.name KVManagerOffPromiseTest005
     * @tc.desc Test Js Api KVManager.Off() testcase 005
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KVManagerOffPromiseTest005', 0, function (done) {
        console.info('KVManagerOffPromiseTest001');
        var deathCallback = function () {
            console.info('death callback call');
        }
        kvManager.on('distributedDataServiceDie', deathCallback);
        kvManager.off('distributedDataServiceDie');
        done();
    })

    /**
     * @tc.name KVStorePutPromiseTest001
     * @tc.desc Test Js Api KVStore.Put() testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KVStorePutPromiseTest001', 0, async function (done) {
        console.info('KVStorePutPromiseTest001');
        try {
            await kvStoreNew.put(TEST_BUNDLE_NAME, TEST_STORE_ID).then((data) => {
                if (err != undefined){
                    console.info('KVStorePutPromiseTest001 put promise fail');
                } else {
                    console.info('KVStorePutPromiseTest001 put promise success');
                    expect(null).assertFail();
                }
                done();
            });
        } catch (e) {
            console.error('KVStorePutPromiseTest001 e ' + `, error code is ${e.code}, message is ${e.message}`);
            done();
        }
    })

    /**
     * @tc.name: KVStoreDeletePromiseTest001
     * @tc.desc: Test Js Api KVManager.Delete testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KVStoreDeletePromiseTest001', 0, async function (done) {
        console.info('KVStoreDeletePromiseTest001');
        try {
            kvStoreNew.put(STORE_KEY, STORE_VALUE).then((data) => {
                console.info('KVStoreDeletePromiseTest001 getKVStore success');
                kvStoreNew.delete(STORE_KEY).then((data) => {
                    console.info("testKVStoreDelete001  promise delete success");
                    expect(null).assertFail();
                }).catch((err) => {
                    console.error('KVStoreDeletePromiseTest001 promise delete fail err' + `, error code is ${err.code}, message is ${err.message}`);
                });
            }).catch((err) => {
                console.error('KVStoreDeletePromiseTest001 promise delete fail err' + `, error code is ${err.code}, message is ${err.message}`);
            });
        }catch (e) {
            console.error('KVStoreDeletePromiseTest001 promise delete fail err' + `, error code is ${err.code}, message is ${err.message}`);
        }
        done();
    })

    /**
     * @tc.name: CreateKVManagerTestPromiseTest001
     * @tc.desc: Test Js Api createKVManager testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
     it('CreateKVManagerTestPromiseTest001', 0, async function (done) {
        console.info('CreateKVManagerTestPromiseTest001');
        const config = {
            bundleName: TEST_BUNDLE_NAME,
            context:context
        }
        try {
            await factory.createKVManager(config).then(async (manager) => {
                kvManager = manager;
                expect(manager !=null).assertTrue();
                console.info('CreateKVManagerTestPromiseTest001 createKVManager success');
                await kvManager.getKVStore(TEST_STORE_ID, options).then(async (store) => {
                    console.info("testcreateKVManager001 getKVStore success");
                    await store.put(STORE_KEY, STORE_VALUE).then(async (data) => {
                        console.info('CreateKVManagerTestPromiseTest001 put data success');
                        await store.get(STORE_KEY).then((data) => {
                            console.info("testcreateKVManager001  get data success");
                            expect(data).assertEqual(STORE_VALUE);
                        }).catch((err) => {
                            console.error('CreateKVManagerTestPromiseTest001 get data err' + `, error code is ${err.code}, message is ${err.message}`);
                        });
                    }).catch((err) => {
                        console.error('CreateKVManagerTestPromiseTest001 put data err' + `, error code is ${err.code}, message is ${err.message}`);
                    });
                }).catch((err) => {
                    console.info("testcreateKVManager001 getKVStore err: "  + JSON.stringify(err));
                    expect(null).assertFail();
                });
            }).catch((err) => {
                console.error('CreateKVManagerTestPromiseTest001 createKVManager err ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail()
            });
        }catch (e) {
            console.error('CreateKVManagerTestPromiseTest001 promise delete fail err' + `, error code is ${err.code}, message is ${err.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name: CreateKVManagerTestPromiseTest002
     * @tc.desc: Test Js Api createKVManager testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
     it('CreateKVManagerTestPromiseTest002', 0, async function (done) {
        console.info('CreateKVManagerTestPromiseTest002');
        const config = {
            bundleName: TEST_BUNDLE_NAME,
            context:contextApplication
        }
        try {
            await factory.createKVManager(config).then(async (manager) => {
                kvManager = manager;
                console.info('CreateKVManagerTestPromiseTest002 createKVManager success');
                await kvManager.getKVStore(TEST_STORE_ID, options).then(async (store) => {
                    console.info("testcreateKVManager002 getKVStore success");
                    await store.put(STORE_KEY, STORE_VALUE).then(async (data) => {
                        console.info('CreateKVManagerTestPromiseTest002 put data success');
                        await store.get(STORE_KEY).then((data) => {
                            console.info("testcreateKVManager002  get data success");
                            expect(data).assertEqual(STORE_VALUE);
                        }).catch((err) => {
                            console.error('CreateKVManagerTestPromiseTest002 get data err' + `, error code is ${err.code}, message is ${err.message}`);
                        });
                    }).catch((err) => {
                        console.error('CreateKVManagerTestPromiseTest002 put data err' + `, error code is ${err.code}, message is ${err.message}`);
                    });
                }).catch((err) => {
                    console.info("testcreateKVManager002 getKVStore err: "  + JSON.stringify(err));
                    expect(null).assertFail();
                });
            }).catch((err) => {
                console.error('CreateKVManagerTestPromiseTest002 createKVManager err ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail()
            });
        }catch (e) {
            console.error('CreateKVManagerTestPromiseTest002 promise delete fail err' + `, error code is ${err.code}, message is ${err.message}`);
            expect(null).assertFail();
        }
        done();
    })
})
