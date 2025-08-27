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

    afterAll(function (done) {
        console.info('afterAll');
        kvManager = null;
        kvStore = null;
        done();
    })

    beforeEach(function (done) {
        console.info('beforeEach' + JSON.stringify(options));
        kvManager.getKVStore(TEST_STORE_ID, options, function (err, store) {
            if (err) {
                console.error('beforeEach getKVStore fail' + `, error code is ${err.code}, message is ${err.message}`);
                done();
                return;
            }
            kvStore = store;
            console.info('beforeEach getKVStore success');
            done();
        });
    })

    afterEach(function (done) {
        console.info('afterEach');
        try {
            kvManager.closeKVStore(TEST_BUNDLE_NAME, TEST_STORE_ID, function (err, data) {
                if (err) {
                    console.error('afterEach closeKVStore fail' + `, error code is ${err.code}, message is ${err.message}`);
                    done();
                }
                console.info('afterEach closeKVStore success: err is: ' + err);
                kvManager.deleteKVStore(TEST_BUNDLE_NAME, TEST_STORE_ID, function (err, data) {
                    if (err) {
                        console.error('afterEach deleteKVStore fail' + `, error code is ${err.code}, message is ${err.message}`);
                        done();
                    }
                    console.info('afterEach deleteKVStore success err is: ' + err);
                    kvStore = null;
                    done();
                });
            });
        } catch (e) {
            console.error('afterEach closeKVStore err ' + `, error code is ${e.code}, message is ${e.message}`);
            done();
        }
    })

    /**
     * @tc.name DeviceKvStorePutStringCallbackSucTest
     * @tc.desc Test Js Api DeviceKvStore.Put(String) successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStorePutStringCallbackSucTest', 0, function (done) {
        console.info('DeviceKvStorePutStringCallbackSucTest');
        try {
            kvStore.put(KEY_TEST_STRING_ELEMENT, VALUE_TEST_STRING_ELEMENT, function (err, data) {
                if (err) {
                    console.error('DeviceKvStorePutStringCallbackSucTest fail' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(null).assertFail();
                    done();
                }
                console.info('DeviceKvStorePutStringCallbackSucTest put success');
                done();
            });
        } catch (e) {
            console.error('DeviceKvStorePutStringCallbackSucTest put e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStorePutStringCallbackInvalidArgsTest
     * @tc.desc Test Js Api DeviceKvStore.Put(String) with invalid args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStorePutStringCallbackInvalidArgsTest', 0, function (done) {
        console.info('DeviceKvStorePutStringCallbackInvalidArgsTest');
        try {
            kvStore.put(KEY_TEST_STRING_ELEMENT, function (err, data) {
                if (err) {
                    console.error('DeviceKvStorePutStringCallbackInvalidArgsTest fail' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(null).assertFail();
                    done();
                }
                expect(null).assertFail();
                console.info('DeviceKvStorePutStringCallbackInvalidArgsTest put success');
                done();
            });
        } catch (e) {
            console.error('DeviceKvStorePutStringCallbackInvalidArgsTest put e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(e.code == 401).assertTrue();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStorePutStringCallbackClosedKvStoreTest
     * @tc.desc Test Js Api DeviceKvStore.Put(String) with closed database
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStorePutStringCallbackClosedKvStoreTest', 0, function (done) {
        console.info('DeviceKvStorePutStringCallbackClosedKvStoreTest');
        try {
            kvManager.closeKVStore(TEST_BUNDLE_NAME, TEST_STORE_ID, function (err) {
                expect(err == undefined).assertTrue();
                kvStore.put(KEY_TEST_STRING_ELEMENT, VALUE_TEST_STRING_ELEMENT, function (err) {
                    if (err) {
                        console.error('DeviceKvStorePutStringCallbackClosedKvStoreTest put fail' + `, error code is ${err.code}, message is ${err.message}`);
                        expect(err.code == 15100005).assertTrue();
                        done();
                        return;
                    }
                    expect(null).assertFail();
                    console.info('DeviceKvStorePutStringCallbackClosedKvStoreTest put success');
                    done();
                });
            });
        } catch (e) {
            console.error('DeviceKvStorePutStringCallbackClosedKvStoreTest put e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStoreGetStringCallbackNoPutTest
     * @tc.desc Test Js Api DeviceKvStore.GetString() with no put
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreGetStringCallbackNoPutTest', 0, function (done) {
        console.info('DeviceKvStoreGetStringCallbackNoPutTest');
        try {
            kvStore.get(localDeviceId, KEY_TEST_STRING_ELEMENT, function (err) {
                if (err) {
                    console.info('DeviceKvStoreGetStringCallbackNoPutTest get fail');
                    expect(err.code == 15100004).assertTrue();
                    done();
                    return;
                }
                console.info('DeviceKvStoreGetStringCallbackNoPutTest get success');
                expect(null).assertFail();
                done();
            });
        } catch (e) {
            console.error('DeviceKvStoreGetStringCallbackTest get e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStoreGetStringCallbackSucTest
     * @tc.desc Test Js Api DeviceKvStore.GetString() successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreGetStringCallbackSucTest', 0, function (done) {
        console.info('DeviceKvStoreGetStringCallbackSucTest');
        try {
            kvStore.put(KEY_TEST_STRING_ELEMENT, VALUE_TEST_STRING_ELEMENT, function (err) {
                if (err) {
                    console.error('DeviceKvStoreGetStringCallbackSucTest put fail' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(null).assertFail();
                    done();
                }
                console.info('DeviceKvStoreGetStringCallbackSucTest put success');
                expect(err == undefined).assertTrue();
                kvStore.get(localDeviceId, KEY_TEST_STRING_ELEMENT, function (err, data) {
                    if (err) {
                        console.error('DeviceKvStoreGetStringCallbackSucTest get fail' + `, error code is ${err.code}, message is ${err.message}`);
                        expect(null).assertFail();
                        done();
                    }
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
     * @tc.name DeviceKvStoreGetStringCallbackClosedKVStoreTest
     * @tc.desc Test Js Api DeviceKvStore.GetString() from a closed kvstore
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreGetStringCallbackClosedKVStoreTest', 0, function (done) {
        try {
            kvStore.put(KEY_TEST_STRING_ELEMENT, VALUE_TEST_STRING_ELEMENT, function (err) {
                if (err) {
                    expect(null).assertFail();
                    done();
                    return;
                }
                expect(err == undefined).assertTrue();
                kvManager.closeKVStore(TEST_BUNDLE_NAME, TEST_STORE_ID, function (err) {
                    if (err) {
                        expect(null).assertFail();
                        done();
                        return;
                    }
                    expect(err == undefined).assertTrue();
                    kvStore.get(localDeviceId, KEY_TEST_STRING_ELEMENT, function (err) {
                        if (err) {
                            expect(err.code == 15100005).assertTrue();
                            done();
                            return;
                        }
                        expect(null).assertFail();
                        done();
                    });
                });
            })
        } catch (e) {
            console.error('DeviceKvStoreGetStringCallbackClosedKVStoreTest get e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStoreGetStringCallbackInvalidArgsTest
     * @tc.desc Test Js Api DeviceKvStore.GetString() with invalid args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreGetStringCallbackInvalidArgsTest', 0, function (done) {
        try {
            kvStore.put(KEY_TEST_STRING_ELEMENT, VALUE_TEST_STRING_ELEMENT, function (err) {
                if (err) {
                    console.error('DeviceKvStoreGetStringCallbackInvalidArgsTest put fail' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(null).assertFail();
                    done();
                    return;
                }
                console.info('DeviceKvStoreGetStringCallbackInvalidArgsTest put success');
                expect(err == undefined).assertTrue();
                try {
                    kvStore.get(function (err, data) {
                        if (err) {
                            console.error('DeviceKvStoreGetStringCallbackInvalidArgsTest get fail' + `, error code is ${err.code}, message is ${err.message}`);
                            expect(null).assertFail();
                            done();
                            return;
                        }
                        expect(null).assertFail();
                        done();
                    });
                } catch (e) {
                    console.error('DeviceKvStoreGetStringCallbackInvalidArgsTest get error' + `, error code is ${e.code}, message is ${e.message}`);
                    expect(e.code == 401).assertTrue();
                    done();
                }
            })
        } catch (e) {
            console.error('DeviceKvStoreGetStringCallbackInvalidArgsTest get e' + `, error code is ${e.code}, message is ${e.message}`);
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
    it('DeviceKvStorePutIntCallbackSucTest', 0, function (done) {
        console.info('DeviceKvStorePutIntCallbackSucTest');
        try {
            kvStore.put(KEY_TEST_INT_ELEMENT, VALUE_TEST_INT_ELEMENT, function (err) {
                if (err) {
                    console.error('DeviceKvStorePutIntCallbackSucTest put fail' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(null).assertFail();
                    done();
                }
                console.info('DeviceKvStorePutIntCallbackSucTest put success');
                expect(err == undefined).assertTrue();
                kvStore.get(localDeviceId, KEY_TEST_INT_ELEMENT, function (err, data) {
                    if (err) {
                        console.error('DeviceKvStorePutIntCallbackSucTest get fail' + `, error code is ${err.code}, message is ${err.message}`);
                        expect(null).assertFail();
                        done();
                    }
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
     * @tc.name DeviceKvStorePutIntCallbackMaxTest
     * @tc.desc Test Js Api DeviceKvStore.Put(Int) with max value
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStorePutIntCallbackMaxTest', 0, function (done) {
        console.info('DeviceKvStorePutIntCallbackMaxTest');
        try {
            var intValue = Number.MIN_VALUE;
            kvStore.put(KEY_TEST_INT_ELEMENT, intValue, function (err) {
                console.info('DeviceKvStorePutIntCallbackMaxTest put success');
                expect(err == undefined).assertTrue();
                kvStore.get(localDeviceId, KEY_TEST_INT_ELEMENT, function (err, data) {
                    console.info('DeviceKvStorePutIntCallbackMaxTest get success');
                    expect((err == undefined) && (intValue == data)).assertTrue();
                    done()
                })
            });
        } catch (e) {
            console.error('DeviceKvStorePutIntCallbackMaxTest put e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done()
        }
    })

    /**
     * @tc.name DeviceKvStorePutIntCallbackMinTest
     * @tc.desc Test Js Api DeviceKvStore.Put(Int) with min value
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStorePutIntCallbackMinTest', 0, function (done) {
        console.info('DeviceKvStorePutIntCallbackMinTest');
        try {
            var intValue = Number.MAX_VALUE;
            kvStore.put(KEY_TEST_INT_ELEMENT, intValue, function (err) {
                console.info('DeviceKvStorePutIntCallbackMinTest put success');
                expect(err == undefined).assertTrue();
                kvStore.get(localDeviceId, KEY_TEST_INT_ELEMENT, function (err, data) {
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
     * @tc.name DeviceKvStoreGetIntCallbackNonExistTest
     * @tc.desc Test Js Api DeviceKvStore.GetInt() get non-exsiting int
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreGetIntCallbackTest', 0, function (done) {
        console.info('DeviceKvStoreGetIntCallbackTest');
        try {
            kvStore.get(localDeviceId, KEY_TEST_INT_ELEMENT, function (err) {
                if (err) {
                    console.error('DeviceKvStoreGetIntCallbackTest get fail');
                    expect(err.code == 15100004).assertTrue();
                    done();
                    return;
                }
                console.info('DeviceKvStoreGetIntCallbackTest get success');
                expect(null).assertFail();
                done();
            })
        } catch (e) {
            console.error('DeviceKvStoreGetIntCallbackTest put e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStorePutBoolCallbackTest
     * @tc.desc Test Js Api DeviceKvStore.Put(Bool) successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStorePutBoolCallbackTest', 0, function (done) {
        console.info('DeviceKvStorePutBoolCallbackTest');
        try {
            kvStore.put(KEY_TEST_BOOLEAN_ELEMENT, VALUE_TEST_BOOLEAN_ELEMENT, function (err) {
                console.info('DeviceKvStorePutBoolCallbackTest put success');
                expect(err == undefined).assertTrue();
                done();
            });
        } catch (e) {
            console.error('DeviceKvStorePutBoolCallbackTest fail' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStoreGetBoolCallbackSucTest
     * @tc.desc Test Js Api DeviceKvStore.GetBool() successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreGetBoolCallbackSucTest', 0, function (done) {
        console.info('DeviceKvStoreGetBoolCallbackSucTest');
        try {
            kvStore.put(KEY_TEST_BOOLEAN_ELEMENT, VALUE_TEST_BOOLEAN_ELEMENT, function (err, data) {
                console.info('DeviceKvStoreGetBoolCallbackSucTest put success');
                expect(err == undefined).assertTrue();
                kvStore.get(localDeviceId, KEY_TEST_BOOLEAN_ELEMENT, function (err, data) {
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
    it('DeviceKvStorePutFloatCallbackSucTest', 0, function (done) {
        console.info('DeviceKvStorePutFloatCallbackSucTest');
        try {
            kvStore.put(KEY_TEST_FLOAT_ELEMENT, VALUE_TEST_FLOAT_ELEMENT, function (err, data) {
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
    it('DeviceKvStoreGetFloatCallbackSucTest', 0, function (done) {
        console.info('DeviceKvStoreGetFloatCallbackSucTest');
        try {
            kvStore.put(KEY_TEST_FLOAT_ELEMENT, VALUE_TEST_FLOAT_ELEMENT, function (err, data) {
                console.info('DeviceKvStoreGetFloatCallbackSucTest put success');
                expect(err == undefined).assertTrue();
                kvStore.get(localDeviceId, KEY_TEST_FLOAT_ELEMENT, function (err, data) {
                    if (err) {
                        console.error('DeviceKvStoreGetFloatCallbackSucTest get fail' + `, error code is ${err.code}, message is ${err.message}`);
                        expect(null).assertFail();
                        done();
                    }
                    console.info('DeviceKvStoreGetFloatCallbackSucTest get success');
                    expect(true).assertTrue();
                    done();
                });
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
    it('DeviceKvStoreDeleteStringCallbackSucTest', 0, function (done) {
        console.info('DeviceKvStoreDeleteStringCallbackSucTest');
        try {
            kvStore.put(KEY_TEST_STRING_ELEMENT, VALUE_TEST_STRING_ELEMENT, function (err, data) {
                if (err) {
                    expect(null).assertFail();
                    done();
                }
                console.info('DeviceKvStoreDeleteStringCallbackSucTest put success');
                kvStore.delete(KEY_TEST_STRING_ELEMENT, function (err, data) {
                    if (err) {
                        expect(null).assertFail();
                        done();
                    }
                    console.info('DeviceKvStoreDeleteStringCallbackSucTest delete success');
                    expect(true).assertTrue();
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
    it('DeviceKvStoreDeleteStringCallbackInvalidArgsTest', 0, function (done) {
        console.info('DeviceKvStoreDeleteStringCallbackInvalidArgsTest');
        try {
            kvStore.put(KEY_TEST_STRING_ELEMENT, VALUE_TEST_STRING_ELEMENT, function (err, data) {
                console.info('DeviceKvStoreDeleteStringCallbackInvalidArgsTest put success');
                expect(err == undefined).assertTrue();
                try {
                    kvStore.delete(function (err) {
                        console.info('DeviceKvStoreDeleteStringCallbackInvalidArgsTest delete success');
                        expect(null).assertFail();
                        done();
                    });
                } catch (e) {
                    console.error('DeviceKvStoreDeleteStringCallbackInvalidArgsTest delete fail' + `, error code is ${e.code}, message is ${e.message}`);
                    expect(e.code == 401).assertTrue();
                    done();
                }
            })
        } catch (e) {
            console.error('DeviceKvStoreDeleteStringCallbackInvalidArgsTest put fail' + `, error code is ${e.code}, message is ${e.message}`);
            expect(e.code == 401).assertTrue();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStoreDeleteCallbackClosedKVStoreTest
     * @tc.desc Test Js Api DeviceKvStore.Delete() into a closed kvstore
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreDeleteCallbackClosedKVStoreTest', 0, function (done) {
        try {
            kvStore.put(KEY_TEST_STRING_ELEMENT, VALUE_TEST_STRING_ELEMENT, function (err) {
                if (err) {
                    console.error('DeviceKvStorePutStringCallbackClosedKvStoreTest put fail' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(null).assertFail();
                    done();
                    return;
                }
                expect(true).assertTrue();
                console.info('DeviceKvStorePutStringCallbackClosedKvStoreTest put success');
                kvManager.closeKVStore(TEST_BUNDLE_NAME, TEST_STORE_ID, function (err) {
                    if (err) {
                        expect(null).assertFail();
                        done();
                        return;
                    }
                    expect(err == undefined).assertTrue();
                    kvStore.delete(KEY_TEST_STRING_ELEMENT, function (err) {
                        if (err) {
                            expect(err.code == 15100005).assertTrue();
                            done();
                            return;
                        }
                        expect(null).assertFail();
                        done();
                    });
                });
            });
        } catch (e) {
            console.error('DeviceKvStorePutStringCallbackClosedKvStoreTest put e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStoreDeleteIntCallbackSucTest
     * @tc.desc Test Js Api DeviceKvStore.DeleteInt() successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreDeleteIntCallbackSucTest', 0, function (done) {
        console.info('DeviceKvStoreDeleteIntCallbackSucTest');
        try {
            kvStore.put(KEY_TEST_INT_ELEMENT, VALUE_TEST_INT_ELEMENT, function (err, data) {
                console.info('DeviceKvStoreDeleteIntCallbackSucTest put success');
                expect(err == undefined).assertTrue();
                kvStore.delete(KEY_TEST_INT_ELEMENT, function (err, data) {
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
    it('DeviceKvStoreDeleteFloatCallbackSucTest', 0, function (done) {
        console.info('DeviceKvStoreDeleteFloatCallbackSucTest');
        try {
            kvStore.put(KEY_TEST_FLOAT_ELEMENT, VALUE_TEST_FLOAT_ELEMENT, function (err, data) {
                console.info('DeviceKvStoreDeleteFloatCallbackSucTest put success');
                expect(err == undefined).assertTrue();
                kvStore.delete(KEY_TEST_FLOAT_ELEMENT, function (err, data) {
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
    it('DeviceKvStoreDeleteBoolCallbackSucTest', 0, function (done) {
        console.info('DeviceKvStoreDeleteBoolCallbackSucTest');
        try {
            kvStore.put(KEY_TEST_BOOLEAN_ELEMENT, VALUE_TEST_BOOLEAN_ELEMENT, function (err, data) {
                console.info('DeviceKvStoreDeleteBoolCallbackSucTest put success');
                expect(err == undefined).assertTrue();
                kvStore.delete(KEY_TEST_BOOLEAN_ELEMENT, function (err, data) {
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
     * @tc.name DeviceKvStoreDeletePredicatesCallbackTest
     * @tc.desc Test Js Api DeviceKvStore.Delete()
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreDeletePredicatesCallbackTest', 0, function (done) {
        console.log('DeviceKvStoreDeletePredicatesCallbackTest');
        try {
            let predicates = new dataShare.DataSharePredicates();
            let arr = ["name"];
            predicates.inKeys(arr);
            kvStore.delete(predicates, function (err, data) {
                if (err) {
                    console.error('DeviceKvStoreDeletePredicatesCallbackTest delete fail' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(null).assertFail();
                    done();
                }
                console.error('DeviceKvStoreDeletePredicatesCallbackTest delete success');
                expect(null).assertFail();
                done();
            });
        } catch (e) {
            console.info('DeviceKvStoreDeletePredicatesCallbackTest fail' + `, error code is ${e.code}, message is ${e.message}`);
            expect(e.code == 202).assertTrue();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStoreOnChangeCallbackType0Test
     * @tc.desc Test Js Api DeviceKvStore.OnChange() with type 0
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreOnChangeCallbackTest', 0, function (done) {
        console.info('DeviceKvStoreOnChangeCallbackTest');
        try {
            kvStore.on('dataChange', 0, function (data) {
                console.info('DeviceKvStoreOnChangeCallbackTest dataChange');
                expect(data != null).assertTrue();
            });
            kvStore.put(KEY_TEST_FLOAT_ELEMENT, VALUE_TEST_FLOAT_ELEMENT, function (err, data) {
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
    it('DeviceKvStoreOnChangeCallbackType1Test', 0, function (done) {
        console.info('DeviceKvStoreOnChangeCallbackType1Test');
        try {
            kvStore.on('dataChange', 1, function (data) {
                console.info('DeviceKvStoreOnChangeCallbackType1Test dataChange');
                expect(data != null).assertTrue();
            });
            kvStore.put(KEY_TEST_FLOAT_ELEMENT, VALUE_TEST_FLOAT_ELEMENT, function (err, data) {
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
    it('DeviceKvStoreOnChangeCallbackType2Test', 0, function (done) {
        console.info('DeviceKvStoreOnChangeCallbackType2Test');
        try {
            kvStore.on('dataChange', 2, function (data) {
                console.info('DeviceKvStoreOnChangeCallbackType2Test dataChange');
                expect(data != null).assertTrue();
            });
            kvStore.put(KEY_TEST_FLOAT_ELEMENT, VALUE_TEST_FLOAT_ELEMENT, function (err, data) {
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
    it('DeviceKvStoreOnChangeCallbackClosedKVStoreTest', 0, function (done) {
        console.info('DeviceKvStoreOnChangeCallbackClosedKVStoreTest');
        try {
            kvManager.closeKVStore(TEST_BUNDLE_NAME, TEST_STORE_ID, function (err) {
                expect(err == undefined).assertTrue();
                try {
                    kvStore.on('dataChange', 2, function () {
                        expect(null).assertFail();
                        done();
                    });
                } catch (e) {
                    console.error('DeviceKvStoreOnChangeCallbackClosedKVStoreTest onDataChange fail' + `, error code is ${e.code}, message is ${e.message}`);
                    expect(e.code == 15100005).assertTrue();
                    done();
                }
            });
        } catch (e) {
            console.error('DeviceKvStoreOnChangeCallbackClosedKVStoreTest closeKVStore' + `, error code is ${e.code}, message is ${e.message}`);
            expect(e.code == 401).assertFail();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStoreOnChangeCallbackPassMaxTest
     * @tc.desc Test Js Api DeviceKvStore.OnChange() pass max subscription time
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreOnChangeCallbackPassMaxTest', 0, function (done) {
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
                done();
            });
        } catch (e) {
            console.error('DeviceKvStoreOnChangeCallbackPassMaxTest e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(e.code == 15100001).assertTrue();
            done();
        }
    })

    /**
     * @tc.name DeviceKvStoreOnChangeCallbackInvalidArgsTest
     * @tc.desc Test Js Api DeviceKvStore.OnChange() with invalid args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('DeviceKvStoreOnChangeCallbackInvalidArgsTest', 0, function (done) {
        console.info('DeviceKvStoreOnChangeCallbackInvalidArgsTest');
        try {
            kvStore.on('dataChange', function () {
                console.info('DeviceKvStoreOnChangeCallbackInvalidArgsTest dataChange');
                expect(null).assertFail();
                done();
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
    it('DeviceKvStoreOffChangeCallbackSucTest', 0, function (done) {
        console.info('DeviceKvStoreOffChangePromiseSucTest');
        try {
            var func = function (data) {
                console.info('DeviceKvStoreOffChangeCallbackSucTest ' + JSON.stringify(data));
            };
            kvStore.on('dataChange', 0, func);
            kvStore.off('dataChange', func);
            done();
        } catch (e) {
            console.error('DeviceKvStoreOffChangeCallbackSucTest fail' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

})
