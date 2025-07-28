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
