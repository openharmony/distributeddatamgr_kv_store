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
const TEST_STORE_ID = 'storeId';
var kvManager = null;
var kvStore = null;
const USED_DEVICE_IDS = ['A12C1F9261528B21F95778D2FDC0B2E33943E6251AC5487F4473D005758905DB'];
const UNUSED_DEVICE_IDS = [];  /* add you test device-ids here */

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

describe('SingleKvStoreCallbackTest', function () {
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
     * @tc.name SingleKvStorePutStringCallbackSucTest
     * @tc.desc Test Js Api SingleKvStore.Put(String) successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStorePutStringCallbackSucTest', 0, async function (done) {
        console.info('SingleKvStorePutStringCallbackSucTest');
        try {
            await kvStore.put(KEY_TEST_STRING_ELEMENT, VALUE_TEST_STRING_ELEMENT, function (err, data) {
                if (err == undefined) {
                    console.info('SingleKvStorePutStringCallbackSucTest put success');
                } else {
                    console.error('SingleKvStorePutStringCallbackSucTest put fail' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(null).assertFail();
                }
                done();
            });
        } catch (e) {
            console.error('SingleKvStorePutStringCallbackSucTest put e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name SingleKvStorePutStringCallbackInvalidArgsTest
     * @tc.desc Test Js Api SingleKvStore.Put(String) with invalid args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStorePutStringCallbackInvalidArgsTest', 0, async function (done) {
        console.info('SingleKvStorePutStringCallbackInvalidArgsTest');
        try {
            await kvStore.put(KEY_TEST_STRING_ELEMENT, function (err, data) {
                if (err == undefined) {
                    expect(null).assertFail();
                    console.info('SingleKvStorePutStringCallbackInvalidArgsTest put success');
                } else {
                    console.error('SingleKvStorePutStringCallbackInvalidArgsTest put fail' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(null).assertFail();
                }
            });
        } catch (e) {
            console.error('SingleKvStorePutStringCallbackInvalidArgsTest put e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(e.code == 401).assertTrue();
        }
        done();
    })

    /**
     * @tc.name SingleKvStorePutStringCallbackClosedKvStoreTest
     * @tc.desc Test Js Api SingleKvStore.Put(String) with closed database
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStorePutStringCallbackClosedKvStoreTest', 0, async function (done) {
        console.info('SingleKvStorePutStringCallbackClosedKvStoreTest');
        try {
            await kvManager.closeKVStore(TEST_BUNDLE_NAME, TEST_STORE_ID, function (err) {
                expect(err == undefined).assertTrue();
            });
            await kvStore.put(KEY_TEST_STRING_ELEMENT, VALUE_TEST_STRING_ELEMENT, function (err) {
                if (err == undefined) {
                    expect(null).assertFail();
                    console.info('SingleKvStorePutStringCallbackClosedKvStoreTest put success');
                } else {
                    console.error('SingleKvStorePutStringCallbackClosedKvStoreTest put fail' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(err.code == 15100005).assertTrue();
                }
            });
        } catch (e) {
            console.error('SingleKvStorePutStringCallbackClosedKvStoreTest put e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreGetStringCallbackSucTest
     * @tc.desc Test Js Api SingleKvStore.GetString() successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreGetStringCallbackSucTest', 0, async function (done) {
        console.info('SingleKvStoreGetStringCallbackSucTest');
        try {
            await kvStore.put(KEY_TEST_STRING_ELEMENT, VALUE_TEST_STRING_ELEMENT, async function (err) {
                console.info('SingleKvStoreGetStringCallbackSucTest put success');
                expect(err == undefined).assertTrue();
                await kvStore.get(KEY_TEST_STRING_ELEMENT, function (err, data) {
                    console.info('SingleKvStoreGetStringCallbackSucTest get success');
                    expect((err == undefined) && (VALUE_TEST_STRING_ELEMENT == data)).assertTrue();
                    done();
                });
            })
        } catch (e) {
            console.error('SingleKvStoreGetStringCallbackSucTest get e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name SingleKvStoreGetStringCallbackClosedKVStoreTest
     * @tc.desc Test Js Api SingleKvStore.GetString() from a closed kvstore
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreGetStringCallbackClosedKVStoreTest', 0, async function (done) {
        try {
            await kvStore.put(KEY_TEST_STRING_ELEMENT, VALUE_TEST_STRING_ELEMENT, async function (err) {
                expect(err == undefined).assertTrue();
            })
            await kvManager.closeKVStore(TEST_BUNDLE_NAME, TEST_STORE_ID, function (err) {
                expect(err == undefined).assertTrue();
            });
            await kvStore.get(KEY_TEST_STRING_ELEMENT, function (err) {
                if (err == undefined) {
                    expect(null).assertFail();
                } else {
                    expect(err.code == 15100005).assertTrue();
                }
            });
        } catch (e) {
            console.error('SingleKvStoreGetStringCallbackClosedKVStoreTest get e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreGetStringCallbackInvalidArgsTest
     * @tc.desc Test Js Api SingleKvStore.GetString() with invalid args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreGetStringCallbackInvalidArgsTest', 0, async function (done) {
        try {
            await kvStore.put(KEY_TEST_STRING_ELEMENT, VALUE_TEST_STRING_ELEMENT, async function (err) {
                console.info('SingleKvStoreGetStringCallbackInvalidArgsTest put success');
                expect(err == undefined).assertTrue();
                await kvStore.get(1, function (err, data) {
                    if (err == undefined) {
                        expect(null).assertFail();
                    } else {
                        expect(null).assertFail();
                    }
                });
            })
        } catch (e) {
            console.error('SingleKvStoreGetStringCallbackInvalidArgsTest get e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(e.code == 401).assertTrue();
        }
        done();
    })

    /**
     * @tc.name SingleKvStorePutIntCallbackSucTest
     * @tc.desc Test Js Api SingleKvStore.Put(Int) successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStorePutIntCallbackSucTest', 0, async function (done) {
        console.info('SingleKvStorePutIntCallbackSucTest');
        try {
            await kvStore.put(KEY_TEST_INT_ELEMENT, VALUE_TEST_INT_ELEMENT, async function (err) {
                console.info('SingleKvStorePutIntCallbackSucTest put success');
                expect(err == undefined).assertTrue();
                await kvStore.get(KEY_TEST_INT_ELEMENT, function (err, data) {
                    console.info('SingleKvStorePutIntCallbackSucTest get success');
                    expect((err == undefined) && (VALUE_TEST_INT_ELEMENT == data)).assertTrue();
                    done();
                })
            });
        } catch (e) {
            console.error('SingleKvStorePutIntCallbackSucTest put e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name SingleKvStorePutIntCallbackMaxTest
     * @tc.desc Test Js Api SingleKvStore.Put(Int) with max value
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStorePutIntCallbackMaxTest', 0, async function (done) {
        console.info('SingleKvStorePutIntCallbackMaxTest');
        try {
            var intValue = Number.MIN_VALUE;
            await kvStore.put(KEY_TEST_INT_ELEMENT, intValue, async function (err) {
                console.info('SingleKvStorePutIntCallbackMaxTest put success');
                expect(err == undefined).assertTrue();
                await kvStore.get(KEY_TEST_INT_ELEMENT, function (err, data) {
                    console.info('SingleKvStorePutIntCallbackMaxTest get success');
                    expect((err == undefined) && (intValue == data)).assertTrue();
                    done();
                })
            });
        } catch (e) {
            console.error('SingleKvStorePutIntCallbackMaxTest put e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name SingleKvStorePutIntCallbackMinTest
     * @tc.desc Test Js Api SingleKvStore.Put(Int) with min value
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStorePutIntCallbackMinTest', 0, async function (done) {
        console.info('SingleKvStorePutIntCallbackMinTest');
        try {
            var intValue = Number.MAX_VALUE;
            await kvStore.put(KEY_TEST_INT_ELEMENT, intValue, async function (err) {
                console.info('SingleKvStorePutIntCallbackMinTest put success');
                expect(err == undefined).assertTrue();
                await kvStore.get(KEY_TEST_INT_ELEMENT, function (err, data) {
                    console.info('SingleKvStorePutIntCallbackMinTest get success');
                    expect((err == undefined) && (intValue == data)).assertTrue();
                    done();
                })
            });
        } catch (e) {
            console.error('SingleKvStorePutIntCallbackMinTest put e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name SingleKvStoreGetIntCallbackNonExistTest
     * @tc.desc Test Js Api SingleKvStore.GetInt() get non-exsiting int
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreGetIntCallbackTest', 0, async function (done) {
        console.info('SingleKvStoreGetIntCallbackTest');
        try {
            await kvStore.get(KEY_TEST_INT_ELEMENT, function (err) {
                if (err == undefined) {
                    console.info('SingleKvStoreGetIntCallbackTest get success');
                    expect(null).assertFail();
                } else {
                    console.info('SingleKvStoreGetIntCallbackTest get fail');
                    expect(err.code == 15100004).assertTrue();
                }
                done();
            })
        } catch (e) {
            console.error('SingleKvStoreGetIntCallbackTest put e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name SingleKvStorePutBoolCallbackTest
     * @tc.desc Test Js Api SingleKvStore.Put(Bool) successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStorePutBoolCallbackTest', 0, async function (done) {
        console.info('SingleKvStorePutBoolCallbackTest');
        try {
            await kvStore.put(KEY_TEST_BOOLEAN_ELEMENT, VALUE_TEST_BOOLEAN_ELEMENT, function (err) {
                console.info('SingleKvStorePutBoolCallbackTest put success');
                expect(err == undefined).assertTrue();
                done();
            });
        } catch (e) {
            console.error('SingleKvStorePutBoolCallbackTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name SingleKvStoreGetBoolCallbackSucTest
     * @tc.desc Test Js Api SingleKvStore.GetBool() successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreGetBoolCallbackSucTest', 0, async function (done) {
        console.info('SingleKvStoreGetBoolCallbackSucTest');
        try {
            await kvStore.put(KEY_TEST_BOOLEAN_ELEMENT, VALUE_TEST_BOOLEAN_ELEMENT, async function (err, data) {
                console.info('SingleKvStoreGetBoolCallbackSucTest put success');
                expect(err == undefined).assertTrue();
                await kvStore.get(KEY_TEST_BOOLEAN_ELEMENT, function (err, data) {
                    console.info('SingleKvStoreGetBoolCallbackSucTest get success');
                    expect((err == undefined) && (VALUE_TEST_BOOLEAN_ELEMENT == data)).assertTrue();
                    done();
                });
            })
        } catch (e) {
            console.error('SingleKvStoreGetBoolCallbackSucTest e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name SingleKvStorePutFloatCallbackSucTest
     * @tc.desc Test Js Api SingleKvStore.Put(Float) successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStorePutFloatCallbackSucTest', 0, async function (done) {
        console.info('SingleKvStorePutFloatCallbackSucTest');
        try {
            await kvStore.put(KEY_TEST_FLOAT_ELEMENT, VALUE_TEST_FLOAT_ELEMENT, function (err, data) {
                console.info('SingleKvStorePutFloatCallbackSucTest put success');
                expect(err == undefined).assertTrue();
                done();
            });
        } catch (e) {
            console.error('SingleKvStorePutFloatCallbackSucTest e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name SingleKvStoreGetFloatCallbackSucTest
     * @tc.desc Test Js Api SingleKvStore.GetFloat() successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreGetFloatCallbackSucTest', 0, async function (done) {
        console.info('SingleKvStoreGetFloatCallbackSucTest');
        try {
            await kvStore.put(KEY_TEST_FLOAT_ELEMENT, VALUE_TEST_FLOAT_ELEMENT, async function (err, data) {
                console.info('SingleKvStoreGetFloatCallbackSucTest put success');
                expect(err == undefined).assertTrue();
                await kvStore.get(KEY_TEST_FLOAT_ELEMENT, function (err, data) {
                    if (err == undefined) {
                        console.info('SingleKvStoreGetFloatCallbackSucTest get success');
                        expect(true).assertTrue();
                    } else {
                        console.error('SingleKvStoreGetFloatCallbackSucTest get fail' + `, error code is ${err.code}, message is ${err.message}`);
                        expect(null).assertFail();
                    }
                });
                done();
            });

        } catch (e) {
            console.error('SingleKvStoreGetFloatCallbackSucTest e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name SingleKvStoreDeleteStringCallbackSucTest
     * @tc.desc Test Js Api SingleKvStore.DeleteString() successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreDeleteStringCallbackSucTest', 0, async function (done) {
        console.info('SingleKvStoreDeleteStringCallbackSucTest');
        try {
            await kvStore.put(KEY_TEST_STRING_ELEMENT, VALUE_TEST_STRING_ELEMENT, async function (err, data) {
                console.info('SingleKvStoreDeleteStringCallbackSucTest put success');
                expect(err == undefined).assertTrue();
                await kvStore.delete(KEY_TEST_STRING_ELEMENT, function (err, data) {
                    console.info('SingleKvStoreDeleteStringCallbackSucTest delete success');
                    expect(err == undefined).assertTrue();
                    done();
                });
            })
        } catch (e) {
            console.error('SingleKvStoreDeleteStringCallbackSucTest e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name SingleKvStoreDeleteStringCallbackInvalid ArgsTest
     * @tc.desc Test Js Api SingleKvStore.DeleteString() with invalid args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreDeleteStringCallbackInvalidArgsTest', 0, async function (done) {
        console.info('SingleKvStoreDeleteStringCallbackInvalidArgsTest');
        try {
            await kvStore.put(KEY_TEST_STRING_ELEMENT, VALUE_TEST_STRING_ELEMENT, async function (err, data) {
                console.info('SingleKvStoreDeleteStringCallbackInvalidArgsTest put success');
                expect(err == undefined).assertTrue();
                await kvStore.delete(function (err) {
                    console.info('SingleKvStoreDeleteStringCallbackInvalidArgsTest delete success');
                    expect(null).assertFail();
                });
            })
        } catch (e) {
            console.error('SingleKvStoreDeleteStringCallbackInvalidArgsTest e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(e.code == 401).assertTrue();
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreDeleteCallbackClosedKVStoreTest
     * @tc.desc Test Js Api SingleKvStore.Delete() into a closed kvstore
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreDeleteCallbackClosedKVStoreTest', 0, async function (done) {
        try {
            await kvStore.put(KEY_TEST_STRING_ELEMENT, VALUE_TEST_STRING_ELEMENT, function (err) {
                if (err == undefined) {
                    expect(true).assertTrue();
                    console.info('SingleKvStorePutStringCallbackClosedKvStoreTest put success');
                } else {
                    console.error('SingleKvStorePutStringCallbackClosedKvStoreTest put fail' + `, error code is ${err.code}, message is ${err.message}`);
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
            console.error('SingleKvStorePutStringCallbackClosedKvStoreTest put e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreDeleteIntCallbackSucTest
     * @tc.desc Test Js Api SingleKvStore.DeleteInt() successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreDeleteIntCallbackSucTest', 0, async function (done) {
        console.info('SingleKvStoreDeleteIntCallbackSucTest');
        try {
            await kvStore.put(KEY_TEST_INT_ELEMENT, VALUE_TEST_INT_ELEMENT, async function (err, data) {
                console.info('SingleKvStoreDeleteIntCallbackSucTest put success');
                expect(err == undefined).assertTrue();
                await kvStore.delete(KEY_TEST_INT_ELEMENT, function (err, data) {
                    console.info('SingleKvStoreDeleteIntCallbackSucTest delete success');
                    expect(err == undefined).assertTrue();
                    done();
                });
            })
        } catch (e) {
            console.error('SingleKvStoreDeleteIntCallbackSucTest e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name SingleKvStoreDeleteFloatCallbackSucTest
     * @tc.desc Test Js Api SingleKvStore.DeleteFloat() successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreDeleteFloatCallbackSucTest', 0, async function (done) {
        console.info('SingleKvStoreDeleteFloatCallbackSucTest');
        try {
            await kvStore.put(KEY_TEST_FLOAT_ELEMENT, VALUE_TEST_FLOAT_ELEMENT, async function (err, data) {
                console.info('SingleKvStoreDeleteFloatCallbackSucTest put success');
                expect(err == undefined).assertTrue();
                await kvStore.delete(KEY_TEST_FLOAT_ELEMENT, function (err, data) {
                    console.info('SingleKvStoreDeleteFloatCallbackSucTest delete success');
                    expect(err == undefined).assertTrue();
                    done();
                });
            })
        } catch (e) {
            console.error('SingleKvStoreDeleteFloatCallbackSucTest e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name SingleKvStoreDeleteBoolCallbackSucTest
     * @tc.desc Test Js Api SingleKvStore.DeleteBool() successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreDeleteBoolCallbackSucTest', 0, async function (done) {
        console.info('SingleKvStoreDeleteBoolCallbackSucTest');
        try {
            await kvStore.put(KEY_TEST_BOOLEAN_ELEMENT, VALUE_TEST_BOOLEAN_ELEMENT, async function (err, data) {
                console.info('SingleKvStoreDeleteBoolCallbackSucTest put success');
                expect(err == undefined).assertTrue();
                await kvStore.delete(KEY_TEST_BOOLEAN_ELEMENT, function (err, data) {
                    console.info('SingleKvStoreDeleteBoolCallbackSucTest delete success');
                    expect(err == undefined).assertTrue();
                    done();
                });
            })
        } catch (e) {
            console.error('SingleKvStoreDeleteBoolCallbackSucTest e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name SingleKvStoreDeletePredicatesCallbackSucTest
     * @tc.desc Test Js Api SingleKvStore.Delete() successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreDeletePredicatesCallbackSucTest', 0, async function (done) {
        console.log('SingleKvStoreDeletePredicatesCallbackSucTest');
        try {
            let predicates = new dataShare.DataSharePredicates();
            let arr = ["name"];
            predicates.inKeys(arr);
            await kvStore.put("name", "Bob", async function (err, data) {
                console.log('SingleKvStoreDeletePredicatesCallbackSucTest put success');
                expect(err == undefined).assertTrue();
                await kvStore.delete(predicates, function (err, data) {
                    console.log('SingleKvStoreDeletePredicatesCallbackSucTest delete success');
                    expect(err == undefined).assertTrue();
                    done();
                });
            })
        } catch (e) {
            console.error('SingleKvStoreDeletePredicatesCallbackSucTest e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name SingleKvStoreBackupCallbackSucTest
     * @tc.desc Test Js Api SingleKvStore.backup() successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreBackupCallbackSucTest ', 0, async function (done) {
        try {
            await kvStore.backup(file, function (error) {
                expect(error == undefined).assertTrue();
            });
        } catch (e) {
            expect(null).assertFail();
        }
        done();
    });
    /**
     * @tc.name SingleKvStoreBackupCallbackInvalidArgsTest
     * @tc.desc Test Js Api SingleKvStore.backup() with invalid args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreBackupCallbackInvalidArgsTest ', 0, async function (done) {
        try {
            kvStore.backup(function () {
                expect(null).assertFail();
            })
        } catch (e) {
            expect(e.code == 401).assertTrue();
        }
        done();
    });

    /**
     * @tc.name SingleKvStoreBackupCallbackClosedKVStoreTest
     * @tc.desc Test Js Api SingleKvStore.backup() with closed kvstore
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreBackupCallbackClosedKVStoreTest ', 0, async function (done) {
        try {
            await kvManager.closeKVStore(TEST_BUNDLE_NAME, TEST_STORE_ID, async function (err) {
                expect(err == undefined).assertTrue();
                await kvManager.deleteKVStore(TEST_BUNDLE_NAME, TEST_STORE_ID, async function (error) {
                    expect(error == undefined).assertTrue();
                });
            });
            await kvStore.backup(file, function (ee) {
                expect(ee.code == 15100005).assertTrue();
            });
        } catch (e) {
            expect(null).assertFail();
        }
        done();
    });
    /**
     * @tc.name SingleKvStoreRestoreCallbackSucTest
     * @tc.desc Test Js Api SingleKvStore.restore() successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreRestoreCallbackSucTest ', 0, async function (done) {
        try {

            await kvStore.restore(file, function (error) {
                expect(error == undefined).assertTrue();
            });
        } catch (e) {
            expect(null).assertFail();
        }
        done();
    });
    /**
     * @tc.name SingleKvStoreRestoreCallbackInvalidArgsTest
     * @tc.desc Test Js Api SingleKvStore.restore() with invalid args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreRestoreCallbackInvalidArgsTest ', 0, async function (done) {
        try {
            kvStore.restore(function () {
                expect(null).assertFail();
            })
        } catch (e) {
            expect(e.code == 401).assertTrue();
        }
        done();
    });

    /**
     * @tc.name SingleKvStoreRestoreCallbackClosedKVStoreTest
     * @tc.desc Test Js Api SingleKvStore.restore() with closed kvstore
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreRestoreCallbackClosedKVStoreTest ', 0, async function (done) {
        try {
            await kvManager.closeKVStore(TEST_BUNDLE_NAME, TEST_STORE_ID, async function (err) {
                expect(err == undefined).assertTrue();
                await kvManager.deleteKVStore(TEST_BUNDLE_NAME, TEST_STORE_ID, async function (error) {
                    expect(error == undefined).assertTrue();
                });
            });
            await kvStore.restore(file, function (error) {
                expect(error.code == 15100005).assertTrue();
            });
        } catch (e) {
            expect(null).assertFail();
        }
        done();
    });

    /**
     * @tc.name SingleKvStoreDeleteBackupCallbackSucTest
     * @tc.desc Test Js Api SingleKvStore.DeleteBackup() successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreDeleteBackupCallbackSucTest ', 0, async function (done) {
        try {
            await kvStore.deleteBackup(files, function (error) {
                expect(error == undefined).assertTrue();
            });
        } catch (e) {
            expect(null).assertFail();
        }
        done();
    });

    /**
     * @tc.name SingleKvStoreDeleteBackupCallbackInvalidArgsTest
     * @tc.desc Test Js Api SingleKvStore.DeleteBackup() with invalid args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreDeleteBackupCallbackInvalidArgsTest ', 0, async function (done) {
        try {
            kvStore.deleteBackup(function () {
                expect(null).assertFail();
            });
        } catch (e) {
            expect(e.code == 401).assertTrue();
        }
        done();
    });

    /**
     * @tc.name SingleKvStoreOnChangeCallbackType0Test
     * @tc.desc Test Js Api SingleKvStore.OnChange() with type 0
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreOnChangeCallbackTest', 0, async function (done) {
        console.info('SingleKvStoreOnChangeCallbackTest');
        try {
            kvStore.on('dataChange', 0, function (data) {
                console.info('SingleKvStoreOnChangeCallbackTest dataChange');
                expect(data != null).assertTrue();
            });
            await kvStore.put(KEY_TEST_FLOAT_ELEMENT, VALUE_TEST_FLOAT_ELEMENT, function (err, data) {
                console.info('SingleKvStoreOnChangeCallbackTest put success');
                expect(err == undefined).assertTrue();
                done();
            });
        } catch (e) {
            console.error('SingleKvStoreOnChangeCallbackTest e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name SingleKvStoreOnChangeCallbackType1Test
     * @tc.desc Test Js Api SingleKvStore.OnChange() with type 1
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreOnChangeCallbackType1Test', 0, async function (done) {
        console.info('SingleKvStoreOnChangeCallbackType1Test');
        try {
            kvStore.on('dataChange', 1, function (data) {
                console.info('SingleKvStoreOnChangeCallbackType1Test dataChange');
                expect(data != null).assertTrue();
            });
            await kvStore.put(KEY_TEST_FLOAT_ELEMENT, VALUE_TEST_FLOAT_ELEMENT, function (err, data) {
                console.info('SingleKvStoreOnChangeCallbackType1Test put success');
                expect(err == undefined).assertTrue();
                done();
            });
        } catch (e) {
            console.error('SingleKvStoreOnChangeCallbackType1Test e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name SingleKvStoreOnChangeCallbackType2Test
     * @tc.desc Test Js Api SingleKvStore.OnChange() with type 2
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreOnChangeCallbackType2Test', 0, async function (done) {
        console.info('SingleKvStoreOnChangeCallbackType2Test');
        try {
            kvStore.on('dataChange', 2, function (data) {
                console.info('SingleKvStoreOnChangeCallbackType2Test dataChange');
                expect(data != null).assertTrue();
            });
            await kvStore.put(KEY_TEST_FLOAT_ELEMENT, VALUE_TEST_FLOAT_ELEMENT, function (err, data) {
                console.info('SingleKvStoreOnChangeCallbackType2Test put success');
                expect(err == undefined).assertTrue();
                done();
            });
        } catch (e) {
            console.error('SingleKvStoreOnChangeCallbackType2Test e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name SingleKvStoreOnChangeCallbackClosedKVStoreTest
     * @tc.desc Test Js Api SingleKvStore.OnChange() subscribe a closed kvstore
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreOnChangeCallbackClosedKVStoreTest', 0, async function (done) {
        console.info('SingleKvStoreOnChangeCallbackClosedKVStoreTest');
        try {
            await kvManager.closeKVStore(TEST_BUNDLE_NAME, TEST_STORE_ID, function (err) {
                expect(err == undefined).assertTrue();
                kvStore.on('dataChange', 2, function () {
                    expect(null).assertFail();
                });
            });
        } catch (e) {
            console.error('SingleKvStoreOnChangeCallbackClosedKVStoreTest e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(e.code == 15100005).assertTrue();
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreOnChangeCallbackPassMaxTest
     * @tc.desc Test Js Api SingleKvStore.OnChange() pass max subscription time
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreOnChangeCallbackPassMaxTest', 0, async function (done) {
        console.info('SingleKvStoreOnChangeCallbackPassMaxTest');
        try {
            for (let i = 0; i < 8; i++) {
                kvStore.on('dataChange', 0, function (data) {
                    console.info('SingleKvStoreOnChangeCallbackPassMaxTest dataChange');
                    expect(data != null).assertTrue();
                });
            }
            kvStore.on('dataChange', 0, function (err) {
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('SingleKvStoreOnChangeCallbackPassMaxTest e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(e.code == 15100001).assertTrue();
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreOnChangeCallbackInvalidArgsTest
     * @tc.desc Test Js Api SingleKvStore.OnChange() with invalid args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreOnChangeCallbackInvalidArgsTest', 0, async function (done) {
        console.info('SingleKvStoreOnChangeCallbackInvalidArgsTest');
        try {
            kvStore.on('dataChange', function () {
                console.info('SingleKvStoreOnChangeCallbackInvalidArgsTest dataChange');
                expect(null).assertFail();
            });

        } catch (e) {
            console.error('SingleKvStoreOnChangeCallbackInvalidArgsTest e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(e.code == 401).assertTrue();
            done();
        }
    })

    /**
     * @tc.name SingleKvStoreOffChangeCallbackSucTest
     * @tc.desc Test Js Api SingleKvStoreOffChange success
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreOffChangeCallbackSucTest', 0, async function (done) {
        console.info('SingleKvStoreOffChangePromiseSucTest');
        try {
            var func = function (data) {
                console.info('SingleKvStoreOffChangeCallbackSucTest ' + JSON.stringify(data));
            };
            kvStore.on('dataChange', 0, func);
            kvStore.off('dataChange', func);
        } catch (e) {
            console.error('SingleKvStoreOffChangeCallbackSucTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreOffChangeCallbackInvalidArgsTest
     * @tc.desc Test Js Api SingleKvStoreOffChange with invalid args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreOffChangeCallbackInvalidArgsTest', 0, async function (done) {
        console.info('SingleKvStoreOffChangeCallbackInvalidArgsTest');
        try {
            kvStore.on('dataChange', 0, function (data) {
                console.info('SingleKvStoreOffChangeCallbackInvalidArgsTest ' + JSON.stringify(data));
            });
            kvStore.off('dataChange', 1, function (err) {
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('SingleKvStoreOffChangeCallbackSucTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(e.code == 401).assertTrue();
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreOffSyncCompleteCallbackSucTest
     * @tc.desc Test Js Api SingleKvStoreOffSyncComplete success
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreOffSyncCompleteCallbackSucTest', 0, async function (done) {
        console.info('SingleKvStoreOffSyncCompleteCallbackSucTest');
        try {
            var func = function (data) {
                console.info('SingleKvStoreOffSyncCompleteCallbackSucTest 0' + data)
            };
            kvStore.off('syncComplete', func);
            expect(true).assertTrue();
        } catch (e) {
            console.error('SingleKvStoreOffSyncCompleteCallbackSucTest put e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreOffSyncCompleteCallbackInvalidArgsTest
     * @tc.desc Test Js Api SingleKvStoreOffSyncComplete with invalid args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreOffSyncCompleteCallbackInvalidArgsTest', 0, async function (done) {
        console.info('SingleKvStoreOffSyncCompleteCallbackInvalidArgsTest');
        try {
            kvStore.off(function (err) {
                if (err = undefined) {
                    expect(null).assertFail();
                } else {
                    expect(null).assertFail();
                }
            });
        } catch (e) {
            console.error('SingleKvStoreOffSyncCompleteCallbackInvalidArgsTest put e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(e.code == 401).assertTrue();
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreSetSyncRangeCallbackDisjointTest
     * @tc.desc Test Js Api SingleKvStore.SetSyncRange() with disjoint ranges
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreSetSyncRangeCallbackDisjointTest', 0, async function (done) {
        console.info('SingleKvStoreSetSyncRangeCallbackDisjointTest');
        try {
            var localLabels = ['A', 'B'];
            var remoteSupportLabels = ['C', 'D'];
            await kvStore.setSyncRange(localLabels, remoteSupportLabels, function (err, data) {
                console.info('SingleKvStoreSetSyncRangeCallbackDisjointTest put success');
                expect(err == undefined).assertTrue();
                done();
            });
        } catch (e) {
            console.error('SingleKvStoreSetSyncRangeCallbackDisjointTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name SingleKvStoreSetSyncRangeCallbackJointTest
     * @tc.desc Test Js Api SingleKvStore.SetSyncRange() with joint range
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreSetSyncRangeCallbackTest', 0, async function (done) {
        console.info('SingleKvStoreSetSyncRangeCallbackTest');
        try {
            var localLabels = ['A', 'B'];
            var remoteSupportLabels = ['B', 'C'];
            await kvStore.setSyncRange(localLabels, remoteSupportLabels, function (err, data) {
                console.info('SingleKvStoreSetSyncRangeCallbackTest put success');
                expect(err == undefined).assertTrue();
                done();
            });
        } catch (e) {
            console.error('SingleKvStoreSetSyncRangeCallbackTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name SingleKvStoreSetSyncRangeCallbackSameTest
     * @tc.desc Test Js Api SingleKvStore.SetSyncRange() with same range
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it(' SingleKvStoreSetSyncRangeCallbackSameTest', 0, async function (done) {
        console.info(' SingleKvStoreSetSyncRangeCallbackSameTest');
        try {
            var localLabels = ['A', 'B'];
            var remoteSupportLabels = ['A', 'B'];
            await kvStore.setSyncRange(localLabels, remoteSupportLabels, function (err, data) {
                console.info(' SingleKvStoreSetSyncRangeCallbackSameTest put success');
                expect(err == undefined).assertTrue();
                done();
            });
        } catch (e) {
            console.error(' SingleKvStoreSetSyncRangeCallbackSameTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name SingleKvStoreSetSyncRangeCallbackInvalidArgsTest
     * @tc.desc Test Js Api SingleKvStore.SetSyncRange() with invalid args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it(' SingleKvStoreSetSyncRangeCallbackSameTest', 0, async function (done) {
        console.info(' SingleKvStoreSetSyncRangeCallbackSameTest');
        try {
            var remoteSupportLabels = ['A', 'B'];
            await kvStore.setSyncRange(remoteSupportLabels, function (err) {
                expect(null).assertFail();
            });
        } catch (e) {
            console.error(' SingleKvStoreSetSyncRangeCallbackSameTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(e.code == 401).assertTrue();
        }
        done();
    })

    /**
     * @tc.name SingleKvStorePutBatchEntryCallbackStringTest
     * @tc.desc Test Js Api SingleKvStore.PutBatch() with string value
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStorePutBatchEntryCallbackStringTest', 0, async function (done) {
        console.info('SingleKvStorePutBatchEntryCallbackStringTest');
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
            console.info('SingleKvStorePutBatchEntryCallbackStringTest entries: ' + JSON.stringify(entries));
            await kvStore.putBatch(entries, async function (err, data) {
                console.info('SingleKvStorePutBatchEntryCallbackStringTest putBatch success');
                expect(err == undefined).assertTrue();
                kvStore.getEntries('batch_test_string_key', function (err, entrys) {
                    expect(entrys.length == 10).assertTrue();
                    expect(entrys[0].value.value == 'batch_test_string_value').assertTrue();
                    done();
                });
            });
        } catch (e) {
            console.error('SingleKvStorePutBatchEntryCallbackStringTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name SingleKvStorePutBatchEntryCallbackIntegerTest
     * @tc.desc Test Js Api SingleKvStore.PutBatch() with integer value
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStorePutBatchEntryCallbackIntegerTest', 0, async function (done) {
        console.info('SingleKvStorePutBatchEntryCallbackIntegerTest');
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
            console.info('SingleKvStorePutBatchEntryCallbackIntegerTest entries: ' + JSON.stringify(entries));
            await kvStore.putBatch(entries, async function (err, data) {
                console.info('SingleKvStorePutBatchEntryCallbackIntegerTest putBatch success');
                expect(err == undefined).assertTrue();
                kvStore.getEntries('batch_test_number_key', function (err, entrys) {
                    expect(entrys.length == 10).assertTrue();
                    expect(entrys[0].value.value == 222).assertTrue();
                    done();
                });
            });
        } catch (e) {
            console.error('SingleKvStorePutBatchEntryCallbackIntegerTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name SingleKvStorePutBatchEntryCallbackFloatTest
     * @tc.desc Test Js Api SingleKvStore.PutBatch() with float value
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStorePutBatchEntryCallbackFloatTest', 0, async function (done) {
        console.info('SingleKvStorePutBatchEntryCallbackFloatTest');
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
            console.info('SingleKvStorePutBatchEntryCallbackFloatTest entries: ' + JSON.stringify(entries));
            await kvStore.putBatch(entries, async function (err, data) {
                console.info('SingleKvStorePutBatchEntryCallbackFloatTest putBatch success');
                expect(err == undefined).assertTrue();
                kvStore.getEntries('batch_test_number_key', function (err, entrys) {
                    expect(entrys.length == 10).assertTrue();
                    expect(entrys[0].value.value == 2.0).assertTrue();
                    done();
                });
            });
        } catch (e) {
            console.error('SingleKvStorePutBatchEntryCallbackFloatTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name SingleKvStorePutBatchEntryCallbackDoubleTest
     * @tc.desc Test Js Api SingleKvStore.PutBatch() with double value
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStorePutBatchEntryCallbackDoubleTest', 0, async function (done) {
        console.info('SingleKvStorePutBatchEntryCallbackDoubleTest');
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
            console.info('SingleKvStorePutBatchEntryCallbackDoubleTest entries: ' + JSON.stringify(entries));
            await kvStore.putBatch(entries, async function (err, data) {
                console.info('SingleKvStorePutBatchEntryCallbackDoubleTest putBatch success');
                expect(err == undefined).assertTrue();
                kvStore.getEntries('batch_test_number_key', function (err, entrys) {
                    expect(entrys.length == 10).assertTrue();
                    expect(entrys[0].value.value == 2.00).assertTrue();
                    done();
                });
            });
        } catch (e) {
            console.error('SingleKvStorePutBatchEntryCallbackDoubleTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name SingleKvStorePutBatchEntryCallbackBooleanTest
     * @tc.desc Test Js Api SingleKvStore.PutBatch() with boolean value
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStorePutBatchEntryCallbackBooleanTest', 0, async function (done) {
        console.info('SingleKvStorePutBatchEntryCallbackBooleanTest');
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
            console.info('SingleKvStorePutBatchEntryCallbackBooleanTest entries: ' + JSON.stringify(entries));
            await kvStore.putBatch(entries, async function (err, data) {
                console.info('SingleKvStorePutBatchEntryCallbackBooleanTest putBatch success');
                expect(err == undefined).assertTrue();
                kvStore.getEntries('batch_test_bool_key', function (err, entrys) {
                    expect(entrys.length == 10).assertTrue();
                    expect(entrys[0].value.value == bo).assertTrue();
                    done();
                });
            });
        } catch (e) {
            console.error('SingleKvStorePutBatchEntryCallbackBooleanTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name SingleKvStorePutBatchEntryCallbackByteArrayTest
     * @tc.desc Test Js Api SingleKvStore.PutBatch() with byte_arrgy value
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStorePutBatchEntryCallbackByteArrayTest', 0, async function (done) {
        console.info('SingleKvStorePutBatchEntryCallbackByteArrayTest');
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
            console.info('SingleKvStorePutBatchEntryCallbackByteArrayTest entries: ' + JSON.stringify(entries));
            await kvStore.putBatch(entries, async function (err, data) {
                console.info('SingleKvStorePutBatchEntryCallbackByteArrayTest putBatch success');
                expect(err == undefined).assertTrue();
                kvStore.getEntries('batch_test_bool_key', function (err, entrys) {
                    expect(entrys.length == 10).assertTrue();
                    expect(entrys[0].value.value.toString() == arr.toString()).assertTrue();
                    done();
                });
            });
        } catch (e) {
            console.error('SingleKvStorePutBatchEntryCallbackByteArrayTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name SingleKvStorePutBatchValueCallbackUint8ArrayTest
     * @tc.desc Test Js Api SingleKvStore.PutBatch() with value unit8array
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStorePutBatchValueCallbackUint8ArrayTest', 0, async function (done) {
        console.info('SingleKvStorePutBatchValueCallbackUint8ArrayTest001');
        try {
            let values = [];
            let arr1 = new Uint8Array([4, 5, 6, 7]);
            let arr2 = new Uint8Array([4, 5, 6, 7, 8]);
            let vb1 = {key: "name_1", value: arr1};
            let vb2 = {key: "name_2", value: arr2};
            values.push(vb1);
            values.push(vb2);
            console.info('SingleKvStorePutBatchValueCallbackUint8ArrayTest001 values: ' + JSON.stringify(values));
            await kvStore.putBatch(values, async function (err, data) {
                console.info('SingleKvStorePutBatchValueCallbackUint8ArrayTest001 putBatch success');
                expect(err == undefined).assertTrue();
                var query = new factory.Query();
                query.prefixKey("name_");
                kvStore.getEntries(query, function (err, entrys) {
                    expect(entrys.length == 2).assertTrue();
                    done();
                });
            });
        } catch (e) {
            console.error('SingleKvStorePutBatchValueCallbackUint8ArrayTest001 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name SingleKvStorePutBatchCallbackInvalidArgsTest
     * @tc.desc Test Js Api SingleKvStore.PutBatch() put invalid args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStorePutBatchCallbackInvalidArgsTest', 0, async function (done) {
        try {
            await kvStore.putBatch(async function (err) {
                if (err == undefined) {
                    expect(null).assertFail();
                } else {
                    expect(null).assertFail();
                }
            });
        } catch (e) {
            console.error('SingleKvStorePutBatchCallbackInvalidArgsTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(e.code == 401).assertTrue();
        }
        done();
    })

    /**
     * @tc.name SingleKvStorePutBatchCallbackClosedKvstoreTest
     * @tc.desc Test Js Api SingleKvStore.PutBatch() put into closed kvstore
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStorePutBatchCallbackClosedKvstoreTest', 0, async function (done) {
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
            console.error('SingleKvStorePutBatchCallbackClosedKvstoreTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreDeleteBatchCallbackSucTest
     * @tc.desc Test Js Api SingleKvStore.DeleteBatch() successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreDeleteBatchCallbackSucTest', 0, async function (done) {
        console.info('SingleKvStoreDeleteBatchCallbackSucTest');
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
            console.info('SingleKvStoreDeleteBatchCallbackSucTest entries: ' + JSON.stringify(entries));
            await kvStore.putBatch(entries, async function (err, data) {
                console.info('SingleKvStoreDeleteBatchCallbackSucTest putBatch success');
                expect(err == undefined).assertTrue();
                await kvStore.deleteBatch(keys, async function (err, data) {
                    console.info('SingleKvStoreDeleteBatchCallbackSucTest deleteBatch success');
                    expect(err == undefined).assertTrue();
                    done();
                });
            });
        } catch (e) {
            console.error('SingleKvStoreDeleteBatchCallbackSucTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name SingleKvStoreDeleteBatchCallbackInvalidArgsTest
     * @tc.desc Test Js Api SingleKvStore.DeleteBatch() with invalid args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreDeleteBatchCallbackInvalidArgsTest', 0, async function (done) {
        console.info('SingleKvStoreDeleteBatchCallbackInvalidArgsTest');
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
            console.error('SingleKvStoreDeleteBatchCallbackInvalidArgsTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(e.code == 401).assertTrue();
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreDeleteBatchCallbackClosedKVStoreTest
     * @tc.desc Test Js Api SingleKvStore.DeleteBatch() with closed kvstore
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreDeleteBatchCallbackClosedKVStoreTest', 0, async function (done) {
        console.info('SingleKvStoreDeleteBatchCallbackClosedKVStoreTest');
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
            console.error('SingleKvStoreDeleteBatchCallbackClosedKVStoreTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreGetEntriesCallbackQueryTest
     * @tc.desc Test Js Api SingleKvStore.GetEntries() with query
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreGetEntriesCallbackQueryTest', 0, async function (done) {
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
                kvStore.getEntries(query, function (err, entrys) {
                    expect(entrys.length == 10).assertTrue();
                    expect(entrys[0].value.value.toString() == arr.toString()).assertTrue();
                    done();
                });
            });
        } catch (e) {
            console.error('SingleKvStoreGetEntriesCallbackQueryTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreGetEntriesCallbackQueryClosedKVStoreTest
     * @tc.desc Test Js Api SingleKvStore.GetEntries() query from a closed kvstore
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreGetEntriesCallbackQueryClosedKVStoreTest', 0, async function (done) {
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
                    kvStore.getEntries(query, function (err) {
                        if (err == undefined) {
                            expect(null).assertFail();
                        } else {
                            expect(err.code == 15100005).assertTrue();
                        }
                    });
                });
            });
        } catch (e) {
            console.error('SingleKvStoreGetEntriesCallbackQueryClosedKVStoreTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreGetEntriesCallbackSucTest
     * @tc.desc Test Js Api SingleKvStore.GetEntries() success
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreGetEntriesCallbackSucTest', 0, async function (done) {
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
                kvStore.getEntries("batch_test", function (err, entrys) {
                    expect(entrys.length == 10).assertTrue();
                    expect(entrys[0].value.value.toString() == arr.toString()).assertTrue();
                    done();
                });
            });
        } catch (e) {
            console.error('SingleKvStoreGetEntriesCallbackSucTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreGetEntriesCallbackClosedKVStoreTest
     * @tc.desc Test Js Api SingleKvStore.GetEntries() from a closed kvstore
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreGetEntriesCallbackClosedKVStoreTest', 0, async function (done) {
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
                    kvStore.getEntries("batch_test", function (err) {
                        if (err == undefined) {
                            expect(null).assertFail();
                        } else {
                            expect(err.code == 15100005).assertTrue();
                        }
                    });
                });
            });
        } catch (e) {
            console.error('SingleKvStoreGetEntriesCallbackClosedKVStoreTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreGetEntriesCallbackInvalidArgsTest
     * @tc.desc Test Js Api SingleKvStore.GetEntries() with invalid args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreGetEntriesCallbackInvalidArgsTest', 0, async function (done) {
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
                kvStore.getEntries("batch_test", function (err, entrys) {
                    if (err == undefined) {
                        expect(null).assertFail();
                    } else {
                        expect(null).assertFail();
                    }
                });
            });
        } catch (e) {
            console.error('SingleKvStoreGetEntriesCallbackInvalidArgsTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(e.code == 401).assertTrue();
        }
        done();
    })


    /**
     * @tc.name SingleKvstoreStartTransactionCallbackCommitTest
     * @tc.desc Test Js Api SingleKvStore.startTransaction() with commit
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvstoreStartTransactionCallbackCommitTest', 0, async function (done) {
        console.info('SingleKvstoreStartTransactionCallbackCommitTest');
        try {
            var count = 0;
            kvStore.on('dataChange', 0, function (data) {
                console.info('SingleKvstoreStartTransactionCallbackCommitTest 0' + data)
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
            console.error('SingleKvstoreStartTransactionCallbackCommitTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name SingleKvstoreStartTransactionCallbackRollbackTest
     * @tc.desc Test Js Api SingleKvStore.startTransaction() with rollback
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvstoreStartTransactionCallbackRollbackTest', 0, async function (done) {
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
            console.error('SingleKvstoreStartTransactionCallbackRollbackTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name SingleKvstoreStartTransactionCallbackClosedKVStoreTest
     * @tc.desc Test Js Api SingleKvStore.startTransaction() with closed kvstore
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvstoreStartTransactionCallbackClosedKVStoreTest', 0, async function (done) {
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
            console.error('SingleKvstoreStartTransactionCallbackClosedKVStoreTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name SingleKvStoreCommitCallbackClosedKVStoreTest
     * @tc.desc Test Js Api SingleKvStore.Commit() with closed kvstore
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreCommitCallbackClosedKVStoreTest', 0, async function (done) {
        console.info('SingleKvStoreCommitCallbackClosedKVStoreTest');
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
            console.error('SingleKvStoreCommitCallbackClosedKVStoreTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name SingleKvStoreRollbackCallbackClosedKVStoreTest
     * @tc.desc Test Js Api SingleKvStore.Rollback() with closed kvstore
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreRollbackCallbackClosedKVStoreTest', 0, async function (done) {
        console.info('SingleKvStoreRollbackCallbackClosedKVStoreTest');
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
            console.error('SingleKvStoreRollbackCallbackClosedKVStoreTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name SingleKvStoreEnableSyncCallbackTrueTest
     * @tc.desc Test Js Api SingleKvStore.EnableSync() with mode true
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreEnableSyncCallbackTrueTest', 0, async function (done) {
        console.info('SingleKvStoreEnableSyncCallbackTrueTest');
        try {
            await kvStore.enableSync(true, function (err, data) {
                if (err == undefined) {
                    console.info('SingleKvStoreEnableSyncCallbackTrueTest enableSync success');
                    expect(err == undefined).assertTrue();
                } else {
                    console.info('SingleKvStoreEnableSyncCallbackTrueTest enableSync fail');
                    expect(null).assertFail();
                }
                done();
            });
        } catch (e) {
            console.error('SingleKvStoreEnableSyncCallbackTrueTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name SingleKvStoreEnableSyncCallbackFalseTest
     * @tc.desc Test Js Api SingleKvStore.EnableSync() with mode false
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreEnableSyncCallbackFalseTest', 0, async function (done) {
        console.info('SingleKvStoreEnableSyncCallbackFalseTest');
        try {
            await kvStore.enableSync(false, function (err, data) {
                if (err == undefined) {
                    console.info('SingleKvStoreEnableSyncCallbackFalseTest enableSync success');
                    expect(err == undefined).assertTrue();
                } else {
                    console.info('SingleKvStoreEnableSyncCallbackFalseTest enableSync fail');
                    expect(null).assertFail();
                }
                done();
            });
        } catch (e) {
            console.error('SingleKvStoreEnableSyncCallbackFalseTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name SingleKvStoreEnableSyncCallbackInvalidArgsTest
     * @tc.desc Test Js Api SingleKvStore.EnableSync() with invlid args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreEnableSyncCallbackInvalidArgsTest', 0, async function (done) {
        console.info('SingleKvStoreEnableSyncCallbackInvalidArgsTest');
        try {
            await kvStore.enableSync(function (err, data) {
                if (err == undefined) {
                    console.info('SingleKvStoreEnableSyncCallbackInvalidArgsTest enableSync success');
                    expect(null).assertFail();
                } else {
                    console.info('SingleKvStoreEnableSyncCallbackInvalidArgsTest enableSync fail');
                    expect(null).assertFail();
                }
                done();
            });
        } catch (e) {
            console.error('SingleKvStoreEnableSyncCallbackInvalidArgsTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(e.code == 401).assertTrue();
            done();
        }
    })

    /**
     * @tc.name SingleKvStoreRemoveDeviceDataCallbackClosedKvstoreTest
     * @tc.desc Test Js Api SingleKvStore.RemoveDeviceData() in a closed kvstore
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreRemoveDeviceDataCallbackClosedKvstoreTest', 0, async function (done) {
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
            console.error('SingleKvStoreRemoveDeviceDataCallbackClosedKvstoreTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreRemoveDeviceDataCallbackInvalidArgsTest
     * @tc.desc Test Js Api SingleKvStore.RemoveDeviceData() with invalid args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreRemoveDeviceDataCallbackInvalidArgsTest', 0, async function (done) {
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
            console.error('SingleKvStoreRemoveDeviceDataCallbackInvalidArgsTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(e.code == 401).assertTrue();
            done();
        }
    })

    /**
     * @tc.name SingleKvStoreSetSyncParamCallbackSucTest
     * @tc.desc Test Js Api SingleKvStore.SetSyncParam() success
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreSetSyncParamCallbackSucTest', 0, async function (done) {
        console.info('SingleKvStoreSetSyncParamCallbackSucTest');
        try {
            var defaultAllowedDelayMs = 500;
            await kvStore.setSyncParam(defaultAllowedDelayMs, function (err, data) {
                console.info('SingleKvStoreSetSyncParamCallbackSucTest put success');
                expect(err == undefined).assertTrue();
                done();
            });
        } catch (e) {
            console.error('SingleKvStoreSetSyncParamCallbackSucTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name SingleKvStoreSetSyncParamCallbackInvalidArgsTest
     * @tc.desc Test Js Api SingleKvStore.SetSyncParam() with invalid args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreSetSyncParamCallbackInvalidArgsTest', 0, async function (done) {
        console.info('SingleKvStoreSetSyncParamCallbackInvalidArgsTest');
        try {
            await kvStore.setSyncParam(function (err, data) {
                if (err == undefined) {
                    console.info('SingleKvStoreSetSyncParamCallbackInvalidArgsTest put success');
                    expect(null).assertFail();
                } else {
                    console.error('SingleKvStoreSetSyncParamCallbackInvalidArgsTest put err' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(null).assertFail();
                }
                done();
            });
        } catch (e) {
            console.error('SingleKvStoreSetSyncParamCallbackInvalidArgsTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(e.code == 401).assertTrue();
            done();
        }
    })

    /**
     * @tc.name SingleKvStoreGetSecurityLevelCallbackSucTest
     * @tc.desc Test Js Api SingleKvStore.GetSecurityLevel() successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreGetSecurityLevelCallbackSucTest', 0, async function (done) {
        console.info('SingleKvStoreGetSecurityLevelCallbackSucTest');
        try {
            await kvStore.getSecurityLevel(function (err, data) {
                expect(data == factory.SecurityLevel.S2).assertTrue();
                done();
            });
        } catch (e) {
            console.error('SingleKvStoreGetSecurityLevelCallbackSucTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name SingleKvStoreGetSecurityLevelCallbackClosedKVStoreTest
     * @tc.desc Test Js Api SingleKvStore.GetSecurityLevel() from a closed kvstore
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreGetSecurityLevelCallbackClosedKVStoreTest', 0, async function (done) {
        console.info('SingleKvStoreGetSecurityLevelCallbackClosedKVStoreTest');
        try {
            await kvManager.closeKVStore(TEST_BUNDLE_NAME, TEST_STORE_ID, function (err) {
                expect(err == undefined).assertTrue();
            });
            await kvStore.getSecurityLevel(function (err) {
                if (err == undefined) {
                    console.info('SingleKvStoreGetSecurityLevelCallbackClosedKVStoreTest getSecurityLevel success');
                    expect(null).assertFail();
                } else {
                    console.error('SingleKvStoreGetSecurityLevelCallbackClosedKVStoreTest getSecurityLevel fail' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(err.code == 15100005).assertTrue();
                }
                done();
            });
        } catch (e) {
            console.error('SingleKvStoreGetSecurityLevelCallbackClosedKVStoreTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name SingleKvStoreGetResultSetCallbackSucTest
     * @tc.desc Test Js Api SingleKvStore.GetResultSet() successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreGetResultSetCallbackSucTest', 0, async function (done) {
        console.info('SingleKvStoreGetResultSetCallbackSucTest');
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
                console.info('SingleKvStoreGetResultSetCallbackSucTest putBatch success');
                expect(err == undefined).assertTrue();
                await kvStore.getResultSet('batch_test_string_key', async function (err, result) {
                    console.info('SingleKvStoreGetResultSetCallbackSucTest getResultSet success');
                    resultSet = result;
                    expect(resultSet.getCount() == 10).assertTrue();
                    await kvStore.closeResultSet(resultSet, function (err, data) {
                        console.info('SingleKvStoreGetResultSetCallbackSucTest closeResultSet success');
                        expect(err == undefined).assertTrue();
                        done();
                    })
                });
            });
        } catch (e) {
            console.error('SingleKvStoreGetResultSetCallbackSucTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name SingleKvStoreGetResultSetCallbackInvalidArgsTest
     * @tc.desc Test Js Api SingleKvStore.GetResultSet() with invalid args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreGetResultSetCallbackInvalidArgsTest', 0, async function (done) {
        console.info('SingleKvStoreGetResultSetCallbackInvalidArgsTest');
        try {
            let resultSet;
            await kvStore.getResultSet(function (err, result) {
                console.info('SingleKvStoreGetResultSetCallbackInvalidArgsTest getResultSet success');
                expect(null).assertFail();
                done();
            });
        } catch (e) {
            console.error('SingleKvStoreGetResultSetCallbackInvalidArgsTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(e.code == 401).assertTrue();
            done();
        }
    })

    /**
     * @tc.name SingleKvStoreGetResultSetPredicatesCallbackTest
     * @tc.desc Test Js Api SingleKvStore.GetResultSet() with predicates
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreGetResultSetPredicatesCallbackTest', 0, async function (done) {
        console.log('SingleKvStoreGetResultSetPredicatesCallbackTest');
        try {
            let resultSet;
            let predicates = new dataShare.DataSharePredicates();
            await kvStore.getResultSet(predicates).then((result) => {
                console.log('SingleKvStoreGetResultSetPredicatesCallbackTest getResultSet success');
                resultSet = result;
                expect(true).assertTrue();
            }).catch((err) => {
                console.error('SingleKvStoreGetResultSetPredicatesCallbackTest getResultSet fail ' + err`, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
            await kvStore.closeResultSet(resultSet).then((err) => {
                expect(err == undefined).assertTrue();
            }).catch((err) => {
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('SingleKvStoreGetResultSetPredicatesCallbackTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreGetResultSetQueryCallbackTest
     * @tc.desc Test Js Api SingleKvStore.GetResultSet() with query
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreGetResultSetQueryCallbackTest', 0, async function (done) {
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
                await kvStore.getResultSet(query, async function (err, result) {
                    resultSet = result;
                    expect(resultSet.getCount() == 10).assertTrue();
                    await kvStore.closeResultSet(resultSet, function (err, data) {
                        expect(err == undefined).assertTrue();
                        done();
                    })
                });
            });
        } catch (e) {
            console.error('SingleKvStoreGetResultSetQueryCallbackTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name SingleKvStoreCloseResultSetCallbackSucTest
     * @tc.desc Test Js Api SingleKvStore.CloseResultSet() successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreCloseResultSetCallbackSucTest', 0, async function (done) {
        console.info('SingleKvStoreCloseResultSetCallbackSucTest');
        try {
            let resultSet = null;
            await kvStore.getResultSet('batch_test_string_key', async function (err, result) {
                console.info('SingleKvStoreCloseResultSetCallbackSucTest getResultSet success');
                resultSet = result;
                await kvStore.closeResultSet(resultSet, function (err, data) {
                    if (err == undefined) {
                        console.info('SingleKvStoreCloseResultSetCallbackSucTest closeResultSet success');
                        expect(err == undefined).assertTrue();
                    } else {
                        console.info('SingleKvStoreCloseResultSetCallbackSucTest closeResultSet fail');
                        expect(null).assertFail();
                    }
                    done();
                });
            });
        } catch (e) {
            console.error('SingleKvStoreCloseResultSetCallbackSucTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name SingleKvStoreCloseResultSetCallbackInvalidArgsTest
     * @tc.desc Test Js Api SingleKvStore.CloseResultSet() with invalid args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreCloseResultSetCallbackInvalidArgsTest', 0, async function (done) {
        console.info('SingleKvStoreCloseResultSetCallbackInvalidArgsTest');
        try {
            console.info('SingleKvStoreCloseResultSetCallbackInvalidArgsTest success');
            await kvStore.closeResultSet(function (err, data) {
                if (err == undefined) {
                    console.info('SingleKvStoreCloseResultSetCallbackInvalidArgsTest closeResultSet success');
                    expect(null).assertFail();
                } else {
                    console.info('SingleKvStoreCloseResultSetCallbackInvalidArgsTest closeResultSet fail');
                    expect(null).assertFail();
                }
                done();
            });
        } catch (e) {
            console.error('SingleKvStoreCloseResultSetCallbackInvalidArgsTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(e.code == 401).assertTrue();
            done();
        }
    })

    /**
     * @tc.name SingleKvStoreGetResultSizeCallbackQueryTest
     * @tc.desc Test Js Api SingleKvStoreGetResultSize with query
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreGetResultSizeCallbackQueryTest', 0, async function (done) {
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
            await kvStore.getResultSize(query, async function (resultSize, err) {
                expect(err == undefined).assertTrue();
                expect(resultSize == 10).assertTrue();
            })
        } catch (e) {
            console.error('SingleKvStoreGetResultSizePromiseQueryTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreGetResultSizeCallbackInvalidArgsTest
     * @tc.desc Test Js Api SingleKvStoreGetResultSize with invalid args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreGetResultSizeCallbackInvalidArgsTest', 0, async function (done) {
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
            await kvStore.getResultSize(async function (err) {
                expect(null).assertFail();
            })
        } catch (e) {
            console.error('SingleKvStoreGetResultSizeCallbackInvalidArgsTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(e.code == 401).assertTrue();
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreGetResultSizeCallbackClosedKVStoreTest
     * @tc.desc Test Js Api SingleKvStoreGetResultSize from a closed kvstore
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreGetResultSizeCallbackClosedKVStoreTest', 0, async function (done) {
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
            await kvStore.getResultSize(query, function (err) {
                if (err == undefined) {
                    expect(null).assertFail();
                } else {
                    expect(err.code == 15100005).assertTrue();
                }
            });
        } catch (e) {
            console.error('SingleKvStoreGetResultSizeCallbackClosedKVStoreTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreGetEntriesCallbackSucTest
     * @tc.desc Test Js Api SingleKvStore.GetEntries() successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreGetEntriesCallbackSucTest', 0, async function (done) {
        console.info('SingleKvStoreGetEntriesCallbackSucTest');
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
            console.info('SingleKvStoreGetEntriesCallbackSucTest entries: ' + JSON.stringify(entries));
            await kvStore.putBatch(entries, async function (err, data) {
                console.info('SingleKvStoreGetEntriesCallbackSucTest putBatch success');
                expect(err == undefined).assertTrue();
                var query = new factory.Query();
                query.prefixKey("batch_test");
                kvStore.getEntries(query, function (err, entrys) {
                    expect(entrys.length == 10).assertTrue();
                    expect(entrys[0].value.value.toString() == arr.toString()).assertTrue();
                    done();
                });
            });
            console.info('SingleKvStoreGetEntriesCallbackSucTest success');
        } catch (e) {
            console.error('SingleKvStoreGetEntriesCallbackSucTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })
})
