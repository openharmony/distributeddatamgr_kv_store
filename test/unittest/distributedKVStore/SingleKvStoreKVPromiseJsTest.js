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
const TEST_STORE_ID = 'storeId1';
var kvManager = null;
var kvStore = null;
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

describe('SingleKvStorePromiseTest', function () {
    const config = {
        bundleName: TEST_BUNDLE_NAME,
        context: context
    }

    const options = {
        createIfMissing: true,
        encrypt: false,
        backup: true,
        autoSync: true,
        kvStoreType: factory.KVStoreType.SINGLE_VERSION,
        schema: '',
        securityLevel: factory.SecurityLevel.S2,
    }

    beforeAll(async function (done) {
        console.info('beforeAll config:' + JSON.stringify(config));
        kvManager =  factory.createKVManager(config);
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
     * @tc.name SingleKvStorePutStringPromiseInvalidArgsTest
     * @tc.desc Test Js Api SingleKvStore.Put(String) with invalid args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStorePutStringPromiseInvalidArgsTest', 0, async function (done) {
        console.info('SingleKvStorePutStringPromiseInvalidArgsTest');
        try {
            await kvStore.put(KEY_TEST_STRING_ELEMENT, null).then((data) => {
                console.info('SingleKvStorePutStringPromiseInvalidArgsTest put success');
                expect(null).assertFail();
            }).catch((error) => {
                console.error('SingleKvStorePutStringPromiseInvalidArgsTest put error' + `, error code is ${error.code}, message is ${error.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('SingleKvStorePutStringPromiseInvalidArgsTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(e.code == 401).assertTrue();
        }
        done();
    })

    /**
     * @tc.name SingleKvStorePutStringPromiseClosedKVStoreTest
     * @tc.desc Test Js Api SingleKvStore.Put(String) in a closed kvstore
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStorePutStringPromiseClosedKVStoreTest', 0, async function (done) {
        console.info('SingleKvStorePutStringPromiseClosedKVStoreTest');
        try {
            await kvManager.closeKVStore(TEST_BUNDLE_NAME, TEST_STORE_ID).then(async () => {
                await kvManager.deleteKVStore(TEST_BUNDLE_NAME, TEST_STORE_ID).then(() => {
                    expect(true).assertTrue();
                }).catch((err) => {
                    console.error('deleteKVStore err ' + `, error code is ${err.code}, message is ${err.message}`);
                });
            })
            await kvStore.put(KEY_TEST_STRING_ELEMENT, VALUE_TEST_STRING_ELEMENT).then((data) => {
                console.info('SingleKvStorePutStringPromiseClosedKVStoreTest put success');
                expect(null).assertFail();
            }).catch((error) => {
                console.error('SingleKvStorePutStringPromiseClosedKVStoreTest put error' + `, error code is ${error.code}, message is ${error.message}`);
                expect(error.code == 15100005).assertTrue();
            });
        } catch (e) {
            console.error('SingleKvStorePutStringPromiseClosedKVStoreTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStorePutStringPromiseSucTest
     * @tc.desc Test Js Api SingleKvStore.Put(String) success
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStorePutStringPromiseSucTest', 0, async function (done) {
        console.info('SingleKvStorePutStringPromiseSucTest');
        try {
            await kvStore.put(KEY_TEST_STRING_ELEMENT, VALUE_TEST_STRING_ELEMENT).then((data) => {
                console.info('SingleKvStorePutStringPromiseSucTest put success');
                expect(data == undefined).assertTrue();
            }).catch((error) => {
                console.error('SingleKvStorePutStringPromiseSucTest put error' + `, error code is ${error.code}, message is ${error.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('SingleKvStorePutStringPromiseSucTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreGetStringPromiseSucTest
     * @tc.desc Test Js Api SingleKvStoreGetString success
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreGetStringPromiseSucTest', 0, async function (done) {
        console.info('SingleKvStoreGetStringPromiseSucTest');
        try {
            await kvStore.put(KEY_TEST_STRING_ELEMENT, VALUE_TEST_STRING_ELEMENT).then(async (data) => {
                expect(data == undefined).assertTrue();
                await kvStore.get(KEY_TEST_STRING_ELEMENT).then((data) => {
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
     * @tc.name SingleKvStoreGetStringPromiseInvalidArgsTest
     * @tc.desc Test Js Api SingleKvStoreGetString with invalid args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreGetStringPromiseInvalidArgsTest', 0, async function (done) {
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
            console.error('SingleKvStoreGetStringPromiseInvalidArgsTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreGetStringPromiseClosedKVStoreTest
     * @tc.desc Test Js Api SingleKvStoreGetString from a closed kvstore
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreGetStringPromiseClosedKVStoreTest', 0, async function (done) {
        try {
            await kvStore.put(KEY_TEST_STRING_ELEMENT, VALUE_TEST_STRING_ELEMENT).then(async (data) => {
                expect(data == undefined).assertTrue();
            }).catch((error) => {
                console.error('SingleKvStoreGetStringPromiseClosedKVStoreTest put error' + `, error code is ${error.code}, message is ${error.message}`);
                expect(null).assertFail();
            });
            await kvManager.closeKVStore(TEST_BUNDLE_NAME, TEST_STORE_ID).then(async () => {
                await kvManager.deleteKVStore(TEST_BUNDLE_NAME, TEST_STORE_ID).then(() => {
                    expect(true).assertTrue();
                }).catch((err) => {
                    console.error('deleteKVStore err ' + `, error code is ${err.code}, message is ${err.message}`);
                });
            })
            await kvStore.get(KEY_TEST_STRING_ELEMENT).then((data) => {
                expect(null).assertFail();
            }).catch((err) => {
                expect(err.code == 15100005).assertTrue();
            });
        } catch (e) {
            console.error('SingleKvStoreGetStringPromiseClosedKVStoreTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreGetStringPromiseNoPutTest
     * @tc.desc Test Js Api SingleKvStoreGetString without put
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreGetStringPromiseNoPutTest', 0, async function (done) {
        console.info('SingleKvStoreGetStringPromiseNoPutTest');
        try {
            await kvStore.get(KEY_TEST_STRING_ELEMENT).then((data) => {
                console.info('SingleKvStoreGetStringPromiseNoPutTest get success');
                expect(null).assertFail();
            }).catch((err) => {
                console.error('SingleKvStoreGetStringPromiseNoPutTest get fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(err.code == 15100004).assertTrue();
            });
        } catch (e) {
            console.error('SingleKvStoreGetStringPromiseNoPutTest get e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStorePutIntPromiseSucTest
     * @tc.desc Test Js Api SingleKvStore.Put(Int) success
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStorePutIntPromiseSucTest', 0, async function (done) {
        console.info('SingleKvStorePutIntPromiseSucTest');
        try {
            await kvStore.put(KEY_TEST_INT_ELEMENT, VALUE_TEST_INT_ELEMENT).then((data) => {
                console.info('SingleKvStorePutIntPromiseSucTest put success');
                expect(data == undefined).assertTrue();
            }).catch((err) => {
                console.error('SingleKvStorePutIntPromiseSucTest put fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('SingleKvStorePutIntPromiseSucTest put e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStorePutIntPromiseMaxTest
     * @tc.desc Test Js Api SingleKvStore.Put(Int) with max value
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStorePutIntPromiseMaxTest', 0, async function (done) {
        console.info('SingleKvStorePutIntPromiseMaxTest');
        try {
            var intValue = Number.MAX_VALUE;
            await kvStore.put(KEY_TEST_INT_ELEMENT, intValue).then(async (data) => {
                console.info('SingleKvStorePutIntPromiseMaxTest put success');
                expect(data == undefined).assertTrue();
                await kvStore.get(KEY_TEST_INT_ELEMENT).then((data) => {
                    console.info('SingleKvStorePutIntPromiseMaxTest get success');
                    expect(intValue == data).assertTrue();
                }).catch((err) => {
                    console.error('SingleKvStorePutIntPromiseMaxTest get fail ' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(null).assertFail();
                });
            }).catch((err) => {
                console.error('SingleKvStorePutIntPromiseMaxTest put fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('SingleKvStorePutIntPromiseMaxTest put e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStorePutIntPromiseMinTest
     * @tc.desc Test Js Api SingleKvStore.Put(Int) with min value
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStorePutIntPromiseMinTest', 0, async function (done) {
        console.info('SingleKvStorePutIntPromiseMinTest');
        try {
            var intValue = Number.MIN_VALUE;
            await kvStore.put(KEY_TEST_INT_ELEMENT, intValue).then(async (data) => {
                console.info('SingleKvStorePutIntPromiseMinTest put success');
                expect(data == undefined).assertTrue();
                await kvStore.get(KEY_TEST_INT_ELEMENT).then((data) => {
                    console.info('SingleKvStorePutIntPromiseMinTest get success');
                    expect(intValue == data).assertTrue();
                }).catch((err) => {
                    console.error('SingleKvStorePutIntPromiseMinTest get fail ' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(null).assertFail();
                });
            }).catch((err) => {
                console.error('SingleKvStorePutIntPromiseMinTest put fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('SingleKvStorePutIntPromiseMinTest put e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreGetIntPromiseSucTest
     * @tc.desc Test Js Api SingleKvStoreGetInt success
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreGetIntPromiseSucTest', 0, async function (done) {
        console.info('SingleKvStoreGetIntPromiseSucTest');
        try {
            await kvStore.put(KEY_TEST_INT_ELEMENT, VALUE_TEST_INT_ELEMENT).then(async (data) => {
                console.info('SingleKvStoreGetIntPromiseSucTest put success');
                expect(data == undefined).assertTrue();
                await kvStore.get(KEY_TEST_INT_ELEMENT).then((data) => {
                    console.info('SingleKvStoreGetIntPromiseSucTest get success');
                    expect(VALUE_TEST_INT_ELEMENT == data).assertTrue();
                }).catch((err) => {
                    console.error('SingleKvStoreGetIntPromiseSucTest get fail ' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(null).assertFail();
                });
            }).catch((err) => {
                console.error('SingleKvStoreGetIntPromiseSucTest put fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('SingleKvStoreGetIntPromiseSucTest put e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStorePutBoolPromiseSucTest
     * @tc.desc Test Js Api SingleKvStore.Put(Bool) success
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStorePutBoolPromiseSucTest', 0, async function (done) {
        console.info('SingleKvStorePutBoolPromiseSucTest');
        try {
            await kvStore.put(KEY_TEST_BOOLEAN_ELEMENT, VALUE_TEST_BOOLEAN_ELEMENT).then((data) => {
                console.info('SingleKvStorePutBoolPromiseSucTest put success');
                expect(data == undefined).assertTrue();
            }).catch((err) => {
                console.error('SingleKvStorePutBoolPromiseSucTest put fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('SingleKvStorePutBoolPromiseSucTest put e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreGetBoolPromiseSucTest
     * @tc.desc Test Js Api SingleKvStoreGetBool success
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreGetBoolPromiseSucTest', 0, async function (done) {
        console.info('SingleKvStoreGetBoolPromiseSucTest');
        try {
            var boolValue = false;
            await kvStore.put(KEY_TEST_BOOLEAN_ELEMENT, boolValue).then(async (data) => {
                console.info('SingleKvStoreGetBoolPromiseSucTest put success');
                expect(data == undefined).assertTrue();
                await kvStore.get(KEY_TEST_BOOLEAN_ELEMENT).then((data) => {
                    console.info('SingleKvStoreGetBoolPromiseSucTest get success');
                    expect(boolValue == data).assertTrue();
                }).catch((err) => {
                    console.error('SingleKvStoreGetBoolPromiseSucTest get fail ' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(null).assertFail();
                });
            }).catch((err) => {
                console.error('SingleKvStoreGetBoolPromiseSucTest put fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('SingleKvStoreGetBoolPromiseSucTest put e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStorePutFloatPromiseSucTest
     * @tc.desc Test Js Api SingleKvStore.Put(Float) success
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStorePutFloatPromiseSucTest', 0, async function (done) {
        console.info('SingleKvStorePutFloatPromiseSucTest');
        try {
            await kvStore.put(KEY_TEST_FLOAT_ELEMENT, VALUE_TEST_FLOAT_ELEMENT).then((data) => {
                console.info('SingleKvStorePutFloatPromiseSucTest put success');
                expect(data == undefined).assertTrue();
            }).catch((err) => {
                console.error('SingleKvStorePutFloatPromiseSucTest put fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('SingleKvStorePutFloatPromiseSucTest put e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreGetFloatPromiseSucTest
     * @tc.desc Test Js Api SingleKvStoreGetFloat success
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreGetFloatPromiseSucTest', 0, async function (done) {
        console.info('SingleKvStoreGetFloatPromiseSucTest');
        try {
            var floatValue = 123456.654321;
            await kvStore.put(KEY_TEST_FLOAT_ELEMENT, floatValue).then(async (data) => {
                console.info('SingleKvStoreGetFloatPromiseSucTest put success');
                expect(data == undefined).assertTrue();
                await kvStore.get(KEY_TEST_FLOAT_ELEMENT).then((data) => {
                    console.info('SingleKvStoreGetFloatPromiseSucTest get success');
                    expect(floatValue == data).assertTrue();
                }).catch((err) => {
                    console.error('SingleKvStoreGetFloatPromiseSucTest get fail ' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(null).assertFail();
                });
            }).catch((err) => {
                console.error('SingleKvStoreGetFloatPromiseSucTest put fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('SingleKvStoreGetFloatPromiseSucTest put e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreDeleteStringPromiseSucTest
     * @tc.desc Test Js Api SingleKvStoreDeleteString success
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreDeleteStringPromiseSucTest', 0, async function (done) {
        console.info('SingleKvStoreDeleteStringPromiseSucTest');
        try {
            var str = 'this is a test string';
            await kvStore.put(KEY_TEST_STRING_ELEMENT, str).then(async (data) => {
                console.info('SingleKvStoreDeleteStringPromiseSucTest put success');
                expect(data == undefined).assertTrue();
                await kvStore.delete(KEY_TEST_STRING_ELEMENT).then((data) => {
                    console.info('SingleKvStoreDeleteStringPromiseSucTest delete success');
                    expect(data == undefined).assertTrue();
                }).catch((err) => {
                    console.error('SingleKvStoreDeleteStringPromiseSucTest delete fail ' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(null).assertFail();
                });
            }).catch((err) => {
                console.error('SingleKvStoreDeleteStringPromiseSucTest put fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('SingleKvStoreDeleteStringPromiseSucTest put e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreDeleteStringPromiseInvalidArgsTest
     * @tc.desc Test Js Api SingleKvStoreDeleteString with invalid args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreDeleteStringPromiseInvalidArgsTest', 0, async function (done) {
        console.info('SingleKvStoreDeleteStringPromiseInvalidArgsTest');
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
     * @tc.name SingleKvStoreDeleteStringPromiseClosedKVStoreTest
     * @tc.desc Test Js Api SingleKvStoreDeleteString into a closed kvstore
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreDeleteStringPromiseClosedKVStoreTest', 0, async function (done) {
        try {
            let str = "test";

            await kvStore.put(KEY_TEST_STRING_ELEMENT, str).then(async () => {
                console.info('SingleKvStoreDeleteStringPromiseSucTest put success');
                expect(true).assertTrue();
            }).catch((err) => {
                console.error('SingleKvStoreDeleteStringPromiseClosedKVStoreTest put fail ' + `, error code is ${err.code}, message is ${err.message}`);
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
                console.info('SingleKvStoreDeleteStringPromiseSucTest delete success');
                expect(null).assertFail();
            }).catch((err) => {
                console.error('SingleKvStoreDeleteStringPromiseSucTest delete fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(err.code == 15100005).assertTrue();
            });
        } catch (e) {
            console.error('SingleKvStorePutStringPromiseClosedKVStoreTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreDeletePredicatesPromiseSucTest
     * @tc.desc Test Js Api SingleKvStore.Delete() success
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreDeletePredicatesPromiseSucTest', 0, async function (done) {
        console.log('SingleKvStoreDeletePredicatesPromiseSucTest');
        try {
            let predicates = new dataShare.DataSharePredicates();
            let arr = ["name"];
            predicates.inKeys(arr);
            await kvStore.put("name", "Bob").then(async (data) => {
                console.log('SingleKvStoreDeletePredicatesPromiseSucTest put success');
                expect(data == undefined).assertTrue();
                await kvStore.delete(predicates).then((data) => {
                    console.log('SingleKvStoreDeletePredicatesPromiseSucTest delete success');
                    expect(data == undefined).assertTrue();
                }).catch((err) => {
                    console.error('SingleKvStoreDeletePredicatesPromiseSucTest delete fail ' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(null).assertFail();
                });
            }).catch((err) => {
                console.error('SingleKvStoreDeletePredicatesPromiseSucTest put fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('SingleKvStoreDeletePredicatesPromiseSucTest put e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreDeletePredicatesPromiseClosedKVStoreTest
     * @tc.desc Test Js Api SingleKvStoreDelete predicates into a closed kvstore
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreDeletePredicatesPromiseClosedKVStoreTest', 0, async function (done) {
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
            console.error('SingleKvStoreDeletePredicatesPromiseClosedKVStoreTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreDeletePredicatesPromiseInvalidArgsTest
     * @tc.desc Test Js Api SingleKvStoreDelete predicates with invalid args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreDeleteStringPromiseInvalidArgsTest', 0, async function (done) {
        console.info('SingleKvStoreDeleteStringPromiseInvalidArgsTest');
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
     * @tc.name SingleKvStoreSetSyncRangePromiseDisjointTest
     * @tc.desc Test Js Api SingleKvStoreSetSyncRange with disjoint ranges
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreSetSyncRangePromiseDisjointTest', 0, async function (done) {
        console.info('SingleKvStoreSetSyncRangePromiseDisjointTest');
        try {
            var localLabels = ['A', 'B'];
            var remoteSupportLabels = ['C', 'D'];
            await kvStore.setSyncRange(localLabels, remoteSupportLabels).then((err) => {
                console.info('SingleKvStoreSetSyncRangePromiseDisjointTest setSyncRange success');
                expect(err == undefined).assertTrue();
            }).catch((err) => {
                console.error('SingleKvStoreSetSyncRangePromiseDisjointTest delete fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('SingleKvStoreSetSyncRangePromiseDisjointTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreSetSyncRangePromiseJointTest
     * @tc.desc Test Js Api SingleKvStoreSetSyncRange with joint range
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreSetSyncRangePromiseJointTest', 0, async function (done) {
        console.info('SingleKvStoreSetSyncRangePromiseJointTest');
        try {
            var localLabels = ['A', 'B'];
            var remoteSupportLabels = ['B', 'C'];
            await kvStore.setSyncRange(localLabels, remoteSupportLabels).then((err) => {
                console.info('SingleKvStoreSetSyncRangePromiseJointTest setSyncRange success');
                expect(err == undefined).assertTrue();
            }).catch((err) => {
                console.error('SingleKvStoreSetSyncRangePromiseJointTest delete fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('SingleKvStoreSetSyncRangePromiseJointTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreSetSyncRangePromiseSameTest
     * @tc.desc Test Js Api SingleKvStoreSetSyncRange with same range
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreSetSyncRangePromiseSameTest', 0, async function (done) {
        console.info('SingleKvStoreSetSyncRangePromiseSameTest');
        try {
            var localLabels = ['A', 'B'];
            var remoteSupportLabels = ['A', 'B'];
            await kvStore.setSyncRange(localLabels, remoteSupportLabels).then((err) => {
                console.info('SingleKvStoreSetSyncRangePromiseSameTest setSyncRange success');
                expect(err == undefined).assertTrue();
            }).catch((err) => {
                console.error('SingleKvStoreSetSyncRangePromiseSameTest delete fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('SingleKvStoreSetSyncRangePromiseSameTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreSetSyncRangePromiseInvalidArgsTest
     * @tc.desc Test Js Api SingleKvStoreSetSyncRange with invalid args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreSetSyncRangePromiseInvalidArgsTest', 0, async function (done) {
        console.info('SingleKvStoreSetSyncRangePromiseSameTest');
        try {
            var remoteSupportLabels = ['A', 'B'];
            await kvStore.setSyncRange(remoteSupportLabels).then((err) => {
                expect(null).assertFail();
            }).catch((err) => {
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('SingleKvStoreSetSyncRangePromiseSameTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(e.code == 401).assertTrue();
        }
        done();
    })

    /**
     * @tc.name SingleKvStorePutBatchPromiseStringTest
     * @tc.desc Test Js Api SingleKvStore.PutBatch() with string value
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStorePutBatchPromiseStringTest', 0, async function (done) {
        console.info('SingleKvStorePutBatchPromiseStringTest');
        try {
            let entries = putBatchString(10, 'batch_test_string_key');
            console.info('SingleKvStorePutBatchPromiseStringTest entries: ' + JSON.stringify(entries));
            await kvStore.putBatch(entries).then(async (err) => {
                console.info('SingleKvStorePutBatchPromiseStringTest putBatch success');
                expect(err == undefined).assertTrue();
                await kvStore.getEntries('batch_test_string_key').then((entrys) => {
                    console.info('SingleKvStorePutBatchPromiseStringTest getEntries success');
                    console.info('SingleKvStorePutBatchPromiseStringTest ' + JSON.stringify(entries));
                    expect(entrys.length == 10).assertTrue();
                    expect(entrys[0].value.value == 'batch_test_string_value').assertTrue();
                }).catch((err) => {
                    console.error('SingleKvStorePutBatchPromiseStringTest getEntries fail ' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(null).assertFail();
                });
            }).catch((err) => {
                console.error('SingleKvStorePutBatchPromiseStringTest putBatch fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('SingleKvStorePutBatchPromiseStringTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStorePutBatchPromiseIntegerTest
     * @tc.desc Test Js Api SingleKvStore.PutBatch() with integer value
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStorePutBatchPromiseIntegerTest', 0, async function (done) {
        console.info('SingleKvStorePutBatchPromiseIntegerTest');
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
            console.info('SingleKvStorePutBatchPromiseIntegerTest entries: ' + JSON.stringify(entries));
            await kvStore.putBatch(entries).then(async (err) => {
                console.info('SingleKvStorePutBatchPromiseIntegerTest putBatch success');
                expect(err == undefined).assertTrue();
                await kvStore.getEntries('batch_test_number_key').then((entrys) => {
                    console.info('SingleKvStorePutBatchPromiseIntegerTest getEntries success');
                    expect(entrys.length == 10).assertTrue();
                    expect(entrys[0].value.value == 222).assertTrue();
                }).catch((err) => {
                    console.error('SingleKvStorePutBatchPromiseIntegerTest getEntries fail ' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(null).assertFail();
                });
            }).catch((err) => {
                console.error('SingleKvStorePutBatchPromiseIntegerTest putBatch fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('SingleKvStorePutBatchPromiseIntegerTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStorePutBatchPromiseFloatTest
     * @tc.desc Test Js Api SingleKvStore.PutBatch() with float value
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStorePutBatchPromiseFloatTest', 0, async function (done) {
        console.info('SingleKvStorePutBatchPromiseFloatTest');
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
            console.info('SingleKvStorePutBatchPromiseFloatTest entries: ' + JSON.stringify(entries));
            await kvStore.putBatch(entries).then(async (err) => {
                console.info('SingleKvStorePutBatchPromiseFloatTest putBatch success');
                expect(err == undefined).assertTrue();
                await kvStore.getEntries('batch_test_number_key').then((entrys) => {
                    console.info('SingleKvStorePutBatchPromiseFloatTest getEntries success');
                    expect(entrys.length == 10).assertTrue();
                    expect(entrys[0].value.value == 2.0).assertTrue();
                }).catch((err) => {
                    console.error('SingleKvStorePutBatchPromiseFloatTest getEntries fail ' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(null).assertFail();
                });
            }).catch((err) => {
                console.error('SingleKvStorePutBatchPromiseFloatTest putBatch fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('SingleKvStorePutBatchPromiseFloatTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStorePutBatchPromiseDoubleTest
     * @tc.desc Test Js Api SingleKvStore.PutBatch() with double value
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStorePutBatchPromiseDoubleTest', 0, async function (done) {
        console.info('SingleKvStorePutBatchPromiseDoubleTest');
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
            console.info('SingleKvStorePutBatchPromiseDoubleTest entries: ' + JSON.stringify(entries));
            await kvStore.putBatch(entries).then(async (err) => {
                console.info('SingleKvStorePutBatchPromiseDoubleTest putBatch success');
                expect(err == undefined).assertTrue();
                await kvStore.getEntries('batch_test_number_key').then((entrys) => {
                    console.info('SingleKvStorePutBatchPromiseDoubleTest getEntries success');
                    expect(entrys.length == 10).assertTrue();
                    expect(entrys[0].value.value == 2.00).assertTrue();
                }).catch((err) => {
                    console.error('SingleKvStorePutBatchPromiseDoubleTest getEntries fail ' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(null).assertFail();
                });
            }).catch((err) => {
                console.error('SingleKvStorePutBatchPromiseDoubleTest putBatch fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('SingleKvStorePutBatchPromiseDoubleTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStorePutBatchPromiseBooleanTest
     * @tc.desc Test Js Api SingleKvStore.PutBatch() with boolean value
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStorePutBatchPromiseBooleanTest', 0, async function (done) {
        console.info('SingleKvStorePutBatchPromiseBooleanTest');
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
            console.info('SingleKvStorePutBatchPromiseBooleanTest entries: ' + JSON.stringify(entries));
            await kvStore.putBatch(entries).then(async (err) => {
                console.info('SingleKvStorePutBatchPromiseBooleanTest putBatch success');
                expect(err == undefined).assertTrue();
                await kvStore.getEntries('batch_test_bool_key').then((entrys) => {
                    console.info('SingleKvStorePutBatchPromiseBooleanTest getEntries success');
                    expect(entrys.length == 10).assertTrue();
                    expect(entrys[0].value.value == bo).assertTrue();
                }).catch((err) => {
                    console.error('SingleKvStorePutBatchPromiseBooleanTest getEntries fail ' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(null).assertFail();
                });
            }).catch((err) => {
                console.error('SingleKvStorePutBatchPromiseBooleanTest putBatch fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('SingleKvStorePutBatchPromiseBooleanTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStorePutBatchPromiseByteArrayTest
     * @tc.desc Test Js Api SingleKvStore.PutBatch() with byte array value
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStorePutBatchPromiseByteArrayTest', 0, async function (done) {
        console.info('SingleKvStorePutBatchPromiseByteArrayTest');
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
            console.info('SingleKvStorePutBatchPromiseByteArrayTest entries: ' + JSON.stringify(entries));
            await kvStore.putBatch(entries).then(async (err) => {
                console.info('SingleKvStorePutBatchPromiseByteArrayTest putBatch success');
                expect(err == undefined).assertTrue();
                await kvStore.getEntries('batch_test_bool_key').then((entrys) => {
                    console.info('SingleKvStorePutBatchPromiseByteArrayTest getEntries success');
                    expect(entrys.length == 10).assertTrue();
                    expect(entrys[0].value.value.toString() == arr.toString()).assertTrue();
                }).catch((err) => {
                    console.error('SingleKvStorePutBatchPromiseByteArrayTest getEntries fail ' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(null).assertFail();
                });
            }).catch((err) => {
                console.error('SingleKvStorePutBatchPromiseByteArrayTest putBatch fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('SingleKvStorePutBatchPromiseBooleanTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStorePutBatchValuePromiseUint8ArrayTest
     * @tc.desc Test Js Api SingleKvStore.PutBatch() put Uint8Array
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStorePutBatchValuePromiseUint8ArrayTest', 0, async function (done) {
        console.info('SingleKvStorePutBatchValuePromiseUint8ArrayTest');
        try {
            let values = [];
            let arr1 = new Uint8Array([4, 5, 6, 7]);
            let arr2 = new Uint8Array([4, 5, 6, 7, 8]);
            let vb1 = {key: "name_1", value: arr1};
            let vb2 = {key: "name_2", value: arr2};
            values.push(vb1);
            values.push(vb2);
            console.info('SingleKvStorePutBatchValuePromiseUint8ArrayTest values: ' + JSON.stringify(values));
            await kvStore.putBatch(values).then(async (err) => {
                console.info('SingleKvStorePutBatchValuePromiseUint8ArrayTest putBatch success');
                expect(err == undefined).assertTrue();
                var query = new factory.Query();
                query.prefixKey("name_");
                await kvStore.getEntries(query).then((entrys) => {
                    expect(entrys.length == 2).assertTrue();
                    done();
                });
            });
        } catch (e) {
            console.error('SingleKvStorePutBatchValuePromiseUint8ArrayTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name SingleKvStorePutBatchValuePromiseStringTest
     * @tc.desc Test Js Api SingleKvStore.PutBatch() put String
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStorePutBatchValuePromiseStringTest', 0, async function (done) {
        console.info('SingleKvStorePutBatchValuePromiseStringTest');
        try {
            let values = [];
            let vb1 = {key: "name_1", value: "arr1"};
            let vb2 = {key: "name_2", value: "arr2"};
            values.push(vb1);
            values.push(vb2);
            console.info('SingleKvStorePutBatchValuePromiseStringTest values: ' + JSON.stringify(values));
            await kvStore.putBatch(values).then(async (err) => {
                console.info('SingleKvStorePutBatchValuePromiseUint8ArrayTest putBatch success');
                expect(err == undefined).assertTrue();
                var query = new factory.Query();
                query.prefixKey("name_");
                await kvStore.getEntries(query).then((entrys) => {
                    expect(entrys.length == 2).assertTrue();
                    done();
                }).catch((err) => {
                    console.log('SingleKvStorePutBatchValueTest delete fail ' + err);
                    expect(null).assertFail();
                });
            }).catch((err) => {
                console.log('SingleKvStorePutBatchValueTest delete fail ' + err);
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('SingleKvStorePutBatchValuePromiseStringTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStorePutBatchValuePromiseNumbersTest
     * @tc.desc Test Js Api SingleKvStore.PutBatch() put numbers
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStorePutBatchValuePromiseNumbersTest', 0, async function (done) {
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
                await kvStore.getEntries(query).then((entrys) => {
                    expect(entrys.length == 3).assertTrue();
                    done();
                });
            });
        } catch (e) {
            console.error('SingleKvStorePutBatchValuePromiseTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name SingleKvStorePutBatchValuePromiseBooleanTest
     * @tc.desc Test Js Api SingleKvStore.PutBatch() put booleans
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStorePutBatchValuePromiseBooleanTest', 0, async function (done) {
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
                await kvStore.getEntries(query).then((entrys) => {
                    expect(entrys.length == 2).assertTrue();
                    done();
                });
            });
        } catch (e) {
            console.error('SingleKvStorePutBatchValuePromiseTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name SingleKvStorePutBatchValuePromiseNullTest
     * @tc.desc Test Js Api SingleKvStore.PutBatch() put null
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStorePutBatchValuePromiseNullTest', 0, async function (done) {
        console.info('SingleKvStorePutBatchValuePromiseNullTest');
        try {
            let values = [];
            let vb1 = {key: "name_1", value: null};
            let vb2 = {key: "name_2", value: null};
            values.push(vb1);
            values.push(vb2);
            console.info('SingleKvStorePutBatchValuePromiseNullTest values: ' + JSON.stringify(values));
            await kvStore.putBatch(values).then(async (err) => {
                console.info('SingleKvStorePutBatchValuePromiseNullTest putBatch success');
                expect(err == undefined).assertTrue();
                var query = new factory.Query();
                query.prefixKey("name_");
                await kvStore.getEntries(query).then((entrys) => {
                    expect(entrys.length == 2).assertTrue();
                    expect(entrys[0].value == null).assertTrue();
                    done();
                });
            });
        } catch (e) {
            console.error('SingleKvStorePutBatchValuePromiseNullTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name SingleKvStorePutBatchValuePromiseInvalidArgsTest
     * @tc.desc Test Js Api SingleKvStore.PutBatch() put invalid args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStorePutBatchValuePromiseInvalidArgsTest', 0, async function (done) {
        try {
            await kvStore.putBatch().then(() => {
                expect(null).assertFail();
            }).catch((err) => {
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('SingleKvStorePutBatchValuePromiseNullTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(e.code == 401).assertTrue();
        }
        done();
    })

    /**
     * @tc.name SingleKvStorePutBatchValuePromiseClosedKvstoreTest
     * @tc.desc Test Js Api SingleKvStore.PutBatch() put into closed kvstore
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStorePutBatchValuePromiseClosedKvstoreTest', 0, async function (done) {
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
            console.error('SingleKvStorePutBatchValuePromiseNullTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreDeleteBatchPromiseSucTest
     * @tc.desc Test Js Api SingleKvStoreDeleteBatch success
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreDeleteBatchPromiseSucTest', 0, async function (done) {
        console.info('SingleKvStoreDeleteBatchPromiseSucTest');
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
            console.error('SingleKvStoreDeleteBatchPromiseSucTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreDeleteBatchPromiseInvalidArgsTest
     * @tc.desc Test Js Api SingleKvStoreDeleteBatch with invalid args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreDeleteBatchPromiseInvalidArgsTest', 0, async function (done) {
        console.info('SingleKvStoreDeleteBatchPromiseInvalidArgsTest');
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
            console.error('SingleKvStoreDeleteBatchPromiseInvalidArgsTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreDeleteBatchPromiseClosedKVStoreTest
     * @tc.desc Test Js Api SingleKvStoreDeleteBatch into closed kvstore
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreDeleteBatchPromiseClosedKVStoreTest', 0, async function (done) {
        console.info('SingleKvStoreDeleteBatchPromiseClosedKVStoreTest');
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
            console.error('SingleKvStoreDeleteBatchPromiseClosedKVStoreTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreGetEntriesPromiseQueryTest
     * @tc.desc Test Js Api SingleKvStore.GetEntries() with query
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreGetEntriesPromiseQueryTest', 0, async function (done) {
        console.info('SingleKvStoreGetEntriesPromiseQueryTest');
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
            console.info('SingleKvStoreGetEntriesPromiseQueryTest entries: ' + JSON.stringify(entries));
            await kvStore.putBatch(entries).then(async (err) => {
                console.info('SingleKvStoreGetEntriesPromiseQueryTest putBatch success');
                expect(err == undefined).assertTrue();
                let query = new factory.Query();
                query.prefixKey("batch_test");
                await kvStore.getEntries(query).then((entrys) => {
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
            console.error('SingleKvStoreGetEntriesPromiseQueryTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            console.log("errr3")
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreGetEntriesPromiseQueryClosedKVStoreTest
     * @tc.desc Test Js Api SingleKvStore.GetEntries() query from a closed kvstore
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreGetEntriesPromiseQueryClosedKVStoreTest', 0, async function (done) {
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
            await kvStore.getEntries(query).then(() => {
                expect(null).assertFail();
            }).catch((err) => {
                expect(err.code == 15100005).assertTrue();
            });
        } catch (e) {
            console.error('SingleKvStoreGetEntriesPromiseQueryClosedKVStoreTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreGetEntriesPromiseSucTest
     * @tc.desc Test Js Api SingleKvStore.GetEntries() success
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreGetEntriesPromiseSucTest', 0, async function (done) {
        console.info('SingleKvStoreGetEntriesPromiseSucTest');
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
                await kvStore.getEntries("batch_test").then((entrys) => {
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
            console.error('SingleKvStoreGetEntriesPromiseSucTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreGetEntriesPromiseClosedKVStoreTest
     * @tc.desc Test Js Api SingleKvStore.GetEntries() from a closed kvstore
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreGetEntriesPromiseClosedKVStoreTest', 0, async function (done) {
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
            console.error('SingleKvStoreGetEntriesPromiseSucTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreGetEntriesPromiseInvalidArgsTest
     * @tc.desc Test Js Api SingleKvStore.GetEntries() with invalid args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreGetEntriesPromiseInvalidArgsTest', 0, async function (done) {
        console.info('SingleKvStoreGetEntriesPromiseInvalidArgsTest');
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
            console.error('SingleKvStoreGetEntriesPromiseInvalidArgsTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStorestartTransactionPromiseCommitTest
     * @tc.desc Test Js Api SingleKvStorestartTransaction with commit
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStorestartTransactionPromiseCommitTest', 0, async function (done) {
        console.info('SingleKvStorestartTransactionPromiseCommitTest');
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
     * @tc.name SingleKvStorestartTransactionPromiseRollbackTest
     * @tc.desc Test Js Api SingleKvStorestartTransaction with Rollback
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStorestartTransactionPromiseRollbackTest', 0, async function (done) {
        console.info('SingleKvStorestartTransactionPromiseRollbackTest');
        try {
            var count = 0;
            kvStore.on('dataChange', 0, function (data) {
                console.info('SingleKvStorestartTransactionPromiseRollbackTest ' + JSON.stringify(data));
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
     * @tc.name SingleKvStorestartTransactionPromiseClosedKVStoreTest
     * @tc.desc Test Js Api SingleKvStorestartTransaction with a closed kvstore
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStorestartTransactionPromiseClosedKVStoreTest', 0, async function (done) {
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
     * @tc.name SingleKvStorestartTransactionPromiseInvalidArgsTest
     * @tc.desc Test Js Api SingleKvStorestartTransaction with invalid args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStorestartTransactionPromiseInvalidArgsTest', 0, async function (done) {
        console.info('SingleKvStorestartTransactionPromiseRollbackTest');
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
     * @tc.name SingleKvStoreCommitPromiseClosedKVStoreTest
     * @tc.desc Test Js Api SingleKvStoreCommit with a closed kvstore
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreCommitPromiseClosedKVStoreTest', 0, async function (done) {
        console.info('SingleKvStoreCommitPromiseClosedKVStoreTest');
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
     * @tc.name SingleKvStoreRollbackPromiseClosedKVStoreTest
     * @tc.desc Test Js Api SingleKvStoreRollback with a closed kvstore
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreRollbackPromiseClosedKVStoreTest', 0, async function (done) {
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
     * @tc.name SingleKvStoreEnableSyncPromiseTrueTest
     * @tc.desc Test Js Api SingleKvStoreEnableSync true
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreEnableSyncPromiseTrueTest', 0, async function (done) {
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
     * @tc.name SingleKvStoreEnableSyncPromiseFalseTest
     * @tc.desc Test Js Api SingleKvStoreEnableSync false
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreEnableSyncPromiseFalseTest', 0, async function (done) {
        console.info('SingleKvStoreEnableSyncPromiseFalseTest');
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
     * @tc.name SingleKvStoreEnableSyncPromiseInvalidArgsTest
     * @tc.desc Test Js Api SingleKvStoreEnableSync with invalid args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreEnableSyncPromiseInvalidArgsTest', 0, async function (done) {
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
     * @tc.name SingleKvStoreSetSyncRangePromiseSameTest
     * @tc.desc Test Js Api SingleKvStore.SetSyncRange() with same range
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreSetSyncRangePromiseSameTest', 0, async function (done) {
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
     * @tc.name SingleKvStoreSetSyncRangePromiseSameTest
     * @tc.desc Test Js Api SingleKvStore.SetSyncRange() with invalid args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreSetSyncRangePromiseSameTest', 0, async function (done) {
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
     * @tc.name SingleKvStoreRemoveDeviceDataPromiseInvalidArgsTest
     * @tc.desc Test Js Api SingleKvStoreRemoveDeviceData with invalid args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreRemoveDeviceDataPromiseInvalidArgsTest', 0, async function (done) {
        console.info('SingleKvStoreRemoveDeviceDataPromiseInvalidArgsTest');
        try {
            await kvStore.removeDeviceData().then((err) => {
                console.info('SingleKvStoreRemoveDeviceDataPromiseInvalidArgsTest removeDeviceData success');
                expect(null).assertFail();
            }).catch((err) => {
                console.error('SingleKvStoreRemoveDeviceDataPromiseInvalidArgsTest removeDeviceData fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('SingleKvStoreRemoveDeviceDataPromiseInvalidArgsTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(e.code == 401).assertTrue();
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreRemoveDeviceDataPromiseClosedKVStoreTest
     * @tc.desc Test Js Api SingleKvStoreRemoveDeviceData in a closed kvstore
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreRemoveDeviceDataPromiseClosedKVStoreTest', 0, async function (done) {
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
            console.error('SingleKvStoreRemoveDeviceDataPromiseClosedKVStoreTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreSetSyncParamPromiseSucTest
     * @tc.desc Test Js Api SingleKvStoreSetSyncParam success
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreSetSyncParamPromiseSucTest', 0, async function (done) {
        try {
            var defaultAllowedDelayMs = 500;
            await kvStore.setSyncParam(defaultAllowedDelayMs).then((err) => {
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
     * @tc.name SingleKvStoreSetSyncParamPromiseInvalidArgsTest
     * @tc.desc Test Js Api SingleKvStoreSetSyncParam with invalid args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreSetSyncParamPromiseInvalidArgsTest', 0, async function (done) {
        console.info('SingleKvStoreSetSyncParamPromiseInvalidArgsTest');
        try {
            await kvStore.setSyncParam().then((err) => {
                expect(null).assertFail();
            }).catch((err) => {
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('SingleKvStoreSetSyncParamPromiseInvalidArgsTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(e.code == 401).assertTrue();
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreGetSecurityLevelPromiseSucTest
     * @tc.desc Test Js Api SingleKvStoreGetSecurityLevel success
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreGetSecurityLevelPromiseSucTest', 0, async function (done) {
        console.info('SingleKvStoreGetSecurityLevelPromiseSucTest');
        try {
            await kvStore.getSecurityLevel().then((data) => {
                expect(data == factory.SecurityLevel.S2).assertTrue();
            }).catch((err) => {
                expect(null).assertFail();
            });
        } catch (e) {
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreGetSecurityLevelPromiseClosedKvStoreTest
     * @tc.desc Test Js Api SingleKvStoreGetSecurityLevel from a closed kvstore
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreGetSecurityLevelPromiseSucTest', 0, async function (done) {
        console.info('SingleKvStoreGetSecurityLevelPromiseSucTest');
        try {
            await kvManager.closeKVStore(TEST_BUNDLE_NAME, TEST_STORE_ID).then(async () => {
                await kvManager.deleteKVStore(TEST_BUNDLE_NAME, TEST_STORE_ID).then(() => {
                    expect(true).assertTrue();
                }).catch((err) => {
                    console.error('deleteKVStore err ' + `, error code is ${err.code}, message is ${err.message}`);
                });
            })
            await kvStore.getSecurityLevel().then((data) => {
                expect(null).assertFail();
            }).catch((err) => {
                console.error('SingleKvStoreGetSecurityLevelPromiseSucTest getSecurityLevel fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(err.code == 15100005).assertTrue();
            });
        } catch (e) {
            console.error('SingleKvStoreGetSecurityLevelPromiseSucTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreGetResultSetPromiseSucTest
     * @tc.desc Test Js Api SingleKvStoreGetResultSet success
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreGetResultSetPromiseSucTest', 0, async function (done) {
        console.info('SingleKvStoreGetResultSetPromiseSucTest');
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
            await kvStore.getResultSet('batch_test_string_key').then((result) => {
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
            console.error('SingleKvStoreGetResultSetPromiseSucTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreGetResultSetPromiseInvalidArgsTest
     * @tc.desc Test Js Api SingleKvStoreGetResultSet with invalid args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreGetResultSetPromiseInvalidArgsTest', 0, async function (done) {
        console.info('SingleKvStoreGetResultSetPromiseInvalidArgsTest');
        try {
            let resultSet;
            await kvStore.getResultSet().then((result) => {
                console.info('SingleKvStoreGetResultSetPromiseInvalidArgsTest getResultSet success');
                expect(null).assertFail();
            }).catch((err) => {
                console.error('SingleKvStoreGetResultSetPromiseInvalidArgsTest getResultSet fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('SingleKvStoreGetResultSetPromiseInvalidArgsTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(e.code == 401).assertTrue();
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreGetResultSetPromiseClosedKVStoreTest
     * @tc.desc Test Js Api SingleKvStoreGetResultSet from a closed kvstore
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreGetResultSetPromiseClosedKVStoreTest', 0, async function (done) {
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
            await kvStore.getResultSet('batch_test_string_key').then((result) => {
                expect(null).assertFail();
            }).catch((err) => {
                console.error('SingleKvStoreGetResultSetPromiseClosedKVStoreTest getResultSet fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(err.code == 15100005).assertTrue();
            });
        } catch (e) {
            console.error('SingleKvStoreGetResultSetPromiseClosedKVStoreTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreGetResultSetPromiseQueryTest
     * @tc.desc Test Js Api SingleKvStoreGetResultSet with query
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreGetResultSetPromiseQueryTest', 0, async function (done) {
        console.info('SingleKvStoreGetResultSetPromiseQueryTest');
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
                console.info('SingleKvStoreGetResultSetPromiseQueryTest putBatch success');
                expect(err == undefined).assertTrue();
            }).catch((err) => {
                console.error('SingleKvStorePutBatchPromiseStringTest putBatch fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
            var query = new factory.Query();
            query.prefixKey("batch_test");
            await kvStore.getResultSet(query).then((result) => {
                console.info('SingleKvStoreGetResultSetPromiseQueryTest getResultSet success');
                resultSet = result;
                expect(resultSet.getCount() == 10).assertTrue();
            }).catch((err) => {
                console.error('SingleKvStoreGetResultSetPromiseQueryTest getResultSet fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
            await kvStore.closeResultSet(resultSet).then((err) => {
                console.info('SingleKvStoreGetResultSetPromiseQueryTest closeResultSet success');
                expect(err == undefined).assertTrue();
            }).catch((err) => {
                console.error('SingleKvStoreGetResultSetPromiseQueryTest closeResultSet fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('SingleKvStoreGetResultSetPromiseQueryTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreGetResultSetQueryPromiseClosedKVStoreTest
     * @tc.desc Test Js Api SingleKvStoreGetResultSet query from a closed kvstore
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreGetResultSetQueryPromiseClosedKVStoreTest', 0, async function (done) {
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
            await kvStore.getResultSet(query).then((result) => {
                expect(null).assertFail();
            }).catch((err) => {
                expect(err.code == 15100005).assertTrue();
            });
        } catch (e) {
            console.error('SingleKvStoreGetResultSetQueryPromiseClosedKVStoreTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreGetResultSetPredicatesPromiseSucTest
     * @tc.desc Test Js Api SingleKvStore.GetResultSet() with predicates success
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreGetResultSetPredicatesPromiseSucTest', 0, async function (done) {
        console.log('SingleKvStoreGetResultSetPredicatesPromiseSucTest');
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
            await kvStore.getResultSet(predicates).then((result) => {
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
     * @tc.name SingleKvStoreGetResultSetPredicatesPromiseClosedKVStoreTest
     * @tc.desc Test Js Api SingleKvStoreGetResultSet predicates from a closed kvstore
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreGetResultSetPredicatesPromiseClosedKVStoreTest', 0, async function (done) {
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
            await kvStore.getResultSet(predicates).then((result) => {
                expect(null).assertFail();
            }).catch((err) => {
                expect(err.code == 15100005).assertTrue();
            });
        } catch (e) {
            console.error('SingleKvStoreGetResultSetPredicatesPromiseClosedKVStoreTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreCloseResultSetPromiseSucTest
     * @tc.desc Test Js Api SingleKvStoreCloseResultSet success
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreCloseResultSetPromiseSucTest', 0, async function (done) {
        console.info('SingleKvStoreCloseResultSetPromiseSucTest');
        try {
            console.info('SingleKvStoreCloseResultSetPromiseSucTest success');
            let resultSet = null;
            await kvStore.getResultSet('batch_test_string_key').then((result) => {
                console.info('SingleKvStoreCloseResultSetPromiseSucTest getResultSet success');
                resultSet = result;
            }).catch((err) => {
                console.error('SingleKvStoreCloseResultSetPromiseSucTest getResultSet fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
            await kvStore.closeResultSet(resultSet).then((err) => {
                console.info('SingleKvStoreCloseResultSetPromiseSucTest closeResultSet success');
                expect(err == undefined).assertTrue();
            }).catch((err) => {
                console.error('SingleKvStoreCloseResultSetPromiseSucTest closeResultSet fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('SingleKvStoreCloseResultSetPromiseSucTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreCloseResultSetPromiseInvalidArgsTest
     * @tc.desc Test Js Api SingleKvStoreCloseResultSet with invalid args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreCloseResultSetPromiseInvalidArgsTest', 0, async function (done) {
        try {
            await kvStore.closeResultSet().then(() => {
                expect(null).assertFail();
            }).catch((err) => {
                console.error('SingleKvStoreCloseResultSetPromiseInvalidArgsTest closeResultSet fail ' + `, error code is ${err.code}, message is ${err.message}`);
            });
        } catch (e) {
            console.error('SingleKvStoreCloseResultSetPromiseInvalidArgsTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(e.code == 401).assertTrue();
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreGetResultSizePromiseQueryTest
     * @tc.desc Test Js Api SingleKvStoreGetResultSize with query
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreGetResultSizePromiseQueryTest', 0, async function (done) {
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
                console.info('SingleKvStoreGetResultSizePromiseQueryTest putBatch success');
                expect(err == undefined).assertTrue();
            }).catch((err) => {
                console.error('SingleKvStorePutBatchPromiseStringTest putBatch fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
            var query = new factory.Query();
            query.prefixKey("batch_test");
            await kvStore.getResultSize(query).then((resultSize) => {
                console.info('SingleKvStoreGetResultSizePromiseQueryTest getResultSet success');
                expect(resultSize == 10).assertTrue();
            }).catch((err) => {
                console.error('SingleKvStoreGetResultSizePromiseQueryTest getResultSet fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('SingleKvStoreGetResultSizePromiseQueryTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreGetResultSizePromiseInvalidArgsTest
     * @tc.desc Test Js Api SingleKvStoreGetResultSize with invalid args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreGetResultSizePromiseInvalidArgsTest', 0, async function (done) {
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
            console.error('SingleKvStoreGetResultSizePromiseInvalidArgsTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(e.code == 401).assertTrue();
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreGetResultSizePromiseClosedKVStoreTest
     * @tc.desc Test Js Api SingleKvStoreGetResultSize from a closed kvstore
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreGetResultSizePromiseClosedKVStoreTest', 0, async function (done) {
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
            await kvStore.getResultSize(query).then(() => {
                console.info('SingleKvStoreGetResultSizePromiseClosedKVStoreTest getResultSet success');
                expect(null).assertFail();
            }).catch((err) => {
                console.error('SingleKvStoreGetResultSizePromiseClosedKVStoreTest getResultSet fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(err.code == 15100005).assertTrue();
            });
        } catch (e) {
            console.error('SingleKvStoreGetResultSizePromiseQueryTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreChangeNotificationPromiseTest
     * @tc.desc Test Js Api SingleKvStoreChangeNotification
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreChangeNotificationPromiseTest', 0, async function (done) {
        var getInsertEntries = new Promise((resolve, reject) => {
            kvStore.on('dataChange', 0, function (ChangeNotification) {
                resolve(ChangeNotification.insertEntries);
            });
            kvStore.put("getInsertEntries", "byPut").then(() => {
                console.info('SingleKvStoreChangeNotificationPromiseTest put success');
            });
        });
        await getInsertEntries.then(function (insertEntries) {
            console.info('SingleKvStoreChangeNotificationPromiseTest getInsertEntries' + JSON.stringify(insertEntries));
            expect(insertEntries != null).assertTrue();
        }).catch((error) => {
            console.error('SingleKvStoreChangeNotificationPromiseTest can NOT getInsertEntries, fail:' + `, error code is ${error.code}, message is ${error.message}`);
            expect(null).assertFail();
        });
        done();
    })

    /**
     * @tc.name SingleKvStoreChangeNotificationPromisePutTest
     * @tc.desc Test Js Api SingleKvStoreChangeNotification put
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreChangeNotificationPromisePutTest', 0, async function (done) {
        await kvStore.put('getUpdateEntries', 'byPut').then(() => {
            console.info('SingleKvStoreChangeNotificationPromisePutTest put success');
        });
        var getUpdateEntries = new Promise((resolve, reject) => {
            kvStore.on('dataChange', 0, function (ChangeNotification) {
                resolve(ChangeNotification.updateEntries);
            });
            kvStore.put("getUpdateEntries", "byPut").then(() => {
                console.info('SingleKvStoreChangeNotificationPromisePutTest update success');
            });
        });
        await getUpdateEntries.then(function (updateEntries) {
            console.info('SingleKvStoreChangeNotificationPromisePutTest getUpdateEntries' + JSON.stringify(updateEntries));
            expect(updateEntries != null).assertTrue();
        }).catch((error) => {
            console.error('SingleKvStoreChangeNotificationPromisePutTest can NOT getUpdateEntries, fail:' + `, error code is ${error.code}, message is ${error.message}`);
            expect(null).assertFail();
        });
        done();
    })

    /**
     * @tc.name SingleKvStoreChangeNotificationPromiseDeleteTest
     * @tc.desc Test Js Api SingleKvStoreChangeNotification delete
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreChangeNotificationPromiseDeleteTest', 0, async function (done) {
        await kvStore.put('deleteEntries', 'byPut').then(() => {
            console.info('SingleKvStoreChangeNotificationPromiseDeleteTest put success');
        });
        var getdeleteEntries = new Promise((resolve, reject) => {
            kvStore.on('dataChange', 0, function (ChangeNotification) {
                resolve(ChangeNotification.deleteEntries);
            });
            kvStore.delete("deleteEntries").then(() => {
                console.info('SingleKvStoreChangeNotificationPromiseDeleteTest delete success');
            });
        });
        await getdeleteEntries.then(function (deleteEntries) {
            console.info('SingleKvStoreChangeNotificationPromiseDeleteTest deleteEntries' + JSON.stringify(getdeleteEntries));
            expect(deleteEntries != null).assertTrue();
        }).catch((error) => {
            console.error('SingleKvStoreChangeNotificationPromiseDeleteTest can NOT getdeleteEntries, fail:' + `, error code is ${error.code}, message is ${error.message}`);
            expect(null).assertFail();
        });
        done();
    })
})
