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

describe('SingleKvStoreCallbackTest', function () {
    const config = {
        bundleName: TEST_BUNDLE_NAME,
        context: context
    }

    const options = {
        createIfMissing: true,
        encrypt: false,
        backup: false,
        autoSync: true,
        kvStoreType: factory.KVStoreType.SINGLE_VERSION,
        schema: '',
        securityLevel: factory.SecurityLevel.S2,
    }

    beforeAll(async function (done) {
        console.info('beforeAll config:' + JSON.stringify(config));
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
        try {
            await kvManager.closeKVStore(TEST_BUNDLE_NAME, TEST_STORE_ID, kvStore, async function (err, data) {
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
     * @tc.name SingleKvStorePutStringCallbackLongStringTest
     * @tc.desc Test Js Api SingleKvStore.Put(String) with a long string
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStorePutStringCallbackTest', 0, async function (done) {
        console.info('SingleKvStorePutStringCallbackTest');
        try {
            var str = '';
            for (var i = 0; i < 4095; i++) {
                str += 'x';
            }
            await kvStore.put(KEY_TEST_STRING_ELEMENT + '102', str, async function (err, data) {
                console.info('SingleKvStorePutStringCallbackTest put success');
                expect(err == undefined).assertTrue();
                await kvStore.get(KEY_TEST_STRING_ELEMENT + '102', function (err, data) {
                    console.info('SingleKvStorePutStringCallbackTest get success');
                    expect(str == data).assertTrue();
                    done();
                });
            });
        } catch (e) {
            console.error('SingleKvStorePutStringCallbackTest put e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name SingleKvStoreGetStringCallbackWrongArgsTest
     * @tc.desc Test Js Api SingleKvStore.GetString() with wrong args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreGetStringCallbackTest', 0, async function (done) {
        console.info('SingleKvStoreGetStringCallbackTest');
        try {
            await kvStore.get(KEY_TEST_STRING_ELEMENT, function (err, data) {
                if (err == undefined) {
                    console.info('SingleKvStoreGetStringCallbackTest get success');
                    expect(true).assertTrue();
                } else {
                    console.info('SingleKvStoreGetStringCallbackTest get fail');
                }
                done();
            });
        } catch (e) {
            console.error('SingleKvStoreGetStringCallbackTest get e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
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
            await kvStore.put(KEY_TEST_STRING_ELEMENT, VALUE_TEST_STRING_ELEMENT, async function (err, data) {
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
     * @tc.name SingleKvStorePutIntCallbackSucTest
     * @tc.desc Test Js Api SingleKvStore.Put(Int) successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStorePutIntCallbackSucTest', 0, async function (done) {
        console.info('SingleKvStorePutIntCallbackSucTest');
        try {
            await kvStore.put(KEY_TEST_INT_ELEMENT, VALUE_TEST_INT_ELEMENT, async function (err, data) {
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
            await kvStore.put(KEY_TEST_INT_ELEMENT, intValue, async function (err, data) {
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
            await kvStore.put(KEY_TEST_INT_ELEMENT, intValue, async function (err, data) {
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
            await kvStore.get(KEY_TEST_INT_ELEMENT, function (err, data) {
                if (err == undefined) {
                    console.info('SingleKvStoreGetIntCallbackTest get success');
                    expect(true).assertTrue();
                } else {
                    console.info('SingleKvStoreGetIntCallbackTest get fail');
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
            await kvStore.put(KEY_TEST_BOOLEAN_ELEMENT, VALUE_TEST_BOOLEAN_ELEMENT, function (err, data) {
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
     * @tc.name SingleKvStoreGetBoolCallbackNonExistTest
     * @tc.desc Test Js Api SingleKvStore.GetBool() get non-existing bool
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreGetBoolCallbackNonExistTest', 0, async function (done) {
        console.info('SingleKvStoreGetBoolCallbackNonExistTest');
        try {
            await kvStore.get(KEY_TEST_BOOLEAN_ELEMENT, function (err, data) {
                if (err == undefined) {
                    console.info('SingleKvStoreGetBoolCallbackNonExistTest get success');
                    expect(true).assertTrue();
                } else {
                    console.error('SingleKvStoreGetBoolCallbackNonExistTest get fail' + `, error code is ${err.code}, message is ${err.message}`);
                }
                done();
            });
        } catch (e) {
            console.error('SingleKvStoreGetBoolCallbackNonExistTest e' + `, error code is ${e.code}, message is ${e.message}`);
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
     * @tc.name SingleKvStoreDeleteStringCallbackNoPutTest
     * @tc.desc Test Js Api SingleKvStore.DeleteString() delete without put
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreDeleteStringCallbackNoPutTest', 0, async function (done) {
        console.info('SingleKvStoreDeleteStringCallbackNoPutTest');
        try {
            await kvStore.delete(KEY_TEST_STRING_ELEMENT, function (err, data) {
                if (err == undefined) {
                    console.info('SingleKvStoreDeleteStringCallbackNoPutTest delete success');
                } else {
                    console.error('SingleKvStoreDeleteStringCallbackNoPutTest delete fail' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(null).assertFail();
                }
                done();
            });
        } catch (e) {
            console.error('SingleKvStoreDeleteStringCallbackNoPutTest e' + `, error code is ${e.code}, message is ${e.message}`);
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
     * @tc.name SingleKvStoreDeleteIntCallbackNoPutTest
     * @tc.desc Test Js Api SingleKvStore.DeleteInt() without put
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreDeleteIntCallbackNoPutTest', 0, async function (done) {
        console.info('SingleKvStoreDeleteIntCallbackNoPutTest');
        try {
            await kvStore.delete(KEY_TEST_INT_ELEMENT, function (err, data) {
                if (err == undefined) {
                    console.info('SingleKvStoreDeleteIntCallbackNoPutTest get success');
                } else {
                    console.error('SingleKvStoreDeleteIntCallbackNoPutTest get fail' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(null).assertFail();
                }
                done();
            });
        } catch (e) {
            console.error('SingleKvStoreDeleteIntCallbackNoPutTest e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
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
     * @tc.name SingleKvStoreDeleteFloatCallbackNoPutTest
     * @tc.desc Test Js Api SingleKvStore.DeleteFloat() without put
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreDeleteFloatCallbackNoPutTest', 0, async function (done) {
        console.info('SingleKvStoreDeleteFloatCallbackNoPutTest');
        try {
            await kvStore.delete(KEY_TEST_FLOAT_ELEMENT, function (err, data) {
                if (err == undefined) {
                    console.info('SingleKvStoreDeleteFloatCallbackNoPutTest get success');
                } else {
                    console.error('SingleKvStoreDeleteFloatCallbackNoPutTest get fail' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(null).assertFail();
                }
                done();
            });
        } catch (e) {
            console.error('SingleKvStoreDeleteFloatCallbackNoPutTest e' + `, error code is ${e.code}, message is ${e.message}`);
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
     * @tc.name SingleKvStoreDeleteBoolCallbackNoPutTest
     * @tc.desc Test Js Api SingleKvStore.DeleteBool() wihtout put
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreDeleteBoolCallbackNoPutTest', 0, async function (done) {
        console.info('SingleKvStoreDeleteBoolCallbackNoPutTest');
        try {
            await kvStore.delete(KEY_TEST_BOOLEAN_ELEMENT, function (err, data) {
                if (err == undefined) {
                    console.info('SingleKvStoreDeleteBoolCallbackNoPutTest get success');
                } else {
                    console.error('SingleKvStoreDeleteBoolCallbackNoPutTest get fail' + `, error code is ${err.code}, message is ${err.message}`);
                    expect(null).assertFail();
                }
                done();
            });
        } catch (e) {
            console.error('SingleKvStoreDeleteBoolCallbackNoPutTest e' + `, error code is ${e.code}, message is ${e.message}`);
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
     * @tc.name SingleKvStoreDeletePredicatesCallbackNoPutTest
     * @tc.desc Test Js Api SingleKvStore.Delete() without put
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreDeletePredicatesCallbackTest', 0, async function (done) {
        console.log('SingleKvStoreDeletePredicatesCallbackTest');
        try {
            let predicates = new dataShare.DataSharePredicates();
            await kvStore.delete(predicates, function (err, data) {
                if (err == undefined) {
                    console.log('SingleKvStoreDeletePredicatesCallbackTest delete success');
                    expect(null).assertFail();
                } else {
                    console.error('SingleKvStoreDeletePredicatesCallbackTest delete fail' + err`, error code is ${err.code}, message is ${err.message}`);
                    expect(err != undefined).assertTrue();
                }
                done();
            });
        } catch (e) {
            console.error('SingleKvStoreDeletePredicatesCallbackTest e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(e.code == 401).assertTrue();
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
     * @tc.name SingleKvStoreDeletePredicatesCallbackNullInkeysTest
     * @tc.desc Test Js Api SingleKvStore.Delete() with null inkeys
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreDeletePredicatesCallbackNullInkeysTest', 0, async function (done) {
        console.log('SingleKvStoreDeletePredicatesCallbackNullInkeysTest');
        try {
            let predicates = new dataShare.DataSharePredicates();
            let arr = [null];
            predicates.inKeys(arr);
            await kvStore.put("name", "Bob", async function (err, data) {
                console.log('SingleKvStoreDeletePredicatesCallbackNullInkeysTest put success');
                expect(err == undefined).assertTrue();
                await kvStore.delete(predicates, function (err, data) {
                    console.log('SingleKvStoreDeletePredicatesCallbackNullInkeysTest delete success: ' + err);
                    expect(err == undefined).assertTrue();
                    done();
                });
            })
        } catch (e) {
            console.error('SingleKvStoreDeletePredicatesCallbackNullInkeysTest e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

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
     * @tc.name SingleKvStoreOnSyncCompleteCallbackTestPullOnly001
     * @tc.desc Test Js Api SingleKvStore.OnSyncComplete() with mode type only
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreOnSyncCompleteCallbackTestPullOnly001', 0, async function (done) {
        try {
            kvStore.on('syncComplete', function (data) {
                console.info('SingleKvStoreOnSyncCompleteCallbackTestPullOnly001 dataChange');
                expect(data != null).assertTrue();
            });
            await kvStore.put(KEY_TEST_SYNC_ELEMENT + 'Sync1Test', VALUE_TEST_SYNC_ELEMENT, function (err, data) {
                console.info('SingleKvStoreOnSyncCompleteCallbackTestPullOnly001 put success');
                expect(err == undefined).assertTrue();
            });
            try {
                var mode = factory.SyncMode.PULL_ONLY;
                console.info('kvStore.sync to ' + JSON.stringify(syncDeviceIds));
                kvStore.sync(syncDeviceIds, mode, 10);
            } catch (e) {
                console.error('SingleKvStoreOnSyncCompleteCallbackTestPullOnly001 sync no peer device :e:' + `, error code is ${e.code}, message is ${e.message}`);
            }
        } catch (e) {
            console.error('SingleKvStoreOnSyncCompleteCallbackTestPullOnly001 e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreOnSyncCompleteCallbackTestPushOnly001
     * @tc.desc Test Js Api SingleKvStore.OnSyncComplete() with mode push only
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreOnSyncCompleteCallbackTestPushOnly001', 0, async function (done) {
        try {
            kvStore.on('syncComplete', function (data) {
                console.info('SingleKvStoreOnSyncCompleteCallbackTestPushOnly001 dataChange');
                expect(data != null).assertTrue();
            });
            await kvStore.put(KEY_TEST_SYNC_ELEMENT + 'Sync1Test', VALUE_TEST_SYNC_ELEMENT, function (err, data) {
                console.info('SingleKvStoreOnSyncCompleteCallbackTestPushOnly001 put success');
                expect(err == undefined).assertTrue();
            });
            try {
                var mode = factory.SyncMode.PUSH_ONLY;
                console.info('kvStore.sync to ' + JSON.stringify(syncDeviceIds));
                kvStore.sync(syncDeviceIds, mode, 10);
            } catch (e) {
                console.error('SingleKvStoreOnSyncCompleteCallbackTestPushOnly001 sync no peer device :e:' + `, error code is ${e.code}, message is ${e.message}`);
            }
        } catch (e) {
            console.error('SingleKvStoreOnSyncCompleteCallbackTestPushOnly001 e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreOnSyncCompleteCallbackTestPushPull001
     * @tc.desc Test Js Api SingleKvStore.OnSyncComplete() with mode push_pull
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreOnSyncCompleteCallbackTestPushPull001', 0, async function (done) {
        try {
            kvStore.on('syncComplete', function (data) {
                console.info('SingleKvStoreOnSyncCompleteCallbackTestPushPull001 dataChange');
                expect(data != null).assertTrue();
            });
            await kvStore.put(KEY_TEST_SYNC_ELEMENT + 'Sync1Test', VALUE_TEST_SYNC_ELEMENT, function (err, data) {
                console.info('SingleKvStoreOnSyncCompleteCallbackTestPushPull001 put success');
                expect(err == undefined).assertTrue();
            });
            try {
                var mode = factory.SyncMode.PUSH_PULL;
                console.info('kvStore.sync to ' + JSON.stringify(syncDeviceIds));
                kvStore.sync(syncDeviceIds, mode, 10);
            } catch (e) {
                console.error('SingleKvStoreOnSyncCompleteCallbackTestPushPull001 sync no peer device :e:' + `, error code is ${e.code}, message is ${e.message}`);
            }
        } catch (e) {
            console.error('SingleKvStoreOnSyncCompleteCallbackTestPushPull001 e' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
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
                await kvStore.getEntries('batch_test_string_key', function (err, entrys) {
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
                await kvStore.getEntries('batch_test_number_key', function (err, entrys) {
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
                await kvStore.getEntries('batch_test_number_key', function (err, entrys) {
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
                await kvStore.getEntries('batch_test_number_key', function (err, entrys) {
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
                await kvStore.getEntries('batch_test_bool_key', function (err, entrys) {
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
                await kvStore.getEntries('batch_test_bool_key', function (err, entrys) {
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
            let arr1 = new Uint8Array([4,5,6,7]);
            let arr2 = new Uint8Array([4,5,6,7,8]);
            let vb1 = {key : "name_1", value : arr1};
            let vb2 = {key : "name_2", value : arr2};
            values.push(vb1);
            values.push(vb2);
            console.info('SingleKvStorePutBatchValueCallbackUint8ArrayTest001 values: ' + JSON.stringify(values));
            await kvStore.putBatch(values, async function (err,data) {
                console.info('SingleKvStorePutBatchValueCallbackUint8ArrayTest001 putBatch success');
                expect(err == undefined).assertTrue();
                var query = new factory.Query();
                query.prefixKey("name_");
                await kvStore.getEntries(query, function (err,entrys) {
                    expect(entrys.length == 2).assertTrue();
                    done();
                });
            });
        }catch(e) {
            console.error('SingleKvStorePutBatchValueCallbackUint8ArrayTest001 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
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
     * @tc.name SingleKvStoreDeleteBatchCallbackNoPutTest
     * @tc.desc Test Js Api SingleKvStore.DeleteBatch() with no putbatch
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreDeleteBatchCallbackNoPutTest', 0, async function (done) {
        console.info('SingleKvStoreDeleteBatchCallbackNoPutTest');
        try {
            let keys = ['batch_test_string_key1', 'batch_test_string_key2'];
            await kvStore.deleteBatch(keys, function (err, data) {
                console.info('SingleKvStoreDeleteBatchCallbackNoPutTest deleteBatch success');
                expect(err == undefined).assertTrue();
                done();
            });
        } catch (e) {
            console.error('SingleKvStoreDeleteBatchCallbackNoPutTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name SingleKvStoreDeleteBatchCallbackWrongKeysTest
     * @tc.desc Test Js Api SingleKvStore.DeleteBatch() with wrong keys
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreDeleteBatchCallbackWrongKeysTest', 0, async function (done) {
        console.info('SingleKvStoreDeleteBatchCallbackWrongKeysTest');
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
            console.info('SingleKvStoreDeleteBatchCallbackWrongKeysTest entries: ' + JSON.stringify(entries));
            await kvStore.putBatch(entries, async function (err, data) {
                console.info('SingleKvStoreDeleteBatchCallbackWrongKeysTest putBatch success');
                expect(err == undefined).assertTrue();
                let keys = ['batch_test_string_key1', 'batch_test_string_keya'];
                await kvStore.deleteBatch(keys, async function (err, data) {
                    console.info('SingleKvStoreDeleteBatchCallbackWrongKeysTest deleteBatch success');
                    expect(err == undefined).assertTrue();
                    done();
                });
            });
        } catch (e) {
            console.error('SingleKvStoreDeleteBatchCallbackWrongKeysTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
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
                console.info('SingleKvstoreStartTransactionCallbackCommitTest startTransaction success');
                expect(err == undefined).assertTrue();
                let entries = putBatchString(10, 'batch_test_string_key');
                console.info('SingleKvstoreStartTransactionCallbackCommitTest entries: ' + JSON.stringify(entries));
                await kvStore.putBatch(entries, async function (err, data) {
                    console.info('SingleKvstoreStartTransactionCallbackCommitTest putBatch success');
                    expect(err == undefined).assertTrue();
                    let keys = Object.keys(entries).slice(5);
                    await kvStore.deleteBatch(keys, async function (err, data) {
                        console.info('SingleKvstoreStartTransactionCallbackCommitTest deleteBatch success');
                        expect(err == undefined).assertTrue();
                        await kvStore.commit(async function (err, data) {
                            console.info('SingleKvstoreStartTransactionCallbackCommitTest commit success');
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
        console.info('SingleKvstoreStartTransactionCallbackRollbackTest');
        try {
            var count = 0;
            kvStore.on('dataChange', 0, function (data) {
                console.info('SingleKvstoreStartTransactionCallbackRollbackTest 0' + data)
                count++;
            });
            await kvStore.startTransaction(async function (err, data) {
                console.info('SingleKvstoreStartTransactionCallbackRollbackTest startTransaction success');
                expect(err == undefined).assertTrue();
                let entries = putBatchString(10, 'batch_test_string_key');
                console.info('SingleKvstoreStartTransactionCallbackRollbackTest entries: ' + JSON.stringify(entries));
                await kvStore.putBatch(entries, async function (err, data) {
                    console.info('SingleKvstoreStartTransactionCallbackRollbackTest putBatch success');
                    expect(err == undefined).assertTrue();
                    let keys = Object.keys(entries).slice(5);
                    await kvStore.deleteBatch(keys, async function (err, data) {
                        console.info('SingleKvstoreStartTransactionCallbackRollbackTest deleteBatch success');
                        expect(err == undefined).assertTrue();
                        await kvStore.rollback(async function (err, data) {
                            console.info('SingleKvstoreStartTransactionCallbackRollbackTest rollback success');
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
     * @tc.name SingleKvstoreStartTransactionCallbackWrongArgsTest
     * @tc.desc Test Js Api SingleKvStore.startTransaction() with wrong arguments
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvstoreStartTransactionCallbackWrongArgsTest', 0, async function (done) {
        console.info('SingleKvstoreStartTransactionCallbackWrongArgsTest');
        try {
            await kvStore.startTransaction(1, function (err, data) {
                if (err == undefined) {
                    console.info('SingleKvstoreStartTransactionCallbackWrongArgsTest startTransaction success');
                    expect(null).assertFail();
                } else {
                    console.info('SingleKvstoreStartTransactionCallbackWrongArgsTest startTransaction fail');
                }
                done();
            });
        } catch (e) {
            console.error('SingleKvstoreStartTransactionCallbackWrongArgsTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            done();
        }
    })

    /**
     * @tc.name SingleKvStoreCommitCallbackWrongArgsTest
     * @tc.desc Test Js Api SingleKvStore.Commit() with wrong args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreCommitCallbackWrongArgsTest', 0, async function (done) {
        console.info('SingleKvStoreCommitCallbackWrongArgsTest');
        try {
            await kvStore.commit(1, function (err, data) {
                if (err == undefined) {
                    console.info('SingleKvStoreCommitCallbackWrongArgsTest commit success');
                    expect(null).assertFail();
                } else {
                    console.info('SingleKvStoreCommitCallbackWrongArgsTest commit fail');
                }
                done();
            });
        } catch (e) {
            console.error('SingleKvStoreCommitCallbackWrongArgsTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            done();
        }
    })

    /**
     * @tc.name SingleKvStoreRollbackCallbackWrongArgsTest
     * @tc.desc Test Js Api SingleKvStore.Rollback() with wrong args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreRollbackCallbackWrongArgsTest', 0, async function (done) {
        console.info('SingleKvStoreRollbackCallbackWrongArgsTest');
        try {
            await kvStore.rollback(1, function (err, data) {
                if (err == undefined) {
                    console.info('SingleKvStoreRollbackCallbackWrongArgsTest commit success');
                    expect(null).assertFail();
                } else {
                    console.info('SingleKvStoreRollbackCallbackWrongArgsTest commit fail');
                }
                done();
            });
        } catch (e) {
            console.error('SingleKvStoreRollbackCallbackWrongArgsTest e ' + `, error code is ${e.code}, message is ${e.message}`);
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
     * @tc.name SingleKvStoreRemoveDeviceDataCallbackNonExistTest
     * @tc.desc Test Js Api SingleKvStore.RemoveDeviceData() with non-exsiting device id
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreRemoveDeviceDataCallbackNonExistTest', 0, async function (done) {
        console.info('SingleKvStoreRemoveDeviceDataCallbackNonExistTest');
        try {
            await kvStore.put(KEY_TEST_STRING_ELEMENT, VALUE_TEST_STRING_ELEMENT, async function (err, data) {
                console.info('SingleKvStoreRemoveDeviceDataCallbackNonExistTest put success');
                expect(err == undefined).assertTrue();
                var deviceid = 'no_exist_device_id';
                await kvStore.removeDeviceData(deviceid, async function (err, data) {
                    if (err == undefined) {
                        console.info('SingleKvStoreRemoveDeviceDataCallbackNonExistTest removeDeviceData success');
                        expect(null).assertFail();
                        done();
                    } else {
                        console.info('SingleKvStoreRemoveDeviceDataCallbackNonExistTest removeDeviceData fail');
                        await kvStore.get(KEY_TEST_STRING_ELEMENT, async function (err, data) {
                            console.info('SingleKvStoreRemoveDeviceDataCallbackNonExistTest get success');
                            expect(data == VALUE_TEST_STRING_ELEMENT).assertTrue();
                            done();
                        });
                    }
                });
            });
        } catch (e) {
            console.error('SingleKvStoreRemoveDeviceDataCallbackNonExistTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
    })

    /**
     * @tc.name SingleKvStoreRemoveDeviceDataCallbackTestNoArgs002
     * @tc.desc Test Js Api SingleKvStore.RemoveDeviceData() with no args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreRemoveDeviceDataCallbackTestNoArgs002', 0, async function (done) {
        console.info('SingleKvStoreRemoveDeviceDataCallbackTestNoArgs002');
        try {
            await kvStore.removeDeviceData(function (err, data) {
                if (err == undefined) {
                    console.info('SingleKvStoreRemoveDeviceDataCallbackTestNoArgs002 removeDeviceData success');
                    expect(null).assertFail();
                } else {
                    console.info('SingleKvStoreRemoveDeviceDataCallbackTestNoArgs002 removeDeviceData fail');
                }
                done();
            });
        } catch (e) {
            console.error('SingleKvStoreRemoveDeviceDataCallbackTestNoArgs002 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(e.code == 401).assertTrue();
            done();
        }
    })

    /**
     * @tc.name SingleKvStoreRemoveDeviceDataCallbackInvalidArgsTest
     * @tc.desc Test Js Api SingleKvStore.RemoveDeviceData() with invalid args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreRemoveDeviceDataCallbackInvalidArgsTest', 0, async function (done) {
        console.info('SingleKvStoreRemoveDeviceDataCallbackInvalidArgsTest');
        try {
            await kvStore.removeDeviceData('', function (err, data) {
                if (err == undefined) {
                    console.info('SingleKvStoreRemoveDeviceDataCallbackInvalidArgsTest removeDeviceData success');
                    expect(null).assertFail();
                } else {
                    console.info('SingleKvStoreRemoveDeviceDataCallbackInvalidArgsTest removeDeviceData fail');
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
     * @tc.name SingleKvStoreGetSecurityLevelCallbackInvalidArgsTest
     * @tc.desc Test Js Api SingleKvStore.GetSecurityLevel() with invalid args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreGetSecurityLevelCallbackInvalidArgsTest', 0, async function (done) {
        console.info('SingleKvStoreGetSecurityLevelCallbackInvalidArgsTest');
        try {
            await kvStore.getSecurityLevel(1, function (err, data) {
                if (err == undefined) {
                    console.info('SingleKvStoreGetSecurityLevelCallbackInvalidArgsTest getSecurityLevel success');
                    expect(null).assertFail();
                } else {
                    console.error('SingleKvStoreGetSecurityLevelCallbackInvalidArgsTest getSecurityLevel fail' + `, error code is ${err.code}, message is ${err.message}`);
                }
                done();
            });
        } catch (e) {
            console.error('SingleKvStoreGetSecurityLevelCallbackInvalidArgsTest e ' + `, error code is ${e.code}, message is ${e.message}`);
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
            let predicates = new dataShare.DataSharePredicates();
            await kvStore.getResultSet(predicates).then((result) => {
                console.log('SingleKvStoreGetResultSetPredicatesCallbackTest getResultSet success');
            }).catch((err) => {
                console.error('SingleKvStoreGetResultSetPredicatesCallbackTest getResultSet fail ' + err`, error code is ${err.code}, message is ${err.message}`);
                expect(err == undefined).assertTrue();
            });
        } catch (e) {
            console.error('SingleKvStoreGetResultSetPredicatesCallbackTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SingleKvStoreCloseResultSetCallbackNullTest
     * @tc.desc Test Js Api SingleKvStore.CloseResultSet() close null resultset
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreCloseResultSetCallbackNullTest', 0, async function (done) {
        console.info('SingleKvStoreCloseResultSetCallbackNullTest');
        try {
            console.info('SingleKvStoreCloseResultSetCallbackNullTest success');
            let resultSet = null;
            await kvStore.closeResultSet(resultSet, function (err, data) {
                if (err == undefined) {
                    console.info('SingleKvStoreCloseResultSetCallbackNullTest closeResultSet success');
                    expect(null).assertFail();
                } else {
                    console.info('SingleKvStoreCloseResultSetCallbackNullTest closeResultSet fail');
                }
                done();
            });
        } catch (e) {
            console.error('SingleKvStoreCloseResultSetCallbackNullTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(e.code == 401).assertTrue();
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
