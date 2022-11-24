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
import factory from '@ohos.data.distributedData';

describe('singleKvStoreEnumTest', function () {

    /**
     * @tc.name SingleKvStoreEnumUsertypeSameUserIdTest001
     * @tc.desc  Test Js Enum Value Usertype.SAME_USER_ID
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreEnumUsertypeSameUserIdTest001', 0, function () {
        var sameUserId = factory.UserType.SAME_USER_ID;
        console.info('sameUserId = ' + sameUserId);
        expect(sameUserId == 0).assertTrue()
    })

    /**
     * @tc.name SingleKvStoreEnumConstantsMaxKeyLengthTest001
     * @tc.desc  Test Js Enum Value Constants.MAX_KEY_LENGTH
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreEnumConstantsMaxKeyLengthTest001', 0, function () {
        var maxKeyLength = factory.Constants.MAX_KEY_LENGTH;
        console.info('maxKeyLength = ' + maxKeyLength);
        expect(maxKeyLength == 1024).assertTrue()
    })

    /**
     * @tc.name SingleKvStoreEnumConstantsMaxValueLengthTest001
     * @tc.desc  Test Js Enum Value Constants.MAX_VALUE_LENGTH
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreEnumConstantsMaxValueLengthTest001', 0, function () {
        var maxValueLength = factory.Constants.MAX_VALUE_LENGTH;
        console.info('maxValueLength = ' + maxValueLength);
        expect(maxValueLength == 4194303).assertTrue();
        expect(factory.Constants.MAX_VALUE_LENGTH).assertEqual(4194303);
    })

    /**
     * @tc.name SingleKvStoreEnumConstantsMaxValueLengthTest002
     * @tc.desc  Test Js Enum Value Constants.MAX_VALUE_LENGTH
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreEnumConstantsMaxValueLengthTest002', 0, function () {
        try {
            factory.Constants.MAX_VALUE_LENGTH = 123;
        } catch (e) {
            console.info('can NOT set value to MAX_VALUE_LENGTH : ' + e);
            expect(factory.Constants.MAX_VALUE_LENGTH).assertEqual(4194303);
        }
    })

    /**
     * @tc.name SingleKvStoreEnumConstantsMaxKeyLengthDeviceTest001
     * @tc.desc  Test Js Enum Value Constants.MAX_KEY_LENGTH_DEVICE
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreEnumConstantsMaxKeyLengthDeviceTest001', 0, function () {
        var maxKeyLengthDevice = factory.Constants.MAX_KEY_LENGTH_DEVICE;
        console.info('maxKeyLengthDevice = ' + maxKeyLengthDevice);
        expect(maxKeyLengthDevice == 896).assertTrue()
    })

    /**
     * @tc.name SingleKvStoreEnumConstantsMaxStoreIdLengthTest001
     * @tc.desc  Test Js Enum Value Constants.MAX_STORE_ID_LENGTH
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreEnumConstantsMaxStoreIdLengthTest001', 0, function () {
        var maxStoreIdLength = factory.Constants.MAX_STORE_ID_LENGTH;
        console.info('maxStoreIdLength = ' + maxStoreIdLength);
        expect(maxStoreIdLength == 128).assertTrue()
    })

    /**
     * @tc.name SingleKvStoreEnumConstantsMaxQueryLengthTest001
     * @tc.desc  Test Js Enum Value Constants.MAX_QUERY_LENGTH
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreEnumConstantsMaxQueryLengthTest001', 0, function () {
        var maxQueryLength = factory.Constants.MAX_QUERY_LENGTH;
        console.info('maxQueryLength = ' + maxQueryLength);
        expect(maxQueryLength == 512000).assertTrue()
    })

    /**
     * @tc.name SingleKvStoreEnumConstantsMaxBatchSizeTest001
     * @tc.desc  Test Js Enum Value Constants.MAX_BATCH_SIZE
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreEnumConstantsMaxBatchSizeTest001', 0, function () {
        var maxBatchSize = factory.Constants.MAX_BATCH_SIZE;
        console.info('maxBatchSize = ' + maxBatchSize);
        expect(maxBatchSize == 128).assertTrue()
    })

    /**
     * @tc.name SingleKvStoreEnumValuetypeStringTest001
     * @tc.desc  Test Js Enum Value Valuetype.STRING
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreEnumValuetypeStringTest001', 0, function () {
        var string = factory.ValueType.STRING;
        console.info('string = ' + string);
        expect(string == 0).assertTrue()
    })

    /**
     * @tc.name SingleKvStoreEnumValuetypeIntegerTest001
     * @tc.desc  Test Js Enum Value Valuetype.INTEGER
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreEnumValuetypeIntegerTest001', 0, function () {
        var integer = factory.ValueType.INTEGER;
        console.info('integer = ' + integer);
        expect(integer == 1).assertTrue()
    })

    /**
     * @tc.name SingleKvStoreEnumValuetypeFloatTest001
     * @tc.desc  Test Js Enum Value Valuetype.FLOAT
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreEnumValuetypeFloatTest001', 0, function () {
        var float = factory.ValueType.FLOAT;
        console.info('float = ' + float);
        expect(float == 2).assertTrue()
    })

    /**
     * @tc.name SingleKvStoreEnumValuetypeByteArrayTest001
     * @tc.desc  Test Js Enum Value Valuetype.BYTEARRAY
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreEnumValuetypeByteArrayTest001', 0, function () {
        var byteArray = factory.ValueType.BYTE_ARRAY;
        console.info('byteArray = ' + byteArray);
        expect(byteArray == 3).assertTrue()
    })

    /**
     * @tc.name SingleKvStoreEnumValuetypeBooleanTest001
     * @tc.desc  Test Js Enum Value Valuetype.BOOLEAN
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreEnumValuetypeBooleanTest001', 0, function () {
        var boolean = factory.ValueType.BOOLEAN;
        console.info('boolean = ' + boolean);
        expect(boolean == 4).assertTrue()
    })

    /**
     * @tc.name SingleKvStoreEnumValuetypeDoubleTest001
     * @tc.desc  Test Js Enum Value Valuetype.DOUBLE
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreEnumValuetypeDoubleTest001', 0, function () {
        var double = factory.ValueType.DOUBLE;
        console.info('double = ' + double);
        expect(double == 5).assertTrue()
    })

    /**
     * @tc.name SingleKvStoreEnumSyncmodePullOnlyTest001
     * @tc.desc  Test Js Enum Value Syncmode.PULL_ONLY
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreEnumSyncmodePullOnlyTest001', 0, function () {
        var pullonly = factory.SyncMode.PULL_ONLY;
        console.info('pullonly = ' + pullonly);
        expect(pullonly == 0).assertTrue()
    })

    /**
     * @tc.name SingleKvStoreEnumSyncmodePushOnlyTest001
     * @tc.desc  Test Js Enum Value Syncmode.PUSH_ONLY
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreEnumSyncmodePushOnlyTest001', 0, function () {
        var pushonly = factory.SyncMode.PUSH_ONLY;
        console.info('pushonly = ' + pushonly);
        expect(pushonly == 1).assertTrue()
    })

    /**
     * @tc.name SingleKvStoreEnumSyncmodePushPullTest001
     * @tc.desc  Test Js Enum Value Syncmode.PUSH_PULL
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreEnumSyncmodePushPullTest001', 0, function () {
        var pushpull = factory.SyncMode.PUSH_PULL;
        console.info('pushpull = ' + pushpull);
        expect(pushpull == 2).assertTrue()
    })

    /**
     * @tc.name SingleKvStoreEnumSubscribetypeSubscribeTypeLocalTest001
     * @tc.desc  Test Js Enum Value Subscribetype.SUBSCRIBE_TYPE_LOCAL
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreEnumSubscribetypeSubscribeTypeLocalTest001', 0, function () {
        var local = factory.SubscribeType.SUBSCRIBE_TYPE_LOCAL;
        console.info('local = ' + local);
        expect(local == 0).assertTrue()
    })

    /**
     * @tc.name SingleKvStoreEnumSubscribetypeSubscribeTypeRemoteTest001
     * @tc.desc  Test Js Enum Value Subscribetype.SUBSCRIBE_TYPE_REMOTE
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreEnumSubscribetypeSubscribeTypeRemoteTest001', 0, function () {
        var remote = factory.SubscribeType.SUBSCRIBE_TYPE_REMOTE;
        console.info('remote = ' + remote);
        expect(remote == 1).assertTrue()
    })

    /**
     * @tc.name SingleKvStoreEnumSubscribetypeSubscribeTypeAllTest001
     * @tc.desc  Test Js Enum Value Subscribetype.SUBSCRIBE_TYPE_ALL
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreEnumSubscribetypeSubscribeTypeAllTest001', 0, function () {
        var all = factory.SubscribeType.SUBSCRIBE_TYPE_ALL;
        console.info('all = ' + all);
        expect(all == 2).assertTrue()
    })

    /**
     * @tc.name SingleKvStoreEnumKvstoretypeDeviceCollaborationTest001
     * @tc.desc  Test Js Enum Value Kvstoretype.DEVICE_COLLABORATION
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreEnumKvstoretypeDeviceCollaborationTest001', 0, function () {
        var collaboration = factory.KVStoreType.DEVICE_COLLABORATION;
        console.info('collaboration = ' + collaboration);
        expect(collaboration == 0).assertTrue()
    })

    /**
     * @tc.name SingleKvStoreEnumKvstoretypeSingleVersionTest001
     * @tc.desc  Test Js Enum Value Kvstoretype.SINGLE_VERSION
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreEnumKvstoretypeSingleVersionTest001', 0, function () {
        var single = factory.KVStoreType.SINGLE_VERSION;
        console.info('single = ' + single);
        expect(single == 1).assertTrue()
    })

    /**
     * @tc.name SingleKvStoreEnumKvstoretypeMultiVersionTest001
     * @tc.desc  Test Js Enum Value Kvstoretype.MULTI_VERSION
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreEnumKvstoretypeMultiVersionTest001', 0, function () {
        var multi = factory.KVStoreType.MULTI_VERSION;
        console.info('multi = ' + multi);
        expect(multi == 2).assertTrue()
    })

    /**
     * @tc.name SingleKvStoreEnumSecuritylevelNoLevelTest001
     * @tc.desc  Test Js Enum Value Securitylevel.NO_LEVEL
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreEnumSecuritylevelNoLevelTest001', 0, function () {
        var nolevel = factory.SecurityLevel.NO_LEVEL;
        console.info('nolevel = ' + nolevel);
        expect(nolevel == 0).assertTrue()
    })

    /**
     * @tc.name SingleKvStoreEnumSecuritylevelS0Test001
     * @tc.desc  Test Js Enum Value Securitylevel.S0
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreEnumSecuritylevelS0Test001', 0, function () {
        var s0 = factory.SecurityLevel.S0;
        console.info('s0 = ' + s0);
        expect(s0 == 1).assertTrue()
    })

    /**
     * @tc.name SingleKvStoreEnumSecuritylevelS1Test001
     * @tc.desc  Test Js Enum Value Securitylevel.S1
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreEnumSecuritylevelS1Test001', 0, function () {
        var s1 = factory.SecurityLevel.S1;
        console.info('s1 = ' + s1);
        expect(s1 == 2).assertTrue()
    })

    /**
     * @tc.name SingleKvStoreEnumSecuritylevelS2Test001
     * @tc.desc  Test Js Enum Value Securitylevel.S2
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreEnumSecuritylevelS2Test001', 0, function () {
        var s2 = factory.SecurityLevel.S2;
        console.info('s2 = ' + s2);
        expect(s2 == 3).assertTrue()
    })

    /**
     * @tc.name SingleKvStoreEnumSecuritylevelS3Test001
     * @tc.desc  Test Js Enum Value Securitylevel.S3
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreEnumSecuritylevelS3Test001', 0, function () {
        var s3 = factory.SecurityLevel.S3;
        console.info('s3 = ' + s3);
        expect(s3 == 5).assertTrue()
    })

    /**
     * @tc.name SingleKvStoreEnumSecuritylevelS4Test001
     * @tc.desc  Test Js Enum Value Securitylevel.S4
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreEnumSecuritylevelS4Test001', 0, function () {
        var s4 = factory.SecurityLevel.S4;
        console.info('s4 = ' + s4);
        expect(s4 == 6).assertTrue()
    })
})
