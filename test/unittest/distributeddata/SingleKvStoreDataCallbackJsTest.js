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

const KEY_TEST_INT_ELEMENT = 'key_test_int_2';
const KEY_TEST_FLOAT_ELEMENT = 'key_test_float_2';
const KEY_TEST_BOOLEAN_ELEMENT = 'key_test_boolean_2';
const KEY_TEST_STRING_ELEMENT = 'key_test_string_2';
const KEY_TEST_SYNC_ELEMENT = 'key_test_sync';

const VALUE_TEST_INT_ELEMENT = 1234;
const VALUE_TEST_FLOAT_ELEMENT = 4321.12;
const VALUE_TEST_BOOLEAN_ELEMENT = true;
const VALUE_TEST_STRING_ELEMENT = 'value-string-002';
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

describe('singleKvStoreCallbackTest', function () {
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
        await factory.createKVManager(config, function (err, manager) {
            kvManager = manager;
            console.info('beforeAll createKVManager success');
            done();
        })
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
        await kvManager.closeKVStore(TEST_BUNDLE_NAME, TEST_STORE_ID, kvStore, async function (err, data) {
            console.info('afterEach closeKVStore success');
            await kvManager.deleteKVStore(TEST_BUNDLE_NAME, TEST_STORE_ID, function (err, data) {
                console.info('afterEach deleteKVStore success');
                done();
            });
        });
        kvStore = null;
    })

    /**
     * @tc.name SingleKvStorePutStringCallbackTest001
     * @tc.desc Test Js Api SingleKvStore.Put(String) testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStorePutStringCallbackTest001', 0, async function (done) {
        console.info('SingleKvStorePutStringCallbackTest001');
        try {
            await kvStore.put(KEY_TEST_STRING_ELEMENT, VALUE_TEST_STRING_ELEMENT, function (err,data) {
                if (err == undefined) {
                    console.info('SingleKvStorePutStringCallbackTest001 put success');
                } else {
                    console.error('SingleKvStorePutStringCallbackTest001 put fail' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(null).assertFail();
                }
                done();
            });
        }catch (e) {
            console.error('SingleKvStorePutStringCallbackTest001 put e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name SingleKvStorePutStringCallbackTest002
     * @tc.desc Test Js Api SingleKvStore.Put(String) testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStorePutStringCallbackTest002', 0, async function (done) {
        console.info('SingleKvStorePutStringCallbackTest002');
        try {
            var str = '';
            for (var i = 0 ; i < 4095; i++) {
                str += 'x';
            }
            await kvStore.put(KEY_TEST_STRING_ELEMENT+'102', str, async function (err,data) {
                console.info('SingleKvStorePutStringCallbackTest002 put success');
                expect(err == undefined).assertTrue();
                await kvStore.get(KEY_TEST_STRING_ELEMENT+'102', function (err,data) {
                    console.info('SingleKvStorePutStringCallbackTest002 get success');
                    expect(str == data).assertTrue();
                    done();
                });
            });
        }catch (e) {
            console.error('SingleKvStorePutStringCallbackTest002 put e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name SingleKvStoreGetStringCallbackTest001
     * @tc.desc Test Js Api SingleKvStore.GetString() testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreGetStringCallbackTest001', 0, async function (done) {
        console.info('SingleKvStoreGetStringCallbackTest001');
        try{
            await kvStore.get(KEY_TEST_STRING_ELEMENT, function (err,data) {
                if (err == undefined) {
                    console.info('SingleKvStoreGetStringCallbackTest001 get success');
                    expect(null).assertFail();
                } else {
                    console.info('SingleKvStoreGetStringCallbackTest001 get fail');
                }
                done();
            });
        }catch(e) {
            console.error('SingleKvStoreGetStringCallbackTest001 get e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name SingleKvStoreGetStringCallbackTest002
     * @tc.desc Test Js Api SingleKvStore.GetString() testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreGetStringCallbackTest002', 0, async function (done) {
        console.info('SingleKvStoreGetStringCallbackTest002');
        try{
            await kvStore.put(KEY_TEST_STRING_ELEMENT, VALUE_TEST_STRING_ELEMENT, async function (err,data) {
                console.info('SingleKvStoreGetStringCallbackTest002 put success');
                expect(err == undefined).assertTrue();
                await kvStore.get(KEY_TEST_STRING_ELEMENT, function (err,data) {
                    console.info('SingleKvStoreGetStringCallbackTest002 get success');
                    expect((err == undefined) && (VALUE_TEST_STRING_ELEMENT == data)).assertTrue();
                    done();
                });
            })
        }catch(e) {
            console.error('SingleKvStoreGetStringCallbackTest002 get e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name SingleKvStorePutIntCallbackTest001
     * @tc.desc Test Js Api SingleKvStore.Put(Int) testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStorePutIntCallbackTest001', 0, async function (done) {
        console.info('SingleKvStorePutIntCallbackTest001');
        try {
            await kvStore.put(KEY_TEST_INT_ELEMENT, VALUE_TEST_INT_ELEMENT, async function (err,data) {
                console.info('SingleKvStorePutIntCallbackTest001 put success');
                expect(err == undefined).assertTrue();
                await kvStore.get(KEY_TEST_INT_ELEMENT, function (err,data) {
                    console.info('SingleKvStorePutIntCallbackTest001 get success');
                    expect((err == undefined) && (VALUE_TEST_INT_ELEMENT == data)).assertTrue();
                    done();
                })
            });
        }catch(e) {
            console.error('SingleKvStorePutIntCallbackTest001 put e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name SingleKvStorePutIntCallbackTest002
     * @tc.desc Test Js Api SingleKvStore.Put(Int) testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStorePutIntCallbackTest002', 0, async function (done) {
        console.info('SingleKvStorePutIntCallbackTest002');
        try {
            var intValue = 987654321;
            await kvStore.put(KEY_TEST_INT_ELEMENT, intValue, async function (err,data) {
                console.info('SingleKvStorePutIntCallbackTest002 put success');
                expect(err == undefined).assertTrue();
                await kvStore.get(KEY_TEST_INT_ELEMENT, function (err,data) {
                    console.info('SingleKvStorePutIntCallbackTest002 get success');
                    expect((err == undefined) && (intValue == data)).assertTrue();
                    done();
                })
            });
        }catch(e) {
            console.error('SingleKvStorePutIntCallbackTest002 put e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name SingleKvStorePutIntCallbackTest003
     * @tc.desc Test Js Api SingleKvStore.Put(Int) testcase 003
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStorePutIntCallbackTest003', 0, async function (done) {
        console.info('SingleKvStorePutIntCallbackTest003');
        try {
            var intValue = Number.MIN_VALUE;
            await kvStore.put(KEY_TEST_INT_ELEMENT, intValue, async function (err,data) {
                console.info('SingleKvStorePutIntCallbackTest003 put success');
                expect(err == undefined).assertTrue();
                await kvStore.get(KEY_TEST_INT_ELEMENT, function (err,data) {
                    console.info('SingleKvStorePutIntCallbackTest003 get success');
                    expect((err == undefined) && (intValue == data)).assertTrue();
                    done();
                })
            });
        }catch(e) {
            console.error('SingleKvStorePutIntCallbackTest003 put e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name SingleKvStorePutIntCallbackTest004
     * @tc.desc Test Js Api SingleKvStore.Put(Int) testcase 004
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStorePutIntCallbackTest004', 0, async function (done) {
        console.info('SingleKvStorePutIntCallbackTest004');
        try {
            var intValue = Number.MAX_VALUE;
            await kvStore.put(KEY_TEST_INT_ELEMENT, intValue, async function (err,data) {
                console.info('SingleKvStorePutIntCallbackTest004 put success');
                expect(err == undefined).assertTrue();
                await kvStore.get(KEY_TEST_INT_ELEMENT, function (err,data) {
                    console.info('SingleKvStorePutIntCallbackTest004 get success');
                    expect((err == undefined) && (intValue == data)).assertTrue();
                    done();
                })
            });
        }catch(e) {
            console.error('SingleKvStorePutIntCallbackTest004 put e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name SingleKvStoreGetIntCallbackTest001
     * @tc.desc Test Js Api SingleKvStore.GetInt() testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreGetIntCallbackTest001', 0, async function (done) {
        console.info('SingleKvStoreGetIntCallbackTest001');
        try {
            await kvStore.put(KEY_TEST_INT_ELEMENT, VALUE_TEST_INT_ELEMENT, async function (err,data) {
                console.info('SingleKvStoreGetIntCallbackTest001 put success');
                expect(err == undefined).assertTrue();
                await kvStore.get(KEY_TEST_INT_ELEMENT, function (err,data) {
                    console.info('SingleKvStoreGetIntCallbackTest001 get success');
                    expect((err == undefined) && (VALUE_TEST_INT_ELEMENT == data)).assertTrue();
                    done();
                })
            });
        }catch(e) {
            console.error('SingleKvStoreGetIntCallbackTest001 put e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name SingleKvStoreGetIntCallbackTest002
     * @tc.desc Test Js Api SingleKvStore.GetInt() testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreGetIntCallbackTest002', 0, async function (done) {
        console.info('SingleKvStoreGetIntCallbackTest002');
        try {
            await kvStore.get(KEY_TEST_INT_ELEMENT, function (err,data) {
                if (err == undefined) {
                    console.info('SingleKvStoreGetIntCallbackTest002 get success');
                    expect(null).assertFail();
                } else {
                    console.info('SingleKvStoreGetIntCallbackTest002 get fail');
                }
                done();
            })
        }catch(e) {
            console.error('SingleKvStoreGetIntCallbackTest002 put e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name SingleKvStorePutBoolCallbackTest001
     * @tc.desc Test Js Api SingleKvStore.Put(Bool) testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStorePutBoolCallbackTest001', 0, async function (done) {
        console.info('SingleKvStorePutBoolCallbackTest001');
        try {
            await kvStore.put(KEY_TEST_BOOLEAN_ELEMENT, VALUE_TEST_BOOLEAN_ELEMENT, function (err,data) {
                console.info('SingleKvStorePutBoolCallbackTest001 put success');
                expect(err == undefined).assertTrue();
                done();
            });
        }catch(e) {
            console.error('SingleKvStorePutBoolCallbackTest001 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name SingleKvStoreGetBoolCallbackTest001
     * @tc.desc Test Js Api SingleKvStore.GetBool() testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreGetBoolCallbackTest001', 0, async function (done) {
        console.info('SingleKvStoreGetBoolCallbackTest001');
        try {
            await kvStore.get(KEY_TEST_BOOLEAN_ELEMENT, function (err,data) {
                if (err == undefined) {
                    console.info('SingleKvStoreGetBoolCallbackTest001 get success');
                    expect(null).assertFail();
                } else {
                    console.error('SingleKvStoreGetBoolCallbackTest001 get fail' + `, error code is ${err.code}, message is ${err.message}`);
                }
                done();
            });
        }catch(e) {
            console.error('SingleKvStoreGetBoolCallbackTest001 e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name SingleKvStoreGetBoolCallbackTest002
     * @tc.desc Test Js Api SingleKvStore.GetBool() testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreGetBoolCallbackTest002', 0, async function (done) {
        console.info('SingleKvStoreGetBoolCallbackTest002');
        try {
            await kvStore.put(KEY_TEST_BOOLEAN_ELEMENT, VALUE_TEST_BOOLEAN_ELEMENT, async function (err, data) {
                console.info('SingleKvStoreGetBoolCallbackTest002 put success');
                expect(err == undefined).assertTrue();
                await kvStore.get(KEY_TEST_BOOLEAN_ELEMENT, function (err,data) {
                    console.info('SingleKvStoreGetBoolCallbackTest002 get success');
                    expect((err == undefined) && (VALUE_TEST_BOOLEAN_ELEMENT == data)).assertTrue();
                    done();
                });
            })
        }catch(e) {
            console.error('SingleKvStoreGetBoolCallbackTest002 e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name SingleKvStorePutFloatCallbackTest001
     * @tc.desc Test Js Api SingleKvStore.Put(Float) testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStorePutFloatCallbackTest001', 0, async function (done) {
        console.info('SingleKvStorePutFloatCallbackTest001');
        try {
            await kvStore.put(KEY_TEST_FLOAT_ELEMENT, VALUE_TEST_FLOAT_ELEMENT, function (err,data) {
                console.info('SingleKvStorePutFloatCallbackTest001 put success');
                expect(err == undefined).assertTrue();
                done();
            });
        }catch(e) {
            console.error('SingleKvStorePutFloatCallbackTest001 e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name SingleKvStorePutFloatCallbackTest002
     * @tc.desc Test Js Api SingleKvStore.Put(Float) testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStorePutFloatCallbackTest002', 0, async function (done) {
        console.info('SingleKvStorePutFloatCallbackTest002');
        try {
            var floatValue = 123456.654321;
            await kvStore.put(KEY_TEST_FLOAT_ELEMENT, floatValue, async function (err,data) {
                console.info('SingleKvStorePutFloatCallbackTest002 put success');
                expect(err == undefined).assertTrue();
                await kvStore.get(KEY_TEST_FLOAT_ELEMENT, function (err, data) {
                    console.info('SingleKvStorePutFloatCallbackTest002 get success');
                    expect((err == undefined) && (floatValue == data)).assertTrue();
                    done();
                })
                done();
            });
        }catch(e) {
            console.error('SingleKvStorePutFloatCallbackTest002 e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name SingleKvStorePutFloatCallbackTest003
     * @tc.desc Test Js Api SingleKvStore.Put(Float) testcase 003
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStorePutFloatCallbackTest003', 0, async function (done) {
        console.info('SingleKvStorePutFloatCallbackTest003');
        try {
            var floatValue = 123456.0;
            await kvStore.put(KEY_TEST_FLOAT_ELEMENT, floatValue, async function (err,data) {
                console.info('SingleKvStorePutFloatCallbackTest003 put success');
                expect(err == undefined).assertTrue();
                await kvStore.get(KEY_TEST_FLOAT_ELEMENT, function (err, data) {
                    console.info('SingleKvStorePutFloatCallbackTest003 get success');
                    expect((err == undefined) && (floatValue == data)).assertTrue();
                    done();
                })
                done();
            });
        }catch(e) {
            console.error('SingleKvStorePutFloatCallbackTest003 e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name SingleKvStorePutFloatCallbackTest004
     * @tc.desc Test Js Api SingleKvStore.Put(Float) testcase 004
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStorePutFloatCallbackTest004', 0, async function (done) {
        console.info('SingleKvStorePutFloatCallbackTest004');
        try {
            var floatValue = 123456.00;
            await kvStore.put(KEY_TEST_FLOAT_ELEMENT, floatValue, async function (err,data) {
                console.info('SingleKvStorePutFloatCallbackTest004 put success');
                expect(err == undefined).assertTrue();
                await kvStore.get(KEY_TEST_FLOAT_ELEMENT, function (err, data) {
                    console.info('SingleKvStorePutFloatCallbackTest004 get success');
                    expect((err == undefined) && (floatValue == data)).assertTrue();
                    done();
                })
                done();
            });
        }catch(e) {
            console.error('SingleKvStorePutFloatCallbackTest004 e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name SingleKvStoreGetFloatCallbackTest001
     * @tc.desc Test Js Api SingleKvStore.GetFloat() testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreGetFloatCallbackTest001', 0, async function (done) {
        console.info('SingleKvStoreGetFloatCallbackTest001');
        try {
            await kvStore.get(KEY_TEST_FLOAT_ELEMENT, function (err,data) {
                if (err == undefined) {
                    console.info('SingleKvStoreGetFloatCallbackTest001 get success');
                    expect(null).assertFail();
                } else {
                    console.error('SingleKvStoreGetFloatCallbackTest001 get fail' + `, error code is ${err.code}, message is ${err.message}`);
                }
                done();
            });
        }catch(e) {
            console.error('SingleKvStoreGetFloatCallbackTest001 e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name SingleKvStoreDeleteStringCallbackTest001
     * @tc.desc Test Js Api SingleKvStore.DeleteString() testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreDeleteStringCallbackTest001', 0, async function (done) {
        console.info('SingleKvStoreDeleteStringCallbackTest001');
        try {
            await kvStore.delete(KEY_TEST_STRING_ELEMENT, function (err,data) {
                if (err == undefined) {
                    console.info('SingleKvStoreDeleteStringCallbackTest001 delete success');
                } else {
                    console.error('SingleKvStoreDeleteStringCallbackTest001 delete fail' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(null).assertFail();
                }
                done();
            });
        }catch(e) {
            console.error('SingleKvStoreDeleteStringCallbackTest001 e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name SingleKvStoreDeleteStringCallbackTest002
     * @tc.desc Test Js Api SingleKvStore.DeleteString() testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreDeleteStringCallbackTest002', 0, async function (done) {
        console.info('SingleKvStoreDeleteStringCallbackTest002');
        try {
            await kvStore.put(KEY_TEST_STRING_ELEMENT, VALUE_TEST_STRING_ELEMENT, async function (err, data) {
                console.info('SingleKvStoreDeleteStringCallbackTest002 put success');
                expect(err == undefined).assertTrue();
                await kvStore.delete(KEY_TEST_STRING_ELEMENT, function (err,data) {
                    console.info('SingleKvStoreDeleteStringCallbackTest002 delete success');
                    expect(err == undefined).assertTrue();
                    done();
                });
            })
        }catch(e) {
            console.error('SingleKvStoreDeleteStringCallbackTest002 e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name SingleKvStoreDeleteIntCallbackTest001
     * @tc.desc Test Js Api SingleKvStore.DeleteInt() testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreDeleteIntCallbackTest001', 0, async function (done) {
        console.info('SingleKvStoreDeleteIntCallbackTest001');
        try{
            await kvStore.delete(KEY_TEST_INT_ELEMENT, function (err,data) {
                if (err == undefined) {
                    console.info('SingleKvStoreDeleteIntCallbackTest001 get success');
                } else {
                    console.error('SingleKvStoreDeleteIntCallbackTest001 get fail' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(null).assertFail();
                }
                done();
            });
        }catch(e) {
            console.error('SingleKvStoreDeleteIntCallbackTest001 e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name SingleKvStoreDeleteIntCallbackTest002
     * @tc.desc Test Js Api SingleKvStore.DeleteInt() testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreDeleteIntCallbackTest002', 0, async function (done) {
        console.info('SingleKvStoreDeleteIntCallbackTest002');
        try{
            await kvStore.put(KEY_TEST_INT_ELEMENT, VALUE_TEST_INT_ELEMENT, async function (err,data) {
                console.info('SingleKvStoreDeleteIntCallbackTest002 put success');
                expect(err == undefined).assertTrue();
                await kvStore.delete(KEY_TEST_INT_ELEMENT, function (err,data) {
                    console.info('SingleKvStoreDeleteIntCallbackTest002 delete success');
                    expect(err == undefined).assertTrue();
                    done();
                });
            })
        }catch(e) {
            console.error('SingleKvStoreDeleteIntCallbackTest002 e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name SingleKvStoreDeleteFloatCallbackTest001
     * @tc.desc Test Js Api SingleKvStore.DeleteFloat() testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreDeleteFloatCallbackTest001', 0, async function (done) {
        console.info('SingleKvStoreDeleteFloatCallbackTest001');
        try{
            await kvStore.delete(KEY_TEST_FLOAT_ELEMENT, function (err,data) {
                if (err == undefined) {
                    console.info('SingleKvStoreDeleteFloatCallbackTest001 get success');
                } else {
                    console.error('SingleKvStoreDeleteFloatCallbackTest001 get fail' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(null).assertFail();
                }
                done();
            });
        }catch(e) {
            console.error('SingleKvStoreDeleteFloatCallbackTest001 e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name SingleKvStoreDeleteFloatCallbackTest002
     * @tc.desc Test Js Api SingleKvStore.DeleteFloat() testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreDeleteFloatCallbackTest002', 0, async function (done) {
        console.info('SingleKvStoreDeleteFloatCallbackTest002');
        try{
            await kvStore.put(KEY_TEST_FLOAT_ELEMENT, VALUE_TEST_FLOAT_ELEMENT, async function (err, data) {
                console.info('SingleKvStoreDeleteFloatCallbackTest002 put success');
                expect(err == undefined).assertTrue();
                await kvStore.delete(KEY_TEST_FLOAT_ELEMENT, function (err,data) {
                    console.info('SingleKvStoreDeleteFloatCallbackTest002 delete success');
                    expect(err == undefined).assertTrue();
                    done();
                });
            })
        }catch(e) {
            console.error('SingleKvStoreDeleteFloatCallbackTest002 e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name SingleKvStoreDeleteBoolCallbackTest001
     * @tc.desc Test Js Api SingleKvStore.DeleteBool() testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreDeleteBoolCallbackTest001', 0, async function (done) {
        console.info('SingleKvStoreDeleteBoolCallbackTest001');
        try{
            await kvStore.delete(KEY_TEST_BOOLEAN_ELEMENT, function (err,data) {
                if (err == undefined) {
                    console.info('SingleKvStoreDeleteBoolCallbackTest001 get success');
                } else {
                    console.error('SingleKvStoreDeleteBoolCallbackTest001 get fail' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(null).assertFail();
                }
                done();
            });
        }catch(e) {
            console.error('SingleKvStoreDeleteBoolCallbackTest001 e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name SingleKvStoreDeleteBoolCallbackTest002
     * @tc.desc Test Js Api SingleKvStore.DeleteBool() testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreDeleteBoolCallbackTest002', 0, async function (done) {
        console.info('SingleKvStoreDeleteBoolCallbackTest002');
        try{
            await kvStore.put(KEY_TEST_BOOLEAN_ELEMENT, VALUE_TEST_BOOLEAN_ELEMENT, async function (err, data) {
                console.info('SingleKvStoreDeleteBoolCallbackTest002 put success');
                expect(err == undefined).assertTrue();
                await kvStore.delete(KEY_TEST_BOOLEAN_ELEMENT, function (err,data) {
                    console.info('SingleKvStoreDeleteBoolCallbackTest002 delete success');
                    expect(err == undefined).assertTrue();
                    done();
                });
            })
        }catch(e) {
            console.error('SingleKvStoreDeleteBoolCallbackTest002 e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name SingleKvStoreOnChangeCallbackTest001
     * @tc.desc Test Js Api SingleKvStore.OnChange() testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreOnChangeCallbackTest001', 0, async function (done) {
        console.info('SingleKvStoreOnChangeCallbackTest001');
        try {
            kvStore.on('dataChange', 0, function (data) {
                console.info('SingleKvStoreOnChangeCallbackTest001 dataChange');
                expect(data != null).assertTrue();
            });
            await kvStore.put(KEY_TEST_FLOAT_ELEMENT, VALUE_TEST_FLOAT_ELEMENT, function (err,data) {
                console.info('SingleKvStoreOnChangeCallbackTest001 put success');
                expect(err == undefined).assertTrue();
                done();
            });
        }catch(e) {
            console.error('SingleKvStoreOnChangeCallbackTest001 e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name SingleKvStoreOnChangeCallbackTest002
     * @tc.desc Test Js Api SingleKvStore.OnChange() testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreOnChangeCallbackTest002', 0, async function (done) {
        console.info('SingleKvStoreOnChangeCallbackTest002');
        try {
            kvStore.on('dataChange', 1, function (data) {
                console.info('SingleKvStoreOnChangeCallbackTest002 dataChange');
                expect(data != null).assertTrue();
            });
            await kvStore.put(KEY_TEST_FLOAT_ELEMENT, VALUE_TEST_FLOAT_ELEMENT, function (err,data) {
                console.info('SingleKvStoreOnChangeCallbackTest002 put success');
                expect(err == undefined).assertTrue();
                done();
            });
        }catch(e) {
            console.error('SingleKvStoreOnChangeCallbackTest002 e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name SingleKvStoreOnChangeCallbackTest003
     * @tc.desc Test Js Api SingleKvStore.OnChange() testcase 003
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreOnChangeCallbackTest003', 0, async function (done) {
        console.info('SingleKvStoreOnChangeCallbackTest003');
        try {
            kvStore.on('dataChange', 2, function (data) {
                console.info('SingleKvStoreOnChangeCallbackTest003 dataChange');
                expect(data != null).assertTrue();
            });
            await kvStore.put(KEY_TEST_FLOAT_ELEMENT, VALUE_TEST_FLOAT_ELEMENT, function (err,data) {
                console.info('SingleKvStoreOnChangeCallbackTest003 put success');
                expect(err == undefined).assertTrue();
                done();
            });
        }catch(e) {
            console.error('SingleKvStoreOnChangeCallbackTest003 e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name SingleKvStoreOnSyncCompleteCallbackTest001
     * @tc.desc Test Js Api SingleKvStore.OnSyncComplete() testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreOnSyncCompleteCallbackTest001', 0, async function (done) {
        try {
            kvStore.on('syncComplete', function (data) {
                console.info('SingleKvStoreOnSyncCompleteCallbackTest001 dataChange');
                expect(data != null).assertTrue();
            });
            await kvStore.put(KEY_TEST_SYNC_ELEMENT + 'SyncTest001', VALUE_TEST_SYNC_ELEMENT, function (err,data) {
                console.info('SingleKvStoreOnSyncCompleteCallbackTest001 put success');
                expect(err == undefined).assertTrue();
            });
            try {
                var mode = factory.SyncMode.PULL_ONLY;
                console.info('kvStore.sync to ' + JSON.stringify(syncDeviceIds));
                kvStore.sync(syncDeviceIds, mode, 10);
            } catch (e) {
                console.error('SingleKvStoreOnSyncCompleteCallbackTest001 sync no peer device :e:' + `, error code is ${e.code}, message is ${e.message}`);
            }
        }catch(e) {
            console.error('SingleKvStoreOnSyncCompleteCallbackTest001 e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreOnSyncCompleteCallbackTest002
     * @tc.desc Test Js Api SingleKvStore.OnSyncComplete() testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreOnSyncCompleteCallbackTest002', 0, async function (done) {
        try {
            kvStore.on('syncComplete', function (data) {
                console.info('SingleKvStoreOnSyncCompleteCallbackTest002 dataChange');
                expect(data != null).assertTrue();
            });
            await kvStore.put(KEY_TEST_SYNC_ELEMENT + 'SyncTest002', VALUE_TEST_SYNC_ELEMENT, function (err,data) {
                console.info('SingleKvStoreOnSyncCompleteCallbackTest002 put success');
                expect(err == undefined).assertTrue();
            });
            try {
                var mode = factory.SyncMode.PUSH_ONLY;
                console.info('kvStore.sync to ' + JSON.stringify(syncDeviceIds));
                kvStore.sync(syncDeviceIds, mode, 10);
            } catch (e) {
                console.error('SingleKvStoreOnSyncCompleteCallbackTest002 sync no peer device :e:' + `, error code is ${e.code}, message is ${e.message}`);
            }
        }catch(e) {
            console.error('SingleKvStoreOnSyncCompleteCallbackTest002 e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreOnSyncCompleteCallbackTest003
     * @tc.desc Test Js Api SingleKvStore.OnSyncComplete() testcase 003
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreOnSyncCompleteCallbackTest003', 0, async function (done) {
        try {
            kvStore.on('syncComplete', function (data) {
                console.info('SingleKvStoreOnSyncCompleteCallbackTest003 dataChange');
                expect(data != null).assertTrue();
            });
            await kvStore.put(KEY_TEST_SYNC_ELEMENT + 'SyncTest003', VALUE_TEST_SYNC_ELEMENT, function (err,data) {
                console.info('SingleKvStoreOnSyncCompleteCallbackTest003 put success');
                expect(err == undefined).assertTrue();
            });
            try {
                var mode = factory.SyncMode.PUSH_PULL;
                console.info('kvStore.sync to ' + JSON.stringify(syncDeviceIds));
                kvStore.sync(syncDeviceIds, mode, 10);
            } catch (e) {
                console.error('SingleKvStoreOnSyncCompleteCallbackTest003 sync no peer device :e:' + `, error code is ${e.code}, message is ${e.message}`);
            }
        }catch(e) {
            console.error('SingleKvStoreOnSyncCompleteCallbackTest003 e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreSetSyncRangeCallbackTest001
     * @tc.desc Test Js Api SingleKvStore.SetSyncRange() testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreSetSyncRangeCallbackTest001', 0, async function (done) {
        console.info('SingleKvStoreSetSyncRangeCallbackTest001');
        try {
            var localLabels = ['A', 'B'];
            var remoteSupportLabels = ['C', 'D'];
            await kvStore.setSyncRange(localLabels, remoteSupportLabels, function (err,data) {
                console.info('SingleKvStoreSetSyncRangeCallbackTest001 put success');
                expect(err == undefined).assertTrue();
                done();
            });
        }catch(e) {
            console.error('SingleKvStoreSetSyncRangeCallbackTest001 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name SingleKvStoreSetSyncRangeCallbackTest002
     * @tc.desc Test Js Api SingleKvStore.SetSyncRange() testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreSetSyncRangeCallbackTest002', 0, async function (done) {
        console.info('SingleKvStoreSetSyncRangeCallbackTest002');
        try {
            var localLabels = ['A', 'B'];
            var remoteSupportLabels = ['B', 'C'];
            await kvStore.setSyncRange(localLabels, remoteSupportLabels, function (err,data) {
                console.info('SingleKvStoreSetSyncRangeCallbackTest002 put success');
                expect(err == undefined).assertTrue();
                done();
            });
        }catch(e) {
            console.error('SingleKvStoreSetSyncRangeCallbackTest002 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name SingleKvStoreSetSyncRangeCallbackTest003
     * @tc.desc Test Js Api SingleKvStore.SetSyncRange() testcase 003
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreSetSyncRangeCallbackTest003', 0, async function (done) {
        console.info('SingleKvStoreSetSyncRangeCallbackTest003');
        try {
            var localLabels = ['A', 'B'];
            var remoteSupportLabels = ['A', 'B'];
            await kvStore.setSyncRange(localLabels, remoteSupportLabels, function (err,data) {
                console.info('SingleKvStoreSetSyncRangeCallbackTest003 put success');
                expect(err == undefined).assertTrue();
                done();
            });
        }catch(e) {
            console.error('SingleKvStoreSetSyncRangeCallbackTest003 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name SingleKvStorePutBatchCallbackTest001
     * @tc.desc Test Js Api SingleKvStore.PutBatch() testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStorePutBatchCallbackTest001', 0, async function (done) {
        console.info('SingleKvStorePutBatchCallbackTest001');
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
            console.info('SingleKvStorePutBatchCallbackTest001 entries: ' + JSON.stringify(entries));
            await kvStore.putBatch(entries, async function (err,data) {
                console.info('SingleKvStorePutBatchCallbackTest001 putBatch success');
                expect(err == undefined).assertTrue();
                await kvStore.getEntries('batch_test_string_key', function (err,entrys) {
                    console.info('SingleKvStorePutBatchCallbackTest001 getEntries success');
                    console.info('SingleKvStorePutBatchCallbackTest001 entrys.length: ' + entrys.length);
                    console.info('SingleKvStorePutBatchCallbackTest001 entrys[0]: ' + JSON.stringify(entrys[0]));
                    expect(entrys.length == 10).assertTrue();
                    expect(entrys[0].value.value == 'batch_test_string_value').assertTrue();
                    done();
                });
            });
        }catch(e) {
            console.error('SingleKvStorePutBatchCallbackTest001 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name SingleKvStorePutBatchCallbackTest002
     * @tc.desc Test Js Api SingleKvStore.PutBatch() testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStorePutBatchCallbackTest002', 0, async function (done) {
        console.info('SingleKvStorePutBatchCallbackTest002');
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
            console.info('SingleKvStorePutBatchCallbackTest002 entries: ' + JSON.stringify(entries));
            await kvStore.putBatch(entries, async function (err,data) {
                console.info('SingleKvStorePutBatchCallbackTest002 putBatch success');
                expect(err == undefined).assertTrue();
                await kvStore.getEntries('batch_test_number_key', function (err,entrys) {
                    console.info('SingleKvStorePutBatchCallbackTest002 getEntries success');
                    console.info('SingleKvStorePutBatchCallbackTest002 entrys.length: ' + entrys.length);
                    console.info('SingleKvStorePutBatchCallbackTest002 entrys[0]: ' + JSON.stringify(entrys[0]));
                    expect(entrys.length == 10).assertTrue();
                    expect(entrys[0].value.value == 222).assertTrue();
                    done();
                });
            });
        }catch(e) {
            console.error('SingleKvStorePutBatchCallbackTest002 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name SingleKvStorePutBatchCallbackTest003
     * @tc.desc Test Js Api SingleKvStore.PutBatch() testcase 003
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStorePutBatchCallbackTest003', 0, async function (done) {
        console.info('SingleKvStorePutBatchCallbackTest003');
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
            console.info('SingleKvStorePutBatchCallbackTest003 entries: ' + JSON.stringify(entries));
            await kvStore.putBatch(entries, async function (err,data) {
                console.info('SingleKvStorePutBatchCallbackTest003 putBatch success');
                expect(err == undefined).assertTrue();
                await kvStore.getEntries('batch_test_number_key', function (err,entrys) {
                    console.info('SingleKvStorePutBatchCallbackTest003 getEntries success');
                    console.info('SingleKvStorePutBatchCallbackTest003 entrys.length: ' + entrys.length);
                    console.info('SingleKvStorePutBatchCallbackTest003 entrys[0]: ' + JSON.stringify(entrys[0]));
                    expect(entrys.length == 10).assertTrue();
                    expect(entrys[0].value.value == 2.0).assertTrue();
                    done();
                });
            });
        }catch(e) {
            console.error('SingleKvStorePutBatchCallbackTest003 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name SingleKvStorePutBatchCallbackTest004
     * @tc.desc Test Js Api SingleKvStore.PutBatch() testcase 004
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStorePutBatchCallbackTest004', 0, async function (done) {
        console.info('SingleKvStorePutBatchCallbackTest004');
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
            console.info('SingleKvStorePutBatchCallbackTest004 entries: ' + JSON.stringify(entries));
            await kvStore.putBatch(entries, async function (err,data) {
                console.info('SingleKvStorePutBatchCallbackTest004 putBatch success');
                expect(err == undefined).assertTrue();
                await kvStore.getEntries('batch_test_number_key', function (err,entrys) {
                    console.info('SingleKvStorePutBatchCallbackTest004 getEntries success');
                    console.info('SingleKvStorePutBatchCallbackTest004 entrys.length: ' + entrys.length);
                    console.info('SingleKvStorePutBatchCallbackTest004 entrys[0]: ' + JSON.stringify(entrys[0]));
                    expect(entrys.length == 10).assertTrue();
                    expect(entrys[0].value.value == 2.00).assertTrue();
                    done();
                });
            });
        }catch(e) {
            console.error('SingleKvStorePutBatchCallbackTest004 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name SingleKvStorePutBatchCallbackTest005
     * @tc.desc Test Js Api SingleKvStore.PutBatch() testcase 005
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStorePutBatchCallbackTest005', 0, async function (done) {
        console.info('SingleKvStorePutBatchCallbackTest005');
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
            console.info('SingleKvStorePutBatchCallbackTest005 entries: ' + JSON.stringify(entries));
            await kvStore.putBatch(entries, async function (err,data) {
                console.info('SingleKvStorePutBatchCallbackTest005 putBatch success');
                expect(err == undefined).assertTrue();
                await kvStore.getEntries('batch_test_bool_key', function (err,entrys) {
                    console.info('SingleKvStorePutBatchCallbackTest005 getEntries success');
                    console.info('SingleKvStorePutBatchCallbackTest005 entrys.length: ' + entrys.length);
                    console.info('SingleKvStorePutBatchCallbackTest005 entrys[0]: ' + JSON.stringify(entrys[0]));
                    expect(entrys.length == 10).assertTrue();
                    expect(entrys[0].value.value == bo).assertTrue();
                    done();
                });
            });
        }catch(e) {
            console.error('SingleKvStorePutBatchCallbackTest005 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name SingleKvStorePutBatchCallbackTest006
     * @tc.desc Test Js Api SingleKvStore.PutBatch() testcase 006
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStorePutBatchCallbackTest006', 0, async function (done) {
        console.info('SingleKvStorePutBatchCallbackTest006');
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
            console.info('SingleKvStorePutBatchCallbackTest006 entries: ' + JSON.stringify(entries));
            await kvStore.putBatch(entries, async function (err,data) {
                console.info('SingleKvStorePutBatchCallbackTest006 putBatch success');
                expect(err == undefined).assertTrue();
                await kvStore.getEntries('batch_test_bool_key', function (err,entrys) {
                    console.info('SingleKvStorePutBatchCallbackTest006 getEntries success');
                    console.info('SingleKvStorePutBatchCallbackTest006 entrys.length: ' + entrys.length);
                    console.info('SingleKvStorePutBatchCallbackTest006 entrys[0]: ' + JSON.stringify(entrys[0]));
                    expect(entrys.length == 10).assertTrue();
                    expect(entrys[0].value.value.toString() == arr.toString()).assertTrue();
                    done();
                });
            });
        }catch(e) {
            console.error('SingleKvStorePutBatchCallbackTest006 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name SingleKvStoreDeleteBatchCallbackTest001
     * @tc.desc Test Js Api SingleKvStore.DeleteBatch() testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreDeleteBatchCallbackTest001', 0, async function (done) {
        console.info('SingleKvStoreDeleteBatchCallbackTest001');
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
            console.info('SingleKvStoreDeleteBatchCallbackTest001 entries: ' + JSON.stringify(entries));
            await kvStore.putBatch(entries, async function (err,data) {
                console.info('SingleKvStoreDeleteBatchCallbackTest001 putBatch success');
                expect(err == undefined).assertTrue();
                await kvStore.deleteBatch(keys, async function (err,data) {
                    console.info('SingleKvStoreDeleteBatchCallbackTest001 deleteBatch success');
                    expect(err == undefined).assertTrue();
                    done();
                });
            });
        }catch(e) {
            console.error('SingleKvStoreDeleteBatchCallbackTest001 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name SingleKvStoreDeleteBatchCallbackTest002
     * @tc.desc Test Js Api SingleKvStore.DeleteBatch() testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreDeleteBatchCallbackTest002', 0, async function (done) {
        console.info('SingleKvStoreDeleteBatchCallbackTest002');
        try {
            let keys = ['batch_test_string_key1', 'batch_test_string_key2'];
            await kvStore.deleteBatch(keys, function (err,data) {
                console.info('SingleKvStoreDeleteBatchCallbackTest002 deleteBatch success');
                expect(err == undefined).assertTrue();
                done();
            });
        }catch(e) {
            console.error('SingleKvStoreDeleteBatchCallbackTest002 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name SingleKvStoreDeleteBatchCallbackTest003
     * @tc.desc Test Js Api SingleKvStore.DeleteBatch() testcase 003
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreDeleteBatchCallbackTest003', 0, async function (done) {
        console.info('SingleKvStoreDeleteBatchCallbackTest003');
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
            console.info('SingleKvStoreDeleteBatchCallbackTest003 entries: ' + JSON.stringify(entries));
            await kvStore.putBatch(entries, async function (err,data) {
                console.info('SingleKvStoreDeleteBatchCallbackTest003 putBatch success');
                expect(err == undefined).assertTrue();
                let keys = ['batch_test_string_key1', 'batch_test_string_keya'];
                await kvStore.deleteBatch(keys, async function (err,data) {
                    console.info('SingleKvStoreDeleteBatchCallbackTest003 deleteBatch success');
                    expect(err == undefined).assertTrue();
                    done();
                });
            });
        }catch(e) {
            console.error('SingleKvStoreDeleteBatchCallbackTest003 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name SingleKvStorestartTransactionCallbackTest001
     * @tc.desc Test Js Api SingleKvStore.startTransaction() testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStorestartTransactionCallbackTest001', 0, async function (done) {
        console.info('SingleKvStorestartTransactionCallbackTest001');
        try {
            var count = 0;
            kvStore.on('dataChange', 0, function (data) {
                console.info('SingleKvStorestartTransactionCallbackTest001 0' + data)
                count++;
            });
            await kvStore.startTransaction(async function (err,data) {
                console.info('SingleKvStorestartTransactionCallbackTest001 startTransaction success');
                expect(err == undefined).assertTrue();
                let entries = putBatchString(10, 'batch_test_string_key');
                console.info('SingleKvStorestartTransactionCallbackTest001 entries: ' + JSON.stringify(entries));
                await kvStore.putBatch(entries, async function (err,data) {
                    console.info('SingleKvStorestartTransactionCallbackTest001 putBatch success');
                    expect(err == undefined).assertTrue();
                    let keys = Object.keys(entries).slice(5); //delete 5 beginnings
                    await kvStore.deleteBatch(keys, async function (err,data) {
                        console.info('SingleKvStorestartTransactionCallbackTest001 deleteBatch success');
                        expect(err == undefined).assertTrue();
                        await kvStore.commit(async function (err,data) {
                            console.info('SingleKvStorestartTransactionCallbackTest001 commit success');
                            expect(err == undefined).assertTrue();
                            await sleep(2000);
                            expect(count == 1).assertTrue();
                            done();
                        });
                    });
                });
            });
        }catch(e) {
            console.error('SingleKvStorestartTransactionCallbackTest001 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name SingleKvStorestartTransactionCallbackTest002
     * @tc.desc Test Js Api SingleKvStore.startTransaction() testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStorestartTransactionCallbackTest002', 0, async function (done) {
        console.info('SingleKvStorestartTransactionCallbackTest002');
        try {
            var count = 0;
            kvStore.on('dataChange', 0, function (data) {
                console.info('SingleKvStorestartTransactionCallbackTest002 0' + data)
                count++;
            });
            await kvStore.startTransaction(async function (err,data) {
                console.info('SingleKvStorestartTransactionCallbackTest002 startTransaction success');
                expect(err == undefined).assertTrue();
                let entries = putBatchString(10, 'batch_test_string_key');
                console.info('SingleKvStorestartTransactionCallbackTest002 entries: ' + JSON.stringify(entries));
                await kvStore.putBatch(entries, async function (err,data) {
                    console.info('SingleKvStorestartTransactionCallbackTest002 putBatch success');
                    expect(err == undefined).assertTrue();
                    let keys = Object.keys(entries).slice(5); //delete 5 beginnings
                    await kvStore.deleteBatch(keys, async function (err,data) {
                        console.info('SingleKvStorestartTransactionCallbackTest002 deleteBatch success');
                        expect(err == undefined).assertTrue();
                        await kvStore.rollback(async function (err,data) {
                            console.info('SingleKvStorestartTransactionCallbackTest002 rollback success');
                            expect(err == undefined).assertTrue();
                            await sleep(2000);
                            expect(count == 0).assertTrue();
                            done();
                        });
                    });
                });
            });
        }catch(e) {
            console.error('SingleKvStorestartTransactionCallbackTest002 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name SingleKvStorestartTransactionCallbackTest003
     * @tc.desc Test Js Api SingleKvStore.startTransaction() testcase 003
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStorestartTransactionCallbackTest003', 0, async function (done) {
        console.info('SingleKvStorestartTransactionCallbackTest003');
        try {
            await kvStore.startTransaction(1, function (err,data) {
                if (err == undefined) {
                    console.info('SingleKvStorestartTransactionCallbackTest003 startTransaction success');
                    expect(null).assertFail();
                } else {
                    console.info('SingleKvStorestartTransactionCallbackTest003 startTransaction fail');
                }
                done();
            });
        }catch(e) {
            console.error('SingleKvStorestartTransactionCallbackTest003 e ' + `, error code is ${e.code}, message is ${e.message}`);
            done();
        }
    })

    /**
     * @tc.name SingleKvStorestartTransactionCallbackTest004
     * @tc.desc Test Js Api SingleKvStore.startTransaction() testcase 004
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStorestartTransactionCallbackTest004', 0, async function (done) {
        console.info('SingleKvStorestartTransactionCallbackTest004');
        try {
            await kvStore.startTransaction('test_string', function (err,data) {
                if (err == undefined) {
                    console.info('SingleKvStorestartTransactionCallbackTest004 startTransaction success');
                    expect(null).assertFail();
                } else {
                    console.info('SingleKvStorestartTransactionCallbackTest004 startTransaction fail');
                }
                done();
            });
        }catch(e) {
            console.error('SingleKvStorestartTransactionCallbackTest004 e ' + `, error code is ${e.code}, message is ${e.message}`);
            done();
        }
    })

    /**
     * @tc.name SingleKvStorestartTransactionCallbackTest005
     * @tc.desc Test Js Api SingleKvStore.startTransaction() testcase 005
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStorestartTransactionCallbackTest005', 0, async function (done) {
        console.info('SingleKvStorestartTransactionCallbackTest005');
        try {
            await kvStore.startTransaction(2.000, function (err,data) {
                if (err == undefined) {
                    console.info('SingleKvStorestartTransactionCallbackTest005 startTransaction success');
                    expect(null).assertFail();
                } else {
                    console.info('SingleKvStorestartTransactionCallbackTest005 startTransaction fail');
                }
                done();
            });
        }catch(e) {
            console.error('SingleKvStorestartTransactionCallbackTest005 e ' + `, error code is ${e.code}, message is ${e.message}`);
            done();
        }
    })

    /**
     * @tc.name SingleKvStoreCommitCallbackTest001
     * @tc.desc Test Js Api SingleKvStore.Commit() testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreCommitCallbackTest001', 0, async function (done) {
        console.info('SingleKvStoreCommitCallbackTest001');
        try {
            await kvStore.commit(1, function (err,data) {
                if (err == undefined) {
                    console.info('SingleKvStoreCommitCallbackTest001 commit success');
                    expect(null).assertFail();
                } else {
                    console.info('SingleKvStoreCommitCallbackTest001 commit fail');
                }
                done();
            });
        }catch(e) {
            console.error('SingleKvStoreCommitCallbackTest001 e ' + `, error code is ${e.code}, message is ${e.message}`);
            done();
        }
    })

    /**
     * @tc.name SingleKvStoreCommitCallbackTest002
     * @tc.desc Test Js Api SingleKvStore.Commit() testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreCommitCallbackTest002', 0, async function (done) {
        console.info('SingleKvStoreCommitCallbackTest002');
        try {
            await kvStore.commit('test_string', function (err,data) {
                if (err == undefined) {
                    console.info('SingleKvStoreCommitCallbackTest002 commit success');
                    expect(null).assertFail();
                } else {
                    console.info('SingleKvStoreCommitCallbackTest002 commit fail');
                }
                done();
            });
        }catch(e) {
            console.error('SingleKvStoreCommitCallbackTest002 e ' + `, error code is ${e.code}, message is ${e.message}`);
            done();
        }
    })

    /**
     * @tc.name SingleKvStoreCommitCallbackTest003
     * @tc.desc Test Js Api SingleKvStore.Commit() testcase 003
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreCommitCallbackTest003', 0, async function (done) {
        console.info('SingleKvStoreCommitCallbackTest003');
        try {
            await kvStore.commit(2.000, function (err,data) {
                if (err == undefined) {
                    console.info('SingleKvStoreCommitCallbackTest003 commit success');
                    expect(null).assertFail();
                } else {
                    console.info('SingleKvStoreCommitCallbackTest003 commit fail');
                }
                done();
            });
        }catch(e) {
            console.error('SingleKvStoreCommitCallbackTest003 e ' + `, error code is ${e.code}, message is ${e.message}`);
            done();
        }
    })

    /**
     * @tc.name SingleKvStoreRollbackCallbackTest001
     * @tc.desc Test Js Api SingleKvStore.Rollback() testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreRollbackCallbackTest001', 0, async function (done) {
        console.info('SingleKvStoreRollbackCallbackTest001');
        try {
            await kvStore.rollback(1, function (err,data) {
                if (err == undefined) {
                    console.info('SingleKvStoreRollbackCallbackTest001 commit success');
                    expect(null).assertFail();
                } else {
                    console.info('SingleKvStoreRollbackCallbackTest001 commit fail');
                }
                done();
            });
        }catch(e) {
            console.error('SingleKvStoreRollbackCallbackTest001 e ' + `, error code is ${e.code}, message is ${e.message}`);
            done();
        }
    })

    /**
     * @tc.name SingleKvStoreRollbackCallbackTest002
     * @tc.desc Test Js Api SingleKvStore.Rollback() testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreRollbackCallbackTest002', 0, async function (done) {
        console.info('SingleKvStoreRollbackCallbackTest002');
        try {
            await kvStore.rollback('test_string', function (err,data) {
                if (err == undefined) {
                    console.info('SingleKvStoreRollbackCallbackTest002 commit success');
                    expect(null).assertFail();
                } else {
                    console.info('SingleKvStoreRollbackCallbackTest002 commit fail');
                }
                done();
            });
        }catch(e) {
            console.error('SingleKvStoreRollbackCallbackTest002 e ' + `, error code is ${e.code}, message is ${e.message}`);
            done();
        }
    })

    /**
     * @tc.name SingleKvStoreRollbackCallbackTest003
     * @tc.desc Test Js Api SingleKvStore.Rollback() testcase 003
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreRollbackCallbackTest003', 0, async function (done) {
        console.info('SingleKvStoreRollbackCallbackTest003');
        try {
            await kvStore.rollback(2.000, function (err,data) {
                if (err == undefined) {
                    console.info('SingleKvStoreRollbackCallbackTest003 commit success');
                    expect(null).assertFail();
                } else {
                    console.info('SingleKvStoreRollbackCallbackTest003 commit fail');
                }
                done();
            });
        }catch(e) {
            console.error('SingleKvStoreRollbackCallbackTest003 e ' + `, error code is ${e.code}, message is ${e.message}`);
            done();
        }
    })

    /**
     * @tc.name SingleKvStoreEnableSyncCallbackTest001
     * @tc.desc Test Js Api SingleKvStore.EnableSync() testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreEnableSyncCallbackTest001', 0, async function (done) {
        console.info('SingleKvStoreEnableSyncCallbackTest001');
        try {
            await kvStore.enableSync(true, function (err,data) {
                if (err == undefined) {
                    console.info('SingleKvStoreEnableSyncCallbackTest001 enableSync success');
                    expect(err == undefined).assertTrue();
                } else {
                    console.info('SingleKvStoreEnableSyncCallbackTest001 enableSync fail');
                    expect(null).assertFail();
                }
                done();
            });
        }catch(e) {
            console.error('SingleKvStoreEnableSyncCallbackTest001 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name SingleKvStoreEnableSyncCallbackTest002
     * @tc.desc Test Js Api SingleKvStore.EnableSync() testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreEnableSyncCallbackTest002', 0, async function (done) {
        console.info('SingleKvStoreEnableSyncCallbackTest002');
        try {
            await kvStore.enableSync(false, function (err,data) {
                if (err == undefined) {
                    console.info('SingleKvStoreEnableSyncCallbackTest002 enableSync success');
                    expect(err == undefined).assertTrue();
                } else {
                    console.info('SingleKvStoreEnableSyncCallbackTest002 enableSync fail');
                    expect(null).assertFail();
                }
                done();
            });
        }catch(e) {
            console.error('SingleKvStoreEnableSyncCallbackTest002 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name SingleKvStoreEnableSyncCallbackTest003
     * @tc.desc Test Js Api SingleKvStore.EnableSync() testcase 003
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreEnableSyncCallbackTest003', 0, async function (done) {
        console.info('SingleKvStoreEnableSyncCallbackTest003');
        try {
            await kvStore.enableSync(function (err,data) {
                if (err == undefined) {
                    console.info('SingleKvStoreEnableSyncCallbackTest003 enableSync success');
                    expect(null).assertFail();
                } else {
                    console.info('SingleKvStoreEnableSyncCallbackTest003 enableSync fail');
                }
                done();
            });
        }catch(e) {
            console.error('SingleKvStoreEnableSyncCallbackTest003 e ' + `, error code is ${e.code}, message is ${e.message}`);
            done();
        }
    })

    /**
     * @tc.name SingleKvStoreEnableSyncCallbackTest004
     * @tc.desc Test Js Api SingleKvStore.EnableSync() testcase 004
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreEnableSyncCallbackTest004', 0, async function (done) {
        console.info('SingleKvStoreEnableSyncCallbackTest004');
        try {
            await kvStore.enableSync(null, function (err,data) {
                if (err == undefined) {
                    console.info('SingleKvStoreEnableSyncCallbackTest004 enableSync success');
                    expect(null).assertFail();
                } else {
                    console.info('SingleKvStoreEnableSyncCallbackTest004 enableSync fail');
                }
                done();
            });
        }catch(e) {
            console.error('SingleKvStoreEnableSyncCallbackTest004 e ' + `, error code is ${e.code}, message is ${e.message}`);
            done();
        }
    })

    /**
     * @tc.name SingleKvStoreRemoveDeviceDataCallbackTest001
     * @tc.desc Test Js Api SingleKvStore.RemoveDeviceData() testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreRemoveDeviceDataCallbackTest001', 0, async function (done) {
        console.info('SingleKvStoreRemoveDeviceDataCallbackTest001');
        try {
            await kvStore.put(KEY_TEST_STRING_ELEMENT, VALUE_TEST_STRING_ELEMENT, async function (err,data) {
                console.info('SingleKvStoreRemoveDeviceDataCallbackTest001 put success');
                expect(err == undefined).assertTrue();
                var deviceid = 'no_exist_device_id';
                await kvStore.removeDeviceData(deviceid, async function (err,data) {
                    if (err == undefined) {
                        console.info('SingleKvStoreRemoveDeviceDataCallbackTest001 removeDeviceData success');
                        expect(null).assertFail();
                        done();
                    } else {
                        console.info('SingleKvStoreRemoveDeviceDataCallbackTest001 removeDeviceData fail');
                        await kvStore.get(KEY_TEST_STRING_ELEMENT, async function (err,data) {
                            console.info('SingleKvStoreRemoveDeviceDataCallbackTest001 get success');
                            expect(data == VALUE_TEST_STRING_ELEMENT).assertTrue();
                            done();
                        });
                    }
                });
            });
        }catch(e) {
            console.error('SingleKvStoreRemoveDeviceDataCallbackTest001 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name SingleKvStoreRemoveDeviceDataCallbackTest002
     * @tc.desc Test Js Api SingleKvStore.RemoveDeviceData() testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreRemoveDeviceDataCallbackTest002', 0, async function (done) {
        console.info('SingleKvStoreRemoveDeviceDataCallbackTest002');
        try {
            await kvStore.removeDeviceData(function (err,data) {
                if (err == undefined) {
                    console.info('SingleKvStoreRemoveDeviceDataCallbackTest002 removeDeviceData success');
                    expect(null).assertFail();
                } else {
                    console.info('SingleKvStoreRemoveDeviceDataCallbackTest002 removeDeviceData fail');
                }
                done();
            });
        }catch(e) {
            console.error('SingleKvStoreRemoveDeviceDataCallbackTest001 e ' + `, error code is ${e.code}, message is ${e.message}`);
            done();
        }
    })

    /**
     * @tc.name SingleKvStoreRemoveDeviceDataCallbackTest003
     * @tc.desc Test Js Api SingleKvStore.RemoveDeviceData() testcase 003
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
     it('SingleKvStoreRemoveDeviceDataCallbackTest003', 0, async function (done) {
        console.info('SingleKvStoreRemoveDeviceDataCallbackTest003');
        try {
            await kvStore.removeDeviceData('', function (err,data) {
                if (err == undefined) {
                    console.info('SingleKvStoreRemoveDeviceDataCallbackTest003 removeDeviceData success');
                    expect(null).assertFail();
                } else {
                    console.info('SingleKvStoreRemoveDeviceDataCallbackTest003 removeDeviceData fail');
                }
                done();
            });
        }catch(e) {
            console.error('SingleKvStoreRemoveDeviceDataCallbackTest003 e ' + `, error code is ${e.code}, message is ${e.message}`);
            done();
        }
    })

    /**
     * @tc.name SingleKvStoreRemoveDeviceDataCallbackTest004
     * @tc.desc Test Js Api SingleKvStore.RemoveDeviceData() testcase 004
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreRemoveDeviceDataCallbackTest004', 0, async function (done) {
        console.info('SingleKvStoreRemoveDeviceDataCallbackTest004');
        try {
            await kvStore.removeDeviceData(null, function (err,data) {
                if (err == undefined) {
                    console.info('SingleKvStoreRemoveDeviceDataCallbackTest004 removeDeviceData success');
                    expect(null).assertFail();
                } else {
                    console.info('SingleKvStoreRemoveDeviceDataCallbackTest004 removeDeviceData fail');
                }
                done();
            });
        }catch(e) {
            console.error('SingleKvStoreRemoveDeviceDataCallbackTest004 e ' + `, error code is ${e.code}, message is ${e.message}`);
            done();
        }
    })

    /**
     * @tc.name SingleKvStoreSetSyncParamCallbackTest001
     * @tc.desc Test Js Api SingleKvStore.SetSyncParam() testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreSetSyncParamCallbackTest001', 0, async function (done) {
        console.info('SingleKvStoreSetSyncParamCallbackTest001');
        try {
            var defaultAllowedDelayMs = 500;
            await kvStore.setSyncParam(defaultAllowedDelayMs, function (err,data) {
                console.info('SingleKvStoreSetSyncParamCallbackTest001 put success');
                expect(err == undefined).assertTrue();
                done();
            });
        }catch(e) {
            console.error('SingleKvStoreSetSyncParamCallbackTest001 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name SingleKvStoreSetSyncParamCallbackTest002
     * @tc.desc Test Js Api SingleKvStore.SetSyncParam() testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreSetSyncParamCallbackTest002', 0, async function (done) {
        console.info('SingleKvStoreSetSyncParamCallbackTest002');
        try {
            await kvStore.setSyncParam(function (err,data) {
                if (err == undefined) {
                    console.info('SingleKvStoreSetSyncParamCallbackTest002 put success');
                    expect(null).assertFail();
                } else {
                    console.error('SingleKvStoreSetSyncParamCallbackTest002 put err' + `, error code is ${err.code}, message is ${err.message}`);
                }
                done();
            });
        }catch(e) {
            console.error('SingleKvStoreSetSyncParamCallbackTest002 e ' + `, error code is ${e.code}, message is ${e.message}`);
            done();
        }
    })

    /**
     * @tc.name SingleKvStoreSetSyncParamCallbackTest003
     * @tc.desc Test Js Api SingleKvStore.SetSyncParam() testcase 003
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreSetSyncParamCallbackTest003', 0, async function (done) {
        console.info('SingleKvStoreSetSyncParamCallbackTest003');
        try {
            await kvStore.setSyncParam('', function (err,data) {
                if (err == undefined) {
                    console.info('SingleKvStoreSetSyncParamCallbackTest003 put success');
                    expect(null).assertFail();
                } else {
                    console.error('SingleKvStoreSetSyncParamCallbackTest003 put err' + `, error code is ${err.code}, message is ${err.message}`);
                }
                done();
            });
        }catch(e) {
            console.error('SingleKvStoreSetSyncParamCallbackTest003 e ' + `, error code is ${e.code}, message is ${e.message}`);
            done();
        }
    })

    /**
     * @tc.name SingleKvStoreSetSyncParamCallbackTest004
     * @tc.desc Test Js Api SingleKvStore.SetSyncParam() testcase 004
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreSetSyncParamCallbackTest004', 0, async function (done) {
        console.info('SingleKvStoreSetSyncParamCallbackTest004');
        try {
            await kvStore.setSyncParam(null, function (err,data) {
                if (err == undefined) {
                    console.info('SingleKvStoreSetSyncParamCallbackTest004 put success');
                    expect(null).assertFail();
                } else {
                    console.error('SingleKvStoreSetSyncParamCallbackTest004 put err' + `, error code is ${err.code}, message is ${err.message}`);
                }
                done();
            });
        }catch(e) {
            console.error('SingleKvStoreSetSyncParamCallbackTest004 e ' + `, error code is ${e.code}, message is ${e.message}`);
            done();
        }
    })

    /**
     * @tc.name SingleKvStoreGetSecurityLevelCallbackTest002
     * @tc.desc Test Js Api SingleKvStore.GetSecurityLevel() testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreGetSecurityLevelCallbackTest002', 0, async function (done) {
        console.info('SingleKvStoreGetSecurityLevelCallbackTest002');
        try {
            await kvStore.getSecurityLevel(1, function (err,data) {
                if (err == undefined) {
                    console.info('SingleKvStoreGetSecurityLevelCallbackTest002 getSecurityLevel success');
                    expect(null).assertFail();
                } else {
                    console.error('SingleKvStoreGetSecurityLevelCallbackTest002 getSecurityLevel fail' + `, error code is ${err.code}, message is ${err.message}`);
                }
                done();
            });
        }catch(e) {
            console.error('SingleKvStoreGetSecurityLevelCallbackTest002 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name SingleKvStoreGetSecurityLevelCallbackTest003
     * @tc.desc Test Js Api SingleKvStore.GetSecurityLevel() testcase 003
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreGetSecurityLevelCallbackTest003', 0, async function (done) {
        console.info('SingleKvStoreGetSecurityLevelCallbackTest003');
        try {
            await kvStore.getSecurityLevel('test_string', function (err,data) {
                if (err == undefined) {
                    console.info('SingleKvStoreGetSecurityLevelCallbackTest003 getSecurityLevel success');
                    expect(null).assertFail();
                } else {
                    console.error('SingleKvStoreGetSecurityLevelCallbackTest003 getSecurityLevel fail' + `, error code is ${err.code}, message is ${err.message}`);
                }
                done();
            });
        }catch(e) {
            console.error('SingleKvStoreGetSecurityLevelCallbackTest003 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name SingleKvStoreGetSecurityLevelCallbackTest004
     * @tc.desc Test Js Api SingleKvStore.GetSecurityLevel() testcase 004
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreGetSecurityLevelCallbackTest004', 0, async function (done) {
        console.info('SingleKvStoreGetSecurityLevelCallbackTest004');
        try {
            await kvStore.getSecurityLevel(2.00, function (err,data) {
                if (err == undefined) {
                    console.info('SingleKvStoreGetSecurityLevelCallbackTest004 getSecurityLevel success');
                    expect(null).assertFail();
                } else {
                    console.error('SingleKvStoreGetSecurityLevelCallbackTest004 getSecurityLevel fail' + `, error code is ${err.code}, message is ${err.message}`);
                }
                done();
            });
        }catch(e) {
            console.error('SingleKvStoreGetSecurityLevelCallbackTest004 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name SingleKvStoreGetResultSetCallbackTest001
     * @tc.desc Test Js Api SingleKvStore.GetResultSet() testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreGetResultSetCallbackTest001', 0, async function (done) {
        console.info('SingleKvStoreGetResultSetCallbackTest001');
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
            await kvStore.putBatch(entries, async function (err, data) {
                console.info('SingleKvStoreGetResultSetCallbackTest001 putBatch success');
                expect(err == undefined).assertTrue();
                await kvStore.getResultSet('batch_test_string_key', async function (err, result) {
                    console.info('SingleKvStoreGetResultSetCallbackTest001 getResultSet success');
                    resultSet = result;
                    expect(resultSet.getCount() == 10).assertTrue();
                    await kvStore.closeResultSet(resultSet, function (err, data) {
                        console.info('SingleKvStoreGetResultSetCallbackTest001 closeResultSet success');
                        expect(err == undefined).assertTrue();
                        done();
                    })
                });
            });
        }catch(e) {
            console.error('SingleKvStoreGetResultSetCallbackTest001 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name SingleKvStoreGetResultSetCallbackTest002
     * @tc.desc Test Js Api SingleKvStore.GetResultSet() testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreGetResultSetCallbackTest002', 0, async function (done) {
        console.info('SingleKvStoreGetResultSetCallbackTest002');
        try {
            let resultSet;
            await kvStore.getResultSet('batch_test_string_key', async function (err, result) {
                console.info('SingleKvStoreGetResultSetCallbackTest002 getResultSet success');
                resultSet = result;
                expect(resultSet.getCount() == 0).assertTrue();
                await kvStore.closeResultSet(resultSet, function (err, data) {
                    console.info('SingleKvStoreGetResultSetCallbackTest002 closeResultSet success');
                    expect(err == undefined).assertTrue();
                    done();
                })
            });
        }catch(e) {
            console.error('SingleKvStoreGetResultSetCallbackTest002 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name SingleKvStoreGetResultSetCallbackTest003
     * @tc.desc Test Js Api SingleKvStore.GetResultSet() testcase 003
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreGetResultSetCallbackTest003', 0, async function (done) {
        console.info('SingleKvStoreGetResultSetCallbackTest003');
        try {
            let resultSet;
            await kvStore.getResultSet(function (err, result) {
                console.info('SingleKvStoreGetResultSetCallbackTest003 getResultSet success');
                expect(err != undefined).assertTrue();
                done();
            });
        }catch(e) {
            console.error('SingleKvStoreGetResultSetCallbackTest003 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name SingleKvStoreGetResultSetCallbackTest004
     * @tc.desc Test Js Api SingleKvStore.GetResultSet() testcase 004
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreGetResultSetCallbackTest004', 0, async function (done) {
        console.info('SingleKvStoreGetResultSetCallbackTest004');
        try {
            let resultSet;
            await kvStore.getResultSet('test_key_string', 123, function (err, result) {
                console.info('SingleKvStoreGetResultSetCallbackTest004 getResultSet success');
                expect(err != undefined).assertTrue();
                done();
            });
        }catch(e) {
            console.error('SingleKvStoreGetResultSetCallbackTest004 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name SingleKvStoreGetResultSetCallbackTest005
     * @tc.desc Test Js Api SingleKvStore.GetResultSet() testcase 005
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreGetResultSetCallbackTest005', 0, async function (done) {
        console.info('SingleKvStoreGetResultSetCallbackTest005');
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
            await kvStore.putBatch(entries, async function (err, data) {
                console.info('SingleKvStoreGetResultSetCallbackTest005 putBatch success');
                expect(err == undefined).assertTrue();
                var query = new factory.Query();
                query.prefixKey("batch_test");
                await kvStore.getResultSet(query, async function (err, result) {
                    console.info('SingleKvStoreGetResultSetCallbackTest005 getResultSet success');
                    resultSet = result;
                    expect(resultSet.getCount() == 10).assertTrue();
                    await kvStore.closeResultSet(resultSet, function (err, data) {
                        console.info('SingleKvStoreGetResultSetCallbackTest005 closeResultSet success');
                        expect(err == undefined).assertTrue();
                        done();
                    })
                });
            });
        } catch(e) {
            console.error('SingleKvStoreGetResultSetCallbackTest005 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name SingleKvStoreGetResultSetCallbackTest006
     * @tc.desc Test Js Api SingleKvStore.GetResultSet() testcase 006
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreGetResultSetCallbackTest006', 0, async function (done) {
        console.info('SingleKvStoreGetResultSetCallbackTest006');
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
            await kvStore.putBatch(entries, async function (err, data) {
                console.info('SingleKvStoreGetResultSetCallbackTest006 putBatch success');
                expect(err == undefined).assertTrue();
                var query = new factory.Query();
                query.prefixKey("batch_test");
                await kvStore.getResultSet(query, async function (err, result) {
                    console.info('SingleKvStoreGetResultSetCallbackTest006 getResultSet success');
                    resultSet = result;
                    expect(resultSet.getCount() == 10).assertTrue();
                    await kvStore.closeResultSet(resultSet, function (err, data) {
                        console.info('SingleKvStoreGetResultSetCallbackTest006 closeResultSet success');
                        expect(err == undefined).assertTrue();
                        done();
                    })
                });
            });
        } catch(e) {
            console.error('SingleKvStoreGetResultSetCallbackTest006 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name SingleKvStoreCloseResultSetCallbackTest001
     * @tc.desc Test Js Api SingleKvStore.CloseResultSet() testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreCloseResultSetCallbackTest001', 0, async function (done) {
        console.info('SingleKvStoreCloseResultSetCallbackTest001');
        try {
            console.info('SingleKvStoreCloseResultSetCallbackTest001 success');
            let resultSet = null;
            await kvStore.closeResultSet(resultSet, function (err, data) {
                if (err == undefined) {
                    console.info('SingleKvStoreCloseResultSetCallbackTest001 closeResultSet success');
                    expect(null).assertFail();
                } else {
                    console.info('SingleKvStoreCloseResultSetCallbackTest001 closeResultSet fail');
                }
                done();
            });
        }catch(e) {
            console.error('SingleKvStoreCloseResultSetCallbackTest001 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name SingleKvStoreCloseResultSetCallbackTest002
     * @tc.desc Test Js Api SingleKvStore.CloseResultSet() testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreCloseResultSetCallbackTest002', 0, async function (done) {
        console.info('SingleKvStoreCloseResultSetCallbackTest002');
        try {
            let resultSet = null;
            await kvStore.getResultSet('batch_test_string_key', async function(err, result) {
                console.info('SingleKvStoreCloseResultSetCallbackTest002 getResultSet success');
                resultSet = result;
                await kvStore.closeResultSet(resultSet, function (err, data) {
                    if (err == undefined) {
                        console.info('SingleKvStoreCloseResultSetCallbackTest002 closeResultSet success');
                        expect(err == undefined).assertTrue();
                    } else {
                        console.info('SingleKvStoreCloseResultSetCallbackTest002 closeResultSet fail');
                        expect(null).assertFail();
                    }
                    done();
                });
            });
        }catch(e) {
            console.error('SingleKvStoreCloseResultSetCallbackTest002 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name SingleKvStoreCloseResultSetCallbackTest003
     * @tc.desc Test Js Api SingleKvStore.CloseResultSet() testcase 003
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreCloseResultSetCallbackTest003', 0, async function (done) {
        console.info('SingleKvStoreCloseResultSetCallbackTest003');
        try {
            console.info('SingleKvStoreCloseResultSetCallbackTest003 success');
            await kvStore.closeResultSet(function (err, data) {
                if (err == undefined) {
                    console.info('SingleKvStoreCloseResultSetCallbackTest003 closeResultSet success');
                    expect(null).assertFail();
                } else {
                    console.info('SingleKvStoreCloseResultSetCallbackTest003 closeResultSet fail');
                }
                done();
            });
        }catch(e) {
            console.error('SingleKvStoreCloseResultSetCallbackTest003 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name SingleKvStoreCloseResultSetCallbackTest004
     * @tc.desc Test Js Api SingleKvStore.CloseResultSet() testcase 004
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreCloseResultSetCallbackTest004', 0, async function (done) {
        console.info('SingleKvStoreCloseResultSetCallbackTest004');
        try {
            console.info('SingleKvStoreCloseResultSetCallbackTest004 success');
        }catch(e) {
            console.error('SingleKvStoreCloseResultSetCallbackTest004 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreGetResultSizeCallbackTest001
     * @tc.desc Test Js Api SingleKvStore.GetResultSize() testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreGetResultSizeCallbackTest001', 0, async function (done) {
        console.info('SingleKvStoreGetResultSizeCallbackTest001');
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
            await kvStore.putBatch(entries, async function (err, data) {
                console.info('SingleKvStoreGetResultSizeCallbackTest001 putBatch success');
                expect(err == undefined).assertTrue();
                var query = new factory.Query();
                query.prefixKey("batch_test");
                await kvStore.getResultSize(query, async function (err, resultSize) {
                    console.info('SingleKvStoreGetResultSizeCallbackTest001 getResultSet success');
                    expect(resultSize == 10).assertTrue();
                    done();
                });
            });
        } catch(e) {
            console.error('SingleKvStoreGetResultSizeCallbackTest001 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name SingleKvStoreGetResultSizeCallbackTest002
     * @tc.desc Test Js Api SingleKvStore.GetResultSize() testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreGetResultSizeCallbackTest002', 0, async function (done) {
        console.info('SingleKvStoreGetResultSizeCallbackTest002');
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
            await kvStore.putBatch(entries, async function (err, data) {
                console.info('SingleKvStoreGetResultSizeCallbackTest002 putBatch success');
                expect(err == undefined).assertTrue();
                var query = new factory.Query();
                query.prefixKey("batch_test");
                await kvStore.getResultSize(query, async function (err, resultSize) {
                    console.info('SingleKvStoreGetResultSizeCallbackTest002 getResultSet success');
                    expect(resultSize == 10).assertTrue();
                    done();
                });
            });
        } catch(e) {
            console.error('SingleKvStoreGetResultSizeCallbackTest002 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name SingleKvStoreGetEntriesCallbackTest001
     * @tc.desc Test Js Api SingleKvStore.GetEntries() testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreGetEntriesCallbackTest001', 0, async function (done) {
        console.info('SingleKvStoreGetEntriesCallbackTest001');
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
            console.info('SingleKvStoreGetEntriesCallbackTest001 entries: ' + JSON.stringify(entries));
            await kvStore.putBatch(entries, async function (err,data) {
                console.info('SingleKvStoreGetEntriesCallbackTest001 putBatch success');
                expect(err == undefined).assertTrue();
                var query = new factory.Query();
                query.prefixKey("batch_test");
                await kvStore.getEntries(query, function (err,entrys) {
                    console.info('SingleKvStoreGetEntriesCallbackTest001 getEntries success');
                    console.info('SingleKvStoreGetEntriesCallbackTest001 entrys.length: ' + entrys.length);
                    console.info('SingleKvStoreGetEntriesCallbackTest001 entrys[0]: ' + JSON.stringify(entrys[0]));
                    expect(entrys.length == 10).assertTrue();
                    expect(entrys[0].value.value.toString() == arr.toString()).assertTrue();
                    done();
                });
            });
            console.info('SingleKvStoreGetEntriesCallbackTest001 success');
        }catch(e) {
            console.error('SingleKvStoreGetEntriesCallbackTest001 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreGetEntriesCallbackTest002
     * @tc.desc Test Js Api SingleKvStore.GetEntries() testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreGetEntriesCallbackTest002', 0, async function (done) {
        console.info('SingleKvStoreGetEntriesCallbackTest002');
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
            console.info('SingleKvStoreGetEntriesCallbackTest002 entries: ' + JSON.stringify(entries));
            await kvStore.putBatch(entries, async function (err,data) {
                console.info('SingleKvStoreGetEntriesCallbackTest002 putBatch success');
                expect(err == undefined).assertTrue();
                var query = new factory.Query();
                query.prefixKey("batch_test");
                await kvStore.getEntries(query, function (err,entrys) {
                    console.info('SingleKvStoreGetEntriesCallbackTest002 getEntries success');
                    console.info('SingleKvStoreGetEntriesCallbackTest002 entrys.length: ' + entrys.length);
                    console.info('SingleKvStoreGetEntriesCallbackTest002 entrys[0]: ' + JSON.stringify(entrys[0]));
                    expect(entrys.length == 10).assertTrue();
                    expect(entrys[0].value.value.toString() == arr.toString()).assertTrue();
                    done();
                });
            });
            console.info('SingleKvStoreGetEntriesCallbackTest002 success');
        }catch(e) {
            console.error('SingleKvStoreGetEntriesCallbackTest002 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })
})
