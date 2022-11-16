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

describe('FieldNodeTest', function () {
    /**
     * @tc.name FieldNodeConstructorInvalidArgsTest
     * @tc.desc  Test Js Api FieldNode.AppendChild() with invalid child node
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('FieldNodeConstructorInvalidArgsTest', 0, async function (done) {
        try {
            let node = new ddm.FieldNode();
            expect(null).assertFail();
        } catch (e) {
            console.info("FieldNodeConstructorInvalidArgsTest throws exception successfully :" + e);
            expect(e.code == 401).assertTrue();
        }
        done();
    })

    /**
     * @tc.name FieldNodeAppendChildSucTest
     * @tc.desc  Test Js Api FieldNode.AppendChild() successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('FieldNodeAppendChildSucTest', 0, async function (done) {
        try {
            let node = new ddm.FieldNode("root");
            let child = new ddm.FieldNode("child");
            node.appendChild(child);
            expect(true).assertTrue();
            child = null;
            node = null;
        } catch (e) {
            console.info("FieldNodeAppendChildSucTest " + e);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name FieldNodeAppendChildInvalidChildTest
     * @tc.desc  Test Js Api FieldNode.AppendChild() with invalid child node
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('FieldNodeAppendChildInvalidChildTest', 0, async function (done) {
        try {
            let node = new ddm.FieldNode("root");
            node.appendChild(null);
            expect(null).assertFail();
        } catch (e) {
            console.info("FieldNodeAppendChildInvalidChildTest throws exception successfully :" + e);
            expect(e.code == 401).assertTrue();
        }
        done();
    })

    /**
     * @tc.name FieldNodedefaultSucTest
     * @tc.desc  Test Js Api FieldNode.default successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('FieldNodedefaultSucTest', 0, async function (done) {
        try {
            let node = new ddm.FieldNode("first");
            node.default = 'first name';
            console.info('defaultValue = ' + node.default);
            expect(node.default === 'first name').assertTrue()
        } catch (e) {
            console.info("FieldNodedefaultSucTest fail on exception: " + e);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name FieldNodenullableSucTest
     * @tc.desc  Test Js Api FieldNode.nullable successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('FieldNodenullableSucTest', 0, async function (done) {
        try {
            let node = new ddm.FieldNode("first");
            node.nullable = false;
            console.info('nullable = ' + node.nullable);
            expect(node.nullable === false).assertTrue()
        } catch (e) {
            console.info("FieldNodenullableSucTest fail on exception: " + e);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name FieldNodetypeStringTest
     * @tc.desc  Test Js Api FieldNode.type String
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('FieldNodetypeStringTest', 0, async function (done) {
        try {
            let node = new ddm.FieldNode("first");
            node.type = ddm.ValueType.STRING;
            console.info('type = ' + node.type);
            expect(node.type === ddm.ValueType.STRING).assertTrue()
        } catch (e) {
            console.info("FieldNodetypeStringTest fail on exception: " + e);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name FieldNodetypeIntegerTest
     * @tc.desc  Test Js Api FieldNode.type integer
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('FieldNodetypeIntegerTest', 0, async function (done) {
        try {
            let node = new ddm.FieldNode("first");
            console.info("success 1");
            node.type = ddm.ValueType.INTEGER;
            console.info('type = ' + node.type);
            expect(node.type === ddm.ValueType.INTEGER).assertTrue()
        } catch (e) {
            console.info("FieldNodetypeIntegerTest fail on exception: " + e);
            expect(null).assertFail;
        }
        done();
    })

    /**
     * @tc.name FieldNodetypeFloatTest
     * @tc.desc  Test Js Api FieldNode.type float
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('FieldNodetypeFloatTest', 0, async function (done) {
        try {
            let node = new ddm.FieldNode("first");
            node.type = ddm.ValueType.FLOAT;
            console.info('type = ' + node.type);
            expect(node.type === ddm.ValueType.FLOAT).assertTrue()
        } catch (e) {
            console.info("FieldNodetypeFloatTest fail on exception: " + e);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name FieldNodetypeByteArrayTest
     * @tc.desc  Test Js Api FieldNode.type byte array
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('FieldNodetypeByteArrayTest', 0, async function (done) {
        try {
            let node = new ddm.FieldNode("first");
            node.type = ddm.ValueType.BYTE_ARRAY;
            console.info('type = ' + node.type);
            expect(node.type === ddm.ValueType.BYTE_ARRAY).assertTrue()
        } catch (e) {
            console.info("FieldNodetypeByteArrayTest fail on exception: " + e);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name FieldNodetypeBooleanTest
     * @tc.desc  Test Js Api FieldNode.type boolean
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('FieldNodetypeBooleanTest', 0, async function (done) {
        try {
            let node = new ddm.FieldNode("first");
            node.type = ddm.ValueType.BOOLEAN;
            console.info('type = ' + node.type);
            expect(node.type === ddm.ValueType.BOOLEAN).assertTrue()
        } catch (e) {
            console.info("FieldNodetypeBooleanTest fail on exception: " + e);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name FieldNodetypeDoubleTest
     * @tc.desc  Test Js Api FieldNode.type double
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('FieldNodetypeDoubleTest', 0, async function (done) {
        try {
            let node = new ddm.FieldNode("first");
            node.type = ddm.ValueType.DOUBLE;
            console.info('type = ' + node.type);
            expect(node.type === ddm.ValueType.DOUBLE).assertTrue()
        } catch (e) {
            console.info("FieldNodetypeDoubleTest fail on exception: " + e);
            expect(null).assertFail();
        }
        done();
    })
})
