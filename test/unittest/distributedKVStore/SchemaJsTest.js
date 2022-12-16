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
import abilityFeatureAbility from '@ohos.ability.featureAbility'

var context = abilityFeatureAbility.getContext();
const TEST_BUNDLE_NAME = 'com.example.myapplication';
const TEST_STORE_ID = 'storeId';

var kvManager = null;
var kvStore = null;

describe('schemaTest', function () {
    const config = {
        bundleName: TEST_BUNDLE_NAME,
        context: context,
    }

    var options = {
        createIfMissing: true,
        encrypt: false,
        backup: false,
        autoSync: true,
        kvStoreType: ddm.KVStoreType.SINGLE_VERSION,
        schema: {},
        securityLevel: ddm.SecurityLevel.S1,
    }

    beforeAll(async function (done) {
        try {
            kvManager =  ddm.createKVManager(config);
            console.info("beforeAll: createKVManager (single) with " + JSON.stringify(options));
        } catch (e) {
            console.info("fail on exception: " + e);
            expect(null).assertFail();
        }
        done();
    })

    afterAll(async function (done) {
        console.info('afterAll');
        kvManager = null;
        kvStore = null;
        done();
    })

    beforeEach(async function (done) {
        console.info('beforeEach testcase will update options:' + JSON.stringify(options));
        done();
    })

    afterEach(async function (done) {
        console.info('afterEach');
        await kvManager.closeKVStore(TEST_BUNDLE_NAME, TEST_STORE_ID, kvStore).then(async () => {
            console.info('afterEach closeKVStore success');
            await kvManager.deleteKVStore(TEST_BUNDLE_NAME, TEST_STORE_ID).then(() => {
                console.info('afterEach deleteKVStore success');
            }).catch((err) => {
                console.info('afterEach deleteKVStore err ' + err);
            });
        }).catch((err) => {
            console.info('afterEach closeKVStore err ' + err);
        });
        kvStore = null;
        done();
    })

    /**
     * @tc.name SchemaToJsonStringSucTest
     * @tc.desc  Test Js Api Schema.ToJsonString() successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SchemaToJsonStringSucTest', 0, async function (done) {
        try {
            let name = new ddm.FieldNode('name');
            name.type = ddm.ValueType.INTEGER;
            name.nullable = false;
            name.default = 0;

            let schema = new ddm.Schema();
            schema.root.appendChild(name);
            schema.indexes = ['$.name'];
            schema.mode = 1; // STRICT
            schema.skip = 0;
            options.kvStoreType = ddm.KVStoreType.SINGLE_VERSION;
            options.schema = schema;
            await kvManager.getKVStore(TEST_STORE_ID, options).then(async (store) => {
                console.info('SchemaToJsonStringSucTest getKVStore success' + JSON.stringify(options));
                kvStore = store;
                expect(store != null).assertTrue();
                await kvStore.put("test_key_1", '{"name":1}');
                await kvStore.put("test_key_2", '{"name":2}');
                await kvStore.put("test_key_3", '{"name":3}');
                console.info('SchemaToJsonStringSucTest Put success');
            });
            console.info('SchemaToJsonStringSucTest start Query ...');
            var query = new ddm.Query();
            query.prefixKey('test_key_');
            query.notEqualTo("$.name", 3);
            await kvStore.getEntries(query).then((entries) => {
                console.info('SchemaToJsonStringSucTest get success : ' + JSON.stringify(entries));
                expect(entries.length == 2).assertTrue();
            }).catch((err) => {
                console.info('SchemaToJsonStringSucTest get fail ' + err);
                expect(null).assertFail();
            });
        } catch (e) {
            console.info("SchemaToJsonStringSucTest fail on exception: " + e);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SchemaRootTest
     * @tc.desc  Test Js Api Schema.root successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SchemaRootTest', 0, async function (done) {
        try {
            let english = new ddm.FieldNode('english');
            english.type = ddm.ValueType.STRING;

            let schema = new ddm.Schema();
            expect(schema.root instanceof ddm.FieldNode).assertTrue();
        } catch (e) {
            console.info("schema fail on exception: " + e);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SchemaIndexesSucTest
     * @tc.desc  Test Js Api Schema.indexes successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SchemaIndexesSucTest', 0, async function (done) {
        try {

            let schema = new ddm.Schema();
            schema.indexes = ['$.english.first', '$.english.second'];
            expect(schema.indexes[0] === '$.english.first' && schema.indexes[1] === '$.english.second').assertTrue();
        } catch (e) {
            console.info("schema fail on exception: " + e);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SchemaMode1Test
     * @tc.desc  Test Js Api Schema.mode with mode 1
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SchemaMode1Test', 0, async function (done) {
        try {

            let schema = new ddm.Schema();
            schema.mode = 1;
            console.info("schema mode = " + schema.mode)
            expect(schema.mode === 1).assertTrue();
        } catch (e) {
            console.info("schema fail on exception: " + e);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SchemaMode0Test
     * @tc.desc  Test Js Api Schema.mode with mode 0
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SchemaMode0Test', 0, async function (done) {
        try {

            let schema = new ddm.Schema();
            schema.mode = 0;
            console.info("schema mode = " + schema.mode)
            expect(schema.mode === 0).assertTrue();
        } catch (e) {
            console.info("schema fail on exception: " + e);
            expect(null).assertFail();
        }
        done();
    })

    /**
     * @tc.name SchemaSkipSucTest
     * @tc.desc  Test Js Api Schema.skip successfully
     * @tc.type: FUNC
     * @tc.require: issueNumber
     */
    it('SchemaSkipSucTest', 0, async function (done) {
        try {

            let schema = new ddm.Schema();
            schema.skip = 0;
            expect(schema.skip === 0).assertTrue();
        } catch (e) {
            console.info("schema fail on exception: " + e);
            expect(null).assertFail();
        }
        done();
    })
})
