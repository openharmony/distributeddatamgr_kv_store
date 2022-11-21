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
import { describe, beforeAll, beforeEach, afterEach, afterAll, it, expect } from 'deccjsunit/index'
import factory from '@ohos.data.distributedData';
import abilityFeatureAbility from '@ohos.ability.featureAbility';

var context = abilityFeatureAbility.getContext();
const TEST_BUNDLE_NAME = 'com.example.myapplication';
const TEST_STORE_ID = 'storeId';
var kvManager = null;
var kvStore = null;
var resultSet = null;

describe('kvStoreResultSetTest', function() {
    const config = {
        bundleName : TEST_BUNDLE_NAME,
        userInfo : {
            userId : '0',
            userType : factory.UserType.SAME_USER_ID
        },
        context:context
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
        console.info('beforeAll');
        console.info('beforeAll config:' + JSON.stringify(config));
        await factory.createKVManager(config).then((manager) => {
            kvManager = manager;
            console.info('beforeAll createKVManager success');
        }).catch((err) => {
            console.error('beforeAll createKVManager err ' + `, error code is ${err.code}, message is ${err.message}`);
        });
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
                key : key + i,
                value : {
                    type : factory.ValueType.STRING,
                    value : 'batch_test_string_value'
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
        await kvManager.closeKVStore(TEST_BUNDLE_NAME, TEST_STORE_ID, kvStore).then(async () => {
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
        resultSet = null;
        done();
    })

    /**
     * @tc.name KvStoreResultSetGetCountTest001
     * @tc.desc Test Js Api KvStoreResultSet.GetCount() testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreResultSetGetCountTest001', 0, async function(done) {
        try {
            var count = resultSet.getCount();
            console.info("KvStoreResultSetGetCountTest001 getCount " + count);
            expect(count == 10).assertTrue();
        } catch (e) {
            console.error("KvStoreResultSetGetCountTest001 fail " + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name KvStoreResultSetGetCountTest002
     * @tc.desc Test Js Api KvStoreResultSet.GetCount() testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreResultSetGetCountTest002', 0, async function(done) {
        try {
            var rs;
            await kvStore.getResultSet('test').then((result) => {
                console.info('KvStoreResultSetGetCountTest002 getResultSet success');
                rs = result;
                expect(rs.getCount() == 0).assertTrue();
            }).catch((err) => {
                console.error('KvStoreResultSetGetCountTest002 getResultSet fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
            await kvStore.closeResultSet(rs).then((err) => {
                console.info('KvStoreResultSetGetCountTest002 closeResultSet success');
            }).catch((err) => {
                console.error('KvStoreResultSetGetCountTest002 closeResultSet fail ' + `, error code is ${err.code}, message is ${err.message}`);
                expect(null).assertFail();
            });
        } catch (e) {
            console.error('KvStoreResultSetGetCountTest002 e ' + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name KvStoreResultSetGetCountTest003
     * @tc.desc Test Js Api KvStoreResultSet.GetCount() testcase 003
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreResultSetGetCountTest003', 0, async function(done) {
        try {
            var count = resultSet.getCount(123);
            console.info("KvStoreResultSetGetCountTest003 getCount " + count);
            expect(null).assertFail();
        } catch (e) {
            console.error("KvStoreResultSetGetCountTest003 fail " + `, error code is ${e.code}, message is ${e.message}`);
        }
        done();
    })

    /**
     * @tc.name KvStoreResultSetGetCountTest004
     * @tc.desc Test Js Api KvStoreResultSet.GetCount() testcase 004
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreResultSetGetCountTest004', 0, async function(done) {
        try {
            var count = resultSet.getCount(123, 'test_string');
            console.info("KvStoreResultSetGetCountTest004 getCount " + count);
            expect(null).assertFail();
        } catch (e) {
            console.error("KvStoreResultSetGetCountTest004 fail " + `, error code is ${e.code}, message is ${e.message}`);
        }
        done();
    })

    /**
     * @tc.name KvStoreResultSetGetPositionTest001
     * @tc.desc Test Js Api KvStoreResultSet.GetPosition() testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreResultSetGetPositionTest001', 0, async function(done) {
        try {
            var position = resultSet.getPosition();
            console.info("KvStoreResultSetGetPositionTest001 getPosition " + position);
            expect(position == -1).assertTrue();
        } catch (e) {
            console.error("KvStoreResultSetGetPositionTest001 fail " + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name KvStoreResultSetGetPositionTest002
     * @tc.desc Test Js Api KvStoreResultSet.GetPosition() testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreResultSetGetPositionTest002', 0, async function(done) {
        try {
            var position = resultSet.getPosition();
            console.info("KvStoreResultSetGetPositionTest002 getPosition " + position);
            expect(position).assertEqual(-1);
            var flag = resultSet.moveToLast();
            expect(flag).assertTrue();
            position = resultSet.getPosition();
            expect(position).assertEqual(9);
        } catch (e) {
            console.error("KvStoreResultSetGetPositionTest002 fail " + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name KvStoreResultSetGetPositionTest003
     * @tc.desc Test Js Api KvStoreResultSet.GetPosition() testcase 003
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreResultSetGetPositionTest003', 0, async function(done) {
        try {
            var position = resultSet.getPosition(123);
            console.info("KvStoreResultSetGetPositionTest003 getPosition " + position);
            expect(null).assertFail();
        } catch (e) {
            console.error("KvStoreResultSetGetPositionTest003 fail " + `, error code is ${e.code}, message is ${e.message}`);
        }
        done();
    })

    /**
     * @tc.name KvStoreResultSetGetPositionTest004
     * @tc.desc Test Js Api KvStoreResultSet.GetPosition() testcase 004
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreResultSetGetPositionTest004', 0, async function(done) {
        try {
            var position = resultSet.getPosition(123, 'test_string');
            console.info("KvStoreResultSetGetPositionTest004 getPosition " + position);
            expect(null).assertFail();
        } catch (e) {
            console.error("KvStoreResultSetGetPositionTest004 fail " + `, error code is ${e.code}, message is ${e.message}`);
        }
        done();
    })

    /**
     * @tc.name KvStoreResultSetMoveToFirstTest001
     * @tc.desc Test Js Api KvStoreResultSet.MoveToFirst() testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreResultSetMoveToFirstTest001', 0, async function(done) {
        try {
            var moved = resultSet.moveToFirst();
            console.info("KvStoreResultSetMoveToFirstTest001 moveToFirst " + moved);
            expect(moved).assertTrue();
        } catch (e) {
            expect(null).assertFail();
            console.error("KvStoreResultSetMoveToFirstTest001 fail " + `, error code is ${e.code}, message is ${e.message}`);
        }
        done();
    })

    /**
     * @tc.name KvStoreResultSetMoveToFirstTest002
     * @tc.desc Test Js Api KvStoreResultSet.MoveToFirst() testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreResultSetMoveToFirstTest002', 0, async function(done) {
        try {
            var moved = resultSet.moveToFirst();
            console.info("KvStoreResultSetMoveToFirstTest002 moveToFirst " + moved);
            expect(moved).assertTrue();
            var pos = resultSet.getPosition();
            console.info("KvStoreResultSetMoveToFirstTest002 getPosition " + pos);
            expect(pos == 0).assertTrue();
        } catch (e) {
            expect(null).assertFail();
            console.error("KvStoreResultSetMoveToFirstTest002 fail " + `, error code is ${e.code}, message is ${e.message}`);
        }
        done();
    })

    /**
     * @tc.name KvStoreResultSetMoveToFirstTest003
     * @tc.desc Test Js Api KvStoreResultSet.MoveToFirst() testcase 003
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreResultSetMoveToFirstTest003', 0, async function(done) {
        try {
            var moved = resultSet.moveToFirst(123);
            console.info("KvStoreResultSetMoveToFirstTest003 moveToFirst " + moved);
            expect(null).assertFail();
        } catch (e) {
            console.error("KvStoreResultSetMoveToFirstTest003 fail " + `, error code is ${e.code}, message is ${e.message}`);
        }
        done();
    })

    /**
     * @tc.name KvStoreResultSetMoveToFirstTest004
     * @tc.desc Test Js Api KvStoreResultSet.MoveToFirst() testcase 004
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreResultSetMoveToFirstTest004', 0, async function(done) {
        try {
            var moved = resultSet.moveToFirst(123, 'test_string');
            console.info("KvStoreResultSetMoveToFirstTest004 moveToFirst " + moved);
            expect(null).assertFail();
        } catch (e) {
            console.error("KvStoreResultSetMoveToFirstTest004 fail " + `, error code is ${e.code}, message is ${e.message}`);
        }
        done();
    })

    /**
     * @tc.name KvStoreResultSetMoveToFirstTest005
     * @tc.desc Test Js Api KvStoreResultSet.MoveToFirst() testcase 005
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreResultSetMoveToFirstTest005', 0, async function(done) {
        try {
            var moved = resultSet.moveToLast();
            console.info("KvStoreResultSetMoveToFirstTest004 moveToFirst " + moved);
            expect(moved && (resultSet.getPosition() == 9)).assertTrue();
            moved = resultSet.moveToFirst();
            expect(moved && (resultSet.getPosition() == 0)).assertTrue();
        } catch (e) {
            console.error("KvStoreResultSetMoveToFirstTest004 fail " + `, error code is ${e.code}, message is ${e.message}`);
        }
        done();
    })

    /**
     * @tc.name KvStoreResultSetMoveToLastTest001
     * @tc.desc Test Js Api KvStoreResultSet.MoveToLast() testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreResultSetMoveToLastTest001', 0, async function(done) {
        try {
            var moved = resultSet.moveToLast();
            console.info("KvStoreResultSetMoveToLastTest001 moveToLast " + moved);
            expect(moved).assertTrue();
        } catch (e) {
            expect(null).assertFail();
            console.error("KvStoreResultSetMoveToLastTest001 fail " + `, error code is ${e.code}, message is ${e.message}`);
        }
        done();
    })

    /**
     * @tc.name KvStoreResultSetMoveToLastTest002
     * @tc.desc Test Js Api KvStoreResultSet.MoveToLast() testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreResultSetMoveToLastTest002', 0, async function(done) {
        try {
            var moved = resultSet.moveToLast();
            console.info("KvStoreResultSetMoveToLastTest002 moveToLast " + moved);
            expect(moved && (resultSet.getPosition() == 9)).assertTrue();
        } catch (e) {
            expect(null).assertFail();
            console.error("KvStoreResultSetMoveToLastTest002 fail " + `, error code is ${e.code}, message is ${e.message}`);
        }
        done();
    })

    /**
     * @tc.name KvStoreResultSetMoveToLastTest003
     * @tc.desc Test Js Api KvStoreResultSet.MoveToLast() testcase 003
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreResultSetMoveToLastTest003', 0, async function(done) {
        try {
            var moved = resultSet.moveToLast(123);
            console.info("KvStoreResultSetMoveToLastTest003 moveToLast " + moved);
            expect(null).assertFail();
        } catch (e) {
            console.error("KvStoreResultSetMoveToLastTest003 fail " + `, error code is ${e.code}, message is ${e.message}`);
        }
        done();
    })

    /**
     * @tc.name KvStoreResultSetMoveToLastTest004
     * @tc.desc Test Js Api KvStoreResultSet.MoveToLast() testcase 004
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreResultSetMoveToLastTest004', 0, async function(done) {
        try {
            var moved = resultSet.moveToLast(123, 'test_string');
            console.info("KvStoreResultSetMoveToLastTest004 moveToLast " + moved);
            expect(null).assertFail();
        } catch (e) {
            console.error("KvStoreResultSetMoveToLastTest004 fail " + `, error code is ${e.code}, message is ${e.message}`);
        }
        done();
    })

    /**
     * @tc.name KvStoreResultSetMoveToNextTest001
     * @tc.desc Test Js Api KvStoreResultSet.MoveToNext() testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreResultSetMoveToNextTest001', 0, async function(done) {
        try {
            var moved = resultSet.moveToNext();
            console.info("KvStoreResultSetMoveToNextTest001 moveToNext " + moved);
            expect(moved && (resultSet.getPosition() == 0)).assertTrue();
        } catch (e) {
            expect(null).assertFail();
            console.error("KvStoreResultSetMoveToNextTest001 fail " + `, error code is ${e.code}, message is ${e.message}`);
        }
        done();
    })

    /**
     * @tc.name KvStoreResultSetMoveToNextTest002
     * @tc.desc Test Js Api KvStoreResultSet.MoveToNext() testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreResultSetMoveToNextTest002', 0, async function(done) {
        try {
            var moved = resultSet.moveToNext();
            console.info("KvStoreResultSetMoveToNextTest002 moveToNext " + moved);
            expect(moved && (resultSet.getPosition() == 0)).assertTrue();
            moved = resultSet.moveToNext();
            expect(moved && (resultSet.getPosition() == 1)).assertTrue();
        } catch (e) {
            expect(null).assertFail();
            console.error("KvStoreResultSetMoveToNextTest002 fail " + `, error code is ${e.code}, message is ${e.message}`);
        }
        done();
    })

    /**
     * @tc.name KvStoreResultSetMoveToNextTest003
     * @tc.desc Test Js Api KvStoreResultSet.MoveToNext() testcase 003
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreResultSetMoveToNextTest003', 0, async function(done) {
        try {
            var moved = resultSet.moveToNext(123);
            console.info("KvStoreResultSetMoveToNextTest003 moveToNext " + moved);
            expect(null).assertFail();
        } catch (e) {
            console.error("KvStoreResultSetMoveToNextTest003 fail " + `, error code is ${e.code}, message is ${e.message}`);
        }
        done();
    })

    /**
     * @tc.name KvStoreResultSetMoveToNextTest004
     * @tc.desc Test Js Api KvStoreResultSet.MoveToNext() testcase 004
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreResultSetMoveToNextTest004', 0, async function(done) {
        try {
            var moved = resultSet.moveToNext(123, 'test_string');
            console.info("KvStoreResultSetMoveToNextTest004 moveToNext " + moved);
            expect(null).assertFail();
        } catch (e) {
            console.error("KvStoreResultSetMoveToNextTest004 fail " + `, error code is ${e.code}, message is ${e.message}`);
        }
        done();
    })

    /**
     * @tc.name KvStoreResultSetMoveToPreviousTest001
     * @tc.desc Test Js Api KvStoreResultSet.MoveToPrevious() testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreResultSetMoveToPreviousTest001', 0, async function(done) {
        try {
            var moved = resultSet.moveToPrevious();
            console.info("KvStoreResultSetMoveToPreviousTest001 moveToPrevious " + moved);
            expect(!moved).assertTrue();
        } catch (e) {
            expect(null).assertFail();
            console.error("KvStoreResultSetMoveToPreviousTest001 fail " + `, error code is ${e.code}, message is ${e.message}`);
        }
        done();
    })

    /**
     * @tc.name KvStoreResultSetMoveToPreviousTest002
     * @tc.desc Test Js Api KvStoreResultSet.MoveToPrevious() testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreResultSetMoveToPreviousTest002', 0, async function(done) {
        try {
            var moved = resultSet.moveToFirst();
            expect(moved && (resultSet.getPosition() == 0)).assertTrue();
            moved = resultSet.moveToNext();
            console.info("KvStoreResultSetMoveToPreviousTest002 moveToNext " + moved);
            expect(moved && (resultSet.getPosition() == 1)).assertTrue();
            moved = resultSet.moveToPrevious();
            console.info("KvStoreResultSetMoveToPreviousTest002 moveToPrevious " + moved);
            expect(moved && (resultSet.getPosition() == 0)).assertTrue();
        } catch (e) {
            console.error("KvStoreResultSetMoveToPreviousTest002 fail " + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name KvStoreResultSetMoveToPreviousTest003
     * @tc.desc Test Js Api KvStoreResultSet.MoveToPrevious() testcase 003
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreResultSetMoveToPreviousTest003', 0, async function(done) {
        try {
            var moved = resultSet.moveToPrevious(123);
            console.info("KvStoreResultSetMoveToPreviousTest003 moveToPrevious " + moved);
            expect(null).assertFail();
        } catch (e) {
            console.error("KvStoreResultSetMoveToPreviousTest003 fail " + `, error code is ${e.code}, message is ${e.message}`);
        }
        done();
    })

    /**
     * @tc.name KvStoreResultSetMoveToPreviousTest004
     * @tc.desc Test Js Api KvStoreResultSet.MoveToPrevious() testcase 004
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreResultSetMoveToPreviousTest004', 0, async function(done) {
        try {
            var moved = resultSet.moveToPrevious(123, 'test_string');
            console.info("KvStoreResultSetMoveToPreviousTest004 moveToPrevious " + moved);
            expect(null).assertFail();
        } catch (e) {
            console.error("KvStoreResultSetMoveToPreviousTest004 fail " + `, error code is ${e.code}, message is ${e.message}`);
        }
        done();
    })

    /**
     * @tc.name KvStoreResultSetMoveToPreviousTest005
     * @tc.desc Test Js Api KvStoreResultSet.MoveToPrevious() testcase 005
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreResultSetMoveToPreviousTest005', 0, async function(done) {
        try {
            var moved = resultSet.moveToFirst();
            expect(moved && (resultSet.getPosition() == 0)).assertTrue();
            moved = resultSet.moveToPrevious();
            console.info("KvStoreResultSetMoveToPreviousTest005 from 0 to -1 return" + moved);
            expect(moved == false).assertTrue();
            console.info("KvStoreResultSetMoveToPreviousTest005 from 0 to " + resultSet.getPosition());
            expect(-1).assertEqual(resultSet.getPosition());
        } catch (e) {
            console.error("KvStoreResultSetMoveToPreviousTest005 fail " + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name KvStoreResultSetMoveTest001
     * @tc.desc Test Js Api KvStoreResultSet.Move() testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreResultSetMoveTest001', 0, async function(done) {
        try {
            var moved = resultSet.move();
            console.info("KvStoreResultSetMoveTest001 move " + moved);
            expect(null).assertFail();
        } catch (e) {
            console.error("KvStoreResultSetMoveTest001 fail " + `, error code is ${e.code}, message is ${e.message}`);
        }
        done();
    })

    /**
     * @tc.name KvStoreResultSetMoveTest002
     * @tc.desc Test Js Api KvStoreResultSet.Move() testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreResultSetMoveTest002', 0, async function(done) {
        try {
            resultSet.moveToFirst();
            expect(resultSet.getPosition() == 0).assertTrue();
            var moved = resultSet.move(3);
            console.info("KvStoreResultSetMoveTest002 move " + moved);
            expect(moved).assertTrue();
            expect(3).assertEqual(resultSet.getPosition());
        } catch (e) {
            console.error("KvStoreResultSetMoveTest002 fail " + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name KvStoreResultSetMoveTest003
     * @tc.desc Test Js Api KvStoreResultSet.Move() testcase 003
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreResultSetMoveTest003', 0, async function(done) {
        try {
            expect(resultSet.getPosition() == -1).assertTrue();
            var moved = resultSet.move(3);
            console.info("KvStoreResultSetMoveTest003 move " + moved);
            expect(moved && (resultSet.getPosition() == 2)).assertTrue();
            moved = resultSet.move(2);
            console.info("KvStoreResultSetMoveTest003 move " + moved);
            expect(moved && (resultSet.getPosition() == 4)).assertTrue();
        } catch (e) {
            console.error("KvStoreResultSetMoveTest003 fail " + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name KvStoreResultSetMoveTest004
     * @tc.desc Test Js Api KvStoreResultSet.Move() testcase 004
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreResultSetMoveTest004', 0, async function(done) {
        try {
            var moved = resultSet.move(3, 'test_string');
            console.info("KvStoreResultSetMoveTest004 move " + moved);
            expect(null).assertFail();
        } catch (e) {
            console.error("KvStoreResultSetMoveTest004 fail " + `, error code is ${e.code}, message is ${e.message}`);
        }
        done();
    })

    /**
     * @tc.name KvStoreResultSetMoveToPositionTest001
     * @tc.desc Test Js Api KvStoreResultSet.MoveToPosition() testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreResultSetMoveToPositionTest001', 0, async function(done) {
        try {
            var moved = resultSet.moveToPosition();
            console.info("KvStoreResultSetMoveToPositionTest001 moveToPosition " + moved);
            expect(null).assertFail();
        } catch (e) {
            console.error("KvStoreResultSetMoveToPositionTest001 fail " + `, error code is ${e.code}, message is ${e.message}`);
        }
        done();
    })

    /**
     * @tc.name KvStoreResultSetMoveToPositionTest002
     * @tc.desc Test Js Api KvStoreResultSet.MoveToPosition() testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreResultSetMoveToPositionTest002', 0, async function(done) {
        try {
            var moved = resultSet.moveToPosition(1, 'test_string');
            console.info("KvStoreResultSetMoveToPositionTest002 moveToPosition " + moved);
            expect(null).assertFail();
        } catch (e) {
            console.error("KvStoreResultSetMoveToPositionTest002 fail " + `, error code is ${e.code}, message is ${e.message}`);
        }
        done();
    })

    /**
     * @tc.name KvStoreResultSetMoveToPositionTest003
     * @tc.desc Test Js Api KvStoreResultSet.MoveToPosition() testcase 003
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreResultSetMoveToPositionTest003', 0, async function(done) {
        try {
            var moved = resultSet.moveToPosition(5);
            console.info("KvStoreResultSetMoveToPositionTest003 moveToPosition " + moved);
            expect(moved && (resultSet.getPosition() == 5)).assertTrue();
        } catch (e) {
            console.error("KvStoreResultSetMoveToPositionTest003 fail " + `, error code is ${e.code}, message is ${e.message}`);
        }
        done();
    })

    /**
     * @tc.name KvStoreResultSetMoveToPositionTest004
     * @tc.desc Test Js Api KvStoreResultSet.MoveToPosition() testcase 004
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreResultSetMoveToPositionTest004', 0, async function(done) {
        try {
            var moved = resultSet.move(3);
            console.info("KvStoreResultSetMoveToPositionTest004 moveToPosition " + moved);
            expect(moved && (resultSet.getPosition() == 2)).assertTrue();
            moved = resultSet.moveToPosition(5);
            console.info("KvStoreResultSetMoveToPositionTest004 moveToPosition " + moved);
            expect(moved && (resultSet.getPosition() == 5)).assertTrue();
        } catch (e) {
            console.error("KvStoreResultSetMoveToPositionTest004 fail " + `, error code is ${e.code}, message is ${e.message}`);
        }
        done();
    })

    /**
     * @tc.name KvStoreResultSetIsFirstTest001
     * @tc.desc Test Js Api KvStoreResultSet.IsFirst() testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreResultSetIsFirstTest001', 0, async function(done) {
        try {
            var flag = resultSet.isFirst();
            console.info("KvStoreResultSetIsFirstTest001 isFirst " + flag);
            expect(!flag).assertTrue();
        } catch (e) {
            expect(null).assertFail();
            console.error("KvStoreResultSetIsFirstTest001 fail " + `, error code is ${e.code}, message is ${e.message}`);
        }
        done();
    })

    /**
     * @tc.name KvStoreResultSetIsFirstTest002
     * @tc.desc Test Js Api KvStoreResultSet.IsFirst() testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreResultSetIsFirstTest002', 0, async function(done) {
        try {
            var flag = resultSet.isFirst();
            console.info("KvStoreResultSetIsFirstTest002 isFirst " + flag);
            expect(!flag).assertTrue();
            resultSet.move(3);
            flag = resultSet.isFirst();
            console.info("KvStoreResultSetIsFirstTest002 isFirst " + flag);
            expect(!flag).assertTrue();
        } catch (e) {
            expect(null).assertFail();
            console.error("KvStoreResultSetIsFirstTest002 fail " + `, error code is ${e.code}, message is ${e.message}`);
        }
        done();
    })

    /**
     * @tc.name KvStoreResultSetIsFirstTest003
     * @tc.desc Test Js Api KvStoreResultSet.IsFirst() testcase 003
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreResultSetIsFirstTest003', 0, async function(done) {
        try {
            var flag = resultSet.isFirst(1);
            console.info("KvStoreResultSetIsFirstTest003 isFirst " + flag);
            expect(null).assertFail();
        } catch (e) {
            console.error("KvStoreResultSetIsFirstTest003 fail " + `, error code is ${e.code}, message is ${e.message}`);
        }
        done();
    })

    /**
     * @tc.name KvStoreResultSetIsFirstTest004
     * @tc.desc Test Js Api KvStoreResultSet.IsFirst() testcase 004
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreResultSetIsFirstTest004', 0, async function(done) {
        try {
            var flag = resultSet.isFirst(1, 'test_string');
            console.info("KvStoreResultSetIsFirstTest004 isFirst " + flag);
            expect(null).assertFail();
        } catch (e) {
            console.error("KvStoreResultSetIsFirstTest004 fail " + `, error code is ${e.code}, message is ${e.message}`);
        }
        done();
    })

    /**
     * @tc.name KvStoreResultSetIsLastTest001
     * @tc.desc Test Js Api KvStoreResultSet.IsLast() testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreResultSetIsLastTest001', 0, async function(done) {
        try {
            var flag = resultSet.isLast();
            console.info("KvStoreResultSetIsLastTest001 isLast " + flag);
            expect(!flag).assertTrue();
        } catch (e) {
            expect(null).assertFail();
            console.error("KvStoreResultSetIsFirstTest004 fail " + `, error code is ${e.code}, message is ${e.message}`);
        }
        done();
    })

    /**
     * @tc.name KvStoreResultSetIsLastTest002
     * @tc.desc Test Js Api KvStoreResultSet.IsLast() testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreResultSetIsLastTest002', 0, async function(done) {
        try {
            resultSet.moveToLast();
            var flag = resultSet.isLast();
            console.info("KvStoreResultSetIsLastTest002 isLast " + flag);
            expect(flag).assertTrue();
        } catch (e) {
            expect(null).assertFail();
            console.error("KvStoreResultSetIsLastTest002 fail " + `, error code is ${e.code}, message is ${e.message}`);
        }
        done();
    })

    /**
     * @tc.name KvStoreResultSetIsLastTest003
     * @tc.desc Test Js Api KvStoreResultSet.IsLast() testcase 003
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreResultSetIsLastTest003', 0, async function(done) {
        try {
            var flag = resultSet.isLast(1);
            console.info("KvStoreResultSetIsLastTest003 isLast " + flag);
            expect(null).assertFail();
        } catch (e) {
            console.error("KvStoreResultSetIsLastTest003 fail " + `, error code is ${e.code}, message is ${e.message}`);
        }
        done();
    })

    /**
     * @tc.name KvStoreResultSetIsLastTest004
     * @tc.desc Test Js Api KvStoreResultSet.IsLast() testcase 004
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreResultSetIsLastTest004', 0, async function(done) {
        try {
            var flag = resultSet.isLast(1, 'test_string');
            console.info("KvStoreResultSetIsLastTest004 isLast " + flag);
            expect(null).assertFail();
        } catch (e) {
            console.error("KvStoreResultSetIsLastTest004 fail " + `, error code is ${e.code}, message is ${e.message}`);
        }
        done();
    })

    /**
     * @tc.name KvStoreResultSetIsBeforeFirstTest001
     * @tc.desc Test Js Api KvStoreResultSet.IsBeforeFirst() testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreResultSetIsBeforeFirstTest001', 0, async function(done) {
        try {
            var flag = resultSet.isBeforeFirst();
            console.info("KvStoreResultSetIsBeforeFirstTest001 isBeforeFirst " + flag);
            expect(flag).assertTrue();
        } catch (e) {
            expect(null).assertFail();
            console.error("KvStoreResultSetIsBeforeFirstTest001 fail " + `, error code is ${e.code}, message is ${e.message}`);
        }
        done();
    })

    /**
     * @tc.name KvStoreResultSetIsBeforeFirstTest002
     * @tc.desc Test Js Api KvStoreResultSet.IsBeforeFirst() testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreResultSetIsBeforeFirstTest002', 0, async function(done) {
        try {
            var moved = resultSet.moveToPrevious();
            expect(!moved).assertTrue();
            var flag = resultSet.isBeforeFirst();
            console.info("KvStoreResultSetIsBeforeFirstTest002 isBeforeFirst " + flag);
            expect(flag).assertTrue();
        } catch (e) {
            expect(null).assertFail();
            console.error("KvStoreResultSetIsBeforeFirstTest002 fail " + `, error code is ${e.code}, message is ${e.message}`);
        }
        done();
    })

    /**
     * @tc.name KvStoreResultSetIsBeforeFirstTest003
     * @tc.desc Test Js Api KvStoreResultSet.IsBeforeFirst() testcase 003
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreResultSetIsBeforeFirstTest003', 0, async function(done) {
        try {
            var flag = resultSet.isBeforeFirst(1);
            console.info("KvStoreResultSetIsBeforeFirstTest003 isBeforeFirst " + flag);
            expect(null).assertFail();
        } catch (e) {
            console.error("KvStoreResultSetIsBeforeFirstTest003 fail " + `, error code is ${e.code}, message is ${e.message}`);
        }
        done();
    })

    /**
     * @tc.name KvStoreResultSetIsBeforeFirstTest004
     * @tc.desc Test Js Api KvStoreResultSet.IsBeforeFirst() testcase 004
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreResultSetIsBeforeFirstTest004', 0, async function(done) {
        try {
            var flag = resultSet.isBeforeFirst(1, 'test_string');
            console.info("KvStoreResultSetIsBeforeFirstTest004 isBeforeFirst " + flag);
            expect(null).assertFail();
        } catch (e) {
            console.error("KvStoreResultSetIsBeforeFirstTest004 fail " + `, error code is ${e.code}, message is ${e.message}`);
        }
        done();
    })

    /**
     * @tc.name KvStoreResultSetIsAfterLastTest001
     * @tc.desc Test Js Api KvStoreResultSet.IsAfterLast() testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreResultSetIsAfterLastTest001', 0, async function(done) {
        try {
            var flag = resultSet.isAfterLast();
            console.info("KvStoreResultSetIsAfterLastTest001 isAfterLast " + flag);
            expect(!flag).assertTrue();
        } catch (e) {
            expect(null).assertFail();
            console.error("KvStoreResultSetIsAfterLastTest001 fail " + `, error code is ${e.code}, message is ${e.message}`);
        }
        done();
    })

    /**
     * @tc.name KvStoreResultSetIsAfterLastTest002
     * @tc.desc Test Js Api KvStoreResultSet.IsAfterLast() testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreResultSetIsAfterLastTest002', 0, async function(done) {
        try {
            var moved = resultSet.moveToLast();
            console.info("KvStoreResultSetIsAfterLastTest002  moveToLast  moved=" + moved);
            expect(moved).assertTrue();
            moved = resultSet.moveToNext();
            console.info("KvStoreResultSetIsAfterLastTest002  moveToNext  moved=" + moved);
            expect(moved == false).assertTrue();
            var flag = resultSet.isAfterLast();
            console.info("KvStoreResultSetIsAfterLastTest002  isAfterLast true=" + flag);
            expect(flag).assertTrue();
        } catch (e) {
            console.error("KvStoreResultSetIsAfterLastTest002 fail " + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name KvStoreResultSetIsAfterLastTest003
     * @tc.desc Test Js Api KvStoreResultSet.IsAfterLast() testcase 003
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreResultSetIsAfterLastTest003', 0, async function(done) {
        try {
            var flag = resultSet.isAfterLast(1);
            console.info("KvStoreResultSetIsAfterLastTest003 isAfterLast " + flag);
            expect(null).assertFail();
        } catch (e) {
            console.error("KvStoreResultSetIsAfterLastTest003 fail " + `, error code is ${e.code}, message is ${e.message}`);
        }
        done();
    })

    /**
     * @tc.name KvStoreResultSetIsAfterLastTest004
     * @tc.desc Test Js Api KvStoreResultSet.IsAfterLast() testcase 004
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreResultSetIsAfterLastTest004', 0, async function(done) {
        try {
            var flag = resultSet.isAfterLast(1, 'test_string');
            console.info("KvStoreResultSetIsAfterLastTest004 isAfterLast " + flag);
            expect(null).assertFail();
        } catch (e) {
            console.error("KvStoreResultSetIsAfterLastTest004 fail " + `, error code is ${e.code}, message is ${e.message}`);
        }
        done();
    })

    /**
     * @tc.name KvStoreResultSetGetEntryTest001
     * @tc.desc Test Js Api KvStoreResultSet.GetEntry() testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreResultSetGetEntryTest001', 0, async function(done) {
        try {
            var moved = resultSet.moveToNext();
            var entry = resultSet.getEntry();
            console.info("KvStoreResultSetGetEntryTest001 getEntry " + entry);
            expect(moved && (entry.key == 'batch_test_string_key0')).assertTrue();
            expect(moved && (entry.value.value == 'batch_test_string_value')).assertTrue();
        } catch (e) {
            expect(null).assertFail();
            console.error("KvStoreResultSetGetEntryTest001 fail " + `, error code is ${e.code}, message is ${e.message}`);
        }
        done();
    })

    /**
     * @tc.name KvStoreResultSetGetEntryTest002
     * @tc.desc Test Js Api KvStoreResultSet.GetEntry() testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreResultSetGetEntryTest002', 0, async function(done) {
        try {
            var moved = resultSet.moveToNext();
            var entry = resultSet.getEntry();
            console.info("KvStoreResultSetGetEntryTest002 getEntry " + entry);
            expect(entry.key == 'batch_test_string_key0').assertTrue();
            expect(entry.value.value == 'batch_test_string_value').assertTrue();
            moved = resultSet.moveToNext();
            expect(moved).assertTrue();
            entry = resultSet.getEntry();
            console.info("KvStoreResultSetGetEntryTest002 getEntry " + entry);
            expect(entry.key == 'batch_test_string_key1').assertTrue();
            expect(entry.value.value == 'batch_test_string_value').assertTrue();
        } catch (e) {
            expect(null).assertFail();
            console.error("KvStoreResultSetGetEntryTest002 fail " + `, error code is ${e.code}, message is ${e.message}`);
        }
        done();
    })

    /**
     * @tc.name KvStoreResultSetGetEntryTest003
     * @tc.desc Test Js Api KvStoreResultSet.GetEntry() testcase 003
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreResultSetGetEntryTest003', 0, async function(done) {
        try {
            var entry = resultSet.getEntry(1);
            console.info("KvStoreResultSetGetEntryTest003 getEntry " + entry);
            expect(null).assertFail();
        } catch (e) {
            console.error("KvStoreResultSetGetEntryTest003 fail " + `, error code is ${e.code}, message is ${e.message}`);
        }
        done();
    })

    /**
     * @tc.name KvStoreResultSetGetEntryTest004
     * @tc.desc Test Js Api KvStoreResultSet.GetEntry() testcase 004
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreResultSetGetEntryTest004', 0, async function(done) {
        try {
            var entry = resultSet.getEntry(1, 'test_string');
            console.info("KvStoreResultSetGetEntryTest004 getEntry " + entry);
            expect(null).assertFail();
        } catch (e) {
            console.error("KvStoreResultSetGetEntryTest004 fail " + `, error code is ${e.code}, message is ${e.message}`);
        }
        done();
    })
})
