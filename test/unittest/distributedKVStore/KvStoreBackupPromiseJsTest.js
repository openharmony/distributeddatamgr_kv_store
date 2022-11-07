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
import distributedData from '@ohos.data.distributedKVStore';
import abilityFeatureAbility from '@ohos.ability.featureAbility'

var context = abilityFeatureAbility.getContext();
let BUNDLE_NAME = 'com.example.myapplication';
var kvManager = null;
var kvStore = null;
var delresult = null;
let STORE_ID = 'kvstoreBackupPromise';

let mKVMgrConfig = {
    bundleName: BUNDLE_NAME,
    context: context
};

function publicgetKvStore(optionsp) {
    console.log(`Test getKvStore `)
    return new Promise(function (resolve, reject) {
        distributedData.createKVManager(mKVMgrConfig, (err, data) => {
            console.info('Test createKVManager begin')
            if (err) {
                console.info('Test createKVManager err = ' + err);
                reject(err);
            }
            console.info('Test createKVManager data = ' + data);


            kvManager = data;
            data.getKVStore(STORE_ID, optionsp, (err, data) => {
                console.info('Test getKVStore begin')
                if (err) {
                    console.info('Test getKVStore err = ' + err);
                    reject(err);
                }
                console.info('Test getKVStore data = ' + data);
                kvStore = data;
                resolve(data);
            });
        });
    })
}

function publicCloseKvStore() {
    console.log(`Test closeKvStore `)
    return new Promise(function (resolve, reject) {
        kvManager.closeKVStore(BUNDLE_NAME, STORE_ID, kvStore, (err, data) => {
            console.info('Test closeKvStore begin')
            if (err) {
                console.info('Test closeKvStore err = ' + err);
                reject(err);
            }
            console.info('Test closeKvStore data = ' + data);
            kvManager.deleteKVStore(BUNDLE_NAME, STORE_ID, (err, data) => {
                console.info('Test deleteKVStore begin')
                if (err) {
                    console.info('Test deleteKVStore err = ' + err);
                    reject(err);
                }
                console.info('Test deleteKVStore data = ' + data);
            });
        });
    })
}

function publicPut(kvStore, key, value) {
    console.log(`Test put ${JSON.stringify(key, value)}`)
    return new Promise(function (resolve, reject) {
        kvStore.put(key, value, function (err, data) {
            console.log("Test put task =" + JSON.stringify(data));
            if (err != undefined) {
                console.log("Test put err information: " + err);
                reject(err);
            } else {
                resolve(data);
            }
        })
    })
}

function publicget(kvStore, key) {
    console.log(`Test get ${JSON.stringify(key)}`)
    return new Promise(function (resolve, reject) {
        kvStore.get(key, function (err, data) {
            console.log("Test get task =" + JSON.stringify(data));
            if (err != undefined) {
                console.log("Test get err information: " + err);
                reject(err);
            } else {
                resolve(data);
            }
        })
    })
}

function publicBackup(kvStore, file) {
    console.log(`Test backup ${JSON.stringify(file)}`)
    return new Promise(function (resolve, reject) {
        kvStore.backup(file).then((data) => {
            console.log("Test backup task =" + data);
            resolve(data);
        }).catch((err) => {
            console.log("Test backup err information: " + err);
            reject(err);
        })
    })
}

function publicDeleteBackup(kvStore, files) {
    console.log(`Test deleteBackup ${JSON.stringify(files)}`)
    return new Promise(function (resolve, reject) {
        kvStore.deleteBackup(files).then((data) => {
            console.log("Test deleteBackup BackUpInfo =" + data);
            var devices = new Array();
            devices = data;
            delresult = devices;
            console.log("Test deleteBackup pass ");
            resolve(data);
        }).catch((err) => {
            console.log("test deleteBackup err information: " + err);
            reject(err);
        })
    })
}

function publicRestoreSp(kvStore, file) {
    console.log(`Test restoresp ${JSON.stringify(file)}`)
    return new Promise(function (resolve, reject) {
        kvStore.restore(file).then((data) => {
            console.log("Test restoresp backupinfo information: " + JSON.stringify(data));
            resolve(data);
        }).catch((err) => {
            console.log("Test restoresp err information: " + err);
            reject(err);
        })
    })
}

function publicRestore(kvStore) {
    console.log(`Test restore `)
    return new Promise(function (resolve, reject) {
        kvStore.restore.then((data) => {
            console.log("Test restore backupinfo information: " + JSON.stringify(data));
            resolve(data);
        }).catch((err) => {
            console.log("Test restore err information: " + err);
            reject(err);
        })
    })
}

function sleep(ms) {
    return new Promise(resolve => setTimeout(resolve, ms));
}

const optionLock = {
    createIfMissing: true,
    encrypt: true,
    backup: true,
    autoSync: false,
    kvStoreType: distributedData.KVStoreType.SINGLE_VERSION,
    securityLevel: distributedData.SecurityLevel.S1,
}

var file = '';

var files = new Array();

describe('kvStoreBackupPromiseJsunittest', function () {
    beforeAll(async function () {
        console.info('Test beforeAll: Prerequisites at the test suite level, ' +
            'which are executed before the test suite is executed.');
        await publicgetKvStore(optionLock);
        await sleep(5000);
        console.info("Test kvstore = " + kvStore)
    })
    beforeEach(function () {
        console.info('beforeEach: Prerequisites at the test case level,' +
            ' which are executed before each test case is executed.');
    })
    afterEach(async function () {
        console.info('afterEach: Test case-level clearance conditions, ' +
            'which are executed after each test case is executed.');
        publicDeleteBackup(kvStore, files);
        await sleep(5000);
    })
    afterAll(async function () {
        console.info('afterAll: Test suite-level cleanup condition, ' +
            'which is executed after the test suite is executed');
        publicCloseKvStore();
        await kvManager.getAllKVStoreId(TEST_BUNDLE_NAME).then((data) => {
            console.info(data.length);
        })
        kvManager = null;
        console.info("Test kvstore = " + kvStore)
    })

    /**
     * @tc.name KvStoreBackupManalRestorePromiseFailTest
     * @tc.desc Test Js Api SingleKvStore.restore() manal restore illegal kvstore
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreBackupManalRestorePromiseFailTest', 0, async function (done) {
        try {
            await publicRestore(kvStore).then((data) => {
                expect(true).assertEqual(data == 'code数字');
                done();
            }).catch((err) => {
                console.log("KvStoreBackupManalRestorePromiseFailTest Manualrestore fail 1" + err);
                expect(true).assertEqual(JSON.stringify(err) == '{}');
                done();
            })
        } catch (e) {
            console.log("KvStoreBackupManalRestorePromiseFailTest Manualrestore fail 2" + JSON.stringify(e));
            expect(e).assertFail();
            done();
        }
    })

    /**
     * @tc.name KvStoreBackupManalRestorePromiseIntTest
     * @tc.desc Test Js Api SingleKvStore.restore() manal with int name
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreBackupManalRestorePromiseIntTest', 0, async function (done) {
        try {
            file = '123';
            files[0] = file;
            await publicBackup(kvStore, file)
                .then(() => publicRestoreSp(kvStore, file))
                .then(() => publicDeleteBackup(kvStore, files))
                .then(() => {
                    let delResult = delresult[0];
                    expect(0).assertEqual(delResult[1]);
                    done();
                })
        } catch (e) {
            console.log("KvStoreBackupManalRestorePromiseIntTest Manualrestore fail 2" + e);
            expect(e).assertFail();
            done();
        }
    })

    /**
     * @tc.name KvStoreBackupManalRestorePromiseSameNameTest
     * @tc.desc Test Js Api SingleKvStore.backup() manal not same file but same name
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreBackupManalRestorePromiseSameNameTest', 0, async function (done) {
        try {
            console.log("KvStoreBackupManalRestorePromiseSameNameTest before getname");
            file = 'legal';
            files[0] = "legal";
            await publicBackup(kvStore, file)
                .then(() => publicDeleteBackup(kvStore, files))
                .then(() => {
                    let delResult = delresult[0];
                    expect(0).assertEqual(delResult[1]);
                    done();
                })
        } catch (e) {
            console.log("KvStoreBackupManalRestorePromiseSameNameTest export fail 2 " + e);
            expect(e).assertFail();
            done();
        }
    })

    /**
     * @tc.name KvStoreBackupManalRestorePromiseNameTrueTest
     * @tc.desc Test Js Api SingleKvStore.backup() manal with name true
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreBackupManalRestorePromiseNameTrueTest', 0, async function (done) {
        try {
            file = 'true';
            files[0] = file;
            await publicBackup(kvStore, file)
                .then(() => publicDeleteBackup(kvStore, files))
                .then(() => {
                    let delResult = delresult[0];
                    expect(0).assertEqual(delResult[1]);
                    done();
                })
        } catch (e) {
            console.log("KvStoreBackupManalRestorePromiseNameTrueTest fail 2 " + e);
            expect(e).assertFail();
            done();
        }
    })

    /**
     * @tc.name KvStoreBackupManalRestorePromiseNameDoubleTest
     * @tc.desc Test Js Api SingleKvStore.backup() manal with name double
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreBackupManalRestorePromiseNameDoubleTest', 0, async function (done) {
        try {
            file = '1.0';
            files[0] = file;
            await publicBackup(kvStore, file)
                .then(() => publicDeleteBackup(kvStore, files))
                .then(() => {
                    let delResult = delresult[0];
                    expect(0).assertEqual(delResult[1]);
                    done();
                })
        } catch (e) {
            console.log("KvStoreBackupManalRestorePromiseNameDoubleTest fail " + e);
            expect(e).assertFail();
            done();
        }
    })

    /**
     * @tc.name KvStoreBackupManalRestorePromiseInvalidNameTest
     * @tc.desc Test Js Api SingleKvStore.backup() manal with illegal name
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreBackupManalRestorePromiseInvalidNameTest', 0, async function (done) {
        try {
            file = '';
            await publicBackup(kvStore, file).then((data) => {
                expect(true).assertEqual(data == "code数字");
                done();
            }).catch((err) => {
                console.log("KvStoreBackupManalRestorePromiseInvalidNameTest fail1 " + err);
                expect(true).assertEqual(err.code == 401)
                done();
            })
        } catch (e) {
            console.log("KvStoreBackupManalRestorePromiseInvalidNameTest fail 2 " + e);
            expect(e).assertFail();
            done();
        }
    })

    /**
     * @tc.name KvStoreBackupManalBackupPromiseMoreThan4Test
     * @tc.desc Test Js Api SingleKvStore.backup() manal back up files more than 4
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreBackupManalBackupPromiseMoreThan4Test', 0, async function (done) {
        try {
            console.log("KvStoreBackupManalBackupPromiseMoreThan4Test before getname");
            files = [];
            var file = '1';
            var file1 = '2';
            var file2 = '3';
            var file3 = '4';
            var file4 = '5';
            var file5 = '6';
            files[0] = file;
            files[1] = file1;
            files[2] = file2;
            files[3] = file3;
            files[4] = file4;
            files[5] = file5;
            await kvStore.backup(file)
                .then(kvStore.backup(file1))
                .then(kvStore.backup(file2))
                .then(kvStore.backup(file3))
                .then(kvStore.backup(file4));
            await publicBackup(kvStore, file5).then((data) => {
                expect(true).assertEqual(data == "code数字");
                done();
            }).catch((err) => {
                console.log("KvStoreBackupManalBackupPromiseMoreThan4Test  fail1 " + err);
                expect(true).assertEqual(JSON.stringify(err) == "{\"code\":\"\"}");
            })
            await sleep(1000);
            await publicDeleteBackup(kvStore, files).then(() => {
                expect("1").assertEqual(delresult[0][0])
                expect(0).assertEqual(delresult[0][1]);

                expect("5").assertEqual(delresult[4][0])
                expect(27459591).assertEqual(delresult[4][1]);

                expect("6").assertEqual(delresult[5][0])
                expect(27459591).assertEqual(delresult[5][1]);
                done();
            })
        } catch (e) {
            console.log("KvStoreBackupManalBackupPromiseMoreThan4Test fail 2 " + e);
            expect(e).assertFail();
            done();
        }
    })

    /**
     * @tc.name KvStoreBackupManalRestorePromiseNoBackupTest
     * @tc.desc Test Js Api SingleKvStore.restore() restore without backup fail
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreBackupManalRestorePromiseNoBackupTest', 0, async function (done) {
        try {
            file = 'legal';
            await publicRestoreSp(kvStore, file).then(() => {
                expect(true).assertEqual(false);
                done();
            }).catch((err) => {
                console.log("KvStoreBackupManalRestorePromiseNoBackupTest Manual restoresp fail 1" + err);
                expect(true).assertEqual(err.code == 401);
                done();
            })
        } catch (e) {
            console.log("KvStoreBackupManalRestorePromiseNoBackupTest Manual restoresp fail 2" + e);
            expect(e).assertFail();
            done();
        }
    })


    /**
     * @tc.name KvStoreBackupDeleteBackupPromiseNoBackupTest
     * @tc.desc Test Js Api SingleKvStore.deleteBackup() delete without backup
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreBackupDeleteBackupPromiseNoBackupTest', 0, async function (done) {
        try {
            file = '123';
            files[0] = file;
            await publicDeleteBackup(kvStore, files).then(() => {
                let delResult = delresult[0];
                expect(27459591).assertEqual(delResult[1]);
                done();
            })
        } catch (err) {
            console.log("KvStoreBackupDeleteBackupPromiseNoBackupTest deleteBackup fail 2" + err);
            expect(err).assertFail();
            done();
        }
    })

    /**
     * @tc.name KvStoreBackupDbBackupPutPromiseSucTest
     * @tc.desc Test Js Api SingleKvStore.put() db successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreBackupDbBackupPutPromiseSucTest', 0, async function (done) {
        try {
            publicPut(kvStore, "key1", "value1");
            done();
        } catch (e) {
            console.log("KvStoreBackupDbBackupPutPromiseSucTest Backupinfo fail" + e);
            expect(e).assertFail();
            done();
        }
    })

    /**
     * @tc.name KvStoreBackupDbBackupPutPromiseValueTest
     * @tc.desc Test Js Api SingleKvStore.put() put value successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreBackupDbBackupPutPromiseValueTest', 0, async function (done) {
        try {
            publicPut(kvStore, "PutPromise0002", "value");
            await publicget(kvStore, "PutPromise0002").then((data) => {
                expect(true).assertEqual(data == "value");
                done();
            }).catch((err) => {
                console.log("KvStoreBackupDbBackupPutPromiseValueTest Get fail 1 " + err);
                expect(err).assertFail();
                done();
            })
        } catch (e) {
            console.log("KvStoreBackupDbBackupPutPromiseValueTest Get fail 2" + e);
            expect(e).assertFail();
            done();
        }
    })

    /**
     * @tc.name KvStoreBackupDbBackupPutPromiseFailTest
     * @tc.desc Test Js Api SingleKvStore.put() db get different keys
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreBackupDbBackupPutPromiseFailTest', 0, async function (done) {
        try {
            publicPut(kvStore, "Put", "value1");
            await publicget(kvStore, "PutPromise").then((data) => {
                expect(true).assertEqual(JSON.stringify(data) == '{}');
                done();
            }).catch((err) => {
                console.log("KvStoreBackupDbBackupPutPromiseFailTest Get fail 1 " + err);
                expect(true).assertEqual(err.code == 15100004);
                done();
            })
        } catch (e) {
            console.log("KvStoreBackupDbBackupPutPromiseFailTest Get fail 2" + e);
            expect(e).assertFail();
            done();
        }
    })
})
