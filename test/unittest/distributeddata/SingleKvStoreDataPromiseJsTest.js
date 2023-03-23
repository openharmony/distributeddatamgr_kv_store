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

describe('singleKvStorePromiseTest', function () {
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
        console.info('beforeAll config:'+ JSON.stringify(config));
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
     * @tc.name SingleKvStorePutStringPromiseTest001
     * @tc.desc Test Js Api SingleKvStore.Put(String) testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStorePutStringPromiseTest001', 0, async function (done) {
        console.info('SingleKvStorePutStringPromiseTest001');
        try {
            await kvStore.put(KEY_TEST_STRING_ELEMENT, null).then((data) => {
                console.info('SingleKvStorePutStringPromiseTest001 put success');
                expect(null).assertFail();
            }).catch((error) => {
                console.error('SingleKvStorePutStringPromiseTest001 put error' + `, error code is ${error.code}, message is ${error.message}`);
            });
        } catch (e) {
            console.error('SingleKvStorePutStringPromiseTest001 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStorePutStringPromiseTest002
     * @tc.desc Test Js Api SingleKvStore.Put(String) testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStorePutStringPromiseTest002', 0, async function (done) {
        console.info('SingleKvStorePutStringPromiseTest002');
        try {
            await kvStore.put(KEY_TEST_STRING_ELEMENT, '').then((data) => {
                console.info('SingleKvStorePutStringPromiseTest002 put success');
                expect(data == undefined).assertTrue();
            }).catch((error) => {
                console.error('SingleKvStorePutStringPromiseTest002 put error' + `, error code is ${error.code}, message is ${error.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('SingleKvStorePutStringPromiseTest002 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStorePutStringPromiseTest003
     * @tc.desc Test Js Api SingleKvStore.Put(String) testcase 003
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStorePutStringPromiseTest003', 0, async function (done) {
        console.info('SingleKvStorePutStringPromiseTest003');
        try {
            await kvStore.put(KEY_TEST_STRING_ELEMENT, VALUE_TEST_STRING_ELEMENT).then((data) => {
                console.info('SingleKvStorePutStringPromiseTest003 put success');
                expect(data == undefined).assertTrue();
            }).catch((error) => {
                console.error('SingleKvStorePutStringPromiseTest003 put error' + `, error code is ${error.code}, message is ${error.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('SingleKvStorePutStringPromiseTest003 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStorePutStringPromiseTest004
     * @tc.desc Test Js Api SingleKvStore.Put(String) testcase 004
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStorePutStringPromiseTest004', 0, async function (done) {
        console.info('SingleKvStorePutStringPromiseTest004');
        try {
            var str = '';
            for (var i = 0 ; i < 4095; i++) {
                str += 'x';
            }
            await kvStore.put(KEY_TEST_STRING_ELEMENT, str).then(async (data) => {
                console.info('SingleKvStorePutStringPromiseTest004 put success');
                expect(data == undefined).assertTrue();
                await kvStore.get(KEY_TEST_STRING_ELEMENT).then((data) => {
                    console.info('SingleKvStorePutStringPromiseTest004 get success data ' + data);
                    expect(str == data).assertTrue();
                }).catch((err) => {
                    console.error('SingleKvStorePutStringPromiseTest004 get fail ' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(null).assertFail();
                });
            }).catch((error) => {
                console.error('SingleKvStorePutStringPromiseTest004 put error' + `, error code is ${error.code}, message is ${error.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('SingleKvStorePutStringPromiseTest004 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreGetStringPromiseTest001
     * @tc.desc Test Js Api SingleKvStoreGetString testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreGetStringPromiseTest001', 0, async function (done) {
        console.info('SingleKvStoreGetStringPromiseTest001');
        try {
            await kvStore.put(KEY_TEST_STRING_ELEMENT, VALUE_TEST_STRING_ELEMENT).then(async (data) => {
                console.info('SingleKvStoreGetStringPromiseTest001 put success');
                expect(data == undefined).assertTrue();
                await kvStore.get(KEY_TEST_STRING_ELEMENT).then((data) => {
                    console.info('SingleKvStoreGetStringPromiseTest001 get success');
                    expect(VALUE_TEST_STRING_ELEMENT == data).assertTrue();
                }).catch((err) => {
                    console.error('SingleKvStoreGetStringPromiseTest001 get fail ' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(null).assertFail();
                });
            }).catch((error) => {
                console.error('SingleKvStoreGetStringPromiseTest001 put error' + `, error code is ${error.code}, message is ${error.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('SingleKvStoreGetStringPromiseTest001 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreGetStringPromiseTest002
     * @tc.desc Test Js Api SingleKvStoreGetString testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreGetStringPromiseTest002', 0, async function (done) {
        console.info('SingleKvStoreGetStringPromiseTest002');
        try {
            await kvStore.get(KEY_TEST_STRING_ELEMENT).then((data) => {
                console.info('SingleKvStoreGetStringPromiseTest002 get success');
                expect(null).assertFail();
            }).catch((err) => {
                console.error('SingleKvStoreGetStringPromiseTest002 get fail ' + `, error code is ${err.code}, message is ${err.message}`);
            });
        } catch (e) {
            console.error('SingleKvStoreGetStringPromiseTest002 get e ' + `, error code is ${e.code}, message is ${e.message}`);
        }
        done();
    })

    /**
     * @tc.name SingleKvStorePutIntPromiseTest001
     * @tc.desc Test Js Api SingleKvStore.Put(Int) testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStorePutIntPromiseTest001', 0, async function (done) {
        console.info('SingleKvStorePutIntPromiseTest001');
        try {
            await kvStore.put(KEY_TEST_INT_ELEMENT, VALUE_TEST_INT_ELEMENT).then((data) => {
                console.info('SingleKvStorePutIntPromiseTest001 put success');
                expect(data == undefined).assertTrue();
            }).catch((err) => {
                console.error('SingleKvStorePutIntPromiseTest001 put fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('SingleKvStorePutIntPromiseTest001 put e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStorePutIntPromiseTest002
     * @tc.desc Test Js Api SingleKvStore.Put(Int) testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStorePutIntPromiseTest002', 0, async function (done) {
        console.info('SingleKvStorePutIntPromiseTest002');
        try {
            var intValue = 987654321;
            await kvStore.put(KEY_TEST_INT_ELEMENT, intValue).then(async (data) => {
                console.info('SingleKvStorePutIntPromiseTest002 put success');
                expect(data == undefined).assertTrue();
                await kvStore.get(KEY_TEST_INT_ELEMENT).then((data) => {
                    console.info('SingleKvStorePutIntPromiseTest002 get success');
                    expect(intValue == data).assertTrue();
                }).catch((err) => {
                    console.error('SingleKvStorePutIntPromiseTest002 get fail ' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(null).assertFail();
                });
            }).catch((err) => {
                console.error('SingleKvStorePutIntPromiseTest002 put fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('SingleKvStorePutIntPromiseTest002 put e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStorePutIntPromiseTest003
     * @tc.desc Test Js Api SingleKvStore.Put(Int) testcase 003
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStorePutIntPromiseTest003', 0, async function (done) {
        console.info('SingleKvStorePutIntPromiseTest003');
        try {
            var intValue = Number.MAX_VALUE;
            await kvStore.put(KEY_TEST_INT_ELEMENT, intValue).then(async (data) => {
                console.info('SingleKvStorePutIntPromiseTest003 put success');
                expect(data == undefined).assertTrue();
                await kvStore.get(KEY_TEST_INT_ELEMENT).then((data) => {
                    console.info('SingleKvStorePutIntPromiseTest003 get success');
                    expect(intValue == data).assertTrue();
                }).catch((err) => {
                    console.error('SingleKvStorePutIntPromiseTest003 get fail ' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(null).assertFail();
                });
            }).catch((err) => {
                console.error('SingleKvStorePutIntPromiseTest003 put fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('SingleKvStorePutIntPromiseTest003 put e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStorePutIntPromiseTest004
     * @tc.desc Test Js Api SingleKvStore.Put(Int) testcase 004
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStorePutIntPromiseTest004', 0, async function (done) {
        console.info('SingleKvStorePutIntPromiseTest004');
        try {
            var intValue = Number.MIN_VALUE;
            await kvStore.put(KEY_TEST_INT_ELEMENT, intValue).then(async (data) => {
                console.info('SingleKvStorePutIntPromiseTest004 put success');
                expect(data == undefined).assertTrue();
                await kvStore.get(KEY_TEST_INT_ELEMENT).then((data) => {
                    console.info('SingleKvStorePutIntPromiseTest004 get success');
                    expect(intValue == data).assertTrue();
                }).catch((err) => {
                    console.error('SingleKvStorePutIntPromiseTest004 get fail ' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(null).assertFail();
                });
            }).catch((err) => {
                console.error('SingleKvStorePutIntPromiseTest004 put fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('SingleKvStorePutIntPromiseTest004 put e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreGetIntPromiseTest001
     * @tc.desc Test Js Api SingleKvStoreGetInt testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreGetIntPromiseTest001', 0, async function (done) {
        console.info('SingleKvStoreGetIntPromiseTest001');
        try {
            await kvStore.put(KEY_TEST_INT_ELEMENT, VALUE_TEST_INT_ELEMENT).then(async (data) => {
                console.info('SingleKvStoreGetIntPromiseTest001 put success');
                expect(data == undefined).assertTrue();
                await kvStore.get(KEY_TEST_INT_ELEMENT).then((data) => {
                    console.info('SingleKvStoreGetIntPromiseTest001 get success');
                    expect(VALUE_TEST_INT_ELEMENT == data).assertTrue();
                }).catch((err) => {
                    console.error('SingleKvStoreGetIntPromiseTest001 get fail ' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(null).assertFail();
                });
            }).catch((err) => {
                console.error('SingleKvStoreGetIntPromiseTest001 put fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('SingleKvStoreGetIntPromiseTest001 put e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStorePutBoolPromiseTest001
     * @tc.desc Test Js Api SingleKvStore.Put(Bool) testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStorePutBoolPromiseTest001', 0, async function (done) {
        console.info('SingleKvStorePutBoolPromiseTest001');
        try {
            await kvStore.put(KEY_TEST_BOOLEAN_ELEMENT, VALUE_TEST_BOOLEAN_ELEMENT).then((data) => {
                console.info('SingleKvStorePutBoolPromiseTest001 put success');
                expect(data == undefined).assertTrue();
            }).catch((err) => {
                console.error('SingleKvStorePutBoolPromiseTest001 put fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('SingleKvStorePutBoolPromiseTest001 put e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreGetBoolPromiseTest001
     * @tc.desc Test Js Api SingleKvStoreGetBool testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreGetBoolPromiseTest001', 0, async function (done) {
        console.info('SingleKvStoreGetBoolPromiseTest001');
        try {
            var boolValue = false;
            await kvStore.put(KEY_TEST_BOOLEAN_ELEMENT, boolValue).then(async (data) => {
                console.info('SingleKvStoreGetBoolPromiseTest001 put success');
                expect(data == undefined).assertTrue();
                await kvStore.get(KEY_TEST_BOOLEAN_ELEMENT).then((data) => {
                    console.info('SingleKvStoreGetBoolPromiseTest001 get success');
                    expect(boolValue == data).assertTrue();
                }).catch((err) => {
                    console.error('SingleKvStoreGetBoolPromiseTest001 get fail ' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(null).assertFail();
                });
            }).catch((err) => {
                console.error('SingleKvStoreGetBoolPromiseTest001 put fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('SingleKvStoreGetBoolPromiseTest001 put e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStorePutFloatPromiseTest001
     * @tc.desc Test Js Api SingleKvStore.Put(Float) testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStorePutFloatPromiseTest001', 0, async function (done) {
        console.info('SingleKvStorePutFloatPromiseTest001');
        try {
            await kvStore.put(KEY_TEST_FLOAT_ELEMENT, VALUE_TEST_FLOAT_ELEMENT).then((data) => {
                console.info('SingleKvStorePutFloatPromiseTest001 put success');
                expect(data == undefined).assertTrue();
            }).catch((err) => {
                console.error('SingleKvStorePutFloatPromiseTest001 put fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('SingleKvStorePutFloatPromiseTest001 put e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreGetFloatPromiseTest001
     * @tc.desc Test Js Api SingleKvStoreGetFloat testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreGetFloatPromiseTest001', 0, async function (done) {
        console.info('SingleKvStoreGetFloatPromiseTest001');
        try {
            var floatValue = 123456.654321;
            await kvStore.put(KEY_TEST_FLOAT_ELEMENT, floatValue).then(async (data) => {
                console.info('SingleKvStoreGetFloatPromiseTest001 put success');
                expect(data == undefined).assertTrue();
                await kvStore.get(KEY_TEST_FLOAT_ELEMENT).then((data) => {
                    console.info('SingleKvStoreGetFloatPromiseTest001 get success');
                    expect(floatValue == data).assertTrue();
                }).catch((err) => {
                    console.error('SingleKvStoreGetFloatPromiseTest001 get fail ' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(null).assertFail();
                });
            }).catch((err) => {
                console.error('SingleKvStoreGetFloatPromiseTest001 put fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('SingleKvStoreGetFloatPromiseTest001 put e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreGetFloatPromiseTest002
     * @tc.desc Test Js Api SingleKvStoreGetFloat testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreGetFloatPromiseTest002', 0, async function (done) {
        console.info('SingleKvStoreGetFloatPromiseTest002');
        try {
            var floatValue = 123456.0;
            await kvStore.put(KEY_TEST_FLOAT_ELEMENT, floatValue).then(async (data) => {
                console.info('SingleKvStoreGetFloatPromiseTest002 put success');
                expect(data == undefined).assertTrue();
                await kvStore.get(KEY_TEST_FLOAT_ELEMENT).then((data) => {
                    console.info('SingleKvStoreGetFloatPromiseTest002 get success');
                    expect(floatValue == data).assertTrue();
                }).catch((err) => {
                    console.error('SingleKvStoreGetFloatPromiseTest002 get fail ' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(null).assertFail();
                });
            }).catch((err) => {
                console.error('SingleKvStoreGetFloatPromiseTest002 put fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('SingleKvStoreGetFloatPromiseTest002 put e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreGetFloatPromiseTest003
     * @tc.desc Test Js Api SingleKvStoreGetFloat testcase 003
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreGetFloatPromiseTest003', 0, async function (done) {
        console.info('SingleKvStoreGetFloatPromiseTest003');
        try {
            var floatValue = 123456.00;
            await kvStore.put(KEY_TEST_FLOAT_ELEMENT, floatValue).then(async (data) => {
                console.info('SingleKvStoreGetFloatPromiseTest003 put success');
                expect(data == undefined).assertTrue();
                await kvStore.get(KEY_TEST_FLOAT_ELEMENT).then((data) => {
                    console.info('SingleKvStoreGetFloatPromiseTest003 get success');
                    expect(floatValue == data).assertTrue();
                }).catch((err) => {
                    console.error('SingleKvStoreGetFloatPromiseTest003 get fail ' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(null).assertFail();
                });
            }).catch((err) => {
                console.error('SingleKvStoreGetFloatPromiseTest003 put fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('SingleKvStoreGetFloatPromiseTest003 put e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreDeleteStringPromiseTest001
     * @tc.desc Test Js Api SingleKvStoreDeleteString testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreDeleteStringPromiseTest001', 0, async function (done) {
        console.info('SingleKvStoreDeleteStringPromiseTest001');
        try {
            var str = 'this is a test string';
            await kvStore.put(KEY_TEST_STRING_ELEMENT, str).then(async (data) => {
                console.info('SingleKvStoreDeleteStringPromiseTest001 put success');
                expect(data == undefined).assertTrue();
                await kvStore.delete(KEY_TEST_STRING_ELEMENT).then((data) => {
                    console.info('SingleKvStoreDeleteStringPromiseTest001 delete success');
                    expect(data == undefined).assertTrue();
                }).catch((err) => {
                    console.error('SingleKvStoreDeleteStringPromiseTest001 delete fail ' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(null).assertFail();
                });
            }).catch((err) => {
                console.error('SingleKvStoreDeleteStringPromiseTest001 put fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('SingleKvStoreDeleteStringPromiseTest001 put e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreDeleteStringPromiseTest002
     * @tc.desc Test Js Api SingleKvStoreDeleteString testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreDeleteStringPromiseTest002', 0, async function (done) {
        console.info('SingleKvStoreDeleteStringPromiseTest002');
        try {
            var str = '';
            for (var i = 0 ; i < 4096; i++) {
                str += 'x';
            }
            await kvStore.put(KEY_TEST_STRING_ELEMENT, str).then(async (data) => {
                console.info('SingleKvStoreDeleteStringPromiseTest002 put success');
                expect(data == undefined).assertTrue();
                await kvStore.delete(KEY_TEST_STRING_ELEMENT).then((data) => {
                    console.info('SingleKvStoreDeleteStringPromiseTest002 delete success');
                    expect(data == undefined).assertTrue();
                }).catch((err) => {
                    console.error('SingleKvStoreDeleteStringPromiseTest002 delete fail ' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(null).assertFail();
                });
            }).catch((err) => {
                console.error('SingleKvStoreDeleteStringPromiseTest002 put fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('SingleKvStoreDeleteStringPromiseTest002 put e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreDeleteStringPromiseTest003
     * @tc.desc Test Js Api SingleKvStoreDeleteString testcase 003
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreDeleteStringPromiseTest003', 0, async function (done) {
        console.info('SingleKvStoreDeleteStringPromiseTest003');
        try {
            await kvStore.delete(KEY_TEST_STRING_ELEMENT).then((data) => {
                console.info('SingleKvStoreDeleteStringPromiseTest003 delete success');
                expect(data == undefined).assertTrue();
            }).catch((err) => {
                console.error('SingleKvStoreDeleteStringPromiseTest003 delete fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('SingleKvStoreDeleteStringPromiseTest003 put e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreDeleteIntPromiseTest001
     * @tc.desc Test Js Api SingleKvStoreDeleteInt testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreDeleteIntPromiseTest001', 0, async function (done) {
        console.info('SingleKvStoreDeleteIntPromiseTest001');
        try {
            await kvStore.put(KEY_TEST_INT_ELEMENT, VALUE_TEST_INT_ELEMENT).then(async (data) => {
                console.info('SingleKvStoreDeleteIntPromiseTest001 put success');
                expect(data == undefined).assertTrue();
                await kvStore.delete(KEY_TEST_INT_ELEMENT).then((data) => {
                    console.info('SingleKvStoreDeleteIntPromiseTest001 delete success');
                    expect(data == undefined).assertTrue();
                }).catch((err) => {
                    console.error('SingleKvStoreDeleteIntPromiseTest001 delete fail ' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(null).assertFail();
                });
            }).catch((err) => {
                console.error('SingleKvStoreDeleteIntPromiseTest001 put fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('SingleKvStoreDeleteIntPromiseTest001 put e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreDeleteFloatPromiseTest001
     * @tc.desc Test Js Api SingleKvStoreDeleteFloat testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreDeleteFloatPromiseTest001', 0, async function (done) {
        console.info('SingleKvStoreDeleteFloatPromiseTest001');
        try {
            await kvStore.put(KEY_TEST_FLOAT_ELEMENT, VALUE_TEST_FLOAT_ELEMENT).then(async (data) => {
                console.info('SingleKvStoreDeleteFloatPromiseTest001 put success');
                expect(data == undefined).assertTrue();
                await kvStore.delete(KEY_TEST_FLOAT_ELEMENT).then((data) => {
                    console.info('SingleKvStoreDeleteFloatPromiseTest001 delete success');
                    expect(data == undefined).assertTrue();
                }).catch((err) => {
                    console.error('SingleKvStoreDeleteFloatPromiseTest001 delete fail ' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(null).assertFail();
                });
            }).catch((err) => {
                console.error('SingleKvStoreDeleteFloatPromiseTest001 put fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('SingleKvStoreDeleteFloatPromiseTest001 put e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreDeleteBoolPromiseTest001
     * @tc.desc Test Js Api SingleKvStoreDeleteBool testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreDeleteBoolPromiseTest001', 0, async function (done) {
        console.info('SingleKvStoreDeleteBoolPromiseTest001');
        try {
            await kvStore.put(KEY_TEST_BOOLEAN_ELEMENT, VALUE_TEST_BOOLEAN_ELEMENT).then(async (data) => {
                console.info('SingleKvStoreDeleteBoolPromiseTest001 put success');
                expect(data == undefined).assertTrue();
                await kvStore.delete(KEY_TEST_BOOLEAN_ELEMENT).then((data) => {
                    console.info('SingleKvStoreDeleteBoolPromiseTest001 delete success');
                    expect(data == undefined).assertTrue();
                }).catch((err) => {
                    console.error('SingleKvStoreDeleteBoolPromiseTest001 delete fail ' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(null).assertFail();
                });
            }).catch((err) => {
                console.error('SingleKvStoreDeleteBoolPromiseTest001 put fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('SingleKvStoreDeleteBoolPromiseTest001 put e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreOnChangePromiseTest001
     * @tc.desc Test Js Api SingleKvStoreOnChange testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreOnChangePromiseTest001', 0, async function (done) {
        try {
            kvStore.on('dataChange', 0, function (data) {
                console.info('SingleKvStoreOnChangePromiseTest001 ' + JSON.stringify(data));
                expect(data != null).assertTrue();
            });
            await kvStore.put(KEY_TEST_FLOAT_ELEMENT, VALUE_TEST_FLOAT_ELEMENT).then((data) => {
                console.info('SingleKvStoreOnChangePromiseTest001 put success');
                expect(data == undefined).assertTrue();
            }).catch((error) => {
                console.error('SingleKvStoreOnChangePromiseTest001 put fail ' + `, error code is ${error.code}, message is ${error.message}`);
                expect(null).assertFail();
            });
        }catch(e) {
            console.error('SingleKvStoreOnChangePromiseTest001 put e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreOnChangePromiseTest002
     * @tc.desc Test Js Api SingleKvStoreOnChange testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreOnChangePromiseTest002', 0, async function (done) {
        try {
            kvStore.on('dataChange', 1, function (data) {
                console.info('SingleKvStoreOnChangePromiseTest002 on ' + JSON.stringify(data));
                expect(data != null).assertTrue();
            });
            await kvStore.put(KEY_TEST_FLOAT_ELEMENT, VALUE_TEST_FLOAT_ELEMENT).then((data) => {
                console.info('SingleKvStoreOnChangePromiseTest002 put success');
                expect(data == undefined).assertTrue();
            }).catch((error) => {
                console.error('SingleKvStoreOnChangePromiseTest002 put fail ' + `, error code is ${error.code}, message is ${error.message}`);
                expect(null).assertFail();
            });
        }catch(e) {
            console.error('SingleKvStoreOnChangePromiseTest002 put e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreOnChangePromiseTest003
     * @tc.desc Test Js Api SingleKvStoreOnChange testcase 003
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreOnChangePromiseTest003', 0, async function (done) {
        try {
            kvStore.on('dataChange', 2, function (data) {
                console.info('SingleKvStoreOnChangePromiseTest003 ' + JSON.stringify(data));
                expect(data != null).assertTrue();
            });
            await kvStore.put(KEY_TEST_FLOAT_ELEMENT, VALUE_TEST_FLOAT_ELEMENT).then((data) => {
                console.info('SingleKvStoreOnChangePromiseTest003 put success');
                expect(data == undefined).assertTrue();
            }).catch((error) => {
                console.error('SingleKvStoreOnChangePromiseTest003 put fail ' + `, error code is ${error.code}, message is ${error.message}`);
                expect(null).assertFail();
            });
        }catch(e) {
            console.error('SingleKvStoreOnChangePromiseTest003 put e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreOnSyncCompletePromiseTest001
     * @tc.desc Test Js Api SingleKvStoreOnSyncComplete testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreOnSyncCompletePromiseTest001', 0, async function (done) {
        try {
            kvStore.on('syncComplete', function (data) {
                console.info('SingleKvStoreOnSyncCompletePromiseTest001 0' + data)
                expect(data != null).assertTrue();
            });
            await kvStore.put(KEY_TEST_SYNC_ELEMENT, VALUE_TEST_SYNC_ELEMENT).then((data) => {
                console.info('SingleKvStoreOnSyncCompletePromiseTest001 put success');
                expect(data == undefined).assertTrue();
            }).catch((error) => {
                console.error('SingleKvStoreOnSyncCompletePromiseTest001 put fail ' + `, error code is ${error.code}, message is ${error.message}`);
                expect(null).assertFail();
            });
            try {
                var mode = factory.SyncMode.PULL_ONLY;
                console.info('kvStore.sync to ' + JSON.stringify(syncDeviceIds));
                kvStore.sync(syncDeviceIds, mode, 10);
            } catch (e) {
                console.error('SingleKvStoreOnSyncCompletePromiseTest001 sync no peer device :e:' + `, error code is ${e.code}, message is ${e.message}`);
            }
        }catch(e) {
            console.error('SingleKvStoreOnSyncCompletePromiseTest001 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreOnSyncCompletePromiseTest002
     * @tc.desc Test Js Api SingleKvStoreOnSyncComplete testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreOnSyncCompletePromiseTest002', 0, async function (done) {
        try {
            kvStore.on('syncComplete', function (data) {
                console.info('SingleKvStoreOnSyncCompletePromiseTest002 0' + data)
                expect(data != null).assertTrue();
            });
            await kvStore.put(KEY_TEST_SYNC_ELEMENT, VALUE_TEST_SYNC_ELEMENT).then((data) => {
                console.info('SingleKvStoreOnSyncCompletePromiseTest002 put success');
                expect(data == undefined).assertTrue();
            }).catch((error) => {
                console.error('SingleKvStoreOnSyncCompletePromiseTest002 put fail ' + `, error code is ${error.code}, message is ${error.message}`);
                expect(null).assertFail();
            });
            try {
                var mode = factory.SyncMode.PUSH_ONLY;
                console.info('kvStore.sync to ' + JSON.stringify(syncDeviceIds));
                kvStore.sync(syncDeviceIds, mode, 10);
            } catch (e) {
                console.error('SingleKvStoreOnSyncCompletePromiseTest002 sync no peer device :e:' + `, error code is ${e.code}, message is ${e.message}`);
            }
        }catch(e) {
            console.error('SingleKvStoreOnSyncCompletePromiseTest002 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreOnSyncCompletePromiseTest003
     * @tc.desc Test Js Api SingleKvStoreOnSyncComplete testcase 003
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreOnSyncCompletePromiseTest003', 0, async function (done) {
        try {
            kvStore.on('syncComplete', function (data) {
                console.info('SingleKvStoreOnSyncCompletePromiseTest003 0' + data)
                expect(data != null).assertTrue();
            });
            await kvStore.put(KEY_TEST_SYNC_ELEMENT, VALUE_TEST_SYNC_ELEMENT).then((data) => {
                console.info('SingleKvStoreOnSyncCompletePromiseTest003 put success');
                expect(data == undefined).assertTrue();
            }).catch((error) => {
                console.error('SingleKvStoreOnSyncCompletePromiseTest003 put fail ' + `, error code is ${error.code}, message is ${error.message}`);
                expect(null).assertFail();
            });
            try {
                var mode = factory.SyncMode.PUSH_PULL;
                console.info('kvStore.sync to ' + JSON.stringify(syncDeviceIds));
                kvStore.sync(syncDeviceIds, mode, 10);
            } catch (e) {
                console.error('SingleKvStoreOnSyncCompletePromiseTest003 sync no peer device :e:' + `, error code is ${e.code}, message is ${e.message}`);
            }
        }catch(e) {
            console.error('SingleKvStoreOnSyncCompletePromiseTest003 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreOffChangePromiseTest001
     * @tc.desc Test Js Api SingleKvStoreOffChange testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreOffChangePromiseTest001', 0, async function (done) {
        console.info('SingleKvStoreOffChangePromiseTest001');
        try {
            var func = function (data) {
                console.info('SingleKvStoreOffChangePromiseTest001 ' + JSON.stringify(data));
            };
            kvStore.on('dataChange', 0, func);
            kvStore.off('dataChange', func);
        }catch(e) {
            console.error('SingleKvStoreOffChangePromiseTest001 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreOffChangePromiseTest002
     * @tc.desc Test Js Api SingleKvStoreOffChange testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreOffChangePromiseTest002', 0, async function (done) {
        console.info('SingleKvStoreOffChangePromiseTest002');
        let ret = false;
        try {
            var func = function (data) {
                console.info('SingleKvStoreOffChangePromiseTest002 ' + JSON.stringify(data));
            };
            var func1 = function (data) {
                console.info('SingleKvStoreOffChangePromiseTest002 ' + JSON.stringify(data));
            };
            kvStore.on('dataChange', 0, func);
            kvStore.on('dataChange', 0, func1);
            kvStore.off('dataChange', func);
        }catch(e) {
            console.error('SingleKvStoreOffChangePromiseTest002 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreOffChangePromiseTest003
     * @tc.desc Test Js Api SingleKvStoreOffChange testcase 003
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreOffChangePromiseTest003', 0, async function (done) {
        console.info('SingleKvStoreOffChangePromiseTest003');
        try {
            var func = function (data) {
                console.info('SingleKvStoreOffChangePromiseTest003 0' + data)
            };
            var func1 = function (data) {
                console.info('SingleKvStoreOffChangePromiseTest003 0' + data)
            };
            kvStore.on('dataChange', 0, func);
            kvStore.on('dataChange', 0, func1);
            kvStore.off('dataChange', func);
            kvStore.off('dataChange', func1);
        }catch(e) {
            console.error('SingleKvStoreOffChangePromiseTest003 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreOffChangePromiseTest004
     * @tc.desc Test Js Api SingleKvStoreOffChange testcase 004
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreOffChangePromiseTest004', 0, async function (done) {
        console.info('SingleKvStoreOffChangePromiseTest004');
        try {
            var func = function (data) {
                console.info('SingleKvStoreOffChangePromiseTest004 ' + JSON.stringify(data));
            };
            kvStore.on('dataChange', 0, func);
            kvStore.off('dataChange');
        }catch(e) {
            console.error('SingleKvStoreOffChangePromiseTest004 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreOffSyncCompletePromiseTest001
     * @tc.desc Test Js Api SingleKvStoreOffSyncComplete testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreOffSyncCompletePromiseTest001', 0, async function (done) {
        console.info('SingleKvStoreOffSyncCompletePromiseTest001');
        try {
            var func = function (data) {
                console.info('SingleKvStoreOffSyncCompletePromiseTest001 0' + data)
            };
            kvStore.on('syncComplete', func);
            kvStore.off('syncComplete', func);
        }catch(e) {
            console.error('SingleKvStoreOffSyncCompletePromiseTest001 put e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreOffSyncCompletePromiseTest002
     * @tc.desc Test Js Api SingleKvStoreOffSyncComplete testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreOffSyncCompletePromiseTest002', 0, async function (done) {
        console.info('SingleKvStoreOffSyncCompletePromiseTest002');
        try {
            var func = function (data) {
                console.info('SingleKvStoreOffSyncCompletePromiseTest002 0' + data)
            };
            var func1 = function (data) {
                console.info('SingleKvStoreOffSyncCompletePromiseTest002 0' + data)
            };
            kvStore.on('syncComplete', func);
            kvStore.on('syncComplete', func1);
            kvStore.off('syncComplete', func);
        }catch(e) {
            console.error('SingleKvStoreOffSyncCompletePromiseTest002 put e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreOffSyncCompletePromiseTest003
     * @tc.desc Test Js Api SingleKvStoreOffSyncComplete testcase 003
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreOffSyncCompletePromiseTest003', 0, async function (done) {
        console.info('SingleKvStoreOffSyncCompletePromiseTest003');
        try {
            var func = function (data) {
                console.info('SingleKvStoreOffSyncCompletePromiseTest003 0' + data)
            };
            var func1 = function (data) {
                console.info('SingleKvStoreOffSyncCompletePromiseTest003 0' + data)
            };
            kvStore.on('syncComplete', func);
            kvStore.on('syncComplete', func1);
            kvStore.off('syncComplete', func);
            kvStore.off('syncComplete', func1);
        }catch(e) {
            console.error('SingleKvStoreOffSyncCompletePromiseTest003 put e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreOffSyncCompletePromiseTest004
     * @tc.desc Test Js Api SingleKvStoreOffSyncComplete testcase 004
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreOffSyncCompletePromiseTest004', 0, async function (done) {
        console.info('SingleKvStoreOffSyncCompletePromiseTest004');
        try {
            var func = function (data) {
                console.info('SingleKvStoreOffSyncCompletePromiseTest004 0' + data)
            };
            kvStore.on('syncComplete', func);
            kvStore.off('syncComplete');
        }catch(e) {
            console.error('SingleKvStoreOffSyncCompletePromiseTest004 put e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreSetSyncRangePromiseTest001
     * @tc.desc Test Js Api SingleKvStoreSetSyncRange testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreSetSyncRangePromiseTest001', 0, async function (done) {
        console.info('SingleKvStoreSetSyncRangePromiseTest001');
        try {
            var localLabels = ['A', 'B'];
            var remoteSupportLabels = ['C', 'D'];
            await kvStore.setSyncRange(localLabels, remoteSupportLabels).then((err) => {
                console.info('SingleKvStoreSetSyncRangePromiseTest001 setSyncRange success');
                expect(err == undefined).assertTrue();
            }).catch((err) => {
                console.error('SingleKvStoreDeleteStringPromiseTest003 delete fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        }catch(e) {
            console.error('SingleKvStoreSetSyncRangePromiseTest001 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreSetSyncRangePromiseTest002
     * @tc.desc Test Js Api SingleKvStoreSetSyncRange testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreSetSyncRangePromiseTest002', 0, async function (done) {
        console.info('SingleKvStoreSetSyncRangePromiseTest002');
        try {
            var localLabels = ['A', 'B'];
            var remoteSupportLabels = ['B', 'C'];
            await kvStore.setSyncRange(localLabels, remoteSupportLabels).then((err) => {
                console.info('SingleKvStoreSetSyncRangePromiseTest002 setSyncRange success');
                expect(err == undefined).assertTrue();
            }).catch((err) => {
                console.error('SingleKvStoreSetSyncRangePromiseTest002 delete fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        }catch(e) {
            console.error('SingleKvStoreSetSyncRangePromiseTest002 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreSetSyncRangePromiseTest003
     * @tc.desc Test Js Api SingleKvStoreSetSyncRange testcase 003
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreSetSyncRangePromiseTest003', 0, async function (done) {
        console.info('SingleKvStoreSetSyncRangePromiseTest003');
        try {
            var localLabels = ['A', 'B'];
            var remoteSupportLabels = ['A', 'B'];
            await kvStore.setSyncRange(localLabels, remoteSupportLabels).then((err) => {
                console.info('SingleKvStoreSetSyncRangePromiseTest003 setSyncRange success');
                expect(err == undefined).assertTrue();
            }).catch((err) => {
                console.error('SingleKvStoreSetSyncRangePromiseTest003 delete fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        }catch(e) {
            console.error('SingleKvStoreSetSyncRangePromiseTest003 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStorePutBatchPromiseTest001
     * @tc.desc Test Js Api SingleKvStore.PutBatch() testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStorePutBatchPromiseTest001', 0, async function (done) {
        console.info('SingleKvStorePutBatchPromiseTest001');
        try {
            let entries = putBatchString(10, 'batch_test_string_key');
            console.info('SingleKvStorePutBatchPromiseTest001 entries: ' + JSON.stringify(entries));
            await kvStore.putBatch(entries).then(async (err) => {
                console.info('SingleKvStorePutBatchPromiseTest001 putBatch success');
                expect(err == undefined).assertTrue();
                await kvStore.getEntries('batch_test_string_key').then((entrys) => {
                    console.info('SingleKvStorePutBatchPromiseTest001 getEntries success');
                    console.info('SingleKvStorePutBatchPromiseTest001 ' + JSON.stringify(entries));
                    expect(entrys.length == 10).assertTrue();
                    expect(entrys[0].value.value == 'batch_test_string_value').assertTrue();
                }).catch((err) => {
                    console.error('SingleKvStorePutBatchPromiseTest001 getEntries fail ' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(null).assertFail();
                });
            }).catch((err) => {
                console.error('SingleKvStorePutBatchPromiseTest001 putBatch fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        }catch(e) {
            console.error('SingleKvStorePutBatchPromiseTest001 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStorePutBatchPromiseTest002
     * @tc.desc Test Js Api SingleKvStore.PutBatch() testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStorePutBatchPromiseTest002', 0, async function (done) {
        console.info('SingleKvStorePutBatchPromiseTest002');
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
            console.info('SingleKvStorePutBatchPromiseTest002 entries: ' + JSON.stringify(entries));
            await kvStore.putBatch(entries).then(async (err) => {
                console.info('SingleKvStorePutBatchPromiseTest002 putBatch success');
                expect(err == undefined).assertTrue();
                await kvStore.getEntries('batch_test_number_key').then((entrys) => {
                    console.info('SingleKvStorePutBatchPromiseTest002 getEntries success');
                    expect(entrys.length == 10).assertTrue();
                    expect(entrys[0].value.value == 222).assertTrue();
                }).catch((err) => {
                    console.error('SingleKvStorePutBatchPromiseTest002 getEntries fail ' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(null).assertFail();
                });
            }).catch((err) => {
                console.error('SingleKvStorePutBatchPromiseTest002 putBatch fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        }catch(e) {
            console.error('SingleKvStorePutBatchPromiseTest002 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStorePutBatchPromiseTest003
     * @tc.desc Test Js Api SingleKvStore.PutBatch() testcase 003
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStorePutBatchPromiseTest003', 0, async function (done) {
        console.info('SingleKvStorePutBatchPromiseTest003');
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
            console.info('SingleKvStorePutBatchPromiseTest003 entries: ' + JSON.stringify(entries));
            await kvStore.putBatch(entries).then(async (err) => {
                console.info('SingleKvStorePutBatchPromiseTest003 putBatch success');
                expect(err == undefined).assertTrue();
                await kvStore.getEntries('batch_test_number_key').then((entrys) => {
                    console.info('SingleKvStorePutBatchPromiseTest003 getEntries success');
                    expect(entrys.length == 10).assertTrue();
                    expect(entrys[0].value.value == 2.0).assertTrue();
                }).catch((err) => {
                    console.error('SingleKvStorePutBatchPromiseTest003 getEntries fail ' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(null).assertFail();
                });
            }).catch((err) => {
                console.error('SingleKvStorePutBatchPromiseTest003 putBatch fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        }catch(e) {
            console.error('SingleKvStorePutBatchPromiseTest003 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStorePutBatchPromiseTest004
     * @tc.desc Test Js Api SingleKvStore.PutBatch() testcase 004
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStorePutBatchPromiseTest004', 0, async function (done) {
        console.info('SingleKvStorePutBatchPromiseTest004');
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
            console.info('SingleKvStorePutBatchPromiseTest004 entries: ' + JSON.stringify(entries));
            await kvStore.putBatch(entries).then(async (err) => {
                console.info('SingleKvStorePutBatchPromiseTest004 putBatch success');
                expect(err == undefined).assertTrue();
                await kvStore.getEntries('batch_test_number_key').then((entrys) => {
                    console.info('SingleKvStorePutBatchPromiseTest004 getEntries success');
                    expect(entrys.length == 10).assertTrue();
                    expect(entrys[0].value.value == 2.00).assertTrue();
                }).catch((err) => {
                    console.error('SingleKvStorePutBatchPromiseTest004 getEntries fail ' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(null).assertFail();
                });
            }).catch((err) => {
                console.error('SingleKvStorePutBatchPromiseTest004 putBatch fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        }catch(e) {
            console.error('SingleKvStorePutBatchPromiseTest004 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStorePutBatchPromiseTest005
     * @tc.desc Test Js Api SingleKvStore.PutBatch() testcase 005
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStorePutBatchPromiseTest005', 0, async function (done) {
        console.info('SingleKvStorePutBatchPromiseTest005');
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
            console.info('SingleKvStorePutBatchPromiseTest005 entries: ' + JSON.stringify(entries));
            await kvStore.putBatch(entries).then(async (err) => {
                console.info('SingleKvStorePutBatchPromiseTest005 putBatch success');
                expect(err == undefined).assertTrue();
                await kvStore.getEntries('batch_test_bool_key').then((entrys) => {
                    console.info('SingleKvStorePutBatchPromiseTest005 getEntries success');
                    expect(entrys.length == 10).assertTrue();
                    expect(entrys[0].value.value == bo).assertTrue();
                }).catch((err) => {
                    console.error('SingleKvStorePutBatchPromiseTest005 getEntries fail ' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(null).assertFail();
                });
            }).catch((err) => {
                console.error('SingleKvStorePutBatchPromiseTest005 putBatch fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        }catch(e) {
            console.error('SingleKvStorePutBatchPromiseTest005 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStorePutBatchPromiseTest006
     * @tc.desc Test Js Api SingleKvStore.PutBatch() testcase 006
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStorePutBatchPromiseTest006', 0, async function (done) {
        console.info('SingleKvStorePutBatchPromiseTest006');
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
            console.info('SingleKvStorePutBatchPromiseTest006 entries: ' + JSON.stringify(entries));
            await kvStore.putBatch(entries).then(async (err) => {
                console.info('SingleKvStorePutBatchPromiseTest006 putBatch success');
                expect(err == undefined).assertTrue();
                await kvStore.getEntries('batch_test_bool_key').then((entrys) => {
                    console.info('SingleKvStorePutBatchPromiseTest006 getEntries success');
                    expect(entrys.length == 10).assertTrue();
                    expect(entrys[0].value.value.toString() == arr.toString()).assertTrue();
                }).catch((err) => {
                    console.error('SingleKvStorePutBatchPromiseTest006 getEntries fail ' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(null).assertFail();
                });
            }).catch((err) => {
                console.error('SingleKvStorePutBatchPromiseTest006 putBatch fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        }catch(e) {
            console.error('SingleKvStorePutBatchPromiseTest005 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreDeleteBatchPromiseTest001
     * @tc.desc Test Js Api SingleKvStoreDeleteBatch testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreDeleteBatchPromiseTest001', 0, async function (done) {
        console.info('SingleKvStoreDeleteBatchPromiseTest001');
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
            console.info('SingleKvStoreDeleteBatchPromiseTest001 entries: ' + JSON.stringify(entries));
            await kvStore.putBatch(entries).then(async (err) => {
                console.info('SingleKvStoreDeleteBatchPromiseTest001 putBatch success');
                expect(err == undefined).assertTrue();
                await kvStore.deleteBatch(keys).then((err) => {
                    console.info('SingleKvStoreDeleteBatchPromiseTest001 deleteBatch success');
                    expect(err == undefined).assertTrue();
                }).catch((err) => {
                    console.error('SingleKvStoreDeleteBatchPromiseTest001 deleteBatch fail ' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(null).assertFail();
                });
            }).catch((err) => {
                console.error('SingleKvStoreDeleteBatchPromiseTest001 putBatch fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        }catch(e) {
            console.error('SingleKvStoreDeleteBatchPromiseTest001 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreDeleteBatchPromiseTest002
     * @tc.desc Test Js Api SingleKvStoreDeleteBatch testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreDeleteBatchPromiseTest002', 0, async function (done) {
        console.info('SingleKvStoreDeleteBatchPromiseTest002');
        try {
            let keys = ['batch_test_string_key1', 'batch_test_string_key2'];
            await kvStore.deleteBatch(keys).then((err) => {
                console.info('SingleKvStoreDeleteBatchPromiseTest002 deleteBatch success');
            }).catch((err) => {
                console.error('SingleKvStoreDeleteBatchPromiseTest002 deleteBatch fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        }catch(e) {
            console.error('SingleKvStoreDeleteBatchPromiseTest002 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreDeleteBatchPromiseTest003
     * @tc.desc Test Js Api SingleKvStoreDeleteBatch testcase 003
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreDeleteBatchPromiseTest003', 0, async function (done) {
        console.info('SingleKvStoreDeleteBatchPromiseTest003');
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
            console.info('SingleKvStoreDeleteBatchPromiseTest003 entries: ' + JSON.stringify(entries));
            await kvStore.putBatch(entries).then(async (err) => {
                console.info('SingleKvStoreDeleteBatchPromiseTest003 putBatch success');
                expect(err == undefined).assertTrue();
                let keys = ['batch_test_string_key1', 'batch_test_string_keya'];
                await kvStore.deleteBatch(keys).then((err) => {
                    console.info('SingleKvStoreDeleteBatchPromiseTest003 deleteBatch success');
                }).catch((err) => {
                    console.error('SingleKvStoreDeleteBatchPromiseTest003 deleteBatch fail ' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(null).assertFail();
                });
            }).catch((err) => {
                console.error('SingleKvStoreDeleteBatchPromiseTest003 putBatch fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        }catch(e) {
            console.error('SingleKvStoreDeleteBatchPromiseTest003 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStorestartTransactionPromiseTest001
     * @tc.desc Test Js Api SingleKvStorestartTransaction testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStorestartTransactionPromiseTest001', 0, async function (done) {
        console.info('SingleKvStorestartTransactionPromiseTest001');
        try {
            var count = 0;
            kvStore.on('dataChange', factory.SubscribeType.SUBSCRIBE_TYPE_ALL, function (data) {
                console.info('SingleKvStorestartTransactionPromiseTest001 ' + JSON.stringify(data));
                count++;
            });
            await kvStore.startTransaction().then(async (err) => {
                console.info('SingleKvStorestartTransactionPromiseTest001 startTransaction success');
                expect(err == undefined).assertTrue();
            }).catch((err) => {
                console.error('SingleKvStorestartTransactionPromiseTest001 startTransaction fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
            let entries = putBatchString(10, 'batch_test_string_key');
            console.info('SingleKvStorestartTransactionPromiseTest001 entries: ' + JSON.stringify(entries));
            await kvStore.putBatch(entries).then(async (err) => {
                console.info('SingleKvStorestartTransactionPromiseTest001 putBatch success');
                expect(err == undefined).assertTrue();
            }).catch((err) => {
                console.error('SingleKvStorestartTransactionPromiseTest001 putBatch fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
            let keys = Object.keys(entries).slice(5); //delete 5 beginnings
            await kvStore.deleteBatch(keys).then((err) => {
                console.info('SingleKvStorestartTransactionPromiseTest001 deleteBatch success');
                expect(err == undefined).assertTrue();
            }).catch((err) => {
                console.error('SingleKvStorestartTransactionPromiseTest001 deleteBatch fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
            await kvStore.commit().then(async (err) => {
                console.info('SingleKvStorestartTransactionPromiseTest001 commit success');
                expect(err == undefined).assertTrue();
            }).catch((err) => {
                console.error('SingleKvStorestartTransactionPromiseTest001 commit fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
            await sleep(2000);
            expect(count == 1).assertTrue();
        }catch(e) {
            console.error('SingleKvStorestartTransactionPromiseTest001 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStorestartTransactionPromiseTest002
     * @tc.desc Test Js Api SingleKvStorestartTransaction testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStorestartTransactionPromiseTest002', 0, async function (done) {
        console.info('SingleKvStorestartTransactionPromiseTest002');
        try {
            var count = 0;
            kvStore.on('dataChange', 0, function (data) {
                console.info('SingleKvStorestartTransactionPromiseTest002 ' + JSON.stringify(data));
                count++;
            });
            await kvStore.startTransaction().then(async (err) => {
                console.info('SingleKvStorestartTransactionPromiseTest002 startTransaction success');
                expect(err == undefined).assertTrue();
            }).catch((err) => {
                console.error('SingleKvStorestartTransactionPromiseTest002 startTransaction fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
            let entries = putBatchString(10, 'batch_test_string_key');
            console.info('SingleKvStorestartTransactionPromiseTest002 entries: ' + JSON.stringify(entries));
            await kvStore.putBatch(entries).then(async (err) => {
                console.info('SingleKvStorestartTransactionPromiseTest002 putBatch success');
                expect(err == undefined).assertTrue();
            }).catch((err) => {
                console.error('SingleKvStorestartTransactionPromiseTest002 putBatch fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
            let keys = Object.keys(entries).slice(5); //delete 5 beginnings
            await kvStore.deleteBatch(keys).then((err) => {
                console.info('SingleKvStorestartTransactionPromiseTest002 deleteBatch success');
                expect(err == undefined).assertTrue();
            }).catch((err) => {
                console.error('SingleKvStorestartTransactionPromiseTest002 deleteBatch fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
            await kvStore.rollback().then(async (err) => {
                console.info('SingleKvStorestartTransactionPromiseTest002 rollback success');
                expect(err == undefined).assertTrue();
            }).catch((err) => {
                console.error('SingleKvStorestartTransactionPromiseTest002 rollback fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
            await sleep(2000);
            expect(count == 0).assertTrue();
        }catch(e) {
            console.error('SingleKvStorestartTransactionPromiseTest002 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStorestartTransactionPromiseTest003
     * @tc.desc Test Js Api SingleKvStorestartTransaction testcase 003
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStorestartTransactionPromiseTest003', 0, async function (done) {
        console.info('SingleKvStorestartTransactionPromiseTest002');
        try {
            await kvStore.startTransaction(1).then(async (err) => {
                console.info('SingleKvStorestartTransactionPromiseTest003 startTransaction success');
                expect(null).assertFail();
            }).catch((err) => {
                console.error('SingleKvStorestartTransactionPromiseTest003 startTransaction fail ' + `, error code is ${err.code}, message is ${err.message}`);
            });
        }catch(e) {
            console.error('SingleKvStorestartTransactionPromiseTest003 e ' + `, error code is ${e.code}, message is ${e.message}`);
        }
        done();
    })

    /**
     * @tc.name SingleKvStorestartTransactionPromiseTest004
     * @tc.desc Test Js Api SingleKvStorestartTransaction testcase 004
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStorestartTransactionPromiseTest004', 0, async function (done) {
        console.info('SingleKvStorestartTransactionPromiseTest004');
        try {
            await kvStore.startTransaction('test_string').then(async (err) => {
                console.info('SingleKvStorestartTransactionPromiseTest004 startTransaction success');
                expect(null).assertFail();
            }).catch((err) => {
                console.error('SingleKvStorestartTransactionPromiseTest004 startTransaction fail ' + `, error code is ${err.code}, message is ${err.message}`);
            });
        }catch(e) {
            console.error('SingleKvStorestartTransactionPromiseTest004 e ' + `, error code is ${e.code}, message is ${e.message}`);
        }
        done();
    })

    /**
     * @tc.name SingleKvStorestartTransactionPromiseTest005
     * @tc.desc Test Js Api SingleKvStorestartTransaction testcase 005
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStorestartTransactionPromiseTest005', 0, async function (done) {
        console.info('SingleKvStorestartTransactionPromiseTest005');
        try {
            await kvStore.startTransaction(2.000).then(async (err) => {
                console.info('SingleKvStorestartTransactionPromiseTest005 startTransaction success');
                expect(null).assertFail();
            }).catch((err) => {
                console.error('SingleKvStorestartTransactionPromiseTest005 startTransaction fail ' + `, error code is ${err.code}, message is ${err.message}`);
            });
        }catch(e) {
            console.error('SingleKvStorestartTransactionPromiseTest005 e ' + `, error code is ${e.code}, message is ${e.message}`);
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreCommitPromiseTest001
     * @tc.desc Test Js Api SingleKvStoreCommit testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreCommitPromiseTest001', 0, async function (done) {
        console.info('SingleKvStoreCommitPromiseTest001');
        try {
            await kvStore.commit(1).then(async (err) => {
                console.info('SingleKvStoreCommitPromiseTest001 commit success');
                expect(null).assertFail();
            }).catch((err) => {
                console.error('SingleKvStoreCommitPromiseTest001 commit fail ' + `, error code is ${err.code}, message is ${err.message}`);
            });
        }catch(e) {
            console.error('SingleKvStoreCommitPromiseTest001 e ' + `, error code is ${e.code}, message is ${e.message}`);
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreCommitPromiseTest002
     * @tc.desc Test Js Api SingleKvStoreCommit testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreCommitPromiseTest002', 0, async function (done) {
        console.info('SingleKvStoreCommitPromiseTest002');
        try {
            await kvStore.commit('test_string').then(async (err) => {
                console.info('SingleKvStoreCommitPromiseTest002 commit success');
                expect(null).assertFail();
            }).catch((err) => {
                console.error('SingleKvStoreCommitPromiseTest002 commit fail ' + `, error code is ${err.code}, message is ${err.message}`);
            });
        }catch(e) {
            console.error('SingleKvStoreCommitPromiseTest002 e ' + `, error code is ${e.code}, message is ${e.message}`);
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreCommitPromiseTest003
     * @tc.desc Test Js Api SingleKvStoreCommit testcase 003
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreCommitPromiseTest003', 0, async function (done) {
        console.info('SingleKvStoreCommitPromiseTest003');
        try {
            await kvStore.commit(2.000).then(async (err) => {
                console.info('SingleKvStoreCommitPromiseTest003 commit success');
                expect(null).assertFail();
            }).catch((err) => {
                console.error('SingleKvStoreCommitPromiseTest003 commit fail ' + `, error code is ${err.code}, message is ${err.message}`);
            });
        }catch(e) {
            console.error('SingleKvStoreCommitPromiseTest003 e ' + `, error code is ${e.code}, message is ${e.message}`);
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreRollbackPromiseTest001
     * @tc.desc Test Js Api SingleKvStoreRollback testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreRollbackPromiseTest001', 0, async function (done) {
        console.info('SingleKvStoreRollbackPromiseTest001');
        try {
            await kvStore.rollback(1).then(async (err) => {
                console.info('SingleKvStoreRollbackPromiseTest001 rollback success');
                expect(null).assertFail();
            }).catch((err) => {
                console.error('SingleKvStoreRollbackPromiseTest001 rollback fail ' + `, error code is ${err.code}, message is ${err.message}`);
            });
        }catch(e) {
            console.error('SingleKvStoreRollbackPromiseTest001 e ' + `, error code is ${e.code}, message is ${e.message}`);
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreRollbackPromiseTest002
     * @tc.desc Test Js Api SingleKvStoreRollback testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreRollbackPromiseTest002', 0, async function (done) {
        console.info('SingleKvStoreRollbackPromiseTest002');
        try {
            await kvStore.rollback('test_string').then(async (err) => {
                console.info('SingleKvStoreRollbackPromiseTest002 rollback success');
                expect(null).assertFail();
            }).catch((err) => {
                console.error('SingleKvStoreRollbackPromiseTest002 rollback fail ' + `, error code is ${err.code}, message is ${err.message}`);
            });
        }catch(e) {
            console.error('SingleKvStoreRollbackPromiseTest002 e ' + `, error code is ${e.code}, message is ${e.message}`);
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreRollbackPromiseTest003
     * @tc.desc Test Js Api SingleKvStoreRollback testcase 003
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreRollbackPromiseTest003', 0, async function (done) {
        console.info('SingleKvStoreRollbackPromiseTest003');
        try {
            await kvStore.rollback(2.000).then(async (err) => {
                console.info('SingleKvStoreRollbackPromiseTest003 rollback success');
                expect(null).assertFail();
            }).catch((err) => {
                console.error('SingleKvStoreRollbackPromiseTest003 rollback fail ' + `, error code is ${err.code}, message is ${err.message}`);
            });
        }catch(e) {
            console.error('SingleKvStoreRollbackPromiseTest003 e ' + `, error code is ${e.code}, message is ${e.message}`);
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreEnableSyncPromiseTest001
     * @tc.desc Test Js Api SingleKvStoreEnableSync testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreEnableSyncPromiseTest001', 0, async function (done) {
        console.info('SingleKvStoreEnableSyncPromiseTest001');
        try {
            await kvStore.enableSync(true).then((err) => {
                console.info('SingleKvStoreEnableSyncPromiseTest001 enableSync success');
                expect(err == undefined).assertTrue();
            }).catch((err) => {
                console.error('SingleKvStoreEnableSyncPromiseTest001 enableSync fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        }catch(e) {
            console.error('SingleKvStoreEnableSyncPromiseTest001 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreEnableSyncPromiseTest002
     * @tc.desc Test Js Api SingleKvStoreEnableSync testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreEnableSyncPromiseTest002', 0, async function (done) {
        console.info('SingleKvStoreEnableSyncPromiseTest002');
        try {
            await kvStore.enableSync(false).then((err) => {
                console.info('SingleKvStoreEnableSyncPromiseTest002 enableSync success');
                expect(err == undefined).assertTrue();
            }).catch((err) => {
                console.error('SingleKvStoreEnableSyncPromiseTest002 enableSync fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        }catch(e) {
            console.error('SingleKvStoreEnableSyncPromiseTest002 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreEnableSyncPromiseTest003
     * @tc.desc Test Js Api SingleKvStoreEnableSync testcase 003
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreEnableSyncPromiseTest003', 0, async function (done) {
        console.info('SingleKvStoreEnableSyncPromiseTest003');
        try {
            await kvStore.enableSync().then((err) => {
                console.info('SingleKvStoreEnableSyncPromiseTest003 enableSync success');
                expect(null).assertFail();
            }).catch((err) => {
                console.error('SingleKvStoreEnableSyncPromiseTest003 enableSync fail ' + `, error code is ${err.code}, message is ${err.message}`);
            });
        }catch(e) {
            console.error('SingleKvStoreEnableSyncPromiseTest003 e ' + `, error code is ${e.code}, message is ${e.message}`);
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreEnableSyncPromiseTest004
     * @tc.desc Test Js Api SingleKvStoreEnableSync testcase 004
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreEnableSyncPromiseTest004', 0, async function (done) {
        console.info('SingleKvStoreEnableSyncPromiseTest004');
        try {
            await kvStore.enableSync(null).then((err) => {
                console.info('SingleKvStoreEnableSyncPromiseTest004 enableSync success');
                expect(null).assertFail();
            }).catch((err) => {
                console.error('SingleKvStoreEnableSyncPromiseTest004 enableSync fail ' + `, error code is ${err.code}, message is ${err.message}`);
            });
        }catch(e) {
            console.error('SingleKvStoreEnableSyncPromiseTest004 e ' + `, error code is ${e.code}, message is ${e.message}`);
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreRemoveDeviceDataPromiseTest001
     * @tc.desc Test Js Api SingleKvStoreRemoveDeviceData testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreRemoveDeviceDataPromiseTest001', 0, async function (done) {
        console.info('SingleKvStoreRemoveDeviceDataPromiseTest001');
        try {
            await kvStore.put(KEY_TEST_STRING_ELEMENT, VALUE_TEST_STRING_ELEMENT).then((err) => {
                console.info('SingleKvStoreRemoveDeviceDataPromiseTest001 put success');
                expect(err == undefined).assertTrue();
            }).catch((err) => {
                console.error('SingleKvStoreRemoveDeviceDataPromiseTest001 put fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
            var deviceid = 'no_exist_device_id';
            await kvStore.removeDeviceData(deviceid).then((err) => {
                console.info('SingleKvStoreRemoveDeviceDataPromiseTest001 removeDeviceData success');
                expect(null).assertFail();
            }).catch((err) => {
                console.error('SingleKvStoreRemoveDeviceDataPromiseTest001 removeDeviceData fail ' + `, error code is ${err.code}, message is ${err.message}`);
            });
            await kvStore.get(KEY_TEST_STRING_ELEMENT).then((data) => {
                console.info('SingleKvStoreRemoveDeviceDataPromiseTest001 get success data:' + data);
                expect(data == VALUE_TEST_STRING_ELEMENT).assertTrue();
            }).catch((err) => {
                console.error('SingleKvStoreRemoveDeviceDataPromiseTest001 get fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        }catch(e) {
            console.error('SingleKvStoreRemoveDeviceDataPromiseTest001 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreRemoveDeviceDataPromiseTest002
     * @tc.desc Test Js Api SingleKvStoreRemoveDeviceData testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreRemoveDeviceDataPromiseTest002', 0, async function (done) {
        console.info('SingleKvStoreRemoveDeviceDataPromiseTest002');
        try {
            await kvStore.removeDeviceData().then((err) => {
                console.info('SingleKvStoreRemoveDeviceDataPromiseTest002 removeDeviceData success');
                expect(null).assertFail();
            }).catch((err) => {
                console.error('SingleKvStoreRemoveDeviceDataPromiseTest002 removeDeviceData fail ' + `, error code is ${err.code}, message is ${err.message}`);
            });
        }catch(e) {
            console.error('SingleKvStoreRemoveDeviceDataPromiseTest002 e ' + `, error code is ${e.code}, message is ${e.message}`);
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreRemoveDeviceDataPromiseTest003
     * @tc.desc Test Js Api SingleKvStoreRemoveDeviceData testcase 003
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
     it('SingleKvStoreRemoveDeviceDataPromiseTest003', 0, async function (done) {
        console.info('SingleKvStoreRemoveDeviceDataPromiseTest003');
        try {
            await kvStore.removeDeviceData('').then((data) => {
                console.info('SingleKvStoreRemoveDeviceDataPromiseTest003 removeDeviceData success');
                expect(null).assertFail();
            }).catch((err) => {
                console.error('SingleKvStoreRemoveDeviceDataPromiseTest003 removeDeviceData fail ' + `, error code is ${err.code}, message is ${err.message}`);
            });
        }catch(e) {
            console.error('SingleKvStoreRemoveDeviceDataPromiseTest003 e ' + `, error code is ${e.code}, message is ${e.message}`);
        }
        done();
    })


    /**
     * @tc.name SingleKvStoreRemoveDeviceDataPromiseTest004
     * @tc.desc Test Js Api SingleKvStoreRemoveDeviceData testcase 004
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreRemoveDeviceDataPromiseTest004', 0, async function (done) {
        console.info('SingleKvStoreRemoveDeviceDataPromiseTest004');
        try {
            await kvStore.removeDeviceData(null).then((err) => {
                console.info('SingleKvStoreRemoveDeviceDataPromiseTest004 removeDeviceData success');
                expect(null).assertFail();
            }).catch((err) => {
                console.error('SingleKvStoreRemoveDeviceDataPromiseTest004 removeDeviceData fail ' + `, error code is ${err.code}, message is ${err.message}`);
            });
        }catch(e) {
            console.error('SingleKvStoreRemoveDeviceDataPromiseTest004 e ' + `, error code is ${e.code}, message is ${e.message}`);
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreSetSyncParamPromiseTest001
     * @tc.desc Test Js Api SingleKvStoreSetSyncParam testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreSetSyncParamPromiseTest001', 0, async function (done) {
        console.info('SingleKvStoreSetSyncParamPromiseTest001');
        try {
            var defaultAllowedDelayMs = 500;
            await kvStore.setSyncParam(defaultAllowedDelayMs).then((err) => {
                console.info('SingleKvStoreSetSyncParamPromiseTest001 put success');
                expect(err == undefined).assertTrue();
            }).catch((err) => {
                console.error('SingleKvStoreSetSyncParamPromiseTest001 put fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        }catch(e) {
            console.error('SingleKvStoreSetSyncParamPromiseTest001 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreSetSyncParamPromiseTest002
     * @tc.desc Test Js Api SingleKvStoreSetSyncParam testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreSetSyncParamPromiseTest002', 0, async function (done) {
        console.info('SingleKvStoreSetSyncParamPromiseTest002');
        try {
            await kvStore.setSyncParam().then((err) => {
                console.info('SingleKvStoreSetSyncParamPromiseTest002 put success');
                expect(null).assertFail();
            }).catch((err) => {
                console.error('SingleKvStoreSetSyncParamPromiseTest002 put fail ' + `, error code is ${err.code}, message is ${err.message}`);
            });
        }catch(e) {
            console.error('SingleKvStoreSetSyncParamPromiseTest002 e ' + `, error code is ${e.code}, message is ${e.message}`);
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreSetSyncParamPromiseTest003
     * @tc.desc Test Js Api SingleKvStoreSetSyncParam testcase 003
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreSetSyncParamPromiseTest003', 0, async function (done) {
        console.info('SingleKvStoreSetSyncParamPromiseTest003');
        try {
            await kvStore.setSyncParam('').then((err) => {
                console.info('SingleKvStoreSetSyncParamPromiseTest003 put success');
                expect(null).assertFail();
            }).catch((err) => {
                console.error('SingleKvStoreSetSyncParamPromiseTest003 put fail ' + `, error code is ${err.code}, message is ${err.message}`);
            });
        }catch(e) {
            console.error('SingleKvStoreSetSyncParamPromiseTest003 e ' + `, error code is ${e.code}, message is ${e.message}`);
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreSetSyncParamPromiseTest004
     * @tc.desc Test Js Api SingleKvStoreSetSyncParam testcase 004
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreSetSyncParamPromiseTest004', 0, async function (done) {
        console.info('SingleKvStoreSetSyncParamPromiseTest004');
        try {
            await kvStore.setSyncParam(null).then((err) => {
                console.info('SingleKvStoreSetSyncParamPromiseTest004 put success');
                expect(null).assertFail();
            }).catch((err) => {
                console.error('SingleKvStoreSetSyncParamPromiseTest004 put fail ' + `, error code is ${err.code}, message is ${err.message}`);
            });
        }catch(e) {
            console.error('SingleKvStoreSetSyncParamPromiseTest004 e ' + `, error code is ${e.code}, message is ${e.message}`);
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreGetSecurityLevelPromiseTest002
     * @tc.desc Test Js Api SingleKvStoreGetSecurityLevel testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreGetSecurityLevelPromiseTest002', 0, async function (done) {
        console.info('SingleKvStoreGetSecurityLevelPromiseTest002');
        try {
            await kvStore.getSecurityLevel(1).then((data) => {
                console.info('SingleKvStoreGetSecurityLevelPromiseTest002 getSecurityLevel success');
                expect(null).assertFail();
            }).catch((err) => {
                console.error('SingleKvStoreGetSecurityLevelPromiseTest002 getSecurityLevel fail ' + `, error code is ${err.code}, message is ${err.message}`);
            });
        }catch(e) {
            console.error('SingleKvStoreGetSecurityLevelPromiseTest002 e ' + `, error code is ${e.code}, message is ${e.message}`);
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreGetSecurityLevelPromiseTest003
     * @tc.desc Test Js Api SingleKvStoreGetSecurityLevel testcase 003
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreGetSecurityLevelPromiseTest003', 0, async function (done) {
        console.info('SingleKvStoreGetSecurityLevelPromiseTest003');
        try {
            await kvStore.getSecurityLevel('test_string').then((data) => {
                console.info('SingleKvStoreGetSecurityLevelPromiseTest003 getSecurityLevel success');
                expect(null).assertFail();
            }).catch((err) => {
                console.error('SingleKvStoreGetSecurityLevelPromiseTest003 getSecurityLevel fail ' + `, error code is ${err.code}, message is ${err.message}`);
            });
        }catch(e) {
            console.error('SingleKvStoreGetSecurityLevelPromiseTest003 e ' + `, error code is ${e.code}, message is ${e.message}`);
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreGetSecurityLevelPromiseTest004
     * @tc.desc Test Js Api SingleKvStoreGetSecurityLevel testcase 004
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreGetSecurityLevelPromiseTest004', 0, async function (done) {
        console.info('SingleKvStoreGetSecurityLevelPromiseTest004');
        try {
            await kvStore.getSecurityLevel(2.000).then((data) => {
                console.info('SingleKvStoreGetSecurityLevelPromiseTest004 getSecurityLevel success');
                expect(null).assertFail();
            }).catch((err) => {
                console.error('SingleKvStoreGetSecurityLevelPromiseTest004 getSecurityLevel fail ' + `, error code is ${err.code}, message is ${err.message}`);
            });
        }catch(e) {
            console.error('SingleKvStoreGetSecurityLevelPromiseTest004 e ' + `, error code is ${e.code}, message is ${e.message}`);
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreGetResultSetPromiseTest001
     * @tc.desc Test Js Api SingleKvStoreGetResultSet testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreGetResultSetPromiseTest001', 0, async function (done) {
        console.info('SingleKvStoreGetResultSetPromiseTest001');
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
                console.info('SingleKvStoreGetResultSetPromiseTest001 putBatch success');
                expect(err == undefined).assertTrue();
            }).catch((err) => {
                console.error('SingleKvStorePutBatchPromiseTest001 putBatch fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
            await kvStore.getResultSet('batch_test_string_key').then((result) => {
                console.info('SingleKvStoreGetResultSetPromiseTest001 getResultSet success');
                resultSet = result;
                expect(resultSet.getCount() == 10).assertTrue();
            }).catch((err) => {
                console.error('SingleKvStoreGetResultSetPromiseTest001 getResultSet fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
            await kvStore.closeResultSet(resultSet).then((err) => {
                console.info('SingleKvStoreGetResultSetPromiseTest001 closeResultSet success');
                expect(err == undefined).assertTrue();
            }).catch((err) => {
                console.error('SingleKvStoreGetResultSetPromiseTest001 closeResultSet fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        }catch(e) {
            console.error('SingleKvStoreGetResultSetPromiseTest001 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreGetResultSetPromiseTest002
     * @tc.desc Test Js Api SingleKvStoreGetResultSet testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreGetResultSetPromiseTest002', 0, async function (done) {
        console.info('SingleKvStoreGetResultSetPromiseTest002');
        try {
            let resultSet;
            await kvStore.getResultSet('batch_test_string_key').then((result) => {
                console.info('SingleKvStoreGetResultSetPromiseTest002 getResultSet success');
                resultSet = result;
                expect(resultSet.getCount() == 0).assertTrue();
            }).catch((err) => {
                console.error('SingleKvStoreGetResultSetPromiseTest002 getResultSet fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
            await kvStore.closeResultSet(resultSet).then((err) => {
                console.info('SingleKvStoreGetResultSetPromiseTest002 closeResultSet success');
                expect(err == undefined).assertTrue();
            }).catch((err) => {
                console.error('SingleKvStoreGetResultSetPromiseTest002 closeResultSet fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        }catch(e) {
            console.error('SingleKvStoreGetResultSetPromiseTest002 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreGetResultSetPromiseTest003
     * @tc.desc Test Js Api SingleKvStoreGetResultSet testcase 003
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreGetResultSetPromiseTest003', 0, async function (done) {
        console.info('SingleKvStoreGetResultSetPromiseTest003');
        try {
            let resultSet;
            await kvStore.getResultSet().then((result) => {
                console.info('SingleKvStoreGetResultSetPromiseTest003 getResultSet success');
                expect(null).assertFail();
            }).catch((err) => {
                console.error('SingleKvStoreGetResultSetPromiseTest003 getResultSet fail ' + `, error code is ${err.code}, message is ${err.message}`);
            });
        }catch(e) {
            console.error('SingleKvStoreGetResultSetPromiseTest003 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreGetResultSetPromiseTest004
     * @tc.desc Test Js Api SingleKvStoreGetResultSet testcase 004
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreGetResultSetPromiseTest004', 0, async function (done) {
        console.info('SingleKvStoreGetResultSetPromiseTest004');
        try {
            let resultSet;
            await kvStore.getResultSet('test_key_string', 123).then((result) => {
                console.info('SingleKvStoreGetResultSetPromiseTest004 getResultSet success');
                expect(null).assertFail();
            }).catch((err) => {
                console.error('SingleKvStoreGetResultSetPromiseTest004 getResultSet fail ' + `, error code is ${err.code}, message is ${err.message}`);
            });
        }catch(e) {
            console.error('SingleKvStoreGetResultSetPromiseTest004 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreGetResultSetPromiseTest005
     * @tc.desc Test Js Api SingleKvStoreGetResultSet testcase 005
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreGetResultSetPromiseTest005', 0, async function (done) {
        console.info('SingleKvStoreGetResultSetPromiseTest005');
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
                console.info('SingleKvStoreGetResultSetPromiseTest005 putBatch success');
                expect(err == undefined).assertTrue();
            }).catch((err) => {
                console.error('SingleKvStorePutBatchPromiseTest001 putBatch fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
            var query = new factory.Query();
            query.prefixKey("batch_test");
            await kvStore.getResultSet(query).then((result) => {
                console.info('SingleKvStoreGetResultSetPromiseTest005 getResultSet success');
                resultSet = result;
                expect(resultSet.getCount() == 10).assertTrue();
            }).catch((err) => {
                console.error('SingleKvStoreGetResultSetPromiseTest005 getResultSet fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
            await kvStore.closeResultSet(resultSet).then((err) => {
                console.info('SingleKvStoreGetResultSetPromiseTest005 closeResultSet success');
                expect(err == undefined).assertTrue();
            }).catch((err) => {
                console.error('SingleKvStoreGetResultSetPromiseTest005 closeResultSet fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        }catch(e) {
            console.error('SingleKvStoreGetResultSetPromiseTest005 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreGetResultSetPromiseTest006
     * @tc.desc Test Js Api SingleKvStoreGetResultSet testcase 006
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreGetResultSetPromiseTest006', 0, async function (done) {
        console.info('SingleKvStoreGetResultSetPromiseTest006');
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
                console.info('SingleKvStoreGetResultSetPromiseTest006 putBatch success');
                expect(err == undefined).assertTrue();
            }).catch((err) => {
                console.error('SingleKvStorePutBatchPromiseTest001 putBatch fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
            var query = new factory.Query();
            query.prefixKey("batch_test");
            await kvStore.getResultSet(query).then((result) => {
                console.info('SingleKvStoreGetResultSetPromiseTest006 getResultSet success');
                resultSet = result;
                expect(resultSet.getCount() == 10).assertTrue();
            }).catch((err) => {
                console.error('SingleKvStoreGetResultSetPromiseTest006 getResultSet fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
            await kvStore.closeResultSet(resultSet).then((err) => {
                console.info('SingleKvStoreGetResultSetPromiseTest006 closeResultSet success');
                expect(err == undefined).assertTrue();
            }).catch((err) => {
                console.error('SingleKvStoreGetResultSetPromiseTest006 closeResultSet fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        }catch(e) {
            console.error('SingleKvStoreGetResultSetPromiseTest006 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreCloseResultSetPromiseTest001
     * @tc.desc Test Js Api SingleKvStoreCloseResultSet testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreCloseResultSetPromiseTest001', 0, async function (done) {
        console.info('SingleKvStoreCloseResultSetPromiseTest001');
        try {
            console.info('SingleKvStoreCloseResultSetPromiseTest001 success');
            let resultSet = null;
            await kvStore.closeResultSet(resultSet).then(() => {
                console.info('SingleKvStoreCloseResultSetPromiseTest001 closeResultSet success');
                expect(null).assertFail();
            }).catch((err) => {
                console.error('SingleKvStoreCloseResultSetPromiseTest001 closeResultSet fail ' + `, error code is ${err.code}, message is ${err.message}`);
            });
        }catch(e) {
            console.error('SingleKvStoreCloseResultSetPromiseTest001 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreCloseResultSetPromiseTest002
     * @tc.desc Test Js Api SingleKvStoreCloseResultSet testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreCloseResultSetPromiseTest002', 0, async function (done) {
        console.info('SingleKvStoreCloseResultSetPromiseTest002');
        try {
            console.info('SingleKvStoreCloseResultSetPromiseTest002 success');
            let resultSet = null;
            await kvStore.getResultSet('batch_test_string_key').then((result) => {
                console.info('SingleKvStoreCloseResultSetPromiseTest002 getResultSet success');
                resultSet = result;
            }).catch((err) => {
                console.error('SingleKvStoreCloseResultSetPromiseTest002 getResultSet fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
            await kvStore.closeResultSet(resultSet).then((err) => {
                console.info('SingleKvStoreCloseResultSetPromiseTest002 closeResultSet success');
                expect(err == undefined).assertTrue();
            }).catch((err) => {
                console.error('SingleKvStoreCloseResultSetPromiseTest002 closeResultSet fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        }catch(e) {
            console.error('SingleKvStoreCloseResultSetPromiseTest002 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreCloseResultSetPromiseTest003
     * @tc.desc Test Js Api SingleKvStoreCloseResultSet testcase 003
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreCloseResultSetPromiseTest003', 0, async function (done) {
        console.info('SingleKvStoreCloseResultSetPromiseTest003');
        try {
            console.info('SingleKvStoreCloseResultSetPromiseTest003 success');
            let resultSet = null;
            await kvStore.closeResultSet().then(() => {
                console.info('SingleKvStoreCloseResultSetPromiseTest003 closeResultSet success');
                expect(null).assertFail();
            }).catch((err) => {
                console.error('SingleKvStoreCloseResultSetPromiseTest003 closeResultSet fail ' + `, error code is ${err.code}, message is ${err.message}`);
            });
        }catch(e) {
            console.error('SingleKvStoreCloseResultSetPromiseTest003 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreCloseResultSetPromiseTest004
     * @tc.desc Test Js Api SingleKvStoreCloseResultSet testcase 004
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreCloseResultSetPromiseTest004', 0, async function (done) {
        console.info('SingleKvStoreCloseResultSetPromiseTest004');
        try {
            console.info('SingleKvStoreCloseResultSetPromiseTest004 success');
        }catch(e) {
            console.error('SingleKvStoreCloseResultSetPromiseTest004 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreGetResultSizePromiseTest001
     * @tc.desc Test Js Api SingleKvStoreGetResultSize testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreGetResultSizePromiseTest001', 0, async function (done) {
        console.info('SingleKvStoreGetResultSizePromiseTest001');
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
                console.info('SingleKvStoreGetResultSizePromiseTest001 putBatch success');
                expect(err == undefined).assertTrue();
            }).catch((err) => {
                console.error('SingleKvStorePutBatchPromiseTest001 putBatch fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
            var query = new factory.Query();
            query.prefixKey("batch_test");
            await kvStore.getResultSize(query).then((resultSize) => {
                console.info('SingleKvStoreGetResultSizePromiseTest001 getResultSet success');
                expect(resultSize == 10).assertTrue();
            }).catch((err) => {
                console.error('SingleKvStoreGetResultSizePromiseTest001 getResultSet fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        }catch(e) {
            console.error('SingleKvStoreGetResultSizePromiseTest001 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreGetResultSizePromiseTest002
     * @tc.desc Test Js Api SingleKvStoreGetResultSize testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreGetResultSizePromiseTest002', 0, async function (done) {
        console.info('SingleKvStoreGetResultSizePromiseTest001');
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
                console.info('SingleKvStoreGetResultSizePromiseTest001 putBatch success');
                expect(err == undefined).assertTrue();
            }).catch((err) => {
                console.error('SingleKvStorePutBatchPromiseTest001 putBatch fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
            var query = new factory.Query();
            query.prefixKey("batch_test");
            await kvStore.getResultSize(query).then((resultSize) => {
                console.info('SingleKvStoreGetResultSizePromiseTest001 getResultSet success');
                expect(resultSize == 10).assertTrue();
            }).catch((err) => {
                console.error('SingleKvStoreGetResultSizePromiseTest001 getResultSet fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        }catch(e) {
            console.error('SingleKvStoreGetResultSizePromiseTest001 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreGetEntriesPromiseTest001
     * @tc.desc Test Js Api SingleKvStoreGetEntries testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreGetEntriesPromiseTest001', 0, async function (done) {
        console.info('SingleKvStoreGetEntriesPromiseTest001');
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
            console.info('SingleKvStoreGetEntriesPromiseTest001 entries: ' + JSON.stringify(entries));
            await kvStore.putBatch(entries).then(async (err) => {
                console.info('SingleKvStoreGetEntriesPromiseTest001 putBatch success');
                expect(err == undefined).assertTrue();
                var query = new factory.Query();
                query.prefixKey("batch_test");
                await kvStore.getEntries(query).then((entrys) => {
                    console.info('SingleKvStoreGetEntriesPromiseTest001 getEntries success');
                    expect(entrys.length == 10).assertTrue();
                    expect(entrys[0].value.value.toString() == arr.toString()).assertTrue();
                }).catch((err) => {
                    console.error('SingleKvStoreGetEntriesPromiseTest001 getEntries fail ' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(null).assertFail();
                });
            }).catch((err) => {
                console.error('SingleKvStoreGetEntriesPromiseTest001 putBatch fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
            console.info('SingleKvStoreGetEntriesPromiseTest001 success');
        }catch(e) {
            console.error('SingleKvStoreGetEntriesPromiseTest001 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreGetEntriesPromiseTest002
     * @tc.desc Test Js Api SingleKvStoreGetEntries testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreGetEntriesPromiseTest002', 0, async function (done) {
        console.info('SingleKvStoreGetEntriesPromiseTest002');
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
            console.info('SingleKvStoreGetEntriesPromiseTest002 entries: ' + JSON.stringify(entries));
            await kvStore.putBatch(entries).then(async (err) => {
                console.info('SingleKvStoreGetEntriesPromiseTest002 putBatch success');
                expect(err == undefined).assertTrue();
                var query = new factory.Query();
                query.prefixKey("batch_test");
                await kvStore.getEntries(query).then((entrys) => {
                    console.info('SingleKvStoreGetEntriesPromiseTest002 getEntries success');
                    expect(entrys.length == 10).assertTrue();
                    expect(entrys[0].value.value.toString() == arr.toString()).assertTrue();
                }).catch((err) => {
                    console.error('SingleKvStoreGetEntriesPromiseTest002 getEntries fail ' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(null).assertFail();
                });
            }).catch((err) => {
                console.error('SingleKvStoreGetEntriesPromiseTest002 putBatch fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
            console.info('SingleKvStoreGetEntriesPromiseTest002 success');
        }catch(e) {
            console.error('SingleKvStoreGetEntriesPromiseTest002 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreChangeNotificationPromiseTest001
     * @tc.desc Test Js Api SingleKvStoreChangeNotification testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
     it('SingleKvStoreChangeNotificationPromiseTest001', 0, async function (done) {
        var getInsertEntries = new Promise((resolve, reject) => {
            kvStore.on('dataChange', 0, function(ChangeNotification){
                resolve(ChangeNotification.insertEntries);
            });
            kvStore.put("getInsertEntries", "byPut").then(() => {
                console.info('SingleKvStoreChangeNotificationPromiseTest001 put success');
            });
        });
        await getInsertEntries.then(function(insertEntries){
            console.info('SingleKvStoreChangeNotificationPromiseTest001 getInsertEntries' + JSON.stringify(insertEntries));
            expect(insertEntries).assertNotNull();
        }).catch((error) => {
            console.error('SingleKvStoreChangeNotificationPromiseTest001 can NOT getInsertEntries, fail:' + `, error code is ${error.code}, message is ${error.message}`);
        });
        done();
    })

    /**
    * @tc.number SUB_DISTRIBUTEDDATAMGR_SINGLEKVSTORE_CHANGENOTIFICATION_0200
    * @tc.name [JS-API8]SingleKvStoreChangeNotification.
    * @tc.desc Test Js Api SingleKvStoreChangeNotification testcase 002
    */
    it('SingleKvStoreChangeNotificationPromiseTest002', 0, async function (done) {
        await kvStore.put('getUpdateEntries', 'byPut').then(() => {
                console.info('SingleKvStoreChangeNotificationPromiseTest002 put success');
        });
        var getUpdateEntries = new Promise((resolve, reject) => {
            kvStore.on('dataChange', 0, function(ChangeNotification){
                resolve(ChangeNotification.updateEntries);
            });
            kvStore.put("getUpdateEntries", "byPut").then(() => {
                console.info('SingleKvStoreChangeNotificationPromiseTest002 update success');
            });
        });
        await getUpdateEntries.then(function(updateEntries){
            console.info('SingleKvStoreChangeNotificationPromiseTest002 getUpdateEntries' + JSON.stringify(updateEntries));
            expect(updateEntries).assertNotNull();
        }).catch((error) => {
            console.error('SingleKvStoreChangeNotificationPromiseTest002 can NOT getUpdateEntries, fail:' + `, error code is ${error.code}, message is ${error.message}`);
        });
        done();
    })

    /**
    * @tc.number SUB_DISTRIBUTEDDATAMGR_SINGLEKVSTORE_CHANGENOTIFICATION_0300
    * @tc.name [JS-API8]SingleKvStoreChangeNotification.
    * @tc.desc Test Js Api SingleKvStoreChangeNotification testcase 003
    */
    it('SingleKvStoreChangeNotificationPromiseTest003', 0, async function (done) {
        await kvStore.put('deleteEntries', 'byPut').then(() => {
            console.info('SingleKvStoreChangeNotificationPromiseTest003 put success');
        });
        var getdeleteEntries = new Promise((resolve, reject) => {
            kvStore.on('dataChange', 0, function(ChangeNotification){
                resolve(ChangeNotification.deleteEntries);
            });
            kvStore.delete("deleteEntries").then(() => {
                console.info('SingleKvStoreChangeNotificationPromiseTest003 delete success');
            });
        });
        await getdeleteEntries.then(function(deleteEntries){
            console.info('SingleKvStoreChangeNotificationPromiseTest003 deleteEntries' + JSON.stringify(getdeleteEntries));
            expect(deleteEntries != null).assertNotNull();
        }).catch((error) => {
            console.error('SingleKvStoreChangeNotificationPromiseTest003 can NOT getdeleteEntries, fail:' + `, error code is ${error.code}, message is ${error.message}`);
        });
        done();
    })
})
