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
import dataShare from '@ohos.data.dataSharePredicates'
import abilityFeatureAbility from '@ohos.ability.featureAbility'

var context = abilityFeatureAbility.getContext();
const KEY_TEST_INT_ELEMENT = 'key_test_int';
const KEY_TEST_FLOAT_ELEMENT = 'key_test_float';
const KEY_TEST_BOOLEAN_ELEMENT = 'key_test_boolean';
const KEY_TEST_STRING_ELEMENT = 'key_test_string';
const KEY_TEST_SYNC_ELEMENT = 'key_test_sync';
const file = "";
const files = [file];

const VALUE_TEST_INT_ELEMENT = 123;
const VALUE_TEST_FLOAT_ELEMENT = 321.12;
const VALUE_TEST_BOOLEAN_ELEMENT = true;
const VALUE_TEST_STRING_ELEMENT = 'value-string-001';
const VALUE_TEST_SYNC_ELEMENT = 'value-string-001';

const TEST_BUNDLE_NAME = 'com.example.myapplication';
const TEST_STORE_ID = 'storeId2';
var kvManager = null;
var kvStore = null;
var localDeviceId = null;
const USED_DEVICE_IDS = ['A12C1F9261528B21F95778D2FDC0B2E33943E6251AC5487F4473D005758905DB'];
const UNUSED_DEVICE_IDS = [];  /* add you test device-ids here */
var syncDeviceIds = USED_DEVICE_IDS.concat(UNUSED_DEVICE_IDS);

function sleep(ms) {
    return new Promise(resolve => setTimeout(resolve, ms));
}

function putBatchString(len, prefix) {
    let entries = [];
    for (var i = 0; i < len; i++) {
        var entry = {
            key: prefix + i,
            value: {
                type: factory.ValueType.STRING,
                value: 'batch_test_string_value'
            }
        }
        entries.push(entry);
    }
    return entries;
}

describe('DeviceKvStorePromiseTest', function () {
    const config = {
        bundleName: TEST_BUNDLE_NAME,
        context: context
    }

    const options = {
        createIfMissing: true,
        encrypt: false,
        backup: true,
        autoSync: true,
        kvStoreType: factory.KVStoreType.DEVICE_COLLABORATION,
        schema: '',
        securityLevel: factory.SecurityLevel.S2,
    }

    beforeAll(async function (done) {
        console.info('beforeAll config:' + JSON.stringify(config));
        kvManager = factory.createKVManager(config);
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
        await getDeviceId.then(function (deviceId) {
            console.info('beforeAll getDeviceId ' + JSON.stringify(deviceId));
            localDeviceId = deviceId;
        }).catch((error) => {
            console.error('beforeAll can NOT getDeviceId, fail: ' + `, error code is ${error.code}, message is ${error.message}`);
            expect(null).assertFail();
        });
        await kvManager.closeKVStore(TEST_BUNDLE_NAME, TEST_STORE_ID);
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
        await kvManager.getKVStore(TEST_STORE_ID, options, function (err, store) {
            kvStore = store;
            console.info('beforeEach getKVStore success');
            done();
        });
    })

    afterEach(async function (done) {
        console.info('afterEach');
        try {
            await kvManager.closeKVStore(TEST_BUNDLE_NAME, TEST_STORE_ID, async function (err, data) {
                console.info('afterEach closeKVStore success: err is: ' + err);
                await kvManager.deleteKVStore(TEST_BUNDLE_NAME, TEST_STORE_ID, function (err, data) {
                    console.info('afterEach deleteKVStore success err is: ' + err);
                    done();
                });
            });
            kvStore = null;
        } catch (e) {
            console.error('afterEach closeKVStore err ' + `, error code is ${err.code}, message is ${err.message}`);
        }
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
            await kvStore.put(KEY_TEST_STRING_ELEMENT, null).then((data) => {
                console.info('DeviceKvStorePutStringPromiseInvalidArgsTest put success');
                expect(null).assertFail();
            }).catch((error) => {
                console.error('DeviceKvStorePutStringPromiseInvalidArgsTest put error' + `, error code is ${error.code}, message is ${error.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('DeviceKvStorePutStringPromiseInvalidArgsTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(e.code == 401).assertTrue();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStorePutStringPromiseClosedKVStoreTest
     * @tc.desc Test Js Api DeviceKvStore.Put(String) in a closed kvstore
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStorePutStringPromiseClosedKVStoreTest', 0, async function (done) {
        console.info('DeviceKvStorePutStringPromiseClosedKVStoreTest');
        try {
            await kvManager.closeKVStore(TEST_BUNDLE_NAME, TEST_STORE_ID).then(async () => {
                await kvManager.deleteKVStore(TEST_BUNDLE_NAME, TEST_STORE_ID).then(() => {
                    expect(true).assertTrue();
                }).catch((err) => {
                    console.error('deleteKVStore err ' + `, error code is ${err.code}, message is ${err.message}`);
                });
            })
            await kvStore.put(KEY_TEST_STRING_ELEMENT, VALUE_TEST_STRING_ELEMENT).then((data) => {
                console.info('DeviceKvStorePutStringPromiseClosedKVStoreTest put success');
                expect(null).assertFail();
            }).catch((error) => {
                console.error('DeviceKvStorePutStringPromiseClosedKVStoreTest put error' + `, error code is ${error.code}, message is ${error.message}`);
                expect(error.code == 15100005).assertTrue();
            });
        } catch (e) {
            console.error('DeviceKvStorePutStringPromiseClosedKVStoreTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStorePutStringPromiseSucTest
     * @tc.desc Test Js Api DeviceKvStore.Put(String) success
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
     * @tc.name DeviceKvStoreGetStringPromiseSucTest
     * @tc.desc Test Js Api DeviceKvStoreGetString success
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreGetStringPromiseSucTest', 0, async function (done) {
        console.info('DeviceKvStoreGetStringPromiseSucTest');
        try {
            await kvStore.put(KEY_TEST_STRING_ELEMENT, VALUE_TEST_STRING_ELEMENT).then(async (data) => {
                expect(data == undefined).assertTrue();
                await kvStore.get(localDeviceId, KEY_TEST_STRING_ELEMENT).then((data) => {
                    expect(VALUE_TEST_STRING_ELEMENT == data).assertTrue();
                }).catch((err) => {
                    expect(null).assertFail();
                });
            }).catch((error) => {
                expect(null).assertFail();
            });
        } catch (e) {
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreGetStringPromiseInvalidArgsTest
     * @tc.desc Test Js Api DeviceKvStoreGetString with invalid args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreGetStringPromiseInvalidArgsTest', 0, async function (done) {
        try {
            await kvStore.put(KEY_TEST_STRING_ELEMENT, VALUE_TEST_STRING_ELEMENT).then(async (data) => {
                expect(data == undefined).assertTrue();
                await kvStore.get().then((data) => {
                    expect(null).assertFail();
                }).catch((err) => {
                    expect(null).assertFail();
                });
            }).catch((error) => {
                expect(error.code == 401).assertTrue();
            });
        } catch (e) {
            console.error('DeviceKvStoreGetStringPromiseInvalidArgsTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreGetStringPromiseClosedKVStoreTest
     * @tc.desc Test Js Api DeviceKvStoreGetString from a closed kvstore
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreGetStringPromiseClosedKVStoreTest', 0, async function (done) {
        try {
            await kvStore.put(KEY_TEST_STRING_ELEMENT, VALUE_TEST_STRING_ELEMENT).then(async (data) => {
                expect(data == undefined).assertTrue();
            }).catch((error) => {
                console.error('DeviceKvStoreGetStringPromiseClosedKVStoreTest put error' + `, error code is ${error.code}, message is ${error.message}`);
                expect(null).assertFail();
            });
            await kvManager.closeKVStore(TEST_BUNDLE_NAME, TEST_STORE_ID).then(async () => {
                await kvManager.deleteKVStore(TEST_BUNDLE_NAME, TEST_STORE_ID).then(() => {
                    expect(true).assertTrue();
                }).catch((err) => {
                    console.error('deleteKVStore err ' + `, error code is ${err.code}, message is ${err.message}`);
                });
            })
            await kvStore.get(localDeviceId, KEY_TEST_STRING_ELEMENT).then((data) => {
                expect(null).assertFail();
            }).catch((err) => {
                expect(err.code == 15100005).assertTrue();
            });
        } catch (e) {
            console.error('DeviceKvStoreGetStringPromiseClosedKVStoreTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreGetStringPromiseNoPutTest
     * @tc.desc Test Js Api DeviceKvStoreGetString without put
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreGetStringPromiseNoPutTest', 0, async function (done) {
        console.info('DeviceKvStoreGetStringPromiseNoPutTest');
        try {
            await kvStore.get(localDeviceId, KEY_TEST_STRING_ELEMENT).then((data) => {
                console.info('DeviceKvStoreGetStringPromiseNoPutTest get success');
                expect(null).assertFail();
            }).catch((err) => {
                console.error('DeviceKvStoreGetStringPromiseNoPutTest get fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(err.code == 15100004).assertTrue();
            });
        } catch (e) {
            console.error('DeviceKvStoreGetStringPromiseNoPutTest get e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStorePutIntPromiseSucTest
     * @tc.desc Test Js Api DeviceKvStore.Put(Int) success
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
     * @tc.desc Test Js Api DeviceKvStore.Put(Int) with max value
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
     * @tc.desc Test Js Api DeviceKvStore.Put(Int) with min value
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
     * @tc.desc Test Js Api DeviceKvStoreGetInt success
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreGetIntPromiseSucTest', 0, async function (done) {
        console.info('DeviceKvStoreGetIntPromiseSucTest');
        try {
            await kvStore.put(KEY_TEST_INT_ELEMENT, VALUE_TEST_INT_ELEMENT).then(async (data) => {
                console.info('DeviceKvStoreGetIntPromiseSucTest put success');
                expect(data == undefined).assertTrue();
                await kvStore.get(localDeviceId, KEY_TEST_INT_ELEMENT).then((data) => {
                    console.info('DeviceKvStoreGetIntPromiseSucTest get success');
                    expect(VALUE_TEST_INT_ELEMENT == data).assertTrue();
                }).catch((err) => {
                    console.error('DeviceKvStoreGetIntPromiseSucTest get fail ' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(null).assertFail();
                });
            }).catch((err) => {
                console.error('DeviceKvStoreGetIntPromiseSucTest put fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('DeviceKvStoreGetIntPromiseSucTest put e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStorePutBoolPromiseSucTest
     * @tc.desc Test Js Api DeviceKvStore.Put(Bool) success
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
     * @tc.desc Test Js Api DeviceKvStoreGetBool success
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreGetBoolPromiseSucTest', 0, async function (done) {
        console.info('DeviceKvStoreGetBoolPromiseSucTest');
        try {
            var boolValue = false;
            await kvStore.put(KEY_TEST_BOOLEAN_ELEMENT, boolValue).then(async (data) => {
                console.info('DeviceKvStoreGetBoolPromiseSucTest put success');
                expect(data == undefined).assertTrue();
                await kvStore.get(localDeviceId, KEY_TEST_BOOLEAN_ELEMENT).then((data) => {
                    console.info('DeviceKvStoreGetBoolPromiseSucTest get success');
                    expect(boolValue == data).assertTrue();
                }).catch((err) => {
                    console.error('DeviceKvStoreGetBoolPromiseSucTest get fail ' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(null).assertFail();
                });
            }).catch((err) => {
                console.error('DeviceKvStoreGetBoolPromiseSucTest put fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('DeviceKvStoreGetBoolPromiseSucTest put e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStorePutFloatPromiseSucTest
     * @tc.desc Test Js Api DeviceKvStore.Put(Float) success
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStorePutFloatPromiseSucTest', 0, async function (done) {
        console.info('DeviceKvStorePutFloatPromiseSucTest');
        try {
            await kvStore.put(KEY_TEST_FLOAT_ELEMENT, VALUE_TEST_FLOAT_ELEMENT).then((data) => {
                console.info('DeviceKvStorePutFloatPromiseSucTest put success');
                expect(data == undefined).assertTrue();
            }).catch((err) => {
                console.error('DeviceKvStorePutFloatPromiseSucTest put fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('DeviceKvStorePutFloatPromiseSucTest put e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreGetFloatPromiseSucTest
     * @tc.desc Test Js Api DeviceKvStoreGetFloat success
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
     * @tc.desc Test Js Api DeviceKvStoreDeleteString success
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
     * @tc.name DeviceKvStoreDeleteStringPromiseInvalidArgsTest
     * @tc.desc Test Js Api DeviceKvStoreDeleteString with invalid args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreDeleteStringPromiseInvalidArgsTest', 0, async function (done) {
        console.info('DeviceKvStoreDeleteStringPromiseInvalidArgsTest');
        try {
            var str = 'this is a test string';
            await kvStore.put(KEY_TEST_STRING_ELEMENT, str).then(async () => {
                expect(true).assertTrue();
                await kvStore.delete().then(() => {
                    expect(null).assertFail();
                }).catch((err) => {
                    expect(null).assertFail();
                });
            }).catch((err) => {
                expect(err.code == 401).assertTrue();
            });
        } catch (e) {
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreDeleteStringPromiseClosedKVStoreTest
     * @tc.desc Test Js Api DeviceKvStoreDeleteString into a closed kvstore
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreDeleteStringPromiseClosedKVStoreTest', 0, async function (done) {
        try {
            let str = "test";

            await kvStore.put(KEY_TEST_STRING_ELEMENT, str).then(async () => {
                console.info('DeviceKvStoreDeleteStringPromiseSucTest put success');
                expect(true).assertTrue();
            }).catch((err) => {
                console.error('DeviceKvStoreDeleteStringPromiseClosedKVStoreTest put fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
            await kvManager.closeKVStore(TEST_BUNDLE_NAME, TEST_STORE_ID).then(async () => {
                await kvManager.deleteKVStore(TEST_BUNDLE_NAME, TEST_STORE_ID).then(() => {
                    expect(true).assertTrue();
                }).catch((err) => {
                    console.error('deleteKVStore err ' + `, error code is ${err.code}, message is ${err.message}`);
                });
            })
            await kvStore.delete(KEY_TEST_STRING_ELEMENT).then((data) => {
                console.info('DeviceKvStoreDeleteStringPromiseSucTest delete success');
                expect(null).assertFail();
            }).catch((err) => {
                console.error('DeviceKvStoreDeleteStringPromiseSucTest delete fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(err.code == 15100005).assertTrue();
            });
        } catch (e) {
            console.error('DeviceKvStorePutStringPromiseClosedKVStoreTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreDeletePredicatesPromiseSucTest
     * @tc.desc Test Js Api DeviceKvStore.Delete() success
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreDeletePredicatesPromiseSucTest', 0, async function (done) {
        console.log('DeviceKvStoreDeletePredicatesPromiseSucTest');
        try {
            let predicates = new dataShare.DataSharePredicates();
            let arr = ["name"];
            predicates.inKeys(arr);
            await kvStore.put("name", "Bob").then(async (data) => {
                console.log('DeviceKvStoreDeletePredicatesPromiseSucTest put success');
                expect(data == undefined).assertTrue();
                await kvStore.delete(predicates).then((data) => {
                    console.log('DeviceKvStoreDeletePredicatesPromiseSucTest delete success');
                    expect(data == undefined).assertTrue();
                }).catch((err) => {
                    console.error('DeviceKvStoreDeletePredicatesPromiseSucTest delete fail ' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(null).assertFail();
                });
            }).catch((err) => {
                console.error('DeviceKvStoreDeletePredicatesPromiseSucTest put fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('DeviceKvStoreDeletePredicatesPromiseSucTest put e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreDeletePredicatesPromiseClosedKVStoreTest
     * @tc.desc Test Js Api DeviceKvStoreDelete predicates into a closed kvstore
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreDeletePredicatesPromiseClosedKVStoreTest', 0, async function (done) {
        try {
            let predicates = new dataShare.DataSharePredicates();
            let arr = ["name"];
            predicates.inKeys(arr);
            await kvStore.put("name", "Bob").then(async () => {
                expect(true).assertTrue();
            }).catch((err) => {
                expect(null).assertFail();
            });
            await kvManager.closeKVStore(TEST_BUNDLE_NAME, TEST_STORE_ID).then(async () => {
                await kvManager.deleteKVStore(TEST_BUNDLE_NAME, TEST_STORE_ID).then(() => {
                    expect(true).assertTrue();
                }).catch((err) => {
                    console.error('deleteKVStore err ' + `, error code is ${err.code}, message is ${err.message}`);
                });
            })
            await kvStore.delete(predicates).then((data) => {
                expect(null).assertFail();
            }).catch((err) => {
                expect(err.code == 15100005).assertTrue();
            });
        } catch (e) {
            console.error('DeviceKvStoreDeletePredicatesPromiseClosedKVStoreTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreDeletePredicatesPromiseInvalidArgsTest
     * @tc.desc Test Js Api DeviceKvStoreDelete predicates with invalid args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreDeleteStringPromiseInvalidArgsTest', 0, async function (done) {
        console.info('DeviceKvStoreDeleteStringPromiseInvalidArgsTest');
        try {
            let predicates = new dataShare.DataSharePredicates();
            let arr = ["name"];
            predicates.inKeys(arr);
            await kvStore.put("name", "bob").then(async () => {
                expect(true).assertTrue();
                await kvStore.delete().then(() => {
                    expect(null).assertFail();
                }).catch((err) => {
                    expect(null).assertFail();
                });
            }).catch((err) => {
                expect(err.code == 401).assertTrue();
            });
        } catch (e) {
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreSetSyncRangePromiseDisjointTest
     * @tc.desc Test Js Api DeviceKvStoreSetSyncRange with disjoint ranges
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
                console.error('DeviceKvStoreDeleteStringPromiseNoPutTest delete fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('DeviceKvStoreSetSyncRangePromiseDisjointTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreSetSyncRangePromiseJointTest
     * @tc.desc Test Js Api DeviceKvStoreSetSyncRange with joint range
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
        } catch (e) {
            console.error('DeviceKvStoreSetSyncRangePromiseJointTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreSetSyncRangePromiseSameTest
     * @tc.desc Test Js Api DeviceKvStoreSetSyncRange with same range
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
        } catch (e) {
            console.error('DeviceKvStoreSetSyncRangePromiseSameTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreSetSyncRangePromiseInvalidArgsTest
     * @tc.desc Test Js Api DeviceKvStoreSetSyncRange with invalid args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreSetSyncRangePromiseInvalidArgsTest', 0, async function (done) {
        console.info('DeviceKvStoreSetSyncRangePromiseSameTest');
        try {
            var remoteSupportLabels = ['A', 'B'];
            await kvStore.setSyncRange(remoteSupportLabels).then((err) => {
                expect(null).assertFail();
            }).catch((err) => {
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('DeviceKvStoreSetSyncRangePromiseSameTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(e.code == 401).assertTrue();
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
                    console.info('DeviceKvStorePutBatchPromiseStringTest getEntries success');
                    console.info('DeviceKvStorePutBatchPromiseStringTest ' + JSON.stringify(entries));
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
        } catch (e) {
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
                    key: key + i,
                    value: {
                        type: factory.ValueType.INTEGER,
                        value: 222
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
        } catch (e) {
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
                    key: key + i,
                    value: {
                        type: factory.ValueType.FLOAT,
                        value: 2.0
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
        } catch (e) {
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
    it('DeviceKvStorePutBatchPromiseDoubleTest', 0, async function (done) {
        console.info('DeviceKvStorePutBatchPromiseDoubleTest');
        try {
            let entries = [];
            for (var i = 0; i < 10; i++) {
                var key = 'batch_test_number_key';
                var entry = {
                    key: key + i,
                    value: {
                        type: factory.ValueType.DOUBLE,
                        value: 2.00
                    }
                }
                entries.push(entry);
            }
            console.info('DeviceKvStorePutBatchPromiseDoubleTest entries: ' + JSON.stringify(entries));
            await kvStore.putBatch(entries).then(async (err) => {
                console.info('DeviceKvStorePutBatchPromiseDoubleTest putBatch success');
                expect(err == undefined).assertTrue();
                await kvStore.getEntries(localDeviceId, 'batch_test_number_key').then((entrys) => {
                    console.info('DeviceKvStorePutBatchPromiseDoubleTest getEntries success');
                    expect(entrys.length == 10).assertTrue();
                    expect(entrys[0].value.value == 2.00).assertTrue();
                }).catch((err) => {
                    console.error('DeviceKvStorePutBatchPromiseDoubleTest getEntries fail ' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(null).assertFail();
                });
            }).catch((err) => {
                console.error('DeviceKvStorePutBatchPromiseDoubleTest putBatch fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('DeviceKvStorePutBatchPromiseDoubleTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStorePutBatchPromiseBooleanTest
     * @tc.desc Test Js Api DeviceKvStore.PutBatch() with boolean value
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
                    key: key + i,
                    value: {
                        type: factory.ValueType.BOOLEAN,
                        value: bo
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
        } catch (e) {
            console.error('DeviceKvStorePutBatchPromiseBooleanTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStorePutBatchPromiseByteArrayTest
     * @tc.desc Test Js Api DeviceKvStore.PutBatch() with byte array value
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStorePutBatchPromiseByteArrayTest', 0, async function (done) {
        console.info('DeviceKvStorePutBatchPromiseByteArrayTest');
        try {
            var arr = new Uint8Array([21, 31]);
            let entries = [];
            for (var i = 0; i < 10; i++) {
                var key = 'batch_test_bool_key';
                var entry = {
                    key: key + i,
                    value: {
                        type: factory.ValueType.BYTE_ARRAY,
                        value: arr
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
        } catch (e) {
            console.error('DeviceKvStorePutBatchPromiseBooleanTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStorePutBatchValuePromiseUint8ArrayTest
     * @tc.desc Test Js Api DeviceKvStore.PutBatch() put Uint8Array
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStorePutBatchValuePromiseUint8ArrayTest', 0, async function (done) {
        console.info('DeviceKvStorePutBatchValuePromiseUint8ArrayTest');
        try {
            let values = [];
            let arr1 = new Uint8Array([4, 5, 6, 7]);
            let arr2 = new Uint8Array([4, 5, 6, 7, 8]);
            let vb1 = {key: "name_1", value: arr1};
            let vb2 = {key: "name_2", value: arr2};
            values.push(vb1);
            values.push(vb2);
            console.info('DeviceKvStorePutBatchValuePromiseUint8ArrayTest values: ' + JSON.stringify(values));
            await kvStore.putBatch(values).then(async (err) => {
                console.info('DeviceKvStorePutBatchValuePromiseUint8ArrayTest putBatch success');
                expect(err == undefined).assertTrue();
                var query = new factory.Query();
                query.prefixKey("name_");
                await kvStore.getEntries(localDeviceId, query).then((entrys) => {
                    expect(entrys.length == 2).assertTrue();
                    done();
                });
            });
        } catch (e) {
            console.error('DeviceKvStorePutBatchValuePromiseUint8ArrayTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStorePutBatchValuePromiseStringTest
     * @tc.desc Test Js Api DeviceKvStore.PutBatch() put String
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStorePutBatchValuePromiseStringTest', 0, async function (done) {
        console.info('DeviceKvStorePutBatchValuePromiseStringTest');
        try {
            let values = [];
            let vb1 = {key: "name_1", value: "arr1"};
            let vb2 = {key: "name_2", value: "arr2"};
            values.push(vb1);
            values.push(vb2);
            console.info('DeviceKvStorePutBatchValuePromiseStringTest values: ' + JSON.stringify(values));
            await kvStore.putBatch(values).then(async (err) => {
                console.info('DeviceKvStorePutBatchValuePromiseUint8ArrayTest putBatch success');
                expect(err == undefined).assertTrue();
                var query = new factory.Query();
                query.prefixKey("name_");
                await kvStore.getEntries(localDeviceId, query).then((entrys) => {
                    expect(entrys.length == 2).assertTrue();
                    done();
                }).catch((err) => {
                    console.log('DeviceKvStorePutBatchValueTest delete fail ' + err);
                    expect(null).assertFail();
                });
            }).catch((err) => {
                console.log('DeviceKvStorePutBatchValueTest delete fail ' + err);
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('DeviceKvStorePutBatchValuePromiseStringTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStorePutBatchValuePromiseNumbersTest
     * @tc.desc Test Js Api DeviceKvStore.PutBatch() put numbers
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStorePutBatchValuePromiseNumbersTest', 0, async function (done) {
        try {
            let values = [];
            let vb1 = {key: "name_1", value: 123};
            let vb2 = {key: "name_2", value: 321.0};
            let vb3 = {key: "name_3", value: 321.00};
            values.push(vb1);
            values.push(vb2);
            values.push(vb3);
            await kvStore.putBatch(values).then(async (err) => {
                expect(err == undefined).assertTrue();
                var query = new factory.Query();
                query.prefixKey("name_");
                await kvStore.getEntries(localDeviceId, query).then((entrys) => {
                    expect(entrys.length == 3).assertTrue();
                    done();
                });
            });
        } catch (e) {
            console.error('DeviceKvStorePutBatchValuePromiseTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStorePutBatchValuePromiseBooleanTest
     * @tc.desc Test Js Api DeviceKvStore.PutBatch() put booleans
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStorePutBatchValuePromiseBooleanTest', 0, async function (done) {
        try {
            let values = [];
            let vb1 = {key: "name_1", value: true};
            let vb2 = {key: "name_2", value: false};
            values.push(vb1);
            values.push(vb2);
            await kvStore.putBatch(values).then(async (err) => {
                expect(err == undefined).assertTrue();
                var query = new factory.Query();
                query.prefixKey("name_");
                await kvStore.getEntries(localDeviceId, query).then((entrys) => {
                    expect(entrys.length == 2).assertTrue();
                    done();
                });
            });
        } catch (e) {
            console.error('DeviceKvStorePutBatchValuePromiseTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStorePutBatchValuePromiseNullTest
     * @tc.desc Test Js Api DeviceKvStore.PutBatch() put null
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStorePutBatchValuePromiseNullTest', 0, async function (done) {
        console.info('DeviceKvStorePutBatchValuePromiseNullTest');
        try {
            let values = [];
            let vb1 = {key: "name_1", value: null};
            let vb2 = {key: "name_2", value: null};
            values.push(vb1);
            values.push(vb2);
            console.info('DeviceKvStorePutBatchValuePromiseNullTest values: ' + JSON.stringify(values));
            await kvStore.putBatch(values).then(async (err) => {
                console.info('DeviceKvStorePutBatchValuePromiseNullTest putBatch success');
                expect(err == undefined).assertTrue();
                var query = new factory.Query();
                query.prefixKey("name_");
                await kvStore.getEntries(localDeviceId, query).then((entrys) => {
                    expect(entrys.length == 2).assertTrue();
                    expect(entrys[0].value == null).assertTrue();
                    done();
                });
            });
        } catch (e) {
            console.error('DeviceKvStorePutBatchValuePromiseNullTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStorePutBatchValuePromiseInvalidArgsTest
     * @tc.desc Test Js Api DeviceKvStore.PutBatch() put invalid args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStorePutBatchValuePromiseInvalidArgsTest', 0, async function (done) {
        try {
            await kvStore.putBatch().then(() => {
                expect(null).assertFail();
            }).catch((err) => {
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('DeviceKvStorePutBatchValuePromiseNullTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(e.code == 401).assertTrue();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStorePutBatchValuePromiseClosedKvstoreTest
     * @tc.desc Test Js Api DeviceKvStore.PutBatch() put into closed kvstore
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStorePutBatchValuePromiseClosedKvstoreTest', 0, async function (done) {
        try {
            await kvManager.closeKVStore(TEST_BUNDLE_NAME, TEST_STORE_ID).then(async () => {
                await kvManager.deleteKVStore(TEST_BUNDLE_NAME, TEST_STORE_ID).then(() => {
                    expect(true).assertTrue();
                }).catch((err) => {
                    console.error('deleteKVStore err ' + `, error code is ${err.code}, message is ${err.message}`);
                });
            })
            let values = [];
            let vb1 = {key: "name_1", value: null};
            let vb2 = {key: "name_2", value: null};
            values.push(vb1);
            values.push(vb2);
            await kvStore.putBatch(values).then(() => {
                expect(null).assertFail();
            }).catch((err) => {
                expect(err.code == 15100005).assertTrue();
            });
        } catch (e) {
            console.error('DeviceKvStorePutBatchValuePromiseNullTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreDeleteBatchPromiseSucTest
     * @tc.desc Test Js Api DeviceKvStoreDeleteBatch success
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
                    key: key + i,
                    value: {
                        type: factory.ValueType.STRING,
                        value: 'batch_test_string_value'
                    }
                }
                entries.push(entry);
                keys.push(key + i);
            }
            await kvStore.putBatch(entries).then(async (err) => {
                expect(err == undefined).assertTrue();
                await kvStore.deleteBatch(keys).then((err) => {
                    expect(err == undefined).assertTrue();
                }).catch((err) => {
                    expect(null).assertFail();
                });
            }).catch((err) => {
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('DeviceKvStoreDeleteBatchPromiseSucTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreDeleteBatchPromiseInvalidArgsTest
     * @tc.desc Test Js Api DeviceKvStoreDeleteBatch with invalid args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreDeleteBatchPromiseInvalidArgsTest', 0, async function (done) {
        console.info('DeviceKvStoreDeleteBatchPromiseInvalidArgsTest');
        try {
            let entries = [];
            let keys = [];
            for (var i = 0; i < 5; i++) {
                var key = 'batch_test_string_key';
                var entry = {
                    key: key + i,
                    value: {
                        type: factory.ValueType.STRING,
                        value: 'batch_test_string_value'
                    }
                }
                entries.push(entry);
                keys.push(key + i);
            }
            await kvStore.putBatch(entries).then(async (err) => {
                expect(err == undefined).assertTrue();
                await kvStore.deleteBatch(1).then((err) => {
                    console.log("fail1")
                    expect(null).assertFail();
                }).catch((err) => {
                    console.log("fail2")
                    expect(null).assertFail();
                });
            }).catch((err) => {
                console.log("fail3")
                expect(err.code == 401).assertTrue();
            });
        } catch (e) {
            console.error('DeviceKvStoreDeleteBatchPromiseInvalidArgsTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreDeleteBatchPromiseClosedKVStoreTest
     * @tc.desc Test Js Api DeviceKvStoreDeleteBatch into closed kvstore
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreDeleteBatchPromiseClosedKVStoreTest', 0, async function (done) {
        console.info('DeviceKvStoreDeleteBatchPromiseClosedKVStoreTest');
        try {
            let entries = [];
            let keys = [];
            for (var i = 0; i < 5; i++) {
                var key = 'batch_test_string_key';
                var entry = {
                    key: key + i,
                    value: {
                        type: factory.ValueType.STRING,
                        value: 'batch_test_string_value'
                    }
                }
                entries.push(entry);
                keys.push(key + i);
            }
            await kvStore.putBatch(entries).then((err) => {
                expect(err == undefined).assertTrue();
            }).catch((err) => {
                expect(null).assertFail();
            });
            await kvManager.closeKVStore(TEST_BUNDLE_NAME, TEST_STORE_ID).then(async () => {
                await kvManager.deleteKVStore(TEST_BUNDLE_NAME, TEST_STORE_ID).then(() => {
                    expect(true).assertTrue();
                }).catch((err) => {
                    console.error('deleteKVStore err ' + `, error code is ${err.code}, message is ${err.message}`);
                });
            })
            kvStore.deleteBatch(keys).then((err) => {
                expect(null).assertFail();
            }).catch((err) => {
                expect(null).assertFail();
                expect(err.code == 15100005).assertTrue();
            });
        } catch (e) {
            console.error('DeviceKvStoreDeleteBatchPromiseClosedKVStoreTest e ' + `, error code is ${e.code}, message is ${e.message}`);
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
            var arr = new Uint8Array([21, 31]);
            let entries = [];
            for (var i = 0; i < 10; i++) {
                var key = 'batch_test_bool_key';
                var entry = {
                    key: key + i,
                    value: {
                        type: factory.ValueType.BYTE_ARRAY,
                        value: arr
                    }
                }
                entries.push(entry);
            }
            console.info('DeviceKvStoreGetEntriesPromiseQueryTest entries: ' + JSON.stringify(entries));
            await kvStore.putBatch(entries).then(async (err) => {
                console.info('DeviceKvStoreGetEntriesPromiseQueryTest putBatch success');
                expect(err == undefined).assertTrue();
                let query = new factory.Query();
                query.prefixKey("batch_test");
                await kvStore.getEntries(localDeviceId, query).then((entrys) => {
                    expect(entrys.length == 10).assertTrue();
                    expect(entrys[0].value.value.toString() == arr.toString()).assertTrue();
                }).catch((err) => {
                    console.log("errr1")
                    expect(null).assertFail();
                });
            }).catch((err) => {
                console.log("errr2")
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('DeviceKvStoreGetEntriesPromiseQueryTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            console.log("errr3")
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreGetEntriesPromiseQueryClosedKVStoreTest
     * @tc.desc Test Js Api DeviceKvStore.GetEntries() query from a closed kvstore
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreGetEntriesPromiseQueryClosedKVStoreTest', 0, async function (done) {
        try {
            var arr = new Uint8Array([21, 31]);
            let entries = [];
            for (var i = 0; i < 10; i++) {
                var key = 'batch_test_bool_key';
                var entry = {
                    key: key + i,
                    value: {
                        type: factory.ValueType.BYTE_ARRAY,
                        value: arr
                    }
                }
                entries.push(entry);
            }
            await kvStore.putBatch(entries).then(async (err) => {
                expect(err == undefined).assertTrue();
            }).catch((err) => {
                expect(null).assertFail();
            });
            await kvManager.closeKVStore(TEST_BUNDLE_NAME, TEST_STORE_ID).then(async () => {
                await kvManager.deleteKVStore(TEST_BUNDLE_NAME, TEST_STORE_ID).then(() => {
                    expect(true).assertTrue();
                }).catch((err) => {
                    console.error('deleteKVStore err ' + `, error code is ${err.code}, message is ${err.message}`);
                });
            })
            let query = new factory.Query();
            query.prefixKey("batch_test");
            await kvStore.getEntries(localDeviceId, query).then(() => {
                expect(null).assertFail();
            }).catch((err) => {
                expect(err.code == 15100005).assertTrue();
            });
        } catch (e) {
            console.error('DeviceKvStoreGetEntriesPromiseQueryClosedKVStoreTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreGetEntriesPromiseSucTest
     * @tc.desc Test Js Api DeviceKvStore.GetEntries() success
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreGetEntriesPromiseSucTest', 0, async function (done) {
        console.info('DeviceKvStoreGetEntriesPromiseSucTest');
        try {
            var arr = new Uint8Array([21, 31]);
            let entries = [];
            for (var i = 0; i < 10; i++) {
                var key = 'batch_test_bool_key';
                var entry = {
                    key: key + i,
                    value: {
                        type: factory.ValueType.BYTE_ARRAY,
                        value: arr
                    }
                }
                entries.push(entry);
            }
            await kvStore.putBatch(entries).then(async (err) => {
                expect(err == undefined).assertTrue();
                await kvStore.getEntries(localDeviceId, "batch_test").then((entrys) => {
                    expect(entrys.length == 10).assertTrue();
                    expect(entrys[0].value.value.toString() == arr.toString()).assertTrue();
                }).catch((err) => {
                    console.log("faillll1" + `${err.code}`)
                    expect(err == undefined).assertTrue();
                });
            }).catch((error) => {
                console.log("faillll2"`${error.code}`)
                expect(error == undefined).assertTrue();
            });
        } catch (e) {
            console.error('DeviceKvStoreGetEntriesPromiseSucTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreGetEntriesPromiseClosedKVStoreTest
     * @tc.desc Test Js Api DeviceKvStore.GetEntries() from a closed kvstore
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreGetEntriesPromiseClosedKVStoreTest', 0, async function (done) {
        try {
            var arr = new Uint8Array([21, 31]);
            let entries = [];
            for (var i = 0; i < 10; i++) {
                var key = 'batch_test_bool_key';
                var entry = {
                    key: key + i,
                    value: {
                        type: factory.ValueType.BYTE_ARRAY,
                        value: arr
                    }
                }
                entries.push(entry);
            }
            await kvStore.putBatch(entries).then(async (err) => {
                expect(err == undefined).assertTrue();
            }).catch((err) => {
                expect(null).assertFail();
            });
            await kvManager.closeKVStore(TEST_BUNDLE_NAME, TEST_STORE_ID).then(async () => {
                await kvManager.deleteKVStore(TEST_BUNDLE_NAME, TEST_STORE_ID).then(() => {
                    expect(true).assertTrue();
                }).catch((err) => {
                    console.error('deleteKVStore err ' + `, error code is ${err.code}, message is ${err.message}`);
                });
            })
            await kvStore.getEntries("batch_test").then(() => {
                expect(null).assertFail();
            }).catch((err) => {
                expect(err.code == 15100005).assertTrue();
            });
        } catch (e) {
            console.error('DeviceKvStoreGetEntriesPromiseSucTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreGetEntriesPromiseInvalidArgsTest
     * @tc.desc Test Js Api DeviceKvStore.GetEntries() with invalid args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreGetEntriesPromiseInvalidArgsTest', 0, async function (done) {
        console.info('DeviceKvStoreGetEntriesPromiseInvalidArgsTest');
        try {
            var arr = new Uint8Array([21, 31]);
            let entries = [];
            for (var i = 0; i < 10; i++) {
                var key = 'batch_test_bool_key';
                var entry = {
                    key: key + i,
                    value: {
                        type: factory.ValueType.BYTE_ARRAY,
                        value: arr
                    }
                }
                entries.push(entry);
            }
            await kvStore.putBatch(entries).then(async (err) => {
                expect(err == undefined).assertTrue();
                await kvStore.getEntries().then(() => {
                    expect(null).assertFail();
                }).catch((err) => {
                    expect(null).assertFail();
                });
            }).catch((err) => {
                expect(err.code == 401).assertTrue();
            });
        } catch (e) {
            console.error('DeviceKvStoreGetEntriesPromiseInvalidArgsTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStorestartTransactionPromiseCommitTest
     * @tc.desc Test Js Api DeviceKvStorestartTransaction with commit
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStorestartTransactionPromiseCommitTest', 0, async function (done) {
        console.info('DeviceKvStorestartTransactionPromiseCommitTest');
        try {
            var count = 0;
            kvStore.on('dataChange', factory.SubscribeType.SUBSCRIBE_TYPE_ALL, function (data) {
                count++;
            });
            await kvStore.startTransaction().then(async (err) => {
                expect(err == undefined).assertTrue();
            }).catch((err) => {
                expect(null).assertFail();
            });
            let entries = putBatchString(10, 'batch_test_string_key');
            await kvStore.putBatch(entries).then(async (err) => {
                expect(err == undefined).assertTrue();
            }).catch((err) => {
                expect(null).assertFail();
            });
            let keys = Object.keys(entries).slice(5);
            await kvStore.deleteBatch(keys).then((err) => {
                expect(err == undefined).assertTrue();
            }).catch((err) => {
                expect(null).assertFail();
            });
            await kvStore.commit().then(async (err) => {
                expect(err == undefined).assertTrue();
            }).catch((err) => {
                expect(null).assertFail();
            });
            await sleep(2000);
            expect(count == 1).assertTrue();
        } catch (e) {
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStorestartTransactionPromiseRollbackTest
     * @tc.desc Test Js Api DeviceKvStorestartTransaction with Rollback
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStorestartTransactionPromiseRollbackTest', 0, async function (done) {
        console.info('DeviceKvStorestartTransactionPromiseRollbackTest');
        try {
            var count = 0;
            kvStore.on('dataChange', 0, function (data) {
                console.info('DeviceKvStorestartTransactionPromiseRollbackTest ' + JSON.stringify(data));
                count++;
            });
            await kvStore.startTransaction().then(async (err) => {
                expect(err == undefined).assertTrue();
            }).catch((err) => {
                expect(null).assertFail();
            });
            let entries = putBatchString(10, 'batch_test_string_key');
            await kvStore.putBatch(entries).then(async (err) => {
                expect(err == undefined).assertTrue();
            }).catch((err) => {
                expect(null).assertFail();
            });
            let keys = Object.keys(entries).slice(5);
            await kvStore.deleteBatch(keys).then((err) => {
                expect(err == undefined).assertTrue();
            }).catch((err) => {
                expect(null).assertFail();
            });
            await kvStore.rollback().then(async (err) => {
                expect(err == undefined).assertTrue();
            }).catch((err) => {
                expect(null).assertFail();
            });
            await sleep(2000);
            expect(count == 0).assertTrue();
        } catch (e) {
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStorestartTransactionPromiseClosedKVStoreTest
     * @tc.desc Test Js Api DeviceKvStorestartTransaction with a closed kvstore
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStorestartTransactionPromiseClosedKVStoreTest', 0, async function (done) {
        try {
            var count = 0;
            kvStore.on('dataChange', 0, function (data) {
                count++;
            });
            await kvManager.closeKVStore(TEST_BUNDLE_NAME, TEST_STORE_ID).then(async () => {
                await kvManager.deleteKVStore(TEST_BUNDLE_NAME, TEST_STORE_ID).then(() => {
                    expect(true).assertTrue();
                }).catch((err) => {
                    console.error('deleteKVStore err ' + `, error code is ${err.code}, message is ${err.message}`);
                });
            })
            await kvStore.startTransaction().then(async (err) => {
                expect(null).assertFail();
            }).catch((err) => {
                expect(err.code == 15100005).assertTrue();
            });

        } catch (e) {
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStorestartTransactionPromiseInvalidArgsTest
     * @tc.desc Test Js Api DeviceKvStorestartTransaction with invalid args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStorestartTransactionPromiseInvalidArgsTest', 0, async function (done) {
        console.info('DeviceKvStorestartTransactionPromiseRollbackTest');
        try {
            await kvStore.startTransaction(1).then(async (err) => {
                expect(null).assertFail();
            }).catch((err) => {
                expect(true).assertTrue();
            });
        } catch (e) {
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreCommitPromiseClosedKVStoreTest
     * @tc.desc Test Js Api DeviceKvStoreCommit with a closed kvstore
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreCommitPromiseClosedKVStoreTest', 0, async function (done) {
        console.info('DeviceKvStoreCommitPromiseClosedKVStoreTest');
        try {
            await kvManager.closeKVStore(TEST_BUNDLE_NAME, TEST_STORE_ID).then(async () => {
                await kvManager.deleteKVStore(TEST_BUNDLE_NAME, TEST_STORE_ID).then(() => {
                    expect(true).assertTrue();
                }).catch((err) => {
                    console.error('deleteKVStore err ' + `, error code is ${err.code}, message is ${err.message}`);
                });
            })
            await kvStore.commit().then(async (err) => {
                expect(null).assertFail();
            }).catch((err) => {
                expect(err.code == 15100005).assertTrue();
            });
        } catch (e) {
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreRollbackPromiseClosedKVStoreTest
     * @tc.desc Test Js Api DeviceKvStoreRollback with a closed kvstore
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreRollbackPromiseClosedKVStoreTest', 0, async function (done) {
        try {
            await kvManager.closeKVStore(TEST_BUNDLE_NAME, TEST_STORE_ID).then(async () => {
                await kvManager.deleteKVStore(TEST_BUNDLE_NAME, TEST_STORE_ID).then(() => {
                    expect(true).assertTrue();
                }).catch((err) => {
                    console.error('deleteKVStore err ' + `, error code is ${err.code}, message is ${err.message}`);
                });
            })
            await kvStore.rollback().then(async (err) => {
                expect(null).assertFail();
            }).catch((err) => {
                expect(err.code == 15100005).assertTrue();
            });
        } catch (e) {
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreEnableSyncPromiseTrueTest
     * @tc.desc Test Js Api DeviceKvStoreEnableSync true
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreEnableSyncPromiseTrueTest', 0, async function (done) {
        try {
            await kvStore.enableSync(true).then((err) => {
                expect(err == undefined).assertTrue();
            }).catch((err) => {
                expect(null).assertFail();
            });
        } catch (e) {
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreEnableSyncPromiseFalseTest
     * @tc.desc Test Js Api DeviceKvStoreEnableSync false
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreEnableSyncPromiseFalseTest', 0, async function (done) {
        console.info('DeviceKvStoreEnableSyncPromiseFalseTest');
        try {
            await kvStore.enableSync(false).then((err) => {
                expect(err == undefined).assertTrue();
            }).catch((err) => {
                expect(null).assertFail();
            });
        } catch (e) {
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreEnableSyncPromiseInvalidArgsTest
     * @tc.desc Test Js Api DeviceKvStoreEnableSync with invalid args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreEnableSyncPromiseInvalidArgsTest', 0, async function (done) {
        try {
            await kvStore.enableSync().then((err) => {
                expect(null).assertFail();
            }).catch((err) => {
                expect(null).assertFail();
            });
        } catch (e) {
            expect(e.code == 401).assertTrue();
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
        try {
            var localLabels = ['A', 'B'];
            var remoteSupportLabels = ['A', 'B'];
            await kvStore.setSyncRange(localLabels, remoteSupportLabels).then((err) => {
                expect(err == undefined).assertTrue();
            }).catch((err) => {
                expect(null).assertFail();
            });
        } catch (e) {
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreSetSyncRangePromiseSameTest
     * @tc.desc Test Js Api DeviceKvStore.SetSyncRange() with invalid args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreSetSyncRangePromiseSameTest', 0, async function (done) {
        try {
            await kvStore.setSyncRange().then((err) => {
                expect(null).assertFail();
            }).catch((err) => {
                expect(null).assertFail();
            });
        } catch (e) {
            expect(e.code == 401).assertTrue();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreRemoveDeviceDataPromiseInvalidArgsTest
     * @tc.desc Test Js Api DeviceKvStoreRemoveDeviceData with invalid args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreRemoveDeviceDataPromiseInvalidArgsTest', 0, async function (done) {
        console.info('DeviceKvStoreRemoveDeviceDataPromiseInvalidArgsTest');
        try {
            await kvStore.removeDeviceData().then((err) => {
                console.info('DeviceKvStoreRemoveDeviceDataPromiseInvalidArgsTest removeDeviceData success');
                expect(null).assertFail();
            }).catch((err) => {
                console.error('DeviceKvStoreRemoveDeviceDataPromiseInvalidArgsTest removeDeviceData fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('DeviceKvStoreRemoveDeviceDataPromiseInvalidArgsTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(e.code == 401).assertTrue();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreRemoveDeviceDataPromiseClosedKVStoreTest
     * @tc.desc Test Js Api DeviceKvStoreRemoveDeviceData in a closed kvstore
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreRemoveDeviceDataPromiseClosedKVStoreTest', 0, async function (done) {
        try {
            await kvStore.put(KEY_TEST_STRING_ELEMENT, VALUE_TEST_STRING_ELEMENT).then((err) => {
                expect(err == undefined).assertTrue();
            }).catch((err) => {
                expect(null).assertFail();
            });
            await kvManager.closeKVStore(TEST_BUNDLE_NAME, TEST_STORE_ID).then(async () => {
                await kvManager.deleteKVStore(TEST_BUNDLE_NAME, TEST_STORE_ID).then(() => {
                    expect(true).assertTrue();
                }).catch((err) => {
                    console.error('deleteKVStore err ' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(null).assertFail();
                });
            })
            var deviceid = 'no_exist_device_id';
            await kvStore.removeDeviceData(deviceid).then(() => {
                expect(null).assertFail();
            }).catch((err) => {
                expect(err.code == 15100005).assertTrue();
            });

        } catch (e) {
            console.error('DeviceKvStoreRemoveDeviceDataPromiseClosedKVStoreTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreGetResultSetPromiseSucTest
     * @tc.desc Test Js Api DeviceKvStoreGetResultSet success
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreGetResultSetPromiseSucTest', 0, async function (done) {
        console.info('DeviceKvStoreGetResultSetPromiseSucTest');
        try {
            let resultSet;
            let entries = [];
            for (var i = 0; i < 10; i++) {
                var key = 'batch_test_string_key';
                var entry = {
                    key: key + i,
                    value: {
                        type: factory.ValueType.STRING,
                        value: 'batch_test_string_value'
                    }
                }
                entries.push(entry);
            }
            await kvStore.putBatch(entries).then(async (err) => {
                expect(err == undefined).assertTrue();
            }).catch((err) => {
                expect(null).assertFail();
            });
            await kvStore.getResultSet(localDeviceId, 'batch_test_string_key').then((result) => {
                resultSet = result;
                expect(resultSet.getCount() == 10).assertTrue();
            }).catch((err) => {
                expect(null).assertFail();
            });
            await kvStore.closeResultSet(resultSet).then((err) => {
                expect(err == undefined).assertTrue();
            }).catch((err) => {
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('DeviceKvStoreGetResultSetPromiseSucTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreGetResultSetPromiseInvalidArgsTest
     * @tc.desc Test Js Api DeviceKvStoreGetResultSet with invalid args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreGetResultSetPromiseInvalidArgsTest', 0, async function (done) {
        console.info('DeviceKvStoreGetResultSetPromiseInvalidArgsTest');
        try {
            let resultSet;
            await kvStore.getResultSet().then((result) => {
                console.info('DeviceKvStoreGetResultSetPromiseInvalidArgsTest getResultSet success');
                expect(null).assertFail();
            }).catch((err) => {
                console.error('DeviceKvStoreGetResultSetPromiseInvalidArgsTest getResultSet fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('DeviceKvStoreGetResultSetPromiseInvalidArgsTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(e.code == 401).assertTrue();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreGetResultSetPromiseClosedKVStoreTest
     * @tc.desc Test Js Api DeviceKvStoreGetResultSet from a closed kvstore
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreGetResultSetPromiseClosedKVStoreTest', 0, async function (done) {
        try {
            let resultSet;
            let entries = [];
            await kvStore.putBatch(entries).then(async (err) => {
                expect(err == undefined).assertTrue();
            }).catch((err) => {
                expect(null).assertFail();
            });
            await kvManager.closeKVStore(TEST_BUNDLE_NAME, TEST_STORE_ID).then(async () => {
                await kvManager.deleteKVStore(TEST_BUNDLE_NAME, TEST_STORE_ID).then(() => {
                    expect(true).assertTrue();
                }).catch((err) => {
                    console.error('deleteKVStore err ' + `, error code is ${err.code}, message is ${err.message}`);
                });
            });
            await kvStore.getResultSet(localDeviceId, 'batch_test_string_key').then((result) => {
                expect(null).assertFail();
            }).catch((err) => {
                console.error('DeviceKvStoreGetResultSetPromiseClosedKVStoreTest getResultSet fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(err.code == 15100005).assertTrue();
            });
        } catch (e) {
            console.error('DeviceKvStoreGetResultSetPromiseClosedKVStoreTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreGetResultSetPromiseQueryTest
     * @tc.desc Test Js Api DeviceKvStoreGetResultSet with query
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreGetResultSetPromiseQueryTest', 0, async function (done) {
        console.info('DeviceKvStoreGetResultSetPromiseQueryTest');
        try {
            let resultSet;
            let entries = [];
            for (var i = 0; i < 10; i++) {
                var key = 'batch_test_string_key';
                var entry = {
                    key: key + i,
                    value: {
                        type: factory.ValueType.STRING,
                        value: 'batch_test_string_value'
                    }
                }
                entries.push(entry);
            }
            await kvStore.putBatch(entries).then(async (err) => {
                console.info('DeviceKvStoreGetResultSetPromiseQueryTest putBatch success');
                expect(err == undefined).assertTrue();
            }).catch((err) => {
                console.error('DeviceKvStorePutBatchPromiseStringTest putBatch fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
            var query = new factory.Query();
            query.prefixKey("batch_test");
            await kvStore.getResultSet(localDeviceId, query).then((result) => {
                console.info('DeviceKvStoreGetResultSetPromiseQueryTest getResultSet success');
                resultSet = result;
                expect(resultSet.getCount() == 10).assertTrue();
            }).catch((err) => {
                console.error('DeviceKvStoreGetResultSetPromiseQueryTest getResultSet fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
            await kvStore.closeResultSet(resultSet).then((err) => {
                console.info('DeviceKvStoreGetResultSetPromiseQueryTest closeResultSet success');
                expect(err == undefined).assertTrue();
            }).catch((err) => {
                console.error('DeviceKvStoreGetResultSetPromiseQueryTest closeResultSet fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('DeviceKvStoreGetResultSetPromiseQueryTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreGetResultSetQueryPromiseClosedKVStoreTest
     * @tc.desc Test Js Api DeviceKvStoreGetResultSet query from a closed kvstore
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreGetResultSetQueryPromiseClosedKVStoreTest', 0, async function (done) {
        try {
            let resultSet;
            let entries = [];
            for (var i = 0; i < 10; i++) {
                var key = 'batch_test_string_key';
                var entry = {
                    key: key + i,
                    value: {
                        type: factory.ValueType.STRING,
                        value: 'batch_test_string_value'
                    }
                }
                entries.push(entry);
            }
            await kvStore.putBatch(entries).then(async (err) => {
                expect(err == undefined).assertTrue();
            }).catch((err) => {
                expect(null).assertFail();
            });
            await kvManager.closeKVStore(TEST_BUNDLE_NAME, TEST_STORE_ID).then(async () => {
                await kvManager.deleteKVStore(TEST_BUNDLE_NAME, TEST_STORE_ID).then(() => {
                    expect(true).assertTrue();
                }).catch((err) => {
                    console.error('deleteKVStore err ' + `, error code is ${err.code}, message is ${err.message}`);
                });
            });
            var query = new factory.Query();
            query.prefixKey("batch_test");
            await kvStore.getResultSet(localDeviceId, query).then((result) => {
                expect(null).assertFail();
            }).catch((err) => {
                expect(err.code == 15100005).assertTrue();
            });
        } catch (e) {
            console.error('DeviceKvStoreGetResultSetQueryPromiseClosedKVStoreTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreGetResultSetPredicatesPromiseSucTest
     * @tc.desc Test Js Api DeviceKvStore.GetResultSet() with predicates success
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreGetResultSetPredicatesPromiseSucTest', 0, async function (done) {
        console.log('DeviceKvStoreGetResultSetPredicatesPromiseSucTest');
        try {
            let entries = [];
            let resultSet;
            for (let i = 0; i < 10; i++) {
                let key = 'name_';
                let value = 'Bob_'
                let entry = {
                    key: key + i,
                    value: {
                        type: factory.ValueType.STRING,
                        value: value + i
                    }
                }
                entries.push(entry);
            }
            await kvStore.putBatch(entries).then(async (err) => {
                expect(err == undefined).assertTrue();
            }).catch((err) => {
                expect(null).assertFail();
            });
            let predicates = new dataShare.DataSharePredicates();
            predicates.prefixKey("name_");
            await kvStore.getResultSet(localDeviceId, predicates).then((result) => {
                resultSet = result;
                expect(resultSet.getCount() == 10).assertTrue();
            }).catch((err) => {
                expect(null).assertFail();
            });
            await kvStore.closeResultSet(resultSet).then((err) => {
                expect(err == undefined).assertTrue();
            }).catch((err) => {
                expect(null).assertFail();
            });
        } catch (e) {
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreGetResultSetPredicatesPromiseClosedKVStoreTest
     * @tc.desc Test Js Api DeviceKvStoreGetResultSet predicates from a closed kvstore
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreGetResultSetPredicatesPromiseClosedKVStoreTest', 0, async function (done) {
        try {
            let resultSet;
            let entries = [];
            for (var i = 0; i < 10; i++) {
                var key = 'batch_test_string_key';
                var entry = {
                    key: key + i,
                    value: {
                        type: factory.ValueType.STRING,
                        value: 'batch_test_string_value'
                    }
                }
                entries.push(entry);
            }
            await kvStore.putBatch(entries).then(async (err) => {
                expect(err == undefined).assertTrue();
            }).catch((err) => {
                expect(null).assertFail();
            });
            await kvManager.closeKVStore(TEST_BUNDLE_NAME, TEST_STORE_ID).then(async () => {
                await kvManager.deleteKVStore(TEST_BUNDLE_NAME, TEST_STORE_ID).then(() => {
                    expect(true).assertTrue();
                }).catch((err) => {
                    console.error('deleteKVStore err ' + `, error code is ${err.code}, message is ${err.message}`);
                });
            });
            let predicates = new dataShare.DataSharePredicates();
            predicates.prefixKey("batch_test");
            await kvStore.getResultSet(localDeviceId, predicates).then((result) => {
                expect(null).assertFail();
            }).catch((err) => {
                expect(err.code == 15100005).assertTrue();
            });
        } catch (e) {
            console.error('DeviceKvStoreGetResultSetPredicatesPromiseClosedKVStoreTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreCloseResultSetPromiseSucTest
     * @tc.desc Test Js Api DeviceKvStoreCloseResultSet success
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreCloseResultSetPromiseSucTest', 0, async function (done) {
        console.info('DeviceKvStoreCloseResultSetPromiseSucTest');
        try {
            console.info('DeviceKvStoreCloseResultSetPromiseSucTest success');
            let resultSet = null;
            await kvStore.getResultSet('batch_test_string_key').then((result) => {
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
        } catch (e) {
            console.error('DeviceKvStoreCloseResultSetPromiseSucTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreCloseResultSetPromiseInvalidArgsTest
     * @tc.desc Test Js Api DeviceKvStoreCloseResultSet with invalid args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreCloseResultSetPromiseInvalidArgsTest', 0, async function (done) {
        try {
            await kvStore.closeResultSet().then(() => {
                expect(null).assertFail();
            }).catch((err) => {
                console.error('DeviceKvStoreCloseResultSetPromiseInvalidArgsTest closeResultSet fail ' + `, error code is ${err.code}, message is ${err.message}`);
            });
        } catch (e) {
            console.error('DeviceKvStoreCloseResultSetPromiseInvalidArgsTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(e.code == 401).assertTrue();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreGetResultSizePromiseQueryTest
     * @tc.desc Test Js Api DeviceKvStoreGetResultSize with query
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreGetResultSizePromiseQueryTest', 0, async function (done) {
        try {
            let entries = [];
            for (var i = 0; i < 10; i++) {
                var key = 'batch_test_string_key';
                var entry = {
                    key: key + i,
                    value: {
                        type: factory.ValueType.STRING,
                        value: 'batch_test_string_value'
                    }
                }
                entries.push(entry);
            }
            await kvStore.putBatch(entries).then(async (err) => {
                console.info('DeviceKvStoreGetResultSizePromiseQueryTest putBatch success');
                expect(err == undefined).assertTrue();
            }).catch((err) => {
                console.error('DeviceKvStorePutBatchPromiseStringTest putBatch fail ' + `, error code is ${err.code}, message is ${err.message}`);
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
        } catch (e) {
            console.error('DeviceKvStoreGetResultSizePromiseQueryTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreGetResultSizePromiseInvalidArgsTest
     * @tc.desc Test Js Api DeviceKvStoreGetResultSize with invalid args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreGetResultSizePromiseInvalidArgsTest', 0, async function (done) {
        try {
            let entries = [];
            for (var i = 0; i < 10; i++) {
                var key = 'batch_test_string_key';
                var entry = {
                    key: key + i,
                    value: {
                        type: factory.ValueType.STRING,
                        value: 'batch_test_string_value'
                    }
                }
                entries.push(entry);
            }
            await kvStore.putBatch(entries).then(async (err) => {
                expect(err == undefined).assertTrue();
            }).catch((err) => {
                expect(null).assertFail();
            });
            await kvStore.getResultSize().then(() => {
                expect(null).assertFail();
            }).catch((err) => {
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('DeviceKvStoreGetResultSizePromiseInvalidArgsTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(e.code == 401).assertTrue();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreGetResultSizePromiseClosedKVStoreTest
     * @tc.desc Test Js Api DeviceKvStoreGetResultSize from a closed kvstore
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreGetResultSizePromiseClosedKVStoreTest', 0, async function (done) {
        try {
            let entries = [];
            for (var i = 0; i < 10; i++) {
                var key = 'batch_test_string_key';
                var entry = {
                    key: key + i,
                    value: {
                        type: factory.ValueType.STRING,
                        value: 'batch_test_string_value'
                    }
                }
                entries.push(entry);
            }
            await kvStore.putBatch(entries).then(async (err) => {
                expect(err == undefined).assertTrue();
            }).catch((err) => {
                expect(null).assertFail();
            });
            await kvManager.closeKVStore(TEST_BUNDLE_NAME, TEST_STORE_ID).then(async () => {
                await kvManager.deleteKVStore(TEST_BUNDLE_NAME, TEST_STORE_ID).then(() => {
                    expect(true).assertTrue();
                }).catch((err) => {
                    console.error('deleteKVStore err ' + `, error code is ${err.code}, message is ${err.message}`);
                });
            });
            var query = new factory.Query();
            query.prefixKey("batch_test");
            await kvStore.getResultSize(localDeviceId, query).then(() => {
                console.info('DeviceKvStoreGetResultSizePromiseClosedKVStoreTest getResultSet success');
                expect(null).assertFail();
            }).catch((err) => {
                console.error('DeviceKvStoreGetResultSizePromiseClosedKVStoreTest getResultSet fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(err.code == 15100005).assertTrue();
            });
        } catch (e) {
            console.error('DeviceKvStoreGetResultSizePromiseQueryTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreChangeNotificationPromiseTest
     * @tc.desc Test Js Api DeviceKvStoreChangeNotification
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreChangeNotificationPromiseTest', 0, async function (done) {
        var getInsertEntries = new Promise((resolve, reject) => {
            kvStore.on('dataChange', 0, function (ChangeNotification) {
                resolve(ChangeNotification.insertEntries);
            });
            kvStore.put("getInsertEntries", "byPut").then(() => {
                console.info('DeviceKvStoreChangeNotificationPromiseTest put success');
            });
        });
        await getInsertEntries.then(function (insertEntries) {
            console.info('DeviceKvStoreChangeNotificationPromiseTest getInsertEntries' + JSON.stringify(insertEntries));
            expect(insertEntries != null).assertTrue();
        }).catch((error) => {
            console.error('DeviceKvStoreChangeNotificationPromiseTest can NOT getInsertEntries, fail:' + `, error code is ${error.code}, message is ${error.message}`);
            expect(null).assertFail();
        });
        done();
    })

    /**
     * @tc.name DeviceKvStoreChangeNotificationPromisePutTest
     * @tc.desc Test Js Api DeviceKvStoreChangeNotification put
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreChangeNotificationPromisePutTest', 0, async function (done) {
        await kvStore.put('getUpdateEntries', 'byPut').then(() => {
            console.info('DeviceKvStoreChangeNotificationPromisePutTest put success');
        });
        var getUpdateEntries = new Promise((resolve, reject) => {
            kvStore.on('dataChange', 0, function (ChangeNotification) {
                resolve(ChangeNotification.updateEntries);
            });
            kvStore.put("getUpdateEntries", "byPut").then(() => {
                console.info('DeviceKvStoreChangeNotificationPromisePutTest update success');
            });
        });
        await getUpdateEntries.then(function (updateEntries) {
            console.info('DeviceKvStoreChangeNotificationPromisePutTest getUpdateEntries' + JSON.stringify(updateEntries));
            expect(updateEntries != null).assertTrue();
        }).catch((error) => {
            console.error('DeviceKvStoreChangeNotificationPromisePutTest can NOT getUpdateEntries, fail:' + `, error code is ${error.code}, message is ${error.message}`);
            expect(null).assertFail();
        });
        done();
    })

    /**
     * @tc.name DeviceKvStoreChangeNotificationPromiseDeleteTest
     * @tc.desc Test Js Api DeviceKvStoreChangeNotification delete
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreChangeNotificationPromiseDeleteTest', 0, async function (done) {
        await kvStore.put('deleteEntries', 'byPut').then(() => {
            console.info('DeviceKvStoreChangeNotificationPromiseDeleteTest put success');
        });
        var getdeleteEntries = new Promise((resolve, reject) => {
            kvStore.on('dataChange', 0, function (ChangeNotification) {
                resolve(ChangeNotification.deleteEntries);
            });
            kvStore.delete("deleteEntries").then(() => {
                console.info('DeviceKvStoreChangeNotificationPromiseDeleteTest delete success');
            });
        });
        await getdeleteEntries.then(function (deleteEntries) {
            console.info('DeviceKvStoreChangeNotificationPromiseDeleteTest deleteEntries' + JSON.stringify(getdeleteEntries));
            expect(deleteEntries != null).assertTrue();
        }).catch((error) => {
            console.error('DeviceKvStoreChangeNotificationPromiseDeleteTest can NOT getdeleteEntries, fail:' + `, error code is ${error.code}, message is ${error.message}`);
            expect(null).assertFail();
        });
        done();
    })
})
