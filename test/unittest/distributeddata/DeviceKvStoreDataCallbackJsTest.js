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
    for (let i = 0; i < len; i++) {
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
describe('deviceKvStoreCallbackTest', function () {
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
     * @tc.name DeviceKvStorePutCallbackTest001
     * @tc.desc Test Js Api DeviceKvStore.Put(String) testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStorePutStringCallbackTest001', 0, async function (done) {
        console.info('DeviceKvStorePutStringCallbackTest001');
        try {
            await kvStore.put(KEY_TEST_STRING_ELEMENT, VALUE_TEST_STRING_ELEMENT, function (err,data) {
                if (err == undefined) {
                    console.info('DeviceKvStorePutStringCallbackTest001 put success');
                } else {
                    console.error('DeviceKvStorePutStringCallbackTest001 put fail' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(null).assertFail();
                }
                done();
            });
        }catch (e) {
            console.error('DeviceKvStorePutStringCallbackTest001 put e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStorePutCallbackTest002
     * @tc.desc Test Js Api DeviceKvStore.Put(String) testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStorePutStringCallbackTest002', 0, async function (done) {
        console.info('DeviceKvStorePutStringCallbackTest002');
        try {
            var str = '';
            for (var i = 0 ; i < 4095; i++) {
                str += 'x';
            }
            await kvStore.put(KEY_TEST_STRING_ELEMENT+'102', str, async function (err,data) {
                console.info('DeviceKvStorePutStringCallbackTest002 put success');
                expect(err == undefined).assertTrue();
                await kvStore.get(localDeviceId, KEY_TEST_STRING_ELEMENT+'102', function (err,data) {
                    console.info('DeviceKvStorePutStringCallbackTest002 get success');
                    expect(str == data).assertTrue();
                    done();
                });
            });
        }catch (e) {
            console.error('DeviceKvStorePutStringCallbackTest002 put e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStoreGetCallbackTest001
     * @tc.desc Test Js Api DeviceKvStore.Get(String) testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreGetStringCallbackTest001', 0, async function (done) {
        console.info('DeviceKvStoreGetStringCallbackTest001');
        try{
            await kvStore.get(localDeviceId, KEY_TEST_STRING_ELEMENT, function (err,data) {
                if (err == undefined) {
                    console.info('DeviceKvStoreGetStringCallbackTest001 get success');
                    expect(null).assertFail();
                } else {
                    console.info('DeviceKvStoreGetStringCallbackTest001 get fail');
                }
                done();
            });
        }catch(e) {
            console.error('DeviceKvStoreGetStringCallbackTest001 get e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStoreGetCallbackTest002
     * @tc.desc Test Js Api DeviceKvStore.Get(String) testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreGetStringCallbackTest002', 0, async function (done) {
        console.info('DeviceKvStoreGetStringCallbackTest002');
        try{
            await kvStore.put(KEY_TEST_STRING_ELEMENT, VALUE_TEST_STRING_ELEMENT, async function (err,data) {
                console.info('DeviceKvStoreGetStringCallbackTest002 put success');
                expect(err == undefined).assertTrue();
                await kvStore.get(localDeviceId, KEY_TEST_STRING_ELEMENT, function (err,data) {
                    console.info('DeviceKvStoreGetStringCallbackTest002 get success');
                    expect((err == undefined) && (VALUE_TEST_STRING_ELEMENT == data)).assertTrue();
                    done();
                });
            })
        }catch(e) {
            console.error('DeviceKvStoreGetStringCallbackTest002 get e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStorePutCallbackTest001
     * @tc.desc Test Js Api DeviceKvStore.Put(Int) testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStorePutIntCallbackTest001', 0, async function (done) {
        console.info('DeviceKvStorePutIntCallbackTest001');
        try {
            await kvStore.put(KEY_TEST_INT_ELEMENT, VALUE_TEST_INT_ELEMENT, async function (err,data) {
                console.info('DeviceKvStorePutIntCallbackTest001 put success');
                expect(err == undefined).assertTrue();
                await kvStore.get(localDeviceId, KEY_TEST_INT_ELEMENT, function (err,data) {
                    console.info('DeviceKvStorePutIntCallbackTest001 get success');
                    expect((err == undefined) && (VALUE_TEST_INT_ELEMENT == data)).assertTrue();
                    done();
                })
            });
        }catch(e) {
            console.error('DeviceKvStorePutIntCallbackTest001 put e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStorePutCallbackTest002
     * @tc.desc Test Js Api DeviceKvStore.Put(Int) testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStorePutIntCallbackTest002', 0, async function (done) {
        console.info('DeviceKvStorePutIntCallbackTest002');
        try {
            var intValue = 987654321;
            await kvStore.put(KEY_TEST_INT_ELEMENT, intValue, async function (err,data) {
                console.info('DeviceKvStorePutIntCallbackTest002 put success');
                expect(err == undefined).assertTrue();
                await kvStore.get(localDeviceId, KEY_TEST_INT_ELEMENT, function (err,data) {
                    console.info('DeviceKvStorePutIntCallbackTest002 get success');
                    expect((err == undefined) && (intValue == data)).assertTrue();
                    done();
                })
            });
        }catch(e) {
            console.error('DeviceKvStorePutIntCallbackTest002 put e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStorePutCallbackTest003
     * @tc.desc Test Js Api DeviceKvStore.Put(Int) testcase 003
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStorePutIntCallbackTest003', 0, async function (done) {
        console.info('DeviceKvStorePutIntCallbackTest003');
        try {
            var intValue = Number.MIN_VALUE;
            await kvStore.put(KEY_TEST_INT_ELEMENT, intValue, async function (err,data) {
                console.info('DeviceKvStorePutIntCallbackTest003 put success');
                expect(err == undefined).assertTrue();
                await kvStore.get(localDeviceId, KEY_TEST_INT_ELEMENT, function (err,data) {
                    console.info('DeviceKvStorePutIntCallbackTest003 get success');
                    expect((err == undefined) && (intValue == data)).assertTrue();
                    done();
                })
            });
        }catch(e) {
            console.error('DeviceKvStorePutIntCallbackTest003 put e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStorePutCallbackTest004
     * @tc.desc Test Js Api DeviceKvStore.Put(Int) testcase 004
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStorePutIntCallbackTest004', 0, async function (done) {
        console.info('DeviceKvStorePutIntCallbackTest004');
        try {
            var intValue = Number.MAX_VALUE;
            await kvStore.put(KEY_TEST_INT_ELEMENT, intValue, async function (err,data) {
                console.info('DeviceKvStorePutIntCallbackTest004 put success');
                expect(err == undefined).assertTrue();
                await kvStore.get(localDeviceId, KEY_TEST_INT_ELEMENT, function (err,data) {
                    console.info('DeviceKvStorePutIntCallbackTest004 get success');
                    expect((err == undefined) && (intValue == data)).assertTrue();
                    done();
                })
            });
        }catch(e) {
            console.error('DeviceKvStorePutIntCallbackTest004 put e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStoreGetCallbackTest001
     * @tc.desc Test Js Api DeviceKvStore.Get(Int) testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreGetIntCallbackTest001', 0, async function (done) {
        console.info('DeviceKvStoreGetIntCallbackTest001');
        try {
            await kvStore.put(KEY_TEST_INT_ELEMENT, VALUE_TEST_INT_ELEMENT, async function (err,data) {
                console.info('DeviceKvStoreGetIntCallbackTest001 put success');
                expect(err == undefined).assertTrue();
                await kvStore.get(localDeviceId, KEY_TEST_INT_ELEMENT, function (err,data) {
                    console.info('DeviceKvStoreGetIntCallbackTest001 get success');
                    expect((err == undefined) && (VALUE_TEST_INT_ELEMENT == data)).assertTrue();
                    done();
                })
            });
        }catch(e) {
            console.error('DeviceKvStoreGetIntCallbackTest001 put e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStoreGetCallbackTest002
     * @tc.desc Test Js Api DeviceKvStore.Get(Int) testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreGetIntCallbackTest002', 0, async function (done) {
        console.info('DeviceKvStoreGetIntCallbackTest002');
        try {
            await kvStore.get(localDeviceId, KEY_TEST_INT_ELEMENT, function (err,data) {
                if (err == undefined) {
                    console.info('DeviceKvStoreGetIntCallbackTest002 get success');
                    expect(null).assertFail();
                } else {
                    console.info('DeviceKvStoreGetIntCallbackTest002 get fail');
                }
                done();
            })
        }catch(e) {
            console.error('DeviceKvStoreGetIntCallbackTest002 put e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStorePutCallbackTest001
     * @tc.desc Test Js Api DeviceKvStore.Put(Bool) testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStorePutBoolCallbackTest001', 0, async function (done) {
        console.info('DeviceKvStorePutBoolCallbackTest001');
        try {
            await kvStore.put(KEY_TEST_BOOLEAN_ELEMENT, VALUE_TEST_BOOLEAN_ELEMENT, function (err,data) {
                console.info('DeviceKvStorePutBoolCallbackTest001 put success');
                expect(err == undefined).assertTrue();
                done();
            });
        }catch(e) {
            console.error('DeviceKvStorePutBoolCallbackTest001 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStoreGetCallbackTest001
     * @tc.desc Test Js Api DeviceKvStore.Get(Bool) testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreGetBoolCallbackTest001', 0, async function (done) {
        console.info('DeviceKvStoreGetBoolCallbackTest001');
        try {
            await kvStore.get(localDeviceId, KEY_TEST_BOOLEAN_ELEMENT, function (err,data) {
                if (err == undefined) {
                    console.info('DeviceKvStoreGetBoolCallbackTest001 get success');
                    expect(null).assertFail();
                } else {
                    console.error('DeviceKvStoreGetBoolCallbackTest001 get fail' + `, error code is ${err.code}, message is ${err.message}`);
                }
                done();
            });
        }catch(e) {
            console.error('DeviceKvStoreGetBoolCallbackTest001 e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStoreGetCallbackTest002
     * @tc.desc Test Js Api DeviceKvStore.Get(Bool) testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreGetBoolCallbackTest002', 0, async function (done) {
        console.info('DeviceKvStoreGetBoolCallbackTest002');
        try {
            await kvStore.put(KEY_TEST_BOOLEAN_ELEMENT, VALUE_TEST_BOOLEAN_ELEMENT, async function (err, data) {
                console.info('DeviceKvStoreGetBoolCallbackTest002 put success');
                expect(err == undefined).assertTrue();
                await kvStore.get(localDeviceId, KEY_TEST_BOOLEAN_ELEMENT, function (err,data) {
                    console.info('DeviceKvStoreGetBoolCallbackTest002 get success');
                    expect((err == undefined) && (VALUE_TEST_BOOLEAN_ELEMENT == data)).assertTrue();
                    done();
                });
            })
        }catch(e) {
            console.error('DeviceKvStoreGetBoolCallbackTest002 e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStorePutCallbackTest001
     * @tc.desc Test Js Api DeviceKvStore.Put(Float) testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStorePutFloatCallbackTest001', 0, async function (done) {
        console.info('DeviceKvStorePutFloatCallbackTest001');
        try {
            await kvStore.put(KEY_TEST_FLOAT_ELEMENT, VALUE_TEST_FLOAT_ELEMENT, function (err,data) {
                console.info('DeviceKvStorePutFloatCallbackTest001 put success');
                expect(err == undefined).assertTrue();
                done();
            });
        }catch(e) {
            console.error('DeviceKvStorePutFloatCallbackTest001 e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStorePutCallbackTest002
     * @tc.desc Test Js Api DeviceKvStore.Put(Float) testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStorePutFloatCallbackTest002', 0, async function (done) {
        console.info('DeviceKvStorePutFloatCallbackTest002');
        try {
            var floatValue = 123456.654321;
            await kvStore.put(KEY_TEST_FLOAT_ELEMENT, floatValue, async function (err,data) {
                console.info('DeviceKvStorePutFloatCallbackTest002 put success');
                expect(err == undefined).assertTrue();
                await kvStore.get(localDeviceId, KEY_TEST_FLOAT_ELEMENT, function (err, data) {
                    console.info('DeviceKvStorePutFloatCallbackTest002 get success');
                    expect((err == undefined) && (floatValue == data)).assertTrue();
                    done();
                })
                done();
            });
        }catch(e) {
            console.error('DeviceKvStorePutFloatCallbackTest002 e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStorePutCallbackTest003
     * @tc.desc Test Js Api DeviceKvStore.Put(Float) testcase 003
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStorePutFloatCallbackTest003', 0, async function (done) {
        console.info('DeviceKvStorePutFloatCallbackTest003');
        try {
            var floatValue = 123456.0;
            await kvStore.put(KEY_TEST_FLOAT_ELEMENT, floatValue, async function (err,data) {
                console.info('DeviceKvStorePutFloatCallbackTest003 put success');
                expect(err == undefined).assertTrue();
                await kvStore.get(localDeviceId, KEY_TEST_FLOAT_ELEMENT, function (err, data) {
                    console.info('DeviceKvStorePutFloatCallbackTest003 get success');
                    expect((err == undefined) && (floatValue == data)).assertTrue();
                    done();
                })
                done();
            });
        }catch(e) {
            console.error('DeviceKvStorePutFloatCallbackTest003 e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStorePutCallbackTest004
     * @tc.desc Test Js Api DeviceKvStore.Put(Float) testcase 004
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStorePutFloatCallbackTest004', 0, async function (done) {
        console.info('DeviceKvStorePutFloatCallbackTest004');
        try {
            var floatValue = 123456.00;
            await kvStore.put(KEY_TEST_FLOAT_ELEMENT, floatValue, async function (err,data) {
                console.info('DeviceKvStorePutFloatCallbackTest004 put success');
                expect(err == undefined).assertTrue();
                await kvStore.get(localDeviceId, KEY_TEST_FLOAT_ELEMENT, function (err, data) {
                    console.info('DeviceKvStorePutFloatCallbackTest004 get success');
                    expect((err == undefined) && (floatValue == data)).assertTrue();
                    done();
                })
                done();
            });
        }catch(e) {
            console.error('DeviceKvStorePutFloatCallbackTest004 e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStoreGetCallbackTest001
     * @tc.desc Test Js Api DeviceKvStore.Get(Float) testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreGetFloatCallbackTest001', 0, async function (done) {
        console.info('DeviceKvStoreGetFloatCallbackTest001');
        try {
            await kvStore.get(localDeviceId, KEY_TEST_FLOAT_ELEMENT, function (err,data) {
                if (err == undefined) {
                    console.info('DeviceKvStoreGetFloatCallbackTest001 get success');
                    expect(null).assertFail();
                } else {
                    console.error('DeviceKvStoreGetFloatCallbackTest001 get fail' + `, error code is ${err.code}, message is ${err.message}`);
                }
                done();
            });
        }catch(e) {
            console.error('DeviceKvStoreGetFloatCallbackTest001 e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStoreDeleteStringCallbackTest001
     * @tc.desc Test Js Api DeviceKvStore.DeleteString testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreDeleteStringCallbackTest001', 0, async function (done) {
        console.info('DeviceKvStoreDeleteStringCallbackTest001');
        try {
            await kvStore.delete(KEY_TEST_STRING_ELEMENT, function (err,data) {
                if (err == undefined) {
                    console.info('DeviceKvStoreDeleteStringCallbackTest001 delete success');
                } else {
                    console.error('DeviceKvStoreDeleteStringCallbackTest001 delete fail' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(null).assertFail();
                }
                done();
            });
        }catch(e) {
            console.error('DeviceKvStoreDeleteStringCallbackTest001 e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStoreDeleteStringCallbackTest002
     * @tc.desc Test Js Api DeviceKvStore.DeleteString testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreDeleteStringCallbackTest002', 0, async function (done) {
        console.info('DeviceKvStoreDeleteStringCallbackTest002');
        try {
            await kvStore.put(KEY_TEST_STRING_ELEMENT, VALUE_TEST_STRING_ELEMENT, async function (err, data) {
                console.info('DeviceKvStoreDeleteStringCallbackTest002 put success');
                expect(err == undefined).assertTrue();
                await kvStore.delete(KEY_TEST_STRING_ELEMENT, function (err,data) {
                    console.info('DeviceKvStoreDeleteStringCallbackTest002 delete success');
                    expect(err == undefined).assertTrue();
                    done();
                });
            })
        }catch(e) {
            console.error('DeviceKvStoreDeleteStringCallbackTest002 e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStoreDeleteIntCallbackTest001
     * @tc.desc Test Js Api DeviceKvStore.DeleteInt testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreDeleteIntCallbackTest001', 0, async function (done) {
        console.info('DeviceKvStoreDeleteIntCallbackTest001');
        try{
            await kvStore.delete(KEY_TEST_INT_ELEMENT, function (err,data) {
                if (err == undefined) {
                    console.info('DeviceKvStoreDeleteIntCallbackTest001 get success');
                } else {
                    console.error('DeviceKvStoreDeleteIntCallbackTest001 get fail' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(null).assertFail();
                }
                done();
            });
        }catch(e) {
            console.error('DeviceKvStoreDeleteIntCallbackTest001 e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStoreDeleteIntCallbackTest002
     * @tc.desc Test Js Api DeviceKvStore.DeleteInt testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreDeleteIntCallbackTest002', 0, async function (done) {
        console.info('DeviceKvStoreDeleteIntCallbackTest002');
        try{
            await kvStore.put(KEY_TEST_INT_ELEMENT, VALUE_TEST_INT_ELEMENT, async function (err,data) {
                console.info('DeviceKvStoreDeleteIntCallbackTest002 put success');
                expect(err == undefined).assertTrue();
                await kvStore.delete(KEY_TEST_INT_ELEMENT, function (err,data) {
                    console.info('DeviceKvStoreDeleteIntCallbackTest002 delete success');
                    expect(err == undefined).assertTrue();
                    done();
                });
            })
        }catch(e) {
            console.error('DeviceKvStoreDeleteIntCallbackTest002 e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStoreDeleteFloatCallbackTest001
     * @tc.desc Test Js Api DeviceKvStore.DeleteFloat testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreDeleteFloatCallbackTest001', 0, async function (done) {
        console.info('DeviceKvStoreDeleteFloatCallbackTest001');
        try{
            await kvStore.delete(KEY_TEST_FLOAT_ELEMENT, function (err,data) {
                if (err == undefined) {
                    console.info('DeviceKvStoreDeleteFloatCallbackTest001 get success');
                } else {
                    console.error('DeviceKvStoreDeleteFloatCallbackTest001 get fail' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(null).assertFail();
                }
                done();
            });
        }catch(e) {
            console.error('DeviceKvStoreDeleteFloatCallbackTest001 e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStoreDeleteFloatCallbackTest002
     * @tc.desc Test Js Api DeviceKvStore.DeleteFloat testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreDeleteFloatCallbackTest002', 0, async function (done) {
        console.info('DeviceKvStoreDeleteFloatCallbackTest002');
        try{
            await kvStore.put(KEY_TEST_FLOAT_ELEMENT, VALUE_TEST_FLOAT_ELEMENT, async function (err, data) {
                console.info('DeviceKvStoreDeleteFloatCallbackTest002 put success');
                expect(err == undefined).assertTrue();
                await kvStore.delete(KEY_TEST_FLOAT_ELEMENT, function (err,data) {
                    console.info('DeviceKvStoreDeleteFloatCallbackTest002 delete success');
                    expect(err == undefined).assertTrue();
                    done();
                });
            })
        }catch(e) {
            console.error('DeviceKvStoreDeleteFloatCallbackTest002 e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStoreDeleteBoolCallbackTest001
     * @tc.desc Test Js Api DeviceKvStore.DeleteBool testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreDeleteBoolCallbackTest001', 0, async function (done) {
        console.info('DeviceKvStoreDeleteBoolCallbackTest001');
        try{
            await kvStore.delete(KEY_TEST_BOOLEAN_ELEMENT, function (err,data) {
                if (err == undefined) {
                    console.info('DeviceKvStoreDeleteBoolCallbackTest001 get success');
                } else {
                    console.error('DeviceKvStoreDeleteBoolCallbackTest001 get fail' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(null).assertFail();
                }
                done();
            });
        }catch(e) {
            console.error('DeviceKvStoreDeleteBoolCallbackTest001 e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStoreDeleteBoolCallbackTest002
     * @tc.desc Test Js Api DeviceKvStore.DeleteBool testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreDeleteBoolCallbackTest002', 0, async function (done) {
        console.info('DeviceKvStoreDeleteBoolCallbackTest002');
        try{
            await kvStore.put(KEY_TEST_BOOLEAN_ELEMENT, VALUE_TEST_BOOLEAN_ELEMENT, async function (err, data) {
                console.info('DeviceKvStoreDeleteBoolCallbackTest002 put success');
                expect(err == undefined).assertTrue();
                await kvStore.delete(KEY_TEST_BOOLEAN_ELEMENT, function (err,data) {
                    console.info('DeviceKvStoreDeleteBoolCallbackTest002 delete success');
                    expect(err == undefined).assertTrue();
                    done();
                });
            })
        }catch(e) {
            console.error('DeviceKvStoreDeleteBoolCallbackTest002 e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStoreOnChangeCallbackTest001
     * @tc.desc Test Js Api DeviceKvStore.OnChange testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreOnChangeCallbackTest001', 0, async function (done) {
        console.info('DeviceKvStoreOnChangeCallbackTest001');
        try {
            kvStore.on('dataChange', 0, function (data) {
                console.info('DeviceKvStoreOnChangeCallbackTest001 dataChange');
                expect(data != null).assertTrue();
            });
            await kvStore.put(KEY_TEST_FLOAT_ELEMENT, VALUE_TEST_FLOAT_ELEMENT, function (err,data) {
                console.info('DeviceKvStoreOnChangeCallbackTest001 put success');
                expect(err == undefined).assertTrue();
                done();
            });
        }catch(e) {
            console.error('DeviceKvStoreOnChangeCallbackTest001 e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStoreOnChangeCallbackTest002
     * @tc.desc Test Js Api DeviceKvStore.OnChange testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreOnChangeCallbackTest002', 0, async function (done) {
        console.info('DeviceKvStoreOnChangeCallbackTest002');
        try {
            kvStore.on('dataChange', 1, function (data) {
                console.info('DeviceKvStoreOnChangeCallbackTest002 dataChange');
                expect(data != null).assertTrue();
            });
            await kvStore.put(KEY_TEST_FLOAT_ELEMENT, VALUE_TEST_FLOAT_ELEMENT, function (err,data) {
                console.info('DeviceKvStoreOnChangeCallbackTest002 put success');
                expect(err == undefined).assertTrue();
                done();
            });
        }catch(e) {
            console.error('DeviceKvStoreOnChangeCallbackTest002 e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStoreOnChangeCallbackTest003
     * @tc.desc Test Js Api DeviceKvStore.OnChange testcase 003
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreOnChangeCallbackTest003', 0, async function (done) {
        console.info('DeviceKvStoreOnChangeCallbackTest003');
        try {
            kvStore.on('dataChange', 2, function (data) {
                console.info('DeviceKvStoreOnChangeCallbackTest003 dataChange');
                expect(data != null).assertTrue();
            });
            await kvStore.put(KEY_TEST_FLOAT_ELEMENT, VALUE_TEST_FLOAT_ELEMENT, function (err,data) {
                console.info('DeviceKvStoreOnChangeCallbackTest003 put success');
                expect(err == undefined).assertTrue();
                done();
            });
        }catch(e) {
            console.error('DeviceKvStoreOnChangeCallbackTest003 e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStoreOnSyncCompleteCallbackTest001
     * @tc.desc Test Js Api DeviceKvStore.OnSyncComplete testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreOnSyncCompleteCallbackTest001', 0, async function (done) {
        try {
            kvStore.on('syncComplete', function (data) {
                console.info('DeviceKvStoreOnSyncCompleteCallbackTest001 dataChange');
                expect(data != null).assertTrue();
            });
            await kvStore.put(KEY_TEST_SYNC_ELEMENT + 'SyncTest001', VALUE_TEST_SYNC_ELEMENT).then((data) => {
                console.info('DeviceKvStoreOnSyncCompleteCallbackTest001 put success');
                expect(data == undefined).assertTrue();
            }).catch((error) => {
                console.error('DeviceKvStoreOnSyncCompleteCallbackTest001 put failed:' + `, error code is ${e.code}, message is ${e.message}`);
                expect(null).assertFail();
            });
            try {
                var mode = factory.SyncMode.PULL_ONLY;
                console.info('kvStore.sync to ' + JSON.stringify(syncDeviceIds));
                kvStore.sync(syncDeviceIds, mode);
            } catch(e) {
                console.error('DeviceKvStoreOnSyncCompleteCallbackTest001 sync no peer device :e:' + `, error code is ${e.code}, message is ${e.message}`);
            }
        }catch(e) {
            console.error('DeviceKvStoreOnSyncCompleteCallbackTest001 no peer device :e:' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreOnSyncCompleteCallbackTest002
     * @tc.desc Test Js Api DeviceKvStore.OnSyncComplete testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreOnSyncCompleteCallbackTest002', 0, async function (done) {
        try {
            kvStore.on('syncComplete', function (data) {
                console.info('DeviceKvStoreOnSyncCompleteCallbackTest002 dataChange');
                expect(data != null).assertTrue();
            });
            await kvStore.put(KEY_TEST_SYNC_ELEMENT + 'SyncTest001', VALUE_TEST_SYNC_ELEMENT).then((data) => {
                console.info('DeviceKvStoreOnSyncCompleteCallbackTest002 put success');
                expect(data == undefined).assertTrue();
            }).catch((error) => {
                console.error('DeviceKvStoreOnSyncCompleteCallbackTest002 put failed:' + `, error code is ${e.code}, message is ${e.message}`);
                expect(null).assertFail();
            });
            try {
                var mode = factory.SyncMode.PUSH_ONLY;
                console.info('kvStore.sync to ' + JSON.stringify(syncDeviceIds));
                kvStore.sync(syncDeviceIds, mode);
            } catch(e) {
                console.error('DeviceKvStoreOnSyncCompleteCallbackTest002 sync no peer device :e:' + `, error code is ${e.code}, message is ${e.message}`);
            }
        }catch(e) {
            console.error('DeviceKvStoreOnSyncCompleteCallbackTest002 no peer device :e:' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreOnSyncCompleteCallbackTest003
     * @tc.desc Test Js Api DeviceKvStore.OnSyncComplete testcase 003
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreOnSyncCompleteCallbackTest003', 0, async function (done) {
        try {
            kvStore.on('syncComplete', function (data) {
                console.info('DeviceKvStoreOnSyncCompleteCallbackTest003 dataChange');
                expect(data != null).assertTrue();
            });
            await kvStore.put(KEY_TEST_SYNC_ELEMENT + 'SyncTest001', VALUE_TEST_SYNC_ELEMENT).then((data) => {
                console.info('DeviceKvStoreOnSyncCompleteCallbackTest003 put success');
                expect(data == undefined).assertTrue();
            }).catch((error) => {
                console.error('DeviceKvStoreOnSyncCompleteCallbackTest003 put failed:' + `, error code is ${e.code}, message is ${e.message}`);
                expect(null).assertFail();
            });
            try {
                var mode = factory.SyncMode.PUSH_PULL;
                console.info('kvStore.sync to ' + JSON.stringify(syncDeviceIds));
                kvStore.sync(syncDeviceIds, mode);
            } catch(e) {
                console.error('DeviceKvStoreOnSyncCompleteCallbackTest003 sync no peer device :e:' + `, error code is ${e.code}, message is ${e.message}`);
            }
        }catch(e) {
            console.error('DeviceKvStoreOnSyncCompleteCallbackTest003 no peer device :e:' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreSetSyncRangeCallbackTest001
     * @tc.desc Test Js Api DeviceKvStore.SetSyncRange testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreSetSyncRangeCallbackTest001', 0, async function (done) {
        console.info('DeviceKvStoreSetSyncRangeCallbackTest001');
        try {
            var localLabels = ['A', 'B'];
            var remoteSupportLabels = ['C', 'D'];
            await kvStore.setSyncRange(localLabels, remoteSupportLabels, function (err,data) {
                console.info('DeviceKvStoreSetSyncRangeCallbackTest001 put success');
                expect(err == undefined).assertTrue();
                done();
            });
        }catch(e) {
            console.error('DeviceKvStoreSetSyncRangeCallbackTest001 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStoreSetSyncRangeCallbackTest002
     * @tc.desc Test Js Api DeviceKvStore.SetSyncRange testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreSetSyncRangeCallbackTest002', 0, async function (done) {
        console.info('DeviceKvStoreSetSyncRangeCallbackTest002');
        try {
            var localLabels = ['A', 'B'];
            var remoteSupportLabels = ['B', 'C'];
            await kvStore.setSyncRange(localLabels, remoteSupportLabels, function (err,data) {
                console.info('DeviceKvStoreSetSyncRangeCallbackTest002 put success');
                expect(err == undefined).assertTrue();
                done();
            });
        }catch(e) {
            console.error('DeviceKvStoreSetSyncRangeCallbackTest002 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStoreSetSyncRangeCallbackTest003
     * @tc.desc Test Js Api DeviceKvStore.SetSyncRange testcase 003
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreSetSyncRangeCallbackTest003', 0, async function (done) {
        console.info('DeviceKvStoreSetSyncRangeCallbackTest003');
        try {
            var localLabels = ['A', 'B'];
            var remoteSupportLabels = ['A', 'B'];
            await kvStore.setSyncRange(localLabels, remoteSupportLabels, function (err,data) {
                console.info('DeviceKvStoreSetSyncRangeCallbackTest003 put success');
                expect(err == undefined).assertTrue();
                done();
            });
        }catch(e) {
            console.error('DeviceKvStoreSetSyncRangeCallbackTest003 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStorePutCallbackTest001
     * @tc.desc Test Js Api DeviceKvStore.Put(Batch) testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStorePutBatchCallbackTest001', 0, async function (done) {
        console.info('DeviceKvStorePutBatchCallbackTest001');
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
            console.info('DeviceKvStorePutBatchCallbackTest001 entries: ' + JSON.stringify(entries));
            await kvStore.putBatch(entries, async function (err,data) {
                console.info('DeviceKvStorePutBatchCallbackTest001 putBatch success');
                expect(err == undefined).assertTrue();
                await kvStore.getEntries(localDeviceId, 'batch_test_string_key', function (err,entrys) {
                    console.info('DeviceKvStorePutBatchCallbackTest001 getEntries success');
                    console.info('DeviceKvStorePutBatchCallbackTest001 entrys.length: ' + entrys.length);
                    console.info('DeviceKvStorePutBatchCallbackTest001 entrys[0]: ' + JSON.stringify(entrys[0]));
                    expect(entrys.length == 10).assertTrue();
                    expect(entrys[0].value.value == 'batch_test_string_value').assertTrue();
                    done();
                });
            });
        }catch(e) {
            console.error('DeviceKvStorePutBatchCallbackTest001 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStorePutCallbackTest002
     * @tc.desc Test Js Api DeviceKvStore.Put(Batch) testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStorePutBatchCallbackTest002', 0, async function (done) {
        console.info('DeviceKvStorePutBatchCallbackTest002');
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
            console.info('DeviceKvStorePutBatchCallbackTest002 entries: ' + JSON.stringify(entries));
            await kvStore.putBatch(entries, async function (err,data) {
                console.info('DeviceKvStorePutBatchCallbackTest002 putBatch success');
                expect(err == undefined).assertTrue();
                await kvStore.getEntries(localDeviceId, 'batch_test_number_key', function (err,entrys) {
                    console.info('DeviceKvStorePutBatchCallbackTest002 getEntries success');
                    console.info('DeviceKvStorePutBatchCallbackTest002 entrys.length: ' + entrys.length);
                    console.info('DeviceKvStorePutBatchCallbackTest002 entrys[0]: ' + JSON.stringify(entrys[0]));
                    expect(entrys.length == 10).assertTrue();
                    expect(entrys[0].value.value == 222).assertTrue();
                    done();
                });
            });
        }catch(e) {
            console.error('DeviceKvStorePutBatchCallbackTest002 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStorePutCallbackTest003
     * @tc.desc Test Js Api DeviceKvStore.Put(Batch) testcase 003
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStorePutBatchCallbackTest003', 0, async function (done) {
        console.info('DeviceKvStorePutBatchCallbackTest003');
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
            console.info('DeviceKvStorePutBatchCallbackTest003 entries: ' + JSON.stringify(entries));
            await kvStore.putBatch(entries, async function (err,data) {
                console.info('DeviceKvStorePutBatchCallbackTest003 putBatch success');
                expect(err == undefined).assertTrue();
                await kvStore.getEntries(localDeviceId, 'batch_test_number_key', function (err,entrys) {
                    console.info('DeviceKvStorePutBatchCallbackTest003 getEntries success');
                    console.info('DeviceKvStorePutBatchCallbackTest003 entrys.length: ' + entrys.length);
                    console.info('DeviceKvStorePutBatchCallbackTest003 entrys[0]: ' + JSON.stringify(entrys[0]));
                    expect(entrys.length == 10).assertTrue();
                    expect(entrys[0].value.value == 2.0).assertTrue();
                    done();
                });
            });
        }catch(e) {
            console.error('DeviceKvStorePutBatchCallbackTest003 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStorePutCallbackTest004
     * @tc.desc Test Js Api DeviceKvStore.Put(Batch) testcase 004
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStorePutBatchCallbackTest004', 0, async function (done) {
        console.info('DeviceKvStorePutBatchCallbackTest004');
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
            console.info('DeviceKvStorePutBatchCallbackTest004 entries: ' + JSON.stringify(entries));
            await kvStore.putBatch(entries, async function (err,data) {
                console.info('DeviceKvStorePutBatchCallbackTest004 putBatch success');
                expect(err == undefined).assertTrue();
                await kvStore.getEntries(localDeviceId, 'batch_test_number_key', function (err,entrys) {
                    console.info('DeviceKvStorePutBatchCallbackTest004 getEntries success');
                    console.info('DeviceKvStorePutBatchCallbackTest004 entrys.length: ' + entrys.length);
                    console.info('DeviceKvStorePutBatchCallbackTest004 entrys[0]: ' + JSON.stringify(entrys[0]));
                    expect(entrys.length == 10).assertTrue();
                    expect(entrys[0].value.value == 2.00).assertTrue();
                    done();
                });
            });
        }catch(e) {
            console.error('DeviceKvStorePutBatchCallbackTest004 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStorePutCallbackTest005
     * @tc.desc Test Js Api DeviceKvStore.Put(Batch) testcase 005
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStorePutBatchCallbackTest005', 0, async function (done) {
        console.info('DeviceKvStorePutBatchCallbackTest005');
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
            console.info('DeviceKvStorePutBatchCallbackTest005 entries: ' + JSON.stringify(entries));
            await kvStore.putBatch(entries, async function (err,data) {
                console.info('DeviceKvStorePutBatchCallbackTest005 putBatch success');
                expect(err == undefined).assertTrue();
                await kvStore.getEntries(localDeviceId, 'batch_test_bool_key', function (err,entrys) {
                    console.info('DeviceKvStorePutBatchCallbackTest005 getEntries success');
                    console.info('DeviceKvStorePutBatchCallbackTest005 entrys.length: ' + entrys.length);
                    console.info('DeviceKvStorePutBatchCallbackTest005 entrys[0]: ' + JSON.stringify(entrys[0]));
                    expect(entrys.length == 10).assertTrue();
                    expect(entrys[0].value.value == bo).assertTrue();
                    done();
                });
            });
        }catch(e) {
            console.error('DeviceKvStorePutBatchCallbackTest005 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStorePutCallbackTest006
     * @tc.desc Test Js Api DeviceKvStore.Put(Batch) testcase 006
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStorePutBatchCallbackTest006', 0, async function (done) {
        console.info('DeviceKvStorePutBatchCallbackTest006');
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
            console.info('DeviceKvStorePutBatchCallbackTest006 entries: ' + JSON.stringify(entries));
            await kvStore.putBatch(entries, async function (err,data) {
                console.info('DeviceKvStorePutBatchCallbackTest006 putBatch success');
                expect(err == undefined).assertTrue();
                await kvStore.getEntries(localDeviceId, 'batch_test_bool_key', function (err,entrys) {
                    console.info('DeviceKvStorePutBatchCallbackTest006 getEntries success');
                    console.info('DeviceKvStorePutBatchCallbackTest006 entrys.length: ' + entrys.length);
                    console.info('DeviceKvStorePutBatchCallbackTest006 entrys[0]: ' + JSON.stringify(entrys[0]));
                    expect(entrys.length == 10).assertTrue();
                    expect(entrys[0].value.value.toString() == arr.toString()).assertTrue();
                    done();
                });
            });
        }catch(e) {
            console.error('DeviceKvStorePutBatchCallbackTest006 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStoreDeleteBatchCallbackTest001
     * @tc.desc Test Js Api DeviceKvStore.DeleteBatch testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreDeleteBatchCallbackTest001', 0, async function (done) {
        console.info('DeviceKvStoreDeleteBatchCallbackTest001');
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
            console.info('DeviceKvStoreDeleteBatchCallbackTest001 entries: ' + JSON.stringify(entries));
            await kvStore.putBatch(entries, async function (err,data) {
                console.info('DeviceKvStoreDeleteBatchCallbackTest001 putBatch success');
                expect(err == undefined).assertTrue();
                await kvStore.deleteBatch(keys, async function (err,data) {
                    console.info('DeviceKvStoreDeleteBatchCallbackTest001 deleteBatch success');
                    expect(err == undefined).assertTrue();
                    done();
                });
            });
        }catch(e) {
            console.error('DeviceKvStoreDeleteBatchCallbackTest001 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStoreDeleteBatchCallbackTest002
     * @tc.desc Test Js Api DeviceKvStore.DeleteBatch testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreDeleteBatchCallbackTest002', 0, async function (done) {
        console.info('DeviceKvStoreDeleteBatchCallbackTest002');
        try {
            let keys = ['batch_test_string_key1', 'batch_test_string_key2'];
            await kvStore.deleteBatch(keys, function (err,data) {
                console.info('DeviceKvStoreDeleteBatchCallbackTest002 deleteBatch success');
                expect(err == undefined).assertTrue();
                done();
            });
        }catch(e) {
            console.error('DeviceKvStoreDeleteBatchCallbackTest002 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStoreDeleteBatchCallbackTest003
     * @tc.desc Test Js Api DeviceKvStore.DeleteBatch testcase 003
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreDeleteBatchCallbackTest003', 0, async function (done) {
        console.info('DeviceKvStoreDeleteBatchCallbackTest003');
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
            console.info('DeviceKvStoreDeleteBatchCallbackTest003 entries: ' + JSON.stringify(entries));
            await kvStore.putBatch(entries, async function (err,data) {
                console.info('DeviceKvStoreDeleteBatchCallbackTest003 putBatch success');
                expect(err == undefined).assertTrue();
                let keys = ['batch_test_string_key1', 'batch_test_string_keya'];
                await kvStore.deleteBatch(keys, async function (err,data) {
                    console.info('DeviceKvStoreDeleteBatchCallbackTest003 deleteBatch success');
                    expect(err == undefined).assertTrue();
                    done();
                });
            });
        }catch(e) {
            console.error('DeviceKvStoreDeleteBatchCallbackTest003 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStorestartTransactionCallbackTest001
     * @tc.desc Test Js Api DeviceKvStore.startTransaction testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStorestartTransactionCallbackTest001', 0, async function (done) {
        console.info('DeviceKvStorestartTransactionCallbackTest001');
        try {
            var count = 0;
            kvStore.on('dataChange', 0, function (data) {
                console.info('DeviceKvStorestartTransactionCallbackTest001 0' + data)
                count++;
            });
            await kvStore.startTransaction(async function (err,data) {
                console.info('DeviceKvStorestartTransactionCallbackTest001 startTransaction success');
                expect(err == undefined).assertTrue();
                let entries = putBatchString(10, 'batch_test_string_key');
                console.info('DeviceKvStorestartTransactionCallbackTest001 entries: ' + JSON.stringify(entries));
                await kvStore.putBatch(entries, async function (err,data) {
                    console.info('DeviceKvStorestartTransactionCallbackTest001 putBatch success');
                    expect(err == undefined).assertTrue();
                    let keys = Object.keys(entries).slice(5); //delete 5 beginnings
                    await kvStore.deleteBatch(keys, async function (err,data) {
                        console.info('DeviceKvStorestartTransactionCallbackTest001 deleteBatch success');
                        expect(err == undefined).assertTrue();
                        await kvStore.commit(async function (err,data) {
                            console.info('DeviceKvStorestartTransactionCallbackTest001 commit success');
                            expect(err == undefined).assertTrue();
                            await sleep(2000);
                            expect(count == 1).assertTrue();
                            done();
                        });
                    });
                });
            });
        }catch(e) {
            console.error('DeviceKvStorestartTransactionCallbackTest001 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStorestartTransactionCallbackTest002
     * @tc.desc Test Js Api DeviceKvStore.startTransaction testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStorestartTransactionCallbackTest002', 0, async function (done) {
        console.info('DeviceKvStorestartTransactionCallbackTest002');
        try {
            var count = 0;
            kvStore.on('dataChange', 0, function (data) {
                console.info('DeviceKvStorestartTransactionCallbackTest002 0' + data)
                count++;
            });
            await kvStore.startTransaction(async function (err,data) {
                console.info('DeviceKvStorestartTransactionCallbackTest002 startTransaction success');
                expect(err == undefined).assertTrue();
                let entries =  putBatchString(10, 'batch_test_string_key');
                console.info('DeviceKvStorestartTransactionCallbackTest002 entries: ' + JSON.stringify(entries));
                await kvStore.putBatch(entries, async function (err,data) {
                    console.info('DeviceKvStorestartTransactionCallbackTest002 putBatch success');
                    expect(err == undefined).assertTrue();
                    let keys = Object.keys(entries).slice(5); //delete 5 beginnings
                    await kvStore.deleteBatch(keys, async function (err,data) {
                        console.info('DeviceKvStorestartTransactionCallbackTest002 deleteBatch success');
                        expect(err == undefined).assertTrue();
                        await kvStore.rollback(async function (err,data) {
                            console.info('DeviceKvStorestartTransactionCallbackTest002 rollback success');
                            expect(err == undefined).assertTrue();
                            await sleep(2000);
                            expect(count == 0).assertTrue();
                            done();
                        });
                    });
                });
            });
        }catch(e) {
            console.error('DeviceKvStorestartTransactionCallbackTest002 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStorestartTransactionCallbackTest003
     * @tc.desc Test Js Api DeviceKvStore.startTransaction testcase 003
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStorestartTransactionCallbackTest003', 0, async function (done) {
        console.info('DeviceKvStorestartTransactionCallbackTest003');
        try {
            await kvStore.startTransaction(1, function (err,data) {
                if (err == undefined) {
                    console.info('DeviceKvStorestartTransactionCallbackTest003 startTransaction success');
                    expect(null).assertFail();
                } else {
                    console.info('DeviceKvStorestartTransactionCallbackTest003 startTransaction fail');
                }
                done();
            });
        }catch(e) {
            console.error('DeviceKvStorestartTransactionCallbackTest003 e ' + `, error code is ${e.code}, message is ${e.message}`);
            done();
        }
    })

    /**
     * @tc.name DeviceKvStorestartTransactionCallbackTest004
     * @tc.desc Test Js Api DeviceKvStore.startTransaction testcase 004
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStorestartTransactionCallbackTest004', 0, async function (done) {
        console.info('DeviceKvStorestartTransactionCallbackTest004');
        try {
            await kvStore.startTransaction('test_string', function (err,data) {
                if (err == undefined) {
                    console.info('DeviceKvStorestartTransactionCallbackTest004 startTransaction success');
                    expect(null).assertFail();
                } else {
                    console.info('DeviceKvStorestartTransactionCallbackTest004 startTransaction fail');
                }
                done();
            });
        }catch(e) {
            console.error('DeviceKvStorestartTransactionCallbackTest004 e ' + `, error code is ${e.code}, message is ${e.message}`);
            done();
        }
    })

    /**
     * @tc.name DeviceKvStorestartTransactionCallbackTest005
     * @tc.desc Test Js Api DeviceKvStore.startTransaction testcase 005
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStorestartTransactionCallbackTest005', 0, async function (done) {
        console.info('DeviceKvStorestartTransactionCallbackTest005');
        try {
            await kvStore.startTransaction(2.000, function (err,data) {
                if (err == undefined) {
                    console.info('DeviceKvStorestartTransactionCallbackTest005 startTransaction success');
                    expect(null).assertFail();
                } else {
                    console.info('DeviceKvStorestartTransactionCallbackTest005 startTransaction fail');
                }
                done();
            });
        }catch(e) {
            console.error('DeviceKvStorestartTransactionCallbackTest005 e ' + `, error code is ${e.code}, message is ${e.message}`);
            done();
        }
    })

    /**
     * @tc.name DeviceKvStoreCommitCallbackTest001
     * @tc.desc Test Js Api DeviceKvStore.Commit testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreCommitCallbackTest001', 0, async function (done) {
        console.info('DeviceKvStoreCommitCallbackTest001');
        try {
            await kvStore.commit(1, function (err,data) {
                if (err == undefined) {
                    console.info('DeviceKvStoreCommitCallbackTest001 commit success');
                    expect(null).assertFail();
                } else {
                    console.info('DeviceKvStoreCommitCallbackTest001 commit fail');
                }
                done();
            });
        }catch(e) {
            console.error('DeviceKvStoreCommitCallbackTest001 e ' + `, error code is ${e.code}, message is ${e.message}`);
            done();
        }
    })

    /**
     * @tc.name DeviceKvStoreCommitCallbackTest002
     * @tc.desc Test Js Api DeviceKvStore.Commit testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreCommitCallbackTest002', 0, async function (done) {
        console.info('DeviceKvStoreCommitCallbackTest002');
        try {
            await kvStore.commit('test_string', function (err,data) {
                if (err == undefined) {
                    console.info('DeviceKvStoreCommitCallbackTest002 commit success');
                    expect(null).assertFail();
                } else {
                    console.info('DeviceKvStoreCommitCallbackTest002 commit fail');
                }
                done();
            });
        }catch(e) {
            console.error('DeviceKvStoreCommitCallbackTest002 e ' + `, error code is ${e.code}, message is ${e.message}`);
            done();
        }
    })

    /**
     * @tc.name DeviceKvStoreCommitCallbackTest003
     * @tc.desc Test Js Api DeviceKvStore.Commit testcase 003
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreCommitCallbackTest003', 0, async function (done) {
        console.info('DeviceKvStoreCommitCallbackTest003');
        try {
            await kvStore.commit(2.000, function (err,data) {
                if (err == undefined) {
                    console.info('DeviceKvStoreCommitCallbackTest003 commit success');
                    expect(null).assertFail();
                } else {
                    console.info('DeviceKvStoreCommitCallbackTest003 commit fail');
                }
                done();
            });
        }catch(e) {
            console.error('DeviceKvStoreCommitCallbackTest003 e ' + `, error code is ${e.code}, message is ${e.message}`);
            done();
        }
    })

    /**
     * @tc.name DeviceKvStoreRollbackCallbackTest001
     * @tc.desc Test Js Api DeviceKvStore.Rollback testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreRollbackCallbackTest001', 0, async function (done) {
        console.info('DeviceKvStoreRollbackCallbackTest001');
        try {
            await kvStore.rollback(1, function (err,data) {
                if (err == undefined) {
                    console.info('DeviceKvStoreRollbackCallbackTest001 commit success');
                    expect(null).assertFail();
                } else {
                    console.info('DeviceKvStoreRollbackCallbackTest001 commit fail');
                }
                done();
            });
        }catch(e) {
            console.error('DeviceKvStoreRollbackCallbackTest001 e ' + `, error code is ${e.code}, message is ${e.message}`);
            done();
        }
    })

    /**
     * @tc.name DeviceKvStoreRollbackCallbackTest002
     * @tc.desc Test Js Api DeviceKvStore.Rollback testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreRollbackCallbackTest002', 0, async function (done) {
        console.info('DeviceKvStoreRollbackCallbackTest002');
        try {
            await kvStore.rollback('test_string', function (err,data) {
                if (err == undefined) {
                    console.info('DeviceKvStoreRollbackCallbackTest002 commit success');
                    expect(null).assertFail();
                } else {
                    console.info('DeviceKvStoreRollbackCallbackTest002 commit fail');
                }
                done();
            });
        }catch(e) {
            console.error('DeviceKvStoreRollbackCallbackTest002 e ' + `, error code is ${e.code}, message is ${e.message}`);
            done();
        }
    })

    /**
     * @tc.name DeviceKvStoreRollbackCallbackTest003
     * @tc.desc Test Js Api DeviceKvStore.Rollback testcase 003
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreRollbackCallbackTest003', 0, async function (done) {
        console.info('DeviceKvStoreRollbackCallbackTest003');
        try {
            await kvStore.rollback(2.000, function (err,data) {
                if (err == undefined) {
                    console.info('DeviceKvStoreRollbackCallbackTest003 commit success');
                    expect(null).assertFail();
                } else {
                    console.info('DeviceKvStoreRollbackCallbackTest003 commit fail');
                }
                done();
            });
        }catch(e) {
            console.error('DeviceKvStoreRollbackCallbackTest003 e ' + `, error code is ${e.code}, message is ${e.message}`);
            done();
        }
    })

    /**
     * @tc.name DeviceKvStoreEnableSyncCallbackTest001
     * @tc.desc Test Js Api DeviceKvStore.EnableSync testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreEnableSyncCallbackTest001', 0, async function (done) {
        console.info('DeviceKvStoreEnableSyncCallbackTest001');
        try {
            await kvStore.enableSync(true, function (err,data) {
                if (err == undefined) {
                    console.info('DeviceKvStoreEnableSyncCallbackTest001 enableSync success');
                    expect(err == undefined).assertTrue();
                } else {
                    console.info('DeviceKvStoreEnableSyncCallbackTest001 enableSync fail');
                    expect(null).assertFail();
                }
                done();
            });
        }catch(e) {
            console.error('DeviceKvStoreEnableSyncCallbackTest001 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStoreEnableSyncCallbackTest002
     * @tc.desc Test Js Api DeviceKvStore.EnableSync testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreEnableSyncCallbackTest002', 0, async function (done) {
        console.info('DeviceKvStoreEnableSyncCallbackTest002');
        try {
            await kvStore.enableSync(false, function (err,data) {
                if (err == undefined) {
                    console.info('DeviceKvStoreEnableSyncCallbackTest002 enableSync success');
                    expect(err == undefined).assertTrue();
                } else {
                    console.info('DeviceKvStoreEnableSyncCallbackTest002 enableSync fail');
                    expect(null).assertFail();
                }
                done();
            });
        }catch(e) {
            console.error('DeviceKvStoreEnableSyncCallbackTest002 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStoreEnableSyncCallbackTest003
     * @tc.desc Test Js Api DeviceKvStore.EnableSync testcase 003
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreEnableSyncCallbackTest003', 0, async function (done) {
        console.info('DeviceKvStoreEnableSyncCallbackTest003');
        try {
            await kvStore.enableSync(function (err,data) {
                if (err == undefined) {
                    console.info('DeviceKvStoreEnableSyncCallbackTest003 enableSync success');
                    expect(null).assertFail();
                } else {
                    console.info('DeviceKvStoreEnableSyncCallbackTest003 enableSync fail');
                }
                done();
            });
        }catch(e) {
            console.error('DeviceKvStoreEnableSyncCallbackTest003 e ' + `, error code is ${e.code}, message is ${e.message}`);
            done();
        }
    })

    /**
     * @tc.name DeviceKvStoreEnableSyncCallbackTest004
     * @tc.desc Test Js Api DeviceKvStore.EnableSync testcase 004
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreEnableSyncCallbackTest004', 0, async function (done) {
        console.info('DeviceKvStoreEnableSyncCallbackTest004');
        try {
            await kvStore.enableSync(null, function (err,data) {
                if (err == undefined) {
                    console.info('DeviceKvStoreEnableSyncCallbackTest004 enableSync success');
                    expect(null).assertFail();
                } else {
                    console.info('DeviceKvStoreEnableSyncCallbackTest004 enableSync fail');
                }
                done();
            });
        }catch(e) {
            console.error('DeviceKvStoreEnableSyncCallbackTest004 e ' + `, error code is ${e.code}, message is ${e.message}`);
            done();
        }
    })

    /**
     * @tc.name DeviceKvStoreRemoveDeviceDataCallbackTest001
     * @tc.desc Test Js Api DeviceKvStore.RemoveDeviceData testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreRemoveDeviceDataCallbackTest001', 0, async function (done) {
        console.info('DeviceKvStoreRemoveDeviceDataCallbackTest001');
        try {
            await kvStore.put(KEY_TEST_STRING_ELEMENT, VALUE_TEST_STRING_ELEMENT, async function (err,data) {
                console.info('DeviceKvStoreRemoveDeviceDataCallbackTest001 put success');
                expect(err == undefined).assertTrue();
                var deviceid = 'no_exist_device_id';
                await kvStore.removeDeviceData(deviceid, async function (err,data) {
                    if (err == undefined) {
                        console.info('DeviceKvStoreRemoveDeviceDataCallbackTest001 removeDeviceData success');
                        expect(null).assertFail();
                        done();
                    } else {
                        console.info('DeviceKvStoreRemoveDeviceDataCallbackTest001 removeDeviceData fail');
                        await kvStore.get(localDeviceId, KEY_TEST_STRING_ELEMENT, async function (err,data) {
                            console.info('DeviceKvStoreRemoveDeviceDataCallbackTest001 get success');
                            expect(data == VALUE_TEST_STRING_ELEMENT).assertTrue();
                            done();
                        });
                    }
                });
            });
        }catch(e) {
            console.error('DeviceKvStoreRemoveDeviceDataCallbackTest001 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStoreRemoveDeviceDataCallbackTest002
     * @tc.desc Test Js Api DeviceKvStore.RemoveDeviceData testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreRemoveDeviceDataCallbackTest002', 0, async function (done) {
        console.info('DeviceKvStoreRemoveDeviceDataCallbackTest002');
        try {
            await kvStore.removeDeviceData(function (err,data) {
                if (err == undefined) {
                    console.info('DeviceKvStoreRemoveDeviceDataCallbackTest002 removeDeviceData success');
                    expect(null).assertFail();
                } else {
                    console.info('DeviceKvStoreRemoveDeviceDataCallbackTest002 removeDeviceData fail');
                }
                done();
            });
        }catch(e) {
            console.error('DeviceKvStoreRemoveDeviceDataCallbackTest001 e ' + `, error code is ${e.code}, message is ${e.message}`);
            done();
        }
    })

    /**
     * @tc.name DeviceKvStoreRemoveDeviceDataCallbackTest003
     * @tc.desc Test Js Api DeviceKvStore.RemoveDeviceData testcase 003
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
     it('DeviceKvStoreRemoveDeviceDataCallbackTest003', 0, async function (done) {
        console.info('DeviceKvStoreRemoveDeviceDataCallbackTest003');
        try {
            await kvStore.removeDeviceData('', function (err,data) {
                if (err == undefined) {
                    console.info('DeviceKvStoreRemoveDeviceDataCallbackTest003 removeDeviceData success');
                    expect(null).assertFail();
                } else {
                    console.info('DeviceKvStoreRemoveDeviceDataCallbackTest003 removeDeviceData fail');
                }
                done();
            });
        }catch(e) {
            console.error('DeviceKvStoreRemoveDeviceDataCallbackTest003 e ' + `, error code is ${e.code}, message is ${e.message}`);
            done();
        }
    })

    /**
     * @tc.name DeviceKvStoreRemoveDeviceDataCallbackTest004
     * @tc.desc Test Js Api DeviceKvStore.RemoveDeviceData testcase 004
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreRemoveDeviceDataCallbackTest004', 0, async function (done) {
        console.info('DeviceKvStoreRemoveDeviceDataCallbackTest004');
        try {
            await kvStore.removeDeviceData(null, function (err,data) {
                if (err == undefined) {
                    console.info('DeviceKvStoreRemoveDeviceDataCallbackTest004 removeDeviceData success');
                    expect(null).assertFail();
                } else {
                    console.info('DeviceKvStoreRemoveDeviceDataCallbackTest004 removeDeviceData fail');
                }
                done();
            });
        }catch(e) {
            console.error('DeviceKvStoreRemoveDeviceDataCallbackTest004 e ' + `, error code is ${e.code}, message is ${e.message}`);
            done();
        }
    })

    /**
     * @tc.name DeviceKvStoreGetResultSetCallbackTest001
     * @tc.desc Test Js Api DeviceKvStore.GetResultSet() testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreGetResultSetCallbackTest001', 0, async function (done) {
        console.info('DeviceKvStoreGetResultSetCallbackTest001');
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
                console.info('DeviceKvStoreGetResultSetCallbackTest001 putBatch success');
                expect(err == undefined).assertTrue();
                await kvStore.getResultSet(localDeviceId, 'batch_test_string_key', async function (err, result) {
                    console.info('DeviceKvStoreGetResultSetCallbackTest001 getResultSet success');
                    resultSet = result;
                    expect(resultSet.getCount() == 10).assertTrue();
                    await kvStore.closeResultSet(resultSet, function (err, data) {
                        console.info('DeviceKvStoreGetResultSetCallbackTest001 closeResultSet success');
                        expect(err == undefined).assertTrue();
                        done();
                    })
                });
            });
        }catch(e) {
            console.error('DeviceKvStoreGetResultSetCallbackTest001 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStoreGetResultSetCallbackTest002
     * @tc.desc Test Js Api DeviceKvStore.GetResultSet() testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreGetResultSetCallbackTest002', 0, async function (done) {
        console.info('DeviceKvStoreGetResultSetCallbackTest002');
        try {
            let resultSet;
            await kvStore.getResultSet(localDeviceId, 'batch_test_string_key', async function (err, result) {
                console.info('DeviceKvStoreGetResultSetCallbackTest002 getResultSet success');
                resultSet = result;
                expect(resultSet.getCount() == 0).assertTrue();
                await kvStore.closeResultSet(resultSet, function (err, data) {
                    console.info('DeviceKvStoreGetResultSetCallbackTest002 closeResultSet success');
                    expect(err == undefined).assertTrue();
                    done();
                })
            });
        }catch(e) {
            console.error('DeviceKvStoreGetResultSetCallbackTest002 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStoreGetResultSetCallbackTest003
     * @tc.desc Test Js Api DeviceKvStore.GetResultSet() testcase 003
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreGetResultSetCallbackTest003', 0, async function (done) {
        console.info('DeviceKvStoreGetResultSetCallbackTest003');
        try {
            let resultSet;
            await kvStore.getResultSet(function (err, result) {
                console.info('DeviceKvStoreGetResultSetCallbackTest003 getResultSet success');
                expect(err != undefined).assertTrue();
                done();
            });
        }catch(e) {
            console.error('DeviceKvStoreGetResultSetCallbackTest003 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStoreGetResultSetCallbackTest004
     * @tc.desc Test Js Api DeviceKvStore.GetResultSet() testcase 004
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreGetResultSetCallbackTest004', 0, async function (done) {
        console.info('DeviceKvStoreGetResultSetCallbackTest004');
        try {
            let resultSet;
            await kvStore.getResultSet('test_key_string', 123, function (err, result) {
                console.info('DeviceKvStoreGetResultSetCallbackTest004 getResultSet success');
                expect(err != undefined).assertTrue();
                done();
            });
        }catch(e) {
            console.error('DeviceKvStoreGetResultSetCallbackTest004 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStoreGetResultSetCallbackTest005
     * @tc.desc Test Js Api DeviceKvStore.GetResultSet() testcase 005
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreGetResultSetCallbackTest005', 0, async function (done) {
        console.info('DeviceKvStoreGetResultSetCallbackTest005');
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
                console.info('DeviceKvStoreGetResultSetCallbackTest005 putBatch success');
                expect(err == undefined).assertTrue();
                var query = new factory.Query();
                query.prefixKey("batch_test");
                await kvStore.getResultSet(localDeviceId, query, async function (err, result) {
                    console.info('DeviceKvStoreGetResultSetCallbackTest005 getResultSet success');
                    resultSet = result;
                    expect(resultSet.getCount() == 10).assertTrue();
                    await kvStore.closeResultSet(resultSet, function (err, data) {
                        console.info('DeviceKvStoreGetResultSetCallbackTest005 closeResultSet success');
                        expect(err == undefined).assertTrue();
                        done();
                    })
                });
            });
        } catch(e) {
            console.error('DeviceKvStoreGetResultSetCallbackTest005 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStoreGetResultSetCallbackTest006
     * @tc.desc Test Js Api DeviceKvStore.GetResultSet() testcase 006
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreGetResultSetCallbackTest006', 0, async function (done) {
        console.info('DeviceKvStoreGetResultSetCallbackTest006');
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
                console.info('DeviceKvStoreGetResultSetCallbackTest006 putBatch success');
                expect(err == undefined).assertTrue();
                var query = new factory.Query();
                query.prefixKey("batch_test");
                query.deviceId(localDeviceId);
                await kvStore.getResultSet(query, async function (err, result) {
                    console.info('DeviceKvStoreGetResultSetCallbackTest006 getResultSet success');
                    resultSet = result;
                    expect(resultSet.getCount() == 10).assertTrue();
                    await kvStore.closeResultSet(resultSet, function (err, data) {
                        console.info('DeviceKvStoreGetResultSetCallbackTest006 closeResultSet success');
                        expect(err == undefined).assertTrue();
                        done();
                    })
                });
            });
        } catch(e) {
            console.error('DeviceKvStoreGetResultSetCallbackTest006 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStoreCloseResultSetCallbackTest001
     * @tc.desc Test Js Api DeviceKvStore.CloseResultSet testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreCloseResultSetCallbackTest001', 0, async function (done) {
        console.info('DeviceKvStoreCloseResultSetCallbackTest001');
        try {
            console.info('DeviceKvStoreCloseResultSetCallbackTest001 success');
            let resultSet = null;
            await kvStore.closeResultSet(resultSet, function (err, data) {
                if (err == undefined) {
                    console.info('DeviceKvStoreCloseResultSetCallbackTest001 closeResultSet success');
                    expect(null).assertFail();
                } else {
                    console.info('DeviceKvStoreCloseResultSetCallbackTest001 closeResultSet fail');
                }
                done();
            });
        }catch(e) {
            console.error('DeviceKvStoreCloseResultSetCallbackTest001 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStoreCloseResultSetCallbackTest002
     * @tc.desc Test Js Api DeviceKvStore.CloseResultSet testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreCloseResultSetCallbackTest002', 0, async function (done) {
        console.info('DeviceKvStoreCloseResultSetCallbackTest002');
        try {
            let resultSet = null;
            await kvStore.getResultSet(localDeviceId, 'batch_test_string_key', async function(err, result) {
                console.info('DeviceKvStoreCloseResultSetCallbackTest002 getResultSet success');
                resultSet = result;
                await kvStore.closeResultSet(resultSet, function (err, data) {
                    if (err == undefined) {
                        console.info('DeviceKvStoreCloseResultSetCallbackTest002 closeResultSet success');
                        expect(err == undefined).assertTrue();
                    } else {
                        console.info('DeviceKvStoreCloseResultSetCallbackTest002 closeResultSet fail');
                        expect(null).assertFail();
                    }
                    done();
                });
            });
        }catch(e) {
            console.error('DeviceKvStoreCloseResultSetCallbackTest002 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStoreCloseResultSetCallbackTest003
     * @tc.desc Test Js Api DeviceKvStore.CloseResultSet testcase 003
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreCloseResultSetCallbackTest003', 0, async function (done) {
        console.info('DeviceKvStoreCloseResultSetCallbackTest003');
        try {
            console.info('DeviceKvStoreCloseResultSetCallbackTest003 success');
            await kvStore.closeResultSet(function (err, data) {
                if (err == undefined) {
                    console.info('DeviceKvStoreCloseResultSetCallbackTest003 closeResultSet success');
                    expect(null).assertFail();
                } else {
                    console.info('DeviceKvStoreCloseResultSetCallbackTest003 closeResultSet fail');
                }
                done();
            });
        }catch(e) {
            console.error('DeviceKvStoreCloseResultSetCallbackTest003 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStoreCloseResultSetCallbackTest004
     * @tc.desc Test Js Api DeviceKvStore.CloseResultSet testcase 004
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreCloseResultSetCallbackTest004', 0, async function (done) {
        console.info('DeviceKvStoreCloseResultSetCallbackTest004');
        try {
            console.info('DeviceKvStoreCloseResultSetCallbackTest004 success');
        }catch(e) {
            console.error('DeviceKvStoreCloseResultSetCallbackTest004 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreGetCallbackTest001
     * @tc.desc Test Js Api DeviceKvStore.Get(ResultSize) testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreGetResultSizeCallbackTest001', 0, async function (done) {
        console.info('DeviceKvStoreGetResultSizeCallbackTest001');
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
                console.info('DeviceKvStoreGetResultSizeCallbackTest001 putBatch success');
                expect(err == undefined).assertTrue();
                var query = new factory.Query();
                query.prefixKey("batch_test");
                query.deviceId(localDeviceId);
                await kvStore.getResultSize(query, async function (err, resultSize) {
                    console.info('DeviceKvStoreGetResultSizeCallbackTest001 getResultSet success');
                    expect(resultSize == 10).assertTrue();
                    done();
                });
            });
        } catch(e) {
            console.error('DeviceKvStoreGetResultSizeCallbackTest001 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStoreGetCallbackTest002
     * @tc.desc Test Js Api DeviceKvStore.Get(ResultSize) testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreGetResultSizeCallbackTest002', 0, async function (done) {
        console.info('DeviceKvStoreGetResultSizeCallbackTest002');
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
                console.info('DeviceKvStoreGetResultSizeCallbackTest002 putBatch success');
                expect(err == undefined).assertTrue();
                var query = new factory.Query();
                query.prefixKey("batch_test");
                await kvStore.getResultSize(localDeviceId, query, async function (err, resultSize) {
                    console.info('DeviceKvStoreGetResultSizeCallbackTest002 getResultSet success');
                    expect(resultSize == 10).assertTrue();
                    done();
                });
            });
        } catch(e) {
            console.error('DeviceKvStoreGetResultSizeCallbackTest002 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStoreGetEntriesCallbackTest001
     * @tc.desc Test Js Api DeviceKvStore.GetEntries() testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreGetEntriesCallbackTest001', 0, async function (done) {
        console.info('DeviceKvStoreGetEntriesCallbackTest001');
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
            console.info('DeviceKvStoreGetEntriesCallbackTest001 entries: ' + JSON.stringify(entries));
            await kvStore.putBatch(entries, async function (err,data) {
                console.info('DeviceKvStoreGetEntriesCallbackTest001 putBatch success');
                expect(err == undefined).assertTrue();
                var query = new factory.Query();
                query.deviceId(localDeviceId);
                query.prefixKey("batch_test");
                await kvStore.getEntries(localDeviceId, query, function (err,entrys) {
                    console.info('DeviceKvStoreGetEntriesCallbackTest001 getEntries success');
                    console.info('DeviceKvStoreGetEntriesCallbackTest001 entrys.length: ' + entrys.length);
                    console.info('DeviceKvStoreGetEntriesCallbackTest001 entrys[0]: ' + JSON.stringify(entrys[0]));
                    expect(entrys.length == 10).assertTrue();
                    expect(entrys[0].value.value.toString() == arr.toString()).assertTrue();
                    done();
                });
            });
            console.info('DeviceKvStoreGetEntriesCallbackTest001 success');
        }catch(e) {
            console.error('DeviceKvStoreGetEntriesCallbackTest001 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreGetEntriesCallbackTest002
     * @tc.desc Test Js Api DeviceKvStore.GetEntries() testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreGetEntriesCallbackTest002', 0, async function (done) {
        console.info('DeviceKvStoreGetEntriesCallbackTest002');
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
            console.info('DeviceKvStoreGetEntriesCallbackTest002 entries: ' + JSON.stringify(entries));
            await kvStore.putBatch(entries, async function (err,data) {
                console.info('DeviceKvStoreGetEntriesCallbackTest002 putBatch success');
                expect(err == undefined).assertTrue();
                var query = new factory.Query();
                query.prefixKey("batch_test");
                query.deviceId(localDeviceId);
                await kvStore.getEntries(query, function (err,entrys) {
                    console.info('DeviceKvStoreGetEntriesCallbackTest002 getEntries success');
                    console.info('DeviceKvStoreGetEntriesCallbackTest002 entrys.length: ' + entrys.length);
                    console.info('DeviceKvStoreGetEntriesCallbackTest002 entrys[0]: ' + JSON.stringify(entrys[0]));
                    expect(entrys.length == 10).assertTrue();
                    expect(entrys[0].value.value.toString() == arr.toString()).assertTrue();
                    done();
                });
            });
            console.info('DeviceKvStoreGetEntriesCallbackTest001 success');
            console.info('DeviceKvStoreGetEntriesCallbackTest002 success');
        }catch(e) {
            console.error('DeviceKvStoreGetEntriesCallbackTest002 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })
})
