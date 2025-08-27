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

})
