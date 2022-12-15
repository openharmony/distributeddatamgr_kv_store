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
import abilityFeatureAbility from '@ohos.ability.featureAbility';

var context = abilityFeatureAbility.getContext();
const TEST_BUNDLE_NAME = 'com.example.myapplication';
const TEST_STORE_ID = 'storeId';
var kvManager = null;
var kvStore = null;
var resultSet = null;

describe('KvStoreResultSetTest', function () {
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
        console.info('beforeAll');
        console.info('beforeAll config:' + JSON.stringify(config));
        kvManager =  factory.createKVManager(config);
        await kvManager.getAllKVStoreId(TEST_BUNDLE_NAME).then(async (data) => {
            console.info('beforeAll getAllKVStoreId size = ' + data.length);
            for (var i = 0; i < data.length; i++) {
                await kvManager.deleteKVStore(TEST_BUNDLE_NAME, data[i]).then(() => {
                    console.info('beforeAll deleteKVStore success ' + data[i]);
                }).catch((err) => {
                    console.info('beforeAll deleteKVStore store: ' + data[i]);
                    console.error('beforeAll deleteKVStore error ' + `, error code is ${err.code}, message is ${err.message}`);
                });
            }
        }).catch((err) => {
            console.error('beforeAll getAllKVStoreId err ' + `, error code is ${err.code}, message is ${err.message}`);
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
        console.info('beforeEach');
        await kvManager.getKVStore(TEST_STORE_ID, options).then((store) => {
            kvStore = store;
            console.info('beforeEach getKVStore success');
        }).catch((err) => {
            console.error('beforeEach getKVStore err ' + `, error code is ${err.code}, message is ${err.message}`);
        });
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
        await kvStore.putBatch(entries).then(async (err) => {
            console.info('beforeEach putBatch success');
        }).catch((err) => {
            console.error('beforeEach putBatch fail ' + `, error code is ${err.code}, message is ${err.message}`);
        });
        await kvStore.getResultSet('batch_test_string_key').then((result) => {
            console.info('beforeEach getResultSet success');
            resultSet = result;
        }).catch((err) => {
            console.error('beforeEach getResultSet fail ' + `, error code is ${err.code}, message is ${err.message}`);
        });
        console.info('beforeEach end');
        done();
    })

    afterEach(async function (done) {
        console.info('afterEach');
        await kvStore.closeResultSet(resultSet).then((err) => {
            console.info('afterEach closeResultSet success');
        }).catch((err) => {
            console.error('afterEach closeResultSet fail ' + `, error code is ${err.code}, message is ${err.message}`);
        });
        resultSet = null;
        await kvManager.closeKVStore(TEST_BUNDLE_NAME, TEST_STORE_ID).then(async () => {
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
     * @tc.name KvStoreResultSetGetCountSucTest
     * @tc.desc Test Js Api KvStoreResultSet.GetCount() successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreResultSetGetCountSucTest', 0, async function (done) {
        try {
            var count = resultSet.getCount();
            expect(count == 10).assertTrue();
        } catch (e) {
            console.error("KvStoreResultSetGetCountTest001 fail " + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name KvStoreResultSetGetCountNullSetTest
     * @tc.desc Test Js Api KvStoreResultSet.GetCount() from a null resultset
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreResultSetGetCountNullSetTest', 0, async function (done) {
        try {
            var rs;
            await kvStore.getResultSet('test').then((result) => {
                rs = result;
                expect(rs.getCount() == 0).assertTrue();
            }).catch((err) => {
                console.error('KvStoreResultSetGetCountNullSetTest getResultSet fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
            await kvStore.closeResultSet(rs).then((err) => {
            }).catch((err) => {
                console.error('KvStoreResultSetGetCountNullSetTest closeResultSet fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('KvStoreResultSetGetCountNullSetTest e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name KvStoreResultSetGetPositionSucTest
     * @tc.desc Test Js Api KvStoreResultSet.GetPosition() successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreResultSetGetPositionSucTest', 0, async function (done) {
        try {
            var position = resultSet.getPosition();
            console.info("KvStoreResultSetGetPositionSucTest getPosition " + position);
            expect(position == -1).assertTrue();
        } catch (e) {
            console.error("KvStoreResultSetGetPositionSucTest fail " + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name KvStoreResultSetGetPositionMoveToLastTest
     * @tc.desc Test Js Api KvStoreResultSet.GetPosition() after move to last
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreResultSetGetPositionMoveToLastTest', 0, async function (done) {
        try {
            var position = resultSet.getPosition();
            console.info("KvStoreResultSetGetPositionMoveToLastTest getPosition " + position);
            expect(position).assertEqual(-1);
            var flag = resultSet.moveToLast();
            expect(flag).assertTrue();
            position = resultSet.getPosition();
            expect(position).assertEqual(9);
        } catch (e) {
            console.error("KvStoreResultSetGetPositionMoveToLastTest fail " + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name KvStoreResultSetMoveToFirstSucTest
     * @tc.desc Test Js Api KvStoreResultSet.MoveToFirst() successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreResultSetMoveToFirstSucTest', 0, async function (done) {
        try {
            var moved = resultSet.moveToFirst();
            console.info("KvStoreResultSetMoveToFirstSucTest moveToFirst " + moved);
            expect(moved).assertTrue();
            var pos = resultSet.getPosition();
            console.info("KvStoreResultSetMoveToFirstSucTest getPosition " + pos);
            expect(pos == 0).assertTrue();
        } catch (e) {
            expect(null).assertFail();
            console.error("KvStoreResultSetMoveToFirstSucTest fail " + `, error code is ${e.code}, message is ${e.message}`);
        }
        done();
    })

    /**
     * @tc.name KvStoreResultSetMoveToFirstSucTest
     * @tc.desc Test Js Api KvStoreResultSet.MoveToFirst() success
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreResultSetMoveToFirstSucTest', 0, async function (done) {
        try {
            var moved = resultSet.moveToLast();
            console.info("KvStoreResultSetMoveToFirstSucTest moveToFirst " + moved);
            expect(moved && (resultSet.getPosition() == 9)).assertTrue();
            moved = resultSet.moveToFirst();
            expect(moved && (resultSet.getPosition() == 0)).assertTrue();
        } catch (e) {
            console.error("KvStoreResultSetMoveToFirstSucTest fail " + `, error code is ${e.code}, message is ${e.message}`);
        }
        done();
    })

    /**
     * @tc.name KvStoreResultSetMoveToLastSucTest
     * @tc.desc Test Js Api KvStoreResultSet.MoveToLast() successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreResultSetMoveToLastSucTest', 0, async function (done) {
        try {
            var moved = resultSet.moveToLast();
            console.info("KvStoreResultSetMoveToLastSucTest moveToLast " + moved);
            expect(moved && (resultSet.getPosition() == 9)).assertTrue();
        } catch (e) {
            expect(null).assertFail();
            console.error("KvStoreResultSetMoveToLastSucTest fail " + `, error code is ${e.code}, message is ${e.message}`);
        }
        done();
    })

    /**
     * @tc.name KvStoreResultSetMoveToNextSucTest
     * @tc.desc Test Js Api KvStoreResultSet.MoveToNext() successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreResultSetMoveToNextSucTest', 0, async function (done) {
        try {
            var moved = resultSet.moveToNext();
            console.info("KvStoreResultSetMoveToNextSucTest moveToNext " + moved);
            expect(moved && (resultSet.getPosition() == 0)).assertTrue();
            moved = resultSet.moveToNext();
            expect(moved && (resultSet.getPosition() == 1)).assertTrue();
        } catch (e) {
            expect(null).assertFail();
            console.error("KvStoreResultSetMoveToNextSucTest fail " + `, error code is ${e.code}, message is ${e.message}`);
        }
        done();
    })

    /**
     * @tc.name KvStoreResultSetMoveToPreviousSucTest
     * @tc.desc Test Js Api KvStoreResultSet.MoveToPrevious() successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreResultSetMoveToPreviousSucTest', 0, async function (done) {
        try {
            var moved = resultSet.moveToFirst();
            expect(moved && (resultSet.getPosition() == 0)).assertTrue();
            moved = resultSet.moveToNext();
            console.info("KvStoreResultSetMoveToPreviousSucTest moveToNext " + moved);
            expect(moved && (resultSet.getPosition() == 1)).assertTrue();
            moved = resultSet.moveToPrevious();
            console.info("KvStoreResultSetMoveToPreviousSucTest moveToPrevious " + moved);
            expect(moved && (resultSet.getPosition() == 0)).assertTrue();
        } catch (e) {
            console.error("KvStoreResultSetMoveToPreviousSucTest fail " + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name KvStoreResultSetMoveSucTest
     * @tc.desc Test Js Api KvStoreResultSet.Move() successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreResultSetMoveSucTest', 0, async function (done) {
        try {
            resultSet.moveToFirst();
            expect(resultSet.getPosition() == 0).assertTrue();
            var moved = resultSet.move(3);
            console.info("KvStoreResultSetMoveSucTest move " + moved);
            expect(moved).assertTrue();
            expect(3).assertEqual(resultSet.getPosition());
        } catch (e) {
            console.error("KvStoreResultSetMoveSucTest fail " + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name KvStoreResultSetMoveInvalidArgsTest
     * @tc.desc Test Js Api KvStoreResultSet.Move() with invalid args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreResultSetMoveInvalidArgsTest', 0, async function (done) {
        try {
            var moved = resultSet.move();
            console.info("KvStoreResultSetMoveInvalidArgsTest move " + moved);
            expect(null).assertFail();
        } catch (e) {
            console.error("KvStoreResultSetMoveInvalidArgsTest fail " + `, error code is ${e.code}, message is ${e.message}`);
            expect(e.code == 401).assertTrue();
        }
        done();
    })

    /**
     * @tc.name KvStoreResultSetMoveToPositionWithoutArgsTest
     * @tc.desc Test Js Api KvStoreResultSet.MoveToPosition() without args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreResultSetMoveToPositionWithoutArgsTest', 0, async function (done) {
        try {
            var moved = resultSet.moveToPosition();
            console.info("KvStoreResultSetMoveToPositionWithoutArgsTest moveToPosition " + moved);
            expect(null).assertFail();
        } catch (e) {
            console.error("KvStoreResultSetMoveToPositionWithoutArgsTest fail " + `, error code is ${e.code}, message is ${e.message}`);
            expect(e.code == 401).assertTrue();
        }
        done();
    })

    /**
     * @tc.name KvStoreResultSetMoveToPositionSucTest
     * @tc.desc Test Js Api KvStoreResultSet.MoveToPosition() successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreResultSetMoveToPositionSucTest', 0, async function (done) {
        try {
            var moved = resultSet.moveToPosition(5);
            console.info("KvStoreResultSetMoveToPositionSucTest moveToPosition " + moved);
            expect(moved && (resultSet.getPosition() == 5)).assertTrue();
        } catch (e) {
            console.error("KvStoreResultSetMoveToPositionSucTest fail " + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name KvStoreResultSetIsFirstSucTest
     * @tc.desc Test Js Api KvStoreResultSet.IsFirst() successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreResultSetIsFirstSucTest', 0, async function (done) {
        try {
            var flag = resultSet.isFirst();
            console.info("KvStoreResultSetIsFirstSucTest isFirst " + flag);
            expect(!flag).assertTrue();
        } catch (e) {
            expect(null).assertFail();
            console.error("KvStoreResultSetIsFirstSucTest fail " + `, error code is ${e.code}, message is ${e.message}`);
        }
        done();
    })

    /**
     * @tc.name KvStoreResultSetIsFirstAfterMoveTest
     * @tc.desc Test Js Api KvStoreResultSet.IsFirst() after move
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreResultSetIsFirstAfterMoveTest', 0, async function (done) {
        try {
            var flag = resultSet.isFirst();
            console.info("KvStoreResultSetIsFirstAfterMoveTest isFirst " + flag);
            expect(!flag).assertTrue();
            resultSet.move(3);
            flag = resultSet.isFirst();
            console.info("KvStoreResultSetIsFirstAfterMoveTest isFirst " + flag);
            expect(!flag).assertTrue();
        } catch (e) {
            expect(null).assertFail();
            console.error("KvStoreResultSetIsFirstAfterMoveTest fail " + `, error code is ${e.code}, message is ${e.message}`);
        }
        done();
    })

    /**
     * @tc.name KvStoreResultSetIsLastFailTest
     * @tc.desc Test Js Api KvStoreResultSet.IsLast() fail
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreResultSetIsLastFailTest', 0, async function (done) {
        try {
            var flag = resultSet.isLast();
            console.info("KvStoreResultSetIsLastFailTest isLast " + flag);
            expect(!flag).assertTrue();
        } catch (e) {
            expect(null).assertFail();
            console.error("KvStoreResultSetIsLastFailTest fail " + `, error code is ${e.code}, message is ${e.message}`);
        }
        done();
    })

    /**
     * @tc.name KvStoreResultSetIsLastSucTest
     * @tc.desc Test Js Api KvStoreResultSet.IsLast() successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreResultSetIsLastSucTest', 0, async function (done) {
        try {
            resultSet.moveToLast();
            var flag = resultSet.isLast();
            console.info("KvStoreResultSetIsLastSucTest isLast " + flag);
            expect(flag).assertTrue();
        } catch (e) {
            expect(null).assertFail();
            console.error("KvStoreResultSetIsLastSucTest fail " + `, error code is ${e.code}, message is ${e.message}`);
        }
        done();
    })

    /**
     * @tc.name KvStoreResultSetIsBeforeFirstSucTest
     * @tc.desc Test Js Api KvStoreResultSet.IsBeforeFirst() successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreResultSetIsBeforeFirstSucTest', 0, async function (done) {
        try {
            var flag = resultSet.isBeforeFirst();
            console.info("KvStoreResultSetIsBeforeFirstSucTest isBeforeFirst " + flag);
            expect(flag).assertTrue();
        } catch (e) {
            expect(null).assertFail();
            console.error("KvStoreResultSetIsBeforeFirstSucTest fail " + `, error code is ${e.code}, message is ${e.message}`);
        }
        done();
    })

    /**
     * @tc.name KvStoreResultSetIsAfterLastSucTest
     * @tc.desc Test Js Api KvStoreResultSet.IsAfterLast() success
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreResultSetIsAfterLastSucTest', 0, async function (done) {
        try {
            var moved = resultSet.moveToLast();
            console.info("KvStoreResultSetIsAfterLastSucTest  moveToLast  moved=" + moved);
            expect(moved).assertTrue();
            moved = resultSet.moveToNext();
            console.info("KvStoreResultSetIsAfterLastSucTest  moveToNext  moved=" + moved);
            expect(moved == false).assertTrue();
            var flag = resultSet.isAfterLast();
            console.info("KvStoreResultSetIsAfterLastSucTest  isAfterLast true=" + flag);
            expect(flag).assertTrue();
        } catch (e) {
            console.error("KvStoreResultSetIsAfterLastSucTest fail " + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name KvStoreResultSetGetEntrySucTest
     * @tc.desc Test Js Api KvStoreResultSet.GetEntry() successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreResultSetGetEntrySucTest', 0, async function (done) {
        try {
            var moved = resultSet.moveToNext();
            var entry = resultSet.getEntry();
            console.info("KvStoreResultSetGetEntrySucTest getEntry " + entry);
            expect(entry.key == 'batch_test_string_key0').assertTrue();
            expect(entry.value.value == 'batch_test_string_value').assertTrue();
            moved = resultSet.moveToNext();
            expect(moved).assertTrue();
            entry = resultSet.getEntry();
            console.info("KvStoreResultSetGetEntrySucTest getEntry " + entry);
            expect(entry.key == 'batch_test_string_key1').assertTrue();
            expect(entry.value.value == 'batch_test_string_value').assertTrue();
        } catch (e) {
            expect(null).assertFail();
            console.error("KvStoreResultSetGetEntrySucTest fail " + `, error code is ${e.code}, message is ${e.message}`);
        }
        done();
    })
})
