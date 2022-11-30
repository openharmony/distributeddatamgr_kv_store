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
import dataShare from '@ohos.data.dataSharePredicates';
import abilityFeatureAbility from '@ohos.ability.featureAbility'

var context = abilityFeatureAbility.getContext();
const KEY_TEST_INT_ELEMENT = 'key_test_int_2';
const KEY_TEST_FLOAT_ELEMENT = 'key_test_float_2';
const KEY_TEST_BOOLEAN_ELEMENT = 'key_test_boolean_2';
const KEY_TEST_STRING_ELEMENT = 'key_test_string_2';
const file = "";
const files = [file];

const VALUE_TEST_INT_ELEMENT = 1234;
const VALUE_TEST_FLOAT_ELEMENT = 4321.12;
const VALUE_TEST_BOOLEAN_ELEMENT = true;
const VALUE_TEST_STRING_ELEMENT = 'value-string-002';

const TEST_BUNDLE_NAME = 'com.example.myapplication';
const TEST_STORE_ID = 'storeId3';
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

describe('DeviceKvStoreCallbackTest', function () {
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
                } else {
                    console.error('DeviceKvStorePutStringCallbackSucTest put fail' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(null).assertFail();
                }
            });
        } catch (e) {
            console.error('DeviceKvStorePutStringCallbackSucTest put e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStorePutStringCallbackInvalidArgsTest
     * @tc.desc Test Js Api DeviceKvStore.Put(String) with invalid args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStorePutStringCallbackInvalidArgsTest', 0, async function (done) {
        console.info('DeviceKvStorePutStringCallbackInvalidArgsTest');
        try {
            await kvStore.put(KEY_TEST_STRING_ELEMENT, function (err, data) {
                if (err == undefined) {
                    expect(null).assertFail();
                    console.info('DeviceKvStorePutStringCallbackInvalidArgsTest put success');
                } else {
                    console.error('DeviceKvStorePutStringCallbackInvalidArgsTest put fail' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(null).assertFail();
                }
            });
        } catch (e) {
            console.error('DeviceKvStorePutStringCallbackInvalidArgsTest put e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(e.code == 401).assertTrue();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStorePutStringCallbackClosedKvStoreTest
     * @tc.desc Test Js Api DeviceKvStore.Put(String) with closed database
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStorePutStringCallbackClosedKvStoreTest', 0, async function (done) {
        console.info('DeviceKvStorePutStringCallbackClosedKvStoreTest');
        try {
            await kvManager.closeKVStore(TEST_BUNDLE_NAME, TEST_STORE_ID, function (err) {
                expect(err == undefined).assertTrue();
            });
            await kvStore.put(KEY_TEST_STRING_ELEMENT, VALUE_TEST_STRING_ELEMENT, function (err) {
                if (err == undefined) {
                    expect(null).assertFail();
                    console.info('DeviceKvStorePutStringCallbackClosedKvStoreTest put success');
                } else {
                    console.error('DeviceKvStorePutStringCallbackClosedKvStoreTest put fail' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(err.code == 15100005).assertTrue();
                }
            });
        } catch (e) {
            console.error('DeviceKvStorePutStringCallbackClosedKvStoreTest put e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreGetStringCallbackNoPutTest
     * @tc.desc Test Js Api DeviceKvStore.GetString() with no put
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreGetStringCallbackNoPutTest', 0, async function (done) {
        console.info('DeviceKvStoreGetStringCallbackNoPutTest');
        try {
            await kvStore.get(localDeviceId, KEY_TEST_STRING_ELEMENT, function (err) {
                if (err == undefined) {
                    console.info('DeviceKvStoreGetStringCallbackNoPutTest get success');
                    expect(null).assertFail();
                } else {
                    console.info('DeviceKvStoreGetStringCallbackNoPutTest get fail');
                    expect(err.code == 15100004).assertTrue();
                }
            });
        } catch (e) {
            console.error('DeviceKvStoreGetStringCallbackTest get e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreGetStringCallbackSucTest
     * @tc.desc Test Js Api DeviceKvStore.GetString() successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreGetStringCallbackSucTest', 0, async function (done) {
        console.info('DeviceKvStoreGetStringCallbackSucTest');
        try {
            await kvStore.put(KEY_TEST_STRING_ELEMENT, VALUE_TEST_STRING_ELEMENT, async function (err) {
                console.info('DeviceKvStoreGetStringCallbackSucTest put success');
                expect(err == undefined).assertTrue();
                await kvStore.get(localDeviceId, KEY_TEST_STRING_ELEMENT, function (err, data) {
                    console.info('DeviceKvStoreGetStringCallbackSucTest get success');
                    expect((err == undefined) && (VALUE_TEST_STRING_ELEMENT == data)).assertTrue();
                });
            })
        } catch (e) {
            console.error('DeviceKvStoreGetStringCallbackSucTest get e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreGetStringCallbackClosedKVStoreTest
     * @tc.desc Test Js Api DeviceKvStore.GetString() from a closed kvstore
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreGetStringCallbackClosedKVStoreTest', 0, async function (done) {
        try {
            await kvStore.put(KEY_TEST_STRING_ELEMENT, VALUE_TEST_STRING_ELEMENT, async function (err) {
                expect(err == undefined).assertTrue();
            })
            await kvManager.closeKVStore(TEST_BUNDLE_NAME, TEST_STORE_ID, function (err) {
                expect(err == undefined).assertTrue();
            });
            await kvStore.get(localDeviceId, KEY_TEST_STRING_ELEMENT, function (err) {
                if (err == undefined) {
                    expect(null).assertFail();
                } else {
                    expect(err.code == 15100005).assertTrue();
                }
            });
        } catch (e) {
            console.error('DeviceKvStoreGetStringCallbackClosedKVStoreTest get e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreGetStringCallbackInvalidArgsTest
     * @tc.desc Test Js Api DeviceKvStore.GetString() with invalid args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreGetStringCallbackInvalidArgsTest', 0, async function (done) {
        try {
            await kvStore.put(KEY_TEST_STRING_ELEMENT, VALUE_TEST_STRING_ELEMENT, async function (err) {
                console.info('DeviceKvStoreGetStringCallbackInvalidArgsTest put success');
                expect(err == undefined).assertTrue();
                await kvStore.get(localDeviceId, function (err, data) {
                    if (err == undefined) {
                        expect(null).assertFail();
                    } else {
                        expect(null).assertFail();
                    }
                });
            })
        } catch (e) {
            console.error('DeviceKvStoreGetStringCallbackInvalidArgsTest get e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(e.code == 401).assertTrue();
        }
        done();
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
            await kvStore.put(KEY_TEST_INT_ELEMENT, VALUE_TEST_INT_ELEMENT, async function (err) {
                console.info('DeviceKvStorePutIntCallbackSucTest put success');
                expect(err == undefined).assertTrue();
                await kvStore.get(localDeviceId, KEY_TEST_INT_ELEMENT, function (err, data) {
                    console.info('DeviceKvStorePutIntCallbackSucTest get success');
                    expect((err == undefined) && (VALUE_TEST_INT_ELEMENT == data)).assertTrue();
                })
            });
        } catch (e) {
            console.error('DeviceKvStorePutIntCallbackSucTest put e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
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
            var intValue = Number.MIN_VALUE;
            await kvStore.put(KEY_TEST_INT_ELEMENT, intValue, async function (err) {
                console.info('DeviceKvStorePutIntCallbackMaxTest put success');
                expect(err == undefined).assertTrue();
                await kvStore.get(localDeviceId, KEY_TEST_INT_ELEMENT, function (err, data) {
                    console.info('DeviceKvStorePutIntCallbackMaxTest get success');
                    expect((err == undefined) && (intValue == data)).assertTrue();
                })
            });
        } catch (e) {
            console.error('DeviceKvStorePutIntCallbackMaxTest put e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
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
            var intValue = Number.MAX_VALUE;
            await kvStore.put(KEY_TEST_INT_ELEMENT, intValue, async function (err) {
                console.info('DeviceKvStorePutIntCallbackMinTest put success');
                expect(err == undefined).assertTrue();
                await kvStore.get(localDeviceId, KEY_TEST_INT_ELEMENT, function (err, data) {
                    console.info('DeviceKvStorePutIntCallbackMinTest get success');
                    expect((err == undefined) && (intValue == data)).assertTrue();
                })
            });
        } catch (e) {
            console.error('DeviceKvStorePutIntCallbackMinTest put e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreGetIntCallbackNonExistTest
     * @tc.desc Test Js Api DeviceKvStore.GetInt() get non-exsiting int
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreGetIntCallbackTest', 0, async function (done) {
        console.info('DeviceKvStoreGetIntCallbackTest');
        try {
            await kvStore.get(localDeviceId, KEY_TEST_INT_ELEMENT, function (err) {
                if (err == undefined) {
                    console.info('DeviceKvStoreGetIntCallbackTest get success');
                    expect(null).assertFail();
                } else {
                    console.info('DeviceKvStoreGetIntCallbackTest get fail');
                    expect(err.code == 15100004).assertTrue();
                }
            })
        } catch (e) {
            console.error('DeviceKvStoreGetIntCallbackTest put e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStorePutBoolCallbackTest
     * @tc.desc Test Js Api DeviceKvStore.Put(Bool) successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStorePutBoolCallbackTest', 0, async function (done) {
        console.info('DeviceKvStorePutBoolCallbackTest');
        try {
            await kvStore.put(KEY_TEST_BOOLEAN_ELEMENT, VALUE_TEST_BOOLEAN_ELEMENT, function (err) {
                console.info('DeviceKvStorePutBoolCallbackTest put success');
                expect(err == undefined).assertTrue();
            });
        } catch (e) {
            console.error('DeviceKvStorePutBoolCallbackTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreGetBoolCallbackSucTest
     * @tc.desc Test Js Api DeviceKvStore.GetBool() successfully
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
                    console.info(data);
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
     * @tc.name DeviceKvStoreGetFloatCallbackSucTest
     * @tc.desc Test Js Api DeviceKvStore.GetFloat() successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreGetFloatCallbackSucTest', 0, async function (done) {
        console.info('DeviceKvStoreGetFloatCallbackSucTest');
        try {
            await kvStore.put(KEY_TEST_FLOAT_ELEMENT, VALUE_TEST_FLOAT_ELEMENT, async function (err, data) {
                console.info('DeviceKvStoreGetFloatCallbackSucTest put success');
                expect(err == undefined).assertTrue();
                await kvStore.get(localDeviceId, KEY_TEST_FLOAT_ELEMENT, function (err, data) {
                    if (err == undefined) {
                        console.info('DeviceKvStoreGetFloatCallbackSucTest get success');
                        expect(true).assertTrue();
                    } else {
                        console.error('DeviceKvStoreGetFloatCallbackSucTest get fail' + `, error code is ${err.code}, message is ${err.message}`);
                        expect(null).assertFail();
                    }
                });
                done();
            });
        } catch (e) {
            console.error('DeviceKvStoreGetFloatCallbackSucTest e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStoreDeleteStringCallbackSucTest
     * @tc.desc Test Js Api DeviceKvStore.DeleteString() successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreDeleteStringCallbackSucTest', 0, async function (done) {
        console.info('DeviceKvStoreDeleteStringCallbackSucTest');
        try {
            await kvStore.put(KEY_TEST_STRING_ELEMENT, VALUE_TEST_STRING_ELEMENT, async function (err, data) {
                console.info('DeviceKvStoreDeleteStringCallbackSucTest put success');
                expect(err == undefined).assertTrue();
                await kvStore.delete(KEY_TEST_STRING_ELEMENT, function (err, data) {
                    console.info('DeviceKvStoreDeleteStringCallbackSucTest delete success');
                    expect(err == undefined).assertTrue();
                    done();
                });
            })
        } catch (e) {
            console.error('DeviceKvStoreDeleteStringCallbackSucTest e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStoreDeleteStringCallbackInvalid ArgsTest
     * @tc.desc Test Js Api DeviceKvStore.DeleteString() with invalid args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreDeleteStringCallbackInvalidArgsTest', 0, async function (done) {
        console.info('DeviceKvStoreDeleteStringCallbackInvalidArgsTest');
        try {
            await kvStore.put(KEY_TEST_STRING_ELEMENT, VALUE_TEST_STRING_ELEMENT, async function (err, data) {
                console.info('DeviceKvStoreDeleteStringCallbackInvalidArgsTest put success');
                expect(err == undefined).assertTrue();
                await kvStore.delete(function (err) {
                    console.info('DeviceKvStoreDeleteStringCallbackInvalidArgsTest delete success');
                    expect(null).assertFail();
                });
            })
        } catch (e) {
            console.error('DeviceKvStoreDeleteStringCallbackInvalidArgsTest e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(e.code == 401).assertTrue();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreDeleteCallbackClosedKVStoreTest
     * @tc.desc Test Js Api DeviceKvStore.Delete() into a closed kvstore
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreDeleteCallbackClosedKVStoreTest', 0, async function (done) {
        try {
            await kvStore.put(KEY_TEST_STRING_ELEMENT, VALUE_TEST_STRING_ELEMENT, function (err) {
                if (err == undefined) {
                    expect(true).assertTrue();
                    console.info('DeviceKvStorePutStringCallbackClosedKvStoreTest put success');
                } else {
                    console.error('DeviceKvStorePutStringCallbackClosedKvStoreTest put fail' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(null).assertFail();
                }
            });
            await kvManager.closeKVStore(TEST_BUNDLE_NAME, TEST_STORE_ID, function (err) {
                if (err == undefined) {
                    expect(err == undefined).assertTrue();
                } else {
                    expect(null).assertFail();
                }
            });
            await kvStore.delete(KEY_TEST_STRING_ELEMENT, function (err) {
                if (err == undefined) {
                    expect(null).assertFail();
                } else {
                    expect(err.code == 15100005).assertTrue();
                }
            });
        } catch (e) {
            console.error('DeviceKvStorePutStringCallbackClosedKvStoreTest put e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreDeleteIntCallbackSucTest
     * @tc.desc Test Js Api DeviceKvStore.DeleteInt() successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreDeleteIntCallbackSucTest', 0, async function (done) {
        console.info('DeviceKvStoreDeleteIntCallbackSucTest');
        try {
            await kvStore.put(KEY_TEST_INT_ELEMENT, VALUE_TEST_INT_ELEMENT, async function (err, data) {
                console.info('DeviceKvStoreDeleteIntCallbackSucTest put success');
                expect(err == undefined).assertTrue();
                await kvStore.delete(KEY_TEST_INT_ELEMENT, function (err, data) {
                    console.info('DeviceKvStoreDeleteIntCallbackSucTest delete success');
                    expect(err == undefined).assertTrue();
                    done();
                });
            })
        } catch (e) {
            console.error('DeviceKvStoreDeleteIntCallbackSucTest e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStoreDeleteFloatCallbackSucTest
     * @tc.desc Test Js Api DeviceKvStore.DeleteFloat() successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreDeleteFloatCallbackSucTest', 0, async function (done) {
        console.info('DeviceKvStoreDeleteFloatCallbackSucTest');
        try {
            await kvStore.put(KEY_TEST_FLOAT_ELEMENT, VALUE_TEST_FLOAT_ELEMENT, async function (err, data) {
                console.info('DeviceKvStoreDeleteFloatCallbackSucTest put success');
                expect(err == undefined).assertTrue();
                await kvStore.delete(KEY_TEST_FLOAT_ELEMENT, function (err, data) {
                    console.info('DeviceKvStoreDeleteFloatCallbackSucTest delete success');
                    expect(err == undefined).assertTrue();
                    done();
                });
            })
        } catch (e) {
            console.error('DeviceKvStoreDeleteFloatCallbackSucTest e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStoreDeleteBoolCallbackSucTest
     * @tc.desc Test Js Api DeviceKvStore.DeleteBool() successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreDeleteBoolCallbackSucTest', 0, async function (done) {
        console.info('DeviceKvStoreDeleteBoolCallbackSucTest');
        try {
            await kvStore.put(KEY_TEST_BOOLEAN_ELEMENT, VALUE_TEST_BOOLEAN_ELEMENT, async function (err, data) {
                console.info('DeviceKvStoreDeleteBoolCallbackSucTest put success');
                expect(err == undefined).assertTrue();
                await kvStore.delete(KEY_TEST_BOOLEAN_ELEMENT, function (err, data) {
                    console.info('DeviceKvStoreDeleteBoolCallbackSucTest delete success');
                    expect(err == undefined).assertTrue();
                    done();
                });
            })
        } catch (e) {
            console.error('DeviceKvStoreDeleteBoolCallbackSucTest e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStoreDeletePredicatesCallbackSucTest
     * @tc.desc Test Js Api DeviceKvStore.Delete() successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreDeletePredicatesCallbackSucTest', 0, async function (done) {
        console.log('DeviceKvStoreDeletePredicatesCallbackSucTest');
        try {
            let predicates = new dataShare.DataSharePredicates();
            let arr = ["name"];
            predicates.inKeys(arr);
            await kvStore.put("name", "Bob", async function (err, data) {
                console.log('DeviceKvStoreDeletePredicatesCallbackSucTest put success');
                expect(err == undefined).assertTrue();
                await kvStore.delete(predicates, function (err, data) {
                    console.log('DeviceKvStoreDeletePredicatesCallbackSucTest delete success');
                    expect(err == undefined).assertTrue();
                    done();
                });
            })
        } catch (e) {
            console.error('DeviceKvStoreDeletePredicatesCallbackSucTest e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStoreOnChangeCallbackType0Test
     * @tc.desc Test Js Api DeviceKvStore.OnChange() with type 0
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreOnChangeCallbackTest', 0, async function (done) {
        console.info('DeviceKvStoreOnChangeCallbackTest');
        try {
            kvStore.on('dataChange', 0, function (data) {
                console.info('DeviceKvStoreOnChangeCallbackTest dataChange');
                expect(data != null).assertTrue();
            });
            await kvStore.put(KEY_TEST_FLOAT_ELEMENT, VALUE_TEST_FLOAT_ELEMENT, function (err, data) {
                console.info('DeviceKvStoreOnChangeCallbackTest put success');
                expect(err == undefined).assertTrue();
                done();
            });
        } catch (e) {
            console.error('DeviceKvStoreOnChangeCallbackTest e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStoreOnChangeCallbackType1Test
     * @tc.desc Test Js Api DeviceKvStore.OnChange() with type 1
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
     * @tc.desc Test Js Api DeviceKvStore.OnChange() with type 2
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
     * @tc.name DeviceKvStoreOnChangeCallbackClosedKVStoreTest
     * @tc.desc Test Js Api DeviceKvStore.OnChange() subscribe a closed kvstore
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreOnChangeCallbackClosedKVStoreTest', 0, async function (done) {
        console.info('DeviceKvStoreOnChangeCallbackClosedKVStoreTest');
        try {
            await kvManager.closeKVStore(TEST_BUNDLE_NAME, TEST_STORE_ID, function (err) {
                expect(err == undefined).assertTrue();
                kvStore.on('dataChange', 2, function () {
                    expect(null).assertFail();
                });
            });
        } catch (e) {
            console.error('DeviceKvStoreOnChangeCallbackClosedKVStoreTest e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(e.code == 15100005).assertTrue();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreOnChangeCallbackPassMaxTest
     * @tc.desc Test Js Api DeviceKvStore.OnChange() pass max subscription time
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreOnChangeCallbackPassMaxTest', 0, async function (done) {
        console.info('DeviceKvStoreOnChangeCallbackPassMaxTest');
        try {
            for (let i = 0; i < 8; i++) {
                kvStore.on('dataChange', 0, function (data) {
                    console.info('DeviceKvStoreOnChangeCallbackPassMaxTest dataChange');
                    expect(data != null).assertTrue();
                });
            }
            kvStore.on('dataChange', 0, function (err) {
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('DeviceKvStoreOnChangeCallbackPassMaxTest e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(e.code == 15100001).assertTrue();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreOnChangeCallbackInvalidArgsTest
     * @tc.desc Test Js Api DeviceKvStore.OnChange() with invalid args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreOnChangeCallbackInvalidArgsTest', 0, async function (done) {
        console.info('DeviceKvStoreOnChangeCallbackInvalidArgsTest');
        try {
            kvStore.on('dataChange', function () {
                console.info('DeviceKvStoreOnChangeCallbackInvalidArgsTest dataChange');
                expect(null).assertFail();
            });

        } catch (e) {
            console.error('DeviceKvStoreOnChangeCallbackInvalidArgsTest e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(e.code == 401).assertTrue();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStoreOffChangeCallbackSucTest
     * @tc.desc Test Js Api DeviceKvStoreOffChange success
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreOffChangeCallbackSucTest', 0, async function (done) {
        console.info('DeviceKvStoreOffChangePromiseSucTest');
        try {
            var func = function (data) {
                console.info('DeviceKvStoreOffChangeCallbackSucTest ' + JSON.stringify(data));
            };
            kvStore.on('dataChange', 0, func);
            kvStore.off('dataChange', func);
        } catch (e) {
            console.error('DeviceKvStoreOffChangeCallbackSucTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreOffChangeCallbackInvalidArgsTest
     * @tc.desc Test Js Api DeviceKvStoreOffChange with invalid args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreOffChangeCallbackInvalidArgsTest', 0, async function (done) {
        console.info('DeviceKvStoreOffChangeCallbackInvalidArgsTest');
        try {
            kvStore.on('dataChange', 0, function (data) {
                console.info('DeviceKvStoreOffChangeCallbackInvalidArgsTest ' + JSON.stringify(data));
            });
            kvStore.off('dataChange', 1, function (err) {
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('DeviceKvStoreOffChangeCallbackSucTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(e.code == 401).assertTrue();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreOffSyncCompleteCallbackSucTest
     * @tc.desc Test Js Api DeviceKvStoreOffSyncComplete success
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreOffSyncCompleteCallbackSucTest', 0, async function (done) {
        console.info('DeviceKvStoreOffSyncCompleteCallbackSucTest');
        try {
            var func = function (data) {
                console.info('DeviceKvStoreOffSyncCompleteCallbackSucTest 0' + data)
            };
            kvStore.off('syncComplete', func);
            expect(true).assertTrue();
        } catch (e) {
            console.error('DeviceKvStoreOffSyncCompleteCallbackSucTest put e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreOffSyncCompleteCallbackInvalidArgsTest
     * @tc.desc Test Js Api DeviceKvStoreOffSyncComplete with invalid args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreOffSyncCompleteCallbackInvalidArgsTest', 0, async function (done) {
        console.info('DeviceKvStoreOffSyncCompleteCallbackInvalidArgsTest');
        try {
            kvStore.off(function (err) {
                if (err = undefined) {
                    expect(null).assertFail();
                } else {
                    expect(null).assertFail();
                }
            });
        } catch (e) {
            console.error('DeviceKvStoreOffSyncCompleteCallbackInvalidArgsTest put e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(e.code == 401).assertTrue();
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
     * @tc.desc Test Js Api DeviceKvStore.SetSyncRange() with joint range
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
     * @tc.desc Test Js Api DeviceKvStore.SetSyncRange() with same range
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it(' DeviceKvStoreSetSyncRangeCallbackSameTest', 0, async function (done) {
        console.info(' DeviceKvStoreSetSyncRangeCallbackSameTest');
        try {
            var localLabels = ['A', 'B'];
            var remoteSupportLabels = ['A', 'B'];
            await kvStore.setSyncRange(localLabels, remoteSupportLabels, function (err, data) {
                console.info(' DeviceKvStoreSetSyncRangeCallbackSameTest put success');
                expect(err == undefined).assertTrue();
                done();
            });
        } catch (e) {
            console.error(' DeviceKvStoreSetSyncRangeCallbackSameTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStoreSetSyncRangeCallbackInvalidArgsTest
     * @tc.desc Test Js Api DeviceKvStore.SetSyncRange() with invalid args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it(' DeviceKvStoreSetSyncRangeCallbackSameTest', 0, async function (done) {
        console.info(' DeviceKvStoreSetSyncRangeCallbackSameTest');
        try {
            var remoteSupportLabels = ['A', 'B'];
            await kvStore.setSyncRange(remoteSupportLabels, function (err) {
                expect(null).assertFail();
            });
        } catch (e) {
            console.error(' DeviceKvStoreSetSyncRangeCallbackSameTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(e.code == 401).assertTrue();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStorePutBatchEntryCallbackStringTest
     * @tc.desc Test Js Api DeviceKvStore.PutBatch() with string value
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStorePutBatchEntryCallbackStringTest', 0, async function (done) {
        console.info('DeviceKvStorePutBatchEntryCallbackStringTest');
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
            console.info('DeviceKvStorePutBatchEntryCallbackStringTest entries: ' + JSON.stringify(entries));
            await kvStore.putBatch(entries, async function (err, data) {
                console.info('DeviceKvStorePutBatchEntryCallbackStringTest putBatch success');
                expect(err == undefined).assertTrue();
                kvStore.getEntries(localDeviceId, 'batch_test_string_key', function (err, entrys) {
                    expect(entrys.length == 10).assertTrue();
                    expect(entrys[0].value.value == 'batch_test_string_value').assertTrue();
                    done();
                });
            });
        } catch (e) {
            console.error('DeviceKvStorePutBatchEntryCallbackStringTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStorePutBatchEntryCallbackIntegerTest
     * @tc.desc Test Js Api DeviceKvStore.PutBatch() with integer value
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStorePutBatchEntryCallbackIntegerTest', 0, async function (done) {
        console.info('DeviceKvStorePutBatchEntryCallbackIntegerTest');
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
            console.info('DeviceKvStorePutBatchEntryCallbackIntegerTest entries: ' + JSON.stringify(entries));
            await kvStore.putBatch(entries, async function (err, data) {
                console.info('DeviceKvStorePutBatchEntryCallbackIntegerTest putBatch success');
                expect(err == undefined).assertTrue();
                kvStore.getEntries(localDeviceId, 'batch_test_number_key', function (err, entrys) {
                    expect(entrys.length == 10).assertTrue();
                    expect(entrys[0].value.value == 222).assertTrue();
                    done();
                });
            });
        } catch (e) {
            console.error('DeviceKvStorePutBatchEntryCallbackIntegerTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStorePutBatchEntryCallbackFloatTest
     * @tc.desc Test Js Api DeviceKvStore.PutBatch() with float value
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStorePutBatchEntryCallbackFloatTest', 0, async function (done) {
        console.info('DeviceKvStorePutBatchEntryCallbackFloatTest');
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
            console.info('DeviceKvStorePutBatchEntryCallbackFloatTest entries: ' + JSON.stringify(entries));
            await kvStore.putBatch(entries, async function (err, data) {
                console.info('DeviceKvStorePutBatchEntryCallbackFloatTest putBatch success');
                expect(err == undefined).assertTrue();
                kvStore.getEntries(localDeviceId, 'batch_test_number_key', function (err, entrys) {
                    expect(entrys.length == 10).assertTrue();
                    expect(entrys[0].value.value == 2.0).assertTrue();
                    done();
                });
            });
        } catch (e) {
            console.error('DeviceKvStorePutBatchEntryCallbackFloatTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStorePutBatchEntryCallbackDoubleTest
     * @tc.desc Test Js Api DeviceKvStore.PutBatch() with double value
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStorePutBatchEntryCallbackDoubleTest', 0, async function (done) {
        console.info('DeviceKvStorePutBatchEntryCallbackDoubleTest');
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
            console.info('DeviceKvStorePutBatchEntryCallbackDoubleTest entries: ' + JSON.stringify(entries));
            await kvStore.putBatch(entries, async function (err, data) {
                console.info('DeviceKvStorePutBatchEntryCallbackDoubleTest putBatch success');
                expect(err == undefined).assertTrue();
                kvStore.getEntries(localDeviceId, 'batch_test_number_key', function (err, entrys) {
                    expect(entrys.length == 10).assertTrue();
                    expect(entrys[0].value.value == 2.00).assertTrue();
                    done();
                });
            });
        } catch (e) {
            console.error('DeviceKvStorePutBatchEntryCallbackDoubleTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStorePutBatchEntryCallbackBooleanTest
     * @tc.desc Test Js Api DeviceKvStore.PutBatch() with boolean value
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStorePutBatchEntryCallbackBooleanTest', 0, async function (done) {
        console.info('DeviceKvStorePutBatchEntryCallbackBooleanTest');
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
            console.info('DeviceKvStorePutBatchEntryCallbackBooleanTest entries: ' + JSON.stringify(entries));
            await kvStore.putBatch(entries, async function (err, data) {
                console.info('DeviceKvStorePutBatchEntryCallbackBooleanTest putBatch success');
                expect(err == undefined).assertTrue();
                kvStore.getEntries(localDeviceId, 'batch_test_bool_key', function (err, entrys) {
                    expect(entrys.length == 10).assertTrue();
                    expect(entrys[0].value.value == bo).assertTrue();
                    done();
                });
            });
        } catch (e) {
            console.error('DeviceKvStorePutBatchEntryCallbackBooleanTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStorePutBatchEntryCallbackByteArrayTest
     * @tc.desc Test Js Api DeviceKvStore.PutBatch() with byte_arrgy value
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStorePutBatchEntryCallbackByteArrayTest', 0, async function (done) {
        console.info('DeviceKvStorePutBatchEntryCallbackByteArrayTest');
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
            console.info('DeviceKvStorePutBatchEntryCallbackByteArrayTest entries: ' + JSON.stringify(entries));
            await kvStore.putBatch(entries, async function (err, data) {
                console.info('DeviceKvStorePutBatchEntryCallbackByteArrayTest putBatch success');
                expect(err == undefined).assertTrue();
                kvStore.getEntries(localDeviceId, 'batch_test_bool_key', function (err, entrys) {
                    expect(entrys.length == 10).assertTrue();
                    expect(entrys[0].value.value.toString() == arr.toString()).assertTrue();
                    done();
                });
            });
        } catch (e) {
            console.error('DeviceKvStorePutBatchEntryCallbackByteArrayTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStorePutBatchValueCallbackUint8ArrayTest
     * @tc.desc Test Js Api DeviceKvStore.PutBatch() with value unit8array
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStorePutBatchValueCallbackUint8ArrayTest', 0, async function (done) {
        console.info('DeviceKvStorePutBatchValueCallbackUint8ArrayTest001');
        try {
            let values = [];
            let arr1 = new Uint8Array([4, 5, 6, 7]);
            let arr2 = new Uint8Array([4, 5, 6, 7, 8]);
            let vb1 = {key: "name_1", value: arr1};
            let vb2 = {key: "name_2", value: arr2};
            values.push(vb1);
            values.push(vb2);
            console.info('DeviceKvStorePutBatchValueCallbackUint8ArrayTest001 values: ' + JSON.stringify(values));
            await kvStore.putBatch(values, async function (err, data) {
                console.info('DeviceKvStorePutBatchValueCallbackUint8ArrayTest001 putBatch success');
                expect(err == undefined).assertTrue();
                var query = new factory.Query();
                query.prefixKey("name_");
                kvStore.getEntries(localDeviceId, query, function (err, entrys) {
                    console.info(JSON.stringify(entrys));
                    expect(entrys.length == 2).assertTrue();
                    done();
                });
            });
        } catch (e) {
            console.error('DeviceKvStorePutBatchValueCallbackUint8ArrayTest001 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStorePutBatchCallbackInvalidArgsTest
     * @tc.desc Test Js Api DeviceKvStore.PutBatch() put invalid args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStorePutBatchCallbackInvalidArgsTest', 0, async function (done) {
        try {
            await kvStore.putBatch(async function (err) {
                if (err == undefined) {
                    expect(null).assertFail();
                } else {
                    expect(null).assertFail();
                }
            });
        } catch (e) {
            console.error('DeviceKvStorePutBatchCallbackInvalidArgsTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(e.code == 401).assertTrue();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStorePutBatchCallbackClosedKvstoreTest
     * @tc.desc Test Js Api DeviceKvStore.PutBatch() put into closed kvstore
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStorePutBatchCallbackClosedKvstoreTest', 0, async function (done) {
        try {
            await kvManager.closeKVStore(TEST_BUNDLE_NAME, TEST_STORE_ID, async function (err) {
                expect(err == undefined).assertTrue();
            });
            let values = [];
            let vb1 = {key: "name_1", value: null};
            let vb2 = {key: "name_2", value: null};
            values.push(vb1);
            values.push(vb2);
            await kvStore.putBatch(values, async function (err) {
                if (err == undefined) {
                    expect(null).assertFail();
                } else {
                    expect(err.code == 15100005).assertFail();
                }
            });
        } catch (e) {
            console.error('DeviceKvStorePutBatchCallbackClosedKvstoreTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreDeleteBatchCallbackSucTest
     * @tc.desc Test Js Api DeviceKvStore.DeleteBatch() successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreDeleteBatchCallbackSucTest', 0, async function (done) {
        console.info('DeviceKvStoreDeleteBatchCallbackSucTest');
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
            console.info('DeviceKvStoreDeleteBatchCallbackSucTest entries: ' + JSON.stringify(entries));
            await kvStore.putBatch(entries, async function (err, data) {
                console.info('DeviceKvStoreDeleteBatchCallbackSucTest putBatch success');
                expect(err == undefined).assertTrue();
                await kvStore.deleteBatch(keys, async function (err, data) {
                    console.info('DeviceKvStoreDeleteBatchCallbackSucTest deleteBatch success');
                    expect(err == undefined).assertTrue();
                    done();
                });
            });
        } catch (e) {
            console.error('DeviceKvStoreDeleteBatchCallbackSucTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStoreDeleteBatchCallbackInvalidArgsTest
     * @tc.desc Test Js Api DeviceKvStore.DeleteBatch() with invalid args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreDeleteBatchCallbackInvalidArgsTest', 0, async function (done) {
        console.info('DeviceKvStoreDeleteBatchCallbackInvalidArgsTest');
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
            await kvStore.putBatch(entries, async function (err) {
                expect(err == undefined).assertTrue();
                await kvStore.deleteBatch(1, async function (err) {
                    if (err == undefined) {
                        expect(null).assertFail();
                    } else {
                        expect(null).assertFail();
                    }
                });
            });
        } catch (e) {
            console.error('DeviceKvStoreDeleteBatchCallbackInvalidArgsTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(e.code == 401).assertTrue();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreDeleteBatchCallbackClosedKVStoreTest
     * @tc.desc Test Js Api DeviceKvStore.DeleteBatch() with closed kvstore
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreDeleteBatchCallbackClosedKVStoreTest', 0, async function (done) {
        console.info('DeviceKvStoreDeleteBatchCallbackClosedKVStoreTest');
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
            await kvStore.putBatch(entries, async function (err) {
                expect(err == undefined).assertTrue();
                await kvManager.closeKVStore(TEST_BUNDLE_NAME, TEST_STORE_ID, async function (err) {
                    expect(err == undefined).assertTrue();
                    await kvStore.deleteBatch(keys, async function (err) {
                        if (err == undefined) {
                            expect(null).assertFail();
                        } else {
                            expect(e.code == 15100005).assertTrue();
                        }
                    });
                });
            });
        } catch (e) {
            console.error('DeviceKvStoreDeleteBatchCallbackClosedKVStoreTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreGetEntriesCallbackQueryTest
     * @tc.desc Test Js Api DeviceKvStore.GetEntries() with query
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreGetEntriesCallbackQueryTest', 0, async function (done) {
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
            await kvStore.putBatch(entries, async function (err) {
                expect(err == undefined).assertTrue();
                let query = new factory.Query();
                query.prefixKey("batch_test");
                kvStore.getEntries(localDeviceId, query, function (err, entrys) {
                    expect(entrys.length == 10).assertTrue();
                    expect(entrys[0].value.value.toString() == arr.toString()).assertTrue();
                    done();
                });
            });
        } catch (e) {
            console.error('DeviceKvStoreGetEntriesCallbackQueryTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreGetEntriesCallbackQueryClosedKVStoreTest
     * @tc.desc Test Js Api DeviceKvStore.GetEntries() query from a closed kvstore
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreGetEntriesCallbackQueryClosedKVStoreTest', 0, async function (done) {
        try {
            var arr = new Uint8Array([21, 31]);
            let entries = [];
            for (let i = 0; i < 10; i++) {
                let key = 'batch_test_bool_key';
                let entry = {
                    key: key + i,
                    value: {
                        type: factory.ValueType.BYTE_ARRAY,
                        value: arr
                    }
                }
                entries.push(entry);
            }
            await kvStore.putBatch(entries, async function (err) {
                expect(err == undefined).assertTrue();
                await kvManager.closeKVStore(TEST_BUNDLE_NAME, TEST_STORE_ID, function (err) {
                    expect(err == undefined).assertTrue();
                    let query = new factory.Query();
                    query.prefixKey("batch_test");
                    kvStore.getEntries(localDeviceId, query, function (err) {
                        if (err == undefined) {
                            expect(null).assertFail();
                        } else {
                            expect(err.code == 15100005).assertTrue();
                        }
                    });
                });
            });
        } catch (e) {
            console.error('DeviceKvStoreGetEntriesCallbackQueryClosedKVStoreTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreGetEntriesCallbackSucTest
     * @tc.desc Test Js Api DeviceKvStore.GetEntries() success
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreGetEntriesCallbackSucTest', 0, async function (done) {
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
            await kvStore.putBatch(entries, async function (err) {
                expect(err == undefined).assertTrue();
                kvStore.getEntries(localDeviceId, "batch_test", function (err, entrys) {
                    expect(entrys.length == 10).assertTrue();
                    expect(entrys[0].value.value.toString() == arr.toString()).assertTrue();
                    done();
                });
            });
        } catch (e) {
            console.error('DeviceKvStoreGetEntriesCallbackSucTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreGetEntriesCallbackClosedKVStoreTest
     * @tc.desc Test Js Api DeviceKvStore.GetEntries() from a closed kvstore
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreGetEntriesCallbackClosedKVStoreTest', 0, async function (done) {
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
            await kvStore.putBatch(entries, async function (err) {
                expect(err == undefined).assertTrue();
                await kvManager.closeKVStore(TEST_BUNDLE_NAME, TEST_STORE_ID, function (err) {
                    expect(err == undefined).assertTrue();
                    kvStore.getEntries(localDeviceId, "batch_test", function (err) {
                        if (err == undefined) {
                            expect(null).assertFail();
                        } else {
                            expect(err.code == 15100005).assertTrue();
                        }
                    });
                });
            });
        } catch (e) {
            console.error('DeviceKvStoreGetEntriesCallbackClosedKVStoreTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreGetEntriesCallbackInvalidArgsTest
     * @tc.desc Test Js Api DeviceKvStore.GetEntries() with invalid args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreGetEntriesCallbackInvalidArgsTest', 0, async function (done) {
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
            await kvStore.putBatch(entries, async function (err) {
                expect(err == undefined).assertTrue();
                kvStore.getEntries(localDeviceId, "batch_test", function (err, entrys) {
                    if (err == undefined) {
                        expect(null).assertFail();
                    } else {
                        expect(null).assertFail();
                    }
                });
            });
        } catch (e) {
            console.error('DeviceKvStoreGetEntriesCallbackInvalidArgsTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(e.code == 401).assertTrue();
        }
        done();
    })


    /**
     * @tc.name DeviceKvstoreStartTransactionCallbackCommitTest
     * @tc.desc Test Js Api DeviceKvStore.startTransaction() with commit
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvstoreStartTransactionCallbackCommitTest', 0, async function (done) {
        console.info('DeviceKvstoreStartTransactionCallbackCommitTest');
        try {
            var count = 0;
            kvStore.on('dataChange', 0, function (data) {
                console.info('DeviceKvstoreStartTransactionCallbackCommitTest 0' + data)
                count++;
            });
            await kvStore.startTransaction(async function (err, data) {
                expect(err == undefined).assertTrue();
                let entries = putBatchString(10, 'batch_test_string_key');
                await kvStore.putBatch(entries, async function (err, data) {
                    expect(err == undefined).assertTrue();
                    let keys = Object.keys(entries).slice(5);
                    await kvStore.deleteBatch(keys, async function (err, data) {
                        expect(err == undefined).assertTrue();
                        await kvStore.commit(async function (err, data) {
                            expect(err == undefined).assertTrue();
                            await sleep(2000);
                            expect(count == 1).assertTrue();
                            done();
                        });
                    });
                });
            });
        } catch (e) {
            console.error('DeviceKvstoreStartTransactionCallbackCommitTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvstoreStartTransactionCallbackRollbackTest
     * @tc.desc Test Js Api DeviceKvStore.startTransaction() with rollback
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvstoreStartTransactionCallbackRollbackTest', 0, async function (done) {
        try {
            var count = 0;
            kvStore.on('dataChange', 0, function (data) {
                count++;
            });
            await kvStore.startTransaction(async function (err, data) {
                expect(err == undefined).assertTrue();
                let entries = putBatchString(10, 'batch_test_string_key');
                await kvStore.putBatch(entries, async function (err, data) {
                    expect(err == undefined).assertTrue();
                    let keys = Object.keys(entries).slice(5);
                    await kvStore.deleteBatch(keys, async function (err, data) {
                        expect(err == undefined).assertTrue();
                        await kvStore.rollback(async function (err, data) {
                            expect(err == undefined).assertTrue();
                            await sleep(2000);
                            expect(count == 0).assertTrue();
                            done();
                        });
                    });
                });
            });
        } catch (e) {
            console.error('DeviceKvstoreStartTransactionCallbackRollbackTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvstoreStartTransactionCallbackClosedKVStoreTest
     * @tc.desc Test Js Api DeviceKvStore.startTransaction() with closed kvstore
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvstoreStartTransactionCallbackClosedKVStoreTest', 0, async function (done) {
        try {
            await kvManager.closeKVStore(TEST_BUNDLE_NAME, TEST_STORE_ID, function (err) {
                expect(err == undefined).assertTrue();
            });
            await kvStore.startTransaction(function (err) {
                if (err == undefined) {
                    expect(null).assertFail();
                } else {
                    expect(err.code == 15100005).assertTrue();
                }
                done();
            });
        } catch (e) {
            console.error('DeviceKvstoreStartTransactionCallbackClosedKVStoreTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStoreCommitCallbackClosedKVStoreTest
     * @tc.desc Test Js Api DeviceKvStore.Commit() with closed kvstore
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreCommitCallbackClosedKVStoreTest', 0, async function (done) {
        console.info('DeviceKvStoreCommitCallbackClosedKVStoreTest');
        try {
            await kvManager.closeKVStore(TEST_BUNDLE_NAME, TEST_STORE_ID, function (err) {
                expect(err == undefined).assertTrue();
            });
            await kvStore.commit(function (err) {
                if (err == undefined) {
                    expect(null).assertFail();
                } else {
                    expect(err.code == 15100005).assertTrue();
                }
                done();
            });
        } catch (e) {
            console.error('DeviceKvStoreCommitCallbackClosedKVStoreTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStoreRollbackCallbackClosedKVStoreTest
     * @tc.desc Test Js Api DeviceKvStore.Rollback() with closed kvstore
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreRollbackCallbackClosedKVStoreTest', 0, async function (done) {
        console.info('DeviceKvStoreRollbackCallbackClosedKVStoreTest');
        try {
            await kvManager.closeKVStore(TEST_BUNDLE_NAME, TEST_STORE_ID, function (err) {
                expect(err == undefined).assertTrue();
            });
            await kvStore.rollback(function (err) {
                if (err == undefined) {
                    expect(null).assertFail();
                } else {
                    expect(err.code == 15100005).assertTrue();
                }
                done();
            });
        } catch (e) {
            console.error('DeviceKvStoreRollbackCallbackClosedKVStoreTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStoreEnableSyncCallbackTrueTest
     * @tc.desc Test Js Api DeviceKvStore.EnableSync() with mode true
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
     * @tc.desc Test Js Api DeviceKvStore.EnableSync() with mode false
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
     * @tc.name DeviceKvStoreEnableSyncCallbackInvalidArgsTest
     * @tc.desc Test Js Api DeviceKvStore.EnableSync() with invlid args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreEnableSyncCallbackInvalidArgsTest', 0, async function (done) {
        console.info('DeviceKvStoreEnableSyncCallbackInvalidArgsTest');
        try {
            await kvStore.enableSync(function (err, data) {
                if (err == undefined) {
                    console.info('DeviceKvStoreEnableSyncCallbackInvalidArgsTest enableSync success');
                    expect(null).assertFail();
                } else {
                    console.info('DeviceKvStoreEnableSyncCallbackInvalidArgsTest enableSync fail');
                    expect(null).assertFail();
                }
                done();
            });
        } catch (e) {
            console.error('DeviceKvStoreEnableSyncCallbackInvalidArgsTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(e.code == 401).assertTrue();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStoreRemoveDeviceDataCallbackClosedKvstoreTest
     * @tc.desc Test Js Api DeviceKvStore.RemoveDeviceData() in a closed kvstore
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreRemoveDeviceDataCallbackClosedKvstoreTest', 0, async function (done) {
        try {
            await kvStore.put(KEY_TEST_STRING_ELEMENT, VALUE_TEST_STRING_ELEMENT, async function (err, data) {
                expect(err == undefined).assertTrue();
            });
            var deviceid = 'no_exist_device_id';
            await kvManager.closeKVStore(TEST_BUNDLE_NAME, TEST_STORE_ID, function (err) {
                expect(err == undefined).assertTrue();
            });
            await kvStore.removeDeviceData(deviceid, async function (err) {
                if (err == undefined) {
                    expect(null).assertFail();
                    done();
                } else {
                    expect(err.code == 15100005).assertTrue();
                }
            });
        } catch (e) {
            console.error('DeviceKvStoreRemoveDeviceDataCallbackClosedKvstoreTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
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
        try {
            await kvStore.removeDeviceData(function (err) {
                if (err == undefined) {
                    expect(null).assertFail();
                } else {
                    expect(null).assertFail();
                }
                done();
            });
        } catch (e) {
            console.error('DeviceKvStoreRemoveDeviceDataCallbackInvalidArgsTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(e.code == 401).assertTrue();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStoreGetResultSetCallbackSucTest
     * @tc.desc Test Js Api DeviceKvStore.GetResultSet() successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreGetResultSetCallbackSucTest', 0, async function (done) {
        console.info('DeviceKvStoreGetResultSetCallbackSucTest');
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
                console.info('DeviceKvStoreGetResultSetCallbackSucTest putBatch success');
                expect(err == undefined).assertTrue();
                await kvStore.getResultSet(localDeviceId, 'batch_test_string_key', async function (err, result) {
                    console.info('DeviceKvStoreGetResultSetCallbackSucTest getResultSet success');
                    resultSet = result;
                    expect(resultSet.getCount() == 10).assertTrue();
                    await kvStore.closeResultSet(resultSet, function (err, data) {
                        console.info('DeviceKvStoreGetResultSetCallbackSucTest closeResultSet success');
                        expect(err == undefined).assertTrue();
                        done();
                    })
                });
            });
        } catch (e) {
            console.error('DeviceKvStoreGetResultSetCallbackSucTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStoreGetResultSetCallbackInvalidArgsTest
     * @tc.desc Test Js Api DeviceKvStore.GetResultSet() with invalid args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreGetResultSetCallbackInvalidArgsTest', 0, async function (done) {
        console.info('DeviceKvStoreGetResultSetCallbackInvalidArgsTest');
        try {
            let resultSet;
            await kvStore.getResultSet(function () {
                console.info('DeviceKvStoreGetResultSetCallbackInvalidArgsTest getResultSet success');
                expect(null).assertFail();
                done();
            });
        } catch (e) {
            console.error('DeviceKvStoreGetResultSetCallbackInvalidArgsTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(e.code == 401).assertTrue();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStoreGetResultSetPredicatesCallbackTest
     * @tc.desc Test Js Api DeviceKvStore.GetResultSet() with predicates
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreGetResultSetPredicatesCallbackTest', 0, async function (done) {
        console.log('DeviceKvStoreGetResultSetPredicatesCallbackTest');
        try {
            let predicates = new dataShare.DataSharePredicates();
            await kvStore.getResultSet(localDeviceId, predicates).then((result) => {
                console.log('DeviceKvStoreGetResultSetPredicatesCallbackTest getResultSet success');
                expect(true).assertTrue();
                kvStore.closeResultSet(result).then(() => {
                    expect(true).assertTrue();
                });
            }).catch((err) => {
                console.error('DeviceKvStoreGetResultSetPredicatesCallbackTest getResultSet fail ' + err`, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('DeviceKvStoreGetResultSetPredicatesCallbackTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreGetResultSetQueryCallbackTest
     * @tc.desc Test Js Api DeviceKvStore.GetResultSet() with query
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreGetResultSetQueryCallbackTest', 0, async function (done) {
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
                expect(err == undefined).assertTrue();
                let query = new factory.Query();
                query.prefixKey("batch_test");
                await kvStore.getResultSet(localDeviceId, query, async function (err, result) {
                    resultSet = result;
                    expect(resultSet.getCount() == 10).assertTrue();
                    await kvStore.closeResultSet(resultSet, function (err, data) {
                        expect(err == undefined).assertTrue();
                        done();
                    })
                });
            });
        } catch (e) {
            console.error('DeviceKvStoreGetResultSetQueryCallbackTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
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
     * @tc.name DeviceKvStoreCloseResultSetCallbackInvalidArgsTest
     * @tc.desc Test Js Api DeviceKvStore.CloseResultSet() with invalid args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreCloseResultSetCallbackInvalidArgsTest', 0, async function (done) {
        console.info('DeviceKvStoreCloseResultSetCallbackInvalidArgsTest');
        try {
            console.info('DeviceKvStoreCloseResultSetCallbackInvalidArgsTest success');
            await kvStore.closeResultSet(function (err, data) {
                if (err == undefined) {
                    console.info('DeviceKvStoreCloseResultSetCallbackInvalidArgsTest closeResultSet success');
                    expect(null).assertFail();
                } else {
                    console.info('DeviceKvStoreCloseResultSetCallbackInvalidArgsTest closeResultSet fail');
                    expect(null).assertFail();
                }
                done();
            });
        } catch (e) {
            console.error('DeviceKvStoreCloseResultSetCallbackInvalidArgsTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(e.code == 401).assertTrue();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStoreGetResultSizeCallbackQueryTest
     * @tc.desc Test Js Api DeviceKvStoreGetResultSize with query
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreGetResultSizeCallbackQueryTest', 0, async function (done) {
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
            await kvStore.putBatch(entries, async function (err) {
                expect(err == undefined).assertTrue();
            });
            var query = new factory.Query();
            query.prefixKey("batch_test");
            await kvStore.getResultSize(localDeviceId, query, async function (resultSize, err) {
                expect(err == undefined).assertTrue();
                expect(resultSize == 10).assertTrue();
            })
        } catch (e) {
            console.error('DeviceKvStoreGetResultSizePromiseQueryTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreGetResultSizeCallbackInvalidArgsTest
     * @tc.desc Test Js Api DeviceKvStoreGetResultSize with invalid args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreGetResultSizeCallbackInvalidArgsTest', 0, async function (done) {
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
            await kvStore.putBatch(entries, async function (err) {
                expect(err == undefined).assertTrue();
            });
            await kvStore.getResultSize(localDeviceId, async function (err) {
                expect(null).assertFail();
            })
        } catch (e) {
            console.error('DeviceKvStoreGetResultSizeCallbackInvalidArgsTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(e.code == 401).assertTrue();
        }
        done();
    })

    /**
     * @tc.name DeviceKvStoreGetResultSizeCallbackClosedKVStoreTest
     * @tc.desc Test Js Api DeviceKvStoreGetResultSize from a closed kvstore
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreGetResultSizeCallbackClosedKVStoreTest', 0, async function (done) {
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
            await kvStore.putBatch(entries, async function (err) {
                expect(err == undefined).assertTrue();
            });
            await kvManager.closeKVStore(TEST_BUNDLE_NAME, TEST_STORE_ID, function (err) {
                expect(err == undefined).assertTrue();
            });
            var query = new factory.Query();
            query.prefixKey("batch_test");
            await kvStore.getResultSize(localDeviceId, query, function (err) {
                if (err == undefined) {
                    expect(null).assertFail();
                } else {
                    expect(err.code == 15100005).assertTrue();
                }
            });
        } catch (e) {
            console.error('DeviceKvStoreGetResultSizeCallbackClosedKVStoreTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
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
