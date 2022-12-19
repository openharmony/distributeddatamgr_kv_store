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
import abilityFeatureAbility from '@ohos.ability.featureAbility'

var context = abilityFeatureAbility.getContext();
const TEST_BUNDLE_NAME = 'com.example.myapplication';
const TEST_STORE_ID = 'storeId';
const STORE_KEY = 'key_test_string';
const STORE_VALUE = 'value-test-string';
var kvManager = null;
var kvStore = null;

describe('kvManagerCallbackTest', function () {
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
        kvManager =  factory.createKVManager(config);
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
        await kvManager.closeKVStore(TEST_BUNDLE_NAME, TEST_STORE_ID, async function () {
            console.info('afterEach closeKVStore success');
            await kvManager.deleteKVStore(TEST_BUNDLE_NAME, TEST_STORE_ID, function () {
                console.info('afterEach deleteKVStore success');
                done();
            });
        });
        kvStore = null;
    })

    /**
     * @tc.name KVManagerGetKVStoreCallbackSucTest
     * @tc.desc Test Js Api KVManager.GetKVStore() successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KVManagerGetKVStoreCallbackSucTest', 0, async function (done) {
        console.info('KVManagerGetKVStoreCallbackSucTest');
        try {
            await kvManager.getKVStore(TEST_STORE_ID, options, function (err, store) {
                console.info('KVManagerGetKVStoreCallbackSucTest getKVStore success');
                expect(true).assertTrue();
                kvStore = store;
                done();
            });
        } catch (e) {
            console.error('KVManagerGetKVStoreCallbackSucTest getKVStore e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name KVManagerGetKVStoreCallbackSucTest
     * @tc.desc Test Js Api KVManager.GetKVStore() successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KVManagerGetKVStoreCallbackSucTest1', 0, async function (done) {
        console.info('KVManagerGetKVStoreCallbackSucTest');
        try {
            await kvManager.getKVStore(TEST_STORE_ID, options, function (err, store) {
                console.info('KVManagerGetKVStoreCallbackSucTest11 getKVStore success');
                expect((err == undefined) && (store != null)).assertTrue();
            });
            const optionsInfo = {
                createIfMissing: true,
                encrypt: false,
                backup: false,
                autoSync: true,
                kvStoreType: factory.KVStoreType.DEVICE_COLLABORATION,
                schema: '',
                securityLevel: factory.SecurityLevel.S1,
            }
            await kvManager.getKVStore(TEST_STORE_ID, optionsInfo, function (err) {
                if (err == undefined) {
                    console.info('KVManagerGetKVStoreCallbackSucTest12 getKVStore success');
                    expect(null).assertFail();
                } else {
                    console.info("throw exception success");
                    expect(err.code == 15100002).assertTrue();
                }
            });
        } catch (e) {
            console.error('KVManagerGetKVStoreCallbackSucTest1 getKVStore e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(e.code == 15100002).assertTrue();
        }
        done();
    })

    /**
     * @tc.name KVManagerGetKVStoreCallbackParameterErrorTest
     * @tc.desc Test Js Api KVManager.GetKVStore() with parameter error
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KVManagerGetKVStoreCallbackParameterErrorTest', 0, async function (done) {
        console.info('KVManagerGetKVStoreCallbackParameterErrorTest');
        try {
            await kvManager.getKVStore(options, function () {
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('KVManagerGetKVStoreCallbackParameterErrorTest getKVStore e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(e.code == 401).assertTrue();
            done();
        }
    })

    /**
     * @tc.name KVManagerGetKVStoreCallbackDeviceS1SucTest
     * @tc.desc Test Js Api KVManager.GetKVStore() device S1
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KVManagerGetKVStoreCallbackDeviceS1SucTest', 0, async function (done) {
        console.info('KVManagerGetKVStoreCallbackDeviceS1SucTest');
        const optionsInfo = {
            createIfMissing: true,
            encrypt: true,
            backup: false,
            autoSync: true,
            kvStoreType: factory.KVStoreType.DEVICE_COLLABORATION,
            schema: '',
            securityLevel: factory.SecurityLevel.S1,
        }
        try {
            await kvManager.getKVStore(TEST_STORE_ID, optionsInfo, function (err, store) {
                console.info('KVManagerGetKVStoreCallbackDeviceS1SucTest getKVStore success');
                expect((err == undefined) && (store != null)).assertTrue();
                done();
            });
        } catch (e) {
            console.error('KVManagerGetKVStoreCallbackDeviceS1SucTest getKVStore e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name KVManagerGetKVStoreCallbackDeviceS2SucTest
     * @tc.desc Test Js Api KVManager.GetKVStore() device s2
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KVManagerGetKVStoreCallbackDeviceS2SucTest', 0, async function (done) {
        console.info('KVManagerGetKVStoreCallbackDeviceS2SucTest');
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
            await kvManager.getKVStore(TEST_STORE_ID, optionsInfo, function (err, store) {
                console.info('KVManagerGetKVStoreCallbackDeviceS2SucTest getKVStore success');
                expect((err == undefined) && (store != null)).assertTrue();
                done();
            });
        } catch (e) {
            console.error('KVManagerGetKVStoreCallbackDeviceS2SucTest getKVStore e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name KVManagerGetKVStoreCallbackDeviceS3SucTest
     * @tc.desc Test Js Api KVManager.GetKVStore() device s3
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KVManagerGetKVStoreCallbackDeviceS3SucTest', 0, async function (done) {
        console.info('KVManagerGetKVStoreCallbackDeviceS3SucTest');
        const optionsInfo = {
            createIfMissing: true,
            encrypt: false,
            backup: true,
            autoSync: true,
            kvStoreType: factory.KVStoreType.DEVICE_COLLABORATION,
            schema: '',
            securityLevel: factory.SecurityLevel.S3,
        }
        try {
            await kvManager.getKVStore(TEST_STORE_ID, optionsInfo, function (err, store) {
                console.info('KVManagerGetKVStoreCallbackDeviceS3SucTest getKVStore success');
                expect((err == undefined) && (store != null)).assertTrue();
                kvStore = store;
                done();
            });
        } catch (e) {
            console.error('KVManagerGetKVStoreCallbackDeviceS3SucTest getKVStore e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name KVManagerGetKVStoreCallbackDeviceS4SucTest
     * @tc.desc Test Js Api KVManager.GetKVStore() device s4
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KVManagerGetKVStoreCallbackDeviceS4SucTest', 0, async function (done) {
        console.info('KVManagerGetKVStoreCallbackDeviceS4SucTest');
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
            await kvManager.getKVStore(TEST_STORE_ID, optionsInfo, function (err, store) {
                console.info('KVManagerGetKVStoreCallbackDeviceS4SucTest getKVStore success');
                expect((err == undefined) && (store != null)).assertTrue();
                done();
            });
        } catch (e) {
            console.error('KVManagerGetKVStoreCallbackDeviceS4SucTest getKVStore e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name KVManagerGetKVStoreCallbackSingleS1SucTest
     * @tc.desc Test Js Api KVManager.GetKVStore() singleversion s1
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KVManagerGetKVStoreCallbackSingleS1SucTest', 0, async function (done) {
        console.info('KVManagerGetKVStoreCallbackSingleS1SucTest');
        const optionsInfo = {
            createIfMissing: true,
            encrypt: false,
            backup: false,
            autoSync: true,
            kvStoreType: factory.KVStoreType.SINGLE_VERSION,
            schema: '',
            securityLevel: factory.SecurityLevel.S1,
        }
        try {
            await kvManager.getKVStore(TEST_STORE_ID, optionsInfo, function (err, store) {
                console.info('KVManagerGetKVStoreCallbackSingleS1SucTest getKVStore success');
                expect((err == undefined) && (store != null)).assertTrue();
                done();
            });
        } catch (e) {
            console.error('KVManagerGetKVStoreCallbackSingleS1SucTest getKVStore e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name KVManagerGetKVStoreCallbackSingleS2SucTest
     * @tc.desc Test Js Api KVManager.GetKVStore() singleversion s2
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KVManagerGetKVStoreCallbackSingleS2SucTest', 0, async function (done) {
        console.info('KVManagerGetKVStoreCallbackSingleS2SucTest');
        const optionsInfo = {
            createIfMissing: true,
            encrypt: false,
            backup: false,
            autoSync: true,
            kvStoreType: factory.KVStoreType.SINGLE_VERSION,
            schema: '',
            securityLevel: factory.SecurityLevel.S2,
        }
        try {
            await kvManager.getKVStore(TEST_STORE_ID, optionsInfo, function (err, store) {
                console.info('KVManagerGetKVStoreCallbackSingleS2SucTest getKVStore success');
                expect((err == undefined) && (store != null)).assertTrue();
                done();
            });
        } catch (e) {
            console.error('KVManagerGetKVStoreCallbackSingleS2SucTest getKVStore e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name KVManagerGetKVStoreCallbackSingleS3SucTest
     * @tc.desc Test Js Api KVManager.GetKVStore() singleversion s3
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KVManagerGetKVStoreCallbackSingleS3SucTest', 0, async function (done) {
        console.info('KVManagerGetKVStoreCallbackSingleS3SucTest');
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
            await kvManager.getKVStore(TEST_STORE_ID, optionsInfo, function (err, store) {
                console.info('KVManagerGetKVStoreCallbackSingleS3SucTest getKVStore success');
                expect((err == undefined) && (store != null)).assertTrue();
                done();
            });
        } catch (e) {
            console.error('KVManagerGetKVStoreCallbackSingleS3SucTest getKVStore e ' + `, error code is ${e.code}, message is ${e.message}`);
            done();
        }
    })

    /**
     * @tc.name KVManagerGetKVStoreCallbackSingleS4SucTest
     * @tc.desc Test Js Api KVManager.GetKVStore() singleversion s4
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KVManagerGetKVStoreCallbackSingleS4SucTest', 0, async function (done) {
        console.info('KVManagerGetKVStoreCallbackSingleS4SucTest');
        const optionsInfo = {
            createIfMissing: true,
            encrypt: false,
            backup: false,
            autoSync: true,
            kvStoreType: factory.KVStoreType.SINGLE_VERSION,
            schema: '',
            securityLevel: factory.SecurityLevel.S4,
        }
        try {
            await kvManager.getKVStore(TEST_STORE_ID, optionsInfo, function (err, store) {
                console.info('KVManagerGetKVStoreCallbackSingleS4SucTest getKVStore success');
                expect((err == undefined) && (store != null)).assertTrue();
                done();
            });
        } catch (e) {
            console.error('KVManagerGetKVStoreCallbackSingleS4SucTest getKVStore e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })
    /**
     * @tc.name KVManagerCloseKVStoreCallbackParaErrorTest
     * @tc.desc Test Js Api KVManager.CloseKVStore() with parameter error
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KVManagerCloseKVStoreCallbackParaErrorTest', 0, async function (done) {
        console.info('KVManagerCloseKVStoreCallbackParaErrorTest');
        try {
            await kvManager.getKVStore(TEST_STORE_ID, options, async function (err, store) {
                console.info('KVManagerCloseKVStoreCallbackParaErrorTest getKVStore success');
                kvStore = store;
                try {
                    await kvManager.closeKVStore(TEST_BUNDLE_NAME, function () {
                        expect(null).assertFail();
                        done();
                    });
                } catch (e) {
                    console.error('KVManagerCloseKVStoreCallbackParaErrorTest closeKVStore e ' + `, error code is ${e.code}, message is ${e.message}`);
                    expect(e.code == 401).assertTrue();
                    done();
                }
            });
        } catch (ee) {
            console.error('KVManagerCloseKVStoreCallbackParaErrorTest getKVStore ee ' + `, error code is ${ee.code}, message is ${ee.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name KVManagerCloseKVStoreCallbackSucTest
     * @tc.desc Test Js Api KVManager.CloseKVStore() successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KVManagerCloseKVStoreCallbackSucTest', 0, async function (done) {
        console.info('KVManagerCloseKVStoreCallbackSucTest');
        try {
            await kvManager.getKVStore(TEST_STORE_ID, options, async function (error, store) {
                expect((error == undefined) && (store != null)).assertTrue();
                console.info('KVManagerCloseKVStoreCallbackSucTest getKVStore success');
                try {
                    await kvManager.closeKVStore(TEST_BUNDLE_NAME, TEST_STORE_ID, function (err) {
                        expect(err == undefined).assertTrue();
                        done();
                    });
                } catch (e) {
                    console.error('KVManagerCloseKVStoreCallbackSucTest closeKVStore e ' + `, error code is ${e.code}, message is ${e.message}`);
                    expect(null).assertFail();
                    done();
                }
            });
        } catch (ee) {
            console.error('KVManagerCloseKVStoreCallbackSucTest getKVStore ee ' + `, error code is ${ee.code}, message is ${ee.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name KVManagerCloseKVStoreCallbackCloseInvalidArgsTest
     * @tc.desc Test Js Api KVManager.CloseKVStore() with invalid args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KVManagerCloseKVStoreCallbackCloseTwiceFailTest', 0, async function (done) {
        console.info('KVManagerCloseKVStoreCallbackCloseTwiceFailTest');
        try {
            await kvManager.getKVStore(TEST_STORE_ID, options, async function (error, store) {
                expect((error == undefined) && (store != null)).assertTrue();
            });
            await kvManager.closeKVStore(TEST_BUNDLE_NAME, function () {
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('KVManagerCloseKVStoreCallbackCloseTwiceFailTest closeKVStore twice e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(e.code == 401).assertTrue();
        }
        done();
    })

    /**
     * @tc.name KVManagerDeleteKVStoreCallbackSucTest
     * @tc.desc Test Js Api KVManager.DeleteKVStore() successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KVManagerDeleteKVStoreCallbackSucTest', 0, async function (done) {
        console.info('KVManagerDeleteKVStoreCallbackSucTest');
        try {
            await kvManager.getKVStore(TEST_STORE_ID, options, async function (error, store) {
                expect((error == undefined) && (store != null)).assertTrue();
                await kvManager.deleteKVStore(TEST_BUNDLE_NAME, TEST_STORE_ID, function (err) {
                    console.info('KVManagerDeleteKVStoreCallbackSucTest deleteKVStore success');
                    expect(err == undefined).assertTrue();
                    done();
                });
            });
        } catch (e) {
            console.error('KVManagerDeleteKVStoreCallbackSucTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name KVManagerDeleteKVStoreCallbackParaErrorTest
     * @tc.desc Test Js Api KVManager.DeleteKVStore() with parameter error
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KVManagerDeleteKVStoreCallbackParaErrorTest', 0, async function (done) {
        console.info('KVManagerDeleteKVStoreCallbackParaErrorTest');
        try {
            await kvManager.getKVStore(TEST_STORE_ID, options, async function (error, store) {
                expect((error == undefined) && (store != null)).assertTrue();
                console.info('KVManagerDeleteKVStoreCallbackParaErrorTest getKVStore success');
                try {
                    await kvManager.deleteKVStore(TEST_BUNDLE_NAME, function () {
                        expect(null).assertFail();
                        done();
                    });
                } catch (e) {
                    console.error('KVManagerDeleteKVStoreCallbackParaErrorTest deleteKVStore e ' + `, error code is ${e.code}, message is ${e.message}`);
                    expect(e.code == 401).assertTrue();
                    done();
                }
            });
        } catch (ee) {
            console.error('KVManagerDeleteKVStoreCallbackParaErrorTest getKVStore ee ' + `, error code is ${ee.code}, message is ${ee.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name KVManagerDeleteKVStoreCallbackNotGetTest
     * @tc.desc Test Js Api KVManager.DeleteKVStore() without get kvstore
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KVManagerDeleteKVStoreCallbackNotGetTest', 0, async function (done) {
        console.info('KVManagerDeleteKVStoreCallbackNotGetTest');
        try {
            await kvManager.deleteKVStore(TEST_BUNDLE_NAME, TEST_STORE_ID, function (err) {
                if (err == undefined) {
                    console.info('KVManagerDeleteKVStoreCallbackNotGetTest deleteKVStore success');
                    expect(null).assertFail();
                } else {
                    console.info('KVManagerDeleteKVStoreCallbackNotGetTest deleteKVStore fail');
                    expect(err.code == 15100004).assertTrue();
                }
                done();
            });
        } catch (e) {
            console.error('KVManagerDeleteKVStoreCallbackNotGetTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            done();
        }
    })

    /**
     * @tc.name KVManagerGetAllKVStoreIdCallbackSucTest
     * @tc.desc Test Js Api KVManager.GetAllKVStoreId() success
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KVManagerGetAllKVStoreIdCallbackSucTest', 0, async function (done) {
        console.info('KVManagerGetAllKVStoreIdCallbackEqual0Test');
        try {
            kvManager.getAllKVStoreId(TEST_BUNDLE_NAME, function (err, data) {
                expect(data.length == 0).assertTrue();
                done();
            });
        } catch (e) {
            console.error('KVManagerGetAllKVStoreIdCallbackEqual0Test e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name KVManagerGetAllKVStoreIdCallbackInvalidArgsTest
     * @tc.desc Test Js Api KVManager.GetAllKVStoreId() with invalid args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KVManagerGetAllKVStoreIdCallbackInvalidArgsTest', 0, async function (done) {
        console.info('KVManagerGetAllKVStoreIdCallbackInvalidArgsTest');
        try {
            await kvManager.getAllKVStoreId(function (err, storeIds) {
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('KVManagerGetAllKVStoreIdCallbackInvalidArgsTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(e.code == 401).assertTrue();
        }
        done();
    })

    /**
     * @tc.name KVStorePutCallbackTest
     * @tc.desc Test Js Api KVStore.Put()
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KVStorePutCallbackTest', 0, async function (done) {
        console.info('KVStorePutCallbackTest');
        try {
            await kvManager.getKVStore(TEST_STORE_ID, options, async function (err, store) {
                console.info('KVStorePutCallbackTest getKVStore success');
                kvStore = store;
                await kvStore.put(STORE_KEY, STORE_VALUE, function (err) {
                    expect(true).assertTrue();
                    done();
                });
            });

        } catch (e) {
            console.error('KVStorePutCallbackTest callback e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })
})
