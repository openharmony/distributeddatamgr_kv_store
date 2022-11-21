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

describe('SingleKvStoreEnumTest', function () {

    /**
     * @tc.name SingleKvStoreEnumConstantsMaxKeyLengthSucTest
     * @tc.desc  Test Js Enum Value Constants.MAX_KEY_LENGTH successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreEnumConstantsMaxKeyLengthSucSucTest', 0, function () {
        try {
            var maxKeyLength = factory.Constants.MAX_KEY_LENGTH;
            console.info('maxKeyLength = ' + maxKeyLength);
            expect(maxKeyLength == 1024).assertTrue()
        } catch (e) {
            expect(null).assertFailed();
        }
    })

    /**
     * @tc.name SingleKvStoreEnumConstantsMaxValueLengthSucTest
     * @tc.desc  Test Js Enum Value Constants.MAX_VALUE_LENGTH successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreEnumConstantsMaxValueLengthSucSucTest', 0, function () {
        try {
            var maxValueLength = factory.Constants.MAX_VALUE_LENGTH;
            console.info('maxValueLength = ' + maxValueLength);
            expect(maxValueLength == 4194303).assertTrue();
            expect(factory.Constants.MAX_VALUE_LENGTH).assertEqual(4194303);
        } catch (e) {
            expect(null).assertFailed();
        }
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

        try {
            var maxKeyLengthDevice = factory.Constants.MAX_KEY_LENGTH_DEVICE;
            console.info('maxKeyLengthDevice = ' + maxKeyLengthDevice);
            expect(maxKeyLengthDevice == 896).assertTrue()
        } catch (e) {
            expect(null).assertFailed();
        }
    })

    /**
     * @tc.name SingleKvStoreEnumConstantsMaxStoreIdLengthSucTest
     * @tc.desc  Test Js Enum Value Constants.MAX_STORE_ID_LENGTH successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreEnumConstantsMaxStoreIdLengthSucTest', 0, function () {

        try {
            var maxStoreIdLength = factory.Constants.MAX_STORE_ID_LENGTH;
            console.info('maxStoreIdLength = ' + maxStoreIdLength);
            expect(maxStoreIdLength == 128).assertTrue()
        } catch (e) {
            expect(null).assertFailed();
        }
    })

    /**
     * @tc.name SingleKvStoreEnumConstantsMaxQueryLengthSucTest
     * @tc.desc  Test Js Enum Value Constants.MAX_QUERY_LENGTH successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreEnumConstantsMaxQueryLengthSucTest', 0, function () {

        try {
            var maxQueryLength = factory.Constants.MAX_QUERY_LENGTH;
            console.info('maxQueryLength = ' + maxQueryLength);
            expect(maxQueryLength == 512000).assertTrue()
        } catch (e) {
            expect(null).assertFailed();
        }
    })

    /**
     * @tc.name SingleKvStoreEnumConstantsMaxBatchSizeSucTest
     * @tc.desc  Test Js Enum Value Constants.MAX_BATCH_SIZE successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreEnumConstantsMaxBatchSizeSucTest', 0, function () {

        try {
            var maxBatchSize = factory.Constants.MAX_BATCH_SIZE;
            console.info('maxBatchSize = ' + maxBatchSize);
            expect(maxBatchSize == 128).assertTrue()
        } catch (e) {
            expect(null).assertFailed();
        }
    })

    /**
     * @tc.name SingleKvStoreEnumSecuritylevelS1SucTest
     * @tc.desc  Test Js Enum Value Securitylevel.S1 successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreEnumSecuritylevelS1SucTest', 0, function () {
        try {
            var s1 = factory.SecurityLevel.S1;
            console.info('s1 = ' + s1);
            expect(s1 == 2).assertTrue();
        } catch (e) {
            expect(null).assertFailed();
        }
    })

    /**
     * @tc.name SingleKvStoreEnumSecuritylevelS2SucTest
     * @tc.desc  Test Js Enum Value Securitylevel.S2 successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreEnumSecuritylevelS2SucTest', 0, function () {
        try {
            var s2 = factory.SecurityLevel.S2;
            console.info('s2 = ' + s2);
            expect(s2 == 3).assertTrue()
        } catch (e) {
            expect(null).assertFailed();
        }
    })

    /**
     * @tc.name SingleKvStoreEnumSecuritylevelS3SucTest
     * @tc.desc  Test Js Enum Value Securitylevel.S3 successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreEnumSecuritylevelS3SucTest', 0, function () {
        try {
            var s3 = factory.SecurityLevel.S3;
            console.info('s3 = ' + s3);
            expect(s3 == 5).assertTrue()
        } catch (e) {
            expect(null).assertFailed();
        }
    })

    /**
     * @tc.name SingleKvStoreEnumSecuritylevelS4SucTest
     * @tc.desc  Test Js Enum Value Securitylevel.S4 successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SingleKvStoreEnumSecuritylevelS4SucTest', 0, function () {
        try {
            var s4 = factory.SecurityLevel.S4;
            console.info('s4 = ' + s4);
            expect(s4 == 6).assertTrue()
        } catch (e) {
            expect(null).assertFailed();
        }
    })
})
