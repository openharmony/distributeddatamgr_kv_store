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
            console.error('beforeAll can NOT getDeviceId, fail:' + `, error code is ${error.code}, message is ${error.message}`);
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
            console.error('afterEach closeKVStore err ' + `, error code is ${e.code}, message is ${e.message}`);
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
                console.error('DeviceKvStorePutStringPromiseInvalidArgsTest put fail' + `, error code is ${error.code}, message is ${error.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('DeviceKvStorePutStringPromiseInvalidArgsTest fail' + `, error code is ${e.code}, message is ${e.message}`);
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
                console.error('DeviceKvStorePutStringPromiseClosedKVStoreTest put fail' + `, error code is ${error.code}, message is ${error.message}`);
                expect(error.code == 15100005).assertTrue();
            });
        } catch (e) {
            console.error('DeviceKvStorePutStringPromiseClosedKVStoreTest fail' + `, error code is ${e.code}, message is ${e.message}`);
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
                console.error('DeviceKvStorePutStringPromiseSucTest put fail' + `, error code is ${error.code}, message is ${error.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('DeviceKvStorePutStringPromiseSucTest fail' + `, error code is ${e.code}, message is ${e.message}`);
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
            console.error('DeviceKvStoreGetStringPromiseInvalidArgsTest fail' + `, error code is ${e.code}, message is ${e.message}`);
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
                console.error('DeviceKvStoreGetStringPromiseClosedKVStoreTest put fail' + `, error code is ${error.code}, message is ${error.message}`);
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
            console.error('DeviceKvStoreGetStringPromiseClosedKVStoreTest fail' + `, error code is ${e.code}, message is ${e.message}`);
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
                console.error('DeviceKvStoreGetStringPromiseNoPutTest get fail' + `, error code is ${err.code}, message is ${err.message}`);
                expect(err.code == 15100004).assertTrue();
            });
        } catch (e) {
            console.error('DeviceKvStoreGetStringPromiseNoPutTest fail' + `, error code is ${e.code}, message is ${e.message}`);
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
                console.error('DeviceKvStorePutIntPromiseSucTest put fail' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('DeviceKvStorePutIntPromiseSucTest put fail' + `, error code is ${e.code}, message is ${e.message}`);
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
                    console.error('DeviceKvStorePutIntPromiseMaxTest get fail' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(null).assertFail();
                });
            }).catch((err) => {
                console.error('DeviceKvStorePutIntPromiseMaxTest put fail' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('DeviceKvStorePutIntPromiseMaxTest fail' + `, error code is ${e.code}, message is ${e.message}`);
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
                    console.error('DeviceKvStorePutIntPromiseMinTest get fail' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(null).assertFail();
                });
            }).catch((err) => {
                console.error('DeviceKvStorePutIntPromiseMinTest put fail' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('DeviceKvStorePutIntPromiseMinTest fail' + `, error code is ${e.code}, message is ${e.message}`);
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
                    console.error('DeviceKvStoreGetIntPromiseSucTest get fail' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(null).assertFail();
                });
            }).catch((err) => {
                console.error('DeviceKvStoreGetIntPromiseSucTest put fail' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('DeviceKvStoreGetIntPromiseSucTest fail' + `, error code is ${e.code}, message is ${e.message}`);
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
                console.error('DeviceKvStorePutBoolPromiseSucTest put fail' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('DeviceKvStorePutBoolPromiseSucTest fail' + `, error code is ${e.code}, message is ${e.message}`);
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
                    console.error('DeviceKvStoreGetBoolPromiseSucTest get fail' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(null).assertFail();
                });
            }).catch((err) => {
                console.error('DeviceKvStoreGetBoolPromiseSucTest put fail' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('DeviceKvStoreGetBoolPromiseSucTest fail' + `, error code is ${e.code}, message is ${e.message}`);
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
                console.error('DeviceKvStorePutFloatPromiseSucTest put fail' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('DeviceKvStorePutFloatPromiseSucTest fail' + `, error code is ${e.code}, message is ${e.message}`);
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
                    console.error('DeviceKvStoreGetFloatPromiseSucTest get fail' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(null).assertFail();
                });
            }).catch((err) => {
                console.error('DeviceKvStoreGetFloatPromiseSucTest put fail' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('DeviceKvStoreGetFloatPromiseSucTest fail' + `, error code is ${e.code}, message is ${e.message}`);
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
                    console.error('DeviceKvStoreDeleteStringPromiseSucTest delete fail' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(null).assertFail();
                });
            }).catch((err) => {
                console.error('DeviceKvStoreDeleteStringPromiseSucTest put fail' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('DeviceKvStoreDeleteStringPromiseSucTest fail' + `, error code is ${e.code}, message is ${e.message}`);
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
                console.error('DeviceKvStoreDeleteStringPromiseClosedKVStoreTest put fail' + `, error code is ${err.code}, message is ${err.message}`);
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
                console.error('DeviceKvStoreDeleteStringPromiseSucTest delete fail' + `, error code is ${err.code}, message is ${err.message}`);
                expect(err.code == 15100005).assertTrue();
            });
        } catch (e) {
            console.error('DeviceKvStoreDeleteStringPromiseClosedKVStoreTest fail' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreDeletePredicatesPromiseTest
     * @tc.desc Test Js Api DeviceKvStore.Delete()
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreDeletePredicatesPromiseTest', 0, async function (done) {
        console.log('DeviceKvStoreDeletePredicatesPromiseTest');
        try {
            let predicates = new dataShare.DataSharePredicates();
            let arr = ["name"];
            predicates.inKeys(arr);
            await kvStore.delete(predicates).then((data) => {
                console.error('DeviceKvStoreDeletePredicatesPromiseTest delete success');
                expect(null).assertFail();
            }).catch((err) => {
                console.error('DeviceKvStoreDeletePredicatesPromiseTest delete fail' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });

        } catch (e) {
            console.info('DeviceKvStoreDeletePredicatesPromiseTest fail' + `, error code is ${e.code}, message is ${e.message}`);
            expect(e.code == 202).assertTrue();
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
                console.error('DeviceKvStoreSetSyncRangePromiseDisjointTest setSyncRange fail' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('DeviceKvStoreSetSyncRangePromiseDisjointTest fail' + `, error code is ${e.code}, message is ${e.message}`);
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
                console.error('DeviceKvStoreSetSyncRangePromiseJointTest setSyncRange fail' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('DeviceKvStoreSetSyncRangePromiseJointTest fail' + `, error code is ${e.code}, message is ${e.message}`);
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
                console.error('DeviceKvStoreSetSyncRangePromiseSameTest setSyncRange fail' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('DeviceKvStoreSetSyncRangePromiseSameTest fail' + `, error code is ${e.code}, message is ${e.message}`);
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
        console.info('DeviceKvStoreSetSyncRangePromiseInvalidArgsTest');
        try {
            var remoteSupportLabels = ['A', 'B'];
            await kvStore.setSyncRange(remoteSupportLabels).then((err) => {
                console.info('DeviceKvStoreSetSyncRangePromiseInvalidArgsTest setSyncRange success');
                expect(null).assertFail();
            }).catch((err) => {
                console.error('DeviceKvStoreSetSyncRangePromiseInvalidArgsTest setSyncRange fail' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('DeviceKvStoreSetSyncRangePromiseInvalidArgsTest fail' + `, error code is ${e.code}, message is ${e.message}`);
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
                    console.error('DeviceKvStorePutBatchPromiseStringTest getEntries fail' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(null).assertFail();
                });
            }).catch((err) => {
                console.error('DeviceKvStorePutBatchPromiseStringTest putBatch fail' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('DeviceKvStorePutBatchPromiseStringTest fail' + `, error code is ${e.code}, message is ${e.message}`);
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
                    console.error('DeviceKvStorePutBatchPromiseIntegerTest getEntries fail' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(null).assertFail();
                });
            }).catch((err) => {
                console.error('DeviceKvStorePutBatchPromiseIntegerTest putBatch fail' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('DeviceKvStorePutBatchPromiseIntegerTest fail' + `, error code is ${e.code}, message is ${e.message}`);
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
                    console.error('DeviceKvStorePutBatchPromiseFloatTest getEntries fail' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(null).assertFail();
                });
            }).catch((err) => {
                console.error('DeviceKvStorePutBatchPromiseFloatTest putBatch fail' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('DeviceKvStorePutBatchPromiseFloatTest fail' + `, error code is ${e.code}, message is ${e.message}`);
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
                    console.error('DeviceKvStorePutBatchPromiseDoubleTest getEntries fail' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(null).assertFail();
                });
            }).catch((err) => {
                console.error('DeviceKvStorePutBatchPromiseDoubleTest putBatch fail' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('DeviceKvStorePutBatchPromiseDoubleTest fail' + `, error code is ${e.code}, message is ${e.message}`);
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
                    console.error('DeviceKvStorePutBatchPromiseBooleanTest getEntries fail' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(null).assertFail();
                });
            }).catch((err) => {
                console.error('DeviceKvStorePutBatchPromiseBooleanTest putBatch fail' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('DeviceKvStorePutBatchPromiseBooleanTest fail' + `, error code is ${e.code}, message is ${e.message}`);
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
                    console.error('DeviceKvStorePutBatchPromiseByteArrayTest getEntries fail' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(null).assertFail();
                });
            }).catch((err) => {
                console.error('DeviceKvStorePutBatchPromiseByteArrayTest putBatch fail' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('DeviceKvStorePutBatchPromiseByteArrayTest fail' + `, error code is ${e.code}, message is ${e.message}`);
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
                console.error('DeviceKvStorePutBatchValuePromiseUint8ArrayTest PutBatch success');
                expect(null).assertFail();
            }).catch((err) => {
                console.error('DeviceKvStorePutBatchValuePromiseUint8ArrayTest PutBatch fail' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            console.info('DeviceKvStorePutBatchValuePromiseUint8ArrayTest fail' + `, error code is ${e.code}, message is ${e.message}`);
            expect(e.code == 202).assertTrue();
        }
        done();
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
                console.error('DeviceKvStorePutBatchValuePromiseStringTest putBatch success');
                expect(null).assertFail();
            }).catch((err) => {
                console.error('DeviceKvStorePutBatchValuePromiseStringTest PutBatch fail' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            console.info('DeviceKvStorePutBatchValuePromiseStringTest fail' + `, error code is ${e.code}, message is ${e.message}`);
            expect(e.code == 202).assertTrue();
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
                console.error('DeviceKvStorePutBatchValuePromiseNumbersTest putBatch success');
                expect(null).assertFail();
            }).catch((err) => {
                console.error('DeviceKvStorePutBatchValuePromiseNumbersTest putBatch fail' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            console.info('DeviceKvStorePutBatchValuePromiseNumbersTest fail' + `, error code is ${e.code}, message is ${e.message}`);
            expect(e.code == 202).assertTrue();
        }
        done();
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
                console.error('DeviceKvStorePutBatchValuePromiseBooleanTest putBatch success');
                expect(null).assertFail();
            }).catch((err) => {
                console.error('DeviceKvStorePutBatchValuePromiseBooleanTest putBatch fail' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            console.info('DeviceKvStorePutBatchValuePromiseBooleanTest fail' + `, error code is ${e.code}, message is ${e.message}`);
            expect(e.code == 202).assertTrue();
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
                console.error('DeviceKvStorePutBatchValuePromiseNullTest putBatch success');
                expect(null).assertFail();
            }).catch((err) => {
                console.error('DeviceKvStorePutBatchValuePromiseNullTest putBatch fail' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            console.info('DeviceKvStorePutBatchValuePromiseNullTest fail' + `, error code is ${e.code}, message is ${e.message}`);
            expect(e.code == 202).assertTrue();
        }
            done();
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
            console.error('DeviceKvStorePutBatchValuePromiseInvalidArgsTest fail' + `, error code is ${e.code}, message is ${e.message}`);
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
                console.error('DeviceKvStorePutBatchValuePromiseClosedKvstoreTest putBatch success');
                expect(null).assertFail();
            }).catch((err) => {
                console.error('DeviceKvStorePutBatchValuePromiseClosedKvstoreTest putBatch fail' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            console.info('DeviceKvStorePutBatchValuePromiseClosedKvstoreTest fail' + `, error code is ${e.code}, message is ${e.message}`);
            expect(e.code == 202).assertTrue();
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
            console.error('DeviceKvStoreDeleteBatchPromiseSucTest fail' + `, error code is ${e.code}, message is ${e.message}`);
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
            console.error('DeviceKvStoreDeleteBatchPromiseInvalidArgsTest fail' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

})
