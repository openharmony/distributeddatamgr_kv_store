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
import ddm from '@ohos.data.distributedData';

describe('queryTest', function() {

    /**
     * @tc.name QueryResetTest001
     * @tc.desc Test Js Api Query.Reset() testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryResetTest001', 0, async function(done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.equalTo("test", 3);
            console.info("query is " + query.getSqlLike());
            expect(query.getSqlLike() !== "").assertTrue();
            query.reset();
            expect("").assertEqual(query.getSqlLike());
            console.info("query is " + query.getSqlLike());
        } catch (e) {
            console.error("simply calls should be ok : " + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        query = null;
        done();
    })

    /**
     * @tc.name QueryResetTest002
     * @tc.desc Test Js Api Query.Reset() testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryResetTest002', 0, async function(done) {
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
            query.reset();
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
     * @tc.name QueryResetTest003
     * @tc.desc Test Js Api Query.Reset() testcase 003
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryResetTest003', 0, async function(done) {
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
     * @tc.name QueryResetTest004
     * @tc.desc Test Js Api Query.Reset() testcase 004
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryResetTest004', 0, async function(done) {
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
            console.error("throw exception is ok : " + `, error code is ${e.code}, message is ${e.message}`);
        }
        query = null;
        done();
    })

    /**
     * @tc.name QueryEqaulToTest001
     * @tc.desc Test Js Api Query.EqaulTo() testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryEqaulToTest001', 0, async function(done) {
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
     * @tc.name QueryEqualToTest002
     * @tc.desc Test Js Api Query.EqualTo() testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryEqualToTest002', 0, async function(done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.equalTo("key1", 1).equalTo("key2", 2).equalTo("key3", 3);
            expect(query.getSqlLike() !== "").assertTrue();
            console.info("query is " + query.getSqlLike());
        } catch (e) {
            console.error("should be ok on Method Chaining : " + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        query = null;
        done();
    })

    /**
     * @tc.name QueryEqualToTest003
     * @tc.desc Test Js Api Query.EqualTo() testcase 003
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryEqualToTest003', 0, async function(done) {
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
     * @tc.name QueryEqualToTest004
     * @tc.desc Test Js Api Query.EqualTo() testcase 004
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryEqualToTest004', 0, async function(done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.equalTo("key1", "value", "too more");
            console.info("should throw exception on invalid arguments");
            console.info("query is " + query.getSqlLike());
            expect(null).assertFail();
        } catch (e) {
            console.error("throw exception is ok : " + `, error code is ${e.code}, message is ${e.message}`);
        }
        query = null;
        done();
    })

    /**
     * @tc.name QueryNotEqualToTest001
     * @tc.desc Test Js Api Query.NotEqualTo() testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryNotEqualToTest001', 0, async function(done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.notEqualTo("key1", 5);
            query.notEqualTo("key2", 5.0);
            query.notEqualTo("key3", false);
            query.notEqualTo("key4", "string");
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
     * @tc.name QueryNotEqualToTest002
     * @tc.desc Test Js Api Query.NotEqualTo() testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryNotEqualToTest002', 0, async function(done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.notEqualTo("key", 5);
            query.reset();
            query.notEqualTo("key0", 5).equalTo("key1", 5).notEqualTo("key2", "str").notEqualTo("key3", false);
            expect(query.getSqlLike() !== "").assertTrue();
            console.info("query is " + query.getSqlLike());
        } catch (e) {
            console.error("should be ok on Method Chaining : " + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        query = null;
        done();
    })

    /**
     * @tc.name QueryNotEqualToTest003
     * @tc.desc Test Js Api Query.NotEqualTo() testcase 003
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryNotEqualToTest003', 0, async function(done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.notEqualTo("key2", NaN);
            expect(query.getSqlLike() !== "").assertTrue();
            console.info("query is " + query.getSqlLike());
        } catch (e) {
            expect(null).assertFail();
        }
        query = null;
        done();
    })

    /**
     * @tc.name QueryNotEqualToTest004
     * @tc.desc Test Js Api Query.NotEqualTo() testcase 004
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryNotEqualToTest004', 0, async function(done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.notEqualTo("key1", "value", "too more", 4);
            console.info("should throw exception on invalid arguments");
        } catch (e) {
            console.error("throw exception is ok : " + `, error code is ${e.code}, message is ${e.message}`);
        }
        query = null;
        done();
    })

    /**
     * @tc.name QueryGreaterThanTest001
     * @tc.desc Test Js Api Query.GreaterThan() testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryGreaterThanTest001', 0, async function(done) {
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
     * @tc.name QueryGreatThanTest002
     * @tc.desc Test Js Api Query.GreatThan() testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryGreatThanTest002', 0, async function(done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.greaterThan("key", 5);
            query.reset();
            query.greaterThan("key0", 5).greaterThan("key1", "v5").greaterThan("key3", false);
            expect(query.getSqlLike() !== "").assertTrue();
            console.info("query is " + query.getSqlLike());
        } catch (e) {
            console.error("should be ok on Method Chaining : " + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        query = null;
        done();
    })

    /**
     * @tc.name QueryGreatThanTest003
     * @tc.desc Test Js Api Query.GreatThan() testcase 003
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryGreatThanTest003', 0, async function(done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.greaterThan("key2", NaN);
            console.info("should throw exception on invalid arguments");
            console.info("query is " + query.getSqlLike());
        } catch (e) {
            expect(null).assertFail();
        }
        query = null;
        done();
    })

    /**
     * @tc.name QueryGreatThanTest004
     * @tc.desc Test Js Api Query.GreatThan() testcase 004
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryGreatThanTest004', 0, async function(done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.greaterThan("key1", "value", "too more", 4);
            console.info("should throw exception on invalid arguments");
            expect(null).assertFail();
        } catch (e) {
            console.error("throw exception is ok : " + `, error code is ${e.code}, message is ${e.message}`);
        }
        query = null;
        done();
    })

    /**
     * @tc.name QueryLessThanTest001
     * @tc.desc Test Js Api Query.LessThan() testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryLessThanTest001', 0, async function(done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.lessThan("key1", 5);
            query.lessThan("key2", 5.0);
            query.lessThan("key3", true);
            query.lessThan("key4", "string");
            expect(query.getSqlLike() !== "").assertTrue();
            console.info("query is " + query.getSqlLike());
        } catch (e) {
            console.error("dumplicated calls should be ok : " + `, error code is ${e.code}, message is ${e.message}`);
        }
        query = null;
        done();
    })

    /**
     * @tc.name QueryLessThanTest002
     * @tc.desc Test Js Api Query.LessThan() testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryLessThanTest002', 0, async function(done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.lessThan("key", 5);
            query.reset();
            query.lessThan("key0", 5).lessThan("key1", "v5").lessThan("key3", false);
            expect(query.getSqlLike() !== "").assertTrue();
            console.info("query is " + query.getSqlLike());
        } catch (e) {
            console.error("should be ok on Method Chaining : " + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertTrue();
        }
        query = null;
        done();
    })

    /**
     * @tc.name QueryLessThanTest003
     * @tc.desc Test Js Api Query.LessThan() testcase 003
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryLessThanTest003', 0, async function(done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.lessThan("key2", NaN);
            console.info("query is " + query.getSqlLike());
        } catch (e) {
            expect(null).assertFail();
        }
        query = null;
        done();
    })

    /**
     * @tc.name QueryLessThanTest004
     * @tc.desc Test Js Api Query.LessThan() testcase 004
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryLessThanTest004', 0, async function(done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.lessThan("key1", "value", "too more", 4);
            console.info("query is " + query.getSqlLike());
            expect(null).assertFail();
        } catch (e) {
            console.error("throw exception is ok : " + `, error code is ${e.code}, message is ${e.message}`);
        }
        query = null;
        done();
    })

    /**
     * @tc.name QueryGreaterThanOrEqualToTest001
     * @tc.desc Test Js Api Query.GreaterThanOrEqualTo() testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryGreaterThanOrEqualToTest001', 0, async function(done) {
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
     * @tc.name QueryGreaterThanOrEqualToTest002
     * @tc.desc Test Js Api Query.GreaterThanOrEqualTo() testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryGreaterThanOrEqualToTest002', 0, async function(done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.greaterThanOrEqualTo("key", 5);
            query.reset();
            query.greaterThanOrEqualTo("key0", 5)
                .greaterThanOrEqualTo("key1", "v5")
                .greaterThanOrEqualTo("key3", false);
            expect(query.getSqlLike() !== "").assertTrue();
            console.info("query is " + query.getSqlLike());
        } catch (e) {
            console.error("should be ok on Method Chaining : " + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        query = null;
        done();
    })

    /**
     * @tc.name QueryGreaterThanOrEqualToTest003
     * @tc.desc Test Js Api Query.GreaterThanOrEqualTo() testcase 003
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryGreaterThanOrEqualToTest003', 0, async function(done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.greaterThanOrEqualTo("key2", NaN);
            expect(query.getSqlLike() !== "").assertTrue();
            console.info("query is " + query.getSqlLike());
        } catch (e) {
            expect(null).assertFail();
        }
        query = null;
        done();
    })

    /**
     * @tc.name QueryGreaterThanOrEqualToTest004
     * @tc.desc Test Js Api Query.GreaterThanOrEqualTo() testcase 004
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryGreaterThanOrEqualToTest004', 0, async function(done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.greaterThanOrEqualTo("key1", "value", "too more", 4);
            console.info("should throw exception on invalid arguments");
            expect(null).assertFail();
        } catch (e) {
            console.error("throw exception is ok : " + `, error code is ${e.code}, message is ${e.message}`);
        }
        query = null;
        done();
    })

    /**
     * @tc.name QueryLessThanOrEqualToTest001
     * @tc.desc Test Js Api Query.LessThanOrEqualTo() testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryLessThanOrEqualToTest001', 0, async function(done) {
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
     * @tc.name QueryLessThanOrEqualToTest002
     * @tc.desc Test Js Api Query.LessThanOrEqualTo() testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryLessThanOrEqualToTest002', 0, async function(done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.lessThanOrEqualTo("key", 5);
            query.reset();
            query.lessThanOrEqualTo("key0", 5).lessThanOrEqualTo("key1", "v5").lessThanOrEqualTo("key3", false);
            expect(query.getSqlLike() !== "").assertTrue();
            console.info("query is " + query.getSqlLike());
        } catch (e) {
            console.error("should be ok on Method Chaining : " + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        query = null;
        done();
    })

    /**
     * @tc.name QueryLessThanOrEqualToTest003
     * @tc.desc Test Js Api Query.LessThanOrEqualTo() testcase 003
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryLessThanOrEqualToTest003', 0, async function(done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.lessThanOrEqualTo("key2", NaN);
            console.info("query is " + query.getSqlLike());
        } catch (e) {
            expect(null).assertFail();
        }
        query = null;
        done();
    })

    /**
     * @tc.name QueryLessThanOrEqualToTest004
     * @tc.desc Test Js Api Query.LessThanOrEqualTo() testcase 004
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryLessThanOrEqualToTest004', 0, async function(done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.lessThanOrEqualTo("key1", "value", "too more", 4);
            console.info("should throw exception on invalid arguments");
            expect(null).assertFail();
        } catch (e) {
            console.error("throw exception is ok : " + `, error code is ${e.code}, message is ${e.message}`);
        }
        query = null;
        done();
    })

    /**
     * @tc.name QueryIsNullTest001
     * @tc.desc Test Js Api Query.IsNull() testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryIsNullTest001', 0, async function(done) {
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
     * @tc.name QueryIsNullTest002
     * @tc.desc Test Js Api Query.IsNull() testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryIsNullTest002', 0, async function(done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.isNull("key").notEqualTo("key1", 4).isNull("key2");
            expect(query.getSqlLike() !== "").assertTrue();
            console.info("query is " + query.getSqlLike());
        } catch (e) {
            console.error("should be ok on Method Chaining : " + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        query = null;
        done();
    })

    /**
     * @tc.name QueryIsNullTest003
     * @tc.desc Test Js Api Query.IsNull() testcase 003
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryIsNullTest003', 0, async function(done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.isNull("key", 0);
            console.info("should throw exception on invalid arguments");
            expect(null).assertFail();
        } catch (e) {
            console.error("throw exception is ok : " + `, error code is ${e.code}, message is ${e.message}`);
        }
        query = null;
        done();
    })

    /**
     * @tc.name QueryIsNullTest004
     * @tc.desc Test Js Api Query.IsNull() testcase 004
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryIsNullTest004', 0, async function(done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.isNull(0);
            console.info("should throw exception on invalid arguments");
            expect(null).assertFail();
        } catch (e) {
            console.error("throw exception is ok : " + `, error code is ${e.code}, message is ${e.message}`);
        }
        query = null;
        done();
    })

    /*
    * =======================================================================================
    *           Int8Array             |  INTEGER
    *           Uint8Array            |  INTEGER
    *           Uint8ClampedArray     |  INTEGER
    *           Int16Array            |  INTEGER
    *           Uint16Array           |  INTEGER
    *           Int32Array            |  INTEGER
    *           Uint32Array           |  LONG
    *           Float32Array          |  DOUBLE
    *           Float64Array          |  DOUBLE
    *           BigInt64Array         |  ERROR: cannot convert to bigint
    *           BigUint64Array        |  ERROR: cannot convert to bigint
    * =======================================================================================
	*           Array                 |  DOUBLE    * not-typedArray treated as array of double.
    */

    /**
     * @tc.name QueryInNumberTest001
     * @tc.desc Test Js Api Query.InNumber() testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryInNumberTest001', 0, async function(done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            console.info("testInNumber001 start ");
            var i8 = new Int8Array([-21,31]);
            query.reset().inNumber("key", i8);
            console.info("inNumber(Int8Array([-21,31])  => " + query.getSqlLike());
            var u8 = new Uint8Array([-21,31]);
            query.reset().inNumber("key", u8);
            console.info("inNumber(Uint8Array([-21,31])  => " + query.getSqlLike());
            var c8 = new Uint8ClampedArray([-21,31]);
            query.reset().inNumber("key", c8);
            console.info("inNumber(Uint8Array([-21,31])  => " + query.getSqlLike());
            var i16 = new Int16Array([-21,31]);
            query.reset().inNumber("key", i16);
            console.info("inNumber(Int16Array([-21,31])  => " + query.getSqlLike());
            var u16 = new Uint16Array([-21,31]);
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
            var f32e = new Float32Array([21, 31, "a"]); // "a" will be ignored as not a float.
            query.reset().inNumber("key", f32e);
            console.info("inNumber(Float32Array([-21,31, 'a'])  => " + query.getSqlLike());
            var f64 = new Float64Array([-21, 31]);
            query.reset().inNumber("key", f64);
            console.info("inNumber(Float64Array([-21,31])  => " + query.getSqlLike());
            query.reset();
        } catch (e) {
            console.error("dumplicated calls should be ok : " + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        query = null;
        done();
    })

    /**
     * @tc.name QueryInNumberTest002
     * @tc.desc Test Js Api Query.InNumber() testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryInNumberTest002', 0, async function(done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.inNumber("key", [1, 2.3, 987654]).
                inNumber("key2", [0x10abcdef]).
                inNumber("key2", [0xf0123456]).
                inNumber("key2", [0b10101]);
            expect(query.getSqlLike() !== "").assertTrue();
            console.info("query is " + query.getSqlLike());
        } catch (e) {
            console.error("should be ok on Method Chaining : " + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        query = null;
        done();
    })

    /**
     * @tc.name QueryInNumberTest003
     * @tc.desc Test Js Api Query.InNumber() testcase 003
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryInNumberTest003', 0, async function(done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.inNumber("key", 0);
            console.info("should throw exception on invalid arguments");
            expect(null).assertFail();
        } catch (e) {
            console.error("throw exception is ok : " + `, error code is ${e.code}, message is ${e.message}`);
        }
        query = null;
        done();
    })

    /**
     * @tc.name QueryInNumberTest004
     * @tc.desc Test Js Api Query.InNumber() testcase 004
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryInNumberTest004', 0, async function(done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.inNumber([0, 1]);
            console.info("should throw exception on invalid arguments");
            expect(null).assertFail();
        } catch (e) {
            console.error("throw exception is ok : " + `, error code is ${e.code}, message is ${e.message}`);
        }
        query = null;
        done();
    })

    /**
     * @tc.name QueryInNumberTest005
     * @tc.desc Test Js Api Query.InNumber() testcase 005
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryInNumberTest005', 0, async function(done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            var u64 = new BigUint64Array([21, 31]);
            query.inNumber("key", u64);
            var b64 = new BigInt64Array([21, 31]);
            query.inNumber("key", b64);
            console.info("should throw exception on invalid arguments");
            expect(null).assertFail();
        } catch (e) {
            console.error("throw exception is ok : " + `, error code is ${e.code}, message is ${e.message}`);
        }
        query = null;
        done();
    })

    /**
     * @tc.name QueryInNumberTest006
     * @tc.desc Test Js Api Query.InNumber() testcase 006
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryInNumberTest006', 0, async function(done) {
        var query = null;
        try {
            query = new ddm.Query();
            console.info("typeof([1, 2, 97])" + typeof([1, 2, 97]))
            console.info("typeof([1, 2, 97][0])" + typeof([1, 2, 97][0]))
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
     * @tc.name QueryInStringTest001
     * @tc.desc Test Js Api Query.InString() testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryInStringTest001', 0, async function(done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.inString("key", ["a2z" , 'z2a']);
            query.inString("key2", ["AAA" ]);
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
     * @tc.name QueryInStringTest002
     * @tc.desc Test Js Api Query.InString() testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryInStringTest002', 0, async function(done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.inString("key", ["a2z" , 'z2a'])
                .inString("key2", ["AAA" ])
                .inString("key2", ["AAA", "AAABBB","CCCAAA" ]);
            expect(query.getSqlLike() !== "").assertTrue();
            console.info("query is " + query.getSqlLike());
        } catch (e) {
            console.error("should be ok on Method Chaining : " + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        query = null;
        done();
    })

    /**
     * @tc.name QueryInStringTest003
     * @tc.desc Test Js Api Query.InString() testcase 003
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryInStringTest003', 0, async function(done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.inString("key", 0);
            console.info("should throw exception on invalid arguments");
            expect(null).assertFail();
        } catch (e) {
            console.error("throw exception is ok : " + `, error code is ${e.code}, message is ${e.message}`);
        }
        query = null;
        done();
    })

    /**
     * @tc.name QueryInStringTest004
     * @tc.desc Test Js Api Query.InString() testcase 004
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryInStringTest004', 0, async function(done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.inString("key", [0, 1]);
            console.info("should throw exception on invalid arguments");
            expect(null).assertFail();
        } catch (e) {
            console.error("throw exception is ok : " + `, error code is ${e.code}, message is ${e.message}`);
        }
        query = null;
        done();
    })

    /**
     * @tc.name QueryNotInNumberTest001
     * @tc.desc Test Js Api Query.NotInNumber() testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryNotInNumberTest001', 0, async function(done) {
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
     * @tc.name QueryNotInNumberTest002
     * @tc.desc Test Js Api Query.NotInNumber() testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryNotInNumberTest002', 0, async function(done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.notInNumber("key", [1, 2, 3]).notInNumber("key", [1, 7, 8]).notEqualTo("kkk", 5);
            expect(query.getSqlLike() !== "").assertTrue();
            console.info("query is " + query.getSqlLike());
        } catch (e) {
            console.error("should be ok on Method Chaining : " + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        query = null;
        done();
    })

    /**
     * @tc.name QueryNotInNumberTest003
     * @tc.desc Test Js Api Query.NotInNumber() testcase 003
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryNotInNumberTest003', 0, async function(done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.notInNumber("key", [1], 2);
            console.info("should throw exception on invalid arguments");
            expect(null).assertFail();
        } catch (e) {
            console.error("throw exception is ok : " + `, error code is ${e.code}, message is ${e.message}`);
        }
        query = null;
        done();
    })

    /**
     * @tc.name QueryNotInNumberTest004
     * @tc.desc Test Js Api Query.NotInNumber() testcase 004
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryNotInNumberTest004', 0, async function(done) {
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
        }
        query = null;
        done();
    })

    /**
     * @tc.name QueryNotInStringTest001
     * @tc.desc Test Js Api Query.NotInString() testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryNotInStringTest001', 0, async function(done) {
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
     * @tc.name QueryNotInStringTest002
     * @tc.desc Test Js Api Query.NotInString() testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryNotInStringTest002', 0, async function(done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.notInString("key", ["v1", "v2", "v3"]).notEqualTo("kkk", "v3");
            expect(query.getSqlLike() !== "").assertTrue();
            console.info("query is " + query.getSqlLike());
        } catch (e) {
            console.error("should be ok on Method Chaining : " + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        query = null;
        done();
    })

    /**
     * @tc.name QueryNotInStringTest003
     * @tc.desc Test Js Api Query.NotInString() testcase 003
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryNotInStringTest003', 0, async function(done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.notInString("key", ["", "abccd"], 2);
            console.info("should throw exception on invalid arguments");
            expect(null).assertFail();
        } catch (e) {
            console.error("throw exception is ok : " + `, error code is ${e.code}, message is ${e.message}`);
        }
        query = null;
        done();
    })

    /**
     * @tc.name QueryNotInStringTest004
     * @tc.desc Test Js Api Query.NotInString() testcase 004
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryNotInStringTest004', 0, async function(done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.notInString("key", [1, 2]);
            console.info("should throw exception on invalid arguments");
            expect(null).assertFail();
        } catch (e) {
            console.error("throw exception is ok : " + `, error code is ${e.code}, message is ${e.message}`);
        }
        query = null;
        done();
    })

    /**
     * @tc.name QueryLikeTest001
     * @tc.desc Test Js Api Query.Like() testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryLikeTest001', 0, async function(done) {
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
     * @tc.name QueryLikeTest002
     * @tc.desc Test Js Api Query.Like() testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryLikeTest002', 0, async function(done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.like("key", "v1").like("key", "v3").like("key", "v2");
            expect(query.getSqlLike() !== "").assertTrue();
            console.info("query is " + query.getSqlLike());
        } catch (e) {
            console.error("should be ok on Method Chaining : " + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        query = null;
        done();
    })

    /**
     * @tc.name QueryLikeTest003
     * @tc.desc Test Js Api Query.Like() testcase 003
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryLikeTest003', 0, async function(done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.like("key", 0);
            console.info("should throw exception on invalid arguments");
            expect(null).assertFail();
        } catch (e) {
            console.error("throw exception is ok : " + `, error code is ${e.code}, message is ${e.message}`);
        }
        query = null;
        done();
    })

    /**
     * @tc.name QueryLikeTest004
     * @tc.desc Test Js Api Query.Like() testcase 004
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryLikeTest004', 0, async function(done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.like("key", "str1", "str2");
            console.info("should throw exception on invalid arguments");
            expect(null).assertFail();
        } catch (e) {
            console.error("throw exception is ok : " + `, error code is ${e.code}, message is ${e.message}`);
        }
        query = null;
        done();
    })

    /**
     * @tc.name QueryUnlikeTest001
     * @tc.desc Test Js Api Query.Unlike() testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryUnlikeTest001', 0, async function(done) {
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
     * @tc.name QueryUnlikeTest002
     * @tc.desc Test Js Api Query.Unlike() testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryUnlikeTest002', 0, async function(done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.unlike("key", "v1").unlike("key", "v3").unlike("key", "v2");
            expect(query.getSqlLike() !== "").assertTrue();
            console.info("query is " + query.getSqlLike());
        } catch (e) {
            console.error("should be ok on Method Chaining : " + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        query = null;
        done();
    })

    /**
     * @tc.name QueryUnlikeTest003
     * @tc.desc Test Js Api Query.Unlike() testcase 003
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryUnlikeTest003', 0, async function(done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.unlike("key", 0);
            console.info("should throw exception on invalid arguments");
            expect(null).assertFail();
        } catch (e) {
            console.error("throw exception is ok : " + `, error code is ${e.code}, message is ${e.message}`);
        }
        query = null;
        done();
    })

    /**
     * @tc.name QueryUnlikeTest004
     * @tc.desc Test Js Api Query.Unlike() testcase 004
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryUnlikeTest004', 0, async function(done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.unlike("key", "str1", "str2");
            console.info("should throw exception on invalid arguments");
            expect(null).assertFail();
        } catch (e) {
            console.error("throw exception is ok : " + `, error code is ${e.code}, message is ${e.message}`);
        }
        query = null;
        done();
    })

    /**
     * @tc.name QueryAndTest001
     * @tc.desc Test Js Api Query.And() testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryAndTest001', 0, async function(done) {
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
     * @tc.name QueryAndTest002
     * @tc.desc Test Js Api Query.And() testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryAndTest002', 0, async function(done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.equalTo("key1", 0).and().equalTo("key2", "v1");
            expect(query.getSqlLike() !== "").assertTrue();
            console.info("query is " + query.getSqlLike());
        } catch (e) {
            console.error("should be ok on Method Chaining : " + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        query = null;
        done();
    })

    /**
     * @tc.name QueryAndTest003
     * @tc.desc Test Js Api Query.And() testcase 003
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryAndTest003', 0, async function(done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.notEqualTo("key", 0).and().notEqualTo("key", 1).and();
            expect(query.getSqlLike() !== "").assertTrue();
            query.reset();
            console.info("query is " + query.getSqlLike());
        } catch (e) {
            expect(null).assertFail();
        }
        query = null;
        done();
    })

    /**
     * @tc.name QueryAndTest004
     * @tc.desc Test Js Api Query.And() testcase 004
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryAndTest004', 0, async function(done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.notEqualTo("key", 0).and(1).notInNumber("key", [1, 3]);
            console.info("should throw exception on invalid arguments");
            expect(null).assertFail();
        } catch (e) {
            console.error("throw exception is ok : " + `, error code is ${e.code}, message is ${e.message}`);
        }
        query = null;
        done();
    })

    /**
     * @tc.name QueryOrTest001
     * @tc.desc Test Js Api Query.Or() testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryOrTest001', 0, async function(done) {
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
     * @tc.name QueryOrTest002
     * @tc.desc Test Js Api Query.Or() testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryOrTest002', 0, async function(done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.equalTo("key1", 0).or().equalTo("key2", "v1");
            expect(query.getSqlLike() !== "").assertTrue();
            query.reset();
            console.info("query is " + query.getSqlLike());
        } catch (e) {
            console.error("should be ok on Method Chaining : " + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        query = null;
        done();
    })

    /**
     * @tc.name QueryOrTest003
     * @tc.desc Test Js Api Query.Or() testcase 003
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryOrTest003', 0, async function(done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.notEqualTo("key", 0).or();
            console.info("or ... sql:" + query.getSqlLike());
            expect(query.getSqlLike() !== "").assertTrue();
            query.reset();
            console.info("query is " + query.getSqlLike());
        } catch (e) {
            console.error("throw exception is ok : " + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        query = null;
        done();
    })

    /**
     * @tc.name QueryOrTest004
     * @tc.desc Test Js Api Query.Or() testcase 004
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryOrTest004', 0, async function(done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.notEqualTo("key", 0).or(1).notInNumber("key", [1, 3]);
            console.info("should throw exception on invalid arguments");
            expect(null).assertFail();
        } catch (e) {
            console.error("throw exception is ok : " + `, error code is ${e.code}, message is ${e.message}`);
        }
        query = null;
        done();
    })

    /**
     * @tc.name QueryOrderByAscTest001
     * @tc.desc Test Js Api Query.OrderByAsc() testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryOrderByAscTest001', 0, async function(done) {
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
     * @tc.name QueryOrderByAscTest002
     * @tc.desc Test Js Api Query.OrderByAsc() testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryOrderByAscTest002', 0, async function(done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.notEqualTo("key", "V0").orderByAsc("sortbykey1").orderByAsc("sortbykey2");
            expect(query.getSqlLike() !== "").assertTrue();
            console.info("query is " + query.getSqlLike());
        } catch (e) {
            console.error("should be ok on Method Chaining : " + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        query = null;
        done();
    })

    /**
     * @tc.name QueryOrderByAscTest003
     * @tc.desc Test Js Api Query.OrderByAsc() testcase 003
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryOrderByAscTest003', 0, async function(done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.notEqualTo("key", false).orderByAsc(1);
            console.info("should throw exception on invalid arguments");
            expect(null).assertFail();
        } catch (e) {
            console.error("throw exception is ok : " + `, error code is ${e.code}, message is ${e.message}`);
        }
        query = null;
        done();
    })

    /**
     * @tc.name QueryOrderByAscTest004
     * @tc.desc Test Js Api Query.OrderByAsc() testcase 004
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryOrderByAscTest004', 0, async function(done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.orderByAsc();
            console.info("should throw exception on invalid arguments");
            expect(null).assertFail();
        } catch (e) {
            console.error("throw exception is ok : " + `, error code is ${e.code}, message is ${e.message}`);
        }
        query = null;
        done();
    })

    /**
     * @tc.name QueryOrderByDescTest001
     * @tc.desc Test Js Api Query.OrderByDesc() testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryOrderByDescTest001', 0, async function(done) {
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
     * @tc.name QueryOrderByDescTest002
     * @tc.desc Test Js Api Query.OrderByDesc() testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryOrderByDescTest002', 0, async function(done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.notEqualTo("key", "V0").orderByDesc("sortbykey1").orderByDesc("sortbykey2");
            expect(query.getSqlLike() !== "").assertTrue();
            console.info("query is " + query.getSqlLike());
        } catch (e) {
            console.error("should be ok on Method Chaining : " + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        query = null;
        done();
    })

    /**
     * @tc.name QueryOrderByDescTest003
     * @tc.desc Test Js Api Query.OrderByDesc() testcase 003
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryOrderByDescTest003', 0, async function(done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.notEqualTo("key", false).orderByDesc(1);
            console.info("should throw exception on invalid arguments");
            expect(null).assertFail();
        } catch (e) {
            console.error("throw exception is ok : " + `, error code is ${e.code}, message is ${e.message}`);
        }
        query = null;
        done();
    })

    /**
     * @tc.name QueryOrderByDescTest004
     * @tc.desc Test Js Api Query.OrderByDesc() testcase 004
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryOrderByDescTest004', 0, async function(done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.orderByDesc();
            console.info("should throw exception on invalid arguments");
            expect(null).assertFail();
        } catch (e) {
            console.error("throw exception is ok : " + `, error code is ${e.code}, message is ${e.message}`);
        }
        query = null;
        done();
    })

    /**
     * @tc.name QueryLimitTest001
     * @tc.desc Test Js Api Query.Limit() testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryLimitTest001', 0, async function(done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.notEqualTo("key", "vx");
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
     * @tc.name QueryLimitTest002
     * @tc.desc Test Js Api Query.Limit() testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryLimitTest002', 0, async function(done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.notEqualTo("key", "vx").limit(10, 2)
                .equalTo("key2", 2).limit(10, 2);
            expect(query.getSqlLike() !== "").assertTrue();
            console.info("query is " + query.getSqlLike());
        } catch (e) {
            console.error("should be ok on Method Chaining : " + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        query = null;
        done();
    })

    /**
     * @tc.name QueryLimitTest003
     * @tc.desc Test Js Api Query.Limit() testcase 003
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryLimitTest003', 0, async function(done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.notEqualTo("key", false).limit(10, 2, "any");
            console.info("should throw exception on invalid arguments");
            expect(null).assertFail();
            console.info("query is " + query.getSqlLike());
        } catch (e) {
            console.error("throw exception is ok : " + `, error code is ${e.code}, message is ${e.message}`);
        }
        query = null;
        done();
    })

    /**
     * @tc.name QueryLimitTest004
     * @tc.desc Test Js Api Query.Limit() testcase 004
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryLimitTest004', 0, async function(done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.notEqualTo("key", false).limit(10);
            console.info("should throw exception on invalid arguments");
            expect(null).assertFail();
        } catch (e) {
            console.error("throw exception is ok : " + `, error code is ${e.code}, message is ${e.message}`);
        }
        query = null;
        done();
    })

    /**
     * @tc.name QueryLimitTest005
     * @tc.desc Test Js Api Query.Limit() testcase 005
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryLimitTest005', 0, async function(done) {
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
        }
        query = null;
        done();
    })

    /**
     * @tc.name QueryIsNotNullTest001
     * @tc.desc Test Js Api Query.IsNotNull() testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryIsNotNullTest001', 0, async function(done) {
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
     * @tc.name QueryIsNotNullTest002
     * @tc.desc Test Js Api Query.IsNotNull() testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryIsNotNullTest002', 0, async function(done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.isNotNull("key1").and().notEqualTo("key1", 123);
            expect(query.getSqlLike() !== "").assertTrue();
            console.info("query is " + query.getSqlLike());
        } catch (e) {
            console.error("should be ok on Method Chaining : " + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        query = null;
        done();
    })

    /**
     * @tc.name QueryIsNotNullTest003
     * @tc.desc Test Js Api Query.IsNotNull() testcase 003
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryIsNotNullTest003', 0, async function(done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.isNotNull("key2", "any");
            console.info("should throw exception on invalid arguments");
            expect(null).assertFail();
        } catch (e) {
            console.error("throw exception is ok : " + `, error code is ${e.code}, message is ${e.message}`);
        }
        query = null;
        done();
    })

    /**
     * @tc.name QueryIsNotNullTest004
     * @tc.desc Test Js Api Query.IsNotNull() testcase 004
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryIsNotNullTest004', 0, async function(done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.isNotNull(1);
            console.info("should throw exception on invalid arguments");
            expect(null).assertFail();
        } catch (e) {
            console.error("throw exception is ok : " + `, error code is ${e.code}, message is ${e.message}`);
        }
        query = null;
        done();
    })

    /**
     * @tc.name QueryBeginGroupTest001
     * @tc.desc Test Js Api Query.BeginGroup() testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryBeginGroupTest001', 0, async function(done) {
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
     * @tc.name QueryBeginGroupTest002
     * @tc.desc Test Js Api Query.BeginGroup() testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryBeginGroupTest002', 0, async function(done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.beginGroup();
            query.beginGroup();
            query.notEqualTo("$.name", 0);
            query.endGroup();
            query.beginGroup();
            expect(query.getSqlLike() !== "").assertTrue();
            console.info("query is " + query.getSqlLike());
        } catch (e) {
            console.error("should be ok on Method Chaining : " + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        query = null;
        done();
    })

    /**
     * @tc.name QueryBeginGroupTest003
     * @tc.desc Test Js Api Query.BeginGroup() testcase 003
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryBeginGroupTest003', 0, async function(done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.beginGroup(1);
            console.info("should throw exception on invalid arguments");
            expect(null).assertFail();
        } catch (e) {
            console.error("throw exception is ok : " + `, error code is ${e.code}, message is ${e.message}`);
        }
        query = null;
        done();
    })

    /**
     * @tc.name QueryBeginGroupTest004
     * @tc.desc Test Js Api Query.BeginGroup() testcase 004
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryBeginGroupTest004', 0, async function(done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.beginGroup("any", 1);
            console.info("should throw exception on invalid arguments");
            expect(null).assertFail();
        } catch (e) {
            console.error("throw exception is ok : " + `, error code is ${e.code}, message is ${e.message}`);
        }
        query = null;
        done();
    })

    /**
     * @tc.name QueryEndGroupTest001
     * @tc.desc Test Js Api Query.EndGroup() testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryEndGroupTest001', 0, async function(done) {
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
            expect(null).assertFail();
        }
        query = null;
        done();
    })

    /**
     * @tc.name QueryEndGroupTest002
     * @tc.desc Test Js Api Query.EndGroup() testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryEndGroupTest002', 0, async function(done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.endGroup();
            query.beginGroup();
            query.isNotNull("$.name");
            query.endGroup();
            expect(query.getSqlLike() !== "").assertTrue();
            console.info("query is " + query.getSqlLike());
        } catch (e) {
            expect(null).assertFail();
        }
        query = null;
        done();
    })

    /**
     * @tc.name QueryEndGroupTest003
     * @tc.desc Test Js Api Query.EndGroup() testcase 003
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryEndGroupTest003', 0, async function(done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.endGroup(0);
            console.info("should throw exception on invalid arguments");
            expect(null).assertFail();
        } catch (e) {
            console.error("throw exception is ok : " + `, error code is ${e.code}, message is ${e.message}`);
        }
        query = null;
        done();
    })

    /**
     * @tc.name QueryEndGroupTest004
     * @tc.desc Test Js Api Query.EndGroup() testcase 004
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryEndGroupTest004', 0, async function(done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.endGroup("any");
            console.info("should throw exception on invalid arguments");
            expect(null).assertFail();
        } catch (e) {
            console.error("throw exception is ok : " + `, error code is ${e.code}, message is ${e.message}`);
        }
        query = null;
        done();
    })

    /**
     * @tc.name QueryPrefixKeyTest001
     * @tc.desc Test Js Api Query.PrefixKey() testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryPrefixKeyTest001', 0, async function(done) {
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
     * @tc.name QueryPrefixKeyTest002
     * @tc.desc Test Js Api Query.PrefixKey() testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryPrefixKeyTest002', 0, async function(done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.prefixKey("kx1").or().prefixKey("kx2").or().prefixKey("kx3");
            expect(query.getSqlLike() !== "").assertTrue();
            console.info("query is " + query.getSqlLike());
        } catch (e) {
            expect(null).assertFail();
        }
        query = null;
        done();
    })

    /**
     * @tc.name QueryPrefixKeyTest003
     * @tc.desc Test Js Api Query.PrefixKey() testcase 003
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryPrefixKeyTest003', 0, async function(done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.prefixKey("k", "any");
            console.info("should throw exception on invalid arguments");
            expect(null).assertFail();
        } catch (e) {
            console.error("throw exception is ok : " + `, error code is ${e.code}, message is ${e.message}`);
        }
        query = null;
        done();
    })

    /**
     * @tc.name QueryPrefixKeyTest004
     * @tc.desc Test Js Api Query.PrefixKey() testcase 004
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryPrefixKeyTest004', 0, async function(done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.prefixKey(123);
            console.info("should throw exception on invalid arguments");
            expect(null).assertFail();
        } catch (e) {
            console.error("throw exception is ok : " + `, error code is ${e.code}, message is ${e.message}`);
        }
        query = null;
        done();
    })

    /**
     * @tc.name QuerySetSuggestIndexTest001
     * @tc.desc Test Js Api Query.SetSuggestIndex() testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QuerySetSuggestIndexTest001', 0, async function(done) {
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
     * @tc.name QuerySetSuggestIndexTest002
     * @tc.desc Test Js Api Query.SetSuggestIndex() testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QuerySetSuggestIndexTest002', 0, async function(done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.setSuggestIndex("kxx").or().equalTo("key2", "v1");
            expect(query.getSqlLike() !== "").assertTrue();
            console.info("query is " + query.getSqlLike());
        } catch (e) {
            expect(null).assertFail();
        }
        query = null;
        done();
    })

    /**
     * @tc.name QuerySetSuggestIndexTest003
     * @tc.desc Test Js Api Query.SetSuggestIndex() testcase 003
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QuerySetSuggestIndexTest003', 0, async function(done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.setSuggestIndex("k", "any");
            console.info("should throw exception on invalid arguments");
            expect(null).assertFail();
        } catch (e) {
            console.error("throw exception is ok : " + `, error code is ${e.code}, message is ${e.message}`);
        }
        query = null;
        done();
    })

    /**
     * @tc.name QuerySetSuggestIndexTest004
     * @tc.desc Test Js Api Query.SetSuggestIndex() testcase 004
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QuerySetSuggestIndexTest004', 0, async function(done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.setSuggestIndex(123);
            console.info("should throw exception on invalid arguments");
            expect(null).assertFail();
        } catch (e) {
            console.error("throw exception is ok : " + `, error code is ${e.code}, message is ${e.message}`);
        }
        query = null;
        done();
    })

    /**
     * @tc.name QueryDeviceIdTest001
     * @tc.desc Test Js Api Query.DeviceId() testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryDeviceIdTest001', 0, async function(done) {
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
     * @tc.name QueryDeviceIdTest002
     * @tc.desc Test Js Api Query.DeviceId() testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryDeviceIdTest002', 0, async function(done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.deviceId("kxx").equalTo("key2", "v1");
            expect(query.getSqlLike() !== "").assertTrue();
            console.info("query is " + query.getSqlLike());
        } catch (e) {
            console.error("should be ok on Method Chaining : " + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        query = null;
        done();
    })

    /**
     * @tc.name QueryDeviceIdTest003
     * @tc.desc Test Js Api Query.DeviceId() testcase 003
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryDeviceIdTest003', 0, async function(done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.deviceId("k", "any");
            console.info("should throw exception on invalid arguments");
            expect(null).assertFail();
        } catch (e) {
            console.error("throw exception is ok : " + `, error code is ${e.code}, message is ${e.message}`);
        }
        query = null;
        done();
    })

    /**
     * @tc.name QueryDeviceIdTest004
     * @tc.desc Test Js Api Query.DeviceId() testcase 004
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryDeviceIdTest004', 0, async function(done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.deviceId(123);
            console.info("should throw exception on invalid arguments");
            expect(null).assertFail();
        } catch (e) {
            console.error("throw exception is ok : " + `, error code is ${e.code}, message is ${e.message}`);
        }
        query = null;
        done();
    })

    /**
     * @tc.name QueryGetSqlLikeTest001
     * @tc.desc Test Js Api Query.GetSqlLike() testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryGetSqlLikeTest001', 0, async function(done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            let sql1 = query.getSqlLike();
            console.info("testGetSqlLike001 sql=" + sql1);
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
     * @tc.name QueryGetSqlLikeTest002
     * @tc.desc Test Js Api Query.GetSqlLike() testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryGetSqlLikeTest002', 0, async function(done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            let sql1 = query.getSqlLike();
            console.info("testGetSqlLike002 sql=" + sql1);
            query.inString("key1", ["AAA", "BBB"])
                .or()
                .notEqualTo("key2", 0);
            let sql2 = query.getSqlLike();
            console.info("testGetSqlLike002 sql=" + sql2);
            console.info("query is " + query.getSqlLike());
            expect(sql1 !== sql2).assertTrue();
        } catch (e) {
            console.error("should be ok on Method Chaining : " + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        query = null;
        done();
    })

    /**
     * @tc.name QueryGetSqlLikeTest003
     * @tc.desc Test Js Api Query.GetSqlLike() testcase 003
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryGetSqlLikeTest003', 0, async function(done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.inNumber("key");
            query.getSqlLike(0);
            console.info("should throw exception on invalid arguments");
            expect(null).assertFail();
        } catch (e) {
            console.error("throw exception is ok : " + `, error code is ${e.code}, message is ${e.message}`);
        }
        query = null;
        done();
    })

    /**
     * @tc.name QueryGetSqlLikeTest004
     * @tc.desc Test Js Api Query.GetSqlLike() testcase 004
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryGetSqlLikeTest004', 0, async function(done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.inNumber("key");
            query.getSqlLike("any");
            console.info("should throw exception on invalid arguments");
            expect(null).assertFail();
        } catch (e) {
            console.error("throw exception is ok : " + `, error code is ${e.code}, message is ${e.message}`);
        }
        query = null;
        done();
    })
})
