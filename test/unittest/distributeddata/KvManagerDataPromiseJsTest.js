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
import factory from '@ohos.data.distributedData';
import abilityFeatureAbility from '@ohos.ability.featureAbility';

var context = abilityFeatureAbility.getContext();
var contextApplication = context.getApplicationContext()

const TEST_BUNDLE_NAME = 'com.example.myapplication';
const TEST_STORE_ID = 'storeId';
var kvManager = null;
var kvStore = null;
const STORE_KEY = 'key_test_string';
const STORE_VALUE = 'value-test-string';

describe('KvManagerPromiseTest', function () {
    const config = {
        bundleName: TEST_BUNDLE_NAME,
        userInfo: {
            userId: '0',
            userType: factory.UserType.SAME_USER_ID
        }
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
     * @tc.desc Test Js Api KVManager.GetKVStore() testcase 003
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KVManagerGetKVStorePromiseTest003', 0, async function (done) {
        console.info('KVManagerGetKVStorePromiseTest003');
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
            console.info('KVManagerGetKVStorePromiseTest003 getKVStore success');
            kvStore = store;
            expect(store != null).assertTrue();
        }).catch((err) => {
            console.error('KVManagerGetKVStorePromiseTest003 getKVStore err ' + `, error code is ${err.code}, message is ${err.message}`);
            expect(null).assertFail();
        });
        done();
    })

    /**
     * @tc.name KVManagerGetKVStorePromiseTest004
     * @tc.desc Test Js Api KVManager.GetKVStore() testcase 004
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KVManagerGetKVStorePromiseTest004', 0, async function (done) {
        console.info('KVManagerGetKVStorePromiseTest004');
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
            console.info('KVManagerGetKVStorePromiseTest004 getKVStore success');
            expect(null).assertFail();
        }).catch((err) => {
            console.error('KVManagerGetKVStorePromiseTest004 getKVStore err ' + `, error code is ${err.code}, message is ${err.message}`);
        });
        done();
    })

    /**
     * @tc.name KVManagerGetKVStorePromiseTest005
     * @tc.desc Test Js Api KVManager.GetKVStore() testcase 005
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KVManagerGetKVStorePromiseTest005', 0, async function (done) {
        console.info('KVManagerGetKVStorePromiseTest005');
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
            console.info('KVManagerGetKVStorePromiseTest005 getKVStore success');
            kvStore = store;
            expect(store != null).assertTrue();
        }).catch((err) => {
            console.error('KVManagerGetKVStorePromiseTest005 getKVStore err ' + `, error code is ${err.code}, message is ${err.message}`);
            expect(null).assertFail();
        });
        done();
    })

    /**
     * @tc.name KVManagerGetKVStorePromiseTest006
     * @tc.desc Test Js Api KVManager.GetKVStore() testcase 006
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KVManagerGetKVStorePromiseTest006', 0, async function (done) {
        console.info('KVManagerGetKVStorePromiseTest006');
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
            console.info('KVManagerGetKVStorePromiseTest006 getKVStore success');
            kvStore = store;
            expect(store != null).assertTrue();
        }).catch((err) => {
            console.error('KVManagerGetKVStorePromiseTest006 getKVStore err ' + `, error code is ${err.code}, message is ${err.message}`);
            expect(null).assertFail();
        });
        done();
    })

    /**
     * @tc.name KVManagerGetKVStorePromiseTest007
     * @tc.desc Test Js Api KVManager.GetKVStore() testcase 007
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KVManagerGetKVStorePromiseTest007', 0, async function (done) {
        console.info('KVManagerGetKVStorePromiseTest006');
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
            console.info('KVManagerGetKVStorePromiseTest007 getKVStore success');
            kvStore = store;
            expect(store != null).assertTrue();
        }).catch((err) => {
            console.error('KVManagerGetKVStorePromiseTest007 getKVStore err ' + `, error code is ${err.code}, message is ${err.message}`);
            expect(null).assertFail();
        });
        done();
    })

    /**
     * @tc.name KVManagerGetKVStorePromiseTest008
     * @tc.desc Test Js Api KVManager.GetKVStore() testcase 008
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KVManagerGetKVStorePromiseTest008', 0, async function (done) {
        console.info('KVManagerGetKVStorePromiseTest008');
        const optionsInfo = {
            createIfMissing: true,
            encrypt: false,
            backup: false,
            autoSync: true,
            kvStoreType: factory.KVStoreType.DEVICE_COLLABORATION,
            schema: '',
            securityLevel: factory.SecurityLevel.NO_LEVEL,
        }
        await kvManager.getKVStore(TEST_STORE_ID, optionsInfo).then((store) => {
            console.info('KVManagerGetKVStorePromiseTest008 getKVStore success');
            kvStore = store;
            expect(store != null).assertTrue();
        }).catch((err) => {
            console.error('KVManagerGetKVStorePromiseTest008 getKVStore err ' + `, error code is ${err.code}, message is ${err.message}`);
            expect(null).assertFail();
        });
        done();
    })

    /**
     * @tc.name KVManagerGetKVStorePromiseTest009
     * @tc.desc Test Js Api KVManager.GetKVStore() testcase 009
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KVManagerGetKVStorePromiseTest009', 0, async function (done) {
        console.info('KVManagerGetKVStorePromiseTest009');
        const optionsInfo = {
            createIfMissing: true,
            encrypt: false,
            backup: false,
            autoSync: true,
            kvStoreType: factory.KVStoreType.DEVICE_COLLABORATION,
            schema: '',
            securityLevel: factory.SecurityLevel.S0,
        }
        await kvManager.getKVStore(TEST_STORE_ID, optionsInfo).then((store) => {
            console.info('KVManagerGetKVStorePromiseTest009 getKVStore success');
            kvStore = store;
            expect(store != null).assertTrue();
        }).catch((err) => {
            console.error('KVManagerGetKVStorePromiseTest009 getKVStore err ' + `, error code is ${err.code}, message is ${err.message}`);
            expect(null).assertFail();
        });
        done();
    })

    /**
     * @tc.name KVManagerGetKVStorePromiseTest010
     * @tc.desc Test Js Api KVManager.GetKVStore() testcase 010
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KVManagerGetKVStorePromiseTest010', 0, async function (done) {
        console.info('KVManagerGetKVStorePromiseTest010');
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
            console.info('KVManagerGetKVStorePromiseTest010 getKVStore success');
            kvStore = store;
            expect(store != null).assertTrue();
        }).catch((err) => {
            console.error('KVManagerGetKVStorePromiseTest010 getKVStore err ' + `, error code is ${err.code}, message is ${err.message}`);
            expect(null).assertFail();
        });
        done();
    })

    /**
     * @tc.name KVManagerGetKVStorePromiseTest011
     * @tc.desc Test Js Api KVManager.GetKVStore() testcase 011
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KVManagerGetKVStorePromiseTest011', 0, async function (done) {
        console.info('KVManagerGetKVStorePromiseTest011');
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
            console.info('KVManagerGetKVStorePromiseTest011 getKVStore success');
            kvStore = store;
            expect(store != null).assertTrue();
        }).catch((err) => {
            console.error('KVManagerGetKVStorePromiseTest011 getKVStore err ' + `, error code is ${err.code}, message is ${err.message}`);
            expect(null).assertFail();
        });
        done();
    })

    /**
     * @tc.name KVManagerGetKVStorePromiseTest012
     * @tc.desc Test Js Api KVManager.GetKVStore() testcase 012
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KVManagerGetKVStorePromiseTest012', 0, async function (done) {
        console.info('KVManagerGetKVStorePromiseTest012');
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
            console.info('KVManagerGetKVStorePromiseTest012 getKVStore success');
            kvStore = store;
            expect(store != null).assertTrue();
        }).catch((err) => {
            console.error('KVManagerGetKVStorePromiseTest012 getKVStore err ' + `, error code is ${err.code}, message is ${err.message}`);
            expect(null).assertFail();
        });
        done();
    })

    /**
     * @tc.name KVManagerGetKVStorePromiseTest013
     * @tc.desc Test Js Api KVManager.GetKVStore() testcase 013
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KVManagerGetKVStorePromiseTest013', 0, async function (done) {
        console.info('KVManagerGetKVStorePromiseTest013');
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
                console.info('KVManagerGetKVStorePromiseTest013 getKVStore success');
            }).catch((err) => {
                console.error('KVManagerGetKVStorePromiseTest013 getKVStore err ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('KVManagerGetKVStorePromiseTest013 getKVStore e ' + `, error code is ${e.code}, message is ${e.message}`);
        }
        done();
    })

    /**
     * @tc.name KVManagerGetKVStorePromiseTest014
     * @tc.desc Test Js Api KVManager.GetKVStore() testcase 014
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KVManagerGetKVStorePromiseTest014', 0, async function (done) {
        console.info('KVManagerGetKVStorePromiseTest014');
        const optionsInfo = {
            createIfMissing: true,
            encrypt: false,
            backup: false,
            autoSync: true,
            kvStoreType: factory.KVStoreType.MULTI_VERSION,
            schema: '',
            securityLevel: factory.SecurityLevel.NO_LEVEL,
        }
        await kvManager.getKVStore(TEST_STORE_ID, optionsInfo).then((store) => {
            console.info('KVManagerGetKVStorePromiseTest014 getKVStore success');
            expect(null).assertFail();
        }).catch((err) => {
            console.error('KVManagerGetKVStorePromiseTest014 getKVStore err ' + `, error code is ${err.code}, message is ${err.message}`);
            expect(true).assertTrue();
        });
        done();
    })

    /**
     * @tc.name KVManagerGetKVStorePromiseTest015
     * @tc.desc Test Js Api KVManager.GetKVStore() testcase 015
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KVManagerGetKVStorePromiseTest015', 0, async function (done) {
        console.info('KVManagerGetKVStorePromiseTest015');
        const optionsInfo = {
            createIfMissing: true,
            encrypt: false,
            backup: false,
            autoSync: true,
            kvStoreType: factory.KVStoreType.MULTI_VERSION,
            schema: '',
            securityLevel: factory.SecurityLevel.S0,
        }
        await kvManager.getKVStore(TEST_STORE_ID, optionsInfo).then((store) => {
            console.info('KVManagerGetKVStorePromiseTest015 getKVStore success');
            expect(null).assertFail();
        }).catch((err) => {
            console.error('KVManagerGetKVStorePromiseTest015 getKVStore err ' + `, error code is ${err.code}, message is ${err.message}`);
            expect(true).assertTrue();
        });
        done();
    })

    /**
     * @tc.name KVManagerGetKVStorePromiseTest016
     * @tc.desc Test Js Api KVManager.GetKVStore() testcase 016
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KVManagerGetKVStorePromiseTest016', 0, async function (done) {
        console.info('KVManagerGetKVStorePromiseTest016');
        const optionsInfo = {
            createIfMissing: true,
            encrypt: false,
            backup: false,
            autoSync: true,
            kvStoreType: factory.KVStoreType.MULTI_VERSION,
            schema: '',
            securityLevel: factory.SecurityLevel.S1,
        }
        await kvManager.getKVStore(TEST_STORE_ID, optionsInfo).then((store) => {
            console.info('KVManagerGetKVStorePromiseTest016 getKVStore success');
            expect(null).assertFail();
        }).catch((err) => {
            console.error('KVManagerGetKVStorePromiseTest016 getKVStore err ' + `, error code is ${err.code}, message is ${err.message}`);
            expect(true).assertTrue();
        });
        done();
    })

    /**
     * @tc.name KVManagerGetKVStorePromiseTest017
     * @tc.desc Test Js Api KVManager.GetKVStore() testcase 017
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KVManagerGetKVStorePromiseTest017', 0, async function (done) {
        console.info('KVManagerGetKVStorePromiseTest017');
        const optionsInfo = {
            createIfMissing: true,
            encrypt: false,
            backup: false,
            autoSync: true,
            kvStoreType: factory.KVStoreType.MULTI_VERSION,
            schema: '',
            securityLevel: factory.SecurityLevel.S2,
        }
        await kvManager.getKVStore(TEST_STORE_ID, optionsInfo).then((store) => {
            console.info('KVManagerGetKVStorePromiseTest017 getKVStore success');
            expect(null).assertFail();
        }).catch((err) => {
            console.error('KVManagerGetKVStorePromiseTest017 getKVStore err ' + `, error code is ${err.code}, message is ${err.message}`);
            expect(true).assertTrue();
        });
        done();
    })

    /**
     * @tc.name KVManagerGetKVStorePromiseTest018
     * @tc.desc Test Js Api KVManager.GetKVStore() testcase 018
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KVManagerGetKVStorePromiseTest018', 0, async function (done) {
        console.info('KVManagerGetKVStorePromiseTest018');
        const optionsInfo = {
            createIfMissing: true,
            encrypt: false,
            backup: false,
            autoSync: true,
            kvStoreType: factory.KVStoreType.MULTI_VERSION,
            schema: '',
            securityLevel: factory.SecurityLevel.S3,
        }
        await kvManager.getKVStore(TEST_STORE_ID, optionsInfo).then((store) => {
            console.info('KVManagerGetKVStorePromiseTest018 getKVStore success');
            expect(null).assertFail();
        }).catch((err) => {
            console.error('KVManagerGetKVStorePromiseTest018 getKVStore err ' + `, error code is ${err.code}, message is ${err.message}`);
            expect(true).assertTrue();
        });
        done();
    })

    /**
     * @tc.name KVManagerGetKVStorePromiseTest019
     * @tc.desc Test Js Api KVManager.GetKVStore() testcase 019
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KVManagerGetKVStorePromiseTest019', 0, async function (done) {
        console.info('KVManagerGetKVStorePromiseTest019');
        const optionsInfo = {
            createIfMissing: true,
            encrypt: false,
            backup: false,
            autoSync: true,
            kvStoreType: factory.KVStoreType.MULTI_VERSION,
            schema: '',
            securityLevel: factory.SecurityLevel.S4,
        }
        await kvManager.getKVStore(TEST_STORE_ID, optionsInfo).then((store) => {
            console.info('KVManagerGetKVStorePromiseTest019 getKVStore success');
            expect(null).assertFail();
        }).catch((err) => {
            console.error('KVManagerGetKVStorePromiseTest019 getKVStore err ' + `, error code is ${err.code}, message is ${err.message}`);
            expect(true).assertTrue();
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
        await kvManager.getKVStore(TEST_STORE_ID, options, async function (err, store) {
            console.info('KVManagerCloseKVStorePromiseTest001 getKVStore success');
            kvStore = store;
            await kvManager.closeKVStore(TEST_BUNDLE_NAME, TEST_STORE_ID, kvStore);
        });
        console.info('KVManagerCloseKVStorePromiseTest001 closeKVStore redo.');
        await kvManager.closeKVStore(TEST_BUNDLE_NAME, TEST_STORE_ID, kvStore).then(() => {
            console.info('KVManagerCloseKVStorePromiseTest001 closeKVStore twice');
            expect(null).assertFail();
        }).catch((err) => {
            console.error('KVManagerCloseKVStorePromiseTest001 closeKVStore twice err ' + `, error code is ${err.code}, message is ${err.message}`);
        });
        done();
    })

    /**
     * @tc.name KVManagerDeleteKVStorePromiseTest001
     * @tc.desc Test Js Api KVManager.DeleteKVStore() testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KVManagerDeleteKVStorePromiseTest001', 0, async function (done) {
        console.info('KVManagerDeleteKVStorePromiseTest001');
        await kvManager.deleteKVStore(TEST_BUNDLE_NAME, TEST_STORE_ID).then(() => {
            console.info('KVManagerDeleteKVStorePromiseTest001 deleteKVStore success');
        }).catch((err) => {
            expect(null).assertFail();
            console.error('KVManagerDeleteKVStorePromiseTest001 deleteKVStore err ' + `, error code is ${err.code}, message is ${err.message}`);
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
     * @tc.name KVManagerGetAllKVStoreIdPromiseTest002
     * @tc.desc Test Js Api KVManager.GetAllKVStoreId() testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KVManagerGetAllKVStoreIdPromiseTest002', 0, async function (done) {
        console.info('KVManagerGetAllKVStoreIdPromiseTest002');
        await kvManager.getKVStore(TEST_STORE_ID, options).then(async (store) => {
            console.info('KVManagerGetAllKVStoreIdPromiseTest002 getKVStore success');
            kvStore = store;
            await kvManager.getAllKVStoreId(TEST_BUNDLE_NAME).then((data) => {
                console.info('KVManagerGetAllKVStoreIdPromiseTest002 getAllKVStoreId success');
                console.info('KVManagerGetAllKVStoreIdPromiseTest002 size = ' + data.length);
                expect(1).assertEqual(data.length);
                console.info('KVManagerGetAllKVStoreIdPromiseTest002 data[0] = ' + data[0]);
                expect(TEST_STORE_ID).assertEqual(data[0]);
            }).catch((err) => {
                console.error('KVManagerGetAllKVStoreIdPromiseTest002 getAllKVStoreId err ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        }).catch((err) => {
            console.error('KVManagerGetAllKVStoreIdPromiseTest002 getKVStore err ' + `, error code is ${err.code}, message is ${err.message}`);
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
     * @tc.name: KVStorePutPromiseTest001
     * @tc.desc: Test Js Api KVManager.Put() testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KVStorePutPromiseTest001', 0, async function (done) {
        console.info('KVStorePutPromiseTest001');
        try {
            await kvStore.put(TEST_BUNDLE_NAME, TEST_STORE_ID).then((data) => {
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
            kvStore.delete(STORE_KEY).then((data) => {
            console.info("testKVStoreDelete001  promise delete success");
            expect(null).assertFail();
            }).catch((err) => {
                console.error('KVStoreDeletePromiseTest001 promise delete fail err' + `, error code is ${err.code}, message is ${err.message}`);
            });
        }catch (e) {
            console.error('KVStoreDeletePromiseTest001 promise delete fail err' + `, error code is ${err.code}, message is ${err.message}`);
        }
        done();
    })

    /**
     * @tc.name: CreateKVManagerPromiseTest001
     * @tc.desc: Test Js Api createKVManager testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
     it('CreateKVManagerPromiseTest001', 0, async function (done) {
        console.info('CreateKVManagerPromiseTest001');
        const config = {
            bundleName: TEST_BUNDLE_NAME,
            userInfo: {
                userId: '0',
                userType: factory.UserType.SAME_USER_ID
            },
            context:context
        }
        try {
            await factory.createKVManager(config).then(async (manager) => {
                kvManager = manager;
                expect(manager !=null).assertTrue();
                console.info('CreateKVManagerPromiseTest001 createKVManager success');
                await kvManager.getKVStore(TEST_STORE_ID, options).then(async (store) => {
                    kvStore = store;
                    console.info("testcreateKVManager001 getKVStore success");
                    await kvStore.put(STORE_KEY, STORE_VALUE).then(async (data) => {
                        console.info('CreateKVManagerPromiseTest001 put data success');
                        await kvStore.get(STORE_KEY).then((data) => {
                            console.info("testcreateKVManager001  get data success");
                            expect(data).assertEqual(STORE_VALUE);
                        }).catch((err) => {
                            console.error('CreateKVManagerPromiseTest001 get data err' + `, error code is ${err.code}, message is ${err.message}`);
                        });
                    }).catch((err) => {
                        console.error('CreateKVManagerPromiseTest001 put data err' + `, error code is ${err.code}, message is ${err.message}`);
                    });
                }).catch((err) => {
                    console.info("testcreateKVManager001 getKVStore err: "  + JSON.stringify(err));
                    expect(null).assertFail();
                });
            }).catch((err) => {
                console.error('CreateKVManagerPromiseTest001 createKVManager err ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail()
            });
        }catch (e) {
            console.error('CreateKVManagerPromiseTest001 promise delete fail err' + `, error code is ${err.code}, message is ${err.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name: CreateKVManagerPromiseTest002
     * @tc.desc: Test Js Api createKVManager testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
     it('CreateKVManagerPromiseTest002', 0, async function (done) {
        console.info('CreateKVManagerPromiseTest002');
        const config = {
            bundleName: TEST_BUNDLE_NAME,
            userInfo: {
                userId: '0',
                userType: factory.UserType.SAME_USER_ID
            },
            context:contextApplication
        }
        try {
            await factory.createKVManager(config).then(async (manager) => {
                kvManager = manager;
                console.info('CreateKVManagerPromiseTest002 createKVManager success');
                await kvManager.getKVStore(TEST_STORE_ID, options).then(async (store) => {
                    console.info("testcreateKVManager002 getKVStore success");
                    kvStore = store;
                    await kvStore.put(STORE_KEY, STORE_VALUE).then(async (data) => {
                        console.info('CreateKVManagerPromiseTest002 put data success');
                        await kvStore.get(STORE_KEY).then((data) => {
                            console.info("testcreateKVManager002  get data success");
                            expect(data).assertEqual(STORE_VALUE);
                        }).catch((err) => {
                            console.error('CreateKVManagerPromiseTest002 get data err' + `, error code is ${err.code}, message is ${err.message}`);
                        });
                    }).catch((err) => {
                        console.error('CreateKVManagerPromiseTest002 put data err' + `, error code is ${err.code}, message is ${err.message}`);
                    });
                }).catch((err) => {
                    console.info("testcreateKVManager002 getKVStore err: "  + JSON.stringify(err));
                    expect(null).assertFail();
                });
            }).catch((err) => {
                console.error('CreateKVManagerPromiseTest002 createKVManager err ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail()
            });
        }catch (e) {
            console.error('CreateKVManagerPromiseTest002 promise delete fail err' + `, error code is ${err.code}, message is ${err.message}`);
            expect(null).assertFail();
        }
        done();
    })

})
