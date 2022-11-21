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
import factory from '@ohos.data.distributedData'
import abilityFeatureAbility from '@ohos.ability.featureAbility'

var context = abilityFeatureAbility.getContext();
const KEY_TEST_INT_ELEMENT = 'key_test_int';
const KEY_TEST_FLOAT_ELEMENT = 'key_test_float';
const KEY_TEST_BOOLEAN_ELEMENT = 'key_test_boolean';
const KEY_TEST_STRING_ELEMENT = 'key_test_string';
const KEY_TEST_SYNC_ELEMENT = 'key_test_sync';

const VALUE_TEST_INT_ELEMENT = 123;
const VALUE_TEST_FLOAT_ELEMENT = 321.12;
const VALUE_TEST_BOOLEAN_ELEMENT = true;
const VALUE_TEST_STRING_ELEMENT = 'value-string-001';
const VALUE_TEST_SYNC_ELEMENT = 'value-string-001';

const TEST_BUNDLE_NAME = 'com.example.myapplication';
const TEST_STORE_ID = 'storeId';
var kvManager = null;
var kvStore = null;
var localDeviceId = null;
const USED_DEVICE_IDS =  ['A12C1F9261528B21F95778D2FDC0B2E33943E6251AC5487F4473D005758905DB'];
const UNUSED_DEVICE_IDS =  [];  /* add you test device-ids here */
var syncDeviceIds = USED_DEVICE_IDS.concat(UNUSED_DEVICE_IDS);

function sleep(ms) {
    return new Promise(resolve => setTimeout(resolve, ms));
}

function putBatchString(len, prefix) {
    let entries = [];
    for (var i = 0; i < len; i++) {
        var entry = {
            key : prefix + i,
            value : {
                type : factory.ValueType.STRING,
                value : 'batch_test_string_value'
            }
        }
        entries.push(entry);
    }
    return entries;
}
describe('deviceKvStorePromiseTest', function () {
    const config = {
        bundleName : TEST_BUNDLE_NAME,
        userInfo : {
            userId : '0',
            userType : factory.UserType.SAME_USER_ID
        },
        context: context
    }

    const options = {
        createIfMissing : true,
        encrypt : false,
        backup : false,
        autoSync : true,
        kvStoreType : factory.KVStoreType.DEVICE_COLLABORATION,
        schema : '',
        securityLevel : factory.SecurityLevel.S2,
    }

    beforeAll(async function (done) {
        console.info('beforeAll config:'+ JSON.stringify(config));
        await factory.createKVManager(config).then((manager) => {
            kvManager = manager;
            console.info('beforeAll createKVManager success');
        }).catch((err) => {
            console.error('beforeAll createKVManager err ' + `, error code is ${err.code}, message is ${err.message}`);
        });
        await kvManager.getKVStore(TEST_STORE_ID, options).then((store) => {
            kvStore = store;
            console.info('beforeAll getKVStore for getDeviceId success');
        }).catch((err) => {
            console.error('beforeAll getKVStore err ' + `, error code is ${err.code}, message is ${err.message}`);
        });
        var getDeviceId = new Promise((resolve, reject) => {
            kvStore.on('dataChange', 0, function (data) {
                console.info('beforeAll on data change: ' + JSON.stringify(data));
                resolve(data.deviceId);
            });
            kvStore.put("getDeviceId", "byPut").then((data) => {
                console.info('beforeAll put success');
                expect(data == undefined).assertTrue();
            });
            setTimeout(() => {
                reject(new Error('not resolved in 2 second, reject it.'))
            }, 2000);
        });
        await getDeviceId.then(function(deviceId) {
            console.info('beforeAll getDeviceId ' + JSON.stringify(deviceId));
            localDeviceId = deviceId;
        }).catch((error) => {
            console.error('beforeAll can NOT getDeviceId, fail: ' + `, error code is ${error.code}, message is ${error.message}`);
            expect(null).assertFail();
        });
        await kvManager.closeKVStore(TEST_BUNDLE_NAME, TEST_STORE_ID, kvStore);
        await kvManager.deleteKVStore(TEST_BUNDLE_NAME, TEST_STORE_ID);
        kvStore = null;
        console.info('beforeAll end');
        done();
    })

    afterAll(async function (done) {
        console.info('afterAll');
        kvManager = null;
        kvStore = null;
        done();
    })

    beforeEach(async function (done) {
        console.info('beforeEach' + JSON.stringify(options));
        await kvManager.getKVStore(TEST_STORE_ID, options).then((store) => {
            kvStore = store;
            console.info('beforeEach getKVStore success');
        }).catch((err) => {
            console.error('beforeEach getKVStore err ' + `, error code is ${err.code}, message is ${err.message}`);
        });
        console.info('beforeEach end');
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
     * @tc.name DeviceKvStorePutStringPromiseTest001
     * @tc.desc Test Js Api DeviceKvStore.Put(String) testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStorePutStringPromiseTest001', 0, async function (done) {
        console.info('DeviceKvStorePutStringPromiseTest001');
        try {
            await kvStore.put(KEY_TEST_STRING_ELEMENT, null).then((data) => {
                console.info('DeviceKvStorePutStringPromiseTest001 put success');
                expect(null).assertFail();
            }).catch((error) => {
                console.error('DeviceKvStorePutStringPromiseTest001 put error' + `, error code is ${error.code}, message is ${error.message}`);
            });
        } catch (e) {
            console.error('DeviceKvStorePutStringPromiseTest001 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStorePutStringPromiseTest002
     * @tc.desc Test Js Api DeviceKvStore.Put(String) testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStorePutStringPromiseTest002', 0, async function (done) {
        console.info('DeviceKvStorePutStringPromiseTest002');
        try {
            await kvStore.put(KEY_TEST_STRING_ELEMENT, '').then((data) => {
                console.info('DeviceKvStorePutStringPromiseTest002 put success');
                expect(data == undefined).assertTrue();
            }).catch((error) => {
                console.error('DeviceKvStorePutStringPromiseTest002 put error' + `, error code is ${error.code}, message is ${error.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('DeviceKvStorePutStringPromiseTest002 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStorePutStringPromiseTest003
     * @tc.desc Test Js Api DeviceKvStore.Put(String) testcase 003
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStorePutStringPromiseTest003', 0, async function (done) {
        console.info('DeviceKvStorePutStringPromiseTest003');
        try {
            await kvStore.put(KEY_TEST_STRING_ELEMENT, VALUE_TEST_STRING_ELEMENT).then((data) => {
                console.info('DeviceKvStorePutStringPromiseTest003 put success');
                expect(data == undefined).assertTrue();
            }).catch((error) => {
                console.error('DeviceKvStorePutStringPromiseTest003 put error' + `, error code is ${error.code}, message is ${error.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('DeviceKvStorePutStringPromiseTest003 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStorePutStringPromiseTest004
     * @tc.desc Test Js Api DeviceKvStore.Put(String) testcase 004
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStorePutStringPromiseTest004', 0, async function (done) {
        console.info('DeviceKvStorePutStringPromiseTest004');
        try {
            var str = '';
            for (var i = 0 ; i < 4095; i++) {
                str += 'x';
            }
            await kvStore.put(KEY_TEST_STRING_ELEMENT, str).then(async (data) => {
                console.info('DeviceKvStorePutStringPromiseTest004 put success');
                expect(data == undefined).assertTrue();
                await kvStore.get(localDeviceId, KEY_TEST_STRING_ELEMENT).then((data) => {
                    console.info('DeviceKvStorePutStringPromiseTest004 get success data ' + data);
                    expect(str == data).assertTrue();
                }).catch((err) => {
                    console.error('DeviceKvStorePutStringPromiseTest004 get fail ' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(null).assertFail();
                });
            }).catch((error) => {
                console.error('DeviceKvStorePutStringPromiseTest004 put error' + `, error code is ${error.code}, message is ${error.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('DeviceKvStorePutStringPromiseTest004 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreGetStringPromiseTest001
     * @tc.desc Test Js Api DeviceKvStore.Get(String) testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreGetStringPromiseTest001', 0, async function (done) {
        console.info('DeviceKvStoreGetStringPromiseTest001');
        try {
            await kvStore.put(KEY_TEST_STRING_ELEMENT, VALUE_TEST_STRING_ELEMENT).then(async (data) => {
                console.info('DeviceKvStoreGetStringPromiseTest001 put success');
                expect(data == undefined).assertTrue();
                await kvStore.get(localDeviceId, KEY_TEST_STRING_ELEMENT).then((data) => {
                    console.info('DeviceKvStoreGetStringPromiseTest001 get success');
                    expect(VALUE_TEST_STRING_ELEMENT == data).assertTrue();
                }).catch((err) => {
                    console.error('DeviceKvStoreGetStringPromiseTest001 get fail ' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(null).assertFail();
                });
            }).catch((error) => {
                console.error('DeviceKvStoreGetStringPromiseTest001 put error' + `, error code is ${error.code}, message is ${error.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('DeviceKvStoreGetStringPromiseTest001 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreGetStringPromiseTest002
     * @tc.desc Test Js Api DeviceKvStore.Get(String) testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreGetStringPromiseTest002', 0, async function (done) {
        console.info('DeviceKvStoreGetStringPromiseTest002');
        try {
            await kvStore.get(localDeviceId, KEY_TEST_STRING_ELEMENT).then((data) => {
                console.info('DeviceKvStoreGetStringPromiseTest002 get success');
                expect(null).assertFail();
            }).catch((err) => {
                console.error('DeviceKvStoreGetStringPromiseTest002 get fail ' + `, error code is ${err.code}, message is ${err.message}`);
            });
        } catch (e) {
            console.error('DeviceKvStoreGetStringPromiseTest002 get e ' + `, error code is ${e.code}, message is ${e.message}`);
        }
        done();
    })

    /**
     * @tc.name DeviceKvStorePutIntPromiseTest001
     * @tc.desc Test Js Api DeviceKvStore.Put(Int) testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStorePutIntPromiseTest001', 0, async function (done) {
        console.info('DeviceKvStorePutIntPromiseTest001');
        try {
            await kvStore.put(KEY_TEST_INT_ELEMENT, VALUE_TEST_INT_ELEMENT).then((data) => {
                console.info('DeviceKvStorePutIntPromiseTest001 put success');
                expect(data == undefined).assertTrue();
            }).catch((err) => {
                console.error('DeviceKvStorePutIntPromiseTest001 put fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('DeviceKvStorePutIntPromiseTest001 put e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStorePutIntPromiseTest002
     * @tc.desc Test Js Api DeviceKvStore.Put(Int) testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStorePutIntPromiseTest002', 0, async function (done) {
        console.info('DeviceKvStorePutIntPromiseTest002');
        try {
            var intValue = 987654321;
            await kvStore.put(KEY_TEST_INT_ELEMENT, intValue).then(async (data) => {
                console.info('DeviceKvStorePutIntPromiseTest002 put success');
                expect(data == undefined).assertTrue();
                await kvStore.get(localDeviceId, KEY_TEST_INT_ELEMENT).then((data) => {
                    console.info('DeviceKvStorePutIntPromiseTest002 get success');
                    expect(intValue == data).assertTrue();
                }).catch((err) => {
                    console.error('DeviceKvStorePutIntPromiseTest002 get fail ' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(null).assertFail();
                });
            }).catch((err) => {
                console.error('DeviceKvStorePutIntPromiseTest002 put fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('DeviceKvStorePutIntPromiseTest002 put e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStorePutIntPromiseTest003
     * @tc.desc Test Js Api DeviceKvStore.Put(Int) testcase 003
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStorePutIntPromiseTest003', 0, async function (done) {
        console.info('DeviceKvStorePutIntPromiseTest003');
        try {
            var intValue = Number.MAX_VALUE;
            await kvStore.put(KEY_TEST_INT_ELEMENT, intValue).then(async (data) => {
                console.info('DeviceKvStorePutIntPromiseTest003 put success');
                expect(data == undefined).assertTrue();
                await kvStore.get(localDeviceId, KEY_TEST_INT_ELEMENT).then((data) => {
                    console.info('DeviceKvStorePutIntPromiseTest003 get success');
                    expect(intValue == data).assertTrue();
                }).catch((err) => {
                    console.error('DeviceKvStorePutIntPromiseTest003 get fail ' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(null).assertFail();
                });
            }).catch((err) => {
                console.error('DeviceKvStorePutIntPromiseTest003 put fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('DeviceKvStorePutIntPromiseTest003 put e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStorePutIntPromiseTest004
     * @tc.desc Test Js Api DeviceKvStore.Put(Int) testcase 004
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStorePutIntPromiseTest004', 0, async function (done) {
        console.info('DeviceKvStorePutIntPromiseTest004');
        try {
            var intValue = Number.MIN_VALUE;
            await kvStore.put(KEY_TEST_INT_ELEMENT, intValue).then(async (data) => {
                console.info('DeviceKvStorePutIntPromiseTest004 put success');
                expect(data == undefined).assertTrue();
                await kvStore.get(localDeviceId, KEY_TEST_INT_ELEMENT).then((data) => {
                    console.info('DeviceKvStorePutIntPromiseTest004 get success');
                    expect(intValue == data).assertTrue();
                }).catch((err) => {
                    console.error('DeviceKvStorePutIntPromiseTest004 get fail ' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(null).assertFail();
                });
            }).catch((err) => {
                console.error('DeviceKvStorePutIntPromiseTest004 put fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('DeviceKvStorePutIntPromiseTest004 put e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreGetIntPromiseTest001
     * @tc.desc Test Js Api DeviceKvStore.Get(Int) testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreGetIntPromiseTest001', 0, async function (done) {
        console.info('DeviceKvStoreGetIntPromiseTest001');
        try {
            await kvStore.put(KEY_TEST_INT_ELEMENT, VALUE_TEST_INT_ELEMENT).then(async (data) => {
                console.info('DeviceKvStoreGetIntPromiseTest001 put success');
                expect(data == undefined).assertTrue();
                await kvStore.get(localDeviceId, KEY_TEST_INT_ELEMENT).then((data) => {
                    console.info('DeviceKvStoreGetIntPromiseTest001 get success');
                    expect(VALUE_TEST_INT_ELEMENT == data).assertTrue();
                }).catch((err) => {
                    console.error('DeviceKvStoreGetIntPromiseTest001 get fail ' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(null).assertFail();
                });
            }).catch((err) => {
                console.error('DeviceKvStoreGetIntPromiseTest001 put fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('DeviceKvStoreGetIntPromiseTest001 put e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStorePutBoolPromiseTest001
     * @tc.desc Test Js Api DeviceKvStore.Put(Bool) testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStorePutBoolPromiseTest001', 0, async function (done) {
        console.info('DeviceKvStorePutBoolPromiseTest001');
        try {
            await kvStore.put(KEY_TEST_BOOLEAN_ELEMENT, VALUE_TEST_BOOLEAN_ELEMENT).then((data) => {
                console.info('DeviceKvStorePutBoolPromiseTest001 put success');
                expect(data == undefined).assertTrue();
            }).catch((err) => {
                console.error('DeviceKvStorePutBoolPromiseTest001 put fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('DeviceKvStorePutBoolPromiseTest001 put e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreGetBoolPromiseTest001
     * @tc.desc Test Js Api DeviceKvStore.Get(Bool) testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreGetBoolPromiseTest001', 0, async function (done) {
        console.info('DeviceKvStoreGetBoolPromiseTest001');
        try {
            var boolValue = false;
            await kvStore.put(KEY_TEST_BOOLEAN_ELEMENT, boolValue).then(async (data) => {
                console.info('DeviceKvStoreGetBoolPromiseTest001 put success');
                expect(data == undefined).assertTrue();
                await kvStore.get(localDeviceId, KEY_TEST_BOOLEAN_ELEMENT).then((data) => {
                    console.info('DeviceKvStoreGetBoolPromiseTest001 get success');
                    expect(boolValue == data).assertTrue();
                }).catch((err) => {
                    console.error('DeviceKvStoreGetBoolPromiseTest001 get fail ' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(null).assertFail();
                });
            }).catch((err) => {
                console.error('DeviceKvStoreGetBoolPromiseTest001 put fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('DeviceKvStoreGetBoolPromiseTest001 put e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStorePutFloatPromiseTest001
     * @tc.desc Test Js Api DeviceKvStore.Put(Float) testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStorePutFloatPromiseTest001', 0, async function (done) {
        console.info('DeviceKvStorePutFloatPromiseTest001');
        try {
            await kvStore.put(KEY_TEST_FLOAT_ELEMENT, VALUE_TEST_FLOAT_ELEMENT).then((data) => {
                console.info('DeviceKvStorePutFloatPromiseTest001 put success');
                expect(data == undefined).assertTrue();
            }).catch((err) => {
                console.error('DeviceKvStorePutFloatPromiseTest001 put fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('DeviceKvStorePutFloatPromiseTest001 put e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreGetFloatPromiseTest001
     * @tc.desc Test Js Api DeviceKvStore.Get(Float) testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreGetFloatPromiseTest001', 0, async function (done) {
        console.info('DeviceKvStoreGetFloatPromiseTest001');
        try {
            var floatValue = 123456.654321;
            await kvStore.put(KEY_TEST_FLOAT_ELEMENT, floatValue).then(async (data) => {
                console.info('DeviceKvStoreGetFloatPromiseTest001 put success');
                expect(data == undefined).assertTrue();
                await kvStore.get(localDeviceId, KEY_TEST_FLOAT_ELEMENT).then((data) => {
                    console.info('DeviceKvStoreGetFloatPromiseTest001 get success');
                    expect(floatValue == data).assertTrue();
                }).catch((err) => {
                    console.error('DeviceKvStoreGetFloatPromiseTest001 get fail ' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(null).assertFail();
                });
            }).catch((err) => {
                console.error('DeviceKvStoreGetFloatPromiseTest001 put fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('DeviceKvStoreGetFloatPromiseTest001 put e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreGetFloatPromiseTest002
     * @tc.desc Test Js Api DeviceKvStore.Get(Float) testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreGetFloatPromiseTest002', 0, async function (done) {
        console.info('DeviceKvStoreGetFloatPromiseTest002');
        try {
            var floatValue = 123456.0;
            await kvStore.put(KEY_TEST_FLOAT_ELEMENT, floatValue).then(async (data) => {
                console.info('DeviceKvStoreGetFloatPromiseTest002 put success');
                expect(data == undefined).assertTrue();
                await kvStore.get(localDeviceId, KEY_TEST_FLOAT_ELEMENT).then((data) => {
                    console.info('DeviceKvStoreGetFloatPromiseTest002 get success');
                    expect(floatValue == data).assertTrue();
                }).catch((err) => {
                    console.error('DeviceKvStoreGetFloatPromiseTest002 get fail ' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(null).assertFail();
                });
            }).catch((err) => {
                console.error('DeviceKvStoreGetFloatPromiseTest002 put fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('DeviceKvStoreGetFloatPromiseTest002 put e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreGetFloatPromiseTest003
     * @tc.desc Test Js Api DeviceKvStore.Get(Float) testcase 003
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreGetFloatPromiseTest003', 0, async function (done) {
        console.info('DeviceKvStoreGetFloatPromiseTest003');
        try {
            var floatValue = 123456.00;
            await kvStore.put(KEY_TEST_FLOAT_ELEMENT, floatValue).then(async (data) => {
                console.info('DeviceKvStoreGetFloatPromiseTest003 put success');
                expect(data == undefined).assertTrue();
                await kvStore.get(localDeviceId, KEY_TEST_FLOAT_ELEMENT).then((data) => {
                    console.info('DeviceKvStoreGetFloatPromiseTest003 get success');
                    expect(floatValue == data).assertTrue();
                }).catch((err) => {
                    console.error('DeviceKvStoreGetFloatPromiseTest003 get fail ' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(null).assertFail();
                });
            }).catch((err) => {
                console.error('DeviceKvStoreGetFloatPromiseTest003 put fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('DeviceKvStoreGetFloatPromiseTest003 put e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreDeleteStringPromiseTest001
     * @tc.desc Test Js Api DeviceKvStore.DeleteString() testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreDeleteStringPromiseTest001', 0, async function (done) {
        console.info('DeviceKvStoreDeleteStringPromiseTest001');
        try {
            var str = 'this is a test string';
            await kvStore.put(KEY_TEST_STRING_ELEMENT, str).then(async (data) => {
                console.info('DeviceKvStoreDeleteStringPromiseTest001 put success');
                expect(data == undefined).assertTrue();
                await kvStore.delete(KEY_TEST_STRING_ELEMENT).then((data) => {
                    console.info('DeviceKvStoreDeleteStringPromiseTest001 delete success');
                    expect(data == undefined).assertTrue();
                }).catch((err) => {
                    console.error('DeviceKvStoreDeleteStringPromiseTest001 delete fail ' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(null).assertFail();
                });
            }).catch((err) => {
                console.error('DeviceKvStoreDeleteStringPromiseTest001 put fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('DeviceKvStoreDeleteStringPromiseTest001 put e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreDeleteStringPromiseTest002
     * @tc.desc Test Js Api DeviceKvStore.DeleteString() testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreDeleteStringPromiseTest002', 0, async function (done) {
        console.info('DeviceKvStoreDeleteStringPromiseTest002');
        try {
            var str = '';
            for (var i = 0 ; i < 4096; i++) {
                str += 'x';
            }
            await kvStore.put(KEY_TEST_STRING_ELEMENT, str).then(async (data) => {
                console.info('DeviceKvStoreDeleteStringPromiseTest002 put success');
                expect(data == undefined).assertTrue();
                await kvStore.delete(KEY_TEST_STRING_ELEMENT).then((data) => {
                    console.info('DeviceKvStoreDeleteStringPromiseTest002 delete success');
                    expect(data == undefined).assertTrue();
                }).catch((err) => {
                    console.error('DeviceKvStoreDeleteStringPromiseTest002 delete fail ' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(null).assertFail();
                });
            }).catch((err) => {
                console.error('DeviceKvStoreDeleteStringPromiseTest002 put fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('DeviceKvStoreDeleteStringPromiseTest002 put e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreDeleteStringPromiseTest003
     * @tc.desc Test Js Api DeviceKvStore.DeleteString() testcase 003
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreDeleteStringPromiseTest003', 0, async function (done) {
        console.info('DeviceKvStoreDeleteStringPromiseTest003');
        try {
            await kvStore.delete(KEY_TEST_STRING_ELEMENT).then((data) => {
                console.info('DeviceKvStoreDeleteStringPromiseTest003 delete success');
                expect(data == undefined).assertTrue();
            }).catch((err) => {
                console.error('DeviceKvStoreDeleteStringPromiseTest003 delete fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('DeviceKvStoreDeleteStringPromiseTest003 put e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreDeleteIntPromiseTest001
     * @tc.desc Test Js Api DeviceKvStore.DeleteInt() testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreDeleteIntPromiseTest001', 0, async function (done) {
        console.info('DeviceKvStoreDeleteIntPromiseTest001');
        try {
            await kvStore.put(KEY_TEST_INT_ELEMENT, VALUE_TEST_INT_ELEMENT).then(async (data) => {
                console.info('DeviceKvStoreDeleteIntPromiseTest001 put success');
                expect(data == undefined).assertTrue();
                await kvStore.delete(KEY_TEST_INT_ELEMENT).then((data) => {
                    console.info('DeviceKvStoreDeleteIntPromiseTest001 delete success');
                    expect(data == undefined).assertTrue();
                }).catch((err) => {
                    console.error('DeviceKvStoreDeleteIntPromiseTest001 delete fail ' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(null).assertFail();
                });
            }).catch((err) => {
                console.error('DeviceKvStoreDeleteIntPromiseTest001 put fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('DeviceKvStoreDeleteIntPromiseTest001 put e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreDeleteFloatPromiseTest001
     * @tc.desc Test Js Api DeviceKvStore.DeleteFloat() testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreDeleteFloatPromiseTest001', 0, async function (done) {
        console.info('DeviceKvStoreDeleteFloatPromiseTest001');
        try {
            await kvStore.put(KEY_TEST_FLOAT_ELEMENT, VALUE_TEST_FLOAT_ELEMENT).then(async (data) => {
                console.info('DeviceKvStoreDeleteFloatPromiseTest001 put success');
                expect(data == undefined).assertTrue();
                await kvStore.delete(KEY_TEST_FLOAT_ELEMENT).then((data) => {
                    console.info('DeviceKvStoreDeleteFloatPromiseTest001 delete success');
                    expect(data == undefined).assertTrue();
                }).catch((err) => {
                    console.error('DeviceKvStoreDeleteFloatPromiseTest001 delete fail ' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(null).assertFail();
                });
            }).catch((err) => {
                console.error('DeviceKvStoreDeleteFloatPromiseTest001 put fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('DeviceKvStoreDeleteFloatPromiseTest001 put e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreDeleteBoolPromiseTest001
     * @tc.desc Test Js Api DeviceKvStore.DeleteBool() testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreDeleteBoolPromiseTest001', 0, async function (done) {
        console.info('DeviceKvStoreDeleteBoolPromiseTest001');
        try {
            await kvStore.put(KEY_TEST_BOOLEAN_ELEMENT, VALUE_TEST_BOOLEAN_ELEMENT).then(async (data) => {
                console.info('DeviceKvStoreDeleteBoolPromiseTest001 put success');
                expect(data == undefined).assertTrue();
                await kvStore.delete(KEY_TEST_BOOLEAN_ELEMENT).then((data) => {
                    console.info('DeviceKvStoreDeleteBoolPromiseTest001 delete success');
                    expect(data == undefined).assertTrue();
                }).catch((err) => {
                    console.error('DeviceKvStoreDeleteBoolPromiseTest001 delete fail ' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(null).assertFail();
                });
            }).catch((err) => {
                console.error('DeviceKvStoreDeleteBoolPromiseTest001 put fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('DeviceKvStoreDeleteBoolPromiseTest001 put e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreOnChangePromiseTest001
     * @tc.desc Test Js Api DeviceKvStore.OnChange() testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreOnChangePromiseTest001', 0, async function (done) {
        try {
            kvStore.on('dataChange', 0, function (data) {
                console.info('DeviceKvStoreOnChangePromiseTest001 0' + JSON.stringify(data))
                expect(data != null).assertTrue();
            });
            await kvStore.put(KEY_TEST_FLOAT_ELEMENT, VALUE_TEST_FLOAT_ELEMENT).then((data) => {
                console.info('DeviceKvStoreOnChangePromiseTest001 put success');
                expect(data == undefined).assertTrue();
            }).catch((error) => {
                console.error('DeviceKvStoreOnChangePromiseTest001 put fail ' + `, error code is ${error.code}, message is ${error.message}`);
                expect(null).assertFail();
            });
        }catch(e) {
            console.error('DeviceKvStoreOnChangePromiseTest001 put e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreOnChangePromiseTest002
     * @tc.desc Test Js Api DeviceKvStore.OnChange() testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreOnChangePromiseTest002', 0, async function (done) {
        try {
            kvStore.on('dataChange', 1, function (data) {
                console.info('DeviceKvStoreOnChangePromiseTest002 0' + JSON.stringify(data))
                expect(data != null).assertTrue();
            });
            await kvStore.put(KEY_TEST_FLOAT_ELEMENT, VALUE_TEST_FLOAT_ELEMENT).then((data) => {
                console.info('DeviceKvStoreOnChangePromiseTest002 put success');
                expect(data == undefined).assertTrue();
            }).catch((error) => {
                console.error('DeviceKvStoreOnChangePromiseTest002 put fail ' + `, error code is ${error.code}, message is ${error.message}`);
                expect(null).assertFail();
            });
        }catch(e) {
            console.error('DeviceKvStoreOnChangePromiseTest002 put e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreOnChangePromiseTest003
     * @tc.desc Test Js Api DeviceKvStore.OnChange() testcase 003
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreOnChangePromiseTest003', 0, async function (done) {
        try {
            kvStore.on('dataChange', 2, function (data) {
                console.info('DeviceKvStoreOnChangePromiseTest003 0' + JSON.stringify(data))
                expect(data != null).assertTrue();
            });
            await kvStore.put(KEY_TEST_FLOAT_ELEMENT, VALUE_TEST_FLOAT_ELEMENT).then((data) => {
                console.info('DeviceKvStoreOnChangePromiseTest003 put success');
                expect(data == undefined).assertTrue();
            }).catch((error) => {
                console.error('DeviceKvStoreOnChangePromiseTest003 put fail ' + `, error code is ${error.code}, message is ${error.message}`);
                expect(null).assertFail();
            });
        }catch(e) {
            console.error('DeviceKvStoreOnChangePromiseTest003 put e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreOnSyncCompletePromiseTest001
     * @tc.desc Test Js Api DeviceKvStore.OnSyncComplete() testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreOnSyncCompletePromiseTest001', 0, async function (done) {
        try {
            kvStore.on('syncComplete', function (data) {
                console.info('DeviceKvStoreOnSyncCompletePromiseTest001 0' + data)
                expect(data != null).assertTrue();
            });
            await kvStore.put(KEY_TEST_SYNC_ELEMENT, VALUE_TEST_SYNC_ELEMENT).then((data) => {
                console.info('DeviceKvStoreOnSyncCompletePromiseTest001 put success');
                expect(data == undefined).assertTrue();
            }).catch((error) => {
                console.error('DeviceKvStoreOnSyncCompletePromiseTest001 put failed:' + `, error code is ${e.code}, message is ${e.message}`);
                expect(null).assertFail();
            });
            try {
                var mode = factory.SyncMode.PULL_ONLY;
                console.info('kvStore.sync to ' + JSON.stringify(syncDeviceIds));
                kvStore.sync(syncDeviceIds, mode);
            } catch (e) {
                console.error('DeviceKvStoreOnSyncCompletePromiseTest001 sync no peer device :e:' + `, error code is ${e.code}, message is ${e.message}`);
            }
        } catch(e) {
            console.error('DeviceKvStoreOnSyncCompletePromiseTest001 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreOnSyncCompletePromiseTest002
     * @tc.desc Test Js Api DeviceKvStore.OnSyncComplete() testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreOnSyncCompletePromiseTest002', 0, async function (done) {
        try {
            kvStore.on('syncComplete', function (data) {
                console.info('DeviceKvStoreOnSyncCompletePromiseTest002 0' + data)
                expect(data != null).assertTrue();
            });
            await kvStore.put(KEY_TEST_SYNC_ELEMENT, VALUE_TEST_SYNC_ELEMENT).then((data) => {
                console.info('DeviceKvStoreOnSyncCompletePromiseTest002 put success');
                expect(data == undefined).assertTrue();
            }).catch((error) => {
                console.error('DeviceKvStoreOnSyncCompletePromiseTest002 put failed:' + `, error code is ${e.code}, message is ${e.message}`);
                expect(null).assertFail();
            });
            try {
                var mode = factory.SyncMode.PUSH_ONLY;
                console.info('kvStore.sync to ' + JSON.stringify(syncDeviceIds));
                kvStore.sync(syncDeviceIds, mode);
            } catch(error) {
                console.error('DeviceKvStoreOnSyncCompletePromiseTest002 no peer device :e:' + `, error code is ${error.code}, message is ${error.message}`);
            }
        } catch(e) {
            console.error('DeviceKvStoreOnSyncCompletePromiseTest002 put e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreOnSyncCompletePromiseTest003
     * @tc.desc Test Js Api DeviceKvStore.OnSyncComplete() testcase 003
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreOnSyncCompletePromiseTest003', 0, async function (done) {
        try {
            kvStore.on('syncComplete', function (data) {
                console.info('DeviceKvStoreOnSyncCompletePromiseTest003 0' + data)
                expect(data != null).assertTrue();
            });
            await kvStore.put(KEY_TEST_SYNC_ELEMENT, VALUE_TEST_SYNC_ELEMENT).then((data) => {
                console.info('DeviceKvStoreOnSyncCompletePromiseTest003 put success');
                expect(data == undefined).assertTrue();
            }).catch((error) => {
                console.error('DeviceKvStoreOnSyncCompletePromiseTest003 put failed:' + `, error code is ${e.code}, message is ${e.message}`);
                expect(null).assertFail();
            });
            try {
                var mode = factory.SyncMode.PUSH_PULL;
                console.info('kvStore.sync to ' + JSON.stringify(syncDeviceIds));
                kvStore.sync(syncDeviceIds, mode);
            } catch(error) {
                console.error('DeviceKvStoreOnSyncCompletePromiseTest003 no peer device :e:' + `, error code is ${error.code}, message is ${error.message}`);
            }
        } catch(e) {
            console.error('DeviceKvStoreOnSyncCompletePromiseTest003 put e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreOffChangePromiseTest001
     * @tc.desc Test Js Api DeviceKvStore.OffChange() testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreOffChangePromiseTest001', 0, async function (done) {
        console.info('DeviceKvStoreOffChangePromiseTest001');
        try {
            var func = function (data) {
                console.info('DeviceKvStoreOffChangePromiseTest001 0' + data)
            };
            kvStore.on('dataChange', 0, func);
            kvStore.off('dataChange', func);
        }catch(e) {
            console.error('DeviceKvStoreOffChangePromiseTest001 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreOffChangePromiseTest002
     * @tc.desc Test Js Api DeviceKvStore.OffChange() testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreOffChangePromiseTest002', 0, async function (done) {
        console.info('DeviceKvStoreOffChangePromiseTest002');
        try {
            var func = function (data) {
                console.info('DeviceKvStoreOffChangePromiseTest002 0' + data)
            };
            var func1 = function (data) {
                console.info('DeviceKvStoreOffChangePromiseTest002 0' + data)
            };
            kvStore.on('dataChange', 0, func);
            kvStore.on('dataChange', 0, func1);
            kvStore.off('dataChange', func);
        }catch(e) {
            console.error('DeviceKvStoreOffChangePromiseTest002 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreOffChangePromiseTest003
     * @tc.desc Test Js Api DeviceKvStore.OffChange() testcase 003
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreOffChangePromiseTest003', 0, async function (done) {
        console.info('DeviceKvStoreOffChangePromiseTest003');
        try {
            var func = function (data) {
                console.info('DeviceKvStoreOffChangePromiseTest003 0' + data)
            };
            var func1 = function (data) {
                console.info('DeviceKvStoreOffChangePromiseTest003 0' + data)
            };
            kvStore.on('dataChange', 0, func);
            kvStore.on('dataChange', 0, func1);
            kvStore.off('dataChange', func);
            kvStore.off('dataChange', func1);
        }catch(e) {
            console.error('DeviceKvStoreOffChangePromiseTest003 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreOffChangePromiseTest004
     * @tc.desc Test Js Api DeviceKvStore.OffChange() testcase 004
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreOffChangePromiseTest004', 0, async function (done) {
        console.info('DeviceKvStoreOffChangePromiseTest004');
        try {
            var func = function (data) {
                console.info('DeviceKvStoreOffChangePromiseTest004 0' + data)
            };
            kvStore.on('dataChange', 0, func);
            kvStore.off('dataChange');
        }catch(e) {
            console.error('DeviceKvStoreOffChangePromiseTest004 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreOffSyncCompletePromiseTest001
     * @tc.desc Test Js Api DeviceKvStore.OffSyncComplete() testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreOffSyncCompletePromiseTest001', 0, async function (done) {
        console.info('DeviceKvStoreOffSyncCompletePromiseTest001');
        try {
            var func = function (data) {
                console.info('DeviceKvStoreOffSyncCompletePromiseTest001 0' + data)
            };
            kvStore.on('syncComplete', func);
            kvStore.off('syncComplete', func);
        }catch(e) {
            console.error('DeviceKvStoreOffSyncCompletePromiseTest001 put e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreOffSyncCompletePromiseTest002
     * @tc.desc Test Js Api DeviceKvStore.OffSyncComplete() testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreOffSyncCompletePromiseTest002', 0, async function (done) {
        console.info('DeviceKvStoreOffSyncCompletePromiseTest002');
        try {
            var func = function (data) {
                console.info('DeviceKvStoreOffSyncCompletePromiseTest002 0' + data)
            };
            var func1 = function (data) {
                console.info('DeviceKvStoreOffSyncCompletePromiseTest002 0' + data)
            };
            kvStore.on('syncComplete', func);
            kvStore.on('syncComplete', func1);
            kvStore.off('syncComplete', func);
        }catch(e) {
            console.error('DeviceKvStoreOffSyncCompletePromiseTest002 put e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreOffSyncCompletePromiseTest003
     * @tc.desc Test Js Api DeviceKvStore.OffSyncComplete() testcase 003
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreOffSyncCompletePromiseTest003', 0, async function (done) {
        console.info('DeviceKvStoreOffSyncCompletePromiseTest003');
        try {
            var func = function (data) {
                console.info('DeviceKvStoreOffSyncCompletePromiseTest003 0' + data)
            };
            var func1 = function (data) {
                console.info('DeviceKvStoreOffSyncCompletePromiseTest003 0' + data)
            };
            kvStore.on('syncComplete', func);
            kvStore.on('syncComplete', func1);
            kvStore.off('syncComplete', func);
            kvStore.off('syncComplete', func1);
        }catch(e) {
            console.error('DeviceKvStoreOffSyncCompletePromiseTest003 put e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreOffSyncCompletePromiseTest004
     * @tc.desc Test Js Api DeviceKvStore.OffSyncComplete() testcase 004
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreOffSyncCompletePromiseTest004', 0, async function (done) {
        console.info('DeviceKvStoreOffSyncCompletePromiseTest004');
        try {
            var func = function (data) {
                console.info('DeviceKvStoreOffSyncCompletePromiseTest004 0' + data)
            };
            kvStore.on('syncComplete', func);
            kvStore.off('syncComplete');
        }catch(e) {
            console.error('DeviceKvStoreOffSyncCompletePromiseTest004 put e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreSetSyncRangePromiseTest001
     * @tc.desc Test Js Api DeviceKvStore.SetSyncRange() testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreSetSyncRangePromiseTest001', 0, async function (done) {
        console.info('DeviceKvStoreSetSyncRangePromiseTest001');
        try {
            var localLabels = ['A', 'B'];
            var remoteSupportLabels = ['C', 'D'];
            await kvStore.setSyncRange(localLabels, remoteSupportLabels).then((err) => {
                console.info('DeviceKvStoreSetSyncRangePromiseTest001 setSyncRange success');
                expect(err == undefined).assertTrue();
            }).catch((err) => {
                console.error('DeviceKvStoreDeleteStringPromiseTest003 delete fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        }catch(e) {
            console.error('DeviceKvStoreSetSyncRangePromiseTest001 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreSetSyncRangePromiseTest002
     * @tc.desc Test Js Api DeviceKvStore.SetSyncRange() testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreSetSyncRangePromiseTest002', 0, async function (done) {
        console.info('DeviceKvStoreSetSyncRangePromiseTest002');
        try {
            var localLabels = ['A', 'B'];
            var remoteSupportLabels = ['B', 'C'];
            await kvStore.setSyncRange(localLabels, remoteSupportLabels).then((err) => {
                console.info('DeviceKvStoreSetSyncRangePromiseTest002 setSyncRange success');
                expect(err == undefined).assertTrue();
            }).catch((err) => {
                console.error('DeviceKvStoreSetSyncRangePromiseTest002 delete fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        }catch(e) {
            console.error('DeviceKvStoreSetSyncRangePromiseTest002 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreSetSyncRangePromiseTest003
     * @tc.desc Test Js Api DeviceKvStore.SetSyncRange() testcase 003
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreSetSyncRangePromiseTest003', 0, async function (done) {
        console.info('DeviceKvStoreSetSyncRangePromiseTest003');
        try {
            var localLabels = ['A', 'B'];
            var remoteSupportLabels = ['A', 'B'];
            await kvStore.setSyncRange(localLabels, remoteSupportLabels).then((err) => {
                console.info('DeviceKvStoreSetSyncRangePromiseTest003 setSyncRange success');
                expect(err == undefined).assertTrue();
            }).catch((err) => {
                console.error('DeviceKvStoreSetSyncRangePromiseTest003 delete fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        }catch(e) {
            console.error('DeviceKvStoreSetSyncRangePromiseTest003 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStorePutPromiseTest001
     * @tc.desc Test Js Api DeviceKvStore.Put(Batch) testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStorePutBatchPromiseTest001', 0, async function (done) {
        console.info('DeviceKvStorePutBatchPromiseTest001');
        try {
            let entries = putBatchString(10, 'batch_test_string_key');
            console.info('DeviceKvStorePutBatchPromiseTest001 entries: ' + JSON.stringify(entries));
            await kvStore.putBatch(entries).then(async (err) => {
                console.info('DeviceKvStorePutBatchPromiseTest001 putBatch success');
                expect(err == undefined).assertTrue();
                await kvStore.getEntries(localDeviceId, 'batch_test_string_key').then((entrys) => {
                    console.info('DeviceKvStorePutBatchPromiseTest001 getEntries success');
                    console.info('DeviceKvStorePutBatchPromiseTest001 entrys.length: ' + entrys.length);
                    console.info('DeviceKvStorePutBatchPromiseTest001 entrys[0]: ' + JSON.stringify(entrys[0]));
                    console.info('DeviceKvStorePutBatchPromiseTest001 entrys[0].value: ' + JSON.stringify(entrys[0].value));
                    console.info('DeviceKvStorePutBatchPromiseTest001 entrys[0].value.value: ' + entrys[0].value.value);
                    expect(entrys.length == 10).assertTrue();
                    expect(entrys[0].value.value == 'batch_test_string_value').assertTrue();
                }).catch((err) => {
                    console.error('DeviceKvStorePutBatchPromiseTest001 getEntries fail ' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(null).assertFail();
                });
            }).catch((err) => {
                console.error('DeviceKvStorePutBatchPromiseTest001 putBatch fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        }catch(e) {
            console.error('DeviceKvStorePutBatchPromiseTest001 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStorePutPromiseTest002
     * @tc.desc Test Js Api DeviceKvStore.Put(Batch) testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStorePutBatchPromiseTest002', 0, async function (done) {
        console.info('DeviceKvStorePutBatchPromiseTest002');
        try {
            let entries = [];
            for (var i = 0; i < 10; i++) {
                var key = 'batch_test_number_key';
                var entry = {
                    key : key + i,
                    value : {
                        type : factory.ValueType.INTEGER,
                        value : 222
                    }
                }
                entries.push(entry);
            }
            console.info('DeviceKvStorePutBatchPromiseTest002 entries: ' + JSON.stringify(entries));
            await kvStore.putBatch(entries).then(async (err) => {
                console.info('DeviceKvStorePutBatchPromiseTest002 putBatch success');
                expect(err == undefined).assertTrue();
                await kvStore.getEntries(localDeviceId, 'batch_test_number_key').then((entrys) => {
                    console.info('DeviceKvStorePutBatchPromiseTest002 getEntries success');
                    expect(entrys.length == 10).assertTrue();
                    expect(entrys[0].value.value == 222).assertTrue();
                }).catch((err) => {
                    console.error('DeviceKvStorePutBatchPromiseTest002 getEntries fail ' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(null).assertFail();
                });
            }).catch((err) => {
                console.error('DeviceKvStorePutBatchPromiseTest002 putBatch fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        }catch(e) {
            console.error('DeviceKvStorePutBatchPromiseTest002 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStorePutPromiseTest003
     * @tc.desc Test Js Api DeviceKvStore.Put(Batch) testcase 003
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStorePutBatchPromiseTest003', 0, async function (done) {
        console.info('DeviceKvStorePutBatchPromiseTest003');
        try {
            let entries = [];
            for (var i = 0; i < 10; i++) {
                var key = 'batch_test_number_key';
                var entry = {
                    key : key + i,
                    value : {
                        type : factory.ValueType.FLOAT,
                        value : 2.0
                    }
                }
                entries.push(entry);
            }
            console.info('DeviceKvStorePutBatchPromiseTest003 entries: ' + JSON.stringify(entries));
            await kvStore.putBatch(entries).then(async (err) => {
                console.info('DeviceKvStorePutBatchPromiseTest003 putBatch success');
                expect(err == undefined).assertTrue();
                await kvStore.getEntries(localDeviceId, 'batch_test_number_key').then((entrys) => {
                    console.info('DeviceKvStorePutBatchPromiseTest003 getEntries success');
                    expect(entrys.length == 10).assertTrue();
                    expect(entrys[0].value.value == 2.0).assertTrue();
                }).catch((err) => {
                    console.error('DeviceKvStorePutBatchPromiseTest003 getEntries fail ' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(null).assertFail();
                });
            }).catch((err) => {
                console.error('DeviceKvStorePutBatchPromiseTest003 putBatch fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        }catch(e) {
            console.error('DeviceKvStorePutBatchPromiseTest003 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStorePutPromiseTest004
     * @tc.desc Test Js Api DeviceKvStore.Put(Batch) testcase 004
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStorePutBatchPromiseTest004', 0, async function (done) {
        console.info('DeviceKvStorePutBatchPromiseTest004');
        try {
            let entries = [];
            for (var i = 0; i < 10; i++) {
                var key = 'batch_test_number_key';
                var entry = {
                    key : key + i,
                    value : {
                        type : factory.ValueType.DOUBLE,
                        value : 2.00
                    }
                }
                entries.push(entry);
            }
            console.info('DeviceKvStorePutBatchPromiseTest004 entries: ' + JSON.stringify(entries));
            await kvStore.putBatch(entries).then(async (err) => {
                console.info('DeviceKvStorePutBatchPromiseTest004 putBatch success');
                expect(err == undefined).assertTrue();
                await kvStore.getEntries(localDeviceId, 'batch_test_number_key').then((entrys) => {
                    console.info('DeviceKvStorePutBatchPromiseTest004 getEntries success');
                    expect(entrys.length == 10).assertTrue();
                    expect(entrys[0].value.value == 2.00).assertTrue();
                }).catch((err) => {
                    console.error('DeviceKvStorePutBatchPromiseTest004 getEntries fail ' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(null).assertFail();
                });
            }).catch((err) => {
                console.error('DeviceKvStorePutBatchPromiseTest004 putBatch fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        }catch(e) {
            console.error('DeviceKvStorePutBatchPromiseTest004 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStorePutPromiseTest005
     * @tc.desc Test Js Api DeviceKvStore.Put(Batch) testcase 005
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStorePutBatchPromiseTest005', 0, async function (done) {
        console.info('DeviceKvStorePutBatchPromiseTest005');
        try {
            var bo = false;
            let entries = [];
            for (var i = 0; i < 10; i++) {
                var key = 'batch_test_bool_key';
                var entry = {
                    key : key + i,
                    value : {
                        type : factory.ValueType.BOOLEAN,
                        value : bo
                    }
                }
                entries.push(entry);
            }
            console.info('DeviceKvStorePutBatchPromiseTest005 entries: ' + JSON.stringify(entries));
            await kvStore.putBatch(entries).then(async (err) => {
                console.info('DeviceKvStorePutBatchPromiseTest005 putBatch success');
                expect(err == undefined).assertTrue();
                await kvStore.getEntries(localDeviceId, 'batch_test_bool_key').then((entrys) => {
                    console.info('DeviceKvStorePutBatchPromiseTest005 getEntries success');
                    expect(entrys.length == 10).assertTrue();
                    expect(entrys[0].value.value == bo).assertTrue();
                }).catch((err) => {
                    console.error('DeviceKvStorePutBatchPromiseTest005 getEntries fail ' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(null).assertFail();
                });
            }).catch((err) => {
                console.error('DeviceKvStorePutBatchPromiseTest005 putBatch fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        }catch(e) {
            console.error('DeviceKvStorePutBatchPromiseTest005 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStorePutPromiseTest006
     * @tc.desc Test Js Api DeviceKvStore.Put(Batch) testcase 006
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStorePutBatchPromiseTest006', 0, async function (done) {
        console.info('DeviceKvStorePutBatchPromiseTest006');
        try {
            var arr = new Uint8Array([21,31]);
            let entries = [];
            for (var i = 0; i < 10; i++) {
                var key = 'batch_test_bool_key';
                var entry = {
                    key : key + i,
                    value : {
                        type : factory.ValueType.BYTE_ARRAY,
                        value : arr
                    }
                }
                entries.push(entry);
            }
            console.info('DeviceKvStorePutBatchPromiseTest006 entries: ' + JSON.stringify(entries));
            await kvStore.putBatch(entries).then(async (err) => {
                console.info('DeviceKvStorePutBatchPromiseTest006 putBatch success');
                expect(err == undefined).assertTrue();
                await kvStore.getEntries(localDeviceId, 'batch_test_bool_key').then((entrys) => {
                    console.info('DeviceKvStorePutBatchPromiseTest006 getEntries success');
                    expect(entrys.length == 10).assertTrue();
                    expect(entrys[0].value.value.toString() == arr.toString()).assertTrue();
                }).catch((err) => {
                    console.error('DeviceKvStorePutBatchPromiseTest006 getEntries fail ' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(null).assertFail();
                });
            }).catch((err) => {
                console.error('DeviceKvStorePutBatchPromiseTest006 putBatch fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        }catch(e) {
            console.error('DeviceKvStorePutBatchPromiseTest005 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreDeleteBatchPromiseTest001
     * @tc.desc Test Js Api DeviceKvStore.DeleteBatch() testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreDeleteBatchPromiseTest001', 0, async function (done) {
        console.info('DeviceKvStoreDeleteBatchPromiseTest001');
        try {
            let entries = [];
            let keys = [];
            for (var i = 0; i < 5; i++) {
                var key = 'batch_test_string_key';
                var entry = {
                    key : key + i,
                    value : {
                        type : factory.ValueType.STRING,
                        value : 'batch_test_string_value'
                    }
                }
                entries.push(entry);
                keys.push(key + i);
            }
            console.info('DeviceKvStoreDeleteBatchPromiseTest001 entries: ' + JSON.stringify(entries));
            await kvStore.putBatch(entries).then(async (err) => {
                console.info('DeviceKvStoreDeleteBatchPromiseTest001 putBatch success');
                expect(err == undefined).assertTrue();
                await kvStore.deleteBatch(keys).then((err) => {
                    console.info('DeviceKvStoreDeleteBatchPromiseTest001 deleteBatch success');
                    expect(err == undefined).assertTrue();
                }).catch((err) => {
                    console.error('DeviceKvStoreDeleteBatchPromiseTest001 deleteBatch fail ' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(null).assertFail();
                });
            }).catch((err) => {
                console.error('DeviceKvStoreDeleteBatchPromiseTest001 putBatch fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        }catch(e) {
            console.error('DeviceKvStoreDeleteBatchPromiseTest001 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreDeleteBatchPromiseTest002
     * @tc.desc Test Js Api DeviceKvStore.DeleteBatch() testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreDeleteBatchPromiseTest002', 0, async function (done) {
        console.info('DeviceKvStoreDeleteBatchPromiseTest002');
        try {
            let keys = ['batch_test_string_key1', 'batch_test_string_key2'];
            await kvStore.deleteBatch(keys).then((err) => {
                console.info('DeviceKvStoreDeleteBatchPromiseTest002 deleteBatch success');
            }).catch((err) => {
                console.error('DeviceKvStoreDeleteBatchPromiseTest002 deleteBatch fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        }catch(e) {
            console.error('DeviceKvStoreDeleteBatchPromiseTest002 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreDeleteBatchPromiseTest003
     * @tc.desc Test Js Api DeviceKvStore.DeleteBatch() testcase 003
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreDeleteBatchPromiseTest003', 0, async function (done) {
        console.info('DeviceKvStoreDeleteBatchPromiseTest003');
        try {
            let entries = [];
            for (var i = 0; i < 10; i++) {
                var key = 'batch_test_string_key';
                var entry = {
                    key : key + i,
                    value : {
                        type : factory.ValueType.STRING,
                        value : 'batch_test_string_value'
                    }
                }
                entries.push(entry);
            }
            console.info('DeviceKvStoreDeleteBatchPromiseTest003 entries: ' + JSON.stringify(entries));
            await kvStore.putBatch(entries).then(async (err) => {
                console.info('DeviceKvStoreDeleteBatchPromiseTest003 putBatch success');
                expect(err == undefined).assertTrue();
                let keys = ['batch_test_string_key1', 'batch_test_string_keya'];
                await kvStore.deleteBatch(keys).then((err) => {
                    console.info('DeviceKvStoreDeleteBatchPromiseTest003 deleteBatch success');
                }).catch((err) => {
                    console.error('DeviceKvStoreDeleteBatchPromiseTest003 deleteBatch fail ' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(null).assertFail();
                });
            }).catch((err) => {
                console.error('DeviceKvStoreDeleteBatchPromiseTest003 putBatch fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        }catch(e) {
            console.error('DeviceKvStoreDeleteBatchPromiseTest003 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStorestartTransactionPromiseTest001
     * @tc.desc Test Js Api DeviceKvStore.startTransaction() testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStorestartTransactionPromiseTest001', 0, async function (done) {
        console.info('DeviceKvStorestartTransactionPromiseTest001');
        try {
            var count = 0;
            kvStore.on('dataChange', 0, function (data) {
                console.info('DeviceKvStorestartTransactionPromiseTest001' + JSON.stringify(data))
                count++;
            });
            await kvStore.startTransaction().then(async (err) => {
                console.info('DeviceKvStorestartTransactionPromiseTest001 startTransaction success');
                expect(err == undefined).assertTrue();
            }).catch((err) => {
                console.error('DeviceKvStorestartTransactionPromiseTest001 startTransaction fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
            let entries = putBatchString(10, 'batch_test_string_key');
            console.info('DeviceKvStorestartTransactionPromiseTest001 entries: ' + JSON.stringify(entries));
            await kvStore.putBatch(entries).then(async (err) => {
                console.info('DeviceKvStorestartTransactionPromiseTest001 putBatch success');
                expect(err == undefined).assertTrue();
            }).catch((err) => {
                console.error('DeviceKvStorestartTransactionPromiseTest001 putBatch fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
            let keys = Object.keys(entries).slice(5); //delete 5 beginnings
            await kvStore.deleteBatch(keys).then((err) => {
                console.info('DeviceKvStorestartTransactionPromiseTest001 deleteBatch success');
                expect(err == undefined).assertTrue();
            }).catch((err) => {
                console.error('DeviceKvStorestartTransactionPromiseTest001 deleteBatch fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
            await kvStore.commit().then(async (err) => {
                console.info('DeviceKvStorestartTransactionPromiseTest001 commit success');
                expect(err == undefined).assertTrue();
            }).catch((err) => {
                console.error('DeviceKvStorestartTransactionPromiseTest001 commit fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
            await sleep(2000);
            expect(count == 1).assertTrue();
        }catch(e) {
            console.error('DeviceKvStorestartTransactionPromiseTest001 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStorestartTransactionPromiseTest002
     * @tc.desc Test Js Api DeviceKvStore.startTransaction() testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStorestartTransactionPromiseTest002', 0, async function (done) {
        console.info('DeviceKvStorestartTransactionPromiseTest002');
        try {
            var count = 0;
            kvStore.on('dataChange', 0, function (data) {
                console.info('DeviceKvStorestartTransactionPromiseTest002' + JSON.stringify(data))
                count++;
            });
            await kvStore.startTransaction().then(async (err) => {
                console.info('DeviceKvStorestartTransactionPromiseTest002 startTransaction success');
                expect(err == undefined).assertTrue();
            }).catch((err) => {
                console.error('DeviceKvStorestartTransactionPromiseTest002 startTransaction fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
            let entries = putBatchString(10, 'batch_test_string_key');
            console.info('DeviceKvStorestartTransactionPromiseTest002 entries: ' + JSON.stringify(entries));
            await kvStore.putBatch(entries).then(async (err) => {
                console.info('DeviceKvStorestartTransactionPromiseTest002 putBatch success');
                expect(err == undefined).assertTrue();
            }).catch((err) => {
                console.error('DeviceKvStorestartTransactionPromiseTest002 putBatch fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
            let keys = Object.keys(entries).slice(5); //delete 5 beginnings
            await kvStore.deleteBatch(keys).then((err) => {
                console.info('DeviceKvStorestartTransactionPromiseTest002 deleteBatch success');
                expect(err == undefined).assertTrue();
            }).catch((err) => {
                console.error('DeviceKvStorestartTransactionPromiseTest002 deleteBatch fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
            await kvStore.rollback().then(async (err) => {
                console.info('DeviceKvStorestartTransactionPromiseTest002 rollback success');
                expect(err == undefined).assertTrue();
            }).catch((err) => {
                console.error('DeviceKvStorestartTransactionPromiseTest002 rollback fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
            await sleep(2000);
            expect(count == 0).assertTrue();
        }catch(e) {
            console.error('DeviceKvStorestartTransactionPromiseTest002 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStorestartTransactionPromiseTest003
     * @tc.desc Test Js Api DeviceKvStore.startTransaction() testcase 003
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStorestartTransactionPromiseTest003', 0, async function (done) {
        console.info('DeviceKvStorestartTransactionPromiseTest002');
        try {
            await kvStore.startTransaction(1).then(async (err) => {
                console.info('DeviceKvStorestartTransactionPromiseTest003 startTransaction success');
                expect(null).assertFail();
            }).catch((err) => {
                console.error('DeviceKvStorestartTransactionPromiseTest003 startTransaction fail ' + `, error code is ${err.code}, message is ${err.message}`);
            });
        }catch(e) {
            console.error('DeviceKvStorestartTransactionPromiseTest003 e ' + `, error code is ${e.code}, message is ${e.message}`);
        }
        done();
    })

    /**
     * @tc.name DeviceKvStorestartTransactionPromiseTest004
     * @tc.desc Test Js Api DeviceKvStore.startTransaction() testcase 004
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStorestartTransactionPromiseTest004', 0, async function (done) {
        console.info('DeviceKvStorestartTransactionPromiseTest004');
        try {
            await kvStore.startTransaction('test_string').then(async (err) => {
                console.info('DeviceKvStorestartTransactionPromiseTest004 startTransaction success');
                expect(null).assertFail();
            }).catch((err) => {
                console.error('DeviceKvStorestartTransactionPromiseTest004 startTransaction fail ' + `, error code is ${err.code}, message is ${err.message}`);
            });
        }catch(e) {
            console.error('DeviceKvStorestartTransactionPromiseTest004 e ' + `, error code is ${e.code}, message is ${e.message}`);
        }
        done();
    })

    /**
     * @tc.name DeviceKvStorestartTransactionPromiseTest005
     * @tc.desc Test Js Api DeviceKvStore.startTransaction() testcase 005
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStorestartTransactionPromiseTest005', 0, async function (done) {
        console.info('DeviceKvStorestartTransactionPromiseTest005');
        try {
            await kvStore.startTransaction(2.000).then(async (err) => {
                console.info('DeviceKvStorestartTransactionPromiseTest005 startTransaction success');
                expect(null).assertFail();
            }).catch((err) => {
                console.error('DeviceKvStorestartTransactionPromiseTest005 startTransaction fail ' + `, error code is ${err.code}, message is ${err.message}`);
            });
        }catch(e) {
            console.error('DeviceKvStorestartTransactionPromiseTest005 e ' + `, error code is ${e.code}, message is ${e.message}`);
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreCommitPromiseTest001
     * @tc.desc Test Js Api DeviceKvStore.Commit() testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreCommitPromiseTest001', 0, async function (done) {
        console.info('DeviceKvStoreCommitPromiseTest001');
        try {
            await kvStore.commit(1).then(async (err) => {
                console.info('DeviceKvStoreCommitPromiseTest001 commit success');
                expect(null).assertFail();
            }).catch((err) => {
                console.error('DeviceKvStoreCommitPromiseTest001 commit fail ' + `, error code is ${err.code}, message is ${err.message}`);
            });
        }catch(e) {
            console.error('DeviceKvStoreCommitPromiseTest001 e ' + `, error code is ${e.code}, message is ${e.message}`);
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreCommitPromiseTest002
     * @tc.desc Test Js Api DeviceKvStore.Commit() testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreCommitPromiseTest002', 0, async function (done) {
        console.info('DeviceKvStoreCommitPromiseTest002');
        try {
            await kvStore.commit('test_string').then(async (err) => {
                console.info('DeviceKvStoreCommitPromiseTest002 commit success');
                expect(null).assertFail();
            }).catch((err) => {
                console.error('DeviceKvStoreCommitPromiseTest002 commit fail ' + `, error code is ${err.code}, message is ${err.message}`);
            });
        }catch(e) {
            console.error('DeviceKvStoreCommitPromiseTest002 e ' + `, error code is ${e.code}, message is ${e.message}`);
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreCommitPromiseTest003
     * @tc.desc Test Js Api DeviceKvStore.Commit() testcase 003
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreCommitPromiseTest003', 0, async function (done) {
        console.info('DeviceKvStoreCommitPromiseTest003');
        try {
            await kvStore.commit(2.000).then(async (err) => {
                console.info('DeviceKvStoreCommitPromiseTest003 commit success');
                expect(null).assertFail();
            }).catch((err) => {
                console.error('DeviceKvStoreCommitPromiseTest003 commit fail ' + `, error code is ${err.code}, message is ${err.message}`);
            });
        }catch(e) {
            console.error('DeviceKvStoreCommitPromiseTest003 e ' + `, error code is ${e.code}, message is ${e.message}`);
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreRollbackPromiseTest001
     * @tc.desc Test Js Api DeviceKvStore.Rollback() testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreRollbackPromiseTest001', 0, async function (done) {
        console.info('DeviceKvStoreRollbackPromiseTest001');
        try {
            await kvStore.rollback(1).then(async (err) => {
                console.info('DeviceKvStoreRollbackPromiseTest001 rollback success');
                expect(null).assertFail();
            }).catch((err) => {
                console.error('DeviceKvStoreRollbackPromiseTest001 rollback fail ' + `, error code is ${err.code}, message is ${err.message}`);
            });
        }catch(e) {
            console.error('DeviceKvStoreRollbackPromiseTest001 e ' + `, error code is ${e.code}, message is ${e.message}`);
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreRollbackPromiseTest002
     * @tc.desc Test Js Api DeviceKvStore.Rollback() testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreRollbackPromiseTest002', 0, async function (done) {
        console.info('DeviceKvStoreRollbackPromiseTest002');
        try {
            await kvStore.rollback('test_string').then(async (err) => {
                console.info('DeviceKvStoreRollbackPromiseTest002 rollback success');
                expect(null).assertFail();
            }).catch((err) => {
                console.error('DeviceKvStoreRollbackPromiseTest002 rollback fail ' + `, error code is ${err.code}, message is ${err.message}`);
            });
        }catch(e) {
            console.error('DeviceKvStoreRollbackPromiseTest002 e ' + `, error code is ${e.code}, message is ${e.message}`);
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreRollbackPromiseTest003
     * @tc.desc Test Js Api DeviceKvStore.Rollback() testcase 003
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreRollbackPromiseTest003', 0, async function (done) {
        console.info('DeviceKvStoreRollbackPromiseTest003');
        try {
            await kvStore.rollback(2.000).then(async (err) => {
                console.info('DeviceKvStoreRollbackPromiseTest003 rollback success');
                expect(null).assertFail();
            }).catch((err) => {
                console.error('DeviceKvStoreRollbackPromiseTest003 rollback fail ' + `, error code is ${err.code}, message is ${err.message}`);
            });
        }catch(e) {
            console.error('DeviceKvStoreRollbackPromiseTest003 e ' + `, error code is ${e.code}, message is ${e.message}`);
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreEnableSyncPromiseTest001
     * @tc.desc Test Js Api DeviceKvStore.EnableSync() testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreEnableSyncPromiseTest001', 0, async function (done) {
        console.info('DeviceKvStoreEnableSyncPromiseTest001');
        try {
            await kvStore.enableSync(true).then((err) => {
                console.info('DeviceKvStoreEnableSyncPromiseTest001 enableSync success');
                expect(err == undefined).assertTrue();
            }).catch((err) => {
                console.error('DeviceKvStoreEnableSyncPromiseTest001 enableSync fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        }catch(e) {
            console.error('DeviceKvStoreEnableSyncPromiseTest001 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreEnableSyncPromiseTest002
     * @tc.desc Test Js Api DeviceKvStore.EnableSync() testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreEnableSyncPromiseTest002', 0, async function (done) {
        console.info('DeviceKvStoreEnableSyncPromiseTest002');
        try {
            await kvStore.enableSync(false).then((err) => {
                console.info('DeviceKvStoreEnableSyncPromiseTest002 enableSync success');
                expect(err == undefined).assertTrue();
            }).catch((err) => {
                console.error('DeviceKvStoreEnableSyncPromiseTest002 enableSync fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        }catch(e) {
            console.error('DeviceKvStoreEnableSyncPromiseTest002 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreEnableSyncPromiseTest003
     * @tc.desc Test Js Api DeviceKvStore.EnableSync() testcase 003
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreEnableSyncPromiseTest003', 0, async function (done) {
        console.info('DeviceKvStoreEnableSyncPromiseTest003');
        try {
            await kvStore.enableSync().then((err) => {
                console.info('DeviceKvStoreEnableSyncPromiseTest003 enableSync success');
                expect(null).assertFail();
            }).catch((err) => {
                console.error('DeviceKvStoreEnableSyncPromiseTest003 enableSync fail ' + `, error code is ${err.code}, message is ${err.message}`);
            });
        }catch(e) {
            console.error('DeviceKvStoreEnableSyncPromiseTest003 e ' + `, error code is ${e.code}, message is ${e.message}`);
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreEnableSyncPromiseTest004
     * @tc.desc Test Js Api DeviceKvStore.EnableSync() testcase 004
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreEnableSyncPromiseTest004', 0, async function (done) {
        console.info('DeviceKvStoreEnableSyncPromiseTest004');
        try {
            await kvStore.enableSync(null).then((err) => {
                console.info('DeviceKvStoreEnableSyncPromiseTest004 enableSync success');
                expect(null).assertFail();
            }).catch((err) => {
                console.error('DeviceKvStoreEnableSyncPromiseTest004 enableSync fail ' + `, error code is ${err.code}, message is ${err.message}`);
            });
        }catch(e) {
            console.error('DeviceKvStoreEnableSyncPromiseTest004 e ' + `, error code is ${e.code}, message is ${e.message}`);
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreRemoveDeviceDataPromiseTest001
     * @tc.desc Test Js Api DeviceKvStore.RemoveDeviceData() testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreRemoveDeviceDataPromiseTest001', 0, async function (done) {
        console.info('DeviceKvStoreRemoveDeviceDataPromiseTest001');
        try {
            await kvStore.put(KEY_TEST_STRING_ELEMENT, VALUE_TEST_STRING_ELEMENT).then((err) => {
                console.info('DeviceKvStoreRemoveDeviceDataPromiseTest001 put success');
                expect(err == undefined).assertTrue();
            }).catch((err) => {
                console.error('DeviceKvStoreRemoveDeviceDataPromiseTest001 put fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
            var deviceid = 'no_exist_device_id';
            await kvStore.removeDeviceData(deviceid).then((err) => {
                console.info('DeviceKvStoreRemoveDeviceDataPromiseTest001 removeDeviceData success');
                expect(null).assertFail();
            }).catch((err) => {
                console.error('DeviceKvStoreRemoveDeviceDataPromiseTest001 removeDeviceData fail ' + `, error code is ${err.code}, message is ${err.message}`);
            });
            await kvStore.get(localDeviceId, KEY_TEST_STRING_ELEMENT).then((data) => {
                console.info('DeviceKvStoreRemoveDeviceDataPromiseTest001 get success data:' + data);
                expect(data == VALUE_TEST_STRING_ELEMENT).assertTrue();
            }).catch((err) => {
                console.error('DeviceKvStoreRemoveDeviceDataPromiseTest001 get fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        }catch(e) {
            console.error('DeviceKvStoreRemoveDeviceDataPromiseTest001 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreRemoveDeviceDataPromiseTest002
     * @tc.desc Test Js Api DeviceKvStore.RemoveDeviceData() testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreRemoveDeviceDataPromiseTest002', 0, async function (done) {
        console.info('DeviceKvStoreRemoveDeviceDataPromiseTest002');
        try {
            await kvStore.removeDeviceData().then((err) => {
                console.info('DeviceKvStoreRemoveDeviceDataPromiseTest002 removeDeviceData success');
                expect(null).assertFail();
            }).catch((err) => {
                console.error('DeviceKvStoreRemoveDeviceDataPromiseTest002 removeDeviceData fail ' + `, error code is ${err.code}, message is ${err.message}`);
            });
        }catch(e) {
            console.error('DeviceKvStoreRemoveDeviceDataPromiseTest002 e ' + `, error code is ${e.code}, message is ${e.message}`);
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreRemoveDeviceDataPromiseTest003
     * @tc.desc Test Js Api DeviceKvStore.RemoveDeviceData() testcase 003
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
     it('DeviceKvStoreRemoveDeviceDataPromiseTest003', 0, async function (done) {
        console.info('DeviceKvStoreRemoveDeviceDataPromiseTest003');
        try {
            await kvStore.removeDeviceData('').then((data) => {
                console.info('DeviceKvStoreRemoveDeviceDataPromiseTest003 removeDeviceData success');
                expect(null).assertFail();
            }).catch((err) => {
                console.error('DeviceKvStoreRemoveDeviceDataPromiseTest003 removeDeviceData fail ' + `, error code is ${err.code}, message is ${err.message}`);
            });
        }catch(e) {
            console.error('DeviceKvStoreRemoveDeviceDataPromiseTest003 e ' + `, error code is ${e.code}, message is ${e.message}`);
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreRemoveDeviceDataPromiseTest004
     * @tc.desc Test Js Api DeviceKvStore.RemoveDeviceData() testcase 004
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreRemoveDeviceDataPromiseTest004', 0, async function (done) {
        console.info('DeviceKvStoreRemoveDeviceDataPromiseTest004');
        try {
            await kvStore.removeDeviceData(null).then((data) => {
                console.info('DeviceKvStoreRemoveDeviceDataPromiseTest004 removeDeviceData success');
                expect(null).assertFail();
            }).catch((err) => {
                console.error('DeviceKvStoreRemoveDeviceDataPromiseTest004 removeDeviceData fail ' + `, error code is ${err.code}, message is ${err.message}`);
            });
        }catch(e) {
            console.error('DeviceKvStoreRemoveDeviceDataPromiseTest004 e ' + `, error code is ${e.code}, message is ${e.message}`);
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoregetResultSetPromiseTest001
     * @tc.desc Test Js Api DeviceKvStore.getResultSet() testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreGetResultSetPromiseTest001', 0, async function (done) {
        console.info('DeviceKvStoreGetResultSetPromiseTest001');
        try {
            let resultSet;
            let entries = [];
            for (var i = 0; i < 10; i++) {
                var key = 'batch_test_string_key';
                var entry = {
                    key : key + i,
                    value : {
                        type : factory.ValueType.STRING,
                        value : 'batch_test_string_value'
                    }
                }
                entries.push(entry);
            }
            await kvStore.putBatch(entries).then(async (err) => {
                console.info('DeviceKvStoreGetResultSetPromiseTest001 putBatch success');
                expect(err == undefined).assertTrue();
            }).catch((err) => {
                console.error('DeviceKvStorePutBatchPromiseTest001 putBatch fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
            await kvStore.getResultSet(localDeviceId, 'batch_test_string_key').then((result) => {
                console.info('DeviceKvStoreGetResultSetPromiseTest001 getResultSet success');
                resultSet = result;
                expect(resultSet.getCount() == 10).assertTrue();
            }).catch((err) => {
                console.error('DeviceKvStoreGetResultSetPromiseTest001 getResultSet fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
            await kvStore.closeResultSet(resultSet).then((err) => {
                console.info('DeviceKvStoreGetResultSetPromiseTest001 closeResultSet success');
                expect(err == undefined).assertTrue();
            }).catch((err) => {
                console.error('DeviceKvStoreGetResultSetPromiseTest001 closeResultSet fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        }catch(e) {
            console.error('DeviceKvStoreGetResultSetPromiseTest001 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoregetResultSetPromiseTest002
     * @tc.desc Test Js Api DeviceKvStore.getResultSet() testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreGetResultSetPromiseTest002', 0, async function (done) {
        console.info('DeviceKvStoreGetResultSetPromiseTest002');
        try {
            let resultSet;
            await kvStore.getResultSet(localDeviceId, 'batch_test_string_key').then((result) => {
                console.info('DeviceKvStoreGetResultSetPromiseTest002 getResultSet success');
                resultSet = result;
                expect(resultSet.getCount() == 0).assertTrue();
            }).catch((err) => {
                console.error('DeviceKvStoreGetResultSetPromiseTest002 getResultSet fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
            await kvStore.closeResultSet(resultSet).then((err) => {
                console.info('DeviceKvStoreGetResultSetPromiseTest002 closeResultSet success');
                expect(err == undefined).assertTrue();
            }).catch((err) => {
                console.error('DeviceKvStoreGetResultSetPromiseTest002 closeResultSet fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        }catch(e) {
            console.error('DeviceKvStoreGetResultSetPromiseTest002 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoregetResultSetPromiseTest003
     * @tc.desc Test Js Api DeviceKvStore.getResultSet() testcase 003
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreGetResultSetPromiseTest003', 0, async function (done) {
        console.info('DeviceKvStoreGetResultSetPromiseTest003');
        try {
            let resultSet;
            await kvStore.getResultSet().then((result) => {
                console.info('DeviceKvStoreGetResultSetPromiseTest003 getResultSet success');
                expect(null).assertFail();
            }).catch((err) => {
                console.error('DeviceKvStoreGetResultSetPromiseTest003 getResultSet fail ' + `, error code is ${err.code}, message is ${err.message}`);
            });
        }catch(e) {
            console.error('DeviceKvStoreGetResultSetPromiseTest003 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoregetResultSetPromiseTest004
     * @tc.desc Test Js Api DeviceKvStore.getResultSet() testcase 004
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreGetResultSetPromiseTest004', 0, async function (done) {
        console.info('DeviceKvStoreGetResultSetPromiseTest004');
        try {
            let resultSet;
            await kvStore.getResultSet('test_key_string', 123).then((result) => {
                console.info('DeviceKvStoreGetResultSetPromiseTest004 getResultSet success');
                expect(null).assertFail();
            }).catch((err) => {
                console.error('DeviceKvStoreGetResultSetPromiseTest004 getResultSet fail ' + `, error code is ${err.code}, message is ${err.message}`);
            });
        }catch(e) {
            console.error('DeviceKvStoreGetResultSetPromiseTest004 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoregetResultSetPromiseTest005
     * @tc.desc Test Js Api DeviceKvStore.getResultSet() testcase 005
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreGetResultSetPromiseTest005', 0, async function (done) {
        console.info('DeviceKvStoreGetResultSetPromiseTest005');
        try {
            let resultSet;
            let entries = [];
            for (var i = 0; i < 10; i++) {
                var key = 'batch_test_string_key';
                var entry = {
                    key : key + i,
                    value : {
                        type : factory.ValueType.STRING,
                        value : 'batch_test_string_value'
                    }
                }
                entries.push(entry);
            }
            await kvStore.putBatch(entries).then(async (err) => {
                console.info('DeviceKvStoreGetResultSetPromiseTest005 putBatch success');
                expect(err == undefined).assertTrue();
            }).catch((err) => {
                console.error('DeviceKvStorePutBatchPromiseTest001 putBatch fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
            var query = new factory.Query();
            query.prefixKey("batch_test");
            await kvStore.getResultSet(localDeviceId, query).then((result) => {
                console.info('DeviceKvStoreGetResultSetPromiseTest005 getResultSet success');
                resultSet = result;
                expect(resultSet.getCount() == 10).assertTrue();
            }).catch((err) => {
                console.error('DeviceKvStoreGetResultSetPromiseTest005 getResultSet fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
            query.deviceId(localDeviceId);
            console.info("testDeviceKvStoreGetResultSet005 " + query.getSqlLike());
            await kvStore.closeResultSet(resultSet).then((err) => {
                console.info('DeviceKvStoreGetResultSetPromiseTest005 closeResultSet success');
                expect(err == undefined).assertTrue();
            }).catch((err) => {
                console.error('DeviceKvStoreGetResultSetPromiseTest005 closeResultSet fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        }catch(e) {
            console.error('DeviceKvStoreGetResultSetPromiseTest005 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoregetResultSetPromiseTest006
     * @tc.desc Test Js Api DeviceKvStore.getResultSet() testcase 006
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreGetResultSetPromiseTest006', 0, async function (done) {
        console.info('DeviceKvStoreGetResultSetPromiseTest006');
        try {
            let resultSet;
            let entries = [];
            for (var i = 0; i < 10; i++) {
                var key = 'batch_test_string_key';
                var entry = {
                    key : key + i,
                    value : {
                        type : factory.ValueType.STRING,
                        value : 'batch_test_string_value'
                    }
                }
                entries.push(entry);
            }
            await kvStore.putBatch(entries).then(async (err) => {
                console.info('DeviceKvStoreGetResultSetPromiseTest006 putBatch success');
                expect(err == undefined).assertTrue();
            }).catch((err) => {
                console.error('DeviceKvStorePutBatchPromiseTest001 putBatch fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
            var query = new factory.Query();
            query.deviceId(localDeviceId);
            query.prefixKey("batch_test");
            console.info("testDeviceKvStoreGetResultSet006 " + query.getSqlLike());
            await kvStore.getResultSet(query).then((result) => {
                console.info('DeviceKvStoreGetResultSetPromiseTest006 getResultSet success');
                resultSet = result;
                expect(resultSet.getCount() == 10).assertTrue();
            }).catch((err) => {
                console.error('DeviceKvStoreGetResultSetPromiseTest006 getResultSet fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
            await kvStore.closeResultSet(resultSet).then((err) => {
                console.info('DeviceKvStoreGetResultSetPromiseTest006 closeResultSet success');
                expect(err == undefined).assertTrue();
            }).catch((err) => {
                console.error('DeviceKvStoreGetResultSetPromiseTest006 closeResultSet fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        }catch(e) {
            console.error('DeviceKvStoreGetResultSetPromiseTest006 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreCloseResultSetPromiseTest001
     * @tc.desc Test Js Api DeviceKvStore.CloseResultSet() testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreCloseResultSetPromiseTest001', 0, async function (done) {
        console.info('DeviceKvStoreCloseResultSetPromiseTest001');
        try {
            console.info('DeviceKvStoreCloseResultSetPromiseTest001 success');
            let resultSet = null;
            await kvStore.closeResultSet(resultSet).then(() => {
                console.info('DeviceKvStoreCloseResultSetPromiseTest001 closeResultSet success');
                expect(null).assertFail();
            }).catch((err) => {
                console.error('DeviceKvStoreCloseResultSetPromiseTest001 closeResultSet fail ' + `, error code is ${err.code}, message is ${err.message}`);
            });
        }catch(e) {
            console.error('DeviceKvStoreCloseResultSetPromiseTest001 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreCloseResultSetPromiseTest002
     * @tc.desc Test Js Api DeviceKvStore.CloseResultSet() testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreCloseResultSetPromiseTest002', 0, async function (done) {
        console.info('DeviceKvStoreCloseResultSetPromiseTest002');
        try {
            console.info('DeviceKvStoreCloseResultSetPromiseTest002 success');
            let resultSet = null;
            await kvStore.getResultSet(localDeviceId, 'batch_test_string_key').then((result) => {
                console.info('DeviceKvStoreCloseResultSetPromiseTest002 getResultSet success');
                resultSet = result;
            }).catch((err) => {
                console.error('DeviceKvStoreCloseResultSetPromiseTest002 getResultSet fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
            await kvStore.closeResultSet(resultSet).then((err) => {
                console.info('DeviceKvStoreCloseResultSetPromiseTest002 closeResultSet success');
                expect(err == undefined).assertTrue();
            }).catch((err) => {
                console.error('DeviceKvStoreCloseResultSetPromiseTest002 closeResultSet fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        }catch(e) {
            console.error('DeviceKvStoreCloseResultSetPromiseTest002 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreCloseResultSetPromiseTest003
     * @tc.desc Test Js Api DeviceKvStore.CloseResultSet() testcase 003
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreCloseResultSetPromiseTest003', 0, async function (done) {
        console.info('DeviceKvStoreCloseResultSetPromiseTest003');
        try {
            console.info('DeviceKvStoreCloseResultSetPromiseTest003 success');
            let resultSet = null;
            await kvStore.closeResultSet().then(() => {
                console.info('DeviceKvStoreCloseResultSetPromiseTest003 closeResultSet success');
                expect(null).assertFail();
            }).catch((err) => {
                console.error('DeviceKvStoreCloseResultSetPromiseTest003 closeResultSet fail ' + `, error code is ${err.code}, message is ${err.message}`);
            });
        }catch(e) {
            console.error('DeviceKvStoreCloseResultSetPromiseTest003 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreCloseResultSetPromiseTest004
     * @tc.desc Test Js Api DeviceKvStore.CloseResultSet() testcase 004
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreCloseResultSetPromiseTest004', 0, async function (done) {
        console.info('DeviceKvStoreCloseResultSetPromiseTest004');
        try {
            console.info('DeviceKvStoreCloseResultSetPromiseTest004 success');
        }catch(e) {
            console.error('DeviceKvStoreCloseResultSetPromiseTest004 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreGetPromiseTest001
     * @tc.desc Test Js Api DeviceKvStore.Get(ResultSize) testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreGetResultSizePromiseTest001', 0, async function (done) {
        console.info('DeviceKvStoreGetResultSizePromiseTest001');
        try {
            let entries = [];
            for (var i = 0; i < 10; i++) {
                var key = 'batch_test_string_key';
                var entry = {
                    key : key + i,
                    value : {
                        type : factory.ValueType.STRING,
                        value : 'batch_test_string_value'
                    }
                }
                entries.push(entry);
            }
            await kvStore.putBatch(entries).then(async (err) => {
                console.info('DeviceKvStoreGetResultSizePromiseTest001 putBatch success');
                expect(err == undefined).assertTrue();
            }).catch((err) => {
                console.error('DeviceKvStorePutBatchPromiseTest001 putBatch fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
            var query = new factory.Query();
            query.prefixKey("batch_test");
            query.deviceId(localDeviceId);
            await kvStore.getResultSize(query).then((resultSize) => {
                console.info('DeviceKvStoreGetResultSizePromiseTest001 getResultSet success');
                expect(resultSize == 10).assertTrue();
            }).catch((err) => {
                console.error('DeviceKvStoreGetResultSizePromiseTest001 getResultSet fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        }catch(e) {
            console.error('DeviceKvStoreGetResultSizePromiseTest001 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreGetPromiseTest002
     * @tc.desc Test Js Api DeviceKvStore.Get(ResultSize) testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreGetResultSizePromiseTest002', 0, async function (done) {
        console.info('DeviceKvStoreGetResultSizePromiseTest001');
        try {
            let entries = [];
            for (var i = 0; i < 10; i++) {
                var key = 'batch_test_string_key';
                var entry = {
                    key : key + i,
                    value : {
                        type : factory.ValueType.STRING,
                        value : 'batch_test_string_value'
                    }
                }
                entries.push(entry);
            }
            await kvStore.putBatch(entries).then(async (err) => {
                console.info('DeviceKvStoreGetResultSizePromiseTest001 putBatch success');
                expect(err == undefined).assertTrue();
            }).catch((err) => {
                console.error('DeviceKvStorePutBatchPromiseTest001 putBatch fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
            var query = new factory.Query();
            query.prefixKey("batch_test");
            await kvStore.getResultSize(localDeviceId, query).then((resultSize) => {
                console.info('DeviceKvStoreGetResultSizePromiseTest001 getResultSet success');
                expect(resultSize == 10).assertTrue();
            }).catch((err) => {
                console.error('DeviceKvStoreGetResultSizePromiseTest001 getResultSet fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        }catch(e) {
            console.error('DeviceKvStoreGetResultSizePromiseTest001 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreGetEntriesPromiseTest001
     * @tc.desc Test Js Api DeviceKvStore.GetEntries() testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreGetEntriesPromiseTest001', 0, async function (done) {
        console.info('DeviceKvStoreGetEntriesPromiseTest001');
        try {
            var arr = new Uint8Array([21,31]);
            let entries = [];
            for (var i = 0; i < 10; i++) {
                var key = 'batch_test_bool_key';
                var entry = {
                    key : key + i,
                    value : {
                        type : factory.ValueType.BYTE_ARRAY,
                        value : arr
                    }
                }
                entries.push(entry);
            }
            console.info('DeviceKvStoreGetEntriesPromiseTest001 entries: ' + JSON.stringify(entries));
            await kvStore.putBatch(entries).then(async (err) => {
                console.info('DeviceKvStoreGetEntriesPromiseTest001 putBatch success');
                expect(err == undefined).assertTrue();
                var query = new factory.Query();
                query.deviceId(localDeviceId);
                query.prefixKey("batch_test");
                await kvStore.getEntries(localDeviceId, query).then((entrys) => {
                    console.info('DeviceKvStoreGetEntriesPromiseTest001 getEntries success');
                    expect(entrys.length == 10).assertTrue();
                    expect(entrys[0].value.value.toString() == arr.toString()).assertTrue();
                }).catch((err) => {
                    console.error('DeviceKvStoreGetEntriesPromiseTest001 getEntries fail ' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(null).assertFail();
                });
            }).catch((err) => {
                console.error('DeviceKvStoreGetEntriesPromiseTest001 putBatch fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
            console.info('DeviceKvStoreGetEntriesPromiseTest001 success');
        }catch(e) {
            console.error('DeviceKvStoreGetEntriesPromiseTest001 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreGetEntriesPromiseTest002
     * @tc.desc Test Js Api DeviceKvStore.GetEntries() testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreGetEntriesPromiseTest002', 0, async function (done) {
        console.info('DeviceKvStoreGetEntriesPromiseTest002');
        try {
            var arr = new Uint8Array([21,31]);
            let entries = [];
            for (var i = 0; i < 10; i++) {
                var key = 'batch_test_bool_key';
                var entry = {
                    key : key + i,
                    value : {
                        type : factory.ValueType.BYTE_ARRAY,
                        value : arr
                    }
                }
                entries.push(entry);
            }
            console.info('DeviceKvStoreGetEntriesPromiseTest002 entries: ' + JSON.stringify(entries));
            await kvStore.putBatch(entries).then(async (err) => {
                console.info('DeviceKvStoreGetEntriesPromiseTest002 putBatch success');
                expect(err == undefined).assertTrue();
                var query = new factory.Query();
                query.prefixKey("batch_test");
                query.deviceId(localDeviceId);
                await kvStore.getEntries(query).then((entrys) => {
                    console.info('DeviceKvStoreGetEntriesPromiseTest002 getEntries success');
                    expect(entrys.length == 10).assertTrue();
                    expect(entrys[0].value.value.toString() == arr.toString()).assertTrue();
                }).catch((err) => {
                    console.error('DeviceKvStoreGetEntriesPromiseTest002 getEntries fail ' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(null).assertFail();
                });
            }).catch((err) => {
                console.error('DeviceKvStoreGetEntriesPromiseTest002 putBatch fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
            console.info('DeviceKvStoreGetEntriesPromiseTest002 success');
        }catch(e) {
            console.error('DeviceKvStoreGetEntriesPromiseTest002 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })
})
