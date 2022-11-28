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

describe('KVManagerPromiseTest', function () {
    const config = {
        bundleName: TEST_BUNDLE_NAME,
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
        await kvManager.closeKVStore(TEST_BUNDLE_NAME, TEST_STORE_ID).then(async () => {
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
            await kvManager.getKVStore(TEST_STORE_ID, options).then((store) => {
                console.info('KVManagerGetKVStorePromiseSucTest getKVStore success');
                expect(store != undefined && store != null).assertTrue();
            }).catch((err) => {
                console.error('KVManagerGetKVStorePromiseSucTest getKVStore err ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('KVManagerGetKVStorePromiseSucTest getKVStore e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
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
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('KVManagerGetKVStorePromiseParaError1Test getKVStore e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(e.code == 401).assertTrue();
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
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('KVManagerGetKVStorePromiseParaError2Test getKVStore e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(e.code == 401).assertTrue();
        }
        done();
    })

    /**
     * @tc.name KVManagerGetKVStorePromiseAlreadyOpenTest
     * @tc.desc Test Js Api KVManager.GetKVStore() get an already open kvstore
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KVManagerGetKVStorePromiseAlreadyOpenTest', 0, async function (done) {
        console.info('KVManagerGetKVStoreCallbackSucTest');
        try {
           await kvManager.getKVStore(TEST_STORE_ID, options).then(async (store) => {
                expect(store != undefined).assertTrue();
                kvStore = store;
                const optionsInfo = {
                    createIfMissing: true,
                    encrypt: false,
                    backup: false,
                    autoSync: true,
                    kvStoreType: factory.KVStoreType.DEVICE_COLLABORATION,
                    schema: '',
                    securityLevel: factory.SecurityLevel.S1,
                }
                await kvManager.getKVStore(TEST_STORE_ID, optionsInfo).then(() => {
                    expect(null).assertFail();
                }).catch((error) => {
                    console.info("throw exception success");
                    expect(error.code == 15100002).assertTrue();
                });
            }).catch((err) => {
                expect(null).assertFail();
            });
        } catch (e) {
            expect(null).assertFail();
        }
        await kvManager.closeKVStore(TEST_BUNDLE_NAME, TEST_STORE_ID).then(async () => {
            await kvManager.deleteKVStore(TEST_BUNDLE_NAME, TEST_STORE_ID).then(() => {
                expect(true).assertTrue();
            }).catch((err) => {
                console.error('deleteKVStore err ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        })
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
            createIfMissing: true,
            encrypt: false,
            backup: false,
            autoSync: true,
            kvStoreType: factory.KVStoreType.SINGLE_VERSION,
            schema: '',
            securityLevel: factory.SecurityLevel.S1,
        };
        try {
            await kvManager.getKVStore(TEST_STORE_ID, optionsInfo).then((store) => {
                console.info('KVManagerGetKVStorePromiseSingleS1Test getKVStore success');
                expect(store != null && store != undefined).assertTrue();
            }).catch((err) => {
                console.error('KVManagerGetKVStorePromiseSingleS1Test getKVStore err ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            expect(null).assertFail();
        }
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
        try {
            await kvManager.getKVStore(TEST_STORE_ID, optionsInfo).then((store) => {
                console.info('KVManagerGetKVStorePromiseSingleS2Test getKVStore success');
                expect(store != null && store != undefined).assertTrue();
            }).catch((err) => {
                console.error('KVManagerGetKVStorePromiseSingleS2Test getKVStore err ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            expect(null).assertFail();
        }
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
        try {
            await kvManager.getKVStore(TEST_STORE_ID, optionsInfo).then((store) => {
                console.info('KVManagerGetKVStorePromiseSingleS3Test getKVStore success');
                expect(store != null && store != undefined).assertTrue();
            }).catch((err) => {
                console.error('KVManagerGetKVStorePromiseSingleS3Test getKVStore err ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            expect(null).assertFail();
        }
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
        try {
            await kvManager.getKVStore(TEST_STORE_ID, optionsInfo).then((store) => {
                console.info('KVManagerGetKVStorePromiseSingleS4Test getKVStore success');
                expect(store != null && store != undefined).assertTrue();
            }).catch((err) => {
                console.error('KVManagerGetKVStorePromiseSingleS4Test getKVStore err ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            expect(null).assertFail();
        }
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
        try {
            await kvManager.getKVStore(TEST_STORE_ID, optionsInfo).then((store) => {
                console.info('KVManagerGetKVStorePromiseDeviceS1Test getKVStore success');
                expect(store != null && store != undefined).assertTrue();
            }).catch((err) => {
                console.error('KVManagerGetKVStorePromiseDeviceS1Test getKVStore err ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            expect(null).assertFail();
        }
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
        try {
            await kvManager.getKVStore(TEST_STORE_ID, optionsInfo).then((store) => {
                console.info('KVManagerGetKVStorePromiseDeviceS2Test getKVStore success');
                expect(store != null && store != undefined).assertTrue();
            }).catch((err) => {
                console.error('KVManagerGetKVStorePromiseDeviceS2Test getKVStore err ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            expect(null).assertFail();
        }
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
        try {
            await kvManager.getKVStore(TEST_STORE_ID, optionsInfo).then((store) => {
                console.info('KVManagerGetKVStorePromiseDeviceS3Test getKVStore success');
                expect(store != null && store != undefined).assertTrue();
            }).catch((err) => {
                console.error('KVManagerGetKVStorePromiseDeviceS3Test getKVStore err ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            expect(null).assertFail();
        }
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
                expect(store != null && store != undefined).assertTrue();
            }).catch((err) => {
                console.error('KVManagerGetKVStorePromiseDeviceS4Test getKVStore err ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            expect(null).assertFail();
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
        console.info('KVManagerCloseKVStorePromiseSucTest');
        await kvManager.getKVStore(TEST_STORE_ID, options).then(async () => {
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
     * @tc.name KVManagerCloseKVStorePromiseInvalidArgsTest
     * @tc.desc Test Js Api KVManager.CloseKVStore() with invalid args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KVManagerCloseKVStorePromiseInvalidArgsTest', 0, async function (done) {
        console.info('KVManagerCloseKVStorePromiseInvalidArgsTest');
        try {
            await kvManager.getKVStore(TEST_STORE_ID, options).then(async () => {
                console.info('KVManagerCloseKVStorePromiseInvalidArgsTest getKVStore success');
                await kvManager.closeKVStore(TEST_BUNDLE_NAME).then(() => {
                    console.info('KVManagerCloseKVStorePromiseInvalidArgsTest closeKVStore success');
                    expect(null).assertFail();
                }).catch((err) => {
                    console.error('KVManagerCloseKVStorePromiseInvalidArgsTest closeKVStore err ' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(null).assertFail();
                });
            });
        } catch (e) {
            console.error('KVManagerCloseKVStorePromiseInvalidArgsTest closeKVStore e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(e.code == 401).assertTrue();
        }
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
            console.info('KVManagerDeleteKVStorePromiseSucTest getKVStore success');
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
     * @tc.name KVManagerDeleteKVStorePromiseInvalidArgsTest
     * @tc.desc Test Js Api KVManager.DeleteKVStore() with invalid args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KVManagerDeleteKVStorePromiseInvalidArgsTest', 0, async function (done) {
        try {
            await kvManager.getKVStore(TEST_STORE_ID, options, async function (err, store) {
                console.info('KVManagerDeleteKVStorePromiseInvalidArgsTest getKVStore success');
                kvStore = store;
                await kvManager.closeKVStore(TEST_BUNDLE_NAME, TEST_STORE_ID);
            });
            await kvManager.deleteKVStore(TEST_BUNDLE_NAME).then(() => {
                console.info('KVManagerDeleteKVStorePromiseInvalidArgsTest deleteKVStore success');
                expect(null).assertFail();
            }).catch((err) => {
                expect(null).assertFail();
                console.error('KVManagerDeleteKVStorePromiseInvalidArgsTest deleteKVStore err ' + `, error code is ${err.code}, message is ${err.message}`);
            });
        } catch (e) {
            console.error('KVManagerDeleteKVStorePromiseInvalidArgsTest deleteKVStore e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(e.code == 401).assertTrue();
        }
        done();
    })

    /**
     * @tc.name KVManagerDeleteKVStorePromiseNoGetTest
     * @tc.desc Test Js Api KVManager.DeleteKVStore() without get
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KVManagerDeleteKVStorePromiseNoGetTest', 0, async function (done) {
        try {
            await kvManager.deleteKVStore(TEST_BUNDLE_NAME, TEST_STORE_ID).then(() => {
                console.info('KVManagerDeleteKVStorePromiseNoGetTest deleteKVStore success');
                expect(null).assertFail();
            }).catch((err) => {
                console.info('KVManagerDeleteKVStoreCallbackNotGetTest deleteKVStore fail');
                expect(err.code == 15100004).assertTrue();
            });
        } catch (e) {
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name KVManagerGetAllKVStoreIdPromiseSucTest
     * @tc.desc Test Js Api KVManager.GetAllKVStoreId() success
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KVManagerGetAllKVStoreIdPromiseSucTest', 0, async function (done) {
        console.info('KVManagerGetAllKVStoreIdPromiseSucTest');
        try {
            await kvManager.getAllKVStoreId(TEST_BUNDLE_NAME).then((data) => {
                expect(0 == data.length).assertTrue();
            }).catch((err) => {
                console.error('KVManagerGetAllKVStoreIdPromiseSucTest getAllKVStoreId err ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name KVManagerGetAllKVStoreIdPromiseInvalidArgsTest
     * @tc.desc Test Js Api KVManager.GetAllKVStoreId() with invalid args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KVManagerGetAllKVStoreIdPromiseInvalidArgsTest', 0, async function (done) {
        console.info('KVManagerGetAllKVStoreIdPromiseInvalidArgsTest');
        try {
            await kvManager.getAllKVStoreId().then((data) => {
                expect(null).assertFail();
            }).catch((err) => {
                console.error('KVManagerGetAllKVStoreIdPromiseInvalidArgsTest getAllKVStoreId err ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            expect(e.code == 401).assertTrue();
        }
        done();
    })

    /**
     * @tc.name KVManagerOnPromiseSucTest
     * @tc.desc Test Js Api KVManager.On() success
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KVManagerOnPromiseSucTest', 0, function (done) {
        console.info('KVManagerOnPromiseSucTest');
        var deathCallback = function () {
            console.info('death callback call');
        }
        try {
            kvManager.on('distributedDataServiceDie', deathCallback);
            kvManager.off('distributedDataServiceDie', deathCallback);
            expect(true).assertTrue();
        } catch (e) {
            console.error(`KVManagerOnPromiseSucTest failed, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name KVManagerOnPromiseInvalidArgsTest
     * @tc.desc Test Js Api KVManager.On() with invalid args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KVManagerOnPromiseInvalidArgsTest', 0, function (done) {
        console.info('KVManagerOnPromiseInvalidArgsTest');
        try {
            kvManager.on('distributedDataServiceDie');
            expect(null).assertFail();
        } catch (e) {
            console.error(`KVManagerOnPromiseInvalidArgsTest failed, error code is ${e.code}, message is ${e.message}`);
            expect(e.code == 401).assertTrue();
        }
        done();
    })

    /**
     * @tc.name KVManagerOffPromiseWithoutOnTest
     * @tc.desc Test Js Api KVManager.Off() without on
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KVManagerOffPromiseWithoutOnTest', 0, function (done) {
        console.info('KVManagerOffPromiseWithoutOnTest');
        var deathCallback = function () {
            console.info('death callback call');
        }
        try {
            kvManager.off('distributedDataServiceDie', deathCallback);
            expect(true).assertTrue();
        } catch (e) {
            console.error(`KVManagerOffPromiseWithoutOnTest failed, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name KVManagerOffPromiseInvalidArgsTest
     * @tc.desc Test Js Api KVManager.Off() with invalid args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KVManagerOffPromiseInvalidArgsTest', 0, function (done) {
        console.info('KVManagerOffPromiseInvalidArgsTest');
        try {
            kvManager.off();
            expect(false).assertFail();
        } catch (e) {
            console.error(`KVManagerOffPromiseInvalidArgsTest failed, error code is ${e.code}, message is ${e.message}`);
            expect(e.code == 401).assertTrue();
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
            context: context
        }
        try {
            kvManager = factory.createKVManager(config);
            expect(kvManager != null).assertTrue();
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
                    console.info("CreateKVManagerPromiseFullFuncTest getKVStore err: " + JSON.stringify(err));
                    expect(null).assertFail();
                });
        } catch (e) {
            console.error('CreateKVManagerPromiseFullFuncTest promise delete fail err' + `, error code is ${err.code}, message is ${err.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name: CreateKVManagerPromiseInvalidArgsTest
     * @tc.desc: Test Js Api createKVManager with invalid args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('CreateKVManagerPromiseInvalidArgsTest', 0, async function (done) {
        try {
            kvManager = factory.createKVManager();
            expect(null).assertFail();
        } catch (e) {
            expect(e.code == 401).assertTrue();
        }
        done();
    })

    /**
     * @tc.name: CreateKVManagerPromiseInvalidConfigTest
     * @tc.desc: Test Js Api createKVManager with invalid config
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('CreateKVManagerPromiseInvalidConfigTest', 0, async function (done) {
        try {
            const config1  = {
                bundleName : TEST_BUNDLE_NAME
            }
            kvManager = factory.createKVManager(config1);
            expect(null).assertFail();
        } catch (e) {
            console.log(e.code);
            console.log(e.message);
            expect(e.code == 401).assertTrue();
        }
        done();
    })
})
