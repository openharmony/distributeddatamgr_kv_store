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

import { describe, beforeAll, beforeEach, afterEach, afterAll, it, expect } from '@ohos/hypium'
import distributedKVStore from '@ohos.data.distributedKVStore'

const TAG = '[SingleKvStoreStageTest]'
const KEY_TEST_STRING_ELEMENT = 'key_test_string';
const VALUE_TEST_STRING_ELEMENT = 'value-string-001';
const TEST_BUNDLE_NAME = 'com.example.myapplication';
const TEST_STORE_ID = 'storeId1';
let kvManager = null;
let kvStore = null;

export default function SingleKvStoreKVPromiseTest() {
  describe('SingleKvStoreKVPromiseTest', function () {
    beforeAll(async () => {
      console.info(TAG + ' beforeAll');
      const config = {
        bundleName: TEST_BUNDLE_NAME,
        context: globalThis.abilityContext
      };
      kvManager = distributedKVStore.createKVManager(config);
    });

    afterAll(async () => {
      console.info(TAG + ' afterAll');
      kvManager = null;
      kvStore = null;
    });

    beforeEach(async (done) => {
      console.info(TAG + ' beforeEach');
      const options = {
        createIfMissing: true,
        encrypt: false,
        backup: true,
        autoSync: true,
        kvStoreType: distributedKVStore.KVStoreType.SINGLE_VERSION,
        schema: '',
        securityLevel: distributedKVStore.SecurityLevel.S2,
      };
      await kvManager.getKVStore(TEST_STORE_ID, options, (err, store) => {
        kvStore = store;
        done();
      });
    });

    afterEach(async (done) => {
      console.info(TAG + ' afterEach');
      try {
        await kvManager.closeKVStore(TEST_BUNDLE_NAME, TEST_STORE_ID, async (err, data) => {
          await kvManager.deleteKVStore(TEST_BUNDLE_NAME, TEST_STORE_ID, (err, data) => {
            done();
          });
        });
        kvStore = null;
      } catch (e) {
        console.error(TAG + ' afterEach error: ' + e);
        done();
      }
    });

    /**
     * @tc.name SingleKvStorePutStringPromiseInvalidArgsTest
     * @tc.desc Test Js Api SingleKvStore.Put(String) with invalid args
     * @tc.type: FUNC
     */
    it('SingleKvStorePutStringPromiseInvalidArgsTest', 0, async (done) => {
      try {
        await kvStore.put(KEY_TEST_STRING_ELEMENT, null).then((data) => {
          expect(null).assertFail();
        }).catch((error) => {
          expect(null).assertFail();
        });
      } catch (e) {
        expect(e.code == 401).assertTrue();
      }
      done();
    });

    /**
     * @tc.name SingleKvStorePutStringPromiseClosedKVStoreTest
     * @tc.desc Test Js Api SingleKvStore.Put(String) in a closed kvstore
     * @tc.type: FUNC
     */
    it('SingleKvStorePutStringPromiseClosedKVStoreTest', 0, async (done) => {
      try {
        await kvManager.closeKVStore(TEST_BUNDLE_NAME, TEST_STORE_ID).then(async () => {
          await kvManager.deleteKVStore(TEST_BUNDLE_NAME, TEST_STORE_ID);
        });
        await kvStore.put(KEY_TEST_STRING_ELEMENT, VALUE_TEST_STRING_ELEMENT).then((data) => {
          expect(null).assertFail();
        }).catch((error) => {
          expect(error.code == 15100005).assertTrue();
        });
      } catch (e) {
        expect(null).assertFail();
      }
      done();
    });

    /**
     * @tc.name SingleKvStorePutStringPromiseSucTest
     * @tc.desc Test Js Api SingleKvStore.Put(String) success
     * @tc.type: FUNC
     */
    it('SingleKvStorePutStringPromiseSucTest', 0, async (done) => {
      try {
        await kvStore.put(KEY_TEST_STRING_ELEMENT, VALUE_TEST_STRING_ELEMENT).then((data) => {
          expect(data == undefined).assertTrue();
        }).catch((error) => {
          expect(null).assertFail();
        });
      } catch (e) {
        expect(null).assertFail();
      }
      done();
    });
  });
} 