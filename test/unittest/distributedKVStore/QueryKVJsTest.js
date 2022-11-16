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
import ddm from '@ohos.data.distributedKVStore';

describe('queryTest', function () {

    /**
     * @tc.name: QueryResetSucTest
     * @tc.desc: Test Js Api Query.reset() successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryResetSucTest', 0, async function (done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.equalTo("number", 5);
            query.equalTo("string", 'v');
            query.equalTo("boolean", false);
            console.info("query is " + query.getSqlLike());
            expect(query.getSqlLike() !== "").assertTrue();
            query.reset();
            expect("").assertEqual(query.getSqlLike());
            console.info("sql after  reset: " + query.getSqlLike());
            console.info("query is " + query.getSqlLike());
        } catch (e) {
            console.error("dumplicated calls should be ok : " + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        query = null;
        done();
    })

    /**
     * @tc.name: QueryResetCalAfterResetTest
     * @tc.desc: Test Js Api Query.reset() call after reset successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryResetCalAfterResetTest', 0, async function (done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.equalTo("key", "value");
            expect(query.getSqlLike() !== "").assertTrue();
            let sql = query.getSqlLike();
            query.reset().equalTo("key", "value");
            console.info("query is " + query.getSqlLike());
            expect(sql === query.getSqlLike()).assertTrue();
        } catch (e) {
            console.error("should be ok on Method Chaining : " + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        query = null;
        done();
    })

    /**
     * @tc.name: QueryResetInvalidArgumentsTest
     * @tc.desc: Test Js Api Query.reset() with invalid arguments
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryResetInvalidArgumentsTest', 0, async function (done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.equalTo("key", "value");
            expect(query.getSqlLike() !== "").assertTrue();
            query.reset(3);
            console.info("should throw exception on invalid arguments");
            expect(null).assertFail();
        } catch (e) {
            console.log("throw exception is ok");
            expect(true).assertTrue();
        }
        query = null;
        done();
    })

    /**
     * @tc.name: QueryEqualToSucTest
     * @tc.desc: Test Js Api Query.equalTo() successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryEqualToSucTest', 0, async function (done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.equalTo("key1", 5);
            query.equalTo("key2", 5.0);
            query.equalTo("key3", false);
            query.equalTo("key3", "string");
            expect(query.getSqlLike() !== "").assertTrue();
            console.info("query is " + query.getSqlLike());
        } catch (e) {
            console.error("dumplicated calls should be ok : " + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        query = null;
        done();
    })

    /**
     * @tc.name: QueryEqualToNanTest
     * @tc.desc: Test Js Api Query.equalTo() with value Nan
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryEqualToNanTest', 0, async function (done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.equalTo("key2", NaN);
            expect(query.getSqlLike() !== "").assertTrue();
            console.info("query is " + query.getSqlLike());
        } catch (e) {
            expect(null).assertFail();
        }
        query = null;
        done();
    })

    /**
     * @tc.name: QueryEqualToInvalidMoreArgTest
     * @tc.desc: Test Js Api Query.equalTo() with invalid more arguments
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryEqualToInvalidArgTest', 0, async function (done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.equalTo("key1", "value", "too more");
            console.info("should not throw exception on invalid arguments");
            expect(query.getSqlLike() !== "").assertTrue();
        } catch (e) {
            console.error("throw exception : " + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        query = null;
        done();
    })

    /**
     * @tc.name: QueryEqualToInvalidLessArgTest
     * @tc.desc: Test Js Api Query.equalTo() with invalid less arguments
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryEqualToInvalidArgTest', 0, async function (done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.equalTo();
            expect(null).assertFail();
        } catch (e) {
            console.error("throw exception : " + `, error code is ${e.code}, message is ${e.message}`);
            expect(e.code == 401).assertTrue();
        }
        query = null;
        done();
    })

    /**
     * @tc.name: QueryNotEqualToSucTest
     * @tc.desc: Test Js Api Query.notEualTo() successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryNotEqualToSucTest', 0, async function (done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.notEqualTo("key1", 5);
            query.notEqualTo("key2", 5.0);
            query.notEqualTo("key3", false);
            query.notEqualTo("key4", "string");
            expect(query.getSqlLike() !== "").assertTrue();
        } catch (e) {
            console.error("dumplicated calls should be ok : " + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        query = null;
        done();
    })

    /**
     * @tc.name: QueryNotEqualToNanTest
     * @tc.desc: Test Js Api Query.equalTo() with nan values
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryNotEqualToNanTest', 0, async function (done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.notEqualTo("key2", NaN);
            expect(query.getSqlLike() !== "").assertTrue();
        } catch (e) {
            expect(null).assertFail();
        }
        query = null;
        done();
    })

    /**
     * @tc.name: QueryNotEqualToInvalidMoreArgTest
     * @tc.desc: Test Js Api Query.equalTo() with invalid more arguments
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryNotEqualToInvalidMoreArgTest', 0, async function (done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.notEqualTo("key1", "value", "too more", 4);
            console.info("should not throw exception on invalid arguments");
            expect(true).assertTrue();
        } catch (e) {
            console.log("throw exception : " + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        query = null;
        done();
    })

    /**
     * @tc.name: QueryNotEqualToInvalidLessArgTest
     * @tc.desc: Test Js Api Query.equalTo() with invalid less arguments
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryNotEqualToInvalidLessArgTest', 0, async function (done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.notEqualTo();
            console.info("should throw exception on invalid arguments");
            expect(null).assertFail();
        } catch (e) {
            console.log("throw exception : " + `, error code is ${e.code}, message is ${e.message}`);
            expect(e.code == 401).assertTrue();
        }
        query = null;
        done();
    })

    /**
     * @tc.name: QueryGreaterThanSucTest
     * @tc.desc: Test Js Api Query.greaterThan() successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryGreaterThanSucTest', 0, async function (done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.greaterThan("key1", 5);
            query.greaterThan("key2", 5.0);
            query.greaterThan("key3", true);
            query.greaterThan("key4", "string");
            expect(query.getSqlLike() !== "").assertTrue();
            console.info("query is " + query.getSqlLike());
        } catch (e) {
            console.error("dumplicated calls should be ok : " + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        query = null;
        done();
    })

    /**
     * @tc.name: QueryGreatThanNanTest
     * @tc.desc: Test Js Api Query.GreatThan() with value nan
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryGreatThanNanTest', 0, async function (done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.greaterThan("key2", NaN);
            console.info("should not throw exception on invalid arguments");
            expect(true).assertTrue();
        } catch (e) {
            expect(null).assertFail();
        }
        query = null;
        done();
    })

    /**
     * @tc.name: QueryGreatThanInvalidMoreArgsTest
     * @tc.desc: Test Js Api Query.GreatThan() with invalid more arguments
     * @tc.type: FUNC
     * @tc.require: issueNumber
     **/
    it('QueryGreatThanInvalidMoreArgsTest', 0, async function (done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.greaterThan("key1", "value", "too more", 4);
            console.info("should not throw exception on invalid arguments");
            expect(query.getSqlLike() !== "").assertTrue();
        } catch (e) {
            console.error("throw exception : " + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        query = null;
        done();
    })

    /**
     * @tc.name: QueryGreatThanInvalidLessArgsTest
     * @tc.desc: Test Js Api Query.GreatThan() with invalid less arguments
     * @tc.type: FUNC
     * @tc.require: issueNumber
     **/
    it('QueryGreatThanInvalidLessArgsTest', 0, async function (done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.greaterThan();
            console.info("should throw exception on invalid arguments");
            expect(null).assertFail();
        } catch (e) {
            console.error("throw exception : " + `, error code is ${e.code}, message is ${e.message}`);
            expect(e.code == 401).assertTrue();
        }
        query = null;
        done();
    })

    /**
     * @tc.name: QueryLessThanSucTest
     * @tc.desc: Test Js Api Query.LessThan() successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryLessThanSucTest', 0, async function (done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.lessThan("key1", 5);
            query.lessThan("key2", 5.0);
            query.lessThan("key3", true);
            query.lessThan("key4", "string");
            expect(query.getSqlLike() !== "").assertTrue();
        } catch (e) {
            console.error("dumplicated calls should be ok : " + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        query = null;
        done();
    })

    /**
     * @tc.name: QueryLessThanNanTest
     * @tc.desc: Test Js Api Query.LessThan() with value nan
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryLessThanNanTest', 0, async function (done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.lessThan("key2", NaN);
            console.info("query is " + query.getSqlLike());
            expect(true).assertTrue();
        } catch (e) {
            expect(null).assertFail();
        }
        query = null;
        done();
    })

    /**
     * @tc.name: QueryLessThanInvalidMoreArgsTest
     * @tc.desc: Test Js Api Query.LessThan() with invalid more arguments
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryLessThanInvalidMoreArgsTest', 0, async function (done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.lessThan("key1", "value", "too more", 4);
            console.info("query is " + query.getSqlLike());
            expect(query.getSqlLike() !== "").assertTrue();
        } catch (e) {
            console.error("throw exception : " + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        query = null;
        done();
    })

    /**
     * @tc.name: QueryLessThanInvalidLessArgsTest
     * @tc.desc: Test Js Api Query.LessThan() with invalid less arguments
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryLessThanInvalidLessArgsTest', 0, async function (done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.lessThan();
            console.info("query is " + query.getSqlLike());
            expect(null).assertFail();
        } catch (e) {
            console.error("throw exception : " + `, error code is ${e.code}, message is ${e.message}`);
            expect(e.code == 401).assertTrue();
        }
        query = null;
        done();
    })

    /**
     * @tc.name: QueryGreaterThanOrEqualToSucTest
     * @tc.desc: Test Js Api Query.GreaterThanOrEqualTo() successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryGreaterThanOrEqualToSucTest', 0, async function (done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.greaterThanOrEqualTo("key1", 5);
            query.greaterThanOrEqualTo("key2", 5.0);
            query.greaterThanOrEqualTo("key3", true);
            query.greaterThanOrEqualTo("key4", "string");
            expect(query.getSqlLike() !== "").assertTrue();
            console.info("query is " + query.getSqlLike());
        } catch (e) {
            console.error("dumplicated calls should be ok : " + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        query = null;
        done();
    })

    /**
     * @tc.name: QueryGreaterThanOrEqualToNanTest
     * @tc.desc: Test Js Api Query.GreaterThanOrEqualTo() with value nan
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryGreaterThanOrEqualToNanTest', 0, async function (done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.greaterThanOrEqualTo("key2", NaN);
            expect(query.getSqlLike() !== "").assertTrue();
        } catch (e) {
            console.error(`failed, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        query = null;
        done();
    })

    /**
     * @tc.name: QueryGreaterThanOrEqualToInvalidMoreArgsTest
     * @tc.desc: Test Js Api Query.GreaterThanOrEqualTo() with invalid more arguments
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryGreaterThanOrEqualToInvalidMoreArgsTest', 0, async function (done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.greaterThanOrEqualTo("key1", "value", "too more", 4);
            console.info("should not throw exception on invalid arguments");
            expect(query.getSqlLike() !== "").assertTrue();
        } catch (e) {
            console.error("throw exception : " + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        query = null;
        done();
    })

    /**
     * @tc.name: QueryGreaterThanOrEqualToInvalidLessArgsTest
     * @tc.desc: Test Js Api Query.GreaterThanOrEqualTo() with invalid less arguments
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryGreaterThanOrEqualToInvalidLessArgsTest', 0, async function (done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.greaterThanOrEqualTo();
            console.info("should not throw exception on invalid arguments");
            expect(null).assertFail();
        } catch (e) {
            console.error("throw exception : " + `, error code is ${e.code}, message is ${e.message}`);
            expect(e.code == 401).assertTrue();
        }
        query = null;
        done();
    })

    /**
     * @tc.name: QueryLessThanOrEqualToSucTest
     * @tc.desc: Test Js Api Query.LessThanOrEqualTo() successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryLessThanOrEqualToSucTest', 0, async function (done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.lessThanOrEqualTo("key1", 5);
            query.lessThanOrEqualTo("key2", 5.0);
            query.lessThanOrEqualTo("key3", true);
            query.lessThanOrEqualTo("key4", "string");
            expect(query.getSqlLike() !== "").assertTrue();
            console.info("query is " + query.getSqlLike());
        } catch (e) {
            console.error("dumplicated calls should be ok : " + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        query = null;
        done();
    })

    /**
     * @tc.name: QueryLessThanOrEqualToNanTest
     * @tc.desc: Test Js Api Query.LessThanOrEqualTo() with value nan
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryLessThanOrEqualToNanTest', 0, async function (done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.lessThanOrEqualTo("key2", NaN);
            console.info("query is " + query.getSqlLike());
            expect(true).assertTrue();
        } catch (e) {
            expect(null).assertFail();
        }
        query = null;
        done();
    })

    /**
     * @tc.name: QueryLessThanOrEqualToInvalidMoreArgsTest
     * @tc.desc: Test Js Api Query.LessThanOrEqualTo() with invalid more args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('nameQueryLessThanOrEqualToInvalidMoreArgsTest', 0, async function (done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.lessThanOrEqualTo("key1", "value", "too more", 4);
            console.info("should not throw exception on invalid arguments");
            expect(query.getSqlLike() !== "").assertTrue();
        } catch (e) {
            console.error("throw exception: " + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        query = null;
        done();
    })

    /**
     * @tc.name: QueryLessThanOrEqualToInvalidLessArgsTest
     * @tc.desc: Test Js Api Query.LessThanOrEqualTo() with invalid less args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryLessThanOrEqualToInvalidLessArgsTest', 0, async function (done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.lessThanOrEqualTo();
            console.info("should not throw exception on invalid arguments");
            expect(null).assertFail();
        } catch (e) {
            console.error("throw exception: " + `, error code is ${e.code}, message is ${e.message}`);
            expect(e.code == 401).assertTrue();
        }
        query = null;
        done();
    })

    /**
     * @tc.name: QueryIsNullSucTest
     * @tc.desc: Test Js Api Query.IsNull() successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryIsNullSucTest', 0, async function (done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.isNull("key");
            query.isNull("key2");
            expect(query.getSqlLike() !== "").assertTrue();
            console.info("query is " + query.getSqlLike());
        } catch (e) {
            console.error("dumplicated calls should be ok : " + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        query = null;
        done();
    })

    /**
     * @tc.name: QueryIsNullInvalidMoreArgsTest
     * @tc.desc: Test Js Api Query.IsNull() with invalid more args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryIsNullInvalidMoreArgsTest', 0, async function (done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.isNull("key", 0);
            console.info("should not throw exception on invalid arguments");
            expect(query.getSqlLike() !== "").assertTrue();
        } catch (e) {
            console.error("throw exception : " + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        query = null;
        done();
    })

    /**
     * @tc.name: QueryIsNullInvalidTypeArgsTest
     * @tc.desc: Test Js Api Query.IsNull() with invalid type args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryIsNullInvalidTypeArgsTest', 0, async function (done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.isNull(0);
            console.info("should throw exception on invalid arguments");
            expect(null).assertFail();
        } catch (e) {
            console.error("throw exception is ok : " + `, error code is ${e.code}, message is ${e.message}`);
            expect(e.code == 401).assertTrue();
        }
        query = null;
        done();
    })

    /**
     * @tc.name: QueryIsNullInvalidLessArgsTest
     * @tc.desc: Test Js Api Query.IsNull() with invalid less args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryIsNullInvalidLessArgsTest', 0, async function (done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.isNull();
            console.info("should throw exception on invalid arguments");
            expect(null).assertFail();
        } catch (e) {
            console.error("throw exception is ok : " + `, error code is ${e.code}, message is ${e.message}`);
            expect(e.code == 401).assertTrue();
        }
        query = null;
        done();
    })

    /**
     * @tc.name: QueryInNumberSucTest
     * @tc.desc: Test Js Api Query.InNumber() successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryInNumberSucTest', 0, async function (done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            var i8 = new Int8Array([-21, 31]);
            query.reset().inNumber("key", i8);
            console.info("inNumber(Int8Array([-21,31])  => " + query.getSqlLike());
            var u8 = new Uint8Array([-21, 31]);
            query.reset().inNumber("key", u8);
            console.info("inNumber(Uint8Array([-21,31])  => " + query.getSqlLike());
            var c8 = new Uint8ClampedArray([-21, 31]);
            query.reset().inNumber("key", c8);
            console.info("inNumber(Uint8Array([-21,31])  => " + query.getSqlLike());
            var i16 = new Int16Array([-21, 31]);
            query.reset().inNumber("key", i16);
            console.info("inNumber(Int16Array([-21,31])  => " + query.getSqlLike());
            var u16 = new Uint16Array([-21, 31]);
            query.reset().inNumber("key", u16);
            console.info("inNumber(Uint16Array([-21,31])  => " + query.getSqlLike());
            var i32 = new Int32Array([-21, 31]);
            query.reset().inNumber("key", i32);
            console.info("inNumber(Int32Array([-21,31])  => " + query.getSqlLike());
            var u32 = new Uint32Array([-21, 31]);
            query.reset().inNumber("key", u32);
            console.info("inNumber(UInt32Array([-21,31])  => " + query.getSqlLike());
            var f32 = new Float32Array([-21, 31]);
            query.reset().inNumber("key", f32);
            console.info("inNumber(Float32Array([-21,31])  => " + query.getSqlLike());
            var f32e = new Float32Array([21, 31, "a"]);
            query.reset().inNumber("key", f32e);
            console.info("inNumber(Float32Array([-21,31, 'a'])  => " + query.getSqlLike());
            var f64 = new Float64Array([-21, 31]);
            query.reset().inNumber("key", f64);
            console.info("inNumber(Float64Array([-21,31])  => " + query.getSqlLike());
            query.reset();
            var u64 = new BigUint64Array([21n, 31n]);
            query.reset().inNumber("key", u64);
            var b64 = new BigInt64Array([21n, 31n]);
            query.reset().inNumber("key", b64);
            expect(true).assertTrue();
        } catch (e) {
            console.error("dumplicated calls should be ok : " + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        query = null;
        done();
    })

    /**
     * @tc.name: QueryInNumberInvalidMoreArgsTest
     * @tc.desc: Test Js Api Query.InNumber() with invalid more args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryInNumberInvalidMoreArgsTest', 0, async function (done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.inNumber("key", 0);
            console.info("should throw exception on invalid arguments");
            expect(null).assertFail();
        } catch (e) {
            console.error("throw exception : " + `, error code is ${e.code}, message is ${e.message}`);
            expect(e.code == 401).assertTrue();
        }
        query = null;
        done();
    })

    /**
     * @tc.name: QueryInNumberInvalidLessArgsTest
     * @tc.desc: Test Js Api Query.InNumber() with invalid less args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryInNumberInvalidTypeArgsTest', 0, async function (done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.inNumber([0, 1]);
            console.info("should throw exception on invalid arguments");
            expect(null).assertFail();
        } catch (e) {
            console.error("throw exception is ok : " + `, error code is ${e.code}, message is ${e.message}`);
            expect(e.code == 401).assertTrue();
        }
        query = null;
        done();
    })

    /**
     * @tc.name: QueryInNumberBoundaryCallsTest
     * @tc.desc: Test Js Api Query.InNumber() boundary calls
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryInNumberBoundaryCallsTest', 0, async function (done) {
        var query = null;
        try {
            query = new ddm.Query();
            console.info("typeof([1, 2, 97])" + typeof ([1, 2, 97]))
            console.info("typeof([1, 2, 97][0])" + typeof ([1, 2, 97][0]))
            query.inNumber("key", [1, 2, 97]);
            console.info("inNumber([1, 2, 97])  => " + query.getSqlLike());
            query.reset();
            query.inNumber("key1", [-1, 3, 987654.123, 0xabc123456]);
            console.info("inNumber([1, 2, 0xa1234567890123456])  => " + query.getSqlLike());
            query.reset();
            query.inNumber("key2", [-1, 3, -987654.123, 0xabc123456]);
            console.info("inNumber([1, 2, 0xa1234567890123456])  => " + query.getSqlLike());
            query.reset();
            query.inNumber("key3", [-1, 4, -987654.123, Number.MAX_VALUE]);
            console.info("inNumber([1, 2, Number.MAX_VALUE])  => " + query.getSqlLike());
            query.reset();
            query.inNumber("key4", [1, -2.3, Number.MIN_VALUE, Number.MAX_VALUE]);
            console.info("inNumber([1, -2.3, Number.MAX_VALUE])  => " + query.getSqlLike());
            expect(query.getSqlLike() !== "").assertTrue();
            console.info("query is " + query.getSqlLike());
            query.reset();
        } catch (e) {
            console.error("dumplicated calls should be ok : " + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        query = null;
        done();
    })

    /**
     * @tc.name: QueryInStringSucTest
     * @tc.desc: Test Js Api Query.InString() successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryInStringSucTest', 0, async function (done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.inString("key", ["a2z", 'z2a']);
            query.inString("key2", ["AAA"]);
            console.info("query is " + query.getSqlLike());
            expect(query.getSqlLike() !== "").assertTrue();
        } catch (e) {
            console.error("dumplicated calls should be ok : " + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        query = null;
        done();
    })

    /**
     * @tc.name: QueryInStringInvalidMoreArgsTest
     * @tc.desc: Test Js Api Query.InString() with invalid more args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryInStringInvalidMoreArgsTest', 0, async function (done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.inString("key", ["a2z", 'z2a'], ["AAA"]);
            console.info("should throw exception on invalid arguments");
            expect(true).assertTrue();
        } catch (e) {
            console.error("throw exception : " + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        query = null;
        done();
    })

    /**
     * @tc.name: QueryInStringInvalidTypeArgsTest
     * @tc.desc: Test Js Api Query.InString() with invalid type args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryInStringInvalidTypeArgsTest', 0, async function (done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.inString("key", 0);
            console.info("should throw exception on invalid arguments");
            expect(null).assertFail();
        } catch (e) {
            console.error("throw exception : " + `, error code is ${e.code}, message is ${e.message}`);
            expect(e.code == 401).assertTrue();
        }
        query = null;
        done();
    })

    /**
     * @tc.name: QueryInStringInvalidListArgsTest
     * @tc.desc: Test Js Api Query.InString() with invalid list args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryInStringInvalidListArgsTest', 0, async function (done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.inString("key", [0, 1]);
            console.info("should throw exception on invalid arguments");
            expect(null).assertFail();
        } catch (e) {
            console.error("throw exception is ok : " + `, error code is ${e.code}, message is ${e.message}`);
            expect(e.code == 401).assertTrue();
        }
        query = null;
        done();
    })

    /**
     * @tc.name: QueryNotInNumberSucTest
     * @tc.desc: Test Js Api Query.NotInNumber() successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryNotInNumberSucTest', 0, async function (done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.notInNumber("key", [1, 2]);
            query.notInNumber("key", [1000]);
            expect(query.getSqlLike() !== "").assertTrue();
            console.info("query is " + query.getSqlLike());
        } catch (e) {
            console.error("dumplicated calls should be ok : " + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        query = null;
        done();
    })

    /**
     * @tc.name: QueryNotInNumberInvalidMoreArgsTest
     * @tc.desc: Test Js Api Query.NotInNumber() with invalid more arguments
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryNotInNumberInvalidMoreArgsTest', 0, async function (done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.notInNumber("key", [1], 2);
            console.info("should not throw exception on invalid arguments");
            expect(query.getSqlLike() !== "").assertTrue();
        } catch (e) {
            console.error("throw exception : " + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        query = null;
        done();
    })

    /**
     * @tc.name: QueryNotInNumberInvalidTypeArgsTest
     * @tc.desc: Test Js Api Query.NotInNumber() with invalid type arguments
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryNotInNumberInvalidTypeArgsTest', 0, async function (done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.notInNumber("key", ["string"]);
            expect(query.getSqlLike() !== "").assertTrue();
            console.info("should throw exception on invalid arguments");
            expect(null).assertFail();
        } catch (e) {
            console.error("throw exception is ok : " + `, error code is ${e.code}, message is ${e.message}`);
            expect(e.code == 401).assertTrue();
        }
        query = null;
        done();
    })

    /**
     * @tc.name: QueryNotInStringSucTest
     * @tc.desc: Test Js Api Query.NotInString() successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryNotInStringSucTest', 0, async function (done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.notInString("key", ["v1", "v2"]);
            query.notInString("key", ["v1", "NaN"]);
            expect(query.getSqlLike() !== "").assertTrue();
            console.info("query is " + query.getSqlLike());
        } catch (e) {
            console.error("dumplicated calls should be ok : " + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        query = null;
        done();
    })

    /**
     * @tc.name: QueryNotInStringInvalidMoreArgsTest
     * @tc.desc: Test Js Api Query.NotInString() with invalid more args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryNotInStringInvalidMoreArgsTest', 0, async function (done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.notInString("key", ["", "abccd"], 2);
            console.info("should not throw exception on invalid arguments");
            expect(query.getSqlLike() !== "").assertTrue();
        } catch (e) {
            console.error("throw exception : " + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        query = null;
        done();
    })

    /**
     * @tc.name: QueryNotInStringInvalidTypeArgsTest
     * @tc.desc: Test Js Api Query.NotInString() with invalid type args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryNotInStringInvalidTypeArgsTest', 0, async function (done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.notInString("key", [1, 2]);
            console.info("should throw exception on invalid arguments");
            expect(null).assertFail();
        } catch (e) {
            console.error("throw exception is ok : " + `, error code is ${e.code}, message is ${e.message}`);
            expect(e.code == 401).assertTrue();
        }
        query = null;
        done();
    })

    /**
     * @tc.name: QueryLikeSucTest
     * @tc.desc: Test Js Api Query.Like() successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryLikeSucTest', 0, async function (done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.like("key", "v1");
            query.like("key2", "v2");
            expect(query.getSqlLike() !== "").assertTrue();
            console.info("query is " + query.getSqlLike());
        } catch (e) {
            console.error("dumplicated calls should be ok : " + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        query = null;
        done();
    })

    /**
     * @tc.name: QueryLikeInvalidArgsTypeTest
     * @tc.desc: Test Js Api Query.Like() with invalid type args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryLikeInvalidArgsTypeTest', 0, async function (done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.like("key", 0);
            console.info("should throw exception on invalid arguments");
            expect(null).assertFail();
        } catch (e) {
            console.error("throw exception is ok : " + `, error code is ${e.code}, message is ${e.message}`);
            expect(e.code == 401).assertTrue();
        }
        query = null;
        done();
    })

    /**
     * @tc.name: QueryLikeInvalidMoreTypesTest
     * @tc.desc: Test Js Api Query.Like() with invalid more args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryLikeInvalidMoreTypesTest', 0, async function (done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.like("key", "str1", "str2");
            console.info("should not throw exception on invalid arguments");
            expect(query.getSqlLike() !== "").assertTrue();
        } catch (e) {
            console.error("throw exception : " + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        query = null;
        done();
    })

    /**
     * @tc.name: QueryUnlikeSucTest
     * @tc.desc: Test Js Api Query.Unlike() successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryUnlikeSucTest', 0, async function (done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.unlike("key", "v1");
            expect(query.getSqlLike() !== "").assertTrue();
            console.info("query is " + query.getSqlLike());
        } catch (e) {
            console.error("dumplicated calls should be ok : " + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        query = null;
        done();
    })

    /**
     * @tc.name: QueryUnlikeInvalidTypeArgsTest
     * @tc.desc: Test Js Api Query.Unlike() with invalid type args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryUnlikeInvalidTypeArgsTest', 0, async function (done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.unlike("key", 0);
            console.info("should throw exception on invalid arguments");
            expect(null).assertFail();
        } catch (e) {
            console.error("throw exception is ok : " + `, error code is ${e.code}, message is ${e.message}`);
            expect(e.code == 401).assertTrue();
        }
        query = null;
        done();
    })

    /**
     * @tc.name: QueryUnlikeInvalidMoreArgsTest
     * @tc.desc: Test Js Api Query.Unlike() with invalid more args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryUnlikeInvalidMoreArgsTest', 0, async function (done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.unlike("key", "str1", "str2");
            console.info("should not throw exception on invalid arguments");
            expect(query.getSqlLike() !== "").assertTrue();
        } catch (e) {
            console.error("throw exception : " + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        query = null;
        done();
    })

    /**
     * @tc.name: QueryAndSucTest
     * @tc.desc: Test Js Api Query.And() successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryAndSucTest', 0, async function (done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.notEqualTo("key", 0);
            query.and();
            query.notEqualTo("key", "v1");
            expect(query.getSqlLike() !== "").assertTrue();
            console.info("query is " + query.getSqlLike());
        } catch (e) {
            console.error("dumplicated calls should be ok : " + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        query = null;
        done();
    })

    /**
     * @tc.name: QueryAndInvalidArgsTest
     * @tc.desc: Test Js Api Query.And() invalid args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryAndInvalidArgsTest', 0, async function (done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.notEqualTo("key", 0).and(1).notInNumber("key", [1, 3]);
            console.info("should throw exception on invalid arguments");
            expect(null).assertFail();
        } catch (e) {
            console.error("throw exception is ok : " + `, error code is ${e.code}, message is ${e.message}`);
            expect(true).assertTrue();
        }
        query = null;
        done();
    })

    /**
     * @tc.name: QueryOrSucTest
     * @tc.desc: Test Js Api Query.Or() successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryOrSucTest', 0, async function (done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.notEqualTo("key", 0);
            query.or();
            query.notEqualTo("key", "v1");
            expect(query.getSqlLike() !== "").assertTrue();
            console.info("query is " + query.getSqlLike());
        } catch (e) {
            console.error("dumplicated calls should be ok : " + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        query = null;
        done();
    })

    /**
     * @tc.name: QueryOrInvalidArgsTest
     * @tc.desc: Test Js Api Query.Or() with invalid args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryOrInvalidArgsTest', 0, async function (done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.notEqualTo("key", 0).or(1).notInNumber("key", [1, 3]);
            console.info("should throw exception on invalid arguments");
            expect(null).assertFail();
        } catch (e) {
            console.error("throw exception is ok : " + `, error code is ${e.code}, message is ${e.message}`);
            expect(true).assertTrue();
        }
        query = null;
        done();
    })

    /**
     * @tc.name: QueryOrderByAscSucTest
     * @tc.desc: Test Js Api Query.OrderByAsc() successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryOrderByAscSucTest', 0, async function (done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.notEqualTo("key", 0);
            query.orderByAsc("sortbykey");
            expect(query.getSqlLike() !== "").assertTrue();
            console.info("query is " + query.getSqlLike());
        } catch (e) {
            console.error("dumplicated calls should be ok : " + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        query = null;
        done();
    })

    /**
     * @tc.name: QueryOrderByAscInvalidTypeArgsTest
     * @tc.desc: Test Js Api Query.OrderByAsc() with invalid type args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryOrderByAscInvalidTypeArgsTest', 0, async function (done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.notEqualTo("key", false).orderByAsc(1);
            console.info("should throw exception on invalid arguments");
            expect(null).assertFail();
        } catch (e) {
            console.error("throw exception is ok : " + `, error code is ${e.code}, message is ${e.message}`);
            expect(e.code == 401).assertTrue();
        }
        query = null;
        done();
    })

    /**
     * @tc.name: QueryOrderByAscWithNullTest
     * @tc.desc: Test Js Api Query.OrderByAsc() null args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryOrderByAscWithNullTest', 0, async function (done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.orderByAsc();
            console.info("should throw exception on invalid arguments");
            expect(null).assertFail();
        } catch (e) {
            console.error("throw exception is ok : ");
            expect(e.code == 401).assertTrue();
        }
        query = null;
        done();
    })

    /**
     * @tc.name: QueryOrderByDescTest
     * @tc.desc: Test Js Api Query.OrderByDesc() successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryOrderByDescTest', 0, async function (done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.notEqualTo("key", 0);
            query.orderByDesc("sortbykey");
            expect(query.getSqlLike() !== "").assertTrue();
            console.info("query is " + query.getSqlLike());
        } catch (e) {
            console.error("dumplicated calls should be ok : " + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        query = null;
        done();
    })

    /**
     * @tc.name: QueryOrderByDescInvalidTypeArgsTest
     * @tc.desc: Test Js Api Query.OrderByDesc() with invalid type args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryOrderByDescInvalidTypeArgsTest', 0, async function (done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.notEqualTo("key", false).orderByDesc(1);
            console.info("should throw exception on invalid arguments");
            expect(null).assertFail();
        } catch (e) {
            console.error("throw exception is ok : " + `, error code is ${e.code}, message is ${e.message}`);
            expect(e.code == 401).assertTrue();
        }
        query = null;
        done();
    })

    /**
     * @tc.name: QueryOrderByDescNullTest
     * @tc.desc: Test Js Api Query.OrderByDesc() with null args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryOrderByDescNullTest', 0, async function (done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.orderByDesc();
            console.info("should throw exception on invalid arguments");
            expect(null).assertFail();
        } catch (e) {
            console.error("throw exception is ok : " + `, error code is ${e.code}, message is ${e.message}`);
            expect(e.code == 401).assertTrue();
        }
        query = null;
        done();
    })

    /**
     * @tc.name: QueryLimitSucTest
     * @tc.desc: Test Js Api Query.Limit() successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryLimitSucTest', 0, async function (done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.notEqualTo("key", "vx");
            expect(query.getSqlLike() !== "").assertTrue();
            query.limit(10, 2);
            expect(query.getSqlLike() !== "").assertTrue();
            console.info("query is " + query.getSqlLike());
        } catch (e) {
            console.error("dumplicated calls should be ok : " + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        query = null;
        done();
    })

    /**
     * @tc.name: QueryLimitMoreArgsTest
     * @tc.desc: Test Js Api Query.Limit() with invalid more args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryLimitMoreArgsTest', 0, async function (done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.notEqualTo("key", false).limit(10, 2, "any");
            console.info("should not throw exception on invalid arguments");
            expect(query.getSqlLike() !== "").assertTrue();
        } catch (e) {
            console.error("throw exception: " + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        query = null;
        done();
    })

    /**
     * @tc.name: QueryLimitLessArgsTest
     * @tc.desc: Test Js Api Query.Limit() with invalid less args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryLimitLessArgsTest', 0, async function (done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.notEqualTo("key", false).limit(10);
            console.info("should throw exception on invalid arguments");
            expect(null).assertFail();
        } catch (e) {
            console.error("throw exception is ok : " + `, error code is ${e.code}, message is ${e.message}`);
            expect(e.code == 401).assertTrue();
        }
        query = null;
        done();
    })

    /**
     * @tc.name: QueryLimitInvalidTypeArgsTest
     * @tc.desc: Test Js Api Query.Limit() with invalid type args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryLimitInvalidTypeArgsTest', 0, async function (done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.notEqualTo("key", false).limit("any", 10);
            console.info("should throw exception on invalid arguments");
            console.info("query is " + query.getSqlLike());
            expect(null).assertFail();
        } catch (e) {
            console.error("throw exception is ok : " + `, error code is ${e.code}, message is ${e.message}`);
            expect(e.code == 401).assertTrue();
        }
        query = null;
        done();
    })

    /**
     * @tc.name: QueryIsNotNullSucTest
     * @tc.desc: Test Js Api Query.IsNotNull() successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryIsNotNullSucTest', 0, async function (done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.isNotNull("key");
            expect(query.getSqlLike() !== "").assertTrue();
            console.info("query is " + query.getSqlLike());
        } catch (e) {
            console.error("dumplicated calls should be ok : " + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        query = null;
        done();
    })

    /**
     * @tc.name: QueryIsNotNullMoreArgsTest
     * @tc.desc: Test Js Api Query.IsNotNull() with invalid more args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryIsNotNullMoreArgsTest', 0, async function (done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.isNotNull("key2", "any");
            console.info("should throw exception on invalid arguments");
            expect(query.getSqlLike() !== "").assertTrue();
        } catch (e) {
            console.error("throw exception: " + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        query = null;
        done();
    })

    /**
     * @tc.name: QueryIsNotNullInvalidTypeArgsTest
     * @tc.desc: Test Js Api Query.IsNotNull() with invalid type args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryIsNotNullInvalidTypeArgsTest', 0, async function (done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.isNotNull(1);
            console.info("should throw exception on invalid arguments");
            expect(null).assertFail();
        } catch (e) {
            console.error("throw exception is ok : " + `, error code is ${e.code}, message is ${e.message}`);
            expect(e.code == 401).assertTrue();
        }
        query = null;
        done();
    })

    /**
     * @tc.name: QueryBeginGroupSucTest
     * @tc.desc: Test Js Api Query.BeginGroup() successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryBeginGroupSucTest', 0, async function (done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.beginGroup();
            query.isNotNull("$.name");
            query.endGroup();
            expect(query.getSqlLike() !== "").assertTrue();
            console.info("query is " + query.getSqlLike());
        } catch (e) {
            console.error("dumplicated calls should be ok : " + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        query = null;
        done();
    })

    /**
     * @tc.name: QueryBeginGroupInvalidTypeArgsTest
     * @tc.desc: Test Js Api Query.BeginGroup() with invalid type args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryBeginGroupInvalidTypeArgsTest', 0, async function (done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.beginGroup(1);
            console.info("should throw exception on invalid arguments");
            expect(null).assertFail();
        } catch (e) {
            console.error("throw exception is ok : " + `, error code is ${e.code}, message is ${e.message}`);
            expect(true).assertTrue();
        }
        query = null;
        done();
    })

    /**
     * @tc.name: QueryBeginGroupInvalidMoreArgsTest
     * @tc.desc: Test Js Api Query.BeginGroup() with invalid more args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryBeginGroupInvalidMoreArgsTest', 0, async function (done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.beginGroup("any", 1);
            console.info("should throw exception on invalid arguments");
            expect(null).assertFail();
        } catch (e) {
            console.error("throw exception: " + `, error code is ${e.code}, message is ${e.message}`);
            expect(true).assertTrue();
        }
        query = null;
        done();
    })


    /**
     * @tc.name: QueryEndGroupSucTest
     * @tc.desc: Test Js Api Query.EndGroup() successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryEndGroupSucTest', 0, async function (done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.beginGroup();
            query.isNotNull("name");
            query.endGroup();
            expect(query.getSqlLike() !== "").assertTrue();
        } catch (e) {
            console.error("dumplicated calls should be ok : " + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        query = null;
        done();
    })

    /**
     * @tc.name: QueryEndGroupInvalidTypeArgsTest
     * @tc.desc: Test Js Api Query.EndGroup() with invalid type args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryEndGroupInvalidTypeArgsTest', 0, async function (done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.endGroup(0);
            console.info("should throw exception on invalid arguments");
            expect(null).assertFail();
        } catch (e) {
            console.error("throw exception is ok : " + `, error code is ${e.code}, message is ${e.message}`);
            expect(true).assertTrue();
        }
        query = null;
        done();
    })

    /**
     * @tc.name: QueryPrefixKeySucTest
     * @tc.desc: Test Js Api Query.PrefixKey() successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryPrefixKeySucTest', 0, async function (done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.prefixKey("$.name");
            query.prefixKey("0");
            expect(query.getSqlLike() !== "").assertTrue();
            console.info("query is " + query.getSqlLike());
        } catch (e) {
            expect(null).assertFail();
        }
        query = null;
        done();
    })

    /**
     * @tc.name: QueryPrefixKeyInvalidMoreArgsTest
     * @tc.desc: Test Js Api Query.PrefixKey() with invalid more args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryPrefixKeyInvalidMoreArgsTest', 0, async function (done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.prefixKey("k", "any");
            console.info("should not throw exception on invalid arguments");
            expect(query.getSqlLike() !== "").assertTrue();
        } catch (e) {
            console.error("throw exception : " + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        query = null;
        done();
    })

    /**
     * @tc.name: QueryPrefixKeyInvalidTypeArgsTest
     * @tc.desc: Test Js Api Query.PrefixKey() with invalid type args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryPrefixKeyInvalidTypeArgsTest', 0, async function (done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.prefixKey(123);
            console.info("should throw exception on invalid arguments");
            expect(null).assertFail();
        } catch (e) {
            console.error("throw exception is ok : " + `, error code is ${e.code}, message is ${e.message}`);
            expect(e.code == 401).assertTrue();
        }
        query = null;
        done();
    })

    /**
     * @tc.name: QuerySetSuggestIndexSucTest
     * @tc.desc: Test Js Api Query.SetSuggestIndex() successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QuerySetSuggestIndexSucTest', 0, async function (done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.setSuggestIndex("$.name");
            query.setSuggestIndex("0");
            expect(query.getSqlLike() !== "").assertTrue();
            console.info("query is " + query.getSqlLike());
        } catch (e) {
            expect(null).assertFail();
        }
        query = null;
        done();
    })

    /**
     * @tc.name: QuerySetSuggestIndexInvalidMoreArgsTest
     * @tc.desc: Test Js Api Query.SetSuggestIndex() with invalid more args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QuerySetSuggestIndexInvalidMoreArgsTest', 0, async function (done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.setSuggestIndex("k", "any");
            console.info("should not throw exception on invalid arguments");
            expect(query.getSqlLike() !== "").assertTrue();
        } catch (e) {
            console.error("throw exception : " + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        query = null;
        done();
    })

    /**
     * @tc.name: QuerySetSuggestIndexInvalidTypeArgsTest
     * @tc.desc: Test Js Api Query.SetSuggestIndex() with invalid more types
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QuerySetSuggestIndexInvalidTypeArgsTest', 0, async function (done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.setSuggestIndex(123);
            console.info("should throw exception on invalid arguments");
            expect(null).assertFail();
        } catch (e) {
            console.error("throw exception : " + `, error code is ${e.code}, message is ${e.message}`);
            expect(e.code == 401).assertTrue();
        }
        query = null;
        done();
    })

    /**
     * @tc.name: QueryDeviceIdSucTest
     * @tc.desc: Test Js Api Query.DeviceId() successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryDeviceIdSucTest', 0, async function (done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.deviceId("$.name");
            query.deviceId("0");
            expect(query.getSqlLike() !== "").assertTrue();
            console.info("query is " + query.getSqlLike());
        } catch (e) {
            console.error("dumplicated calls should be ok : " + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        query = null;
        done();
    })

    /**
     * @tc.name: QueryDeviceIdInvalidMoreArgsTest
     * @tc.desc: Test Js Api Query.DeviceId() with invalid more args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryDeviceIdInvalidMoreArgsTest', 0, async function (done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.deviceId("k", "any");
            console.info("should not throw exception on invalid arguments");
            expect(query.getSqlLike() !== "").assertTrue();
        } catch (e) {
            console.error("throw exception : " + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        query = null;
        done();
    })

    /**
     * @tc.name: QueryDeviceIdInvalidTypeArgsTest
     * @tc.desc: Test Js Api Query.DeviceId() with invalid type args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryDeviceIdInvalidTypeArgsTest', 0, async function (done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.deviceId(123);
            console.info("should throw exception on invalid arguments");
            expect(null).assertFail();
        } catch (e) {
            console.error("throw exception is ok : " + `, error code is ${e.code}, message is ${e.message}`);
            expect(e.code == 401).assertTrue();
        }
        query = null;
        done();
    })

    /**
     * @tc.name: QueryGetSqlLikeSucTest
     * @tc.desc: Test Js Api Query.GetSqlLike() successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryGetSqlLikeSucTest', 0, async function (done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            let sql1 = query.getSqlLike();
            console.info("QueryGetSqlLikeSucTest sql=" + sql1);
            let sql2 = query.getSqlLike();
            expect(sql1).assertEqual(sql2);
            console.info("query is " + query.getSqlLike());
        } catch (e) {
            console.error("dumplicated calls should be ok : " + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        query = null;
        done();
    })

    /**
     * @tc.name: QueryGetSqlLikeInvalidMoreArgsTest
     * @tc.desc: Test Js Api Query.GetSqlLike() with invalid more args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryGetSqlLikeInvalidMoreArgsTest', 0, async function (done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.inNumber("key");
            query.getSqlLike(0);
            console.info("should throw exception on invalid arguments");
            expect(null).assertFail();
        } catch (e) {
            console.error("throw exception : " + `, error code is ${e.code}, message is ${e.message}`);
            expect(e.code == 401).assertTrue();
        }
        query = null;
        done();
    })
})
