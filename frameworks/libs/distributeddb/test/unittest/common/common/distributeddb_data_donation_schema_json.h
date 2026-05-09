/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef DISTRIBUTEDDB_DATA_DONATION_SCHEMA_JSON_H
#define DISTRIBUTEDDB_DATA_DONATION_SCHEMA_JSON_H

namespace DistributedDBUnitTest {
class DataDonationSchemaJsonTest {
public:
    static constexpr const char *DATA_DONATION_SCHEMA_JSON = \
        "{\"dbSchema\":[{\"tables\":[{\"tableName\":\"TableA\",\"fields\":[{" \
        "\"columnName\":\"filter\",\"type\":\"Integer\",\"notNull\":false},{" \
        "\"columnName\":\"KeyId\",\"type\":\"Integer\",\"primaryKey\":true," \
        "\"autoIncrement\":true,\"notNull\":false,\"foreignKey\":[{" \
        "\"tableName\":\"TableB\",\"columnName\":\"KeyId\"}]},{\"columnName\":\"title\"," \
        "\"type\":\"Text\",\"notNull\":false},{\"columnName\":\"F1\"," \
        "\"type\":\"Integer\",\"notNull\":false},{\"columnName\":\"F2\"," \
        "\"type\":\"Integer\",\"notNull\":false},{\"columnName\":\"F3\"," \
        "\"type\":\"Integer\",\"notNull\":false},{\"columnName\":\"F4\"," \
        "\"type\":\"Text\",\"notNull\":false},{\"columnName\":\"F5\",\"type\":\"Text\"," \
        "\"notNull\":false},{\"columnName\":\"F6\",\"type\":\"Text\",\"notNull\":false},{" \
        "\"columnName\":\"F7\",\"type\":\"Integer\",\"notNull\":false},{" \
        "\"columnName\":\"F8\",\"type\":\"Integer\",\"notNull\":false}],\"indexes\":[]},{" \
        "\"tableName\":\"TableC\",\"fields\":[{\"columnName\":\"KeyId\"," \
        "\"type\":\"Integer\",\"primaryKey\":true,\"notNull\":false,\"foreignKey\":[{" \
        "\"tableName\":\"TableA\",\"columnName\":\"KeyId\"}]},{\"columnName\":\"F9\"," \
        "\"type\":\"Text\",\"notNull\":false}],\"indexes\":[]},{\"tableName\":\"TableB\"," \
        "\"fields\":[{\"columnName\":\"KeyId\",\"type\":\"Integer\",\"primaryKey\":true," \
        "\"notNull\":false,\"foreignKey\":[{\"tableName\":\"TableA\"," \
        "\"columnName\":\"KeyId\"}]},{\"columnName\":\"F9\",\"type\":\"Text\"," \
        "\"notNull\":false}],\"indexes\":[]},{\"tableName\":\"TableD\",\"fields\":[{" \
        "\"columnName\":\"KeyId\",\"type\":\"Integer\",\"primaryKey\":true," \
        "\"notNull\":false,\"foreignKey\":[{\"tableName\":\"TableA\"," \
        "\"columnName\":\"KeyId\"}]}],\"indexes\":[]},{\"tableName\":\"TableE\"," \
        "\"fields\":[{\"columnName\":\"F44\",\"type\":\"Integer\",\"primaryKey\":true," \
        "\"notNull\":true,\"foreignKey\":[{\"tableName\":\"TableA\"," \
        "\"columnName\":\"KeyId\"}]}],\"indexes\":[]}]}],\"searchConfig\":{" \
        "\"UTDMapping\":[{\"dbName\":[\"test_library\"],\"tables\":[\"TableA\"]," \
        "\"parts\":[{\"tables\":[\"TableA\",\"TableB\",\"TableD\",\"TableC\"]," \
        "\"mappings\":[{\"name\":\"map54\",\"value\":{\"tableName\":\"TableE\"," \
        "\"columnName\":\"F44\"},\"where\":[{\"equalto\":{\"tableName\":\"TableA\"," \
        "\"columnName\":\"filter\",\"value\":1}}]},{\"name\":\"map3\"," \
        "\"primaryKey\":true,\"value\":{\"tableName\":\"TableA\"," \
        "\"columnName\":\"KeyId\"}},{\"name\":\"map4\",\"value\":{" \
        "\"tableName\":\"TableA\",\"columnName\":\"F1\"}},{\"name\":\"map5\",\"value\":{" \
        "\"tableName\":\"TableA\",\"columnName\":\"F2\"}},{\"name\":\"map6\"," \
        "\"function\":{\"lib\":\"/system/lib64/libtest_function.so\"," \
        "\"name\":\"fun_test1\",\"argList\":[{\"tableName\":\"TableA\"," \
        "\"columnName\":\"F3\"}],\"return_type\":\"Text\"}},{\"name\":\"map7\"," \
        "\"value\":{\"tableName\":\"TableA\",\"columnName\":\"F4\"}},{\"name\":\"map8\"," \
        "\"value\":{\"tableName\":\"TableA\",\"columnName\":\"F5\"}},{\"name\":\"map9\"," \
        "\"value\":{\"tableName\":\"TableA\",\"columnName\":\"F6\"}},{\"name\":\"map10\"," \
        "\"function\":{\"lib\":\"/system/lib64/libtest_function.so\"," \
        "\"name\":\"fun_test2\",\"argList\":[{\"tableName\":\"TableA\"," \
        "\"columnName\":\"F11\"},{\"tableName\":\"TableA\",\"columnName\":\"F12\"},{" \
        "\"tableName\":\"TableA\",\"columnName\":\"F8\"}],\"return_type\":\"Text\"}},{" \
        "\"name\":\"map11\",\"value\":{\"tableName\":\"TableA\",\"columnName\":\"F13\"}}," \
        "{\"name\":\"map12\",\"value\":{\"tableName\":\"TableA\"," \
        "\"columnName\":\"F14\"}},{\"name\":\"map13\",\"value\":{" \
        "\"tableName\":\"TableA\",\"columnName\":\"F15\"}},{\"name\":\"map14\"," \
        "\"value\":{\"tableName\":\"TableA\",\"columnName\":\"filter\"}},{" \
        "\"name\":\"map15\",\"value\":[{\"key\":\"KeyId\",\"tableName\":\"TableA\"," \
        "\"columnName\":\"KeyId\"},{\"key\":\"F16\",\"tableName\":\"TableA\"," \
        "\"columnName\":\"F16\"},{\"key\":\"F1\",\"tableName\":\"TableA\"," \
        "\"columnName\":\"F1\"},{\"key\":\"F17\",\"tableName\":\"TableA\"," \
        "\"columnName\":\"F17\"}]},{\"name\":\"map16\",\"values\":[{\"key\":\"KeyId\"," \
        "\"tableName\":\"TableA\",\"columnName\":\"KeyId\"},{\"key\":\"F16\"," \
        "\"tableName\":\"TableA\",\"columnName\":\"F16\"},{\"key\":\"F1\"," \
        "\"tableName\":\"TableA\",\"columnName\":\"F1\"}]},{\"name\":\"map17\"," \
        "\"value\":{\"tableName\":\"TableA\",\"columnName\":\"F53\"}},{" \
        "\"name\":\"map18\",\"value\":{\"tableName\":\"TableA\",\"columnName\":\"F54\"}}," \
        "{\"name\":\"map19\",\"value\":{\"tableName\":\"TableA\"," \
        "\"columnName\":\"F55\"}},{\"name\":\"map20\",\"value\":{" \
        "\"tableName\":\"TableA\",\"columnName\":\"F8\"}},{\"name\":\"map21\",\"where\":[" \
        "{\"equalto\":{\"tableName\":\"TableA\",\"columnName\":\"filter\"," \
        "\"value\":1}}]},{\"name\":\"map22\",\"where\":[{\"equalto\":{" \
        "\"tableName\":\"TableA\",\"columnName\":\"filter\",\"value\":2}}]},{" \
        "\"name\":\"map23\",\"value\":{\"tableName\":\"TableA\",\"columnName\":\"F56\"}}," \
        "{\"name\":\"map24\",\"value\":{\"tableName\":\"TableA\"," \
        "\"columnName\":\"F57\"}},{\"name\":\"map25\",\"value\":{" \
        "\"tableName\":\"TableA\",\"columnName\":\"F17\"}},{\"name\":\"map26\"," \
        "\"value\":{\"tableName\":\"TableA\",\"columnName\":\"F18\"}},{" \
        "\"name\":\"map27\",\"function\":{\"lib\":\"/system/lib64/libtest_function.so\"," \
        "\"name\":\"fun_test3\",\"argList\":[{\"tableName\":\"TableA\"," \
        "\"columnName\":\"F10\"}],\"return_type\":\"Text\",\"returnList\":[{" \
        "\"name\":\"tagFirstType\",\"type\":\"Text\"},{\"name\":\"tagSecondType\"," \
        "\"type\":\"Text\"}]}},{\"name\":\"map28\",\"function\":{" \
        "\"lib\":\"/system/lib64/libtest_function.so\",\"name\":\"fun_test4\"," \
        "\"argList\":[{\"tableName\":\"TableA\",\"columnName\":\"F10\"}]," \
        "\"return_type\":\"Text\"}},{\"name\":\"map29\",\"function\":{" \
        "\"lib\":\"/system/lib64/libtest_function.so\",\"name\":\"fun_test5\"," \
        "\"argList\":[{\"tableName\":\"TableA\",\"columnName\":\"F10\"}]," \
        "\"return_type\":\"Text\"}},{\"name\":\"map30\",\"value\":{" \
        "\"tableName\":\"TableA\",\"columnName\":\"F19\"}},{\"name\":\"map31\"," \
        "\"function\":{\"lib\":\"/system/lib64/libtest_function.so\"," \
        "\"name\":\"fun_test6\",\"argList\":[{\"tableName\":\"TableA\"," \
        "\"columnName\":\"F20\"},{\"tableName\":\"TableA\",\"columnName\":\"F21\"},{" \
        "\"tableName\":\"TableA\",\"columnName\":\"F22\"}],\"return_type\":\"Text\"}},{" \
        "\"name\":\"map32\",\"primaryKey\":true,\"value\":{\"tableName\":\"TableB\"," \
        "\"columnName\":\"id\"},\"where\":[{\"equalto\":{\"tableName\":\"TableA\"," \
        "\"columnName\":\"filter\",\"value\":1}}]},{\"name\":\"map33\",\"value\":{" \
        "\"tableName\":\"TableD\",\"columnName\":\"F23\"}},{\"name\":\"map34\"," \
        "\"value\":{\"tableName\":\"TableD\",\"columnName\":\"F24\"}},{" \
        "\"name\":\"map35\",\"value\":{\"tableName\":\"TableD\",\"columnName\":\"F25\"}}," \
        "{\"name\":\"map36\",\"value\":{\"tableName\":\"TableD\"," \
        "\"columnName\":\"F26\"}},{\"name\":\"map37\",\"value\":{" \
        "\"tableName\":\"TableD\",\"columnName\":\"F27\"}},{\"name\":\"map38\"," \
        "\"value\":{\"tableName\":\"TableD\",\"columnName\":\"F28\"}},{" \
        "\"name\":\"map39\",\"value\":{\"tableName\":\"TableD\",\"columnName\":\"F29\"}}," \
        "{\"name\":\"map40\",\"value\":{\"tableName\":\"TableD\"," \
        "\"columnName\":\"F30\"}},{\"name\":\"map41\",\"function\":{" \
        "\"lib\":\"/system/lib64/libtest_function.so\",\"name\":\"fun_test7\"," \
        "\"argList\":[{\"tableName\":\"TableC\",\"columnName\":\"F9\"},{" \
        "\"tableName\":\"TableA\",\"columnName\":\"filter\"},{\"tableName\":\"TableB\"," \
        "\"columnName\":\"F9\"}],\"return_type\":\"Text\"},\"where\":[{\"equalto\":{" \
        "\"tableName\":\"TableA\",\"columnName\":\"filter\",\"value\":1}}]},{" \
        "\"name\":\"map42\",\"function\":{\"lib\":\"/system/lib64/libtest_function.so\"," \
        "\"name\":\"fun_test8\",\"argList\":[{\"tableName\":\"TableB\"," \
        "\"columnName\":\"F31\"},{\"tableName\":\"TableA\",\"columnName\":\"filter\"}]," \
        "\"return_type\":\"Text\"},\"where\":[{\"equalto\":{\"tableName\":\"TableA\"," \
        "\"columnName\":\"filter\",\"value\":2}}]},{\"name\":\"map43\",\"function\":{" \
        "\"lib\":\"/system/lib64/libtest_function.so\",\"name\":\"fun_test9\"," \
        "\"argList\":[{\"tableName\":\"TableC\",\"columnName\":\"F32\"},{" \
        "\"tableName\":\"TableA\",\"columnName\":\"filter\"},{\"tableName\":\"TableB\"," \
        "\"columnName\":\"F33\"}],\"return_type\":\"Text\"},\"where\":[{\"equalto\":{" \
        "\"tableName\":\"TableA\",\"columnName\":\"filter\",\"value\":1}}]},{" \
        "\"name\":\"map44\",\"function\":{\"lib\":\"/system/lib64/libtest_function.so\"," \
        "\"name\":\"fun_test10\",\"argList\":[{\"tableName\":\"TableC\"," \
        "\"columnName\":\"F34\"},{\"tableName\":\"TableB\",\"columnName\":\"F35\"},{" \
        "\"tableName\":\"TableA\",\"columnName\":\"filter\"}],\"return_type\":\"Text\"}}," \
        "{\"name\":\"map45\",\"function\":{\"lib\":\"/system/lib64/libtest_function.so\"," \
        "\"name\":\"fun_test11\",\"argList\":[{\"tableName\":\"TableC\"," \
        "\"columnName\":\"F36\"},{\"tableName\":\"TableB\",\"columnName\":\"F37\"},{" \
        "\"tableName\":\"TableA\",\"columnName\":\"filter\"}],\"return_type\":\"Text\"}}," \
        "{\"name\":\"map46\",\"function\":{\"lib\":\"/system/lib64/libtest_function.so\"," \
        "\"name\":\"fun_test12\",\"argList\":[{\"tableName\":\"TableC\"," \
        "\"columnName\":\"F38\"},{\"tableName\":\"TableB\",\"columnName\":\"F39\"},{" \
        "\"tableName\":\"TableA\",\"columnName\":\"filter\"}],\"return_type\":\"Text\"}}," \
        "{\"name\":\"map47\",\"function\":{\"lib\":\"/system/lib64/libtest_function.so\"," \
        "\"name\":\"fun_test13\",\"argList\":[{\"tableName\":\"TableC\"," \
        "\"columnName\":\"F9\"},{\"tableName\":\"TableA\",\"columnName\":\"filter\"}]," \
        "\"return_type\":\"Text\"}},{\"name\":\"map48\",\"function\":{" \
        "\"lib\":\"/system/lib64/libtest_function.so\",\"name\":\"fun_test14\"," \
        "\"argList\":[{\"tableName\":\"TableB\",\"columnName\":\"F31\"},{" \
        "\"tableName\":\"TableA\",\"columnName\":\"filter\"}],\"return_type\":\"Text\"}}," \
        "{\"name\":\"map49\",\"function\":{\"lib\":\"/system/lib64/libtest_function.so\"," \
        "\"name\":\"fun_test15\",\"argList\":[{\"tableName\":\"TableC\"," \
        "\"columnName\":\"F32\"},{\"tableName\":\"TableA\",\"columnName\":\"filter\"},{" \
        "\"tableName\":\"TableB\",\"columnName\":\"F33\"}],\"return_type\":\"Text\"}},{" \
        "\"name\":\"map50\",\"value\":{\"tableName\":\"TableD\",\"columnName\":\"F40\"}}," \
        "{\"name\":\"map51\",\"value\":{\"tableName\":\"TableD\"," \
        "\"columnName\":\"F41\"}},{\"name\":\"map52\",\"value\":{" \
        "\"tableName\":\"TableI\",\"columnName\":\"F42\"}},{\"name\":\"map53\"," \
        "\"value\":{\"tableName\":\"TableB\",\"columnName\":\"F43\"}}]},{\"tables\":[" \
        "\"TableA\",\"TableE\",\"TableF\",\"TableG\"],\"mappings\":[{\"name\":\"map54\"," \
        "\"values\":{\"tableName\":\"TableE\",\"columnName\":\"F44\"},\"where\":[{" \
        "\"equalto\":{\"tableName\":\"TableA\",\"columnName\":\"filter\",\"value\":1}}]}," \
        "{\"name\":\"map56\",\"function\":{\"lib\":\"/system/lib64/libtest_function.so\"," \
        "\"name\":\"fun_test16\",\"argList\":[{\"tableName\":\"TableA\"," \
        "\"columnName\":\"F1\"},{\"tableName\":\"TableG\",\"columnName\":\"F45\"}]," \
        "\"return_type\":\"Text\"}},{\"name\":\"map59\",\"value\":[{\"key\":\"F11\"," \
        "\"tableName\":\"TableG\",\"columnName\":\"F47\"},{\"key\":\"id\"," \
        "\"tableName\":\"TableG\",\"columnName\":\"F45\"}]},{\"name\":\"map60\"," \
        "\"value\":[{\"key\":\"F11\",\"tableName\":\"TableF\",\"columnName\":\"F47\"},{" \
        "\"key\":\"id\",\"tableName\":\"TableF\",\"columnName\":\"F45\"}]},{" \
        "\"name\":\"map61\",\"value\":[{\"key\":\"F11\",\"tableName\":\"TableG\"," \
        "\"columnName\":\"F47\"},{\"key\":\"name\",\"tableName\":\"TableG\"," \
        "\"columnName\":\"F46\"}]},{\"name\":\"map62\",\"value\":[{\"key\":\"F11\"," \
        "\"tableName\":\"TableF\",\"columnName\":\"F47\"},{\"key\":\"name\"," \
        "\"tableName\":\"TableF\",\"columnName\":\"F46\"}]}]},{\"tables\":[\"TableA\"," \
        "\"TableH\"],\"mappings\":[{\"name\":\"map63\",\"function\":{" \
        "\"lib\":\"/system/lib64/libtest_function.so\",\"name\":\"fun_test17\"," \
        "\"argList\":[{\"tableName\":\"TableH\",\"columnName\":\"F48\"},{" \
        "\"tableName\":\"TableH\",\"columnName\":\"F49\"},{\"tableName\":\"TableH\"," \
        "\"columnName\":\"F50\"},{\"tableName\":\"TableH\",\"columnName\":\"F51\"},{" \
        "\"tableName\":\"TableH\",\"columnName\":\"F52\"}],\"return_type\":\"Text\"}}]}]}" \
        "]}}";
};
}

#endif // DISTRIBUTEDDB_DATA_DONATION_SCHEMA_JSON_H
