/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
import abilityFeatureAbility from '@ohos.ability.featureAbility'

var context = abilityFeatureAbility.getContext();
const TEST_BUNDLE_NAME = 'com.example.myapplication';
const TEST_STORE_ID = 'rekeyStoreId';
var kvManager = null;

const TAG = "[SINGLE_KV_STORE_REKEY_TEST]";

describe('SingleKvStoreRekeyTest', function () {
    const config = {
        bundleName: TEST_BUNDLE_NAME,
        context: context
    }

    const encryptedOptions = {
        createIfMissing: true,
        encrypt: true,
        backup: false,
        autoSync: false,
        kvStoreType: factory.KVStoreType.SINGLE_VERSION,
        schema: '',
        securityLevel: factory.SecurityLevel.S2,
    }

    const unencryptedOptions = {
        createIfMissing: true,
        encrypt: false,
        backup: false,
        autoSync: false,
        kvStoreType: factory.KVStoreType.SINGLE_VERSION,
        schema: '',
        securityLevel: factory.SecurityLevel.S2,
    }

    beforeAll(async function (done) {
        console.info(TAG + 'beforeAll');
        kvManager = factory.createKVManager(config);
        done();
    })

    afterAll(async function (done) {
        console.info(TAG + 'afterAll');
        kvManager = null;
        done();
    })

    afterEach(async function (done) {
        console.info(TAG + 'afterEach');
        try {
            await kvManager.closeKVStore(TEST_BUNDLE_NAME, TEST_STORE_ID);
            await kvManager.deleteKVStore(TEST_BUNDLE_NAME, TEST_STORE_ID);
        } catch (e) {
            console.info(TAG + 'afterEach cleanup err: ' + e.message);
        }
        done();
    })

    /**
     * @tc.number SingleKvStoreRekeyPromiseSucTest001
     * @tc.name Test basic rekey on encrypted KV store using promise
     * @tc.desc Rekey an encrypted store and verify data is accessible after rekey
     * @tc.type FUNC
     */
    it('SingleKvStoreRekeyPromiseSucTest001', 0, async function (done) {
        console.info(TAG + "************* SingleKvStoreRekeyPromiseSucTest001 start *************");
        try {
            let kvStore = await kvManager.getKVStore(TEST_STORE_ID, encryptedOptions);
            expect(kvStore != null && kvStore != undefined).assertTrue();

            await kvStore.put("rekey_key1", "rekey_value1");
            await kvStore.rekey();

            let value = await kvStore.get("rekey_key1");
            expect("rekey_value1").assertEqual(value);
            console.info(TAG + "rekey success, data verified");
        } catch (e) {
            console.error(TAG + `fail: error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
        console.info(TAG + "************* SingleKvStoreRekeyPromiseSucTest001 end *************");
    })

    /**
     * @tc.number SingleKvStoreRekeyCallbackSucTest002
     * @tc.name Test basic rekey on encrypted KV store using callback
     * @tc.desc Rekey an encrypted store with callback and verify data is accessible
     * @tc.type FUNC
     */
    it('SingleKvStoreRekeyCallbackSucTest002', 0, async function (done) {
        console.info(TAG + "************* SingleKvStoreRekeyCallbackSucTest002 start *************");
        try {
            let kvStore = await kvManager.getKVStore(TEST_STORE_ID, encryptedOptions);
            expect(kvStore != null && kvStore != undefined).assertTrue();

            await kvStore.put("rekey_key1", "rekey_value1");
            kvStore.rekey(function (err) {
                if (err) {
                    console.error(TAG + `rekey callback err: code=${err.code}, message=${err.message}`);
                    expect(null).assertFail();
                    done();
                    return;
                }
                console.info(TAG + "rekey callback success");
                kvStore.get("rekey_key1", function (err, value) {
                    if (err) {
                        expect(null).assertFail();
                        done();
                        return;
                    }
                    expect("rekey_value1").assertEqual(value);
                    done();
                });
            });
        } catch (e) {
            console.error(TAG + `fail: error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
        console.info(TAG + "************* SingleKvStoreRekeyCallbackSucTest002 end *************");
    })

    /**
     * @tc.number SingleKvStoreRekeyUnencryptedTest003
     * @tc.name Test rekey on unencrypted store should fail
     * @tc.desc Rekey on a non-encrypted store should return error
     * @tc.type FUNC
     */
    it('SingleKvStoreRekeyUnencryptedTest003', 0, async function (done) {
        console.info(TAG + "************* SingleKvStoreRekeyUnencryptedTest003 start *************");
        try {
            let kvStore = await kvManager.getKVStore(TEST_STORE_ID, unencryptedOptions);
            expect(kvStore != null && kvStore != undefined).assertTrue();

            await kvStore.put("test_key", "test_value");
            await kvStore.rekey();
            expect(null).assertFail();
        } catch (e) {
            console.info(TAG + `expected error: code=${e.code}, message=${e.message}`);
            expect(e.code != undefined).assertTrue();
        }
        done();
        console.info(TAG + "************* SingleKvStoreRekeyUnencryptedTest003 end *************");
    })

    /**
     * @tc.number SingleKvStoreRekeyMultipleTimesTest004
     * @tc.name Test rekey multiple times consecutively
     * @tc.desc Rekey twice and verify data integrity after each rekey
     * @tc.type FUNC
     */
    it('SingleKvStoreRekeyMultipleTimesTest004', 0, async function (done) {
        console.info(TAG + "************* SingleKvStoreRekeyMultipleTimesTest004 start *************");
        try {
            let kvStore = await kvManager.getKVStore(TEST_STORE_ID, encryptedOptions);
            expect(kvStore != null && kvStore != undefined).assertTrue();

            await kvStore.put("key1", "value1");
            await kvStore.rekey();

            let value = await kvStore.get("key1");
            expect("value1").assertEqual(value);

            await kvStore.put("key2", "value2");
            await kvStore.rekey();

            value = await kvStore.get("key1");
            expect("value1").assertEqual(value);
            value = await kvStore.get("key2");
            expect("value2").assertEqual(value);
        } catch (e) {
            console.error(TAG + `fail: error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
        console.info(TAG + "************* SingleKvStoreRekeyMultipleTimesTest004 end *************");
    })

    /**
     * @tc.number SingleKvStoreRekeyWithBatchDataTest005
     * @tc.name Test rekey with batch data operations
     * @tc.desc Put batch data, rekey, then verify all entries are accessible
     * @tc.type FUNC
     */
    it('SingleKvStoreRekeyWithBatchDataTest005', 0, async function (done) {
        console.info(TAG + "************* SingleKvStoreRekeyWithBatchDataTest005 start *************");
        try {
            let kvStore = await kvManager.getKVStore(TEST_STORE_ID, encryptedOptions);
            expect(kvStore != null && kvStore != undefined).assertTrue();

            let entries = [];
            for (var i = 0; i < 20; i++) {
                entries.push({
                    key: "batch_key_" + i,
                    value: {
                        type: factory.ValueType.STRING,
                        value: "batch_value_" + i,
                    }
                });
            }
            await kvStore.putBatch(entries);

            await kvStore.rekey();

            let resultEntries = await kvStore.getEntries("batch_key_");
            expect(resultEntries.length).assertEqual(20);
        } catch (e) {
            console.error(TAG + `fail: error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
        console.info(TAG + "************* SingleKvStoreRekeyWithBatchDataTest005 end *************");
    })

    /**
     * @tc.number SingleKvStoreRekeyAfterDeleteTest006
     * @tc.name Test rekey after deleting data
     * @tc.desc Delete some entries, rekey, then verify remaining data
     * @tc.type FUNC
     */
    it('SingleKvStoreRekeyAfterDeleteTest006', 0, async function (done) {
        console.info(TAG + "************* SingleKvStoreRekeyAfterDeleteTest006 start *************");
        try {
            let kvStore = await kvManager.getKVStore(TEST_STORE_ID, encryptedOptions);
            expect(kvStore != null && kvStore != undefined).assertTrue();

            await kvStore.put("del_key1", "del_value1");
            await kvStore.put("del_key2", "del_value2");
            await kvStore.delete("del_key1");

            await kvStore.rekey();

            try {
                await kvStore.get("del_key1");
                expect(null).assertFail();
            } catch (getErr) {
                console.info(TAG + "del_key1 correctly not found after rekey");
            }

            let value = await kvStore.get("del_key2");
            expect("del_value2").assertEqual(value);
        } catch (e) {
            console.error(TAG + `fail: error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
        console.info(TAG + "************* SingleKvStoreRekeyAfterDeleteTest006 end *************");
    })

    /**
     * @tc.number SingleKvStoreRekeyThenPutGetTest007
     * @tc.name Test put and get after rekey
     * @tc.desc After rekey, put new data and verify get returns correct values
     * @tc.type FUNC
     */
    it('SingleKvStoreRekeyThenPutGetTest007', 0, async function (done) {
        console.info(TAG + "************* SingleKvStoreRekeyThenPutGetTest007 start *************");
        try {
            let kvStore = await kvManager.getKVStore(TEST_STORE_ID, encryptedOptions);
            expect(kvStore != null && kvStore != undefined).assertTrue();

            await kvStore.rekey();

            await kvStore.put("after_rekey_key", "after_rekey_value");
            let value = await kvStore.get("after_rekey_key");
            expect("after_rekey_value").assertEqual(value);

            await kvStore.put("int_key", 12345);
            value = await kvStore.get("int_key");
            expect(12345).assertEqual(value);

            await kvStore.put("bool_key", true);
            value = await kvStore.get("bool_key");
            expect(true).assertEqual(value);

            await kvStore.put("float_key", 3.14);
            value = await kvStore.get("float_key");
            expect(3.14).assertEqual(value);
        } catch (e) {
            console.error(TAG + `fail: error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
        console.info(TAG + "************* SingleKvStoreRekeyThenPutGetTest007 end *************");
    })

    /**
     * @tc.number SingleKvStoreRekeyClosedStoreTest008
     * @tc.name Test rekey on closed store should fail
     * @tc.desc Close the store then call rekey, expect error
     * @tc.type FUNC
     */
    it('SingleKvStoreRekeyClosedStoreTest008', 0, async function (done) {
        console.info(TAG + "************* SingleKvStoreRekeyClosedStoreTest008 start *************");
        try {
            let kvStore = await kvManager.getKVStore(TEST_STORE_ID, encryptedOptions);
            expect(kvStore != null && kvStore != undefined).assertTrue();

            await kvManager.closeKVStore(TEST_BUNDLE_NAME, TEST_STORE_ID);
            await kvStore.rekey();
            expect(null).assertFail();
        } catch (e) {
            console.info(TAG + `expected error on closed store: code=${e.code}, message=${e.message}`);
            expect(e.code != undefined).assertTrue();
        }
        done();
        console.info(TAG + "************* SingleKvStoreRekeyClosedStoreTest008 end *************");
    })

    /**
     * @tc.number SingleKvStoreRekeyEmptyStoreTest009
     * @tc.name Test rekey on empty encrypted store
     * @tc.desc Rekey an encrypted store with no data should succeed
     * @tc.type FUNC
     */
    it('SingleKvStoreRekeyEmptyStoreTest009', 0, async function (done) {
        console.info(TAG + "************* SingleKvStoreRekeyEmptyStoreTest009 start *************");
        try {
            let kvStore = await kvManager.getKVStore(TEST_STORE_ID, encryptedOptions);
            expect(kvStore != null && kvStore != undefined).assertTrue();

            await kvStore.rekey();
            console.info(TAG + "rekey on empty store succeeded");
        } catch (e) {
            console.error(TAG + `fail: error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
        console.info(TAG + "************* SingleKvStoreRekeyEmptyStoreTest009 end *************");
    })

    /**
     * @tc.number SingleKvStoreRekeyCallbackUnencryptedTest010
     * @tc.name Test rekey callback on unencrypted store should fail
     * @tc.desc Rekey on a non-encrypted store with callback should return error
     * @tc.type FUNC
     */
    it('SingleKvStoreRekeyCallbackUnencryptedTest010', 0, async function (done) {
        console.info(TAG + "************* SingleKvStoreRekeyCallbackUnencryptedTest010 start *************");
        try {
            let kvStore = await kvManager.getKVStore(TEST_STORE_ID, unencryptedOptions);
            expect(kvStore != null && kvStore != undefined).assertTrue();

            kvStore.rekey(function (err) {
                if (err) {
                    console.info(TAG + `expected callback error: code=${err.code}`);
                    expect(err.code != undefined).assertTrue();
                    done();
                    return;
                }
                expect(null).assertFail();
                done();
            });
        } catch (e) {
            console.info(TAG + `caught error: code=${e.code}`);
            expect(e.code != undefined).assertTrue();
            done();
        }
        console.info(TAG + "************* SingleKvStoreRekeyCallbackUnencryptedTest010 end *************");
    })

    /**
     * @tc.number SingleKvStoreRekeyDeleteBatchTest011
     * @tc.name Test rekey after batch delete operations
     * @tc.desc Delete batch entries, rekey, then verify remaining data
     * @tc.type FUNC
     */
    it('SingleKvStoreRekeyDeleteBatchTest011', 0, async function (done) {
        console.info(TAG + "************* SingleKvStoreRekeyDeleteBatchTest011 start *************");
        try {
            let kvStore = await kvManager.getKVStore(TEST_STORE_ID, encryptedOptions);
            expect(kvStore != null && kvStore != undefined).assertTrue();

            let entries = [];
            for (var i = 0; i < 10; i++) {
                entries.push({
                    key: "delbatch_key_" + i,
                    value: {
                        type: factory.ValueType.STRING,
                        value: "delbatch_value_" + i,
                    }
                });
            }
            await kvStore.putBatch(entries);

            let keysToDelete = [];
            for (var i = 0; i < 5; i++) {
                keysToDelete.push("delbatch_key_" + i);
            }
            await kvStore.deleteBatch(keysToDelete);

            await kvStore.rekey();

            let remainingEntries = await kvStore.getEntries("delbatch_key_");
            expect(remainingEntries.length).assertEqual(5);
        } catch (e) {
            console.error(TAG + `fail: error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
        console.info(TAG + "************* SingleKvStoreRekeyDeleteBatchTest011 end *************");
    })

    /**
     * @tc.number SingleKvStoreRekeyThreeTimesTest012
     * @tc.name Test rekey three consecutive times
     * @tc.desc Rekey three times and verify data integrity after each
     * @tc.type FUNC
     */
    it('SingleKvStoreRekeyThreeTimesTest012', 0, async function (done) {
        console.info(TAG + "************* SingleKvStoreRekeyThreeTimesTest012 start *************");
        try {
            let kvStore = await kvManager.getKVStore(TEST_STORE_ID, encryptedOptions);
            expect(kvStore != null && kvStore != undefined).assertTrue();

            for (var i = 0; i < 3; i++) {
                await kvStore.put("three_key_" + i, "three_value_" + i);
                await kvStore.rekey();
            }

            for (var j = 0; j < 3; j++) {
                let value = await kvStore.get("three_key_" + j);
                expect("three_value_" + j).assertEqual(value);
            }
        } catch (e) {
            console.error(TAG + `fail: error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
        console.info(TAG + "************* SingleKvStoreRekeyThreeTimesTest012 end *************");
    })

    /**
     * @tc.number SingleKvStoreRekeyLargeValueTest013
     * @tc.name Test rekey with large string values
     * @tc.desc Put entries with large values, rekey, then verify
     * @tc.type FUNC
     */
    it('SingleKvStoreRekeyLargeValueTest013', 0, async function (done) {
        console.info(TAG + "************* SingleKvStoreRekeyLargeValueTest013 start *************");
        try {
            let kvStore = await kvManager.getKVStore(TEST_STORE_ID, encryptedOptions);
            expect(kvStore != null && kvStore != undefined).assertTrue();

            let largeValue = "a".repeat(1000);
            await kvStore.put("large_key", largeValue);
            await kvStore.rekey();

            let value = await kvStore.get("large_key");
            expect(largeValue).assertEqual(value);
        } catch (e) {
            console.error(TAG + `fail: error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
        console.info(TAG + "************* SingleKvStoreRekeyLargeValueTest013 end *************");
    })

    /**
     * @tc.number SingleKvStoreRekeyGetResultSetTest014
     * @tc.name Test rekey and verify data via ResultSet
     * @tc.desc Rekey then use getResultSet to verify data count
     * @tc.type FUNC
     */
    it('SingleKvStoreRekeyGetResultSetTest014', 0, async function (done) {
        console.info(TAG + "************* SingleKvStoreRekeyGetResultSetTest014 start *************");
        try {
            let kvStore = await kvManager.getKVStore(TEST_STORE_ID, encryptedOptions);
            expect(kvStore != null && kvStore != undefined).assertTrue();

            for (var i = 0; i < 10; i++) {
                await kvStore.put("rs_key_" + i, "rs_value_" + i);
            }
            await kvStore.rekey();

            let resultSet = await kvStore.getResultSet("rs_key_");
            expect(resultSet.getCount()).assertEqual(10);
            await kvStore.closeResultSet(resultSet);
        } catch (e) {
            console.error(TAG + `fail: error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
        console.info(TAG + "************* SingleKvStoreRekeyGetResultSetTest014 end *************");
    })

    /**
     * @tc.number SingleKvStoreRekeyCallbackMultipleDataTest015
     * @tc.name Test rekey callback with multiple data entries
     * @tc.desc Put multiple entries, rekey via callback, verify all data
     * @tc.type FUNC
     */
    it('SingleKvStoreRekeyCallbackMultipleDataTest015', 0, async function (done) {
        console.info(TAG + "************* SingleKvStoreRekeyCallbackMultipleDataTest015 start *************");
        try {
            let kvStore = await kvManager.getKVStore(TEST_STORE_ID, encryptedOptions);
            expect(kvStore != null && kvStore != undefined).assertTrue();

            await kvStore.put("cb_key1", "cb_value1");
            await kvStore.put("cb_key2", "cb_value2");
            await kvStore.put("cb_key3", "cb_value3");

            kvStore.rekey(function (err) {
                if (err) {
                    console.error(TAG + `rekey callback err: code=${err.code}`);
                    expect(null).assertFail();
                    done();
                    return;
                }
                kvStore.get("cb_key1", function (err, value) {
                    if (err) {
                        expect(null).assertFail();
                        done();
                        return;
                    }
                    expect("cb_value1").assertEqual(value);
                    kvStore.get("cb_key2", function (err, value) {
                        if (err) {
                            expect(null).assertFail();
                            done();
                            return;
                        }
                        expect("cb_value2").assertEqual(value);
                        kvStore.get("cb_key3", function (err, value) {
                            if (err) {
                                expect(null).assertFail();
                                done();
                                return;
                            }
                            expect("cb_value3").assertEqual(value);
                            done();
                        });
                    });
                });
            });
        } catch (e) {
            console.error(TAG + `fail: error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
            done();
        }
        console.info(TAG + "************* SingleKvStoreRekeyCallbackMultipleDataTest015 end *************");
    })

    /**
     * @tc.number SingleKvStoreRekeyPutBatchVerifyValuesTest016
     * @tc.name Test rekey with batch data and verify individual values
     * @tc.desc PutBatch, rekey, then get each entry individually to verify
     * @tc.type FUNC
     */
    it('SingleKvStoreRekeyPutBatchVerifyValuesTest016', 0, async function (done) {
        console.info(TAG + "************* SingleKvStoreRekeyPutBatchVerifyValuesTest016 start *************");
        try {
            let kvStore = await kvManager.getKVStore(TEST_STORE_ID, encryptedOptions);
            expect(kvStore != null && kvStore != undefined).assertTrue();

            let entries = [];
            for (var i = 0; i < 10; i++) {
                entries.push({
                    key: "verify_key_" + i,
                    value: {
                        type: factory.ValueType.STRING,
                        value: "verify_value_" + i,
                    }
                });
            }
            await kvStore.putBatch(entries);
            await kvStore.rekey();

            for (var i = 0; i < 10; i++) {
                let value = await kvStore.get("verify_key_" + i);
                expect("verify_value_" + i).assertEqual(value);
            }
        } catch (e) {
            console.error(TAG + `fail: error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
        console.info(TAG + "************* SingleKvStoreRekeyPutBatchVerifyValuesTest016 end *************");
    })

    /**
     * @tc.number SingleKvStoreRekeyBeforeAndAfterPutTest017
     * @tc.name Test rekey before and after putting data
     * @tc.desc Rekey empty store, put data, rekey again, verify data
     * @tc.type FUNC
     */
    it('SingleKvStoreRekeyBeforeAndAfterPutTest017', 0, async function (done) {
        console.info(TAG + "************* SingleKvStoreRekeyBeforeAndAfterPutTest017 start *************");
        try {
            let kvStore = await kvManager.getKVStore(TEST_STORE_ID, encryptedOptions);
            expect(kvStore != null && kvStore != undefined).assertTrue();

            await kvStore.rekey();

            await kvStore.put("mid_key1", "mid_value1");
            await kvStore.put("mid_key2", "mid_value2");

            await kvStore.rekey();

            let value = await kvStore.get("mid_key1");
            expect("mid_value1").assertEqual(value);
            value = await kvStore.get("mid_key2");
            expect("mid_value2").assertEqual(value);
        } catch (e) {
            console.error(TAG + `fail: error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
        console.info(TAG + "************* SingleKvStoreRekeyBeforeAndAfterPutTest017 end *************");
    })

    /**
     * @tc.number SingleKvStoreRekeyCallbackClosedStoreTest018
     * @tc.name Test rekey callback on closed store should fail
     * @tc.desc Close the store then call rekey with callback, expect error
     * @tc.type FUNC
     */
    it('SingleKvStoreRekeyCallbackClosedStoreTest018', 0, async function (done) {
        console.info(TAG + "************* SingleKvStoreRekeyCallbackClosedStoreTest018 start *************");
        try {
            let kvStore = await kvManager.getKVStore(TEST_STORE_ID, encryptedOptions);
            expect(kvStore != null && kvStore != undefined).assertTrue();

            await kvManager.closeKVStore(TEST_BUNDLE_NAME, TEST_STORE_ID);
            kvStore.rekey(function (err) {
                if (err) {
                    console.info(TAG + `expected callback error on closed store: code=${err.code}`);
                    expect(err.code != undefined).assertTrue();
                    done();
                    return;
                }
                expect(null).assertFail();
                done();
            });
        } catch (e) {
            console.info(TAG + `expected error on closed store: code=${e.code}, message=${e.message}`);
            expect(e.code != undefined).assertTrue();
            done();
        }
        console.info(TAG + "************* SingleKvStoreRekeyCallbackClosedStoreTest018 end *************");
    })

    /**
     * @tc.number SingleKvStoreRekeyWithGetEntriesVerifyTest019
     * @tc.name Test rekey preserves all entry values accessible by getEntries
     * @tc.desc Put entries, rekey, use getEntries to verify all values
     * @tc.type FUNC
     */
    it('SingleKvStoreRekeyWithGetEntriesVerifyTest019', 0, async function (done) {
        console.info(TAG + "************* SingleKvStoreRekeyWithGetEntriesVerifyTest019 start *************");
        try {
            let kvStore = await kvManager.getKVStore(TEST_STORE_ID, encryptedOptions);
            expect(kvStore != null && kvStore != undefined).assertTrue();

            let entries = [];
            for (var i = 0; i < 15; i++) {
                entries.push({
                    key: "ent_key_" + i,
                    value: {
                        type: factory.ValueType.STRING,
                        value: "ent_value_" + i,
                    }
                });
            }
            await kvStore.putBatch(entries);
            await kvStore.rekey();

            let resultEntries = await kvStore.getEntries("ent_key_");
            expect(resultEntries.length).assertEqual(15);

            for (var i = 0; i < resultEntries.length; i++) {
                expect(resultEntries[i].key.indexOf("ent_key_")).assertEqual(0);
                expect(resultEntries[i].value.value.indexOf("ent_value_")).assertEqual(0);
            }
        } catch (e) {
            console.error(TAG + `fail: error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
        console.info(TAG + "************* SingleKvStoreRekeyWithGetEntriesVerifyTest019 end *************");
    })
})
