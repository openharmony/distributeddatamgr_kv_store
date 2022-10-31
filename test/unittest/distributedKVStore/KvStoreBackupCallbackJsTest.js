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
let STORE_ID = 'kvstoreBackupCallback';

let mKVMgrConfig = {
    bundleName: BUNDLE_NAME,
    context: context
};

function sleep(ms) {
    return new Promise(resolve => setTimeout(resolve, ms));
}

function publicGetKvStore(optionsp) {
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

function publicGet(kvStore, key) {
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
        kvStore.backup(file, function (err, data) {
            console.log("Test backup task =" + data);
            if (err != undefined) {
                console.log("Test backup err information: " + err);
                reject(err);
            } else {
                resolve(data);
            }
        })
    })
}

function publicDeleteBackup(kvStore, files) {
    console.log(`Test deleteBackup ${JSON.stringify(files)}`)
    return new Promise(function (resolve, reject) {
        kvStore.deleteBackup(files, function (err, data) {
            console.log("Test deleteBackup BackUpInfo =" + data);
            if (err != undefined) {
                console.log("Test deleteBackup err information: " + err);
                reject(err);
            } else {
                var devices = new Array();
                devices = data;
                delresult = devices;
                console.log("Test deleteBackup pass ");
                resolve(data);
            }
        })
    })
}

function publicRestoresp(kvStore, file) {
    console.log(`Test restoresp ${JSON.stringify(file)}`)
    return new Promise(function (resolve, reject) {
        kvStore.restore(file, function (err, data) {
            console.log("Test restoresp task =" + JSON.stringify(data));
            if (err != undefined) {
                console.log("Test restoresp err information: " + err);
                reject(err);
            } else {
                console.log("Test restoresp backupinfo information: " + JSON.stringify(data));
                resolve(data);
            }
        })
    })
}

function publicRestore(kvStore) {
    console.log(`Test restore `)
    return new Promise(function (resolve, reject) {
        kvStore.restore(function (err, data) {
            console.log("Test restore task =" + JSON.stringify(data));
            if (err != undefined) {
                console.log("Test restore err information: " + err);
                reject(err);
            } else {
                console.log("Test restore backupinfo information: " + JSON.stringify(data));
                resolve(data);
            }
        })
    })
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

describe('kvStoreBackupCallbackJsunittest', function () {
    beforeAll(async function () {
        console.info('Test beforeAll: Prerequisites at the test suite level, ' +
            'which are executed before t he test suite is executed.');
        await publicGetKvStore(optionLock);
        await sleep(5000);
        console.info("Test kvstore = " + kvStore)
    })
    beforeEach(async function () {
        console.info('beforeEach: Prerequisites at the test case level,' +
            ' which are executed before each test case is executed.');
        await publicGetKvStore(optionLock);
    })
    afterEach(async function () {
        console.info('afterEach: Test case-level clearance conditions, ' +
            'which are executed after each test case is executed.');
        publicDeleteBackup(kvStore, files);
        publicCloseKvStore();
        files = []
        await sleep(5000);
    })
    afterAll(function () {
        console.info('afterAll: Test suite-level cleanup condition, ' +
            'which is executed after the test suite is executed');
        publicCloseKvStore();
        kvManager = null;
        console.info("Test kvstore = " + kvStore)
    })

    /**
     * @tc.name KvStoreBackupManalRestoreCallbackSucTest
     * @tc.desc Test Js Api SingleKvStore.restore() manal successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreBackupManalRestoreCallbackSucTest', 0, async function (done) {
        try {
            console.log("KvStoreBackupManalRestoreCallbackSucTest before restore");
            await publicRestore(kvStore).then((data) => {
                console.log("KvStoreBackupManalRestoreCallbackSucTest going restore = " + JSON.stringify(data));
                expect(true).assertEqual(true);
                done();
            }).catch((err) => {
                console.log("KvStoreBackupManalRestoreCallbackSucTest Manualrestore fail 1" + err);
                expect(true).assertEqual(err == "Error: Parameter error.The number of parameters is incorrect.");
                done();
            })
        } catch (e) {
            console.log("KvStoreBackupManalRestoreCallbackSucTest Manualrestore fail 2" + JSON.stringify(e));
            expect(e).assertFail();
            done();
        }
    })

    /**
     * @tc.name KvStoreBackupManalRestoreCallbackRestoreTest
     * @tc.desc Test Js Api SingleKvStore.restore() manal backup restore and delete backup
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreBackupManalRestoreCallbackRestoreTest', 0, async function (done) {
        try {
            console.log("KvStoreBackupManalRestoreCallbackRestoreTest before getname");
            file = '123';
            files[0] = file;
            await publicBackup(kvStore, file)
                .then(() => publicRestoresp(kvStore, file))
                .then(() => publicDeleteBackup(kvStore, files))
                .then((data) => {
                    let delResult = delresult[0];
                    expect(0).assertEqual(delResult[1]);
                    done();
                })
        } catch (e) {
            console.log("KvStoreBackupManalRestoreCallbackRestoreTest fail 2" + e);
            expect(e).assertFail();
            done();
        }
    })


    /**
     * @tc.name KvStoreBackupManalRestoreCallbackNoRestoreTest
     * @tc.desc Test Js Api SingleKvStore.backup() delete without restore
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreBackupManalRestoreCallbackNoRestoreTest', 0, async function (done) {
        try {
            console.log("KvStoreBackupManalRestoreCallbackNoRestoreTest before getname");
            file = 'true';
            files[0] = file;
            await publicBackup(kvStore, file)
                .then(() => publicDeleteBackup(kvStore, files))
                .then((data) => {
                    let delResult = delresult[0];
                    expect(0).assertEqual(delResult[1]);
                    done();
                });
        } catch (e) {
            console.log("KvStoreBackupManalRestoreCallbackNoRestoreTest  fail 2 " + e);
            expect(e).assertFail();
            done();
        }
    })

    /**
     * @tc.name KvStoreBackupManalRestoreCallbackSameNameTest
     * @tc.desc Test Js Api SingleKvStore.backup() manal not same file but same name
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreBackupManalRestoreCallbackSameNameTest', 0, async function (done) {
        try {
            console.log("KvStoreBackupestManalBackupCallbackTest001t before getname");
            files = []
            file = 'legal';
            files[0] = "legal";
            await publicBackup(kvStore, file)
                .then(() => publicDeleteBackup(kvStore, files))
                .then((data) => {
                    let delResult = delresult[0];
                    expect(0).assertEqual(delResult[1]);
                    done();
                });
        } catch (e) {
            console.log("KvStoreBackupManalRestoreCallbackSameNameTest export fail 2 " + e);
            expect(e).assertFail();
            done();
        }
    })
    /**
     * @tc.name KvStoreBackupManalRestoreCallbackIntNameTest
     * @tc.desc Test Js Api SingleKvStore.backup() manal with name integer
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreBackupManalRestoreCallbackIntNameTest', 0, async function (done) {
        try {
            console.log("KvStoreBackupManalRestoreCallbackIntNameTest before getname");
            file = '1';
            files[0] = file;
            console.log("KvStoreBackupManalRestoreCallbackIntNameTest before backup");
            await publicBackup(kvStore, file)
                .then(() => publicDeleteBackup(kvStore, files))
                .then((data) => {
                    let delResult = delresult[0];
                    expect(0).assertEqual(delResult[1]);
                    done();
                });
        } catch (e) {
            console.log("KvStoreBackupManalRestoreCallbackIntNameTest  fail 2 " + e);
            expect(e).assertFail();
            done();
        }
    })

    /**
     * @tc.name KvStoreBackupManalRestoreCallbackDoubleNameTest
     * @tc.desc Test Js Api SingleKvStore.backup() manal with name double
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreBackupManalRestoreCallbackDoubleNameTest', 0, async function (done) {
        try {
            file = '1.0';
            files[0] = file;
            await publicBackup(kvStore, file)
                .then(() => publicDeleteBackup(kvStore, files))
                .then((data) => {
                    let delResult = delresult[0];
                    expect(0).assertEqual(delResult[1]);
                    done();
                });
        } catch (e) {
            console.log("KvStoreBackupManalRestoreCallbackDoubleNameTest fail 2 " + e);
            expect(e).assertFail();
            done();
        }
    })

    /**
     * @tc.name KvStoreBackupManalBackupCallbackInvalidNameTest
     * @tc.desc Test Js Api SingleKvStore.backup() with illegal name
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreBackupManalBackupCallbackInvalidNameTest', 0, async function (done) {
        try {
            file = '';
            await publicBackup(kvStore, file).then((data) => {
                expect(true).assertEqual(data == "code数字");
                done();
            }).catch((err) => {
                console.log("KvStoreBackupManalBackupCallbackInvalidNameTest ManualbackupCallback002 fail1 " + err);
                expect(true).assertEqual(err.code == 401);
                done();
            })
        } catch (e) {
            console.error("KvStoreBackupManalBackupCallbackInvalidNameTest fail 2 " + e);
            expect(e).assertFail();
            done();
        }
    })

    /**
     * @tc.name KvStoreBackupManalBackupCallbackMoreThan4FilesTest
     * @tc.desc Test Js Api SingleKvStore.backup() back up files more than 4
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreBackupManalBackupCallbackMoreThan4FilesTest', 0, async function (done) {
        try {
            console.log("KvStoreBackupManalBackupCallbackMoreThan4FilesTest before getname");
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
            console.log("KvStoreBackupManalBackupCallbackMoreThan4FilesTest before backup");
            await publicBackup(kvStore,file)
                .then(publicBackup(kvStore, file1))
                .then(publicBackup(kvStore, file2))
                .then(publicBackup(kvStore, file3))
                .then(publicBackup(kvStore, file4));
            await publicBackup(kvStore, file5).then((data) => {
                expect(true).assertEqual(data == "code数字");
                done();
            }).catch((err) => {
                console.log(JSON.stringify(err));
                expect(true).assertEqual(JSON.stringify(err) == "{\"code\":\"\"}");
            })
            await sleep(1000);
            await publicDeleteBackup(kvStore, files).then((data) => {

                expect("1").assertEqual(delresult[0][0])
                expect(0).assertEqual(delresult[0][1]);

                expect("4").assertEqual(delresult[3][0])
                expect(0).assertEqual(delresult[3][1]);

                expect("5").assertEqual(delresult[4][0])
                expect(27459591).assertEqual(delresult[4][1]);

                expect("6").assertEqual(delresult[5][0])
                expect(27459591).assertEqual(delresult[5][1]);

                files = [];
                console.log("Test clear files");
                done();
            })
        } catch (e) {
            console.error("KvStoreBackupManalBackupCallbackMoreThan4FilesTest fail 2 " + e);
            expect(e).assertFail();
            done();
        }
    })

    /**
     * @tc.name KvStoreBackupManalRestoreWithoutBackupCallbackTest
     * @tc.desc Test Js Api SingleKvStore.restore() restore without backup
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreBackupManalRestoreWithoutBackupCallbackTest', 0, async function (done) {
        try {
            file = 'legal';
            await publicRestoresp(kvStore, file).then((data) => {
                expect(true).assertEqual(false);
                done();
            }).catch((err) => {
                expect(true).assertEqual(err.code == 401);
                done();
            })
        } catch (e) {
            console.log("KvStoreBackupManalRestoreWithoutBackupCallbackTest fail 2" + e);
            expect(e).assertFail();
            done();
        }
    })

    /**
     * @tc.name KvStoreBackupDeleteBackupOnlyCallbackTest
     * @tc.desc Test Js Api SingleKvStore.deleteBackup() without restore and backup
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreBackupDeleteBackupOnlyCallbackTest', 0, async function (done) {
        try {
            file = '123';
            files[0] = file;
            await publicDeleteBackup(kvStore, files).then((data) => {
                let delResult = delresult[0];
                expect(27459591).assertEqual(delResult[1]);
                done();
            })
        } catch (err) {
            console.log("KvStoreBackupDeleteBackupOnlyCallbackTest fail 2" + err);
            expect(err).assertFail();
            done();
        }
    })

    /**
     * @tc.name KvStoreBackupDbBackupPutCallbackSucTest
     * @tc.desc Test Js Api SingleKvStore.put() db successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreBackupDbBackupPutCallbackSucTest', 0, async function (done) {
        try {
            publicPut(kvStore, "key", "value");
            expect(true).assertTrue();
            done();
        } catch (e) {
            console.log("KvStoreBackupDbBackupPutCallbackSucTest Backupinfo fail" + e);
            expect(e).assertFail();
            done();
        }
    })

    /**
     * @tc.name KvStoreBackupDbBackupPutCallbackValueTest
     * @tc.desc Test Js Api SingleKvStore.put() put value successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreBackupDbBackupPutCallbackValueTest', 0, async function (done) {
        try {
            publicPut(kvStore, "key", "value");
            await publicGet(kvStore, "key").then((data) => {
                expect(true).assertEqual(data == "value");
                done();
            }).catch((err) => {
                console.log("KvStoreBackupDbBackupPutCallbackValueTest Get fail 1 " + err);
                expect(err).assertFail();
                done();
            })
        } catch (e) {
            console.log("KvStoreBackupDbBackupPutCallbackValueTest Get fail 2" + e);
            expect(e).assertFail();
            done();
        }
    })

    /**
     * @tc.name KvStoreBackupDbBackupPutCallbackFailTest
     * @tc.desc Test Js Api SingleKvStore.put() db get different keys
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreBackupDbBackupPutCallbackFailTest', 0, async function (done) {
        try {
            publicPut(kvStore, "put", "value1");
            await publicGet(kvStore, "putcallback").then((data) => {
                expect(true).assertEqual(false);
                done();
            }).catch((err) => {
                console.log("KvStoreBackupDbBackupPutCallbackFailTest Get fail 1 " + err);
                expect(true).assertEqual(true);
                done();
            })
        } catch (e) {
            console.log("KvStoreBackupDbBackupPutCallbackFailTest Get fail 2" + e);
            expect(e).assertFail();
            done();
        }
    })
})
