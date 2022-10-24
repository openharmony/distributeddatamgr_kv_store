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
let STORE_ID = 'kvstoreBackupCallback';

let mKVMgrConfig = {
    bundleName: BUNDLE_NAME,
    context : context
};

function sleep(ms) {
    return new Promise(resolve => setTimeout(resolve, ms));
}
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
        kvStore.backup(file, function(err, data){
            console.log("Test backup task =" + data);
            if (err != undefined) {
                console.log("Test backup err information: " + err );
                reject(err);
            }else{
                resolve(data);
            }
        })
    })
}

function publicdeleteBackup(kvStore,files){
    console.log(`Test deleteBackup ${JSON.stringify(files)}`)
    return new Promise(function(resolve, reject) {
        kvStore.deleteBackup(files, function(err, data){
            console.log("Test deleteBackup BackUpInfo =" + data);
            if (err != undefined) {
                console.log("Test deleteBackup err information: " + err );
                reject(err);
            }else{
                var devices =new Array();
                devices = data;
                delresult = devices;
                console.log("Test deleteBackup pass ");
                resolve(data);
            }
        })
    })
}

function publicrestoresp(kvStore,file){
    console.log(`Test restoresp ${JSON.stringify(file)}`)
    return new Promise(function(resolve, reject) {
        kvStore.restore(file, function(err, data){
            console.log("Test restoresp task =" + JSON.stringify(data));
            if (err != undefined) {
                console.log("Test restoresp err information: " + err );
                reject(err);
            }else{
                console.log("Test restoresp backupinfo information: " + JSON.stringify(data) );
                resolve(data);
            }
        })
    })
}

function publicrestore(kvStore){
    console.log(`Test restore `)
    return new Promise(function(resolve, reject) {
        kvStore.restore(function(err,data){
            console.log("Test restore task =" + JSON.stringify(data));
            if (err != undefined) {
                console.log("Test restore err information: " + err );
                reject(err);
            }else{
                console.log("Test restore backupinfo information: " + JSON.stringify(data) );
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

var files =new Array();

describe('kvStoreBackupCallbackJsunittest', function () {
    beforeAll( async function () {
        console.info('Test beforeAll: Prerequisites at the test suite level, ' +
        'which are executed before t he test suite is executed.');
        await publicgetKvStore(optionLock);
        await sleep(5000);
        console.info("Test kvstore = " + kvStore)
    })
    beforeEach( async function () {
        console.info('beforeEach: Prerequisites at the test case level,' +
        ' which are executed before each test case is executed.');
        await publicgetKvStore(optionLock);
    })
    afterEach( async function () {
        console.info('afterEach: Test case-level clearance conditions, ' +
        'which are executed after each test case is executed.');
        publicdeleteBackup(kvStore,files);
        publiccloseKvStore();
        files = []
        await sleep(5000);
    })
    afterAll(function () {
        console.info('afterAll: Test suite-level cleanup condition, ' +
        'which is executed after the test suite is executed');
        publiccloseKvStore();
        kvManager = null;
        console.info("Test kvstore = " + kvStore)
    })

    /**
     * @tc.name KvStoreBackupManalRestoreCallbackTest001
     * @tc.desc Test Js Api SingleKvStore.restore() manal testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreBackupManalRestoreCallbackTest001', 0, async function (done) {
        try{
            console.log("KvStoreBackupManalRestoreCallbackTest001 before restore");
            await publicrestore(kvStore).then((data) => {
                console.log("KvStoreBackupManalRestoreCallbackTest001 going restore = " + JSON.stringify(data));
                expect(true).assertEqual(true);
                done();
            }).catch((err) => {
                console.log("KvStoreBackupManalRestoreCallbackTest001 Manualrestore fail 1" + err);
                expect(true).assertEqual(err == "Error: Parameter error.The number of parameters is incorrect.");
                done();
            })
        } catch (e) {
            console.log("KvStoreBackupManalRestoreCallbackTest001 Manualrestore fail 2" + JSON.stringify(e));
            expect(e).assertFail();
            done();
        }
    })

    /**
     * @tc.name KvStoreBackupManalRestoreCallbackTest002
     * @tc.desc Test Js Api SingleKvStore.restore() manal testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreBackupManalRestoreCallbackTest002', 0, async function (done) {
        try {
            console.log("KvStoreBackupManalRestoreCallbackTest002 before getname");
            file  = '123' ;
            files[0] = file ;
            publicbackup(kvStore,file) ;
            console.log("KvStoreBackupManalRestoreCallbackTest002 before restore");
            await sleep(1000);
            publicrestore(kvStore);
            console.log("KvStoreBackupManalRestoreCallbackTest002 going restore ");
            await sleep(1000);
            await publicdeleteBackup(kvStore,files).then((data) => {
                let delResult = delresult[0];
                console.info("KvStoreBackupManalRestoreCallbackTest002 delResult = " + delResult);
                console.info("KvStoreBackupManalRestoreCallbackTest002 delResult[1] = " + delResult[1]);
                expect(0).assertEqual(delResult[1]);
                console.log("KvStoreBackupManalRestoreCallbackTest002 publicdeleteBackup" + JSON.stringify(data));
                done();
            })
        } catch (e) {
            console.log("KvStoreBackupManalRestoreCallbackTest002 fail 2" + e);
            expect(e).assertFail();
            done();
        }
    })


    /**
     * @tc.name KvStoreBackupManalRestoreCallbackTest001
     * @tc.desc Test Js Api SingleKvStore.backup() db testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreBackupDbBackupCallbackTest001', 0, async function (done) {
        try {
            console.log("KvStoreBackupDbBackupCallbackTest001 before getname");
            file  = 'true' ;
            files[0] = file ;
            console.log("KvStoreBackupDbBackupCallbackTest001 before backup");
            publicbackup(kvStore,file) ;
            console.log("KvStoreBackupDbBackupCallbackTest001 going backup");
            await sleep(1000);
            await publicdeleteBackup(kvStore,files).then((data) => {
                let delResult = delresult[0];
                console.info("KvStoreBackupDbBackupCallbackTest001 delResult = " + delResult);
                console.info("KvStoreBackupDbBackupCallbackTest001 delResult[1] = " + delResult[1]);
                expect(0).assertEqual(delResult[1]);
                console.log("KvStoreBackupDbBackupCallbackTest001 publicdeleteBackup" + JSON.stringify(data));
                done();
            })
        } catch (e) {
            console.log("KvStoreBackupDbBackupCallbackTest001  fail 2 " + e);
            expect(e).assertFail();
            done();
        }
    })

    /**
     * @tc.name KvStoreBackupManalRestoreCallbackTest001
     * @tc.desc Test Js Api SingleKvStore.backup() manal testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreBackupManalBackupCallbackTest001', 0, async function (done) {
        try{
            console.log("KvStoreBackupestManalBackupCallbackTest001t before getname");
            files = []
            file = 'legal' ;
            files[0] = "legal" ;
            console.log("KvStoreBackupManalBackupCallbackTest001 before backup");
            publicbackup(kvStore,file) ;
            console.log("KvStoreBackupManalBackupCallbackTest001 going backup");
            await sleep(1000);
            await publicdeleteBackup(kvStore,files).then((data) => {
                let delResult = delresult[0];
                console.info("KvStoreBackupManalBackupCallbackTest001 delResult = " + delResult);
                console.info("KvStoreBackupManalBackupCallbackTest001 delResult[1] = " + delResult[1]);
                expect(0).assertEqual(delResult[1]);
                console.log("KvStoreBackupManalBackupCallbackTest001 publicdeleteBackup" + JSON.stringify(data));
                done();
            })
        } catch (e) {
            console.log("KvStoreBackupManalBackupCallbackTest001 export fail 2 " + e);
            expect(e).assertFail();
            done();
        }
    })
    /**
     * @tc.name KvStoreBackupManalRestoreCallbackTest002
     * @tc.desc Test Js Api SingleKvStore.backup() manal testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreBackupManalBackupCallbackTest002', 0, async function (done) {
        try {
            console.log("KvStoreBackupManalBackupCallbackTest002 before getname");
            file  = '1' ;
            files[0] = file ;
            console.log("KvStoreBackupManalBackupCallbackTest002 before backup");
            publicbackup(kvStore,file) ;
            console.log("KvStoreBackupManalBackupCallbackTest002 going backup");
            await sleep(1000);
            await publicdeleteBackup(kvStore,files).then((data) => {
                let delResult = delresult[0];
                console.info("KvStoreBackupManalBackupCallbackTest002 delResult = " + delResult);
                console.info("KvStoreBackupManalBackupCallbackTest002 delResult[1] = " + delResult[1]);
                expect(0).assertEqual(delResult[1]);
                console.log("KvStoreBackupManalBackupCallbackTest002 publicdeleteBackup" + JSON.stringify(data));
                done();
            })
        } catch (e) {
            console.log("KvStoreBackupManalBackupCallbackTest002  fail 2 " + e);
            expect(e).assertFail();
            done();
        }
    })

    /**
     * @tc.name KvStoreBackupManalRestoreCallbackTest003
     * @tc.desc Test Js Api SingleKvStore.backup() manal testcase 003
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreBackupManalBackupCallbackTest003', 0, async function (done) {
        try {
            console.log("KvStoreBackupManalBackupCallbackTest003 before getname");
            file  = '1.0' ;
            files[0] = file ;
            console.log("KvStoreBackupManalBackupCallbackTest003 before backup");
            publicbackup(kvStore,file) ;
            console.log("KvStoreBackupManalBackupCallbackTest003 going backup");
            await sleep(1000);
            await publicdeleteBackup(kvStore,files).then((data) => {
                let delResult = delresult[0];
                console.info("KvStoreBackupManalBackupCallbackTest003 delResult = " + delResult);
                console.info("KvStoreBackupManalBackupCallbackTest003 delResult[1] = " + delResult[1]);
                expect(0).assertEqual(delResult[1]);
                console.log("KvStoreBackupManalBackupCallbackTest003 publicdeleteBackup" + JSON.stringify(data));
                done();
            })
        } catch (e) {
            console.log("KvStoreBackupManalBackupCallbackTest003 fail 2 " + e);
            expect(e).assertFail();
            done();
        }
    })

    /**
     * @tc.name KvStoreBackupManalRestoreCallbackTest004
     * @tc.desc Test Js Api SingleKvStore.backup() testcase 004
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreBackupManalBackupCallbackTest004', 0, async function (done) {
        try {
            console.log("KvStoreBackupManalBackupCallbackTest004 before getname");
            file  = '' ;
            console.log("KvStoreBackupManalBackupCallbackTest004 before backup");
            await publicbackup(kvStore,file).then((data) => {
                console.log("KvStoreBackupManalBackupCallbackTest004 going backup");
                expect(true).assertEqual(data == "code数字");
                done();
            }).catch((err) => {
                console.log("KvStoreBackupManalBackupCallbackTest004 ManualbackupCallback002 fail1 " + err);
                expect(true).assertEqual(err.code == 401);
                done();
            })
        } catch (e) {
            console.log("KvStoreBackupManalBackupCallbackTest004 fail 2 " + e);
            expect(e).assertFail();
            done();
        }
    })

    /**
     * @tc.name KvStoreBackupManalRestoreCallbackTest005
     * @tc.desc Test Js Api SingleKvStore.backup() testcase 005
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreBackupManalBackupCallbackTest005', 0, async function (done) {
        try {
            console.log("KvStoreBackupManalBackupCallbackTest005 before getname");
            files = []
            var file  = '1';
            var file1 = '2';
            var file2 = '3';
            var file3 = '4';
            var file4 = '5';
            files[0] = file ;
            files[1] = file1 ;
            files[2] = file2 ;
            files[3] = file3 ;
            files[4] = file4 ;
            console.log("KvStoreBackupManalBackupCallbackTest005 before backup");
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

            console.log("KvStoreBackupManalBackupCallbackTest005 before publicdeleteBackup");
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

                console.log("KvStoreBackupManalBackupCallbackTest005 publicdeleteBackup" + JSON.stringify(data));
				files = [];
                console.log("Test clear files");
                done();
            })
        } catch (e) {
            console.log("KvStoreBackupManalBackupCallbackTest005 fail 2 " + e);
            expect(e).assertFail();
            done();
        }
    })

    /**
     * @tc.name KvStoreBackupManalRestoreCallbackTest006
     * @tc.desc Test Js Api SingleKvStore.backup() testcase 006
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreBackupManalBackupCallbackTest006', 0, async function (done) {
        try {
            console.log("KvStoreBackupManalBackupCallbackTest006 before getname");
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
            console.log("KvStoreBackupManalBackupCallbackTest006 before backup");
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
            console.log("KvStoreBackupManalBackupCallbackTest006 before Sixth backup");
            await publicbackup(kvStore,file5).then((data) => {
                console.log("KvStoreBackupManalBackupCallbackTest006 going backup");
                expect(true).assertEqual(data == "code数字");
                done();
            }).catch((err) => {
                console.log("KvStoreBackupManalBackupCallbackTest006 ManualbackupCallback002 fail1 " + err);
                console.log(JSON.stringify(err));
                expect(true).assertEqual(JSON.stringify(err) == "{\"code\":\"\"}");
                console.log("KvStoreBackupManalBackupCallbackTest006 Sixth backup err");
            })
            await sleep(1000);
            console.log("KvStoreBackupManalBackupCallbackTest006 before publicdeleteBackup");
            await publicdeleteBackup(kvStore,files).then((data) => {

                expect("1").assertEqual(delresult[0][0])
                expect(0).assertEqual(delresult[0][1]);

                expect("5").assertEqual(delresult[4][0])
                expect(27459591).assertEqual(delresult[4][1]);

                expect("6").assertEqual(delresult[5][0])
                expect(27459591).assertEqual(delresult[5][1]);

                console.log("KvStoreBackupManalBackupCallbackTest006 publicdeleteBackup" + JSON.stringify(data));
				files = [];
                console.log("Test clear files");
                done();
            })
        } catch (e) {
            console.log("KvStoreBackupManalBackupCallbackTest006 fail 2 " + e);
            expect(e).assertFail();
            done();
        }
    })

    /**
     * @tc.name KvStoreBackupManalRestoreCallbackTest001
     * @tc.desc Test Js Api SingleKvStore.restore() SpecifiedVerision testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreBackupManalRestoreSpecifiedVerisionCallbackTest001', 0, async function (done) {
        try {
            console.log("KvStoreBackupManalRestoreSpecifiedVerisionCallbackTest001 before getname");
            file  = 'legal' ;
            files[0] = file ;
            publicbackup(kvStore,file);
            console.log("KvStoreBackupManalRestoreSpecifiedVerisionCallbackTest001 before restoresp");
            await sleep(1000);
            publicrestoresp(kvStore,file);
            console.log("KvStoreBackupManalRestoreSpecifiedVerisionCallbackTest001 going restoresp");
            await sleep(1000);
            await publicdeleteBackup(kvStore,files).then((data) => {
                let delResult = delresult[0];
                console.info("KvStoreBackupManalRestoreSpecifiedVerisionCallbackTest001 delResult = " + delResult);
                console.info("KvStoreBackupManalRestoreSpecifiedVerisionCallbackTest001 delResult[1] = " + delResult[1]);
                expect(0).assertEqual(delResult[1]);
                console.log("KvStoreBackupManalRestoreSpecifiedVerisionCallbackTest001 publicdeleteBackup" + JSON.stringify(data));
                done();
            })
        } catch (e) {
            console.log("KvStoreBackupManalRestoreSpecifiedVerisionCallbackTest001 fail 2" + e);
            expect(e).assertFail();
            done();
        }
    })

    /**
     * @tc.name KvStoreBackupManalRestoreCallbackTest002
     * @tc.desc Test Js Api SingleKvStore.restore() SpecifiedVerision testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreBackupManalRestoreSpecifiedVerisionCallbackTest002', 0, async function (done) {
        try {
            console.log("KvStoreBackupManalRestoreSpecifiedVerisionCallbackTest002 before getname");
            file  = 'true' ;
            files[0] = file ;
            publicbackup(kvStore,file);
            console.log("KvStoreBackupManalRestoreSpecifiedVerisionCallbackTest002 before restoresp");
            await sleep(1000);
            publicrestoresp(kvStore,file);
            console.log("KvStoreBackupManalRestoreSpecifiedVerisionCallbackTest002 going restoresp");
            await sleep(1000);
            await publicdeleteBackup(kvStore,files).then((data) => {
                let delResult = delresult[0];
                console.info("KvStoreBackupManalRestoreSpecifiedVerisionCallbackTest002 delResult = " + delResult);
                console.info("KvStoreBackupManalRestoreSpecifiedVerisionCallbackTest002 delResult[1] = " + delResult[1]);
                expect(0).assertEqual(delResult[1]);
                console.log("KvStoreBackupManalRestoreSpecifiedVerisionCallbackTest002 publicdeleteBackup" + JSON.stringify(data));
                done();
            })
        } catch (e) {
            console.log("KvStoreBackupManalRestoreSpecifiedVerisionCallbackTest002 fail 2" + e);
            expect(e).assertFail();
            done();
        }
    })

    /**
     * @tc.name KvStoreBackupManalRestoreCallbackTest003
     * @tc.desc Test Js Api SingleKvStore.restore() SpecifiedVerision testcase 003
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreBackupManalRestoreSpecifiedVerisionCallbackTest003', 0, async function (done) {
        try {
            console.log("KvStoreBackupManalRestoreSpecifiedVerisionCallbackTest003 before getname");
            file  = '1' ;
            files[0] = file ;
            publicbackup(kvStore,file);
            console.log("KvStoreBackupManalRestoreSpecifiedVerisionCallbackTest003 before restoresp");
            await sleep(1000);
            publicrestoresp(kvStore,file);
            console.log("KvStoreBackupManalRestoreSpecifiedVerisionCallbackTest003 going restoresp");
            await sleep(1000);
            await publicdeleteBackup(kvStore,files).then((data) => {
                let delResult = delresult[0];
                console.info("KvStoreBackupManalRestoreSpecifiedVerisionCallbackTest003 delResult = " + delResult);
                console.info("KvStoreBackupManalRestoreSpecifiedVerisionCallbackTest003 delResult[1] = " + delResult[1]);
                expect(0).assertEqual(delResult[1]);
                console.log("KvStoreBackupManalRestoreSpecifiedVerisionCallbackTest003 publicdeleteBackup" + JSON.stringify(data));
                done();
            })
        } catch (e) {
            console.log("KvStoreBackupManalRestoreSpecifiedVerisionCallbackTest003 fail 2" + e);
            expect(e).assertFail();
            done();
        }
    })

    /**
     * @tc.name KvStoreBackupManalRestoreCallbackTest004
     * @tc.desc Test Js Api SingleKvStore.restore() SpecifiedVerision testcase 004
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreBackupManalRestoreSpecifiedVerisionCallbackTest004', 0, async function (done) {
        try {
            console.log("KvStoreBackupManalRestoreSpecifiedVerisionCallbackTest004 before getname");
            file  = '1.0' ;
            files[0] = file ;
            publicbackup(kvStore,file);
            console.log("KvStoreBackupManalRestoreSpecifiedVerisionCallbackTest004 before restoresp");
            await sleep(1000);
            publicrestoresp(kvStore,file);
            console.log("KvStoreBackupManalRestoreSpecifiedVerisionCallbackTest004 going restoresp");
            await sleep(1000);
            await publicdeleteBackup(kvStore,files).then((data) => {
                let delResult = delresult[0];
                console.info("KvStoreBackupManalRestoreSpecifiedVerisionCallbackTest004 delResult = " + delResult);
                console.info("KvStoreBackupManalRestoreSpecifiedVerisionCallbackTest004 delResult[1] = " + delResult[1]);
                expect(0).assertEqual(delResult[1]);
                console.log("KvStoreBackupManalRestoreSpecifiedVerisionCallbackTest004 publicdeleteBackup" + JSON.stringify(data));
                done();
            })
        } catch (e) {
            console.log("KvStoreBackupManalRestoreSpecifiedVerisionCallbackTest004 fail 2" + e);
            expect(e).assertFail();
            done();
        }
    })

    /**
     * @tc.name KvStoreBackupManalRestoreCallbackTest005
     * @tc.desc Test Js Api SingleKvStore.restore() SpecifiedVerision testcase 005
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreBackupManalRestoreSpecifiedVerisionCallbackTest005', 0, async function (done) {
        try {
            console.log("KvStoreBackupManalRestoreSpecifiedVerisionCallbackTest005 before getname");
            file  = '' ;
            publicbackup(kvStore,file) ;
            console.log("KvStoreBackupManalRestoreSpecifiedVerisionCallbackTest005 before restoresp");
            await sleep(1000);
            await publicrestoresp(kvStore,file).then((data) => {
                console.log("KvStoreBackupManalRestoreSpecifiedVerisionCallbackTest005 going restoresp = " + JSON.stringify(data));
                expect(true).assertEqual(false);
                done();
            }).catch((err) => {
                console.log("KvStoreBackupManalRestoreSpecifiedVerisionCallbackTest005 fail 1" + err);
                expect(true).assertEqual(err.code == 401);
                done();
            })
        } catch (e) {
            console.log("KvStoreBackupManalRestoreSpecifiedVerisionCallbackTest005 fail 2" + e);
            expect(e).assertFail();
            done();
        }
    })

    /**
     * @tc.name KvStoreBackupManalRestoreCallbackTest006
     * @tc.desc Test Js Api SingleKvStore.restore() SpecifiedVerision testcase 006
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreBackupManalRestoreSpecifiedVerisionCallbackTest006', 0, async function (done) {
        try {
            console.log("KvStoreBackupManalRestoreSpecifiedVerisionCallbackTest006 before getname");
            file  = 'legal' ;
            console.log("KvStoreBackupManalRestoreSpecifiedVerisionCallbackTest006 before restoresp");
            await publicrestoresp(kvStore,file).then((data) => {
                console.log("KvStoreBackupManalRestoreSpecifiedVerisionCallbackTest006 going restoresp = " + JSON.stringify(data));
                expect(true).assertEqual(false);
                done();
            }).catch((err) => {
                console.log("KvStoreBackupManalRestoreSpecifiedVerisionCallbackTest006 fail 1" + err);
                expect(true).assertEqual(err.code == 401);
                done();
            })
        } catch (e) {
            console.log("KvStoreBackupManalRestoreSpecifiedVerisionCallbackTest006 fail 2" + e);
            expect(e).assertFail();
            done();
        }
    })

    /**
     * @tc.name KvStoreBackupDeleteBackupCallbackTest001
     * @tc.desc Test Js Api SingleKvStore.deleteBackup() testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreBackupDeleteBackupCallbackTest001', 0, async function (done) {
        try {
            console.log("KvStoreBackupDeleteBackupCallbackTest001 before deleteBackup");
            file  = '123' ;
            files[0] = file ;
            await publicdeleteBackup(kvStore,files).then((data) => {
                let delResult = delresult[0];
                console.info("KvStoreBackupDeleteBackupCallbackTest001 delResult = " + delResult);
                console.info("KvStoreBackupDeleteBackupCallbackTest001 delResult[1] = " + delResult[1]);
                expect(27459591).assertEqual(delResult[1]);
                console.log("KvStoreBackupDeleteBackupCallbackTest001 publicdeleteBackup" + JSON.stringify(data));
                done();
            })
            console.log("KvStoreBackupDeleteBackupCallbackTest001 going deleteBackup");
        } catch (err) {
            console.log("KvStoreBackupDeleteBackupCallbackTest001 fail 2" + err);
            expect(err).assertFail();
            done();
        }
    })

    /**
     * @tc.name KvStoreBackupDbBackupPutCallbackTest001
     * @tc.desc Test Js Api SingleKvStore.put() db testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreBackupDbBackupPutCallbackTest001', 0, async function (done) {
        try {
            console.log("KvStoreBackupDbBackupPutCallbackTest001 before putdata");
            publicput(kvStore,"key","value") ;
            console.log("KvStoreBackupDbBackupPutCallbackTest001 going putdata");
            done();
        } catch (e) {
            console.log("KvStoreBackupDbBackupPutCallbackTest001 Backupinfo fail" + e);
            expect(e).assertFail();
            done();
        }
    })

    /**
     * @tc.name KvStoreBackupDbBackupPutCallbackTest002
     * @tc.desc Test Js Api SingleKvStore.put() db testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreBackupDbBackupPutCallbackTest002', 0, async function (done) {
        try {
            console.log("KvStoreBackupDbBackupPutCallbackTest002 before putdata");
            publicput(kvStore,"key","value") ;
            console.log("KvStoreBackupDbBackupPutCallbackTest002 going putdata");
            await publicget(kvStore,"key").then((data) => {
                console.log("KvStoreBackupDbBackupPutCallbackTest002 going getdata" + JSON.stringify(data));
                expect(true).assertEqual(data == "value");
                done();
            }).catch((err) => {
                console.log("KvStoreBackupDbBackupPutCallbackTest002 Get fail 1 " + err);
                expect(err).assertFail();
                done();
            })
        } catch (e) {
            console.log("KvStoreBackupDbBackupPutCallbackTest002 Get fail 2" + e);
            expect(e).assertFail();
            done();
        }
    })

    /**
     * @tc.name KvStoreBackupDbBackupPutCallbackTest003
     * @tc.desc Test Js Api SingleKvStore.put() db testcase 003
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('KvStoreBackupDbBackupPutCallbackTest003', 0, async function (done) {
        try {
            console.log("KvStoreBackupDbBackupPutCallbackTest003 before putdata");
            publicput(kvStore,"putcallback003","value1") ;
            console.log("KvStoreBackupDbBackupPutCallbackTest003 going putdata");
            await publicget(kvStore,"putcallback").then((data) => {
                console.log("KvStoreBackupDbBackupPutCallbackTest003 going getdata" + JSON.stringify(data));
                expect(true).assertEqual(false);
                done();
            }).catch((err) => {
                console.log("KvStoreBackupDbBackupPutCallbackTest003 Get fail 1 " + err);
                expect(true).assertEqual(true);
                done();
            })
        } catch (e) {
            console.log("KvStoreBackupDbBackupPutCallbackTest003 Get fail 2" + e);
            expect(e).assertFail();
            done();
        }
    })
})
