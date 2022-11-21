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

describe('fieldNodeTest', function() {

    /**
     * @tc.name FieldNodeAppendChildTest001
     * @tc.desc  Test Js Api FieldNode.AppendChild() testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('FieldNodeAppendChildTest001', 0, async function(done) {
        try {
            let node = new ddm.FieldNode("root");
            let child1 = new ddm.FieldNode("child1");
            let child2 = new ddm.FieldNode("child2");
            let child3 = new ddm.FieldNode("child3");
            node.appendChild(child1);
            node.appendChild(child2);
            node.appendChild(child3);
            child1 = null;
            child2 = null;
            child3 = null;
            node = null;
        } catch (e) {
            console.info("FieldNodeAppendChildTest001 " + e);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name FieldNodeAppendChildTest002
     * @tc.desc  Test Js Api FieldNode.AppendChild() testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('FieldNodeAppendChildTest002', 0, async function(done) {
        try {
            let node = new ddm.FieldNode("root");
            let child = new ddm.FieldNode("child");
            node.appendChild(child);
            child = null;
            node = null;
        } catch (e) {
            console.info("FieldNodeAppendChildTest002 " + e);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name FieldNodeAppendChildTest003
     * @tc.desc  Test Js Api FieldNode.AppendChild() testcase 003
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('FieldNodeAppendChildTest003', 0, async function(done) {
        try {
            let node = new ddm.FieldNode("root");
            let child = new ddm.FieldNode();
            node.appendChild(child);
            expect(null).assertFail();
        } catch (e) {
            console.info("FieldNodeAppendChildTest003 failed on exception :" + e);
        }
        done();
    })

    /**
     * @tc.name FieldNodeToJsonTest001
     * @tc.desc  Test Js Api FieldNode.ToJson() testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('FieldNodeToJsonTest001', 0, async function(done) {
        try {
            let node = new ddm.FieldNode("root");
            let child = new ddm.FieldNode("child");
            node.appendChild(child);
        } catch (e) {
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name FieldNodeToJsonTest002
     * @tc.desc  Test Js Api FieldNode.ToJson() testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('FieldNodeToJsonTest002', 0, async function(done) {
        try {
            let node = new ddm.FieldNode("root");
            let child = new ddm.FieldNode("child");
            node.appendChild(child);
        } catch (e) {
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name FieldNodeToJsonTest003
     * @tc.desc  Test Js Api FieldNode.ToJson() testcase 003
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('FieldNodeToJsonTest003', 0, async function(done) {
        try {
            let node = new ddm.FieldNode();
            let child = new ddm.FieldNode();
            node.appendChild(child);
            expect(null).assertFail();
        } catch (e) {
            console.info("FieldNodeToJsonTest003 failed on exception : " + e);
        }
        done();
    })

    /**
     * @tc.name FieldNodedefaultTest001
     * @tc.desc  Test Js Api FieldNode.default testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
     it('FieldNodedefaultTest001', 0, async function(done) {
        try {
            let node = new ddm.FieldNode('first');
            node.default = 'first name';
            console.info('defaultValue = ' + node.default);
            expect(node.default === 'first name').assertTrue()
        } catch (e) {
            console.info("FieldNodedefaultTest001 fail on exception: " + e);
        }
        done();
    })

    /**
     * @tc.name FieldNodenullableTest001
     * @tc.desc  Test Js Api FieldNode.nullable testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
     it('FieldNodenullableTest001', 0, async function(done) {
        try {
            let node = new ddm.FieldNode('first');
            node.nullable = false;
            console.info('nullable = ' + node.nullable);
            expect(node.nullable === false).assertTrue()
        } catch (e) {
            console.info("FieldNodenullableTest001 fail on exception: " + e);
        }
        done();
    })

    /**
     * @tc.name FieldNodetypeTest001
     * @tc.desc  Test Js Api FieldNode.type testcase 001
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
     it('FieldNodetypeTest001', 0, async function(done) {
        try {
            let node = new ddm.FieldNode('first');
            node.type = ddm.value.type.STRING;
            console.info('type = ' + node.type);
            expect(node.type === ddm.type.STRING).assertTrue()
        } catch (e) {
            console.info("FieldNodetypeTest001 fail on exception: " + e);
        }
        done();
    })

    /**
     * @tc.name FieldNodetypeTest002
     * @tc.desc  Test Js Api FieldNode.type testcase 002
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
     it('FieldNodetypeTest002', 0, async function(done) {
        try {
            let node = new ddm.FieldNode('first');
            node.type = ddm.type.INTEGER;
            console.info('type = ' + node.type);
            expect(node.type === ddm.type.INTEGER).assertTrue()
        } catch (e) {
            console.info("FieldNodetypeTest002 fail on exception: " + e);
        }
        done();
    })

    /**
     * @tc.name FieldNodetypeTest003
     * @tc.desc  Test Js Api FieldNode.type testcase 003
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
     it('FieldNodetypeTest003', 0, async function(done) {
        try {
            let node = new ddm.FieldNode('first');
            node.type = ddm.type.FLOAT;
            console.info('type = ' + node.type);
            expect(node.type === ddm.type.FLOAT).assertTrue()
        } catch (e) {
            console.info("FieldNodetypeTest003 fail on exception: " + e);
        }
        done();
    })

    /**
     * @tc.name FieldNodetypeTest004
     * @tc.desc  Test Js Api FieldNode.type testcase 004
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
     it('FieldNodetypeTest004', 0, async function(done) {
        try {
            let node = new ddm.FieldNode('first');
            node.type = ddm.type.BYTE_ARRAY;
            console.info('type = ' + node.type);
            expect(node.type === ddm.type.BYTE_ARRAY).assertTrue()
        } catch (e) {
            console.info("FieldNodetypeTest004 fail on exception: " + e);
        }
        done();
    })

    /**
     * @tc.name FieldNodetypeTest005
     * @tc.desc  Test Js Api FieldNode.type testcase 005
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
     it('FieldNodetypeTest005', 0, async function(done) {
        try {
            let node = new ddm.FieldNode('first');
            node.type = ddm.type.BOOLEAN;
            console.info('type = ' + node.type);
            expect(node.type === ddm.ValueType.BOOLEAN).assertTrue()
        } catch (e) {
            console.info("FieldNodetypeTest005 fail on exception: " + e);
        }
        done();
    })

    /**
     * @tc.name FieldNodetypeTest006
     * @tc.desc  Test Js Api FieldNode.type testcase 006
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
     it('FieldNodetypeTest006', 0, async function(done) {
        try {
            let node = new ddm.FieldNode('first');
            node.type = ddm.type.DOUBLE;
            console.info('type = ' + node.type);
            expect(node.type === ddm.type.DOUBLE).assertTrue()
        } catch (e) {
            console.info("FieldNodetypeTest006 fail on exception: " + e);
        }
        done();
    })
})
