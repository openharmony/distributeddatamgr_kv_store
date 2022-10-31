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
     * @tc.name KVManagerGetKVStorePromiseSucTest
     * @tc.desc Test Js Api KVManager.GetKVStore() successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KVManagerGetKVStorePromiseSucTest', 0, async function (done) {
        console.info('KVManagerGetKVStorePromiseSucTest');
        try {
            await kvManager.getKVStore(TEST_STORE_ID, options).then((err, store) => {
                console.info('KVManagerGetKVStorePromiseSucTest getKVStore success');
                kvStore = store;
                done();
            })
        } catch (e) {
            console.error('KVManagerGetKVStorePromiseSucTest getKVStore e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name KVManagerGetKVStorePromiseParaError1Test
     * @tc.desc Test Js Api KVManager.GetKVStore() with parameter error
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KVManagerGetKVStorePromiseParaError1Test', 0, async function (done) {
        console.info('KVManagerGetKVStorePromiseParaError1Test');
        try {
            await kvManager.getKVStore(TEST_STORE_ID).then((store) => {
                console.info('KVManagerGetKVStorePromiseParaError1Test getKVStore success');
                expect(null).assertFail();
            }).catch((err) => {
                console.error('KVManagerGetKVStorePromiseParaError1Test getKVStore err ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(err.code == 401).assertTrue();
            });
        } catch (e) {
            console.error('KVManagerGetKVStorePromiseParaError1Test getKVStore e ' + `, error code is ${e.code}, message is ${e.message}`);
        }
        done();
    })

    /**
     * @tc.name KVManagerGetKVStorePromiseParaError2Test
     * @tc.desc Test Js Api KVManager.GetKVStore()with parameter error
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KVManagerGetKVStorePromiseParaError2Test', 0, async function (done) {
        console.info('KVManagerGetKVStorePromiseParaError2Test');
        try {
            await kvManager.getKVStore(options).then((store) => {
                console.info('KVManagerGetKVStorePromiseParaError2Test getKVStore success');
                expect(null).assertFail();
            }).catch((err) => {
                console.error('KVManagerGetKVStorePromiseParaError2Test getKVStore err ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(err.code == 401).assertTrue();
            });
        } catch (e) {
            console.error('KVManagerGetKVStorePromiseParaError2Test getKVStore e ' + `, error code is ${e.code}, message is ${e.message}`);
        }
        done();
    })

    /**
     * @tc.name KVManagerGetKVStorePromiseSingleS1Test
     * @tc.desc Test Js Api KVManager.GetKVStore() single s1
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KVManagerGetKVStorePromiseSingleS1Test', 0, async function (done) {
        console.info('KVManagerGetKVStorePromiseSingleS1Test');
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
            console.info('KVManagerGetKVStorePromiseSingleS1Test getKVStore success');
            expect(null).assertFail();
        }).catch((err) => {
            console.error('KVManagerGetKVStorePromiseSingleS1Test getKVStore err ' + `, error code is ${err.code}, message is ${err.message}`);
        });
        done();
    })

    /**
     * @tc.name KVManagerGetKVStorePromiseSingleS2Test
     * @tc.desc Test Js Api KVManager.GetKVStore() single s2
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KVManagerGetKVStorePromiseSingleS2Test', 0, async function (done) {
        console.info('KVManagerGetKVStorePromiseSingleS2Test');
        const optionsInfo = {
            createIfMissing: true,
            encrypt: true,
            backup: false,
            autoSync: true,
            kvStoreType: factory.KVStoreType.SINGLE_VERSION,
            schema: '',
            securityLevel: factory.SecurityLevel.S2,
        }
        await kvManager.getKVStore(TEST_STORE_ID, optionsInfo).then((store) => {
            console.info('KVManagerGetKVStorePromiseSingleS2Test getKVStore success');
            expect(store != null).assertTrue();
        }).catch((err) => {
            console.error('KVManagerGetKVStorePromiseSingleS2Test getKVStore err ' + `, error code is ${err.code}, message is ${err.message}`);
            expect(null).assertFail();
        });
        done();
    })

    /**
     * @tc.name KVManagerGetKVStorePromiseSingleS3Test
     * @tc.desc Test Js Api KVManager.GetKVStore() single s3
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KVManagerGetKVStorePromiseSingleS3Test', 0, async function (done) {
        console.info('KVManagerGetKVStorePromiseSingleS3Test');
        const optionsInfo = {
            createIfMissing: true,
            encrypt: false,
            backup: false,
            autoSync: true,
            kvStoreType: factory.KVStoreType.SINGLE_VERSION,
            schema: '',
            securityLevel: factory.SecurityLevel.S3,
        }
        await kvManager.getKVStore(TEST_STORE_ID, optionsInfo).then((store) => {
            console.info('KVManagerGetKVStorePromiseSingleS3Test getKVStore success');
            kvStore = store;
            expect(store != null).assertTrue();
        }).catch((err) => {
            console.error('KVManagerGetKVStorePromiseSingleS3Test getKVStore err ' + `, error code is ${err.code}, message is ${err.message}`);
            expect(null).assertFail();
        });
        done();
    })

    /**
     * @tc.name KVManagerGetKVStorePromiseSingleS4Test
     * @tc.desc Test Js Api KVManager.GetKVStore() single s4
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KVManagerGetKVStorePromiseSingleS4Test', 0, async function (done) {
        console.info('KVManagerGetKVStorePromiseSingleS4Test');
        const optionsInfo = {
            createIfMissing: true,
            encrypt: false,
            backup: true,
            autoSync: true,
            kvStoreType: factory.KVStoreType.SINGLE_VERSION,
            schema: '',
            securityLevel: factory.SecurityLevel.S4,
        }
        await kvManager.getKVStore(TEST_STORE_ID, optionsInfo).then((store) => {
            console.info('KVManagerGetKVStorePromiseSingleS4Test getKVStore success');
            kvStore = store;
            expect(store != null).assertTrue();
        }).catch((err) => {
            console.error('KVManagerGetKVStorePromiseSingleS4Test getKVStore err ' + `, error code is ${err.code}, message is ${err.message}`);
            expect(null).assertFail();
        });
        done();
    })

    /**
     * @tc.name KVManagerGetKVStorePromiseDeviceS1Test
     * @tc.desc Test Js Api KVManager.GetKVStore() device s1
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KVManagerGetKVStorePromiseDeviceS1Test', 0, async function (done) {
        console.info('KVManagerGetKVStorePromiseDeviceS1Test');
        const optionsInfo = {
            createIfMissing: true,
            encrypt: false,
            backup: false,
            autoSync: false,
            kvStoreType: factory.KVStoreType.DEVICE_COLLABORATION,
            schema: '',
            securityLevel: factory.SecurityLevel.S1,
        }
        await kvManager.getKVStore(TEST_STORE_ID, optionsInfo).then((store) => {
            console.info('KVManagerGetKVStorePromiseDeviceS1Test getKVStore success');
            kvStore = store;
            expect(store != null).assertTrue();
        }).catch((err) => {
            console.error('KVManagerGetKVStorePromiseDeviceS1Test getKVStore err ' + `, error code is ${err.code}, message is ${err.message}`);
            expect(null).assertFail();
        });
        done();
    })

    /**
     * @tc.name KVManagerGetKVStorePromiseDeviceS2Test
     * @tc.desc Test Js Api KVManager.GetKVStore() device s2
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KVManagerGetKVStorePromiseDeviceS2Test', 0, async function (done) {
        console.info('KVManagerGetKVStorePromiseDeviceS2Test');
        const optionsInfo = {
            createIfMissing: true,
            encrypt: false,
            backup: false,
            autoSync: true,
            kvStoreType: factory.KVStoreType.DEVICE_COLLABORATION,
            schema: '',
            securityLevel: factory.SecurityLevel.S2,
        }
        await kvManager.getKVStore(TEST_STORE_ID, optionsInfo).then((store) => {
            console.info('KVManagerGetKVStorePromiseDeviceS2Test getKVStore success');
            kvStore = store;
            expect(store != null).assertTrue();
        }).catch((err) => {
            console.error('KVManagerGetKVStorePromiseDeviceS2Test getKVStore err ' + `, error code is ${err.code}, message is ${err.message}`);
            expect(null).assertFail();
        });
        done();
    })

    /**
     * @tc.name KVManagerGetKVStorePromiseDeviceS3Test
     * @tc.desc Test Js Api KVManager.GetKVStore() device s3
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KVManagerGetKVStorePromiseDeviceS3Test', 0, async function (done) {
        console.info('KVManagerGetKVStorePromiseDeviceS3Test');
        const optionsInfo = {
            createIfMissing: true,
            encrypt: false,
            backup: false,
            autoSync: true,
            kvStoreType: factory.KVStoreType.DEVICE_COLLABORATION,
            schema: '',
            securityLevel: factory.SecurityLevel.S3,
        }
        await kvManager.getKVStore(TEST_STORE_ID, optionsInfo).then((store) => {
            console.info('KVManagerGetKVStorePromiseDeviceS3Test getKVStore success');
            kvStore = store;
            expect(store != null).assertTrue();
        }).catch((err) => {
            console.error('KVManagerGetKVStorePromiseDeviceS3Test getKVStore err ' + `, error code is ${err.code}, message is ${err.message}`);
            expect(null).assertFail();
        });
        done();
    })

    /**
     * @tc.name KVManagerGetKVStorePromiseDeviceS4Test
     * @tc.desc Test Js Api KVManager.GetKVStore() device s4
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KVManagerGetKVStorePromiseDeviceS4Test', 0, async function (done) {
        console.info('KVManagerGetKVStorePromiseDeviceS4Test');
        const optionsInfo = {
            createIfMissing: true,
            encrypt: false,
            backup: false,
            autoSync: true,
            kvStoreType: factory.KVStoreType.DEVICE_COLLABORATION,
            schema: '',
            securityLevel: factory.SecurityLevel.S4,
        }
        try {
            await kvManager.getKVStore(TEST_STORE_ID, optionsInfo).then((store) => {
                console.info('KVManagerGetKVStorePromiseDeviceS4Test getKVStore success');
            }).catch((err) => {
                console.error('KVManagerGetKVStorePromiseDeviceS4Test getKVStore err ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('KVManagerGetKVStorePromiseDeviceS4Test getKVStore e ' + `, error code is ${e.code}, message is ${e.message}`);
        }
        done();
    })

    /**
     * @tc.name KVManagerCloseKVStorePromiseSucTest
     * @tc.desc Test Js Api KVManager.CloseKVStore() successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KVManagerCloseKVStorePromiseSucTest', 0, async function (done) {
        console.info('KVManagerCloseKVStorePromiseTest004');
        await kvManager.getKVStore(TEST_STORE_ID, options).then( async () => {
            console.info('KVManagerCloseKVStorePromiseSucTest getKVStore success');
            await kvManager.closeKVStore(TEST_BUNDLE_NAME, TEST_STORE_ID).then(() => {
                console.info('KVManagerCloseKVStorePromiseSucTest closeKVStore success');
                expect(true).assertTrue();
            }).catch((err) => {
                console.error('KVManagerCloseKVStorePromiseSucTest closeKVStore err ' + `, error code is ${err.code}, message is ${err.message}`);
            });
        });
        done();
    })

    /**
     * @tc.name KVManagerDeleteKVStorePromiseSucTest
     * @tc.desc Test Js Api KVManager.DeleteKVStore() successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KVManagerDeleteKVStorePromiseSucTest', 0, async function (done) {
        console.info('KVManagerDeleteKVStorePromiseSucTest');
        await kvManager.getKVStore(TEST_STORE_ID, options, async function (err, store) {
            console.info('KVManagerCloseKVStorePromiseSucTest getKVStore success');
            kvStore = store;
            await kvManager.closeKVStore(TEST_BUNDLE_NAME, TEST_STORE_ID);
        });
        await kvManager.deleteKVStore(TEST_BUNDLE_NAME, TEST_STORE_ID).then(() => {
            console.info('KVManagerDeleteKVStorePromiseSucTest deleteKVStore success');
        }).catch((err) => {
            expect(null).assertFail();
            console.error('KVManagerDeleteKVStorePromiseSucTest deleteKVStore err ' + `, error code is ${err.code}, message is ${err.message}`);
        });
        done();
    })

    /**
     * @tc.name KVManagerGetAllKVStoreIdPromiseEqua0Test
     * @tc.desc Test Js Api KVManager.GetAllKVStoreId() equals 0
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KVManagerGetAllKVStoreIdPromiseEqua0Test', 0, async function (done) {
        console.info('KVManagerGetAllKVStoreIdPromiseEqua0Test');
        await kvManager.getAllKVStoreId(TEST_BUNDLE_NAME).then((data) => {
            expect(0).assertEqual(data.length);
        }).catch((err) => {
            console.error('KVManagerGetAllKVStoreIdPromiseEqua0Test getAllKVStoreId err ' + `, error code is ${err.code}, message is ${err.message}`);
            expect(null).assertFail();
        });
        done();
    })

    /**
     * @tc.name KVManagerGetAllKVStoreIdPromiseEqua1Test
     * @tc.desc Test Js Api KVManager.GetAllKVStoreId() equals 1
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KVManagerGetAllKVStoreIdPromiseEqua1Test', 0, async function (done) {
        console.info('KVManagerGetAllKVStoreIdPromiseEqua1Test');
        await kvManager.getKVStore(TEST_STORE_ID, options).then(async (store) => {
            console.info('KVManagerGetAllKVStoreIdPromiseEqua1Test getKVStore success');
            kvStore = store;
            await kvManager.getAllKVStoreId(TEST_BUNDLE_NAME).then((data) => {
                expect(1).assertEqual(data.length);
                expect(TEST_STORE_ID).assertEqual(data[0]);
            }).catch((err) => {
                console.error('KVManagerGetAllKVStoreIdPromiseEqua1Test getAllKVStoreId err ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        }).catch((err) => {
            console.error('KVManagerGetAllKVStoreIdPromiseEqua1Test getKVStore err ' + `, error code is ${err.code}, message is ${err.message}`);
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
     * @tc.desc Test Js Api KVManager.On() twice
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
     * @tc.name KVStorePutPromiseTest
     * @tc.desc Test Js Api KVStore.Put() successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KVStorePutPromiseTest', 0, async function (done) {
        console.info('KVStorePutPromiseTest');
        try {
            await kvStoreNew.put(TEST_BUNDLE_NAME, TEST_STORE_ID).then((data) => {
                if (err != undefined){
                    console.info('KVStorePutPromiseTest put promise fail');
                } else {
                    console.info('KVStorePutPromiseTest put promise success');
                    expect(null).assertFail();
                }
                done();
            });
        } catch (e) {
            console.error('KVStorePutPromiseTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            done();
        }
    })

    /**
     * @tc.name: KVStoreDeletePromiseTest
     * @tc.desc: Test Js Api KVManager.Delete
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KVStoreDeletePromiseTest', 0, async function (done) {
        console.info('KVStoreDeletePromiseTest');
        try {
            kvStoreNew.put(STORE_KEY, STORE_VALUE).then((data) => {
                console.info('KVStoreDeletePromiseTest getKVStore success');
                kvStoreNew.delete(STORE_KEY).then((data) => {
                    console.info("testKVStoreDelete001  promise delete success");
                    expect(null).assertFail();
                }).catch((err) => {
                    console.error('KVStoreDeletePromiseTest promise delete fail err' + `, error code is ${err.code}, message is ${err.message}`);
                });
            }).catch((err) => {
                console.error('KVStoreDeletePromiseTest promise delete fail err' + `, error code is ${err.code}, message is ${err.message}`);
            });
        }catch (e) {
            console.error('KVStoreDeletePromiseTest promise delete fail err' + `, error code is ${err.code}, message is ${err.message}`);
        }
        done();
    })

    /**
     * @tc.name: CreateKVManagerPromiseFullFuncTest
     * @tc.desc: Test Js Api createKVManager full functions test
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
     it('CreateKVManagerPromiseFullFuncTest', 0, async function (done) {
        console.info('CreateKVManagerPromiseFullFuncTest');
        const config = {
            bundleName: TEST_BUNDLE_NAME,
            context:context
        }
        try {
            await factory.createKVManager(config).then(async (manager) => {
                kvManager = manager;
                expect(manager !=null).assertTrue();
                console.info('CreateKVManagerPromiseFullFuncTest createKVManager success');
                await kvManager.getKVStore(TEST_STORE_ID, options).then(async (store) => {
                    console.info("CreateKVManagerPromiseFullFuncTest getKVStore success");
                    await store.put(STORE_KEY, STORE_VALUE).then(async (data) => {
                        console.info('CreateKVManagerPromiseFullFuncTest put data success');
                        await store.get(STORE_KEY).then((data) => {
                            console.info("CreateKVManagerPromiseFullFuncTest  get data success");
                            expect(data).assertEqual(STORE_VALUE);
                        }).catch((err) => {
                            console.error('CreateKVManagerPromiseFullFuncTest get data err' + `, error code is ${err.code}, message is ${err.message}`);
                        });
                    }).catch((err) => {
                        console.error('CreateKVManagerPromiseFullFuncTest put data err' + `, error code is ${err.code}, message is ${err.message}`);
                    });
                }).catch((err) => {
                    console.info("CreateKVManagerPromiseFullFuncTest getKVStore err: "  + JSON.stringify(err));
                    expect(null).assertFail();
                });
            }).catch((err) => {
                console.error('CreateKVManagerPromiseFullFuncTest createKVManager err ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail()
            });
        }catch (e) {
            console.error('CreateKVManagerPromiseFullFuncTest promise delete fail err' + `, error code is ${err.code}, message is ${err.message}`);
            expect(null).assertFail();
        }
        done();
    })
})
