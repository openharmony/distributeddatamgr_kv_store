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
import dataSharePredicates from '@ohos.data.dataSharePredicates';
import abilityFeatureAbility from '@ohos.ability.featureAbility'

var context = abilityFeatureAbility.getContext();
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
var localDeviceId = null;
// const USED_DEVICE_IDS = ['A12C1F9261528B21F95778D2FDC0B2E33943E6251AC5487F4473D005758905DB'];
const USED_DEVICE_IDS = ['7001005458323933328a00bce0493800'];
const UNUSED_DEVICE_IDS = ['7001005458323933328a01bce1ca3800'];  /* add you test device-ids here */
var syncDeviceIds = USED_DEVICE_IDS.concat(UNUSED_DEVICE_IDS);

function sleep(ms) {
    return new Promise(resolve => setTimeout(resolve, ms));
}

function putBatchString(len, prefix) {
    let entries = [];
    for (let i = 0; i < len; i++) {
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

describe('deviceKvStoreCallbackTest', function () {
    const config = {
        bundleName: TEST_BUNDLE_NAME,
        context: context
    }

    const options = {
        createIfMissing: true,
        encrypt: false,
        backup: false,
        autoSync: true,
        kvStoreType: factory.KVStoreType.DEVICE_COLLABORATION,
        schema: '',
        securityLevel: factory.SecurityLevel.S2,
    }

    beforeAll(async function (done) {
        console.info('beforeAll config:' + JSON.stringify(config));
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
        await getDeviceId.then(function (deviceId) {
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
        await kvManager.closeKVStore(TEST_BUNDLE_NAME, TEST_STORE_ID, async function (err, data) {
            console.info('afterEach closeKVStore success');
            await kvManager.deleteKVStore(TEST_BUNDLE_NAME, TEST_STORE_ID, function (err, data) {
                console.info('afterEach deleteKVStore success');
                done();
            });
        });
        kvStore = null;
    })

    /**
     * @tc.name DeviceKvStorePutStringCallbackSucTest
     * @tc.desc Test Js Api DeviceKvStore.Put(String) successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStorePutStringCallbackSucTest', 0, async function (done) {
        console.info('DeviceKvStorePutStringCallbackSucTest');
        try {
            await kvStore.put(KEY_TEST_STRING_ELEMENT, VALUE_TEST_STRING_ELEMENT, function (err, data) {
                if (err == undefined) {
                    console.info('DeviceKvStorePutStringCallbackSucTest put success');
                    expect(err == undefined).assertTrue();
                } else {
                    console.error('DeviceKvStorePutStringCallbackSucTest put fail' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(null).assertFail();
                }
                done();
            });
        } catch (e) {
            console.error('DeviceKvStorePutStringCallbackSucTest put e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStorePutStringCallbackLongStringTest
     * @tc.desc Test Js Api DeviceKvStore.Put(String) put a long string
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStorePutStringCallbackLongStringTest', 0, async function (done) {
        console.info('DeviceKvStorePutStringCallbackLongStringTest');
        try {
            var str = '';
            for (var i = 0; i < 4095; i++) {
                str += 'x';
            }
            await kvStore.put(KEY_TEST_STRING_ELEMENT + '102', str, async function (err, data) {
                console.info('DeviceKvStorePutStringCallbackLongStringTest put success');
                expect(err == undefined).assertTrue();
                await kvStore.get(localDeviceId, KEY_TEST_STRING_ELEMENT + '102', function (err, data) {
                    console.info('DeviceKvStorePutStringCallbackLongStringTest get success');
                    expect(str == data).assertTrue();
                    done();
                });
            });
        } catch (e) {
            console.error('DeviceKvStorePutStringCallbackLongStringTest put e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStoreGetStringCallbackNonExistingTest
     * @tc.desc Test Js Api DeviceKvStore.Get(String) get a non-existing string
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreGetStringCallbackNonExistingTest', 0, async function (done) {
        console.info('DeviceKvStoreGetStringCallbackNonExistingTest');
        try {
            await kvStore.get(localDeviceId, KEY_TEST_STRING_ELEMENT, function (err, data) {
                if (err == undefined) {
                    console.info('DeviceKvStoreGetStringCallbackNonExistingTest get success');
                    expect(null).assertFail();
                } else {
                    console.info('DeviceKvStoreGetStringCallbackNonExistingTest get fail');
                    expect(true).assertTrue();
                }
                done();
            });
        } catch (e) {
            console.error('DeviceKvStoreGetStringCallbackNonExistingTest get e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStoreGetStringCallbackSucTest
     * @tc.desc Test Js Api DeviceKvStore.Get(String) successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreGetStringCallbackSucTest', 0, async function (done) {
        console.info('DeviceKvStoreGetStringCallbackSucTest');
        try {
            await kvStore.put(KEY_TEST_STRING_ELEMENT, VALUE_TEST_STRING_ELEMENT, async function (err, data) {
                console.info('DeviceKvStoreGetStringCallbackSucTest put success');
                expect(err == undefined).assertTrue();
                await kvStore.get(localDeviceId, KEY_TEST_STRING_ELEMENT, function (err, data) {
                    console.info('DeviceKvStoreGetStringCallbackSucTest get success');
                    expect((err == undefined) && (VALUE_TEST_STRING_ELEMENT == data)).assertTrue();
                    done();
                });
            })
        } catch (e) {
            console.error('DeviceKvStoreGetStringCallbackSucTest get e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStorePutIntCallbackSucTest
     * @tc.desc Test Js Api DeviceKvStore.Put(Int) successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStorePutIntCallbackSucTest', 0, async function (done) {
        console.info('DeviceKvStorePutIntCallbackSucTest');
        try {
            await kvStore.put(KEY_TEST_INT_ELEMENT, VALUE_TEST_INT_ELEMENT, async function (err, data) {
                console.info('DeviceKvStorePutIntCallbackSucTest put success');
                expect(err == undefined).assertTrue();
                await kvStore.get(localDeviceId, KEY_TEST_INT_ELEMENT, function (err, data) {
                    console.info('DeviceKvStorePutIntCallbackSucTest get success');
                    expect((err == undefined) && (VALUE_TEST_INT_ELEMENT == data)).assertTrue();
                    done();
                })
            });
        } catch (e) {
            console.error('DeviceKvStorePutIntCallbackSucTest put e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStorePutIntCallbackMinTest
     * @tc.desc Test Js Api DeviceKvStore.Put(Int) with min value
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStorePutIntCallbackMinTest', 0, async function (done) {
        console.info('DeviceKvStorePutIntCallbackMinTest');
        try {
            var intValue = Number.MIN_VALUE;
            await kvStore.put(KEY_TEST_INT_ELEMENT, intValue, async function (err, data) {
                console.info('DeviceKvStorePutIntCallbackMinTest put success');
                expect(err == undefined).assertTrue();
                await kvStore.get(localDeviceId, KEY_TEST_INT_ELEMENT, function (err, data) {
                    console.info('DeviceKvStorePutIntCallbackMinTest get success');
                    expect((err == undefined) && (intValue == data)).assertTrue();
                    done();
                })
            });
        } catch (e) {
            console.error('DeviceKvStorePutIntCallbackMinTest put e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStorePutIntCallbackMaxTest
     * @tc.desc Test Js Api DeviceKvStore.Put(Int) with max value
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStorePutIntCallbackMaxTest', 0, async function (done) {
        console.info('DeviceKvStorePutIntCallbackMaxTest');
        try {
            var intValue = Number.MAX_VALUE;
            await kvStore.put(KEY_TEST_INT_ELEMENT, intValue, async function (err, data) {
                console.info('DeviceKvStorePutIntCallbackMaxTest put success');
                expect(err == undefined).assertTrue();
                await kvStore.get(localDeviceId, KEY_TEST_INT_ELEMENT, function (err, data) {
                    console.info('DeviceKvStorePutIntCallbackMaxTest get success');
                    expect((err == undefined) && (intValue == data)).assertTrue();
                    done();
                })
            });
        } catch (e) {
            console.error('DeviceKvStorePutIntCallbackMaxTest put e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStoreGetIntCallbackSucTest
     * @tc.desc Test Js Api DeviceKvStore.Get(Int) successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreGetIntCallbackSucTest', 0, async function (done) {
        console.info('DeviceKvStoreGetIntCallbackSucTest');
        try {
            await kvStore.put(KEY_TEST_INT_ELEMENT, VALUE_TEST_INT_ELEMENT, async function (err, data) {
                console.info('DeviceKvStoreGetIntCallbackSucTest put success');
                expect(err == undefined).assertTrue();
                await kvStore.get(localDeviceId, KEY_TEST_INT_ELEMENT, function (err, data) {
                    console.info('DeviceKvStoreGetIntCallbackSucTest get success');
                    expect((err == undefined) && (VALUE_TEST_INT_ELEMENT == data)).assertTrue();
                    done();
                })
            });
        } catch (e) {
            console.error('DeviceKvStoreGetIntCallbackSucTest put e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStoreGetIntCallbackNonExistTest
     * @tc.desc Test Js Api DeviceKvStore.Get(Int) get non-existing int
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreGetIntCallbackNonExistTest', 0, async function (done) {
        console.info('DeviceKvStoreGetIntCallbackNonExistTest');
        try {
            await kvStore.get(localDeviceId, KEY_TEST_INT_ELEMENT, function (err, data) {
                if (err == undefined) {
                    console.info('DeviceKvStoreGetIntCallbackNonExistTest get success');
                    expect(null).assertFail();
                } else {
                    console.info('DeviceKvStoreGetIntCallbackNonExistTest get fail');
                    expect(true).assertTrue();
                }
                done();
            })
        } catch (e) {
            console.error('DeviceKvStoreGetIntCallbackNonExistTest put e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStorePutBoolCallbackSucTest
     * @tc.desc Test Js Api DeviceKvStore.Put(Bool) successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStorePutBoolCallbackSucTest', 0, async function (done) {
        console.info('DeviceKvStorePutBoolCallbackSucTest');
        try {
            await kvStore.put(KEY_TEST_BOOLEAN_ELEMENT, VALUE_TEST_BOOLEAN_ELEMENT, function (err, data) {
                console.info('DeviceKvStorePutBoolCallbackSucTest put success');
                expect(err == undefined).assertTrue();
                done();
            });
        } catch (e) {
            console.error('DeviceKvStorePutBoolCallbackSucTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStoreGetBoolCallbackNonExistTest
     * @tc.desc Test Js Api DeviceKvStore.Get(Bool) get non-existing bool
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreGetBoolCallbackNonExistTest', 0, async function (done) {
        console.info('DeviceKvStoreGetBoolCallbackNonExistTest');
        try {
            await kvStore.get(localDeviceId, KEY_TEST_BOOLEAN_ELEMENT, function (err, data) {
                if (err == undefined) {
                    console.info('DeviceKvStoreGetBoolCallbackNonExistTest get success');
                    expect(null).assertFail();
                } else {
                    console.error('DeviceKvStoreGetBoolCallbackNonExistTest get fail' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(true).assertTrue();
                }
                done();
            });
        } catch (e) {
            console.error('DeviceKvStoreGetBoolCallbackNonExistTest e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStoreGetBoolCallbackSucTest
     * @tc.desc Test Js Api DeviceKvStore.Get(Bool) successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreGetBoolCallbackSucTest', 0, async function (done) {
        console.info('DeviceKvStoreGetBoolCallbackSucTest');
        try {
            await kvStore.put(KEY_TEST_BOOLEAN_ELEMENT, VALUE_TEST_BOOLEAN_ELEMENT, async function (err, data) {
                console.info('DeviceKvStoreGetBoolCallbackSucTest put success');
                expect(err == undefined).assertTrue();
                await kvStore.get(localDeviceId, KEY_TEST_BOOLEAN_ELEMENT, function (err, data) {
                    console.info('DeviceKvStoreGetBoolCallbackSucTest get success');
                    expect((err == undefined) && (VALUE_TEST_BOOLEAN_ELEMENT == data)).assertTrue();
                    done();
                });
            })
        } catch (e) {
            console.error('DeviceKvStoreGetBoolCallbackSucTest e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStorePutFloatCallbackSucTest
     * @tc.desc Test Js Api DeviceKvStore.Put(Float) successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStorePutFloatCallbackSucTest', 0, async function (done) {
        console.info('DeviceKvStorePutFloatCallbackSucTest');
        try {
            await kvStore.put(KEY_TEST_FLOAT_ELEMENT, VALUE_TEST_FLOAT_ELEMENT, function (err, data) {
                console.info('DeviceKvStorePutFloatCallbackSucTest put success');
                expect(err == undefined).assertTrue();
                done();
            });
        } catch (e) {
            console.error('DeviceKvStorePutFloatCallbackSucTest e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStoreGetFloatCallbackNonExistTest
     * @tc.desc Test Js Api DeviceKvStore.Get(Float) get non-existing float
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreGetFloatCallbackNonExistTest', 0, async function (done) {
        console.info('DeviceKvStoreGetFloatCallbackNonExistTest');
        try {
            await kvStore.get(localDeviceId, KEY_TEST_FLOAT_ELEMENT, function (err, data) {
                if (err == undefined) {
                    console.info('DeviceKvStoreGetFloatCallbackNonExistTest get success');
                    expect(null).assertFail();
                } else {
                    console.error('DeviceKvStoreGetFloatCallbackNonExistTest get fail' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(err.code == 15100004).assertTrue();
                }
                done();
            });
        } catch (e) {
            console.error('DeviceKvStoreGetFloatCallbackNonExistTest e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStoreDeleteStringCallbackTest
     * @tc.desc Test Js Api DeviceKvStore.DeleteString() testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreDeleteStringCallbackTest', 0, async function (done) {
        console.info('DeviceKvStoreDeleteStringCallbackTest');
        try {
            await kvStore.delete(KEY_TEST_STRING_ELEMENT, function (err, data) {
                if (err == undefined) {
                    console.info('DeviceKvStoreDeleteStringCallbackTest delete success');
                    expect(err == undefined).assertTrue();
                } else {
                    console.error('DeviceKvStoreDeleteStringCallbackTest delete fail' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(null).assertFail();
                }
                done();
            });
        } catch (e) {
            console.error('DeviceKvStoreDeleteStringCallbackTest e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStoreDeleteStringCallbackInvalidKeyTest
     * @tc.desc Test Js Api DeviceKvStore.DeleteString() with invalid key name
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreDeleteStringCallbackInvalidKeyTest', 0, async function (done) {
        console.info('DeviceKvStoreDeleteStringCallbackInvalidKeyTest');
        try {
            await kvStore.delete('', function (err, data) {
                if (err == undefined) {
                    console.info('DeviceKvStoreDeleteStringCallbackInvalidKeyTest delete success');
                    expect(null).assertFail();
                } else {
                    console.error('DeviceKvStoreDeleteStringCallbackInvalidKeyTest delete fail' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(err.code == 401).assertTrue();
                }
                done();
            });
        } catch (e) {
            console.error('DeviceKvStoreDeleteStringCallbackInvalidKeyTest e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStoreDeleteStringCallbackTest
     * @tc.desc Test Js Api DeviceKvStore.DeleteString() testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreDeleteStringCallbackTest', 0, async function (done) {
        console.info('DeviceKvStoreDeleteStringCallbackTest');
        try {
            await kvStore.put(KEY_TEST_STRING_ELEMENT, VALUE_TEST_STRING_ELEMENT, async function (err, data) {
                console.info('DeviceKvStoreDeleteStringCallbackTest put success');
                expect(err == undefined).assertTrue();
                await kvStore.delete(KEY_TEST_STRING_ELEMENT, function (err, data) {
                    console.info('DeviceKvStoreDeleteStringCallbackTest delete success');
                    expect(err == undefined).assertTrue();
                    done();
                });
            })
        } catch (e) {
            console.error('DeviceKvStoreDeleteStringCallbackTest e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStoreOnChangeCallbackType0Test
     * @tc.desc Test Js Api DeviceKvStore.OnChange() type 0
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreOnChangeCallbackType0Test', 0, async function (done) {
        console.info('DeviceKvStoreOnChangeCallbackType0Test');
        try {
            kvStore.on('dataChange', 0, function (data) {
                console.info('DeviceKvStoreOnChangeCallbackType0Test dataChange');
                expect(data != null).assertTrue();
            });
            await kvStore.put(KEY_TEST_FLOAT_ELEMENT, VALUE_TEST_FLOAT_ELEMENT, function (err, data) {
                console.info('DeviceKvStoreOnChangeCallbackType0Test put success');
                expect(err == undefined).assertTrue();
                done();
            });
        } catch (e) {
            console.error('DeviceKvStoreOnChangeCallbackType0Test e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStoreOnChangeCallbackType1Test
     * @tc.desc Test Js Api DeviceKvStore.OnChange() testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreOnChangeCallbackType1Test', 0, async function (done) {
        console.info('DeviceKvStoreOnChangeCallbackType1Test');
        try {
            kvStore.on('dataChange', 1, function (data) {
                console.info('DeviceKvStoreOnChangeCallbackType1Test dataChange');
                expect(data != null).assertTrue();
            });
            await kvStore.put(KEY_TEST_FLOAT_ELEMENT, VALUE_TEST_FLOAT_ELEMENT, function (err, data) {
                console.info('DeviceKvStoreOnChangeCallbackType1Test put success');
                expect(err == undefined).assertTrue();
                done();
            });
        } catch (e) {
            console.error('DeviceKvStoreOnChangeCallbackType1Test e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStoreOnChangeCallbackType2Test
     * @tc.desc Test Js Api DeviceKvStore.OnChange() type 2
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreOnChangeCallbackType2Test', 0, async function (done) {
        console.info('DeviceKvStoreOnChangeCallbackType2Test');
        try {
            kvStore.on('dataChange', 2, function (data) {
                console.info('DeviceKvStoreOnChangeCallbackType2Test dataChange');
                expect(data != null).assertTrue();
            });
            await kvStore.put(KEY_TEST_FLOAT_ELEMENT, VALUE_TEST_FLOAT_ELEMENT, function (err, data) {
                console.info('DeviceKvStoreOnChangeCallbackType2Test put success');
                expect(err == undefined).assertTrue();
                done();
            });
        } catch (e) {
            console.error('DeviceKvStoreOnChangeCallbackType2Test e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStoreOnSyncCompleteCallbackPullOnlyTest
     * @tc.desc Test Js Api DeviceKvStore.OnSyncComplete() pull only
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreOnSyncCompleteCallbackPullOnlyTest', 0, async function (done) {
        try {
            kvStore.on('syncComplete', function (data) {
                console.info('DeviceKvStoreOnSyncCompleteCallbackPullOnlyTest dataChange');
                expect(data != null).assertTrue();
            });
            await kvStore.put(KEY_TEST_SYNC_ELEMENT + 'testSync101', VALUE_TEST_SYNC_ELEMENT).then((data) => {
                console.info('DeviceKvStoreOnSyncCompleteCallbackPullOnlyTest put success');
                expect(data == undefined).assertTrue();
            }).catch((error) => {
                console.error('DeviceKvStoreOnSyncCompleteCallbackPullOnlyTest put failed:' + `, error code is ${e.code}, message is ${e.message}`);
                expect(null).assertFail();
            });
            try {
                var mode = factory.SyncMode.PULL_ONLY;
                console.info('kvStore.sync to ' + JSON.stringify(syncDeviceIds));
                kvStore.sync(syncDeviceIds, mode);
                expect(true).assertTrue();
            } catch (e) {
                console.error('DeviceKvStoreOnSyncCompleteCallbackPullOnlyTest sync no peer device :e:' + `, error code is ${e.code}, message is ${e.message}`);
                expect(null).assertFail();
            }
        } catch (e) {
            console.error('DeviceKvStoreOnSyncCompleteCallbackPullOnlyTest no peer device :e:' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreOnSyncCompleteCallbackPushOnlyTest
     * @tc.desc Test Js Api DeviceKvStore.OnSyncComplete() push only
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreOnSyncCompleteCallbackPushOnlyTest', 0, async function (done) {
        try {
            kvStore.on('syncComplete', function (data) {
                console.info('DeviceKvStoreOnSyncCompleteCallbackPushOnlyTest dataChange');
                expect(data != null).assertTrue();
            });
            await kvStore.put(KEY_TEST_SYNC_ELEMENT + 'testSync101', VALUE_TEST_SYNC_ELEMENT).then((data) => {
                console.info('DeviceKvStoreOnSyncCompleteCallbackPushOnlyTest put success');
                expect(data == undefined).assertTrue();
            }).catch((error) => {
                console.error('DeviceKvStoreOnSyncCompleteCallbackPushOnlyTest put failed:' + `, error code is ${e.code}, message is ${e.message}`);
                expect(null).assertFail();
            });
            try {
                var mode = factory.SyncMode.PUSH_ONLY;
                console.info('kvStore.sync to ' + JSON.stringify(syncDeviceIds));
                kvStore.sync(syncDeviceIds, mode);
                expect(true).assertTrue();
            } catch (e) {
                console.error('DeviceKvStoreOnSyncCompleteCallbackPushOnlyTest sync no peer device :e:' + `, error code is ${e.code}, message is ${e.message}`);
                expect(null).assertFail();
            }
        } catch (e) {
            console.error('DeviceKvStoreOnSyncCompleteCallbackPushOnlyTest no peer device :e:' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreOnSyncCompleteCallbackPushPullTest
     * @tc.desc Test Js Api DeviceKvStore.OnSyncComplete() pull push
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreOnSyncCompleteCallbackPushPullTest', 0, async function (done) {
        try {
            kvStore.on('syncComplete', function (data) {
                console.info('DeviceKvStoreOnSyncCompleteCallbackPushPullTest dataChange');
                expect(data != null).assertTrue();
            });
            await kvStore.put(KEY_TEST_SYNC_ELEMENT + 'testSync101', VALUE_TEST_SYNC_ELEMENT).then((data) => {
                console.info('DeviceKvStoreOnSyncCompleteCallbackPushPullTest put success');
                expect(data == undefined).assertTrue();
            }).catch((error) => {
                console.error('DeviceKvStoreOnSyncCompleteCallbackPushPullTest put failed:' + `, error code is ${e.code}, message is ${e.message}`);
                expect(null).assertFail();
            });
            try {
                var mode = factory.SyncMode.PUSH_PULL;
                console.info('kvStore.sync to ' + JSON.stringify(syncDeviceIds));
                kvStore.sync(syncDeviceIds, mode);
                expect(true).assertTrue();
            } catch (e) {
                console.error('DeviceKvStoreOnSyncCompleteCallbackPushPullTest sync no peer device :e:' + `, error code is ${e.code}, message is ${e.message}`);
                expect(null).assertFail();
            }
        } catch (e) {
            console.error('DeviceKvStoreOnSyncCompleteCallbackPushPullTest no peer device :e:' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreSetSyncRangeCallbackDisjointTest
     * @tc.desc Test Js Api DeviceKvStore.SetSyncRange() with disjoint ranges
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreSetSyncRangeCallbackDisjointTest', 0, async function (done) {
        console.info('DeviceKvStoreSetSyncRangeCallbackDisjointTest');
        try {
            var localLabels = ['A', 'B'];
            var remoteSupportLabels = ['C', 'D'];
            await kvStore.setSyncRange(localLabels, remoteSupportLabels, function (err, data) {
                console.info('DeviceKvStoreSetSyncRangeCallbackDisjointTest put success');
                expect(err == undefined).assertTrue();
                done();
            });
        } catch (e) {
            console.error('DeviceKvStoreSetSyncRangeCallbackDisjointTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStoreSetSyncRangeCallbackJointTest
     * @tc.desc Test Js Api DeviceKvStore.SetSyncRange() with joint ranges
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreSetSyncRangeCallbackTest', 0, async function (done) {
        console.info('DeviceKvStoreSetSyncRangeCallbackTest');
        try {
            var localLabels = ['A', 'B'];
            var remoteSupportLabels = ['B', 'C'];
            await kvStore.setSyncRange(localLabels, remoteSupportLabels, function (err, data) {
                console.info('DeviceKvStoreSetSyncRangeCallbackTest put success');
                expect(err == undefined).assertTrue();
                done();
            });
        } catch (e) {
            console.error('DeviceKvStoreSetSyncRangeCallbackTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStoreSetSyncRangeCallbackSameTest
     * @tc.desc Test Js Api DeviceKvStore.SetSyncRange() with same ranges
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreSetSyncRangeCallbackSameTest', 0, async function (done) {
        console.info('DeviceKvStoreSetSyncRangeCallbackSameTest');
        try {
            var localLabels = ['A', 'B'];
            var remoteSupportLabels = ['A', 'B'];
            await kvStore.setSyncRange(localLabels, remoteSupportLabels, function (err, data) {
                console.info('DeviceKvStoreSetSyncRangeCallbackSameTest put success');
                expect(err == undefined).assertTrue();
                done();
            });
        } catch (e) {
            console.error('DeviceKvStoreSetSyncRangeCallbackSameTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStorePutBatchCallbackStringTest
     * @tc.desc Test Js Api DeviceKvStore.Put(Batch) with string value
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStorePutBatchCallbackStringTest', 0, async function (done) {
        console.info('DeviceKvStorePutBatchCallbackStringTest');
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
            console.info('DeviceKvStorePutBatchCallbackStringTest entries: ' + JSON.stringify(entries));
            await kvStore.putBatch(entries, async function (err, data) {
                console.info('DeviceKvStorePutBatchCallbackStringTest putBatch success');
                expect(err == undefined).assertTrue();
                kvStore.getEntries(localDeviceId, 'batch_test_string_key', function (err, entrys) {
                    expect(entrys.length == 10).assertTrue();
                    expect(entrys[0].value.value == 'batch_test_string_value').assertTrue();
                    done();
                });
            });
        } catch (e) {
            console.error('DeviceKvStorePutBatchCallbackStringTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStorePutBatchCallbackIntegerTest
     * @tc.desc Test Js Api DeviceKvStore.Put(Batch) with int value
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStorePutBatchCallbackIntegerTest', 0, async function (done) {
        console.info('DeviceKvStorePutBatchCallbackIntegerTest');
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
            console.info('DeviceKvStorePutBatchCallbackIntegerTest entries: ' + JSON.stringify(entries));
            await kvStore.putBatch(entries, async function (err, data) {
                console.info('DeviceKvStorePutBatchCallbackIntegerTest putBatch success');
                expect(err == undefined).assertTrue();
                kvStore.getEntries(localDeviceId, 'batch_test_number_key', function (err, entrys) {
                    expect(entrys.length == 10).assertTrue();
                    expect(entrys[0].value.value == 222).assertTrue();
                    done();
                });
            });
        } catch (e) {
            console.error('DeviceKvStorePutBatchCallbackIntegerTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStorePutBatchCallbackFloatTest
     * @tc.desc Test Js Api DeviceKvStore.Put(Batch) with float value
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStorePutBatchCallbackFloatTest', 0, async function (done) {
        console.info('DeviceKvStorePutBatchCallbackFloatTest');
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
            console.info('DeviceKvStorePutBatchCallbackFloatTest entries: ' + JSON.stringify(entries));
            await kvStore.putBatch(entries, async function (err, data) {
                console.info('DeviceKvStorePutBatchCallbackFloatTest putBatch success');
                expect(err == undefined).assertTrue();
                kvStore.getEntries(localDeviceId, 'batch_test_number_key', function (err, entrys) {
                    expect(entrys.length == 10).assertTrue();
                    expect(entrys[0].value.value == 2.0).assertTrue();
                    done();
                });
            });
        } catch (e) {
            console.error('DeviceKvStorePutBatchCallbackFloatTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStorePutBatchCallbackDoubleTest
     * @tc.desc Test Js Api DeviceKvStore.Put(Batch) with double value
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStorePutBatchCallbackDoubleTest', 0, async function (done) {
        console.info('DeviceKvStorePutBatchCallbackDoubleTest');
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
            console.info('DeviceKvStorePutBatchCallbackDoubleTest entries: ' + JSON.stringify(entries));
            await kvStore.putBatch(entries, async function (err, data) {
                console.info('DeviceKvStorePutBatchCallbackDoubleTest putBatch success');
                expect(err == undefined).assertTrue();
                kvStore.getEntries(localDeviceId, 'batch_test_number_key', function (err, entrys) {
                    expect(entrys.length == 10).assertTrue();
                    expect(entrys[0].value.value == 2.00).assertTrue();
                    done();
                });
            });
        } catch (e) {
            console.error('DeviceKvStorePutBatchCallbackDoubleTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStorePutBatchCallbackBooleanTest
     * @tc.desc Test Js Api DeviceKvStore.Put(Batch) with boolean value
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStorePutBatchCallbackBooleanTest', 0, async function (done) {
        console.info('DeviceKvStorePutBatchCallbackBooleanTest');
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
            console.info('DeviceKvStorePutBatchCallbackBooleanTest entries: ' + JSON.stringify(entries));
            await kvStore.putBatch(entries, async function (err, data) {
                console.info('DeviceKvStorePutBatchCallbackBooleanTest putBatch success');
                expect(err == undefined).assertTrue();
                kvStore.getEntries(localDeviceId, 'batch_test_bool_key', function (err, entrys) {
                    expect(entrys.length == 10).assertTrue();
                    expect(entrys[0].value.value == bo).assertTrue();
                    done();
                });
            });
        } catch (e) {
            console.error('DeviceKvStorePutBatchCallbackBooleanTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStorePutBatchCallbackByteArrayTest
     * @tc.desc Test Js Api DeviceKvStore.Put(Batch) with byte array value
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStorePutBatchCallbackByteArrayTest', 0, async function (done) {
        console.info('DeviceKvStorePutBatchCallbackByteArrayTest');
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
            console.info('DeviceKvStorePutBatchCallbackByteArrayTest entries: ' + JSON.stringify(entries));
            await kvStore.putBatch(entries, async function (err, data) {
                console.info('DeviceKvStorePutBatchCallbackByteArrayTest putBatch success');
                expect(err == undefined).assertTrue();
                kvStore.getEntries(localDeviceId, 'batch_test_bool_key', function (err, entrys) {
                    expect(entrys.length == 10).assertTrue();
                    expect(entrys[0].value.value.toString() == arr.toString()).assertTrue();
                    done();
                });
            });
        } catch (e) {
            console.error('DeviceKvStorePutBatchCallbackByteArrayTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStoreDeleteBatchCallbackStringTest
     * @tc.desc Test Js Api DeviceKvStore.DeleteBatch() with string value
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreDeleteBatchCallbackStringTest', 0, async function (done) {
        console.info('DeviceKvStoreDeleteBatchCallbackStringTest');
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
            console.info('DeviceKvStoreDeleteBatchCallbackStringTest entries: ' + JSON.stringify(entries));
            await kvStore.putBatch(entries, async function (err, data) {
                console.info('DeviceKvStoreDeleteBatchCallbackStringTest putBatch success');
                expect(err == undefined).assertTrue();
                await kvStore.deleteBatch(keys, async function (err, data) {
                    console.info('DeviceKvStoreDeleteBatchCallbackStringTest deleteBatch success');
                    expect(err == undefined).assertTrue();
                    done();
                });
            });
        } catch (e) {
            console.error('DeviceKvStoreDeleteBatchCallbackStringTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStoreDeleteBatchCallbackNoBatchTest
     * @tc.desc Test Js Api DeviceKvStore.DeleteBatch() without put batches
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreDeleteBatchCallbackNoBatchTest', 0, async function (done) {
        console.info('DeviceKvStoreDeleteBatchCallbackNoBatchTest');
        try {
            let keys = ['batch_test_string_key1', 'batch_test_string_key2'];
            await kvStore.deleteBatch(keys, function (err, data) {
                console.info('DeviceKvStoreDeleteBatchCallbackNoBatchTest deleteBatch success');
                expect(err == undefined).assertTrue();
                done();
            });
        } catch (e) {
            console.error('DeviceKvStoreDeleteBatchCallbackNoBatchTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStoreDeleteBatchCallbackWrongKeysTest
     * @tc.desc Test Js Api DeviceKvStore.DeleteBatch() with wrong keys
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreDeleteBatchCallbackWrongKeysTest', 0, async function (done) {
        console.info('DeviceKvStoreDeleteBatchCallbackWrongKeysTest');
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
            console.info('DeviceKvStoreDeleteBatchCallbackWrongKeysTest entries: ' + JSON.stringify(entries));
            await kvStore.putBatch(entries, async function (err, data) {
                console.info('DeviceKvStoreDeleteBatchCallbackWrongKeysTest putBatch success');
                expect(err == undefined).assertTrue();
                let keys = ['batch_test_string_key1', 'batch_test_string_keya'];
                await kvStore.deleteBatch(keys, async function (err, data) {
                    console.info('DeviceKvStoreDeleteBatchCallbackWrongKeysTest deleteBatch success');
                    expect(err == undefined).assertTrue();
                    done();
                });
            });
        } catch (e) {
            console.error('DeviceKvStoreDeleteBatchCallbackWrongKeysTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStorestartTransactionCallbackCommitTest
     * @tc.desc Test Js Api DeviceKvStore.startTransaction() commit kvstore
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStorestartTransactionCallbackCommitTest', 0, async function (done) {
        console.info('DeviceKvStorestartTransactionCallbackCommitTest');
        try {
            var count = 0;
            kvStore.on('dataChange', 0, function (data) {
                console.info('DeviceKvStorestartTransactionCallbackCommitTest 0' + data)
                count++;
            });
            await kvStore.startTransaction(async function (err, data) {
                console.info('DeviceKvStorestartTransactionCallbackCommitTest startTransaction success');
                expect(err == undefined).assertTrue();
                let entries = putBatchString(10, 'batch_test_string_key');
                console.info('DeviceKvStorestartTransactionCallbackCommitTest entries: ' + JSON.stringify(entries));
                await kvStore.putBatch(entries, async function (err, data) {
                    console.info('DeviceKvStorestartTransactionCallbackCommitTest putBatch success');
                    expect(err == undefined).assertTrue();
                    let keys = Object.keys(entries).slice(5);
                    await kvStore.deleteBatch(keys, async function (err, data) {
                        console.info('DeviceKvStorestartTransactionCallbackCommitTest deleteBatch success');
                        expect(err == undefined).assertTrue();
                        await kvStore.commit(async function (err, data) {
                            console.info('DeviceKvStorestartTransactionCallbackCommitTest commit success');
                            expect(err == undefined).assertTrue();
                            await sleep(2000);
                            expect(count == 1).assertTrue();
                            done();
                        });
                    });
                });
            });
        } catch (e) {
            console.error('DeviceKvStorestartTransactionCallbackCommitTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStorestartTransactionCallbackRollbackTest
     * @tc.desc Test Js Api DeviceKvStore.startTransaction() rollback
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStorestartTransactionCallbackRollbackTest', 0, async function (done) {
        console.info('DeviceKvStorestartTransactionCallbackRollbackTest');
        try {
            var count = 0;
            kvStore.on('dataChange', 0, function (data) {
                console.info('DeviceKvStorestartTransactionCallbackRollbackTest 0' + data)
                count++;
            });
            await kvStore.startTransaction(async function (err, data) {
                console.info('DeviceKvStorestartTransactionCallbackRollbackTest startTransaction success');
                expect(err == undefined).assertTrue();
                let entries = putBatchString(10, 'batch_test_string_key');
                console.info('DeviceKvStorestartTransactionCallbackRollbackTest entries: ' + JSON.stringify(entries));
                await kvStore.putBatch(entries, async function (err, data) {
                    console.info('DeviceKvStorestartTransactionCallbackRollbackTest putBatch success');
                    expect(err == undefined).assertTrue();
                    let keys = Object.keys(entries).slice(5);
                    await kvStore.deleteBatch(keys, async function (err, data) {
                        console.info('DeviceKvStorestartTransactionCallbackRollbackTest deleteBatch success');
                        expect(err == undefined).assertTrue();
                        await kvStore.rollback(async function (err, data) {
                            console.info('DeviceKvStorestartTransactionCallbackRollbackTest rollback success');
                            expect(err == undefined).assertTrue();
                            await sleep(2000);
                            expect(count == 0).assertTrue();
                            done();
                        });
                    });
                });
            });
        } catch (e) {
            console.error('DeviceKvStorestartTransactionCallbackRollbackTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStorestartTransactionCallbackInvalidArgsTest
     * @tc.desc Test Js Api DeviceKvStore.startTransaction() with invalid args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStorestartTransactionCallbackInvalidArgsTest', 0, async function (done) {
        console.info('DeviceKvStorestartTransactionCallbackInvalidArgsTest');
        try {
            await kvStore.startTransaction(1, function (err, data) {
                if (err == undefined) {
                    console.info('DeviceKvStorestartTransactionCallbackInvalidArgsTest startTransaction success');
                    expect(null).assertFail();
                } else {
                    console.info('DeviceKvStorestartTransactionCallbackInvalidArgsTest startTransaction fail');
                    expect(true).assertTrue();
                }
                done();
            });
        } catch (e) {
            console.error('DeviceKvStorestartTransactionCallbackInvalidArgsTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStoreCommitCallbackInvalidArgsTest
     * @tc.desc Test Js Api DeviceKvStore.Commit() invalid args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreCommitCallbackInvalidArgsTest', 0, async function (done) {
        console.info('DeviceKvStoreCommitCallbackInvalidArgsTest');
        try {
            await kvStore.commit(1, function (err, data) {
                if (err == undefined) {
                    console.info('DeviceKvStoreCommitCallbackInvalidArgsTest commit success');
                    expect(null).assertFail();
                } else {
                    console.info('DeviceKvStoreCommitCallbackInvalidArgsTest commit fail');
                    expect(true).assertTrue();
                }
                done();
            });
        } catch (e) {
            console.error('DeviceKvStoreCommitCallbackInvalidArgsTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStoreRollbackCallbackInvalidArgsTest
     * @tc.desc Test Js Api DeviceKvStore.Rollback() with invalid args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreRollbackCallbackInvalidArgsTest', 0, async function (done) {
        console.info('DeviceKvStoreRollbackCallbackInvalidArgsTest');
        try {
            await kvStore.rollback(1, function (err, data) {
                if (err == undefined) {
                    console.info('DeviceKvStoreRollbackCallbackInvalidArgsTest commit success');
                    expect(null).assertFail();
                } else {
                    console.info('DeviceKvStoreRollbackCallbackInvalidArgsTest commit fail');
                    expect(true).assertTrue();
                }
                done();
            });
        } catch (e) {
            console.error('DeviceKvStoreRollbackCallbackInvalidArgsTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStoreEnableSyncCallbackTrueTest
     * @tc.desc Test Js Api DeviceKvStore.EnableSync() true successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreEnableSyncCallbackTrueTest', 0, async function (done) {
        console.info('DeviceKvStoreEnableSyncCallbackTrueTest');
        try {
            await kvStore.enableSync(true, function (err, data) {
                if (err == undefined) {
                    console.info('DeviceKvStoreEnableSyncCallbackTrueTest enableSync success');
                    expect(err == undefined).assertTrue();
                } else {
                    console.info('DeviceKvStoreEnableSyncCallbackTrueTest enableSync fail');
                    expect(null).assertFail();
                }
                done();
            });
        } catch (e) {
            console.error('DeviceKvStoreEnableSyncCallbackTrueTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStoreEnableSyncCallbackFalseTest
     * @tc.desc Test Js Api DeviceKvStore.EnableSync() false successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreEnableSyncCallbackFalseTest', 0, async function (done) {
        console.info('DeviceKvStoreEnableSyncCallbackFalseTest');
        try {
            await kvStore.enableSync(false, function (err, data) {
                if (err == undefined) {
                    console.info('DeviceKvStoreEnableSyncCallbackFalseTest enableSync success');
                    expect(err == undefined).assertTrue();
                } else {
                    console.info('DeviceKvStoreEnableSyncCallbackFalseTest enableSync fail');
                    expect(null).assertFail();
                }
                done();
            });
        } catch (e) {
            console.error('DeviceKvStoreEnableSyncCallbackFalseTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStoreEnableSyncCallbackNoArgsTest
     * @tc.desc Test Js Api DeviceKvStore.EnableSync() with no args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreEnableSyncCallbackNoArgsTest', 0, async function (done) {
        console.info('DeviceKvStoreEnableSyncCallbackNoArgsTest');
        try {
            await kvStore.enableSync(function (err, data) {
                if (err == undefined) {
                    console.info('DeviceKvStoreEnableSyncCallbackNoArgsTest enableSync success');
                    expect(null).assertFail();
                } else {
                    console.info('DeviceKvStoreEnableSyncCallbackNoArgsTest enableSync fail');
                    expect(null).assertFail();
                }
                done();
            });
        } catch (e) {
            console.error('DeviceKvStoreEnableSyncCallbackNoArgsTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(e.code == 401).assertTrue();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStoreEnableSyncCallbackWrongArgsTest
     * @tc.desc Test Js Api DeviceKvStore.EnableSync() with wrong args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreEnableSyncCallbackWrongArgsTest', 0, async function (done) {
        console.info('DeviceKvStoreEnableSyncCallbackWrongArgsTest');
        try {
            await kvStore.enableSync(null, function (err, data) {
                if (err == undefined) {
                    console.info('DeviceKvStoreEnableSyncCallbackWrongArgsTest enableSync success');
                    expect(null).assertFail();
                } else {
                    console.info('DeviceKvStoreEnableSyncCallbackWrongArgsTest enableSync fail');
                    expect(err.code == 401).assertTrue();
                }
                done();
            });
        } catch (e) {
            console.error('DeviceKvStoreEnableSyncCallbackWrongArgsTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            done();
        }
    })

    /**
     * @tc.name DeviceKvStoreRemoveDeviceDataCallbackWrongDeviceIdTest
     * @tc.desc Test Js Api DeviceKvStore.RemoveDeviceData() remove wrong deviceid
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreRemoveDeviceDataCallbackWrongDeviceIdTest', 0, async function (done) {
        console.info('DeviceKvStoreRemoveDeviceDataCallbackWrongDeviceIdTest');
        try {
            await kvStore.put(KEY_TEST_STRING_ELEMENT, VALUE_TEST_STRING_ELEMENT, async function (err, data) {
                console.info('DeviceKvStoreRemoveDeviceDataCallbackWrongDeviceIdTest put success');
                expect(err == undefined).assertTrue();
                var deviceid = 'no_exist_device_id';
                await kvStore.removeDeviceData(deviceid, async function (err, data) {
                    if (err == undefined) {
                        console.info('DeviceKvStoreRemoveDeviceDataCallbackWrongDeviceIdTest removeDeviceData success');
                        expect(null).assertFail();
                        done();
                    } else {
                        console.info('DeviceKvStoreRemoveDeviceDataCallbackWrongDeviceIdTest removeDeviceData fail');
                        await kvStore.get(localDeviceId, KEY_TEST_STRING_ELEMENT, async function (err, data) {
                            console.info('DeviceKvStoreRemoveDeviceDataCallbackWrongDeviceIdTest get success');
                            expect(data == VALUE_TEST_STRING_ELEMENT).assertTrue();
                            done();
                        });
                    }
                });
            });
        } catch (e) {
            console.error('DeviceKvStoreRemoveDeviceDataCallbackWrongDeviceIdTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStoreRemoveDeviceDataCallbackWrongArgsTest
     * @tc.desc Test Js Api DeviceKvStore.RemoveDeviceData() with wrong args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreRemoveDeviceDataCallbackWrongArgsTest', 0, async function (done) {
        console.info('DeviceKvStoreRemoveDeviceDataCallbackWrongArgsTest');
        try {
            await kvStore.removeDeviceData(function (err, data) {
                if (err == undefined) {
                    console.info('DeviceKvStoreRemoveDeviceDataCallbackWrongArgsTest removeDeviceData success');
                    expect(null).assertFail();
                } else {
                    console.info('DeviceKvStoreRemoveDeviceDataCallbackWrongArgsTest removeDeviceData fail');
                    expect(null).assertFail();
                }
                done();
            });
        } catch (e) {
            console.error('DeviceKvStoreRemoveDeviceDataCallbackWrongArgsTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(e.code == 401).assertTrue();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreRemoveDeviceDataCallbackInvalidArgsTest
     * @tc.desc Test Js Api DeviceKvStore.RemoveDeviceData() with invalid args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreRemoveDeviceDataCallbackInvalidArgsTest', 0, async function (done) {
        console.info('DeviceKvStoreRemoveDeviceDataCallbackInvalidArgsTest');
        try {
            await kvStore.removeDeviceData('', function (err, data) {
                if (err == undefined) {
                    console.info('DeviceKvStoreRemoveDeviceDataCallbackInvalidArgsTest removeDeviceData success');
                    expect(null).assertFail();
                } else {
                    console.info('DeviceKvStoreRemoveDeviceDataCallbackInvalidArgsTest removeDeviceData fail');
                    expect(null).assertFail();
                }
                done();
            });
        } catch (e) {
            console.error('DeviceKvStoreRemoveDeviceDataCallbackInvalidArgsTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(e.code == 401).assertTrue();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreGetResultSetCallbackTestSuc001
     * @tc.desc Test Js Api DeviceKvStore.GetResultSet() successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreGetResultSetCallbackTestSuc001', 0, async function (done) {
        console.info('DeviceKvStoreGetResultSetCallbackTestSuc001');
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
            await kvStore.putBatch(entries, async function (err, data) {
                console.info('DeviceKvStoreGetResultSetCallbackTestSuc001 putBatch success');
                expect(err == undefined).assertTrue();
                await kvStore.getResultSet(localDeviceId, 'batch_test_string_key', async function (err, result) {
                    console.info('DeviceKvStoreGetResultSetCallbackTestSuc001 getResultSet success');
                    resultSet = result;
                    expect(resultSet.getCount() == 10).assertTrue();
                    await kvStore.closeResultSet(resultSet, function (err, data) {
                        console.info('DeviceKvStoreGetResultSetCallbackTestSuc001 closeResultSet success');
                        expect(err == undefined).assertTrue();
                        done();
                    })
                });
            });
        } catch (e) {
            console.error('DeviceKvStoreGetResultSetCallbackTestSuc001 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStoreGetResultSetCallbackEmptyResultsetTest
     * @tc.desc Test Js Api DeviceKvStore.GetResultSet() get empty resultset
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreGetResultSetCallbackEmptyResultsetTest', 0, async function (done) {
        console.info('DeviceKvStoreGetResultSetCallbackEmptyResultsetTest');
        try {
            let resultSet;
            await kvStore.getResultSet(localDeviceId, 'batch_test_string_key', async function (err, result) {
                console.info('DeviceKvStoreGetResultSetCallbackEmptyResultsetTest getResultSet success');
                resultSet = result;
                expect(resultSet.getCount() == 0).assertTrue();
                await kvStore.closeResultSet(resultSet, function (err, data) {
                    console.info('DeviceKvStoreGetResultSetCallbackEmptyResultsetTest closeResultSet success');
                    expect(err == undefined).assertTrue();
                    done();
                })
            });
        } catch (e) {
            console.error('DeviceKvStoreGetResultSetCallbackEmptyResultsetTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStoreGetResultSetCallbackNoArgsTest
     * @tc.desc Test Js Api DeviceKvStore.GetResultSet() with no args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreGetResultSetCallbackNoArgsTest', 0, async function (done) {
        console.info('DeviceKvStoreGetResultSetCallbackNoArgsTest');
        try {
            let resultSet;
            await kvStore.getResultSet(function (err, result) {
                console.info('DeviceKvStoreGetResultSetCallbackNoArgsTest getResultSet success');
                expect(err != undefined).assertTrue();
                done();
            });
        } catch (e) {
            console.error('DeviceKvStoreGetResultSetCallbackNoArgsTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(e.code == 401).assertTrue();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStoreGetResultSetCallbackNoDeviceIdTest
     * @tc.desc Test Js Api DeviceKvStore.GetResultSet() with no deviceid
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreGetResultSetCallbackNoDeviceIdTest', 0, async function (done) {
        console.info('DeviceKvStoreGetResultSetCallbackNoDeviceIdTest');
        try {
            let resultSet;
            await kvStore.getResultSet('test_key_string', 123, function (err, result) {
                console.info('DeviceKvStoreGetResultSetCallbackNoDeviceIdTest getResultSet success');
                expect(err != undefined).assertTrue();
                done();
            });
        } catch (e) {
            console.error('DeviceKvStoreGetResultSetCallbackNoDeviceIdTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(e.code == 401).assertTrue();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStoreGetResultSetCallbackPrefixkeyTest
     * @tc.desc Test Js Api DeviceKvStore.GetResultSet() with prefixkey
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreGetResultSetCallbackPrefixkeyTest', 0, async function (done) {
        console.info('DeviceKvStoreGetResultSetCallbackPrefixkeyTest');
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
            await kvStore.putBatch(entries, async function (err, data) {
                console.info('DeviceKvStoreGetResultSetCallbackPrefixkeyTest putBatch success');
                expect(err == undefined).assertTrue();
                var query = new factory.Query();
                query.prefixKey("batch_test");
                await kvStore.getResultSet(localDeviceId, query, async function (err, result) {
                    console.info('DeviceKvStoreGetResultSetCallbackPrefixkeyTest getResultSet success');
                    resultSet = result;
                    expect(resultSet.getCount() == 10).assertTrue();
                    await kvStore.closeResultSet(resultSet, function (err, data) {
                        console.info('DeviceKvStoreGetResultSetCallbackPrefixkeyTest closeResultSet success');
                        expect(err == undefined).assertTrue();
                        done();
                    })
                });
            });
        } catch (e) {
            console.error('DeviceKvStoreGetResultSetCallbackPrefixkeyTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStoreGetResultSetCallbackQueryDeviceIdTest
     * @tc.desc Test Js Api DeviceKvStore.GetResultSet() device id in query
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreGetResultSetCallbackQueryDeviceIdTest', 0, async function (done) {
        console.info('DeviceKvStoreGetResultSetCallbackQueryDeviceIdTest');
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
            await kvStore.putBatch(entries, async function (err, data) {
                console.info('DeviceKvStoreGetResultSetCallbackQueryDeviceIdTest putBatch success');
                expect(err == undefined).assertTrue();
                var query = new factory.Query();
                query.prefixKey("batch_test");
                query.deviceId(localDeviceId);
                await kvStore.getResultSet(query, async function (err, result) {
                    console.info('DeviceKvStoreGetResultSetCallbackQueryDeviceIdTest getResultSet success');
                    resultSet = result;
                    expect(resultSet.getCount() == 10).assertTrue();
                    await kvStore.closeResultSet(resultSet, function (err, data) {
                        console.info('DeviceKvStoreGetResultSetCallbackQueryDeviceIdTest closeResultSet success');
                        expect(err == undefined).assertTrue();
                        done();
                    })
                });
            });
        } catch (e) {
            console.error('DeviceKvStoreGetResultSetCallbackQueryDeviceIdTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStoreGetResultSetCallbackPredicatesTest
     * @tc.desc Test Js Api DeviceKvStore.GetResultSet() with predicates
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreGetResultSetCallbackPredicatesTest', 0, async function (done) {
        console.info('DeviceKvStoreGetResultSetCallbackPredicatesTest');
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
            await kvStore.putBatch(entries, async function (err, data) {
                console.info('DeviceKvStoreGetResultSetCallbackPredicatesTest putBatch success');
                expect(err == undefined).assertTrue();
                let predicates = new dataSharePredicates.DataSharePredicates();
                predicates.inKeys("batch_test");
                await kvStore.getResultSet(localDeviceId, predicates, async function (err, result) {
                    console.info('DeviceKvStoreGetResultSetCallbackPredicatesTest getResultSet success');
                    resultSet = result;
                    expect(resultSet.getCount() == 10).assertTrue();
                    await kvStore.closeResultSet(resultSet, function (err, data) {
                        console.info('DeviceKvStoreGetResultSetCallbackPredicatesTest closeResultSet success');
                        expect(err == undefined).assertTrue();
                        done();
                    })
                });
            });
        } catch (e) {
            console.error('DeviceKvStoreGetResultSetCallbackPredicatesTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStoreCloseResultSetCallbackNullResultsetTest
     * @tc.desc Test Js Api DeviceKvStore.CloseResultSet() close null resultset
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreCloseResultSetCallbackNullResultsetTest', 0, async function (done) {
        try {
            let resultSet = null;
            await kvStore.closeResultSet(resultSet, function (err, data) {
                if (err == undefined) {
                    console.info('DeviceKvStoreCloseResultSetCallbackNullResultsetTest closeResultSet success');
                    expect(null).assertFail();
                } else {
                    console.info('DeviceKvStoreCloseResultSetCallbackNullResultsetTest closeResultSet fail');
                }
                done();
            });
        } catch (e) {
            console.error('DeviceKvStoreCloseResultSetCallbackNullResultsetTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(e.code == 401).assertTrue();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStoreCloseResultSetCallbackSucTest
     * @tc.desc Test Js Api DeviceKvStore.CloseResultSet() successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreCloseResultSetCallbackSucTest', 0, async function (done) {
        console.info('DeviceKvStoreCloseResultSetCallbackSucTest');
        try {
            let resultSet = null;
            await kvStore.getResultSet(localDeviceId, 'batch_test_string_key', async function (err, result) {
                console.info('DeviceKvStoreCloseResultSetCallbackSucTest getResultSet success');
                resultSet = result;
                await kvStore.closeResultSet(resultSet, function (err, data) {
                    if (err == undefined) {
                        console.info('DeviceKvStoreCloseResultSetCallbackSucTest closeResultSet success');
                        expect(err == undefined).assertTrue();
                    } else {
                        console.info('DeviceKvStoreCloseResultSetCallbackSucTest closeResultSet fail');
                        expect(null).assertFail();
                    }
                    done();
                });
            });
        } catch (e) {
            console.error('DeviceKvStoreCloseResultSetCallbackSucTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStoreCloseResultSetCallbackNoArgsTest
     * @tc.desc Test Js Api DeviceKvStore.CloseResultSet() with no args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreCloseResultSetCallbackNoArgsTest', 0, async function (done) {
        console.info('DeviceKvStoreCloseResultSetCallbackNoArgsTest');
        try {
            await kvStore.closeResultSet(function (err) {
                if (err == undefined) {
                    console.info('DeviceKvStoreCloseResultSetCallbackNoArgsTest closeResultSet success');
                    expect(null).assertFail();
                } else {
                    console.info('DeviceKvStoreCloseResultSetCallbackNoArgsTest closeResultSet fail');
                    expect(null).assertFail();
                }
                done();
            });
        } catch (e) {
            console.error('DeviceKvStoreCloseResultSetCallbackNoArgsTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(e.code == 401).assertTrue();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStoreCloseResultSetCallbackWrongArgsTest
     * @tc.desc Test Js Api DeviceKvStore.CloseResultSet() with wrong args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreCloseResultSetCallbackWrongArgsTest', 0, async function (done) {
        console.info('DeviceKvStoreCloseResultSetCallbackWrongArgsTest');
        let errorInfo = undefined;
        try {
            await kvStore.closeResultSet(1, function (err) {
                console.log("DeviceKvStoreCloseResultSetCallbackWrongArgsTest tailed");
                expect(null).assertFail();
            })
        } catch (e) {
            console.error('DeviceKvStoreCloseResultSetCallbackWrongArgsTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            errorInfo = e;
            expect(e.code == 401).assertTrue();
        }
        expect(errorInfo != undefined).assertEqual(true);
        done();
    })

    /**
     * @tc.name DeviceKvStoreGetResultSizeCallbackSucTest
     * @tc.desc Test Js Api DeviceKvStore.Get(ResultSize) successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreGetResultSizeCallbackSucTest', 0, async function (done) {
        console.info('DeviceKvStoreGetResultSizeCallbackSucTest');
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
            await kvStore.putBatch(entries, async function (err, data) {
                console.info('DeviceKvStoreGetResultSizeCallbackSucTest putBatch success');
                expect(err == undefined).assertTrue();
                var query = new factory.Query();
                query.prefixKey("batch_test");
                query.deviceId(localDeviceId);
                await kvStore.getResultSize(query, async function (err, resultSize) {
                    console.info('DeviceKvStoreGetResultSizeCallbackSucTest getResultSet success');
                    expect(resultSize == 10).assertTrue();
                    done();
                });
            });
        } catch (e) {
            console.error('DeviceKvStoreGetResultSizeCallbackSucTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStoreGetResultSizeCallbackQueryTest
     * @tc.desc Test Js Api DeviceKvStore.Get(ResultSize) with query
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreGetResultSizeCallbackQueryTest', 0, async function (done) {
        console.info('DeviceKvStoreGetResultSizeCallbackQueryTest');
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
            await kvStore.putBatch(entries, async function (err, data) {
                console.info('DeviceKvStoreGetResultSizeCallbackQueryTest putBatch success');
                expect(err == undefined).assertTrue();
                var query = new factory.Query();
                query.prefixKey("batch_test");
                await kvStore.getResultSize(localDeviceId, query, async function (err, resultSize) {
                    console.info('DeviceKvStoreGetResultSizeCallbackQueryTest getResultSet success');
                    expect(resultSize == 10).assertTrue();
                    done();
                });
            });
        } catch (e) {
            console.error('DeviceKvStoreGetResultSizeCallbackQueryTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStoreGetEntriesCallbackSucTest
     * @tc.desc Test Js Api DeviceKvStore.GetEntries() successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreGetEntriesCallbackSucTest', 0, async function (done) {
        console.info('DeviceKvStoreGetEntriesCallbackSucTest');
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
            console.info('DeviceKvStoreGetEntriesCallbackSucTest entries: ' + JSON.stringify(entries));
            await kvStore.putBatch(entries, async function (err, data) {
                console.info('DeviceKvStoreGetEntriesCallbackSucTest putBatch success');
                expect(err == undefined).assertTrue();
                var query = new factory.Query();
                query.deviceId(localDeviceId);
                query.prefixKey("batch_test");
                kvStore.getEntries(localDeviceId, query, function (err, entrys) {
                    expect(entrys.length == 10).assertTrue();
                    expect(entrys[0].value.value.toString() == arr.toString()).assertTrue();
                    done();
                });
            });
            console.info('DeviceKvStoreGetEntriesCallbackSucTest success');
        } catch (e) {
            console.error('DeviceKvStoreGetEntriesCallbackSucTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })
})
