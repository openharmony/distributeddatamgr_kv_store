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
    context : context
};

function publicgetKvStore(optionsp){
    console.log(`Test getKvStore `)
    return new Promise(function(resolve, reject) {
        distributedData.createKVManager(mKVMgrConfig, (err, data) => {
            console.info('Test createKVManager begin')
            if (err) {
                console.info('Test createKVManager err = ' + err );
                reject(err);
            }
            console.info('Test createKVManager data = ' + data);

            //            this.log('create success: ' + JSON.stringify(data));
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

function publiccloseKvStore() {
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

function publicput(kvStore,key,value){
    console.log(`Test put ${JSON.stringify(key,value)}`)
    return new Promise(function(resolve, reject) {
        kvStore.put(key,value, function(err, data){
            console.log("Test put task =" + JSON.stringify(data));
            if (err != undefined) {
                console.log("Test put err information: " + err );
                reject(err);
            }else{
                resolve(data);
            }
        })
    })
}

function publicget(kvStore,key){
    console.log(`Test get ${JSON.stringify(key)}`)
    return new Promise(function(resolve, reject) {
        kvStore.get(key, function(err, data){
            console.log("Test get task =" + JSON.stringify(data));
            if (err != undefined) {
                console.log("Test get err information: " + err );
                reject(err);
            }else{
                resolve(data);
            }
        })
    })
}

function publicbackup(kvStore,file){
    console.log(`Test backup ${JSON.stringify(file)}`)
    return new Promise(function(resolve, reject) {
        kvStore.backup(file).then((data) => {
            console.log("Test backup task =" + data);
            resolve(data);
        }).catch((err) => {
            console.log("Test backup err information: " + err);
            reject(err);
        })
    })
}

function publicdeleteBackup(kvStore,files) {
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

function publicrestoresp(kvStore,file){
    console.log(`Test restoresp ${JSON.stringify(file)}`)
    return new Promise(function(resolve, reject) {
        kvStore.restore(file).then((data) => {
            console.log("Test restoresp backupinfo information: " + JSON.stringify(data));
            resolve(data);
        }).catch((err) => {
            console.log("Test restoresp err information: " + err  );
            reject(err);
        })
    })
}

function publicrestore(kvStore){
    console.log(`Test restore `)
    return new Promise(function(resolve, reject) {
        kvStore.restore.then((data) => {
            console.log("Test restore backupinfo information: " + JSON.stringify(data) );
            resolve(data);
        }).catch((err) => {
            console.log("Test restore err information: " + err );
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

var files =new Array();

describe('kvStoreBackupPromiseJsunittest', function () {
    beforeAll( async function () {
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
    afterEach( async function () {
        console.info('afterEach: Test case-level clearance conditions, ' +
        'which are executed after each test case is executed.');
        publicdeleteBackup(kvStore,files);
        await sleep(5000);
    })
    afterAll( async function () {
        console.info('afterAll: Test suite-level cleanup condition, ' +
        'which is executed after the test suite is executed');
        publiccloseKvStore();
        await kvManager.getAllKVStoreId(TEST_BUNDLE_NAME).then((data) => {
            console.info(data.length);
        })
        kvManager = null;
        console.info("Test kvstore = " + kvStore)
    })

    /**
     * @tc.name KvStoreBackupManalRestorePromiseTest001
     * @tc.desc Test Js Api SingleKvStore.restore() manal testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreBackupManalRestorePromiseTest001', 0, async function (done) {
        try {
            console.log("KvStoreBackupManalRestorePromiseTest001 before restore");
            await publicrestore(kvStore).then((data) => {
                console.log("KvStoreBackupManalRestorePromiseTest001 going restore = " + JSON.stringify(data));
                expect(true).assertEqual(data == 'code数字');
                done();
            }).catch((err) => {
                console.log("KvStoreBackupManalRestorePromiseTest001 Manualrestore fail 1" + err);
                expect(true).assertEqual(JSON.stringify(err) == '{}');
                done();
            })
        } catch (e) {
            console.log("KvStoreBackupManalRestorePromiseTest001 Manualrestore fail 2" + JSON.stringify(e));
            expect(e).assertFail();
            done();
        }
    })

    /**
     * @tc.name KvStoreBackupManalRestorePromiseTest002
     * @tc.desc Test Js Api SingleKvStore.restore() manal testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreBackupManalRestorePromiseTest002', 0, async function (done) {
        try {
            console.log("KvStoreBackupManalRestorePromiseTest002 before getname");
            file  = '123' ;
            files[0] = file ;
            publicbackup(kvStore,file) ;
            console.log("KvStoreBackupManalRestorePromiseTest002 before restore");
            await sleep(1000);
            publicrestore(kvStore);
            console.log("KvStoreBackupManalRestorePromiseTest002 going restore ");
            await sleep(1000);
            await publicdeleteBackup(kvStore,files).then((data) => {
                let delResult = delresult[0];
                console.info("KvStoreBackupManalRestorePromiseTest002 delResult = " + delResult);
                console.info("KvStoreBackupManalRestorePromiseTest002 delResult[1] = " + delResult[1]);
                expect(0).assertEqual(delResult[1]);
                console.log("KvStoreBackupManalRestorePromiseTest002 publicdeleteBackup" + JSON.stringify(data));
                done();
            })
        } catch (e) {
            console.log("KvStoreBackupManalRestorePromiseTest002 Manualrestore fail 2" + e);
            expect(e).assertFail();
            done();
        }
    })

    /**
     * @tc.name KvStoreBackupManalRestorePromiseTest001
     * @tc.desc Test Js Api SingleKvStore.backup() manal testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreBackupManalBackupPromiseTest001', 0, async function (done) {
        try {
            console.log("KvStoreBackupManalBackupPromiseTest001 before getname");
            file = 'legal' ;
            files[0] = "legal" ;
            console.log("KvStoreBackupManalBackupPromiseTest001 before backup");
            publicbackup(kvStore,file) ;
            console.log("KvStoreBackupManalBackupPromiseTest001 going backup");
            await sleep(1000);
            await publicdeleteBackup(kvStore,files).then((data) => {
                let delResult = delresult[0];
                console.info("KvStoreBackupManalBackupPromiseTest001 delResult = " + delResult);
                console.info("KvStoreBackupManalBackupPromiseTest001 delResult[1] = " + delResult[1]);
                expect(0).assertEqual(delResult[1]);
                console.log("KvStoreBackupManalBackupPromiseTest001 publicdeleteBackup" + JSON.stringify(data));
                done();
            })
        } catch (e) {
            console.log("KvStoreBackupManalBackupPromiseTest001 export fail 2 " + e);
            expect(e).assertFail();
            done();
        }
    })

    /**
     * @tc.name KvStoreBackupManalRestorePromiseTest002
     * @tc.desc Test Js Api SingleKvStore.backup() manal testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreBackupManalBackupPromiseTest002', 0, async function (done) {
        try {
            console.log("KvStoreBackupManalBackupPromiseTest002 before getname");
            file  = 'true' ;
            files[0] = file ;
            console.log("KvStoreBackupManalBackupPromiseTest002 before backup");
            publicbackup(kvStore,file) ;
            console.log("KvStoreBackupManalBackupPromiseTest002 going backup");
            await sleep(1000);
            await publicdeleteBackup(kvStore,files).then((data) => {
                let delResult = delresult[0];
                console.info("KvStoreBackupManalBackupPromiseTest002 delResult = " + delResult);
                console.info("KvStoreBackupManalBackupPromiseTest002 delResult[1] = " + delResult[1]);
                expect(0).assertEqual(delResult[1]);
                console.log("KvStoreBackupManalBackupPromiseTest002 publicdeleteBackup" + JSON.stringify(data));
                done();
            })
        } catch (e) {
            console.log("KvStoreBackupManalBackupPromiseTest002 fail 2 " + e);
            expect(e).assertFail();
            done();
        }
    })

    /**
     * @tc.name KvStoreBackupManalRestorePromiseTest003
     * @tc.desc Test Js Api SingleKvStore.backup() manal testcase 003
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreBackupManalBackupPromiseTest003', 0, async function (done) {
        try {
            console.log("KvStoreBackupManalBackupPromiseTest003 before getname");
            file  = '1' ;
            files[0] = file ;
            console.log("KvStoreBackupManalBackupPromiseTest003 before backup");
            publicbackup(kvStore,file) ;
            console.log("KvStoreBackupManalBackupPromiseTest003 going backup");
            await sleep(1000);
            await publicdeleteBackup(kvStore,files).then((data) => {
                let delResult = delresult[0];
                console.info("KvStoreBackupManalBackupPromiseTest003 delResult = " + delResult);
                console.info("KvStoreBackupManalBackupPromiseTest003 delResult[1] = " + delResult[1]);
                expect(0).assertEqual(delResult[1]);
                console.log("KvStoreBackupManalBackupPromiseTest003 publicdeleteBackup" + JSON.stringify(data));
                done();
            })
        } catch (e) {
            console.log("KvStoreBackupManalBackupPromiseTest003 fail 2 " + e);
            expect(e).assertFail();
            done();
        }
    })

    /**
     * @tc.name KvStoreBackupManalRestorePromiseTest004
     * @tc.desc Test Js Api SingleKvStore.backup() manal testcase 004
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreBackupManalBackupPromiseTest004', 0, async function (done) {
        try {
            console.log("KvStoreBackupManalBackupPromiseTest004 before getname");
            file  = '1.0' ;
            files[0] = file ;
            console.log("KvStoreBackupManalBackupPromiseTest004 before backup");
            publicbackup(kvStore,file) ;
            console.log("KvStoreBackupManalBackupPromiseTest004 going backup");
            await sleep(1000);
            await publicdeleteBackup(kvStore,files).then((data) => {
                let delResult = delresult[0];
                console.info("KvStoreBackupManalBackupPromiseTest004 delResult = " + delResult);
                console.info("KvStoreBackupManalBackupPromiseTest004 delResult[1] = " + delResult[1]);
                expect(0).assertEqual(delResult[1]);
                console.log("KvStoreBackupManalBackupPromiseTest004 publicdeleteBackup" + JSON.stringify(data));
                done();
            })
        } catch (e) {
            console.log("KvStoreBackupManalBackupPromiseTest004 fail 2 " + e);
            expect(e).assertFail();
            done();
        }
    })

    /**
     * @tc.name KvStoreBackupManalRestorePromiseTest005
     * @tc.desc Test Js Api SingleKvStore.backup() manal testcase 005
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreBackupManalBackupPromiseTest005', 0, async function (done) {
        try {
            console.log("KvStoreBackupManalBackupPromiseTest005 before getname");
            file  = '' ;
            console.log("KvStoreBackupManalBackupPromiseTest005 before backup");
            await publicbackup(kvStore,file).then((data) => {
                console.log("KvStoreBackupManalBackupPromiseTest005 going backup");
                expect(true).assertEqual(data == "code数字");
                done();
            }).catch((err) => {
                console.log("KvStoreBackupManalBackupPromiseTest005 fail1 " + err);
                console.log(JSON.stringify(err));
                expect(true).assertEqual(err.code == 401)
                done();
            })
        } catch (e) {
            console.log("KvStoreBackupManalBackupPromiseTest005 fail 2 " + e);
            expect(e).assertFail();
            done();
        }
    })

    /**
     * @tc.name KvStoreBackupManalRestorePromiseTest006
     * @tc.desc Test Js Api SingleKvStore.backup() manal testcase 006
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreBackupManalBackupPromiseTest006', 0, async function (done) {
        try {
            console.log("KvStoreBackupManalBackupPromiseTest006 before getname");
            file  = '1' ;
            var file1 = '2';
            var file2 = '3';
            var file3 = '4';
            var file4 = '5';
            files[0] = file ;
            files[1] = file1 ;
            files[2] = file2 ;
            files[3] = file3 ;
            files[4] = file4 ;
            console.log("KvStoreBackupManalBackupPromiseTest006 before backup");
            publicbackup(kvStore,file);
            await sleep(500);
            publicbackup(kvStore,file1);
            await sleep(500);
            publicbackup(kvStore,file2);
            await sleep(500);
            publicbackup(kvStore,file3);
            await sleep(500);
            publicbackup(kvStore,file4);
            await sleep(500);
            console.log("KvStoreBackupManalBackupPromiseTest006 before publicdeleteBackup");
            await sleep(1000);
            await publicdeleteBackup(kvStore,files).then((data) => {

                expect("1").assertEqual(delresult[0][0])
                expect(0).assertEqual(delresult[0][1]);

                expect("2").assertEqual(delresult[1][0])
                expect(0).assertEqual(delresult[1][1])

                expect("3").assertEqual(delresult[2][0])
                expect(0).assertEqual(delresult[2][1])

                expect("4").assertEqual(delresult[3][0])
                expect(0).assertEqual(delresult[3][1])

                expect("5").assertEqual(delresult[4][0])
                expect(27459591).assertEqual(delresult[4][1])

                console.log("KvStoreBackupManalBackupPromiseTest006 publicdeleteBackup" + JSON.stringify(data));
                done();
            })
        } catch (e) {
            console.log("KvStoreBackupManalBackupPromiseTest006 fail 2 " + e);
            expect(e).assertFail();
            done();
        }
    })

    /**
     * @tc.name KvStoreBackupManalRestorePromiseTest007
     * @tc.desc Test Js Api SingleKvStore.backup() manal testcase 007
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreBackupManalBackupPromiseTest007', 0, async function (done) {
        try {
            console.log("KvStoreBackupManalBackupPromiseTest007 before getname");
            files = [];
            var file  = '1';
            var file1 = '2';
            var file2 = '3';
            var file3 = '4';
            var file4 = '5';
            var file5 = '6';
            files[0] = file ;
            files[1] = file1 ;
            files[2] = file2 ;
            files[3] = file3 ;
            files[4] = file4 ;
            files[5] = file5 ;
            console.log("KvStoreBackupManalBackupPromiseTest007 before backup");
            publicbackup(kvStore,file);
            await sleep(500);
            publicbackup(kvStore,file1);
            await sleep(500);
            publicbackup(kvStore,file2);
            await sleep(500);
            publicbackup(kvStore,file3);
            await sleep(500);
            publicbackup(kvStore,file4);
            await sleep(500);
            console.log("KvStoreBackupManalBackupPromiseTest007 before Sixth backup");
            await publicbackup(kvStore,file5).then((data) => {
                console.log("KvStoreBackupManalBackupPromiseTest007 going backup");
                expect(true).assertEqual(data == "code数字");
                done();
            }).catch((err) => {
                console.log("KvStoreBackupManalBackupPromiseTest007  fail1 " + err);
                console.log("err is " + JSON.stringify(err) + "code is " + err.code);
                expect(true).assertEqual(JSON.stringify(err) == "{\"code\":\"\"}");
                console.log("KvStoreBackupManalBackupPromiseTest007 Sixth backup err");
            })
            await sleep(1000);
            console.log("KvStoreBackupManalBackupPromiseTest007 before publicdeleteBackup");
            await publicdeleteBackup(kvStore,files).then((data) => {

                expect("1").assertEqual(delresult[0][0])
                console.log(delresult[0][0]);
                console.log(delresult[0][1]);
                expect(0).assertEqual(delresult[0][1]);

                expect("5").assertEqual(delresult[4][0])
                expect(27459591).assertEqual(delresult[4][1]);

                expect("6").assertEqual(delresult[5][0])
                expect(27459591).assertEqual(delresult[5][1]);

                console.log("KvStoreBackupManalBackupPromiseTest007 publicdeleteBackup" + JSON.stringify(data));
                done();
            })
        } catch (e) {
            console.log("KvStoreBackupManalBackupPromiseTest007 fail 2 " + e);
            expect(e).assertFail();
            done();
        }
    })
    /**
     * @tc.name KvStoreBackupManalRestorePromiseTest001
     * @tc.desc Test Js Api SingleKvStore.restore() SpecifiedVerision testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreBackupManalRestoreSpecifiedVersionPromiseTest001', 0, async function (done) {
        try {
            console.log("KvStoreBackupManalRestoreSpecifiedVersionPromiseTest001 before getname");
            file  = 'legal' ;
            files[0] = file ;
            publicbackup(kvStore,file);
            console.log("KvStoreBackupManalRestoreSpecifiedVersionPromiseTest001 before restoresp");
            await sleep(1000);
            publicrestoresp(kvStore,file);
            console.log("KvStoreBackupManalRestoreSpecifiedVersionPromiseTest001 going restoresp");
            await sleep(1000);
            await publicdeleteBackup(kvStore,files).then((data) => {
                let delResult = delresult[5];
                console.info("KvStoreBackupManalRestoreSpecifiedVersionPromiseTest001 delResult = " + delResult);
                console.info("KvStoreBackupManalRestoreSpecifiedVersionPromiseTest001 delResult[1] = " + delResult[1]);
                expect(0).assertEqual(delResult[1]);
                console.log("KvStoreBackupManalRestoreSpecifiedVersionPromiseTest001 publicdeleteBackup" + JSON.stringify(data));
                done();
            })
        } catch (e) {
            console.log("KvStoreBackupManalRestoreSpecifiedVersionPromiseTest001 Manualrestoresp fail 2" + e);
            expect(e).assertFail();
            done();
        }
    })

    /**
     * @tc.name KvStoreBackupManalRestorePromiseTest002
     * @tc.desc Test Js Api SingleKvStore.restore() SpecifiedVerision testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreBackupManalRestoreSpecifiedVersionPromiseTest002', 0, async function (done) {
        try {
            console.log("KvStoreBackupManalRestoreSpecifiedVersionPromiseTest002 before getname");
            file  = 'true' ;
            files[0] = file ;
            publicbackup(kvStore,file);
            console.log("KvStoreBackupManalRestoreSpecifiedVersionPromiseTest002 before restoresp");
            await sleep(1000);
            publicrestoresp(kvStore,file);
            console.log("KvStoreBackupManalRestoreSpecifiedVersionPromiseTest002 going restoresp");
            await sleep(1000);
            await publicdeleteBackup(kvStore,files).then((data) => {
                let delResult = delresult[5];
                console.info("KvStoreBackupManalRestoreSpecifiedVersionPromiseTest002 delResult = " + delResult);
                console.info("KvStoreBackupManalRestoreSpecifiedVersionPromiseTest002 delResult[1] = " + delResult[1]);
                expect(0).assertEqual(delResult[1]);
                console.log("KvStoreBackupManalRestoreSpecifiedVersionPromiseTest002 publicdeleteBackup" + JSON.stringify(data));
                done();
            })
        } catch (e) {
            console.log("KvStoreBackupManalRestoreSpecifiedVersionPromiseTest002 fail 2" + e);
            expect(e).assertFail();
            done();
        }
    })

    /**
     * @tc.name KvStoreBackupManalRestorePromiseTest003
     * @tc.desc Test Js Api SingleKvStore.restore() SpecifiedVerision testcase 003
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreBackupManalRestoreSpecifiedVersionPromiseTest003', 0, async function (done) {
        try {
            console.log("KvStoreBackupManalRestoreSpecifiedVersionPromiseTest003 before getname");
            file  = '1' ;
            files[0] = file ;
            publicbackup(kvStore,file);
            console.log("KvStoreBackupManalRestoreSpecifiedVersionPromiseTest003 before restoresp");
            await sleep(1000);
            publicrestoresp(kvStore,file);
            console.log("KvStoreBackupManalRestoreSpecifiedVersionPromiseTest003 going restoresp");
            await sleep(1000);
            await publicdeleteBackup(kvStore,files).then((data) => {
                let delResult = delresult[0];
                console.info("KvStoreBackupManalRestoreSpecifiedVersionPromiseTest003 delResult = " + delResult);
                console.info("KvStoreBackupManalRestoreSpecifiedVersionPromiseTest003 delResult[1] = " + delResult[1]);
                expect(0).assertEqual(delResult[1]);
                console.log("KvStoreBackupManalRestoreSpecifiedVersionPromiseTest003 publicdeleteBackup" + JSON.stringify(data));
                done();
            })
        } catch (e) {
            console.log("KvStoreBackupManalRestoreSpecifiedVersionPromiseTest003 fail 2" + e);
            expect(e).assertFail();
            done();
        }
    })

    /**
     * @tc.name KvStoreBackupManalRestorePromiseTest004
     * @tc.desc Test Js Api SingleKvStore.restore() SpecifiedVerision testcase 004
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreBackupManalRestoreSpecifiedVersionPromiseTest004', 0, async function (done) {
        try {
            console.log("KvStoreBackupManalRestoreSpecifiedVersionPromiseTest004 before getname");
            file  = '1.0' ;
            files[0] = file ;
            publicbackup(kvStore,file);
            console.log("KvStoreBackupManalRestoreSpecifiedVersionPromiseTest004 before restoresp");
            await sleep(1000);
            publicrestoresp(kvStore,file);
            console.log("KvStoreBackupManalRestoreSpecifiedVersionPromiseTest004 going restoresp");
            await sleep(1000);
            await publicdeleteBackup(kvStore,files).then((data) => {
                let delResult = delresult[0];
                console.info("KvStoreBackupManalRestoreSpecifiedVersionPromiseTest004 delResult = " + delResult);
                console.info("KvStoreBackupManalRestoreSpecifiedVersionPromiseTest004 delResult[1] = " + delResult[1]);
                expect(0).assertEqual(delResult[1]);
                console.log("KvStoreBackupManalRestoreSpecifiedVersionPromiseTest004 publicdeleteBackup" + JSON.stringify(data));
                done();
            })
        } catch (e) {
            console.log("KvStoreBackupManalRestoreSpecifiedVersionPromiseTest004 Manualrestoresp fail 2" + e);
            expect(e).assertFail();
            done();
        }
    })

    /**
     * @tc.name KvStoreBackupManalRestorePromiseTest005
     * @tc.desc Test Js Api SingleKvStore.restore() SpecifiedVerision testcase 005
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreBackupManalRestoreSpecifiedVersionPromiseTest005', 0, async function (done) {
        try {
            console.log("KvStoreBackupManalRestoreSpecifiedVersionPromiseTest005 before getname");
            file  = '' ;
            publicbackup(kvStore,file) ;
            console.log("KvStoreBackupManalRestoreSpecifiedVersionPromiseTest005 before restoresp");
            await sleep(1000);
            await publicrestoresp(kvStore,file).then((data) => {
                console.log("KvStoreBackupManalRestoreSpecifiedVersionPromiseTest005 going restoresp = " + JSON.stringify(data));
                expect(true).assertEqual(false);
                done();
            }).catch((err) => {
                console.log("KvStoreBackupManalRestoreSpecifiedVersionPromiseTest005 Manualrestoresp fail 1" + err);
                expect(true).assertEqual(err.code == 401);
                done();
            })
        } catch (e) {
            console.log("KvStoreBackupManalRestoreSpecifiedVersionPromiseTest005 Manualrestoresp fail 2" + e);
            expect(e).assertFail();
            done();
        }
    })

    /**
     * @tc.name KvStoreBackupManalRestorePromiseTest006
     * @tc.desc Test Js Api SingleKvStore.restore() SpecifiedVerision testcase 006
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreBackupManalRestoreSpecifiedVersionPromiseTest006', 0, async function (done) {
        try {
            console.log("KvStoreBackupManalRestoreSpecifiedVersionPromiseTest006 before getname");
            file  = 'legal' ;
            console.log("KvStoreBackupManalRestoreSpecifiedVersionPromiseTest006 before restoresp");
            await publicrestoresp(kvStore,file).then((data) => {
                console.log("KvStoreBackupManalRestoreSpecifiedVersionPromiseTest006 going restoresp = " + JSON.stringify(data));
                expect(true).assertEqual(false);
                done();
            }).catch((err) => {
                console.log("KvStoreBackupManalRestoreSpecifiedVersionPromiseTest006 Manualrestoresp fail 1" + err);
                expect(true).assertEqual(err.code == 401);
                done();
            })
        } catch (e) {
            console.log("KvStoreBackupManalRestoreSpecifiedVersionPromiseTest006 Manualrestoresp fail 2" + e);
            expect(e).assertFail();
            done();
        }
    })



    /**
     * @tc.name KvStoreBackupDeleteBackupPromiseTest001
     * @tc.desc Test Js Api SingleKvStore.deleteBackup() testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreBackupDeleteBuckupPromiseTest001', 0, async function (done) {
        try {
            console.log("KvStoreBackupDeleteBuckupPromiseTest001 before deleteBackup");
            file  = '123' ;
            files[0] = file ;
            await publicdeleteBackup(kvStore,files).then((data) => {
                let delResult = delresult[0];
                console.info("KvStoreBackupDeleteBuckupPromiseTest001 delResult = " + delResult);
                console.info("KvStoreBackupDeleteBuckupPromiseTest001 delResult[1] = " + delResult[1]);
                expect(27459591).assertEqual(delResult[1]);
                console.log("KvStoreBackupDeleteBuckupPromiseTest001 publicdeleteBackup" + JSON.stringify(data));
                done();
            })
            console.log("KvStoreBackupDeleteBuckupPromiseTest001 going deleteBackup");
        } catch (err) {
            console.log("KvStoreBackupDeleteBuckupPromiseTest001 deleteBackup fail 2" + err);
            expect(err).assertFail();
            done();
        }
    })

    /**
     * @tc.name KvStoreBackupDbBackupPutPromiseTest001
     * @tc.desc Test Js Api SingleKvStore.put() db testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreBackupDbBuckupPutPromiseTest001', 0, async function (done) {
        try {
            console.log("KvStoreBackupDbBuckupPutPromiseTest001 before putdata");
            publicput(kvStore,"key1","value1") ;
            console.log("KvStoreBackupDbBuckupPutPromiseTest001 going putdata");
            done();
        } catch (e) {
            console.log("KvStoreBackupDbBuckupPutPromiseTest001 Backupinfo fail" + e);
            expect(e).assertFail();
            done();
        }
    })

    /**
     * @tc.name KvStoreBackupDbBackupPutPromiseTest002
     * @tc.desc Test Js Api SingleKvStore.put() db testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreBackupDbBuckupPutPromiseTest002', 0, async function (done) {
        try {
            console.log("KvStoreBackupDbBuckupPutPromiseTest002 before putdata");
            publicput(kvStore,"PutPromise0002","value") ;
            console.log("KvStoreBackupDbBuckupPutPromiseTest002 going putdata");
            await publicget(kvStore,"PutPromise0002").then((data) => {
                console.log("KvStoreBackupDbBuckupPutPromiseTest002 going getdata" + JSON.stringify(data));
                expect(true).assertEqual(data == "value");
                done();
            }).catch((err) => {
                console.log("KvStoreBackupDbBuckupPutPromiseTest002 Get fail 1 " + err);
                expect(err).assertFail();
                done();
            })
        } catch (e) {
            console.log("KvStoreBackupDbBuckupPutPromiseTest002 Get fail 2" + e);
            expect(e).assertFail();
            done();
        }
    })

    /**
     * @tc.name KvStoreBackupDbBackupPutPromiseTest003
     * @tc.desc Test Js Api SingleKvStore.put() db testcase 003
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreBackupDbBuckupPutPromiseTest004', 0, async function (done) {
        try {
            console.log("KvStoreBackupDbBuckupPutPromiseTest004 before putdata");
            publicput(kvStore,"PutPromise0004","value1") ;
            console.log("KvStoreBackupestDbBuckupPutPromiseTest004t going putdata");
            await publicget(kvStore,"PutPromise").then((data) => {
                console.log("KvStoreBackupDbBuckupPutPromiseTest004 going getdata" + JSON.stringify(data));
                expect(true).assertEqual(JSON.stringify(data) == '{}');
                done();
            }).catch((err) => {
                console.log("KvStoreBackupDbBuckupPutPromiseTest004 Get fail 1 " + err);
                console.log(JSON.stringify(err));
                expect(true).assertEqual(err.code == 15100004);
                done();
            })
        } catch (e) {
            console.log("KvStoreBackupDbBuckupPutPromiseTest004 Get fail 2" + e);
            expect(e).assertFail();
            done();
        }
    })
})
