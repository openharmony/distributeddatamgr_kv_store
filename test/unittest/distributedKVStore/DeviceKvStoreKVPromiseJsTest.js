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
import dataSharePredicates from '@ohos.data.dataSharePredicates'
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
     * @tc.name DeviceKvStorePutStringPromiseFewerArgsTest
     * @tc.desc Test Js Api DeviceKvStore.Put(String) with fewer args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStorePutStringPromiseFewerArgsTest', 0, async function (done) {
        console.info('DeviceKvStorePutStringPromiseFewerArgsTest');
        try {
            await kvStore.put(KEY_TEST_STRING_ELEMENT, null).then((data) => {
                console.info('DeviceKvStorePutStringPromiseFewerArgsTest put success');
                expect(null).assertFail();
            }).catch((error) => {
                console.error('DeviceKvStorePutStringPromiseFewerArgsTest put error' + `, error code is ${error.code}, message is ${error.message}`);
            });
        } catch (e) {
            console.error('DeviceKvStorePutStringPromiseFewerArgsTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(e.code == 401).assertTrue();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStorePutStringPromiseInvalidArgsTest
     * @tc.desc Test Js Api DeviceKvStore.Put(String) with invalid args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStorePutStringPromiseInvalidArgsTest', 0, async function (done) {
        console.info('DeviceKvStorePutStringPromiseInvalidArgsTest');
        try {
            await kvStore.put(KEY_TEST_STRING_ELEMENT, '').then((data) => {
                console.info('DeviceKvStorePutStringPromiseInvalidArgsTest put success');
                expect(data == undefined).assertTrue();
            }).catch((error) => {
                console.error('DeviceKvStorePutStringPromiseInvalidArgsTest put error' + `, error code is ${error.code}, message is ${error.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('DeviceKvStorePutStringPromiseInvalidArgsTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStorePutStringPromiseSucTest
     * @tc.desc Test Js Api DeviceKvStore.Put(String) successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStorePutStringPromiseSucTest', 0, async function (done) {
        console.info('DeviceKvStorePutStringPromiseSucTest');
        try {
            await kvStore.put(KEY_TEST_STRING_ELEMENT, VALUE_TEST_STRING_ELEMENT).then((data) => {
                console.info('DeviceKvStorePutStringPromiseSucTest put success');
                expect(data == undefined).assertTrue();
            }).catch((error) => {
                console.error('DeviceKvStorePutStringPromiseSucTest put error' + `, error code is ${error.code}, message is ${error.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('DeviceKvStorePutStringPromiseSucTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStorePutStringPromiseLongStringTest
     * @tc.desc Test Js Api DeviceKvStore.Put(String) with a long string
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStorePutStringPromiseLongStringTest', 0, async function (done) {
        console.info('DeviceKvStorePutStringPromiseLongStringTest');
        try {
            var str = '';
            for (var i = 0 ; i < 4095; i++) {
                str += 'x';
            }
            await kvStore.put(KEY_TEST_STRING_ELEMENT, str).then(async (data) => {
                console.info('DeviceKvStorePutStringPromiseLongStringTest put success');
                expect(data == undefined).assertTrue();
                await kvStore.get(localDeviceId, KEY_TEST_STRING_ELEMENT).then((data) => {
                    console.info('DeviceKvStorePutStringPromiseLongStringTest get success data ' + data);
                    expect(str == data).assertTrue();
                }).catch((err) => {
                    console.error('DeviceKvStorePutStringPromiseLongStringTest get fail ' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(null).assertFail();
                });
            }).catch((error) => {
                console.error('DeviceKvStorePutStringPromiseLongStringTest put error' + `, error code is ${error.code}, message is ${error.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('DeviceKvStorePutStringPromiseLongStringTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreGetStringPromiseSucTest
     * @tc.desc Test Js Api DeviceKvStore.Get(String) successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreGetStringPromiseSucTest', 0, async function (done) {
        console.info('DeviceKvStoreGetStringPromiseSucTest');
        try {
            await kvStore.put(KEY_TEST_STRING_ELEMENT, VALUE_TEST_STRING_ELEMENT).then(async (data) => {
                console.info('DeviceKvStoreGetStringPromiseSucTest put success');
                expect(data == undefined).assertTrue();
                await kvStore.get(localDeviceId, KEY_TEST_STRING_ELEMENT).then((data) => {
                    console.info('DeviceKvStoreGetStringPromiseSucTest get success');
                    expect(VALUE_TEST_STRING_ELEMENT == data).assertTrue();
                }).catch((err) => {
                    console.error('DeviceKvStoreGetStringPromiseSucTest get fail ' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(null).assertFail();
                });
            }).catch((error) => {
                console.error('DeviceKvStoreGetStringPromiseSucTest put error' + `, error code is ${error.code}, message is ${error.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('DeviceKvStoreGetStringPromiseSucTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreGetStringPromiseNonExistingTest
     * @tc.desc Test Js Api DeviceKvStore.Get(String) getting a non-existing string
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreGetStringPromiseTest', 0, async function (done) {
        console.info('DeviceKvStoreGetStringPromiseTest');
        try {
            await kvStore.get(localDeviceId, KEY_TEST_STRING_ELEMENT).then((data) => {
                console.info('DeviceKvStoreGetStringPromiseTest get success');
                expect(null).assertFail();
            }).catch((err) => {
                console.error('DeviceKvStoreGetStringPromiseTest get fail ' + `, error code is ${err.code}, message is ${err.message}`);
            });
        } catch (e) {
            console.error('DeviceKvStoreGetStringPromiseTest get e ' + `, error code is ${e.code}, message is ${e.message}`);
        }
        done();
    })

    /**
     * @tc.name DeviceKvStorePutIntPromiseSucTest
     * @tc.desc Test Js Api DeviceKvStore.Put(Int) successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStorePutIntPromiseSucTest', 0, async function (done) {
        console.info('DeviceKvStorePutIntPromiseSucTest');
        try {
            await kvStore.put(KEY_TEST_INT_ELEMENT, VALUE_TEST_INT_ELEMENT).then((data) => {
                console.info('DeviceKvStorePutIntPromiseSucTest put success');
                expect(data == undefined).assertTrue();
            }).catch((err) => {
                console.error('DeviceKvStorePutIntPromiseSucTest put fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('DeviceKvStorePutIntPromiseSucTest put e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStorePutIntPromiseMaxTest
     * @tc.desc Test Js Api DeviceKvStore.Put(Int) with max int
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStorePutIntPromiseMaxTest', 0, async function (done) {
        console.info('DeviceKvStorePutIntPromiseMaxTest');
        try {
            var intValue = Number.MAX_VALUE;
            await kvStore.put(KEY_TEST_INT_ELEMENT, intValue).then(async (data) => {
                console.info('DeviceKvStorePutIntPromiseMaxTest put success');
                expect(data == undefined).assertTrue();
                await kvStore.get(localDeviceId, KEY_TEST_INT_ELEMENT).then((data) => {
                    console.info('DeviceKvStorePutIntPromiseMaxTest get success');
                    expect(intValue == data).assertTrue();
                }).catch((err) => {
                    console.error('DeviceKvStorePutIntPromiseMaxTest get fail ' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(null).assertFail();
                });
            }).catch((err) => {
                console.error('DeviceKvStorePutIntPromiseMaxTest put fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('DeviceKvStorePutIntPromiseMaxTest put e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStorePutIntPromiseMinTest
     * @tc.desc Test Js Api DeviceKvStore.Put(Int) with minimize int
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStorePutIntPromiseMinTest', 0, async function (done) {
        console.info('DeviceKvStorePutIntPromiseMinTest');
        try {
            var intValue = Number.MIN_VALUE;
            await kvStore.put(KEY_TEST_INT_ELEMENT, intValue).then(async (data) => {
                console.info('DeviceKvStorePutIntPromiseMinTest put success');
                expect(data == undefined).assertTrue();
                await kvStore.get(localDeviceId, KEY_TEST_INT_ELEMENT).then((data) => {
                    console.info('DeviceKvStorePutIntPromiseMinTest get success');
                    expect(intValue == data).assertTrue();
                }).catch((err) => {
                    console.error('DeviceKvStorePutIntPromiseMinTest get fail ' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(null).assertFail();
                });
            }).catch((err) => {
                console.error('DeviceKvStorePutIntPromiseMinTest put fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('DeviceKvStorePutIntPromiseMinTest put e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreGetIntPromiseSucTest
     * @tc.desc Test Js Api DeviceKvStore.Get(Int) successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreGetIntPromiseTest', 0, async function (done) {
        console.info('DeviceKvStoreGetIntPromiseTest');
        try {
            await kvStore.put(KEY_TEST_INT_ELEMENT, VALUE_TEST_INT_ELEMENT).then(async (data) => {
                console.info('DeviceKvStoreGetIntPromiseTest put success');
                expect(data == undefined).assertTrue();
                await kvStore.get(localDeviceId, KEY_TEST_INT_ELEMENT).then((data) => {
                    console.info('DeviceKvStoreGetIntPromiseTest get success');
                    expect(VALUE_TEST_INT_ELEMENT == data).assertTrue();
                }).catch((err) => {
                    console.error('DeviceKvStoreGetIntPromiseTest get fail ' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(null).assertFail();
                });
            }).catch((err) => {
                console.error('DeviceKvStoreGetIntPromiseTest put fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('DeviceKvStoreGetIntPromiseTest put e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStorePutBoolPromiseSucTest
     * @tc.desc Test Js Api DeviceKvStore.Put(Bool) successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStorePutBoolPromiseSucTest', 0, async function (done) {
        console.info('DeviceKvStorePutBoolPromiseSucTest');
        try {
            await kvStore.put(KEY_TEST_BOOLEAN_ELEMENT, VALUE_TEST_BOOLEAN_ELEMENT).then((data) => {
                console.info('DeviceKvStorePutBoolPromiseSucTest put success');
                expect(data == undefined).assertTrue();
            }).catch((err) => {
                console.error('DeviceKvStorePutBoolPromiseSucTest put fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('DeviceKvStorePutBoolPromiseSucTest put e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreGetBoolPromiseSucTest
     * @tc.desc Test Js Api DeviceKvStore.Get(Bool) successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreGetBoolPromiseTest', 0, async function (done) {
        console.info('DeviceKvStoreGetBoolPromiseTest');
        try {
            var boolValue = false;
            await kvStore.put(KEY_TEST_BOOLEAN_ELEMENT, boolValue).then(async (data) => {
                console.info('DeviceKvStoreGetBoolPromiseTest put success');
                expect(data == undefined).assertTrue();
                await kvStore.get(localDeviceId, KEY_TEST_BOOLEAN_ELEMENT).then((data) => {
                    console.info('DeviceKvStoreGetBoolPromiseTest get success');
                    expect(boolValue == data).assertTrue();
                }).catch((err) => {
                    console.error('DeviceKvStoreGetBoolPromiseTest get fail ' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(null).assertFail();
                });
            }).catch((err) => {
                console.error('DeviceKvStoreGetBoolPromiseTest put fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('DeviceKvStoreGetBoolPromiseTest put e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStorePutFloatPromiseTestSuc001
     * @tc.desc Test Js Api DeviceKvStore.Put(Float) successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStorePutFloatPromiseTestSuc001', 0, async function (done) {
        console.info('DeviceKvStorePutFloatPromiseTestSuc001');
        try {
            await kvStore.put(KEY_TEST_FLOAT_ELEMENT, VALUE_TEST_FLOAT_ELEMENT).then((data) => {
                console.info('DeviceKvStorePutFloatPromiseTestSuc001 put success');
                expect(data == undefined).assertTrue();
            }).catch((err) => {
                console.error('DeviceKvStorePutFloatPromiseTestSuc001 put fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('DeviceKvStorePutFloatPromiseTestSuc001 put e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreGetFloatPromiseSucTest
     * @tc.desc Test Js Api DeviceKvStore.Get(Float) successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreGetFloatPromiseSucTest', 0, async function (done) {
        console.info('DeviceKvStoreGetFloatPromiseSucTest');
        try {
            var floatValue = 123456.654321;
            await kvStore.put(KEY_TEST_FLOAT_ELEMENT, floatValue).then(async (data) => {
                console.info('DeviceKvStoreGetFloatPromiseSucTest put success');
                expect(data == undefined).assertTrue();
                await kvStore.get(localDeviceId, KEY_TEST_FLOAT_ELEMENT).then((data) => {
                    console.info('DeviceKvStoreGetFloatPromiseSucTest get success');
                    expect(floatValue == data).assertTrue();
                }).catch((err) => {
                    console.error('DeviceKvStoreGetFloatPromiseSucTest get fail ' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(null).assertFail();
                });
            }).catch((err) => {
                console.error('DeviceKvStoreGetFloatPromiseSucTest put fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('DeviceKvStoreGetFloatPromiseSucTest put e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreDeleteStringPromiseSucTest
     * @tc.desc Test Js Api DeviceKvStore.Delete(String) successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreDeleteStringPromiseSucTest', 0, async function (done) {
        console.info('DeviceKvStoreDeleteStringPromiseSucTest');
        try {
            var str = 'this is a test string';
            await kvStore.put(KEY_TEST_STRING_ELEMENT, str).then(async (data) => {
                console.info('DeviceKvStoreDeleteStringPromiseSucTest put success');
                expect(data == undefined).assertTrue();
                await kvStore.delete(KEY_TEST_STRING_ELEMENT).then((data) => {
                    console.info('DeviceKvStoreDeleteStringPromiseSucTest delete success');
                    expect(data == undefined).assertTrue();
                }).catch((err) => {
                    console.error('DeviceKvStoreDeleteStringPromiseSucTest delete fail ' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(null).assertFail();
                });
            }).catch((err) => {
                console.error('DeviceKvStoreDeleteStringPromiseSucTest put fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('DeviceKvStoreDeleteStringPromiseSucTest put e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreDeleteStringPromiseLongStringTest
     * @tc.desc Test Js Api DeviceKvStore.Delete(String) with a long string
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreDeleteStringPromiseLongStringTest', 0, async function (done) {
        console.info('DeviceKvStoreDeleteStringPromiseLongStringTest');
        try {
            var str = '';
            for (var i = 0 ; i < 4096; i++) {
                str += 'x';
            }
            await kvStore.put(KEY_TEST_STRING_ELEMENT, str).then(async (data) => {
                console.info('DeviceKvStoreDeleteStringPromiseLongStringTest put success');
                expect(data == undefined).assertTrue();
                await kvStore.delete(KEY_TEST_STRING_ELEMENT).then((data) => {
                    console.info('DeviceKvStoreDeleteStringPromiseLongStringTest delete success');
                    expect(data == undefined).assertTrue();
                }).catch((err) => {
                    console.error('DeviceKvStoreDeleteStringPromiseLongStringTest delete fail ' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(null).assertFail();
                });
            }).catch((err) => {
                console.error('DeviceKvStoreDeleteStringPromiseLongStringTest put fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('DeviceKvStoreDeleteStringPromiseLongStringTest put e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreDeleteStringPromiseNonExistingTest
     * @tc.desc Test Js Api DeviceKvStore.Delete(String) deleting a non-exsiting string
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreDeleteStringPromiseNonExistingTest', 0, async function (done) {
        console.info('DeviceKvStoreDeleteStringPromiseNonExistingTest');
        try {
            await kvStore.delete(KEY_TEST_STRING_ELEMENT).then((data) => {
                console.info('DeviceKvStoreDeleteStringPromiseNonExistingTest delete success');
                expect(data == undefined).assertTrue();
            }).catch((err) => {
                console.error('DeviceKvStoreDeleteStringPromiseNonExistingTest delete fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('DeviceKvStoreDeleteStringPromiseNonExistingTest put e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreDeleteIntPromiseSucTest
     * @tc.desc Test Js Api DeviceKvStore.Delete(Int) successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreDeleteIntPromiseTest', 0, async function (done) {
        console.info('DeviceKvStoreDeleteIntPromiseTest');
        try {
            await kvStore.put(KEY_TEST_INT_ELEMENT, VALUE_TEST_INT_ELEMENT).then(async (data) => {
                console.info('DeviceKvStoreDeleteIntPromiseTest put success');
                expect(data == undefined).assertTrue();
                await kvStore.delete(KEY_TEST_INT_ELEMENT).then((data) => {
                    console.info('DeviceKvStoreDeleteIntPromiseTest delete success');
                    expect(data == undefined).assertTrue();
                }).catch((err) => {
                    console.error('DeviceKvStoreDeleteIntPromiseTest delete fail ' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(null).assertFail();
                });
            }).catch((err) => {
                console.error('DeviceKvStoreDeleteIntPromiseTest put fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('DeviceKvStoreDeleteIntPromiseTest put e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreDeleteFloatPromiseSucTest
     * @tc.desc Test Js Api DeviceKvStore.Delete(Float) successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreDeleteFloatPromiseSucTest', 0, async function (done) {
        console.info('DeviceKvStoreDeleteFloatPromiseSucTest');
        try {
            await kvStore.put(KEY_TEST_FLOAT_ELEMENT, VALUE_TEST_FLOAT_ELEMENT).then(async (data) => {
                console.info('DeviceKvStoreDeleteFloatPromiseSucTest put success');
                expect(data == undefined).assertTrue();
                await kvStore.delete(KEY_TEST_FLOAT_ELEMENT).then((data) => {
                    console.info('DeviceKvStoreDeleteFloatPromiseSucTest delete success');
                    expect(data == undefined).assertTrue();
                }).catch((err) => {
                    console.error('DeviceKvStoreDeleteFloatPromiseSucTest delete fail ' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(null).assertFail();
                });
            }).catch((err) => {
                console.error('DeviceKvStoreDeleteFloatPromiseSucTest put fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('DeviceKvStoreDeleteFloatPromiseSucTest put e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreDeleteBoolPromiseSucTest
     * @tc.desc Test Js Api DeviceKvStore.Delete(Bool) successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreDeleteBoolPromiseSucTest', 0, async function (done) {
        console.info('DeviceKvStoreDeleteBoolPromiseSucTest');
        try {
            await kvStore.put(KEY_TEST_BOOLEAN_ELEMENT, VALUE_TEST_BOOLEAN_ELEMENT).then(async (data) => {
                console.info('DeviceKvStoreDeleteBoolPromiseSucTest put success');
                expect(data == undefined).assertTrue();
                await kvStore.delete(KEY_TEST_BOOLEAN_ELEMENT).then((data) => {
                    console.info('DeviceKvStoreDeleteBoolPromiseSucTest delete success');
                    expect(data == undefined).assertTrue();
                }).catch((err) => {
                    console.error('DeviceKvStoreDeleteBoolPromiseSucTest delete fail ' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(null).assertFail();
                });
            }).catch((err) => {
                console.error('DeviceKvStoreDeleteBoolPromiseSucTest put fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('DeviceKvStoreDeleteBoolPromiseSucTest put e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreOnChangePromiseType0Test
     * @tc.desc Test Js Api DeviceKvStore.OnChange() with subscribe type 0
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreOnChangePromiseType0Test', 0, async function (done) {
        try {
            kvStore.on('dataChange', 0, function (data) {
                console.info('DeviceKvStoreOnChangePromiseType0Test 0' + JSON.stringify(data))
                expect(data != null).assertTrue();
            });
            await kvStore.put(KEY_TEST_FLOAT_ELEMENT, VALUE_TEST_FLOAT_ELEMENT).then((data) => {
                console.info('DeviceKvStoreOnChangePromiseType0Test put success');
                expect(data == undefined).assertTrue();
            }).catch((error) => {
                console.error('DeviceKvStoreOnChangePromiseType0Test put fail ' + `, error code is ${error.code}, message is ${error.message}`);
                expect(null).assertFail();
            });
        }catch(e) {
            console.error('DeviceKvStoreOnChangePromiseType0Test put e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreOnChangePromiseType1Test
     * @tc.desc Test Js Api DeviceKvStore.OnChange() with subscribe mode 1
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreOnChangePromiseType1Test', 0, async function (done) {
        try {
            kvStore.on('dataChange', 1, function (data) {
                console.info('DeviceKvStoreOnChangePromiseType1Test 0' + JSON.stringify(data))
                expect(data != null).assertTrue();
            });
            await kvStore.put(KEY_TEST_FLOAT_ELEMENT, VALUE_TEST_FLOAT_ELEMENT).then((data) => {
                console.info('DeviceKvStoreOnChangePromiseType1Test put success');
                expect(data == undefined).assertTrue();
            }).catch((error) => {
                console.error('DeviceKvStoreOnChangePromiseType1Test put fail ' + `, error code is ${error.code}, message is ${error.message}`);
                expect(null).assertFail();
            });
        }catch(e) {
            console.error('DeviceKvStoreOnChangePromiseType1Test put e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreOnChangePromiseType2Test
     * @tc.desc Test Js Api DeviceKvStore.OnChange() with subscribe type 2
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreOnChangePromiseType2Test', 0, async function (done) {
        try {
            kvStore.on('dataChange', 2, function (data) {
                console.info('DeviceKvStoreOnChangePromiseType2Test 0' + JSON.stringify(data))
                expect(data != null).assertTrue();
            });
            await kvStore.put(KEY_TEST_FLOAT_ELEMENT, VALUE_TEST_FLOAT_ELEMENT).then((data) => {
                console.info('DeviceKvStoreOnChangePromiseType2Test put success');
                expect(data == undefined).assertTrue();
            }).catch((error) => {
                console.error('DeviceKvStoreOnChangePromiseType2Test put fail ' + `, error code is ${error.code}, message is ${error.message}`);
                expect(null).assertFail();
            });
        }catch(e) {
            console.error('DeviceKvStoreOnChangePromiseType2Test put e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreOnSyncCompletePromisePullOnlyTest
     * @tc.desc Test Js Api DeviceKvStore.OnSyncComplete() pull only
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreOnSyncCompletePromiseTest', 0, async function (done) {
        try {
            kvStore.on('syncComplete', function (data) {
                console.info('DeviceKvStoreOnSyncCompletePromiseTest 0' + data)
                expect(data != null).assertTrue();
            });
            await kvStore.put(KEY_TEST_SYNC_ELEMENT, VALUE_TEST_SYNC_ELEMENT).then((data) => {
                console.info('DeviceKvStoreOnSyncCompletePromiseTest put success');
                expect(data == undefined).assertTrue();
            }).catch((error) => {
                console.error('DeviceKvStoreOnSyncCompletePromiseTest put failed:' + `, error code is ${e.code}, message is ${e.message}`);
                expect(null).assertFail();
            });
            try {
                var mode = factory.SyncMode.PULL_ONLY;
                console.info('kvStore.sync to ' + JSON.stringify(syncDeviceIds));
                kvStore.sync(syncDeviceIds, mode);
            } catch (e) {
                console.error('DeviceKvStoreOnSyncCompletePromiseTest sync no peer device :e:' + `, error code is ${e.code}, message is ${e.message}`);
            }
        } catch(e) {
            console.error('DeviceKvStoreOnSyncCompletePromiseTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreOnSyncCompletePromisePushOnlyTest
     * @tc.desc Test Js Api DeviceKvStore.OnSyncComplete() push only
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreOnSyncCompletePromisePushOnlyTest', 0, async function (done) {
        try {
            kvStore.on('syncComplete', function (data) {
                console.info('DeviceKvStoreOnSyncCompletePromisePushOnlyTest 0' + data)
                expect(data != null).assertTrue();
            });
            await kvStore.put(KEY_TEST_SYNC_ELEMENT, VALUE_TEST_SYNC_ELEMENT).then((data) => {
                console.info('DeviceKvStoreOnSyncCompletePromisePushOnlyTest put success');
                expect(data == undefined).assertTrue();
            }).catch((error) => {
                console.error('DeviceKvStoreOnSyncCompletePromisePushOnlyTest put failed:' + `, error code is ${e.code}, message is ${e.message}`);
                expect(null).assertFail();
            });
            try {
                var mode = factory.SyncMode.PUSH_ONLY;
                console.info('kvStore.sync to ' + JSON.stringify(syncDeviceIds));
                kvStore.sync(syncDeviceIds, mode);
            } catch(error) {
                console.error('DeviceKvStoreOnSyncCompletePromisePushOnlyTest no peer device :e:' + `, error code is ${error.code}, message is ${error.message}`);
            }
        } catch(e) {
            console.error('DeviceKvStoreOnSyncCompletePromisePushOnlyTest put e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreOnSyncCompletePromisePushPullTest
     * @tc.desc Test Js Api DeviceKvStore.OnSyncComplete() push pull
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreOnSyncCompletePromisePushPullTest', 0, async function (done) {
        try {
            kvStore.on('syncComplete', function (data) {
                console.info('DeviceKvStoreOnSyncCompletePromisePushPullTest 0' + data)
                expect(data != null).assertTrue();
            });
            await kvStore.put(KEY_TEST_SYNC_ELEMENT, VALUE_TEST_SYNC_ELEMENT).then((data) => {
                console.info('DeviceKvStoreOnSyncCompletePromisePushPullTest put success');
                expect(data == undefined).assertTrue();
            }).catch((error) => {
                console.error('DeviceKvStoreOnSyncCompletePromisePushPullTest put failed:' + `, error code is ${e.code}, message is ${e.message}`);
                expect(null).assertFail();
            });
            try {
                var mode = factory.SyncMode.PUSH_PULL;
                console.info('kvStore.sync to ' + JSON.stringify(syncDeviceIds));
                kvStore.sync(syncDeviceIds, mode);
            } catch(error) {
                console.error('DeviceKvStoreOnSyncCompletePromisePushPullTest no peer device :e:' + `, error code is ${error.code}, message is ${error.message}`);
            }
        } catch(e) {
            console.error('DeviceKvStoreOnSyncCompletePromisePushPullTest put e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreOffChangePromiseDataChangeTest
     * @tc.desc Test Js Api DeviceKvStore.OffChange() successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreOffChangePromiseDataChangeTest', 0, async function (done) {
        console.info('DeviceKvStoreOffChangePromiseDataChangeTest');
        try {
            var func = function (data) {
                console.info('DeviceKvStoreOffChangePromiseDataChangeTest 0' + data)
            };
            kvStore.on('dataChange', 0, func);
            kvStore.off('dataChange', func);
        }catch(e) {
            console.error('DeviceKvStoreOffChangePromiseDataChangeTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreOffChangePromiseInvalidArgsTest
     * @tc.desc Test Js Api DeviceKvStore.OffChange() with invalid args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreOffChangePromiseInvalidArgsTest', 0, async function (done) {
        console.info('DeviceKvStoreOffChangePromiseInvalidArgsTest');
        try {
            var func = function (data) {
                console.info('DeviceKvStoreOffChangePromiseInvalidArgsTest 0' + data)
            };
            kvStore.on('dataChange', 0, func);
            kvStore.off('dataChange');
        }catch(e) {
            console.error('DeviceKvStoreOffChangePromiseInvalidArgsTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreOffChangePromiseSyncCompleteTest
     * @tc.desc Test Js Api DeviceKvStore.OffSyncComplete() successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreOffChangePromiseSyncCompleteTest', 0, async function (done) {
        console.info('DeviceKvStoreOffChangePromiseSyncCompleteTest');
        try {
            var func = function (data) {
                console.info('DeviceKvStoreOffChangePromiseSyncCompleteTest 0' + data)
            };
            kvStore.on('syncComplete', func);
            kvStore.off('syncComplete', func);
        }catch(e) {
            console.error('DeviceKvStoreOffChangePromiseSyncCompleteTest put e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreOffSyncCompletePromiseInvalidArgsTest
     * @tc.desc Test Js Api DeviceKvStore.OffSyncComplete() with invalid arguments
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreOffSyncCompletePromiseInvalidArgsTest', 0, async function (done) {
        console.info('DeviceKvStoreOffSyncCompletePromiseInvalidArgsTest');
        try {
            var func = function (data) {
                console.info('DeviceKvStoreOffSyncCompletePromiseInvalidArgsTest 0' + data)
            };
            kvStore.on('syncComplete', func);
            kvStore.off('syncComplete');
        }catch(e) {
            console.error('DeviceKvStoreOffSyncCompletePromiseInvalidArgsTest put e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreSetSyncRangePromiseDisjointTest
     * @tc.desc Test Js Api DeviceKvStore.SetSyncRange() with disjoint ranges
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreSetSyncRangePromiseDisjointTest', 0, async function (done) {
        console.info('DeviceKvStoreSetSyncRangePromiseDisjointTest');
        try {
            var localLabels = ['A', 'B'];
            var remoteSupportLabels = ['C', 'D'];
            await kvStore.setSyncRange(localLabels, remoteSupportLabels).then((err) => {
                console.info('DeviceKvStoreSetSyncRangePromiseDisjointTest setSyncRange success');
                expect(err == undefined).assertTrue();
            }).catch((err) => {
                console.error('DeviceKvStoreDeleteStringPromiseNonExistingTest delete fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        }catch(e) {
            console.error('DeviceKvStoreSetSyncRangePromiseDisjointTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreSetSyncRangePromiseJointTest
     * @tc.desc Test Js Api DeviceKvStore.SetSyncRange() with joint raqnges
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreSetSyncRangePromiseJointTest', 0, async function (done) {
        console.info('DeviceKvStoreSetSyncRangePromiseJointTest');
        try {
            var localLabels = ['A', 'B'];
            var remoteSupportLabels = ['B', 'C'];
            await kvStore.setSyncRange(localLabels, remoteSupportLabels).then((err) => {
                console.info('DeviceKvStoreSetSyncRangePromiseJointTest setSyncRange success');
                expect(err == undefined).assertTrue();
            }).catch((err) => {
                console.error('DeviceKvStoreSetSyncRangePromiseJointTest delete fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        }catch(e) {
            console.error('DeviceKvStoreSetSyncRangePromiseJointTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreSetSyncRangePromiseSameTest
     * @tc.desc Test Js Api DeviceKvStore.SetSyncRange() with same range
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreSetSyncRangePromiseSameTest', 0, async function (done) {
        console.info('DeviceKvStoreSetSyncRangePromiseSameTest');
        try {
            var localLabels = ['A', 'B'];
            var remoteSupportLabels = ['A', 'B'];
            await kvStore.setSyncRange(localLabels, remoteSupportLabels).then((err) => {
                console.info('DeviceKvStoreSetSyncRangePromiseSameTest setSyncRange success');
                expect(err == undefined).assertTrue();
            }).catch((err) => {
                console.error('DeviceKvStoreSetSyncRangePromiseSameTest delete fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        }catch(e) {
            console.error('DeviceKvStoreSetSyncRangePromiseSameTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStorePutBatchPromiseStringTest
     * @tc.desc Test Js Api DeviceKvStore.PutBatch() with string value
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStorePutBatchPromiseStringTest', 0, async function (done) {
        console.info('DeviceKvStorePutBatchPromiseStringTest');
        try {
            let entries = putBatchString(10, 'batch_test_string_key');
            console.info('DeviceKvStorePutBatchPromiseStringTest entries: ' + JSON.stringify(entries));
            await kvStore.putBatch(entries).then(async (err) => {
                console.info('DeviceKvStorePutBatchPromiseStringTest putBatch success');
                expect(err == undefined).assertTrue();
                await kvStore.getEntries(localDeviceId, 'batch_test_string_key').then((entrys) => {
                    expect(entrys.length == 10).assertTrue();
                    expect(entrys[0].value.value == 'batch_test_string_value').assertTrue();
                }).catch((err) => {
                    console.error('DeviceKvStorePutBatchPromiseStringTest getEntries fail ' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(null).assertFail();
                });
            }).catch((err) => {
                console.error('DeviceKvStorePutBatchPromiseStringTest putBatch fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        }catch(e) {
            console.error('DeviceKvStorePutBatchPromiseStringTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStorePutBatchPromiseIntegerTest
     * @tc.desc Test Js Api DeviceKvStore.PutBatch() with integer value
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStorePutBatchPromiseIntegerTest', 0, async function (done) {
        console.info('DeviceKvStorePutBatchPromiseIntegerTest');
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
            console.info('DeviceKvStorePutBatchPromiseIntegerTest entries: ' + JSON.stringify(entries));
            await kvStore.putBatch(entries).then(async (err) => {
                console.info('DeviceKvStorePutBatchPromiseIntegerTest putBatch success');
                expect(err == undefined).assertTrue();
                await kvStore.getEntries(localDeviceId, 'batch_test_number_key').then((entrys) => {
                    console.info('DeviceKvStorePutBatchPromiseIntegerTest getEntries success');
                    expect(entrys.length == 10).assertTrue();
                    expect(entrys[0].value.value == 222).assertTrue();
                }).catch((err) => {
                    console.error('DeviceKvStorePutBatchPromiseIntegerTest getEntries fail ' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(null).assertFail();
                });
            }).catch((err) => {
                console.error('DeviceKvStorePutBatchPromiseIntegerTest putBatch fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        }catch(e) {
            console.error('DeviceKvStorePutBatchPromiseIntegerTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStorePutBatchPromiseFloatTest
     * @tc.desc Test Js Api DeviceKvStore.PutBatch() with float value
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStorePutBatchPromiseFloatTest', 0, async function (done) {
        console.info('DeviceKvStorePutBatchPromiseFloatTest');
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
            console.info('DeviceKvStorePutBatchPromiseFloatTest entries: ' + JSON.stringify(entries));
            await kvStore.putBatch(entries).then(async (err) => {
                console.info('DeviceKvStorePutBatchPromiseFloatTest putBatch success');
                expect(err == undefined).assertTrue();
                await kvStore.getEntries(localDeviceId, 'batch_test_number_key').then((entrys) => {
                    console.info('DeviceKvStorePutBatchPromiseFloatTest getEntries success');
                    expect(entrys.length == 10).assertTrue();
                    expect(entrys[0].value.value == 2.0).assertTrue();
                }).catch((err) => {
                    console.error('DeviceKvStorePutBatchPromiseFloatTest getEntries fail ' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(null).assertFail();
                });
            }).catch((err) => {
                console.error('DeviceKvStorePutBatchPromiseFloatTest putBatch fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        }catch(e) {
            console.error('DeviceKvStorePutBatchPromiseFloatTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStorePutBatchPromiseDoubleTest
     * @tc.desc Test Js Api DeviceKvStore.PutBatch() with double value
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStorePutBatchPromiseTest', 0, async function (done) {
        console.info('DeviceKvStorePutBatchPromiseTest');
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
            console.info('DeviceKvStorePutBatchPromiseTest entries: ' + JSON.stringify(entries));
            await kvStore.putBatch(entries).then(async (err) => {
                console.info('DeviceKvStorePutBatchPromiseTest putBatch success');
                expect(err == undefined).assertTrue();
                await kvStore.getEntries(localDeviceId, 'batch_test_number_key').then((entrys) => {
                    console.info('DeviceKvStorePutBatchPromiseTest getEntries success');
                    expect(entrys.length == 10).assertTrue();
                    expect(entrys[0].value.value == 2.00).assertTrue();
                }).catch((err) => {
                    console.error('DeviceKvStorePutBatchPromiseTest getEntries fail ' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(null).assertFail();
                });
            }).catch((err) => {
                console.error('DeviceKvStorePutBatchPromiseTest putBatch fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        }catch(e) {
            console.error('DeviceKvStorePutBatchPromiseTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStorePutBatchPromiseBooleanTest
     * @tc.desc Test Js Api DeviceKvStore.PutBatch() with boolean walue
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStorePutBatchPromiseBooleanTest', 0, async function (done) {
        console.info('DeviceKvStorePutBatchPromiseBooleanTest');
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
            console.info('DeviceKvStorePutBatchPromiseBooleanTest entries: ' + JSON.stringify(entries));
            await kvStore.putBatch(entries).then(async (err) => {
                console.info('DeviceKvStorePutBatchPromiseBooleanTest putBatch success');
                expect(err == undefined).assertTrue();
                await kvStore.getEntries(localDeviceId, 'batch_test_bool_key').then((entrys) => {
                    console.info('DeviceKvStorePutBatchPromiseBooleanTest getEntries success');
                    expect(entrys.length == 10).assertTrue();
                    expect(entrys[0].value.value == bo).assertTrue();
                }).catch((err) => {
                    console.error('DeviceKvStorePutBatchPromiseBooleanTest getEntries fail ' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(null).assertFail();
                });
            }).catch((err) => {
                console.error('DeviceKvStorePutBatchPromiseBooleanTest putBatch fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        }catch(e) {
            console.error('DeviceKvStorePutBatchPromiseBooleanTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStorePutBatchPromiseByteArrayTest
     * @tc.desc Test Js Api DeviceKvStore.PutBatch() with byte_array value
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStorePutBatchPromiseByteArrayTest', 0, async function (done) {
        console.info('DeviceKvStorePutBatchPromiseByteArrayTest');
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
            console.info('DeviceKvStorePutBatchPromiseByteArrayTest entries: ' + JSON.stringify(entries));
            await kvStore.putBatch(entries).then(async (err) => {
                console.info('DeviceKvStorePutBatchPromiseByteArrayTest putBatch success');
                expect(err == undefined).assertTrue();
                await kvStore.getEntries(localDeviceId, 'batch_test_bool_key').then((entrys) => {
                    console.info('DeviceKvStorePutBatchPromiseByteArrayTest getEntries success');
                    expect(entrys.length == 10).assertTrue();
                    expect(entrys[0].value.value.toString() == arr.toString()).assertTrue();
                }).catch((err) => {
                    console.error('DeviceKvStorePutBatchPromiseByteArrayTest getEntries fail ' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(null).assertFail();
                });
            }).catch((err) => {
                console.error('DeviceKvStorePutBatchPromiseByteArrayTest putBatch fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        }catch(e) {
            console.error('DeviceKvStorePutBatchPromiseBooleanTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreDeleteBatchPromiseSucTest
     * @tc.desc Test Js Api DeviceKvStore.DeleteBatch() successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreDeleteBatchPromiseSucTest', 0, async function (done) {
        console.info('DeviceKvStoreDeleteBatchPromiseSucTest');
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
            console.info('DeviceKvStoreDeleteBatchPromiseSucTest entries: ' + JSON.stringify(entries));
            await kvStore.putBatch(entries).then(async (err) => {
                console.info('DeviceKvStoreDeleteBatchPromiseSucTest putBatch success');
                expect(err == undefined).assertTrue();
                await kvStore.deleteBatch(keys).then((err) => {
                    console.info('DeviceKvStoreDeleteBatchPromiseSucTest deleteBatch success');
                    expect(err == undefined).assertTrue();
                }).catch((err) => {
                    console.error('DeviceKvStoreDeleteBatchPromiseSucTest deleteBatch fail ' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(null).assertFail();
                });
            }).catch((err) => {
                console.error('DeviceKvStoreDeleteBatchPromiseSucTest putBatch fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        }catch(e) {
            console.error('DeviceKvStoreDeleteBatchPromiseSucTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreDeleteBatchPromiseNoBatchTest
     * @tc.desc Test Js Api DeviceKvStore.DeleteBatch() delete non-existing batches
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreDeleteBatchPromiseTest', 0, async function (done) {
        console.info('DeviceKvStoreDeleteBatchPromiseTest');
        try {
            let keys = ['batch_test_string_key1', 'batch_test_string_key2'];
            await kvStore.deleteBatch(keys).then((err) => {
                console.info('DeviceKvStoreDeleteBatchPromiseTest deleteBatch success');
            }).catch((err) => {
                console.error('DeviceKvStoreDeleteBatchPromiseTest deleteBatch fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        }catch(e) {
            console.error('DeviceKvStoreDeleteBatchPromiseTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreDeleteBatchPromiseWrongKeysTest
     * @tc.desc Test Js Api DeviceKvStore.DeleteBatch() with wrong keys
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreDeleteBatchPromiseWrongKeysTest', 0, async function (done) {
        console.info('DeviceKvStoreDeleteBatchPromiseWrongKeysTest');
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
            console.info('DeviceKvStoreDeleteBatchPromiseWrongKeysTest entries: ' + JSON.stringify(entries));
            await kvStore.putBatch(entries).then(async (err) => {
                console.info('DeviceKvStoreDeleteBatchPromiseWrongKeysTest putBatch success');
                expect(err == undefined).assertTrue();
                let keys = ['batch_test_string_key1', 'batch_test_string_keya'];
                await kvStore.deleteBatch(keys).then((err) => {
                    console.info('DeviceKvStoreDeleteBatchPromiseWrongKeysTest deleteBatch success');
                }).catch((err) => {
                    console.error('DeviceKvStoreDeleteBatchPromiseWrongKeysTest deleteBatch fail ' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(null).assertFail();
                });
            }).catch((err) => {
                console.error('DeviceKvStoreDeleteBatchPromiseWrongKeysTest putBatch fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        }catch(e) {
            console.error('DeviceKvStoreDeleteBatchPromiseWrongKeysTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStorestartTransactionPromiseCommitTest
     * @tc.desc Test Js Api DeviceKvStore.startTransaction() Commit kvstore
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStorestartTransactionPromiseCommitTest', 0, async function (done) {
        console.info('DeviceKvStorestartTransactionPromiseCommitTest');
        try {
            var count = 0;
            kvStore.on('dataChange', 0, function (data) {
                console.info('DeviceKvStorestartTransactionPromiseCommitTest' + JSON.stringify(data))
                count++;
            });
            await kvStore.startTransaction().then(async (err) => {
                console.info('DeviceKvStorestartTransactionPromiseCommitTest startTransaction success');
                expect(err == undefined).assertTrue();
            }).catch((err) => {
                console.error('DeviceKvStorestartTransactionPromiseCommitTest startTransaction fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
            let entries = putBatchString(10, 'batch_test_string_key');
            console.info('DeviceKvStorestartTransactionPromiseCommitTest entries: ' + JSON.stringify(entries));
            await kvStore.putBatch(entries).then(async (err) => {
                console.info('DeviceKvStorestartTransactionPromiseCommitTest putBatch success');
                expect(err == undefined).assertTrue();
            }).catch((err) => {
                console.error('DeviceKvStorestartTransactionPromiseCommitTest putBatch fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
            let keys = Object.keys(entries).slice(5);
            await kvStore.deleteBatch(keys).then((err) => {
                console.info('DeviceKvStorestartTransactionPromiseCommitTest deleteBatch success');
                expect(err == undefined).assertTrue();
            }).catch((err) => {
                console.error('DeviceKvStorestartTransactionPromiseCommitTest deleteBatch fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
            await kvStore.commit().then(async (err) => {
                console.info('DeviceKvStorestartTransactionPromiseCommitTest commit success');
                expect(err == undefined).assertTrue();
            }).catch((err) => {
                console.error('DeviceKvStorestartTransactionPromiseCommitTest commit fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
            await sleep(2000);
            expect(count == 1).assertTrue();
        }catch(e) {
            console.error('DeviceKvStorestartTransactionPromiseCommitTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStorestartTransactionPromiseRollbackTest
     * @tc.desc Test Js Api DeviceKvStore.startTransaction() rollback kvstore
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStorestartTransactionPromiseRollbackTest', 0, async function (done) {
        console.info('DeviceKvStorestartTransactionPromiseRollbackTest');
        try {
            var count = 0;
            kvStore.on('dataChange', 0, function (data) {
                console.info('DeviceKvStorestartTransactionPromiseRollbackTest' + JSON.stringify(data))
                count++;
            });
            await kvStore.startTransaction().then(async (err) => {
                console.info('DeviceKvStorestartTransactionPromiseRollbackTest startTransaction success');
                expect(err == undefined).assertTrue();
            }).catch((err) => {
                console.error('DeviceKvStorestartTransactionPromiseRollbackTest startTransaction fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
            let entries = putBatchString(10, 'batch_test_string_key');
            console.info('DeviceKvStorestartTransactionPromiseRollbackTest entries: ' + JSON.stringify(entries));
            await kvStore.putBatch(entries).then(async (err) => {
                console.info('DeviceKvStorestartTransactionPromiseRollbackTest putBatch success');
                expect(err == undefined).assertTrue();
            }).catch((err) => {
                console.error('DeviceKvStorestartTransactionPromiseRollbackTest putBatch fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
            let keys = Object.keys(entries).slice(5);
            await kvStore.deleteBatch(keys).then((err) => {
                console.info('DeviceKvStorestartTransactionPromiseRollbackTest deleteBatch success');
                expect(err == undefined).assertTrue();
            }).catch((err) => {
                console.error('DeviceKvStorestartTransactionPromiseRollbackTest deleteBatch fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
            await kvStore.rollback().then(async (err) => {
                console.info('DeviceKvStorestartTransactionPromiseRollbackTest rollback success');
                expect(err == undefined).assertTrue();
            }).catch((err) => {
                console.error('DeviceKvStorestartTransactionPromiseRollbackTest rollback fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
            await sleep(2000);
            expect(count == 0).assertTrue();
        }catch(e) {
            console.error('DeviceKvStorestartTransactionPromiseRollbackTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStorestartTransactionPromiseInvalidArgsTest
     * @tc.desc Test Js Api DeviceKvStore.startTransaction() with invalid args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStorestartTransactionPromiseInvalidArgsTest', 0, async function (done) {
        console.info('DeviceKvStorestartTransactionPromiseRollbackTest');
        try {
            await kvStore.startTransaction(1).then(async (err) => {
                console.info('DeviceKvStorestartTransactionPromiseInvalidArgsTest startTransaction success');
                expect(null).assertFail();
            }).catch((err) => {
                console.error('DeviceKvStorestartTransactionPromiseInvalidArgsTest startTransaction fail ' + `, error code is ${err.code}, message is ${err.message}`);
            });
        }catch(e) {
            console.error('DeviceKvStorestartTransactionPromiseInvalidArgsTest e ' + `, error code is ${e.code}, message is ${e.message}`);
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreCommitPromiseInvalidArgsTest
     * @tc.desc Test Js Api DeviceKvStore.Commit() with invalid args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreCommitPromiseInvalidArgsTest', 0, async function (done) {
        console.info('DeviceKvStoreCommitPromiseInvalidArgsTest');
        try {
            await kvStore.commit(1).then(async (err) => {
                console.info('DeviceKvStoreCommitPromiseInvalidArgsTest commit success');
                expect(null).assertFail();
            }).catch((err) => {
                console.error('DeviceKvStoreCommitPromiseInvalidArgsTest commit fail ' + `, error code is ${err.code}, message is ${err.message}`);
            });
        }catch(e) {
            console.error('DeviceKvStoreCommitPromiseInvalidArgsTest e ' + `, error code is ${e.code}, message is ${e.message}`);
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreRollbackPromiseInvalidArgsTest
     * @tc.desc Test Js Api DeviceKvStore.Rollback() testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreRollbackPromiseInvalidArgsTest', 0, async function (done) {
        console.info('DeviceKvStoreRollbackPromiseInvalidArgsTest');
        try {
            await kvStore.rollback(1).then(async (err) => {
                console.info('DeviceKvStoreRollbackPromiseInvalidArgsTest rollback success');
                expect(null).assertFail();
            }).catch((err) => {
                console.error('DeviceKvStoreRollbackPromiseInvalidArgsTest rollback fail ' + `, error code is ${err.code}, message is ${err.message}`);
            });
        }catch(e) {
            console.error('DeviceKvStoreRollbackPromiseInvalidArgsTest e ' + `, error code is ${e.code}, message is ${e.message}`);
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreEnableSyncPromiseTrueTest
     * @tc.desc Test Js Api DeviceKvStore.EnableSync() true successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreEnableSyncPromiseTrueTest', 0, async function (done) {
        console.info('DeviceKvStoreEnableSyncPromiseTrueTest');
        try {
            await kvStore.enableSync(true).then((err) => {
                console.info('DeviceKvStoreEnableSyncPromiseTrueTest enableSync success');
                expect(err == undefined).assertTrue();
            }).catch((err) => {
                console.error('DeviceKvStoreEnableSyncPromiseTrueTest enableSync fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        }catch(e) {
            console.error('DeviceKvStoreEnableSyncPromiseTrueTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreEnableSyncPromiseFalseTest
     * @tc.desc Test Js Api DeviceKvStore.EnableSync() false successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreEnableSyncPromiseFalseTest', 0, async function (done) {
        console.info('DeviceKvStoreEnableSyncPromiseFalseTest');
        try {
            await kvStore.enableSync(false).then((err) => {
                console.info('DeviceKvStoreEnableSyncPromiseFalseTest enableSync success');
                expect(err == undefined).assertTrue();
            }).catch((err) => {
                console.error('DeviceKvStoreEnableSyncPromiseFalseTest enableSync fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        }catch(e) {
            console.error('DeviceKvStoreEnableSyncPromiseFalseTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreEnableSyncPromiseNoArgsTest
     * @tc.desc Test Js Api DeviceKvStore.EnableSync() with no args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreEnableSyncPromiseNoArgsTest', 0, async function (done) {
        console.info('DeviceKvStoreEnableSyncPromiseNoArgsTest');
        try {
            await kvStore.enableSync().then((err) => {
                console.info('DeviceKvStoreEnableSyncPromiseNoArgsTest enableSync success');
                expect(null).assertFail();
            }).catch((err) => {
                console.error('DeviceKvStoreEnableSyncPromiseNoArgsTest enableSync fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        }catch(e) {
            console.error('DeviceKvStoreEnableSyncPromiseNoArgsTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(e.code == 401).assertTrue();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreEnableSyncPromiseInvalidArgsTest
     * @tc.desc Test Js Api DeviceKvStore.EnableSync() with invalid args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreEnableSyncPromiseInvalidArgsTest', 0, async function (done) {
        console.info('DeviceKvStoreEnableSyncPromiseInvalidArgsTest');
        try {
            await kvStore.enableSync(null).then((err) => {
                console.info('DeviceKvStoreEnableSyncPromiseInvalidArgsTest enableSync success');
                expect(null).assertFail();
            }).catch((err) => {
                console.error('DeviceKvStoreEnableSyncPromiseInvalidArgsTest enableSync fail ' + `, error code is ${err.code}, message is ${err.message}`);
            });
        }catch(e) {
            console.info('DeviceKvStoreEnableSyncPromiseInvalidArgsTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(e.code == 401).assertTrue();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreRemoveDeviceDataPromiseWrongDeviceIdTest
     * @tc.desc Test Js Api DeviceKvStore.RemoveDeviceData() with wrong device id
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreRemoveDeviceDataPromiseWrongDeviceIdTest', 0, async function (done) {
        console.info('DeviceKvStoreRemoveDeviceDataPromiseWrongDeviceIdTest');
        try {
            await kvStore.put(KEY_TEST_STRING_ELEMENT, VALUE_TEST_STRING_ELEMENT).then((err) => {
                console.info('DeviceKvStoreRemoveDeviceDataPromiseWrongDeviceIdTest put success');
                expect(err == undefined).assertTrue();
            }).catch((err) => {
                console.error('DeviceKvStoreRemoveDeviceDataPromiseWrongDeviceIdTest put fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
            var deviceid = 'no_exist_device_id';
            await kvStore.removeDeviceData(deviceid).then((err) => {
                console.info('DeviceKvStoreRemoveDeviceDataPromiseWrongDeviceIdTest removeDeviceData success');
                expect(null).assertFail();
            }).catch((err) => {
                console.error('DeviceKvStoreRemoveDeviceDataPromiseWrongDeviceIdTest removeDeviceData fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(err.code == 401).assertTrue();
            });
            await kvStore.get(localDeviceId, KEY_TEST_STRING_ELEMENT).then((data) => {
                console.info('DeviceKvStoreRemoveDeviceDataPromiseWrongDeviceIdTest get success data:' + data);
                expect(data == VALUE_TEST_STRING_ELEMENT).assertTrue();
            }).catch((err) => {
                console.error('DeviceKvStoreRemoveDeviceDataPromiseWrongDeviceIdTest get fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        }catch(e) {
            console.error('DeviceKvStoreRemoveDeviceDataPromiseWrongDeviceIdTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreRemoveDeviceDataPromiseWrongArgsTest
     * @tc.desc Test Js Api DeviceKvStore.RemoveDeviceData() with wrong args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreRemoveDeviceDataPromiseWrongArgsTest', 0, async function (done) {
        console.info('DeviceKvStoreRemoveDeviceDataPromiseWrongArgsTest');
        try {
            await kvStore.removeDeviceData().then((err) => {
                console.info('DeviceKvStoreRemoveDeviceDataPromiseWrongArgsTest removeDeviceData success');
                expect(null).assertFail();
            }).catch((err) => {
                console.error('DeviceKvStoreRemoveDeviceDataPromiseWrongArgsTest removeDeviceData fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        }catch(e) {
            console.error('DeviceKvStoreRemoveDeviceDataPromiseWrongArgsTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(e.code == 401).assertTrue();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreRemoveDeviceDataPromiseInvalidArgsTest
     * @tc.desc Test Js Api DeviceKvStore.RemoveDeviceData() with invalid args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
     it('DeviceKvStoreRemoveDeviceDataPromiseInvalidArgsTest', 0, async function (done) {
        console.info('DeviceKvStoreRemoveDeviceDataPromiseInvalidArgsTest');
        try {
            await kvStore.removeDeviceData('').then((data) => {
                console.info('DeviceKvStoreRemoveDeviceDataPromiseInvalidArgsTest removeDeviceData success');
                expect(null).assertFail();
            }).catch((err) => {
                console.error('DeviceKvStoreRemoveDeviceDataPromiseInvalidArgsTest removeDeviceData fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        }catch(e) {
            console.error('DeviceKvStoreRemoveDeviceDataPromiseInvalidArgsTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(e.code == 401).assertTrue();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoregetResultSetPromiseSucTest
     * @tc.desc Test Js Api DeviceKvStore.getResultSet() successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoregetResultSetPromiseSucTest', 0, async function (done) {
        console.info('DeviceKvStoregetResultSetPromiseSucTest');
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
                console.info('DeviceKvStoregetResultSetPromiseSucTest putBatch success');
                expect(err == undefined).assertTrue();
            }).catch((err) => {
                console.error('DeviceKvStorePutBatchPromiseStringTest putBatch fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
            await kvStore.getResultSet(localDeviceId, 'batch_test_string_key').then((result) => {
                console.info('DeviceKvStoregetResultSetPromiseSucTest getResultSet success');
                resultSet = result;
                expect(resultSet.getCount() == 10).assertTrue();
            }).catch((err) => {
                console.error('DeviceKvStoregetResultSetPromiseSucTest getResultSet fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
            await kvStore.closeResultSet(resultSet).then((err) => {
                console.info('DeviceKvStoregetResultSetPromiseSucTest closeResultSet success');
                expect(err == undefined).assertTrue();
            }).catch((err) => {
                console.error('DeviceKvStoregetResultSetPromiseSucTest closeResultSet fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        }catch(e) {
            console.error('DeviceKvStoregetResultSetPromiseSucTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoregetResultSetPromiseEmptyTest
     * @tc.desc Test Js Api DeviceKvStore.getResultSet() get empty resultset
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoregetResultSetPromiseEmptyTest', 0, async function (done) {
        console.info('DeviceKvStoregetResultSetPromiseEmptyTest');
        try {
            let resultSet;
            await kvStore.getResultSet(localDeviceId, 'batch_test_string_key').then((result) => {
                console.info('DeviceKvStoregetResultSetPromiseEmptyTest getResultSet success');
                resultSet = result;
                expect(resultSet.getCount() == 0).assertTrue();
            }).catch((err) => {
                console.error('DeviceKvStoregetResultSetPromiseEmptyTest getResultSet fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
            await kvStore.closeResultSet(resultSet).then((err) => {
                console.info('DeviceKvStoregetResultSetPromiseEmptyTest closeResultSet success');
                expect(err == undefined).assertTrue();
            }).catch((err) => {
                console.error('DeviceKvStoregetResultSetPromiseEmptyTest closeResultSet fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        }catch(e) {
            console.error('DeviceKvStoregetResultSetPromiseEmptyTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoregetResultSetPromiseNoArgsTest
     * @tc.desc Test Js Api DeviceKvStore.getResultSet() with no args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoregetResultSetPromiseNoArgsTest', 0, async function (done) {
        console.info('DeviceKvStoregetResultSetPromiseNoArgsTest');
        try {
            let resultSet;
            await kvStore.getResultSet().then((result) => {
                console.info('DeviceKvStoregetResultSetPromiseNoArgsTest getResultSet success');
                expect(null).assertFail();
            }).catch((err) => {
                console.error('DeviceKvStoregetResultSetPromiseNoArgsTest getResultSet fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        }catch(e) {
            console.error('DeviceKvStoregetResultSetPromiseNoArgsTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(e.code == 401).assertTrue();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoregetResultSetPromiseInvalidArgsTest
     * @tc.desc Test Js Api DeviceKvStore.getResultSet() with invalid args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoregetResultSetPromiseInvalidArgsTest', 0, async function (done) {
        console.info('DeviceKvStoregetResultSetPromiseInvalidArgsTest');
        try {
            let resultSet;
            await kvStore.getResultSet('test_key_string', 123).then((result) => {
                console.info('DeviceKvStoregetResultSetPromiseInvalidArgsTest getResultSet success');
                expect(null).assertFail();
            }).catch((err) => {
                console.error('DeviceKvStoregetResultSetPromiseInvalidArgsTest getResultSet fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        }catch(e) {
            console.error('DeviceKvStoregetResultSetPromiseInvalidArgsTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(e.code == 401).assertTrue();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoregetResultSetPromisePrefixKeyTest
     * @tc.desc Test Js Api DeviceKvStore.getResultSet() with prefixkey
     * @tc.require: issueNumber
     */
    it('DeviceKvStoregetResultSetPromisePrefixKeyTest', 0, async function (done) {
        console.info('DeviceKvStoregetResultSetPromisePrefixKeyTest');
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
                console.info('DeviceKvStoregetResultSetPromisePrefixKeyTest putBatch success');
                expect(err == undefined).assertTrue();
            }).catch((err) => {
                console.error('DeviceKvStorePutBatchPromiseStringTest putBatch fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
            var query = new factory.Query();
            query.prefixKey("batch_test");
            await kvStore.getResultSet(localDeviceId, query).then((result) => {
                console.info('DeviceKvStoregetResultSetPromisePrefixKeyTest getResultSet success');
                resultSet = result;
                expect(resultSet.getCount() == 10).assertTrue();
            }).catch((err) => {
                console.error('DeviceKvStoregetResultSetPromisePrefixKeyTest getResultSet fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
            query.deviceId(localDeviceId);
            console.info("testDeviceKvStoreGetResultSet005 " + query.getSqlLike());
            await kvStore.closeResultSet(resultSet).then((err) => {
                console.info('DeviceKvStoregetResultSetPromisePrefixKeyTest closeResultSet success');
                expect(err == undefined).assertTrue();
            }).catch((err) => {
                console.error('DeviceKvStoregetResultSetPromisePrefixKeyTest closeResultSet fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        }catch(e) {
            console.error('DeviceKvStoregetResultSetPromisePrefixKeyTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoregetResultSetPromiseQueryDeviceIdTest
     * @tc.desc Test Js Api DeviceKvStore.getResultSet() device id in query
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoregetResultSetPromiseQueryDeviceIdTest', 0, async function (done) {
        console.info('DeviceKvStoregetResultSetPromiseQueryDeviceIdTest');
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
                console.info('DeviceKvStoregetResultSetPromiseQueryDeviceIdTest putBatch success');
                expect(err == undefined).assertTrue();
            }).catch((err) => {
                console.error('DeviceKvStorePutBatchPromiseStringTest putBatch fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
            var query = new factory.Query();
            query.deviceId(localDeviceId);
            query.prefixKey("batch_test");
            console.info("testDeviceKvStoreGetResultSet006 " + query.getSqlLike());
            await kvStore.getResultSet(query).then((result) => {
                console.info('DeviceKvStoregetResultSetPromiseQueryDeviceIdTest getResultSet success');
                resultSet = result;
                expect(resultSet.getCount() == 10).assertTrue();
            }).catch((err) => {
                console.error('DeviceKvStoregetResultSetPromiseQueryDeviceIdTest getResultSet fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
            await kvStore.closeResultSet(resultSet).then((err) => {
                console.info('DeviceKvStoregetResultSetPromiseQueryDeviceIdTest closeResultSet success');
                expect(err == undefined).assertTrue();
            }).catch((err) => {
                console.error('DeviceKvStoregetResultSetPromiseQueryDeviceIdTest closeResultSet fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        }catch(e) {
            console.error('DeviceKvStoregetResultSetPromiseQueryDeviceIdTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreGetResultSetPromisePredicatesTest
     * @tc.desc Test Js Api DeviceKvStore.GetResultSet() with predicates
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
     it('DeviceKvStoreGetResultSetPromisePredicatesTest', 0, async function (done) {
        console.info('DeviceKvStoreGetResultSetPromisePredicatesTest');
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
            await kvStore.putBatch(entries).then( async (err) => {
                console.info('DeviceKvStoreGetResultSetPromisePredicatesTest putBatch success');
                expect(err == undefined).assertTrue();
                let predicates = new dataSharePredicates.DataSharePredicates();
                predicates.inKeys("batch_test");
                await kvStore.getResultSet(localDeviceId, predicates).then( async (result) => {
                    console.info('DeviceKvStoreGetResultSetPromisePredicatesTest getResultSet success');
                    resultSet = result;
                    expect(resultSet.getCount() == 10).assertTrue();
                    await kvStore.closeResultSet(resultSet).then(err => {
                        console.info('DeviceKvStoreGetResultSetPromisePredicatesTest closeResultSet success');
                        expect(err == undefined).assertTrue();
                        done();
                    })
                });
            });
        } catch(e) {
            console.error('DeviceKvStoreGetResultSetPromisePredicatesTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStoreCloseResultSetPromiseCloseNullTest
     * @tc.desc Test Js Api DeviceKvStore.CloseResultSet() close null resultset
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreCloseResultSetPromiseCloseNullTest', 0, async function (done) {
        console.info('DeviceKvStoreCloseResultSetPromiseCloseNullTest');
        try {
            console.info('DeviceKvStoreCloseResultSetPromiseCloseNullTest success');
            let resultSet = null;
            await kvStore.closeResultSet(resultSet).then(() => {
                console.info('DeviceKvStoreCloseResultSetPromiseCloseNullTest closeResultSet success');
                expect(null).assertFail();
            }).catch((err) => {
                console.error('DeviceKvStoreCloseResultSetPromiseCloseNullTest closeResultSet fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        }catch(e) {
            console.error('DeviceKvStoreCloseResultSetPromiseCloseNullTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(e.code == 401).assertTrue();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreCloseResultSetPromiseSucTest
     * @tc.desc Test Js Api DeviceKvStore.CloseResultSet() successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreCloseResultSetPromiseSucTest', 0, async function (done) {
        console.info('DeviceKvStoreCloseResultSetPromiseSucTest');
        try {
            console.info('DeviceKvStoreCloseResultSetPromiseSucTest success');
            let resultSet = null;
            await kvStore.getResultSet(localDeviceId, 'batch_test_string_key').then((result) => {
                console.info('DeviceKvStoreCloseResultSetPromiseSucTest getResultSet success');
                resultSet = result;
            }).catch((err) => {
                console.error('DeviceKvStoreCloseResultSetPromiseSucTest getResultSet fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
            await kvStore.closeResultSet(resultSet).then((err) => {
                console.info('DeviceKvStoreCloseResultSetPromiseSucTest closeResultSet success');
                expect(err == undefined).assertTrue();
            }).catch((err) => {
                console.error('DeviceKvStoreCloseResultSetPromiseSucTest closeResultSet fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        }catch(e) {
            console.error('DeviceKvStoreCloseResultSetPromiseSucTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreCloseResultSetPromiseNoArgsTest
     * @tc.desc Test Js Api DeviceKvStore.CloseResultSet() with no args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreCloseResultSetPromiseNoArgsTest', 0, async function (done) {
        console.info('DeviceKvStoreCloseResultSetPromiseNoArgsTest');
        try {
            console.info('DeviceKvStoreCloseResultSetPromiseNoArgsTest success');
            let resultSet = null;
            await kvStore.closeResultSet().then(() => {
                console.info('DeviceKvStoreCloseResultSetPromiseNoArgsTest closeResultSet success');
                expect(null).assertFail();
            }).catch((err) => {
                console.error('DeviceKvStoreCloseResultSetPromiseNoArgsTest closeResultSet fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        }catch(e) {
            console.error('DeviceKvStoreCloseResultSetPromiseNoArgsTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(e.code == 401).assertTrue();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreCloseResultSetPromiseWrongArgsTest
     * @tc.desc Test Js Api DeviceKvStore.CloseResultSet() with wrong args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreCloseResultSetPromiseTest', 0, async function (done) {
        console.info('DeviceKvStoreCloseResultSetPromiseTest');
        let errorInfo;
        try{
            kvStore.closeResultSet(1).then(() => {
                expect(null).assertFail();
            }).catch((err) => {
                console.error('DeviceKvStoreCloseResultSetPromiseTest failed');
                expect(null).assertFail();
            })
        }catch (e) {
            console.error('DeviceKvStoreCloseResultSetPromiseTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            errorInfo = e;
            expect(e.code == 401).assertTrue();
        }
        expect(errorInfo != undefined).assertEqual(true);
        done();
    })

    /**
     * @tc.name DeviceKvStoreGetResultSizePromiseSucTest
     * @tc.desc Test Js Api DeviceKvStore.Get(ResultSize) successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreGetResultSizePromiseSucTest', 0, async function (done) {
        console.info('DeviceKvStoreGetResultSizePromiseSucTest');
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
                console.info('DeviceKvStoreGetResultSizePromiseSucTest putBatch success');
                expect(err == undefined).assertTrue();
            }).catch((err) => {
                console.error('DeviceKvStorePutBatchPromiseStringTest putBatch fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
            var query = new factory.Query();
            query.prefixKey("batch_test");
            query.deviceId(localDeviceId);
            await kvStore.getResultSize(query).then((resultSize) => {
                console.info('DeviceKvStoreGetResultSizePromiseSucTest getResultSet success');
                expect(resultSize == 10).assertTrue();
            }).catch((err) => {
                console.error('DeviceKvStoreGetResultSizePromiseSucTest getResultSet fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        }catch(e) {
            console.error('DeviceKvStoreGetResultSizePromiseSucTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreGetResultSizePromiseQueryTest
     * @tc.desc Test Js Api DeviceKvStore.Get(ResultSize) with query
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreGetResultSizePromiseQueryTest', 0, async function (done) {
        console.info('DeviceKvStoreGetResultSizePromiseQueryTest');
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
                console.info('DeviceKvStoreGetResultSizePromiseQueryTest putBatch success');
                expect(err == undefined).assertTrue();
            }).catch((err) => {
                console.error('DeviceKvStoreGetResultSizePromiseQueryTest putBatch fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
            var query = new factory.Query();
            query.prefixKey("batch_test");
            await kvStore.getResultSize(localDeviceId, query).then((resultSize) => {
                console.info('DeviceKvStoreGetResultSizePromiseQueryTest getResultSet success');
                expect(resultSize == 10).assertTrue();
            }).catch((err) => {
                console.error('DeviceKvStoreGetResultSizePromiseQueryTest getResultSet fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        }catch(e) {
            console.error('DeviceKvStoreGetResultSizePromiseQueryTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreGetEntriesPromiseSucTest
     * @tc.desc Test Js Api DeviceKvStore.GetEntries() successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreGetEntriesPromiseSucTest', 0, async function (done) {
        console.info('DeviceKvStoreGetEntriesPromiseSucTest');
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
            console.info('DeviceKvStoreGetEntriesPromiseSucTest entries: ' + JSON.stringify(entries));
            await kvStore.putBatch(entries).then(async (err) => {
                console.info('DeviceKvStoreGetEntriesPromiseSucTest putBatch success');
                expect(err == undefined).assertTrue();
                var query = new factory.Query();
                query.deviceId(localDeviceId);
                query.prefixKey("batch_test");
                await kvStore.getEntries(localDeviceId, query).then((entrys) => {
                    console.info('DeviceKvStoreGetEntriesPromiseSucTest getEntries success');
                    console.info(entrys.length);
                    console.info(entrys[0].value.value.toString());
                    expect(entrys.length == 10).assertTrue();
                    expect(entrys[0].value.value.toString() == arr.toString()).assertTrue();
                }).catch((err) => {
                    console.error('DeviceKvStoreGetEntriesPromiseSucTest getEntries fail ' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(null).assertFail();
                });
            }).catch((err) => {
                console.error('DeviceKvStoreGetEntriesPromiseSucTest putBatch fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
            console.info('DeviceKvStoreGetEntriesPromiseSucTest success');
        }catch(e) {
            console.error('DeviceKvStoreGetEntriesPromiseSucTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreGetEntriesPromiseQueryTest
     * @tc.desc Test Js Api DeviceKvStore.GetEntries() with query
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreGetEntriesPromiseQueryTest', 0, async function (done) {
        console.info('DeviceKvStoreGetEntriesPromiseQueryTest');
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
            console.info('DeviceKvStoreGetEntriesPromiseQueryTest entries: ' + JSON.stringify(entries));
            await kvStore.putBatch(entries).then(async (err) => {
                console.info('DeviceKvStoreGetEntriesPromiseQueryTest putBatch success');
                expect(err == undefined).assertTrue();
                var query = new factory.Query();
                query.prefixKey("batch_test");
                query.deviceId(localDeviceId);
                await kvStore.getEntries(query).then((entrys) => {
                    console.info('DeviceKvStoreGetEntriesPromiseQueryTest getEntries success');
                    expect(entrys.length == 10).assertTrue();
                    expect(entrys[0].value.value.toString() == arr.toString()).assertTrue();
                }).catch((err) => {
                    console.error('DeviceKvStoreGetEntriesPromiseQueryTest getEntries fail ' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(null).assertFail();
                });
            }).catch((err) => {
                console.error('DeviceKvStoreGetEntriesPromiseQueryTest putBatch fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
            console.info('DeviceKvStoreGetEntriesPromiseQueryTest success');
        }catch(e) {
            console.error('DeviceKvStoreGetEntriesPromiseQueryTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })
})
