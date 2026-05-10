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
#ifndef DATA_DONATION_SCHEMA_H
#define DATA_DONATION_SCHEMA_H

#include <vector>
#include "store_types.h"
#include "db_types.h"
#include "json_object.h"

#ifdef RELATIONAL_STORE
namespace DistributedDB {
using namespace std;

class DataDonationSchema {
public:
    int Init(const std::string &schema);
    void Clear();

    struct DdField {
        std::string table;
        std::string field;
    };
    struct DdCondition {
        bool enable = false;
        DdField field;
        int value = 0;
    };
    struct DdTrigger {
        std::string table; // Table that triggers the change
        // Fields and conditions that trigger the change: field, condition
        std::multimap<std::string, DdCondition> fields;
    };
    struct DdForeignKey {
        DdField localField; // Local table field, currently only supports single field
        DdField foreignField; // Associated foreign table field, currently only supports single field
    };
    struct DdKeyOut {
        // Key field to output, currently only supports single field; use vector<DdKeyOut> for multiple keys
        DdField item;
        DdCondition condition; // Condition for looking up the key
    };
    struct DdRelation {
        DdForeignKey key; // Store the association hops from this table to the target key
        DdField localField; // keyOut field, local table field, empty means no output needed
        DdField foreignField; // keyOut field, foreign table field, empty means no output needed
    };
    struct DdRelationsPath {
        // Currently, trigger conditions are not distinguished during Query
        // todo! Change to DdTrigger change trigger condition;
        // when any field in this table's fields meets the condition, the change is triggered
        std::string table;
        vector<DdRelation> relations;  // Store the association hops from this table to the target key
    };
    // Determine whether donation is needed, used by the wakeup interface
    bool NeedWakeup(DdTrigger &trigger);

    std::vector<DdKeyOut> GetKeyOut();

    // If no path is found, no wakeup is needed
    DataDonationSchema::DdRelationsPath& GetRelationPath(const std::string &table);
    // Used for full donation, provides keysOut via the shortest path
    DataDonationSchema::DdRelationsPath& GetRelationPath();
private:
    std::string str;
    std::unordered_map<std::string, DdRelationsPath> ddRelations;
    // Fields and conditions that trigger the change
    std::unordered_map<std::string, DdTrigger> triggers;
    std::unordered_map<std::string, DdForeignKey> foreignKeys;
    vector<DdKeyOut> keysOut;  // Output keys
    std::string FieldTypeString(FieldType inType) const;
    int ExtractJsonObj(const JsonObject &inJsonObject, const std::string &field, JsonObject &out) const;
    int ExtractJsonObjArray(const JsonObject &inJsonObject,
        const std::string &field, std::vector<JsonObject> &out) const;
    void DecodeWheres4KeyOut(const std::vector<JsonObject> &wheres, DdKeyOut &keyOut) const;
    void DecodeMappings4KeyOut(const std::vector<JsonObject> &mappings);
    int DecodeKeysOut(const JsonObject &src);
    int DecodePrimaryKeyMapping(const JsonObject &mapping, DdKeyOut &keyOut) const;
    int DecodeWhereConditions(const JsonObject &mapping, DdKeyOut &keyOut) const;
    int DecodeForeignKeys(const JsonObject &src);
    void DecodeMappings4Trigger(const std::vector<JsonObject> &mappings);
    int DecodeForeignKeyFromField(const std::string &tableName, const JsonObject &field);
    int DecodeTriggers(const JsonObject &src);
    void MergeRelationsMaps(DdTrigger &trigger);
    int DecodeRelationsMaps();
    int Decode(const std::string &schema);
    int MergeFields4Triggers(const JsonObject &arg, DdCondition &condition);
    int DecodeCondition4Triggers(const JsonObject &parent, DdCondition &condition) const;
    int DecodeFunction4Triggers(const JsonObject &mapping, const JsonObject &function);
    int DecodeOthers4Triggers(const JsonObject &mapping);
    bool InsertKeyOutAndCheckCompleted(DdRelationsPath &path);
};

}  // namespace DistributedDB
#endif  // RELATIONAL_STORE
#endif  // DATA_DONATION_SCHEMA_H