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

const TEST_BUNDLE_NAME = 'com.example.myapplication';
const TEST_STORE_ID = 'storeId';
const STORE_KEY = 'key_test_string';
const STORE_VALUE = 'value-test-string';
var kvManager = null;
var kvStore = null;

describe('KvManagerCallbackTest', function () {
    const config = {
        bundleName : TEST_BUNDLE_NAME,
        userInfo : {
            userId : '0',
            userType : factory.UserType.SAME_USER_ID
        }
    }

    const options = {
        createIfMissing : true,
        encrypt : false,
        backup : false,
        autoSync : true,
        kvStoreType : factory.KVStoreType.SINGLE_VERSION,
        schema : '',
        securityLevel : factory.SecurityLevel.S2,
    }

    beforeAll(async function (done) {
        console.info('beforeAll');
        await factory.createKVManager(config, function (err, manager) {
            kvManager = manager;
            done();
        });
        console.info('beforeAll end');
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
        await kvManager.closeKVStore(TEST_BUNDLE_NAME, TEST_STORE_ID, kvStore, async function () {
            console.info('afterEach closeKVStore success');
            await kvManager.deleteKVStore(TEST_BUNDLE_NAME, TEST_STORE_ID, function () {
                console.info('afterEach deleteKVStore success');
                done();
            });
        });
        kvStore = null;
    })

    /**
     * @tc.name KVManagerGetKVStoreCallbackTest001
     * @tc.desc Test Js Api KVManager.GetKVStore() testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KVManagerGetKVStoreCallbackTest001', 0, async function (done) {
        console.info('KVManagerGetKVStoreCallbackTest001');
        try {
            await kvManager.getKVStore(TEST_STORE_ID, options, function (err, store) {
                console.info('KVManagerGetKVStoreCallbackTest001 getKVStore success');
                kvStore = store;
                done();
            });
        } catch (e) {
            console.error('KVManagerGetKVStoreCallbackTest001 getKVStore e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name KVManagerGetKVStoreCallbackTest002
     * @tc.desc Test Js Api KVManager.GetKVStore() testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KVManagerGetKVStoreCallbackTest002', 0, async function (done) {
        console.info('KVManagerGetKVStoreCallbackTest002');
        try {
            await kvManager.getKVStore(options, function (err, store) {
                if (err == undefined) {
                    console.info('KVManagerGetKVStoreCallbackTest002 getKVStore success');
                    expect(null).assertFail();
                } else {
                    console.info('KVManagerGetKVStoreCallbackTest002 getKVStore fail');
                }
                done();
            });
        } catch (e) {
            console.error('KVManagerGetKVStoreCallbackTest002 getKVStore e ' + `, error code is ${e.code}, message is ${e.message}`);
            done();
        }
    })

    /**
     * @tc.name KVManagerGetKVStoreCallbackTest003
     * @tc.desc Test Js Api KVManager.GetKVStore() testcase 003
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KVManagerGetKVStoreCallbackTest003', 0, async function (done) {
        console.info('KVManagerGetKVStoreCallbackTest003');
        const optionsInfo = {
            createIfMissing : true,
            encrypt : false,
            backup : false,
            autoSync : true,
            kvStoreType : factory.KVStoreType.DEVICE_COLLABORATION,
            schema : '',
            securityLevel : factory.SecurityLevel.NO_LEVEL,
        }
        try {
            await kvManager.getKVStore(TEST_STORE_ID, optionsInfo, function (err, store) {
                console.info('KVManagerGetKVStoreCallbackTest003 getKVStore success');
                console.info('KVManagerGetKVStoreCallbackTest003 err ' + err + ' store ' + store);
                expect((err == undefined) && (store != null)).assertTrue();
                kvStore = store;
                done();
            });
        } catch (e) {
            console.error('KVManagerGetKVStoreCallbackTest003 getKVStore e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name KVManagerGetKVStoreCallbackTest004
     * @tc.desc Test Js Api KVManager.GetKVStore() testcase 004
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KVManagerGetKVStoreCallbackTest004', 0, async function (done) {
        console.info('KVManagerGetKVStoreCallbackTest004');
        const optionsInfo = {
            createIfMissing : false,
            encrypt : false,
            backup : false,
            autoSync : true,
            kvStoreType : factory.KVStoreType.DEVICE_COLLABORATION,
            schema : '',
            securityLevel : factory.SecurityLevel.S0,
        }
        try {
            await kvManager.getKVStore(TEST_STORE_ID, optionsInfo, function (err, store) {
                if (err == undefined) {
                    console.info('KVManagerGetKVStoreCallbackTest004 getKVStore success');
                    expect(null).assertFail();
                } else {
                    console.info('KVManagerGetKVStoreCallbackTest004 getKVStore fail');
                }
                done();
            });
        } catch (e) {
            console.error('KVManagerGetKVStoreCallbackTest004 getKVStore e ' + `, error code is ${e.code}, message is ${e.message}`);
            done();
        }
    })

    /**
     * @tc.name KVManagerGetKVStoreCallbackTest005
     * @tc.desc Test Js Api KVManager.GetKVStore() testcase 005
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KVManagerGetKVStoreCallbackTest005', 0, async function (done) {
        console.info('KVManagerGetKVStoreCallbackTest005');
        const optionsInfo = {
            createIfMissing : true,
            encrypt : true,
            backup : false,
            autoSync : true,
            kvStoreType : factory.KVStoreType.DEVICE_COLLABORATION,
            schema : '',
            securityLevel : factory.SecurityLevel.S1,
        }
        try {
            await kvManager.getKVStore(TEST_STORE_ID, optionsInfo, function (err, store) {
                console.info('KVManagerGetKVStoreCallbackTest005 getKVStore success');
                expect((err == undefined) && (store != null)).assertTrue();
                kvStore = store;
                done();
            });
        } catch (e) {
            console.error('KVManagerGetKVStoreCallbackTest005 getKVStore e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name KVManagerGetKVStoreCallbackTest006
     * @tc.desc Test Js Api KVManager.GetKVStore() testcase 006
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KVManagerGetKVStoreCallbackTest006', 0, async function (done) {
        console.info('KVManagerGetKVStoreCallbackTest006');
        const optionsInfo = {
            createIfMissing : true,
            encrypt : false,
            backup : false,
            autoSync : true,
            kvStoreType : factory.KVStoreType.DEVICE_COLLABORATION,
            schema : '',
            securityLevel : factory.SecurityLevel.S2,
        }
        try {
            await kvManager.getKVStore(TEST_STORE_ID, optionsInfo, function (err, store) {
                console.info('KVManagerGetKVStoreCallbackTest006 getKVStore success');
                expect((err == undefined) && (store != null)).assertTrue();
                kvStore = store;
                done();
            });
        } catch (e) {
            console.error('KVManagerGetKVStoreCallbackTest006 getKVStore e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name KVManagerGetKVStoreCallbackTest007
     * @tc.desc Test Js Api KVManager.GetKVStore() testcase 007
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KVManagerGetKVStoreCallbackTest007', 0, async function (done) {
        console.info('KVManagerGetKVStoreCallbackTest007');
        const optionsInfo = {
            createIfMissing : true,
            encrypt : false,
            backup : true,
            autoSync : true,
            kvStoreType : factory.KVStoreType.DEVICE_COLLABORATION,
            schema : '',
            securityLevel : factory.SecurityLevel.S3,
        }
        try {
            await kvManager.getKVStore(TEST_STORE_ID, optionsInfo, function (err, store) {
                console.info('KVManagerGetKVStoreCallbackTest007 getKVStore success');
                expect((err == undefined) && (store != null)).assertTrue();
                kvStore = store;
                done();
            });
        } catch (e) {
            console.error('KVManagerGetKVStoreCallbackTest007 getKVStore e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name KVManagerGetKVStoreCallbackTest008
     * @tc.desc Test Js Api KVManager.GetKVStore() testcase 008
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KVManagerGetKVStoreCallbackTest008', 0, async function (done) {
        console.info('KVManagerGetKVStoreCallbackTest008');
        const optionsInfo = {
            createIfMissing : true,
            encrypt : false,
            backup : false,
            autoSync : true,
            kvStoreType : factory.KVStoreType.DEVICE_COLLABORATION,
            schema : '',
            securityLevel : factory.SecurityLevel.S4,
        }
        try {
            await kvManager.getKVStore(TEST_STORE_ID, optionsInfo, function (err, store) {
                console.info('KVManagerGetKVStoreCallbackTest008 getKVStore success');
                expect((err == undefined) && (store != null)).assertTrue();
                kvStore = store;
                done();
            });
        } catch (e) {
            console.error('KVManagerGetKVStoreCallbackTest008 getKVStore e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name KVManagerGetKVStoreCallbackTest009
     * @tc.desc Test Js Api KVManager.GetKVStore() testcase 009
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KVManagerGetKVStoreCallbackTest009', 0, async function (done) {
        console.info('KVManagerGetKVStoreCallbackTest009');
        const optionsInfo = {
            createIfMissing : true,
            encrypt : false,
            backup : false,
            autoSync : true,
            kvStoreType : factory.KVStoreType.SINGLE_VERSION,
            schema : '',
            securityLevel : factory.SecurityLevel.NO_LEVEL,
        }
        try {
            await kvManager.getKVStore(TEST_STORE_ID, optionsInfo, function (err, store) {
                console.info('KVManagerGetKVStoreCallbackTest009 getKVStore success');
                expect((err == undefined) && (store != null)).assertTrue();
                kvStore = store;
                done();
            });
        } catch (e) {
            console.error('KVManagerGetKVStoreCallbackTest009 getKVStore e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name KVManagerGetKVStoreCallbackTest010
     * @tc.desc Test Js Api KVManager.GetKVStore() testcase 010
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KVManagerGetKVStoreCallbackTest010', 0, async function (done) {
        console.info('KVManagerGetKVStoreCallbackTest010');
        const optionsInfo = {
            createIfMissing : true,
            encrypt : false,
            backup : false,
            autoSync : false,
            kvStoreType : factory.KVStoreType.SINGLE_VERSION,
            schema : '',
            securityLevel : factory.SecurityLevel.S0,
        }
        try {
            await kvManager.getKVStore(TEST_STORE_ID, optionsInfo, function (err, store) {
                console.info('KVManagerGetKVStoreCallbackTest010 getKVStore success');
                expect((err == undefined) && (store != null)).assertTrue();
                kvStore = store;
                done();
            });
        } catch (e) {
            console.error('KVManagerGetKVStoreCallbackTest010 getKVStore e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name KVManagerGetKVStoreCallbackTest011
     * @tc.desc Test Js Api KVManager.GetKVStore() testcase 011
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KVManagerGetKVStoreCallbackTest011', 0, async function (done) {
        console.info('KVManagerGetKVStoreCallbackTest011');
        const optionsInfo = {
            createIfMissing : true,
            encrypt : false,
            backup : false,
            autoSync : true,
            kvStoreType : factory.KVStoreType.SINGLE_VERSION,
            schema : '',
            securityLevel : factory.SecurityLevel.S1,
        }
        try {
            await kvManager.getKVStore(TEST_STORE_ID, optionsInfo, function (err, store) {
                console.info('KVManagerGetKVStoreCallbackTest011 getKVStore success');
                expect((err == undefined) && (store != null)).assertTrue();
                kvStore = store;
                done();
            });
        } catch (e) {
            console.error('KVManagerGetKVStoreCallbackTest011 getKVStore e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name KVManagerGetKVStoreCallbackTest012
     * @tc.desc Test Js Api KVManager.GetKVStore() testcase 012
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KVManagerGetKVStoreCallbackTest012', 0, async function (done) {
        console.info('KVManagerGetKVStoreCallbackTest012');
        const optionsInfo = {
            createIfMissing : true,
            encrypt : false,
            backup : false,
            autoSync : true,
            kvStoreType : factory.KVStoreType.SINGLE_VERSION,
            schema : '',
            securityLevel : factory.SecurityLevel.S2,
        }
        try {
            await kvManager.getKVStore(TEST_STORE_ID, optionsInfo, function (err, store) {
                console.info('KVManagerGetKVStoreCallbackTest012 getKVStore success');
                expect((err == undefined) && (store != null)).assertTrue();
                kvStore = store;
                done();
            });
        } catch (e) {
            console.error('KVManagerGetKVStoreCallbackTest012 getKVStore e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name KVManagerGetKVStoreCallbackTest013
     * @tc.desc Test Js Api KVManager.GetKVStore() testcase 013
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KVManagerGetKVStoreCallbackTest013', 0, async function (done) {
        console.info('KVManagerGetKVStoreCallbackTest013');
        const optionsInfo = {
            createIfMissing : true,
            encrypt : false,
            backup : false,
            autoSync : true,
            kvStoreType : factory.KVStoreType.SINGLE_VERSION,
            schema : '',
            securityLevel : factory.SecurityLevel.S3,
        }
        try {
            await kvManager.getKVStore(TEST_STORE_ID, optionsInfo, function (err, store) {
                if (err == undefined) {
                    console.info('KVManagerGetKVStoreCallbackTest013 getKVStore success');
                    expect((err == undefined) && (store != null)).assertTrue();
                } else {
                    console.info('KVManagerGetKVStoreCallbackTest013 getKVStore fail');
                    expect(null).assertFail();
                }
                done();
            });
        } catch (e) {
            console.error('KVManagerGetKVStoreCallbackTest013 getKVStore e ' + `, error code is ${e.code}, message is ${e.message}`);
            done();
        }
    })

    /**
     * @tc.name KVManagerGetKVStoreCallbackTest014
     * @tc.desc Test Js Api KVManager.GetKVStore() testcase 014
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KVManagerGetKVStoreCallbackTest014', 0, async function (done) {
        console.info('KVManagerGetKVStoreCallbackTest014');
        const optionsInfo = {
            createIfMissing : true,
            encrypt : false,
            backup : false,
            autoSync : true,
            kvStoreType : factory.KVStoreType.SINGLE_VERSION,
            schema : '',
            securityLevel : factory.SecurityLevel.S4,
        }
        try {
            await kvManager.getKVStore(TEST_STORE_ID, optionsInfo, function (err, store) {
                console.info('KVManagerGetKVStoreCallbackTest014 getKVStore success');
                expect((err == undefined) && (store != null)).assertTrue();
                kvStore = store;
                done();
            });
        } catch (e) {
            console.error('KVManagerGetKVStoreCallbackTest014 getKVStore e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name KVManagerGetKVStoreCallbackTest015
     * @tc.desc Test Js Api KVManager.GetKVStore() testcase 015
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KVManagerGetKVStoreCallbackTest015', 0, async function (done) {
        console.info('KVManagerGetKVStoreCallbackTest015');
        const optionsInfo = {
            createIfMissing : true,
            encrypt : false,
            backup : false,
            autoSync : true,
            kvStoreType : factory.KVStoreType.MULTI_VERSION,
            schema : '',
            securityLevel : factory.SecurityLevel.NO_LEVEL,
        }
        try {
            await kvManager.getKVStore(TEST_STORE_ID, optionsInfo, function (err, store) {
                console.info('KVManagerGetKVStoreCallbackTest015 getKVStore success');
                expect(true).assertTrue();
                done();
            });
        } catch (e) {
            console.error('KVManagerGetKVStoreCallbackTest015 getKVStore e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name KVManagerGetKVStoreCallbackTest016
     * @tc.desc Test Js Api KVManager.GetKVStore() testcase 016
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KVManagerGetKVStoreCallbackTest016', 0, async function (done) {
        console.info('KVManagerGetKVStoreCallbackTest016');
        const optionsInfo = {
            createIfMissing : true,
            encrypt : false,
            backup : false,
            autoSync : true,
            kvStoreType : factory.KVStoreType.MULTI_VERSION,
            schema : '',
            securityLevel : factory.SecurityLevel.S0,
        }
        try {
            await kvManager.getKVStore(TEST_STORE_ID, optionsInfo, function (err, store) {
                console.info('KVManagerGetKVStoreCallbackTest016 getKVStore success');
                expect(true).assertTrue();
                done();
            });
        } catch (e) {
            console.error('KVManagerGetKVStoreCallbackTest016 getKVStore e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name KVManagerGetKVStoreCallbackTest017
     * @tc.desc Test Js Api KVManager.GetKVStore() testcase 017
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KVManagerGetKVStoreCallbackTest017', 0, async function (done) {
        console.info('KVManagerGetKVStoreCallbackTest017');
        const optionsInfo = {
            createIfMissing : true,
            encrypt : false,
            backup : false,
            autoSync : true,
            kvStoreType : factory.KVStoreType.MULTI_VERSION,
            schema : '',
            securityLevel : factory.SecurityLevel.S1,
        }
        try {
            await kvManager.getKVStore(TEST_STORE_ID, optionsInfo, function (err, store) {
                console.info('KVManagerGetKVStoreCallbackTest017 getKVStore success');
                expect(true).assertTrue();
                done();
            });
        } catch (e) {
            console.error('KVManagerGetKVStoreCallbackTest017 getKVStore e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name KVManagerGetKVStoreCallbackTest018
     * @tc.desc Test Js Api KVManager.GetKVStore() testcase 018
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KVManagerGetKVStoreCallbackTest018', 0, async function (done) {
        console.info('KVManagerGetKVStoreCallbackTest018');
        const optionsInfo = {
            createIfMissing : true,
            encrypt : false,
            backup : false,
            autoSync : true,
            kvStoreType : factory.KVStoreType.MULTI_VERSION,
            schema : '',
            securityLevel : factory.SecurityLevel.S2,
        }
        try {
            await kvManager.getKVStore(TEST_STORE_ID, optionsInfo, function (err, store) {
                console.info('KVManagerGetKVStoreCallbackTest018 getKVStore success');
                expect(true).assertTrue();
                done();
            });
        } catch (e) {
            console.error('KVManagerGetKVStoreCallbackTest018 getKVStore e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name KVManagerGetKVStoreCallbackTest019
     * @tc.desc Test Js Api KVManager.GetKVStore() testcase 019
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KVManagerGetKVStoreCallbackTest019', 0, async function (done) {
        console.info('KVManagerGetKVStoreCallbackTest019');
        const optionsInfo = {
            createIfMissing : true,
            encrypt : false,
            backup : false,
            autoSync : true,
            kvStoreType : factory.KVStoreType.MULTI_VERSION,
            schema : '',
            securityLevel : factory.SecurityLevel.S3,
        }
        try {
            await kvManager.getKVStore(TEST_STORE_ID, optionsInfo, function (err, store) {
                console.info('KVManagerGetKVStoreCallbackTest019 getKVStore success');
                expect(true).assertTrue();
                done();
            });
        } catch (e) {
            console.error('KVManagerGetKVStoreCallbackTest019 getKVStore e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name KVManagerGetKVStoreCallbackTest020
     * @tc.desc Test Js Api KVManager.GetKVStore() testcase 020
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KVManagerGetKVStoreCallbackTest020', 0, async function (done) {
        console.info('KVManagerGetKVStoreCallbackTest020');
        const optionsInfo = {
            createIfMissing : true,
            encrypt : false,
            backup : false,
            autoSync : true,
            kvStoreType : factory.KVStoreType.MULTI_VERSION,
            schema : '',
            securityLevel : factory.SecurityLevel.S4,
        }
        try {
            await kvManager.getKVStore(TEST_STORE_ID, optionsInfo, function (err, store) {
                console.info('KVManagerGetKVStoreCallbackTest020 getKVStore success');
                expect(true).assertTrue();
                done();
            });
        } catch (e) {
            console.error('KVManagerGetKVStoreCallbackTest019 getKVStore e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name KVManagerCloseKVStoreCallbackTest001
     * @tc.desc Test Js Api KVManager.CloseKVStore() testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KVManagerCloseKVStoreCallbackTest001', 0, async function (done) {
        console.info('KVManagerCloseKVStoreCallbackTest001');
        try {
            await kvManager.getKVStore(TEST_STORE_ID, options, async function (err, store) {
                console.info('KVManagerCloseKVStoreCallbackTest001 getKVStore success');
                kvStore = store;
                await kvManager.closeKVStore(TEST_BUNDLE_NAME, TEST_STORE_ID, kvStore, function (err, data) {
                    console.info('KVManagerCloseKVStoreCallbackTest001 closeKVStore success');
                    expect((err == undefined) && (data == undefined)).assertTrue();
                    done();
                });
            });
        } catch (e) {
            console.error('KVManagerCloseKVStoreCallbackTest001 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name KVManagerCloseKVStoreCallbackTest002
     * @tc.desc Test Js Api KVManager.CloseKVStore() testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KVManagerCloseKVStoreCallbackTest002', 0, async function (done) {
        console.info('KVManagerCloseKVStoreCallbackTest002');
        try {
            await kvManager.getKVStore(TEST_STORE_ID, options, async function (err, store) {
                console.info('KVManagerCloseKVStoreCallbackTest002 getKVStore success');
                kvStore = store;
                try {
                    await kvManager.closeKVStore(TEST_BUNDLE_NAME, function (err, data) {
                        if (err == undefined) {
                            console.info('KVManagerCloseKVStoreCallbackTest002 closeKVStore success');
                            expect(null).assertFail();
                        } else {
                            console.info('KVManagerCloseKVStoreCallbackTest002 closeKVStore fail');
                        }
                        done();
                    });
                } catch (e) {
                    console.error('KVManagerCloseKVStoreCallbackTest002 closeKVStore e ' + `, error code is ${e.code}, message is ${e.message}`);
                    done();
                }
            });
        } catch (ee) {
            console.error('KVManagerCloseKVStoreCallbackTest002 getKVStore ee ' + `, error code is ${ee.code}, message is ${ee.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name KVManagerCloseKVStoreCallbackTest003
     * @tc.desc Test Js Api KVManager.CloseKVStore() testcase 003
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KVManagerCloseKVStoreCallbackTest003', 0, async function (done) {
        console.info('KVManagerCloseKVStoreCallbackTest003');
        try {
            await kvManager.getKVStore(TEST_STORE_ID, options, async function (err, store) {
                console.info('KVManagerCloseKVStoreCallbackTest003 getKVStore success');
                kvStore = store;
                try {
                    await kvManager.closeKVStore(TEST_BUNDLE_NAME, TEST_STORE_ID, function (err, data) {
                        if (err == undefined) {
                            console.info('KVManagerCloseKVStoreCallbackTest003 closeKVStore success');
                            expect(null).assertFail();
                        } else {
                            console.info('KVManagerCloseKVStoreCallbackTest003 closeKVStore fail');
                        }
                        done();
                    });
                } catch (e) {
                    console.error('KVManagerCloseKVStoreCallbackTest003 closeKVStore e ' + `, error code is ${e.code}, message is ${e.message}`);
                    done();
                }
            });
        } catch (ee) {
            console.error('KVManagerCloseKVStoreCallbackTest003 getKVStore ee ' + `, error code is ${ee.code}, message is ${ee.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name KVManagerCloseKVStoreCallbackTest004
     * @tc.desc Test Js Api KVManager.CloseKVStore() testcase 004
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KVManagerCloseKVStoreCallbackTest004', 0, async function (done) {
        console.info('KVManagerCloseKVStoreCallbackTest004');
        try {
            await kvManager.getKVStore(TEST_STORE_ID, options, async function (err, store) {
                console.info('KVManagerCloseKVStoreCallbackTest004 getKVStore success');
                kvStore = store;
                await kvManager.closeKVStore(TEST_BUNDLE_NAME, TEST_STORE_ID, kvStore);
            });
            console.info('KVManagerCloseKVStoreCallbackTest004 closeKVStore redo.');
            await kvManager.closeKVStore(TEST_BUNDLE_NAME, TEST_STORE_ID, kvStore, function (err, data) {
                console.error('KVManagerCloseKVStoreCallbackTest004 closeKVStore twice ' + `, error code is ${err.code}, message is ${err.message}`);
                if (err == undefined) {
                    expect(null).assertFail();
                }
            });
        } catch (e) {
            console.error('KVManagerCloseKVStoreCallbackTest004 closeKVStore twice e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name KVManagerDeleteKVStoreCallbackTest001
     * @tc.desc Test Js Api KVManager.DeleteKVStore() testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KVManagerDeleteKVStoreCallbackTest001', 0, async function (done) {
        console.info('KVManagerDeleteKVStoreCallbackTest001');
        try {
            await kvManager.getKVStore(TEST_STORE_ID, options, async function (err, store) {
                console.info('KVManagerDeleteKVStoreCallbackTest001 getKVStore success');
                kvStore = store;
                await kvManager.deleteKVStore(TEST_BUNDLE_NAME, TEST_STORE_ID, function (err, data) {
                    console.info('KVManagerDeleteKVStoreCallbackTest001 deleteKVStore success');
                    expect((err == undefined) && (data == undefined)).assertTrue();
                    done();
                });
            });
        } catch (e) {
            console.error('KVManagerDeleteKVStoreCallbackTest001 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name KVManagerDeleteKVStoreCallbackTest002
     * @tc.desc Test Js Api KVManager.DeleteKVStore() testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KVManagerDeleteKVStoreCallbackTest002', 0, async function (done) {
        console.info('KVManagerDeleteKVStoreCallbackTest002');
        try {
            await kvManager.getKVStore(TEST_STORE_ID, options, async function (err, store) {
                console.info('KVManagerDeleteKVStoreCallbackTest002 getKVStore success');
                kvStore = store;
                try {
                    await kvManager.deleteKVStore(TEST_BUNDLE_NAME, function (err, data) {
                        if (err == undefined) {
                            console.info('KVManagerDeleteKVStoreCallbackTest002 deleteKVStore success');
                            expect(null).assertFail();
                        } else {
                            console.info('KVManagerDeleteKVStoreCallbackTest002 deleteKVStore fail');
                        }
                        done();
                    });
                } catch (e) {
                    console.error('KVManagerDeleteKVStoreCallbackTest002 deleteKVStore e ' + `, error code is ${e.code}, message is ${e.message}`);
                    done();
                }
            });
        } catch (ee) {
            console.error('KVManagerDeleteKVStoreCallbackTest002 getKVStore ee ' + `, error code is ${ee.code}, message is ${ee.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name KVManagerDeleteKVStoreCallbackTest003
     * @tc.desc Test Js Api KVManager.DeleteKVStore() testcase 003
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KVManagerDeleteKVStoreCallbackTest003', 0, async function (done) {
        console.info('KVManagerDeleteKVStoreCallbackTest003');
        try {
            await kvManager.deleteKVStore(TEST_BUNDLE_NAME, TEST_STORE_ID, function (err, data) {
                if (err == undefined){
                    console.info('KVManagerDeleteKVStoreCallbackTest003 deleteKVStore success');
                    expect(null).assertFail();
                } else {
                    console.info('KVManagerDeleteKVStoreCallbackTest003 deleteKVStore fail');
                }
                done();
            });
        } catch (e) {
            console.error('KVManagerDeleteKVStoreCallbackTest003 e ' + `, error code is ${e.code}, message is ${e.message}`);
            done();
        }
    })

    /**
     * @tc.name KVManagerGetAllKVStoreIdCallbackTest001
     * @tc.desc Test Js Api KVManager.GetAllKVStoreId() testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KVManagerGetAllKVStoreIdCallbackTest001', 0, async function (done) {
        console.info('KVManagerGetAllKVStoreIdCallbackTest001');
        try {
            await kvManager.getAllKVStoreId(TEST_BUNDLE_NAME, function (err, data) {
                console.info('KVManagerGetAllKVStoreIdCallbackTest001 getAllKVStoreId success');
                console.info('KVManagerGetAllKVStoreIdCallbackTest001 size = ' + data.length);
                expect(0).assertEqual(data.length);
                done();
            });
        } catch (e) {
            console.error('KVManagerGetAllKVStoreIdCallbackTest001 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name KVManagerGetAllKVStoreIdCallbackTest002
     * @tc.desc Test Js Api KVManager.GetAllKVStoreId() testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KVManagerGetAllKVStoreIdCallbackTest002', 0, async function (done) {
        console.info('KVManagerGetAllKVStoreIdCallbackTest002');
        try {
            await kvManager.getKVStore(TEST_STORE_ID, options, async function (err, store) {
                console.info('KVManagerGetAllKVStoreIdCallbackTest002 getKVStore success');
                kvStore = store;
                try {
                    await kvManager.getAllKVStoreId(TEST_BUNDLE_NAME, function (err, data) {
                        console.info('testKVManagerGetAllKVStoreId002 getKVStore success');
                        console.info('testKVManagerGetAllKVStoreId002 getkvmanager success: ' + kvManager)
                        console.info('testKVManagerGetAllKVStoreId002 get kvstore: ' + store)
                        console.info('KVManagerGetAllKVStoreIdCallbackTest002 getAllKVStoreId success');
                        console.info('KVManagerGetAllKVStoreIdCallbackTest002 size = ' + data.length);
                        expect(1).assertEqual(data.length);
                        console.info('KVManagerGetAllKVStoreIdCallbackTest002 data[0] = ' + data[0]);
                        expect(TEST_STORE_ID).assertEqual(data[0]);
                        done();
                    });
                } catch (e) {
                    console.error('KVManagerGetAllKVStoreIdCallbackTest002 getAllKVStoreId e ' + `, error code is ${e.code}, message is ${e.message}`);
                    expect(null).assertFail();
                    done();
                }
            });
        } catch (ee) {
            console.error('KVManagerGetAllKVStoreIdCallbackTest002 getKVStore ee ' + `, error code is ${ee.code}, message is ${ee.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name KvStorePutTest001
     * @tc.desc Test Js Api KVManager.Put() testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStorePutTest001', 0, async function (done) {
        console.info('KvStorePutTest001');
        try {
            await kvManager.getKVStore(TEST_STORE_ID, options, async function (err, store) {
                console.info('KvStorePutTest001 getKVStore success');
                kvStore = store;
                    await kvStore.put(STORE_KEY, STORE_VALUE, function (err, data) {
                        if (err != undefined){
                            console.info('KvStorePutTest001 put callback fail');
                            expect(null).assertFail();
                        } else {
                            console.info('KvStorePutTest001 put callback success');
                            expect(true).assertTrue();
                        }
                        done();
                    });
                });

        } catch (e) {
            console.error('KvStorePutTest001 callback e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })
})
