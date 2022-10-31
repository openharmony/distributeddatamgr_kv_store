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
import factory from '@ohos.data.distributedKVStore';

describe('SingleKvStoreEnumSucTestfunction',function () {

    /**
     * @tc.name SingleKvStoreEnumConstantsMaxKeyLengthSucTest
     * @tc.desc  Test Js Enum Value Constants.MAX_KEY_LENGTH successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreEnumConstantsMaxKeyLengthSucSucTest', 0, function () {
        var maxKeyLength = factory.Constants.MAX_KEY_LENGTH;
        console.info('maxKeyLength = ' + maxKeyLength);
        expect(maxKeyLength == 1024).assertTrue()
    })

    /**
     * @tc.name SingleKvStoreEnumConstantsMaxValueLengthSucTest
     * @tc.desc  Test Js Enum Value Constants.MAX_VALUE_LENGTH successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreEnumConstantsMaxValueLengthSucSucTest', 0, function () {
        var maxValueLength = factory.Constants.MAX_VALUE_LENGTH;
        console.info('maxValueLength = ' + maxValueLength);
        expect(maxValueLength == 4194303).assertTrue();
        expect(factory.Constants.MAX_VALUE_LENGTH).assertEqual(4194303);
    })

    /**
     * @tc.name SingleKvStoreEnumConstantsMaxValueLengthSucTest
     * @tc.desc  Test Js Enum Value Constants.MAX_VALUE_LENGTH successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreEnumConstantsMaxValueLengthSucTest', 0, function () {
        try {
            factory.Constants.MAX_VALUE_LENGTH = 123;
        } catch (e) {
            console.info('can NOT set value to MAX_VALUE_LENGTH : ' + e);
            expect(factory.Constants.MAX_VALUE_LENGTH).assertEqual(4194303);
        }
    })

    /**
     * @tc.name SingleKvStoreEnumConstantsMaxKeyLengthDeviceSucTest
     * @tc.desc  Test Js Enum Value Constants.MAX_KEY_LENGTH_DEVICE successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreEnumConstantsMaxKeyLengthDeviceSucTest', 0, function () {
        var maxKeyLengthDevice = factory.Constants.MAX_KEY_LENGTH_DEVICE;
        console.info('maxKeyLengthDevice = ' + maxKeyLengthDevice);
        expect(maxKeyLengthDevice == 896).assertTrue()
    })

    /**
     * @tc.name SingleKvStoreEnumConstantsMaxStoreIdLengthSucTest
     * @tc.desc  Test Js Enum Value Constants.MAX_STORE_ID_LENGTH successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreEnumConstantsMaxStoreIdLengthSucTest', 0, function () {
        var maxStoreIdLength = factory.Constants.MAX_STORE_ID_LENGTH;
        console.info('maxStoreIdLength = ' + maxStoreIdLength);
        expect(maxStoreIdLength == 128).assertTrue()
    })

    /**
     * @tc.name SingleKvStoreEnumConstantsMaxQueryLengthSucTest
     * @tc.desc  Test Js Enum Value Constants.MAX_QUERY_LENGTH successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreEnumConstantsMaxQueryLengthSucTest', 0, function () {
        var maxQueryLength = factory.Constants.MAX_QUERY_LENGTH;
        console.info('maxQueryLength = ' + maxQueryLength);
        expect(maxQueryLength == 512000).assertTrue()
    })

    /**
     * @tc.name SingleKvStoreEnumConstantsMaxBatchSizeSucTest
     * @tc.desc  Test Js Enum Value Constants.MAX_BATCH_SIZE successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreEnumConstantsMaxBatchSizeSucTest', 0, function () {
        var maxBatchSize = factory.Constants.MAX_BATCH_SIZE;
        console.info('maxBatchSize = ' + maxBatchSize);
        expect(maxBatchSize == 128).assertTrue()
    })

    /**
     * @tc.name SingleKvStoreEnumValuetypeStringSucTest
     * @tc.desc  Test Js Enum Value Valuetype.STRING successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreEnumValuetypeStringSucTest', 0, function () {
        var string = factory.ValueType.STRING;
        console.info('string = ' + string);
        expect(string == 0).assertTrue()
    })

    /**
     * @tc.name SingleKvStoreEnumValuetypeIntegerSucTest
     * @tc.desc  Test Js Enum Value Valuetype.INTEGER successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreEnumValuetypeIntegerSucTest', 0, function () {
        var integer = factory.ValueType.INTEGER;
        console.info('integer = ' + integer);
        expect(integer == 1).assertTrue()
    })

    /**
     * @tc.name SingleKvStoreEnumValuetypeFloatSucTest
     * @tc.desc  Test Js Enum Value Valuetype.FLOAT successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreEnumValuetypeFloatSucTest', 0, function () {
        var float = factory.ValueType.FLOAT;
        console.info('float = ' + float);
        expect(float == 2).assertTrue()
    })

    /**
     * @tc.name SingleKvStoreEnumValuetypeByteArraySucTest
     * @tc.desc  Test Js Enum Value Valuetype.BYTEARRAY successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreEnumValuetypeByteArraySucTest', 0, function () {
        var byteArray = factory.ValueType.BYTE_ARRAY;
        console.info('byteArray = ' + byteArray);
        expect(byteArray == 3).assertTrue()
    })

    /**
     * @tc.name SingleKvStoreEnumValuetypeBooleanSucTest
     * @tc.desc  Test Js Enum Value Valuetype.BOOLEAN successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreEnumValuetypeBooleanSucTest', 0, function () {
        var boolean = factory.ValueType.BOOLEAN;
        console.info('boolean = ' + boolean);
        expect(boolean == 4).assertTrue()
    })

    /**
     * @tc.name SingleKvStoreEnumValuetypeDoubleSucTest
     * @tc.desc  Test Js Enum Value Valuetype.DOUBLE successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreEnumValuetypeDoubleSucTest', 0, function () {
        var double = factory.ValueType.DOUBLE;
        console.info('double = ' + double);
        expect(double == 5).assertTrue()
    })

    /**
     * @tc.name SingleKvStoreEnumSyncmodePullOnlySucTest
     * @tc.desc  Test Js Enum Value Syncmode.PULL_ONLY successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreEnumSyncmodePullOnlySucTest', 0, function () {
        var pullonly = factory.SyncMode.PULL_ONLY;
        console.info('pullonly = ' + pullonly);
        expect(pullonly == 0).assertTrue()
    })

    /**
     * @tc.name SingleKvStoreEnumSyncmodePushOnlySucTest
     * @tc.desc  Test Js Enum Value Syncmode.PUSH_ONLY successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreEnumSyncmodePushOnlySucTest', 0, function () {
        var pushonly = factory.SyncMode.PUSH_ONLY;
        console.info('pushonly = ' + pushonly);
        expect(pushonly == 1).assertTrue()
    })

    /**
     * @tc.name SingleKvStoreEnumSyncmodePushPullSucTest
     * @tc.desc  Test Js Enum Value Syncmode.PUSH_PULL successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreEnumSyncmodePushPullSucTest', 0, function () {
        var pushpull = factory.SyncMode.PUSH_PULL;
        console.info('pushpull = ' + pushpull);
        expect(pushpull == 2).assertTrue()
    })

    /**
     * @tc.name SingleKvStoreEnumSubscribetypeSubscribeTypeLocalSucTest
     * @tc.desc  Test Js Enum Value Subscribetype.SUBSCRIBE_TYPE_LOCAL successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreEnumSubscribetypeSubscribeTypeLocalSucTest', 0, function () {
        var local = factory.SubscribeType.SUBSCRIBE_TYPE_LOCAL;
        console.info('local = ' + local);
        expect(local == 0).assertTrue()
    })

    /**
     * @tc.name SingleKvStoreEnumSubscribetypeSubscribeTypeRemoteSucTest
     * @tc.desc  Test Js Enum Value Subscribetype.SUBSCRIBE_TYPE_REMOTE successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreEnumSubscribetypeSubscribeTypeRemoteSucTest', 0, function () {
        var remote = factory.SubscribeType.SUBSCRIBE_TYPE_REMOTE;
        console.info('remote = ' + remote);
        expect(remote == 1).assertTrue()
    })

    /**
     * @tc.name SingleKvStoreEnumSubscribetypeSubscribeTypeAllSucTest
     * @tc.desc  Test Js Enum Value Subscribetype.SUBSCRIBE_TYPE_ALL successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreEnumSubscribetypeSubscribeTypeAllSucTest', 0, function () {
        var all = factory.SubscribeType.SUBSCRIBE_TYPE_ALL;
        console.info('all = ' + all);
        expect(all == 2).assertTrue()
    })

    /**
     * @tc.name SingleKvStoreEnumKvstoretypeDeviceCollaborationSucTest
     * @tc.desc  Test Js Enum Value Kvstoretype.DEVICE_COLLABORATION successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreEnumKvstoretypeDeviceCollaborationSucTest', 0, function () {
        var collaboration = factory.KVStoreType.DEVICE_COLLABORATION;
        console.info('collaboration = ' + collaboration);
        expect(collaboration == 0).assertTrue()
    })

    /**
     * @tc.name SingleKvStoreEnumKvstoretypeSingleVersionSucTest
     * @tc.desc  Test Js Enum Value Kvstoretype.SINGLE_VERSION successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreEnumKvstoretypeSingleVersionSucTest', 0, function () {
        var single = factory.KVStoreType.SINGLE_VERSION;
        console.info('single = ' + single);
        expect(single == 1).assertTrue()
    })

    /**
     * @tc.name SingleKvStoreEnumSecuritylevelS1SucTest
     * @tc.desc  Test Js Enum Value Securitylevel.S1 successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreEnumSecuritylevelS1SucTest', 0, function () {
        var s1 = factory.SecurityLevel.S1;
        console.info('s1 = ' + s1);
        expect(s1 == 2).assertTrue()
    })

    /**
     * @tc.name SingleKvStoreEnumSecuritylevelS2SucTest
     * @tc.desc  Test Js Enum Value Securitylevel.S2 successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreEnumSecuritylevelS2SucTest', 0, function () {
        var s2 = factory.SecurityLevel.S2;
        console.info('s2 = ' + s2);
        expect(s2 == 3).assertTrue()
    })

    /**
     * @tc.name SingleKvStoreEnumSecuritylevelS3SucTest
     * @tc.desc  Test Js Enum Value Securitylevel.S3 successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreEnumSecuritylevelS3SucTest', 0, function () {
        var s3 = factory.SecurityLevel.S3;
        console.info('s3 = ' + s3);
        expect(s3 == 5).assertTrue()
    })

    /**
     * @tc.name SingleKvStoreEnumSecuritylevelS4SucTest
     * @tc.desc  Test Js Enum Value Securitylevel.S4 successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreEnumSecuritylevelS4SucTest', 0, function () {
        var s4 = factory.SecurityLevel.S4;
        console.info('s4 = ' + s4);
        expect(s4 == 6).assertTrue()
    })
})
