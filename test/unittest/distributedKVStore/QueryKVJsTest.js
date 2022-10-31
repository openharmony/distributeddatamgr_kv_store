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
     * @tc.nameQueryResetOneSucTest
     * @tc.desc: Test Js Api Query.reset() reset one calls successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryResetOneSucTest', 0, async function (done) {
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
     * @tc.nameQueryResetDumpSucTest
     * @tc.desc: Test Js Api Query.reset() reset dumplicated calls successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryResetDumpSucTest', 0, async function (done) {
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
     * @tc.nameQueryResetCalAfterResetTest
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
     * @tc.nameQueryResetInvalidArgumentsTest
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
     * @tc.nameQueryEqualToSucTest
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
     * @tc.nameQueryEqualToMChainCallsTest
     * @tc.desc: Test Js Api Query.equalTo() with chaining calls successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryEqualToMChainCallsTest', 0, async function (done) {
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
     * @tc.nameQueryEqualToNanTest
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
     * @tc.nameQueryEqualToInvalidArgTest
     * @tc.desc: Test Js Api Query.equalTo() with invalid arguments
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryEqualToInvalidArgTest', 0, async function (done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.equalTo("key1", "value", "too more");
            console.info("should throw exception on invalid arguments");
            console.info("query is " + query.getSqlLike());
            expect(query.getSqlLike() !== "").assertTrue();
        } catch (e) {
            console.error("throw exception is ok : " + `, error code is ${e.code}, message is ${e.message}`);
            expect(true).assertTrue();
        }
        query = null;
        done();
    })

    /**
     * @tc.nameQueryNotEqualToSucTest
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
            console.info("query is " + query.getSqlLike());
        } catch (e) {
            console.error("dumplicated calls should be ok : " + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        query = null;
        done();
    })

    /**
     * @tc.nameQueryNotEqualToChainCallsTest
     * @tc.desc: Test Js Api Query.equalTo() with chaining calls successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryNotEqualToChainCallsTest', 0, async function (done) {
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
     * @tc.nameQueryNotEqualToNanTest
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
            console.info("query is " + query.getSqlLike());
        } catch (e) {
            expect(null).assertFail();
        }
        query = null;
        done();
    })

    /**
     * @tc.nameQueryNotEqualToInvalidArgTest
     * @tc.desc: Test Js Api Query.equalTo() with invalid arguments
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryNotEqualToInvalidArgTest', 0, async function (done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.notEqualTo("key1", "value", "too more", 4);
            console.info("should not throw exception on invalid arguments");
        } catch (e) {
            console.log("throw exception : " + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        query = null;
        done();
    })

    /**
     * @tc.nameQueryGreaterThanSucTest
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
     * @tc.nameQueryGreatThanChainCallsTest
     * @tc.desc: Test Js Api Query.GreatThan() with chaining calls
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryGreatThanChainCallsTest', 0, async function (done) {
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
     * @tc.nameQueryGreatThanNanTest
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
            console.info("should throw exception on invalid arguments");
            console.info("query is " + query.getSqlLike());
        } catch (e) {
            expect(null).assertFail();
        }
        query = null;
        done();
    })

    /**
     * @tc.nameQueryGreatThanInvalidArgsTest
     * @tc.desc: Test Js Api Query.GreatThan() with invalid arguments
     * @tc.type: FUNC
     * @tc.require: issueNumber
     **/
    it('QueryGreatThanInvalidArgsTest', 0, async function (done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.greaterThan("key1", "value", "too more", 4);
            console.info("should throw exception on invalid arguments");
            expect(query.getSqlLike() !== "").assertTrue();
        } catch (e) {
            console.error("throw exception : " + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        query = null;
        done();
    })

    /**
     * @tc.nameQueryLessThanSucTest
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
            console.info("query is " + query.getSqlLike());
        } catch (e) {
            console.error("dumplicated calls should be ok : " + `, error code is ${e.code}, message is ${e.message}`);
        }
        query = null;
        done();
    })

    /**
     * @tc.nameQueryLessThanChainCallsTest
     * @tc.desc: Test Js Api Query.LessThan() with chaining calls
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryLessThanChainCallsTest', 0, async function (done) {
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
     * @tc.nameQueryLessThanNanTest
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
        } catch (e) {
            expect(null).assertFail();
        }
        query = null;
        done();
    })

    /**
     * @tc.nameQueryLessThanInvalidArgsTest
     * @tc.desc: Test Js Api Query.LessThan() with invalid arguments
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryLessThanInvalidArgsTest', 0, async function (done) {
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
     * @tc.nameQueryGreaterThanOrEqualToSucTest
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
     * @tc.nameQueryGreaterThanOrEqualToChainCallsTest
     * @tc.desc: Test Js Api Query.GreaterThanOrEqualTo() with chaining calls
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryGreaterThanOrEqualToChainCallsTest', 0, async function (done) {
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
     * @tc.nameQueryGreaterThanOrEqualToNanTest
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
     * @tc.nameQueryGreaterThanOrEqualToInvalidArgsTest
     * @tc.desc: Test Js Api Query.GreaterThanOrEqualTo() with invalid arguments
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryGreaterThanOrEqualToInvalidArgsTest', 0, async function (done) {
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
     * @tc.nameQueryLessThanOrEqualToSucTest
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
     * @tc.nameQueryLessThanOrEqualToChainCallsTest
     * @tc.desc: Test Js Api Query.LessThanOrEqualTo() with chaining calls
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryLessThanOrEqualToChainCallsTest', 0, async function (done) {
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
     * @tc.nameQueryLessThanOrEqualToNanTest
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
        } catch (e) {
            expect(null).assertFail();
        }
        query = null;
        done();
    })

    /**
     * @tc.nameQueryLessThanOrEqualToInvalidMoreArgsTest
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
     * @tc.nameQueryIsNullSucTest
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
     * @tc.nameQueryIsNullChainCallsTest
     * @tc.desc: Test Js Api Query.IsNull() with chaining calls
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryIsNullChainCallsTest', 0, async function (done) {
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
     * @tc.nameQueryIsNullInvalidMoreArgsTest
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
     * @tc.nameQueryIsNullInvalidTypeArgsTest
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
     * @tc.nameQueryInNumberSucTest
     * @tc.desc: Test Js Api Query.InNumber() successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryInNumberSucTest', 0, async function (done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            console.info("QueryInNumberSucTest start ");
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
        } catch (e) {
            console.error("dumplicated calls should be ok : " + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        query = null;
        done();
    })

    /**
     * @tc.nameQueryInNumberChainCallsTest
     * @tc.desc: Test Js Api Query.InNumber() with chaining calls
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryInNumberChainCallsTest', 0, async function (done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.inNumber("key", [1, 2.3, 987654]).inNumber("key2", [0x10abcdef]).inNumber("key2", [0xf0123456]).inNumber("key2", [0b10101]);
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
     * @tc.nameQueryInNumberInvalidMoreArgsTest
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
     * @tc.nameQueryInNumberInvalidTypeArgsTest
     * @tc.desc: Test Js Api Query.InNumber() with invalid type args
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
        }
        query = null;
        done();
    })

    /**
     * @tc.nameQueryInNumberInvalidArrayArgsTest
     * @tc.desc: Test Js Api Query.InNumber() with invalid array args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryInNumberInvalidArrayArgsTest', 0, async function (done) {
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
     * @tc.nameQueryInNumberDumpCallsTest
     * @tc.desc: Test Js Api Query.InNumber() dumplicated calls
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryInNumberDumpCallsTest', 0, async function (done) {
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
     * @tc.nameQueryInStringSucTest
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
     * @tc.nameQueryInStringChainCallsTest
     * @tc.desc: Test Js Api Query.InString() with chaining calls
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryInStringChainCallsTest', 0, async function (done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            query.inString("key", ["a2z", 'z2a'])
                .inString("key2", ["AAA"])
                .inString("key2", ["AAA", "AAABBB", "CCCAAA"]);
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
     * @tc.nameQueryInStringInvalidMoreArgsTest
     * @tc.desc: Test Js Api Query.InString() with invalid more args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryInStringInvalidMoreArgsTest', 0, async function (done) {
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
     * @tc.nameQueryInStringInvalidTypeArgsTest
     * @tc.desc: Test Js Api Query.InString() with invalid type args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryInStringInvalidTypeArgsTest', 0, async function (done) {
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
     * @tc.nameQueryNotInNumberSucTest
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
     * @tc.nameQueryNotInNumberChainCallsTest
     * @tc.desc: Test Js Api Query.NotInNumber() with chaining calls
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryNotInNumberChainCallsTest', 0, async function (done) {
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
     * @tc.nameQueryNotInNumberInvalidMoreArgsTest
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
     * @tc.nameQueryNotInNumberInvalidTypeArgsTest
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
        }
        query = null;
        done();
    })

    /**
     * @tc.nameQueryNotInStringSucTest
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
     * @tc.nameQueryNotInStringChainCallsTest
     * @tc.desc: Test Js Api Query.NotInString() with chaining calls
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryNotInStringChainCallsTest', 0, async function (done) {
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
     * @tc.nameQueryNotInStringInvalidMoreArgsTest
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
     * @tc.nameQueryNotInStringInvalidTypeArgsTest
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
        }
        query = null;
        done();
    })

    /**
     * @tc.nameQueryLikeSucTest
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
     * @tc.nameQueryLikeChainCallsTest
     * @tc.desc: Test Js Api Query.Like() with chaining calls
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryLikeChainCallsTest', 0, async function (done) {
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
     * @tc.nameQueryLikeInvalidArgsTypeTest
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
        }
        query = null;
        done();
    })

    /**
     * @tc.nameQueryLikeInvalidMoreTypesTest
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
     * @tc.nameQueryUnlikeSucTest
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
     * @tc.nameQueryUnlikeChainCallsTest
     * @tc.desc: Test Js Api Query.Unlike() with chaining calls
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryUnlikeChainCallsTest', 0, async function (done) {
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
     * @tc.nameQueryUnlikeInvalidTypeArgsTest
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
        }
        query = null;
        done();
    })

    /**
     * @tc.nameQueryUnlikeInvalidMoreArgsTest
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
     * @tc.nameQueryAndSucTest
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
     * @tc.nameQueryAndEqualToTest
     * @tc.desc: Test Js Api Query.And() equalto
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryAndEqualToTest', 0, async function (done) {
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
     * @tc.nameQueryAndNotEqualToTest
     * @tc.desc: Test Js Api Query.And() notEqualTo
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryAndNotEqualToTest', 0, async function (done) {
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
     * @tc.nameQueryAndDiffTest
     * @tc.desc: Test Js Api Query.And() different calls
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryAndDiffTest', 0, async function (done) {
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
     * @tc.nameQueryOrSucTest
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
     * @tc.nameQueryOrEqualToTest
     * @tc.desc: Test Js Api Query.Or() equalto
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryOrEqualToTest', 0, async function (done) {
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
     * @tc.nameQueryOrNotEqualToTest
     * @tc.desc: Test Js Api Query.Or() notequalto
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryOrNotEqualToTest', 0, async function (done) {
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
     * @tc.nameQueryOrDiffTest
     * @tc.desc: Test Js Api Query.Or() with diff calls
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryOrDiffTest', 0, async function (done) {
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
     * @tc.nameQueryOrderByAscSucTest
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
     * @tc.nameQueryOrderByAscChainCallsTest
     * @tc.desc: Test Js Api Query.OrderByAsc() with chaining calls
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryOrderByAscChainCallsTest', 0, async function (done) {
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
     * @tc.nameQueryOrderByAscInvalidArgsTest
     * @tc.desc: Test Js Api Query.OrderByAsc() with invalid args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryOrderByAscInvalidArgsTest', 0, async function (done) {
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
     * @tc.nameQueryOrderByAscWithNullTest
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
        }
        query = null;
        done();
    })

    /**
     * @tc.nameQueryOrderByDescTest
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
     * @tc.nameQueryOrderByDescChainCallsTest
     * @tc.desc: Test Js Api Query.OrderByDesc() with chaining calls
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryOrderByDescChainCallsTest', 0, async function (done) {
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
     * @tc.nameQueryOrderByDescInvalidArgsTest
     * @tc.desc: Test Js Api Query.OrderByDesc() with invalid args
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryOrderByDescInvalidArgsTest', 0, async function (done) {
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
     * @tc.nameQueryOrderByDescNullTest
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
        }
        query = null;
        done();
    })

    /**
     * @tc.nameQueryLimitSucTest
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
     * @tc.nameQueryLimitChainCallsTest
     * @tc.desc: Test Js Api Query.Limit() with chaining calls
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryLimitChainCallsTest', 0, async function (done) {
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
     * @tc.nameQueryLimitMoreArgsTest
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
            console.info("query is " + query.getSqlLike());
        } catch (e) {
            console.error("throw exception: " + `, error code is ${e.code}, message is ${e.message}`);
            expect(null).assertFail();
        }
        query = null;
        done();
    })

    /**
     * @tc.nameQueryLimitLessArgsTest
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
        }
        query = null;
        done();
    })

    /**
     * @tc.nameQueryLimitInvalidTypeArgsTest
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
        }
        query = null;
        done();
    })

    /**
     * @tc.nameQueryIsNotNullSucTest
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
     * @tc.nameQueryIsNotNullChainCallsTest
     * @tc.desc: Test Js Api Query.IsNotNull() with chaining calls
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryIsNotNullChainCallsTest', 0, async function (done) {
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
     * @tc.nameQueryIsNotNullMoreArgsTest
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
     * @tc.nameQueryIsNotNullInvalidTypeArgsTest
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
        }
        query = null;
        done();
    })

    /**
     * @tc.nameQueryBeginGroupSucTest
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
     * @tc.nameQueryBeginGroupChainCallsTest
     * @tc.desc: Test Js Api Query.BeginGroup() with chaining calls
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryBeginGroupChainCallsTest', 0, async function (done) {
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
     * @tc.nameQueryBeginGroupInvalidTypeArgsTest
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
        }
        query = null;
        done();
    })

    /**
     * @tc.nameQueryBeginGroupInvalidMoreArgsTest
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
     * @tc.nameQueryEndGroupInvalidTypeArgsTest
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
        }
        query = null;
        done();
    })

    /**
     * @tc.nameQueryPrefixKeySucTest
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
     * @tc.nameQueryPrefixKeyChainCallsTest
     * @tc.desc: Test Js Api Query.PrefixKey() with chaining calls
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryPrefixKeyChainCallsTest', 0, async function (done) {
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
     * @tc.nameQueryPrefixKeyInvalidMoreArgsTest
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
     * @tc.nameQueryPrefixKeyInvalidTypeArgsTest
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
        }
        query = null;
        done();
    })

    /**
     * @tc.nameQuerySetSuggestIndexSucTest
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
     * @tc.nameQuerySetSuggestIndexChainCallsTest
     * @tc.desc: Test Js Api Query.SetSuggestIndex() with chaining calls
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QuerySetSuggestIndexChainCallsTest', 0, async function (done) {
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
     * @tc.nameQuerySetSuggestIndexInvalidMoreArgsTest
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
     * @tc.nameQuerySetSuggestIndexInvalidTypeArgsTest
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
     * @tc.nameQueryDeviceIdSucTest
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
     * @tc.nameQueryDeviceIdChainCallsTest
     * @tc.desc: Test Js Api Query.DeviceId() with chaining calls
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryDeviceIdChainCallsTest', 0, async function (done) {
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
     * @tc.nameQueryDeviceIdInvalidMoreArgsTest
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
     * @tc.nameQueryDeviceIdInvalidTypeArgsTest
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
        }
        query = null;
        done();
    })

    /**
     * @tc.nameQueryGetSqlLikeSucTest
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
     * @tc.nameQueryGetSqlLikeChainCallsTest
     * @tc.desc: Test Js Api Query.GetSqlLike() with chaining calls
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('QueryGetSqlLikeChainCallsTest', 0, async function (done) {
        var query = null;
        try {
            query = new ddm.Query();
            expect("").assertEqual(query.getSqlLike());
            let sql1 = query.getSqlLike();
            console.info("QueryGetSqlLikeChainCallsTest sql=" + sql1);
            query.inString("key1", ["AAA", "BBB"])
                .or()
                .notEqualTo("key2", 0);
            let sql2 = query.getSqlLike();
            console.info("QueryGetSqlLikeChainCallsTest sql=" + sql2);
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
     * @tc.nameQueryGetSqlLikeInvalidMoreArgsTest
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
