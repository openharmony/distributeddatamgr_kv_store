/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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
#include "sqlite_query_helper.h"

#include <iomanip>
#include <sstream>

#include "db_common.h"
#include "db_constant.h"
#include "db_errno.h"
#include "log_print.h"
#include "macro_utils.h"
#include "sqlite_utils.h"
#include "sqlite_single_ver_storage_executor_sql.h"
#include "cloud/cloud_storage_utils.h"
#include "res_finalizer.h"

namespace DistributedDB {
using namespace TriggerMode;
namespace {
const std::string PRE_QUERY_KV_SQL = "SELECT key, value FROM sync_data ";
const std::string PRE_QUERY_ITEM_SQL = "SELECT * FROM ";
const std::string PRE_QUERY_COUNT_ITEM_SQL = "SELECT count(*) FROM ";
const std::string PRE_QUERY_ROWID_SQL = "SELECT rowid FROM sync_data ";
const std::string PRE_GET_COUNT_SQL = "SELECT count(*) FROM sync_data ";
const std::string FILTER_NATIVE_DATA_SQL = "WHERE (flag&0x01=0) ";
const std::string FILTER_REMOTE_QUERY = "WHERE (flag&0x03=0x02)";
const std::string USING_INDEX = "INDEXED BY ";
const int MAX_SQL_LEN = 1024 * 1024; // 1M bytes
const int SINGLE_FIELD_VALUE_SIZE = 1;
const int MAX_CONDITIONS_SIZE = 128;
const int MAX_SQLITE_BIND_SIZE = 50000;
const uint32_t SYMBOL_TYPE_MASK = 0xff00;

const std::map<QueryObjType, std::string> RELATIONAL_SYMBOL_TO_SQL {
    {QueryObjType::EQUALTO, "= "},
    {QueryObjType::NOT_EQUALTO, "!= "},
    {QueryObjType::GREATER_THAN, "> "},
    {QueryObjType::LESS_THAN, "< "},
    {QueryObjType::GREATER_THAN_OR_EQUALTO, ">= "},
    {QueryObjType::LESS_THAN_OR_EQUALTO, "<= "},
    {QueryObjType::LIKE, " LIKE "},
    {QueryObjType::NOT_LIKE, " NOT LIKE "},
    {QueryObjType::IS_NULL, " IS NULL "},
    {QueryObjType::IS_NOT_NULL, " IS NOT NULL "},
    {QueryObjType::IN, " IN ("},
    {QueryObjType::NOT_IN, " NOT IN ("},
};

const std::map<QueryObjType, std::string> LOGIC_SYMBOL_TO_SQL {
    {QueryObjType::AND, " AND "},
    {QueryObjType::OR, " OR "},
    {QueryObjType::BEGIN_GROUP, "("},
    {QueryObjType::END_GROUP, ")"},
};

std::string FieldValue2String(const FieldValue &val, QueryValueType type)
{
    std::stringstream ss;
    switch (type) {
        case QueryValueType::VALUE_TYPE_NULL:
            return "NULL";
        case QueryValueType::VALUE_TYPE_BOOL:
            return val.boolValue ? "1" : "0";
        case QueryValueType::VALUE_TYPE_INTEGER:
            return std::to_string(val.integerValue);
        case QueryValueType::VALUE_TYPE_LONG:
            return std::to_string(val.longValue);
        case QueryValueType::VALUE_TYPE_DOUBLE:
            ss << std::setprecision(DBConstant::DOUBLE_PRECISION) << val.doubleValue;
            return ss.str();
        case QueryValueType::VALUE_TYPE_STRING:
            return "'" + val.stringValue + "'";
        case QueryValueType::VALUE_TYPE_INVALID:
        default:
            return "";
    }
}

std::string GetSelectAndFromClauseForRDB(const std::string &tableName, const std::vector<std::string> &fieldNames)
{
    std::string sql = "SELECT b.data_key,"
        "b.device,"
        "b.ori_device,"
        "b.timestamp as " + DBConstant::TIMESTAMP_ALIAS + ","
        "b.wtimestamp,"
        "b.flag,"
        "b.hash_key,";
    if (fieldNames.empty()) {  // For query check. If column count changed, can be discovered.
        sql += "a.*";
    } else {
        for (const auto &fieldName : fieldNames) {  // For query data.
            sql += "a.'" + fieldName + "',";
        }
        sql.pop_back();
    }
    sql += " FROM '" + tableName + "' AS a INNER JOIN " + DBConstant::RELATIONAL_PREFIX + tableName + "_log AS b "
        "ON a." + std::string(DBConstant::SQLITE_INNER_ROWID) + "=b.data_key ";
    return sql;
}

std::string GetTimeRangeClauseForRDB()
{
    return " AND (" + DBConstant::TIMESTAMP_ALIAS + ">=? AND " + DBConstant::TIMESTAMP_ALIAS + "<?) ";
}

std::string GetOuterQueryClauseForRDB(const std::string &subQueryClause)
{
    return "SELECT * "
        "FROM ( " + subQueryClause + " ) "
        "WHERE (" + DBConstant::TIMESTAMP_ALIAS + ">=? AND " + DBConstant::TIMESTAMP_ALIAS + "<?) "
        "ORDER BY " + DBConstant::TIMESTAMP_ALIAS + ";";
}

std::string GetFlagClauseForRDB()
{
    return "WHERE (b.flag&0x03=0x02)";
}

std::string GetMissQueryFlagClauseForRDB()
{
    return "WHERE (b.flag&0x23=0x22)";
}
}

SqliteQueryHelper::SqliteQueryHelper(const QueryObjInfo &info)
    : schema_(info.schema_),
      queryObjNodes_(info.queryObjNodes_),
      prefixKey_(info.prefixKey_),
      suggestIndex_(info.suggestIndex_),
      tableName_(info.tableName_),
      keys_(info.keys_),
      orderByCounts_(info.orderByCounts_),
      isValid_(info.isValid_),
      transformed_(false),
      hasOrderBy_(info.hasOrderBy_),
      hasLimit_(info.hasLimit_),
      isOrderByAppeared_(false),
      hasPrefixKey_(info.hasPrefixKey_),
      isNeedOrderbyKey_(false),
      isRelationalQuery_(info.isRelationalQuery_),
      sortType_(info.sortType_)
{}

SymbolType SqliteQueryHelper::GetSymbolType(const QueryObjType &queryObjType)
{
    return static_cast<SymbolType>(static_cast<uint32_t>(queryObjType) & SYMBOL_TYPE_MASK);
}

bool SqliteQueryHelper::FilterSymbolToAddBracketLink(std::string &querySql, bool isNeedLink) const
{
    bool isNeedEndBracket = false;
    for (const auto &iter : queryObjNodes_) {
        SymbolType symbolType = GetSymbolType(iter.operFlag);
        if (symbolType == SymbolType::COMPARE_SYMBOL || symbolType == SymbolType::RELATIONAL_SYMBOL ||
            symbolType == SymbolType::RANGE_SYMBOL) {
            querySql += isNeedLink ? " AND (" : " (";
            isNeedEndBracket = true;
            break;
        } else if (symbolType == SymbolType::LOGIC_SYMBOL || symbolType == SymbolType::PREFIXKEY_SYMBOL ||
            symbolType == SymbolType::IN_KEYS_SYMBOL) {
            continue;
        } else {
            break;
        }
    }
    return isNeedEndBracket;
}


int SqliteQueryHelper::ParseQueryObjNodeToSQL(bool isQueryForSync)
{
    if (queryObjNodes_.empty()) {
        if (!isQueryForSync) {
            querySql_ += ";";
        }
        return E_OK;
    }

    bool isNeedEndBracket = FilterSymbolToAddBracketLink(querySql_);

    int errCode = E_OK;
    for (const QueryObjNode &objNode : queryObjNodes_) {
        SymbolType symbolType = GetSymbolType(objNode.operFlag);
        if (symbolType == SymbolType::SPECIAL_SYMBOL && isNeedEndBracket) {
            querySql_ += ") ";
            isNeedEndBracket = false;
        }
        errCode = ParseQueryExpression(objNode, querySql_);
        if (errCode != E_OK) {
            querySql_.clear();
            return errCode;
        }
    }

    if (isNeedEndBracket) {
        querySql_ += ") ";
    }

    return errCode;
}

int SqliteQueryHelper::ToQuerySql()
{
    int errCode = ParseQueryObjNodeToSQL(false);
    if (errCode != E_OK) {
        return errCode;
    }

    // Limit needs to be placed after orderBy and processed separately in the limit branch
    if (hasPrefixKey_ && !hasOrderBy_ && !hasLimit_ && isNeedOrderbyKey_) {
        LOGD("Need add order by key at last when has prefixKey no need order by value and limit!");
        querySql_ += "ORDER BY key ASC";
    }
    querySql_ += ";";
    return errCode;
}

int SqliteQueryHelper::ToQuerySyncSql(bool hasSubQuery, bool useTimestampAlias)
{
    int errCode = ParseQueryObjNodeToSQL(true);
    if (errCode != E_OK) {
        return errCode;
    }

    // Order by time when no order by and no limit and no need order by key.
    if (!hasOrderBy_ && !hasLimit_ && !isNeedOrderbyKey_) {
        querySql_ += (useTimestampAlias ?
            ("ORDER BY " + DBConstant::TIMESTAMP_ALIAS + " ASC") :
            "ORDER BY timestamp ASC");
    }

    if (!hasSubQuery) {
        querySql_ += ";";
    }
    return errCode;
}

int SqliteQueryHelper::ToGetCountSql()
{
    countSql_.clear();
    if (queryObjNodes_.empty()) {
        countSql_ += ";";
        return E_OK;
    }
    bool isNeedEndBracket = FilterSymbolToAddBracketLink(countSql_);

    int errCode = E_OK;
    for (const QueryObjNode &objNode : queryObjNodes_) {
        SymbolType symbolType = GetSymbolType(objNode.operFlag);
        if (symbolType == SymbolType::SPECIAL_SYMBOL && isNeedEndBracket) {
            countSql_ += ") ";
            isNeedEndBracket = false;
        }

        if (objNode.operFlag == QueryObjType::LIMIT) {
            hasLimit_ = true;
            continue;
        }
        if (objNode.operFlag == QueryObjType::ORDERBY) {
            hasOrderBy_ = true;
            continue;
        }
        errCode = ParseQueryExpression(objNode, countSql_);
        if (errCode != E_OK) {
            countSql_.clear();
            return errCode;
        }
    }

    if (isNeedEndBracket) {
        countSql_ += ") ";
    }

    // Limit needs to be placed after orderBy and processed separately in the limit branch
    if (hasPrefixKey_ && !hasOrderBy_ && !hasLimit_ && isNeedOrderbyKey_) {
        LOGD("Need add order by key at last when has prefixKey no need order by value and limit!");
        countSql_ += "ORDER BY key ASC";
    }
    countSql_ += ";";
    return errCode;
}

int SqliteQueryHelper::GetQuerySql(std::string &sql, bool onlyRowid)
{
    if (!isValid_) {
        return -E_INVALID_QUERY_FORMAT;
    }

    const std::string &querySqlForUse = (onlyRowid ? PRE_QUERY_ROWID_SQL : PRE_QUERY_KV_SQL);
    sql = AssembleSqlForSuggestIndex(querySqlForUse, FILTER_NATIVE_DATA_SQL);
    sql = !hasPrefixKey_ ? sql : (sql + " AND (key>=? AND key<=?) ");
    sql = keys_.empty() ? sql : (sql + " AND " + MapKeysInToSql(keys_.size()));
    sql += " AND (flag&0x200=0) ";
    if (sortType_ != SortType::NONE) {
        sql += (sortType_ == SortType::TIMESTAMP_ASC) ? "ORDER BY timestamp asc " : "ORDER BY timestamp desc ";
    }
    if (transformed_) {
        LOGD("This query object has been parsed.");
        sql += querySql_;
        return E_OK;
    }
    int errCode = ToQuerySql();
    if (errCode != E_OK) {
        LOGE("Transfer to query sql failed! errCode[%d]", errCode);
        return errCode;
    }
    transformed_ = true;
    sql += querySql_;
    return errCode;
}

int SqliteQueryHelper::GetSyncDataCheckSql(std::string &sql)
{
    int errCode = E_OK;
    if (!isValid_) {
        return -E_INVALID_QUERY_FORMAT;
    }
    sql = PRE_QUERY_ITEM_SQL + tableName_ + " WHERE hash_key=? AND (flag&0x01=0) ";
    sql += hasPrefixKey_ ? " AND (key>=? AND key<=?) " : "";
    sql = keys_.empty() ? sql : (sql + " AND " + MapKeysInToSql(keys_.size()));
    if (!transformed_) {
        errCode = ToQuerySql();
        if (errCode != E_OK) {
            LOGE("Transfer query to sync data check sql failed! errCode[%d]", errCode);
            return errCode;
        }
        transformed_ = true;
    }
    sql += querySql_;
    return errCode;
}

int SqliteQueryHelper::BindSyncDataCheckStmt(sqlite3_stmt *statement, const Key &hashKey) const
{
    if (statement == nullptr) {
        return -E_INVALID_ARGS;
    }
    int index = 1; // bind statement start index 1
    int errCode = SQLiteUtils::BindBlobToStatement(statement, index++, hashKey, false);
    if (errCode != E_OK) {
        LOGE("Get sync data check statement failed when bind hash key, errCode = %d", errCode);
        return errCode;
    }
    if (hasPrefixKey_) {
        // bind the prefix key for the first and second args.
        errCode = SQLiteUtils::BindPrefixKey(statement, index, prefixKey_);
        if (errCode != E_OK) {
            LOGE("Get sync data check statement failed when bind prefix key, errCode = %d", errCode);
            return errCode;
        }
        index += 2; // prefixKey takes 2 position
    }

    errCode = BindKeysToStmt(keys_, statement, index);
    if (errCode != E_OK) {
        int ret = E_OK;
        SQLiteUtils::ResetStatement(statement, true, ret);
        return errCode;
    }

    for (const QueryObjNode &objNode : queryObjNodes_) {
        errCode = BindFieldValue(statement, objNode, index);
        if (errCode != E_OK) {
            LOGE("Get sync data check statement failed when bind field value, errCode = %d", errCode);
            return errCode;
        }
    }
    return errCode;
}

int SqliteQueryHelper::GetCountQuerySql(std::string &sql)
{
    if (!isValid_) {
        return -E_INVALID_QUERY_FORMAT;
    }

    int errCode = ToGetCountSql();
    if (errCode != E_OK) {
        return errCode;
    }
    sql = AssembleSqlForSuggestIndex(PRE_GET_COUNT_SQL, FILTER_NATIVE_DATA_SQL);
    sql = !hasPrefixKey_ ? sql : (sql + " AND (key>=? AND key<=?) ");
    sql = keys_.empty() ? sql : (sql + " AND " + MapKeysInToSql(keys_.size()));
    sql += " AND (flag&0x200=0) ";
    sql += countSql_;
    return E_OK;
}

int SqliteQueryHelper::GetQuerySqlStatement(sqlite3 *dbHandle, const std::string &sql, sqlite3_stmt *&statement)
{
    int errCode = SQLiteUtils::GetStatement(dbHandle, sql, statement);
    if (errCode != E_OK) {
        LOGE("[Query] Get statement fail!");
        return -E_INVALID_QUERY_FORMAT;
    }
    int index = 1;
    int ret = E_OK;
    if (hasPrefixKey_) {
        // bind the prefix key for the first and second args.
        errCode = SQLiteUtils::BindPrefixKey(statement, 1, prefixKey_);
        if (errCode != E_OK) {
            SQLiteUtils::ResetStatement(statement, true, ret);
            LOGE("[Query] Get statement when bind prefix key, errCode = %d", errCode);
            return errCode;
        }
        index = 3; // begin from 3rd args
    }

    errCode = BindKeysToStmt(keys_, statement, index);
    if (errCode != E_OK) {
        SQLiteUtils::ResetStatement(statement, true, ret);
        return errCode;
    }

    for (const QueryObjNode &objNode : queryObjNodes_) {
        errCode = BindFieldValue(statement, objNode, index);
        if (errCode != E_OK) {
            SQLiteUtils::ResetStatement(statement, true, ret);
            LOGE("[Query] Get statement fail when bind field value, errCode = %d", errCode);
            return errCode;
        }
    }
    return errCode;
}

int SqliteQueryHelper::GetQuerySqlStatement(sqlite3 *dbHandle, bool onlyRowid, sqlite3_stmt *&statement)
{
    std::string sql;
    int errCode = GetQuerySql(sql, onlyRowid);
    if (errCode != E_OK) {
        return errCode;
    }

    errCode = SQLiteUtils::GetStatement(dbHandle, sql, statement);
    if (errCode != E_OK) {
        LOGE("[Query] Get statement fail!");
        return -E_INVALID_QUERY_FORMAT;
    }
    int index = 1;
    if (hasPrefixKey_) {
        // bind the prefix key for the first and second args.
        errCode = SQLiteUtils::BindPrefixKey(statement, 1, prefixKey_);
        if (errCode != E_OK) {
            LOGE("[Query] Get statement when bind prefix key, errCode = %d", errCode);
            return errCode;
        }
        index = 3; // begin from 3rd args
    }

    errCode = BindKeysToStmt(keys_, statement, index);
    if (errCode != E_OK) {
        return errCode;
    }

    for (const QueryObjNode &objNode : queryObjNodes_) {
        errCode = BindFieldValue(statement, objNode, index);
        if (errCode != E_OK) {
            LOGE("[Query] Get statement fail when bind field value, errCode = %d", errCode);
            return errCode;
        }
    }
    return errCode;
}

int SqliteQueryHelper::GetCountSqlStatement(sqlite3 *dbHandle, sqlite3_stmt *&countStmt)
{
    std::string countSql;
    int errCode = GetCountQuerySql(countSql);
    if (errCode != E_OK) {
        return errCode;
    }

    // bind statement for count
    errCode = SQLiteUtils::GetStatement(dbHandle, countSql, countStmt);
    if (errCode != E_OK) {
        LOGE("Get count statement error:%d", errCode);
        return -E_INVALID_QUERY_FORMAT;
    }
    int index = 1;
    if (hasPrefixKey_) {
        // bind the prefix key for the first and second args.
        errCode = SQLiteUtils::BindPrefixKey(countStmt, 1, prefixKey_);
        if (errCode != E_OK) {
            LOGE("[Query] Get count statement fail when bind prefix key, errCode = %d", errCode);
            return errCode;
        }
        index = 3; // begin from 3rd args
    }

    errCode = BindKeysToStmt(keys_, countStmt, index);
    if (errCode != E_OK) {
        return errCode;
    }

    for (const QueryObjNode &objNode : queryObjNodes_) {
        if (GetSymbolType(objNode.operFlag) == SymbolType::SPECIAL_SYMBOL) {
            continue;
        }
        errCode = BindFieldValue(countStmt, objNode, index);
        if (errCode != E_OK) {
            LOGE("[Query] Get count statement fail when bind field value, errCode = %d", errCode);
            return errCode;
        }
    }
    return errCode;
}

int SqliteQueryHelper::GetSyncDataQuerySql(std::string &sql, bool hasSubQuery, bool isCount)
{
    if (!isValid_) {
        return -E_INVALID_QUERY_FORMAT;
    }

    if (hasLimit_) {
        hasSubQuery = true; // Need sub query.
    } else {
        isNeedOrderbyKey_ = false; // Need order by timestamp.
    }

    sql = AssembleSqlForSuggestIndex(((isCount && !hasSubQuery) ?
        PRE_QUERY_COUNT_ITEM_SQL : PRE_QUERY_ITEM_SQL) + tableName_ + " ", FILTER_REMOTE_QUERY);
    sql = !hasPrefixKey_ ? sql : (sql + " AND (key>=? AND key<=?) ");
    sql = keys_.empty() ? sql : (sql + " AND " + MapKeysInToSql(keys_.size()));
    sql = hasSubQuery ? sql : (sql + " AND (timestamp>=? AND timestamp<?) ");
    sql += " AND (flag&0x200=0) ";

    querySql_.clear(); // clear local query sql format
    int errCode = ToQuerySyncSql(hasSubQuery);
    if (errCode != E_OK) {
        LOGE("To query sql fail! errCode[%d]", errCode);
        return errCode;
    }
    sql += querySql_;
    if (hasSubQuery) {
        // The last timestamp in one query will be stored in continue token and used for next query.
        // Therefore all query data must be ordered by timestamp.
        // When there is limit in SQL, data should be ordered by key in sub query, and timestamp is ordered by outside.
        std::string sqlHead = isCount ? PRE_QUERY_COUNT_ITEM_SQL : PRE_QUERY_ITEM_SQL;
        sql = sqlHead + "( " + sql + " ) WHERE (timestamp >= ? AND timestamp < ?) ORDER BY timestamp;";
    }
    return errCode;
}

int SqliteQueryHelper::BindTimeRange(sqlite3_stmt *&statement, int &index, uint64_t beginTime, uint64_t endTime) const
{
    int ret = E_OK;
    int errCode = SQLiteUtils::BindInt64ToStatement(statement, index++, beginTime);
    if (errCode != E_OK) {
        SQLiteUtils::ResetStatement(statement, true, ret);
        return errCode;
    }

    errCode = SQLiteUtils::BindInt64ToStatement(statement, index++, endTime);
    if (errCode != E_OK) {
        SQLiteUtils::ResetStatement(statement, true, ret);
    }
    return errCode;
}

int SqliteQueryHelper::BindObjNodes(sqlite3_stmt *&statement, int &index) const
{
    int errCode = E_OK;
    int ret = E_OK;
    for (const QueryObjNode &objNode : queryObjNodes_) {
        errCode = BindFieldValue(statement, objNode, index);
        if (errCode != E_OK) {
            SQLiteUtils::ResetStatement(statement, true, ret);
            LOGE("[Query] Get statement fail when bind field value, errCode = %d", errCode);
            break;
        }
    }
    return errCode;
}

int SqliteQueryHelper::GetQuerySyncStatement(sqlite3 *dbHandle, uint64_t beginTime, uint64_t endTime,
    sqlite3_stmt *&statement, bool isCount)
{
    bool hasSubQuery = false;
    if (hasLimit_) {
        hasSubQuery = true; // Need sub query.
    } else {
        isNeedOrderbyKey_ = false; // Need order by timestamp.
    }
    std::string sql;
    int errCode = GetSyncDataQuerySql(sql, hasSubQuery, isCount);
    if (errCode != E_OK) {
        LOGE("[Query] Get SQL fail!");
        return -E_INVALID_QUERY_FORMAT;
    }

    errCode = SQLiteUtils::GetStatement(dbHandle, sql, statement);
    if (errCode != E_OK) {
        LOGE("[Query] Get statement fail!");
        return -E_INVALID_QUERY_FORMAT;
    }

    int index = 1; // begin with 1.
    int ret = E_OK;
    if (hasPrefixKey_) {
        // bind the prefix key for the first and second args.
        errCode = SQLiteUtils::BindPrefixKey(statement, index, prefixKey_);
        if (errCode != E_OK) {
            SQLiteUtils::ResetStatement(statement, true, ret);
            LOGE("[Query] Get statement when bind prefix key, errCode = %d", errCode);
            return errCode;
        }
        index = 3; // begin with 3 next if prefix key exists.
    }

    errCode = BindKeysToStmt(keys_, statement, index);
    if (errCode != E_OK) {
        SQLiteUtils::ResetStatement(statement, true, ret);
        return errCode;
    }

    if (hasSubQuery) {
        // For sub query SQL, timestamp must be last : (prefix key), (objNodes), timestamp.
        // SQL: SELECT * FROM ( SELECT * FROM sync_data WHERE (flag&0x03=0x02) LIMIT 10 OFFSET 0 ) WHERE (timestamp>=?
        //      AND timestamp<?) ORDER BY timestamp;
        errCode = BindObjNodes(statement, index);
        if (errCode != E_OK) {
            return errCode;
        }
        errCode = BindTimeRange(statement, index, beginTime, endTime);
    } else {
        // For normal SQL(no sub query), timestamp is just behind prefix key : (prefix key), timestamp, (ojbNodes).
        // SQL: SELECT * FROM sync_data WHERE (timestamp>=? AND timestamp<?) AND (flag&0x03=0x02) ORDER BY timestamp;
        errCode = BindTimeRange(statement, index, beginTime, endTime);
        if (errCode != E_OK) {
            return errCode;
        }
        errCode = BindObjNodes(statement, index);
    }
    return errCode;
}

std::string SqliteQueryHelper::MapRelationalSymbolToSql(const QueryObjNode &queryNode, bool placeholder) const
{
    if (RELATIONAL_SYMBOL_TO_SQL.find(queryNode.operFlag) == RELATIONAL_SYMBOL_TO_SQL.end()) {
        return "";
    };
    std::string sql = RELATIONAL_SYMBOL_TO_SQL.at(queryNode.operFlag) + MapValueToSql(queryNode, placeholder);
    if (GetSymbolType(queryNode.operFlag) == SymbolType::RANGE_SYMBOL) {
        sql += ")";
    }
    return sql;
}

std::string SqliteQueryHelper::MapLogicSymbolToSql(const QueryObjNode &queryNode) const
{
    if (LOGIC_SYMBOL_TO_SQL.find(queryNode.operFlag) == LOGIC_SYMBOL_TO_SQL.end()) {
        return "";
    }
    return LOGIC_SYMBOL_TO_SQL.at(queryNode.operFlag);
}

std::string SqliteQueryHelper::MapKeywordSymbolToSql(const QueryObjNode &queryNode)
{
    std::string sql;
    switch (queryNode.operFlag) {
        case QueryObjType::ORDERBY:
            if (queryNode.fieldValue.size() == SINGLE_FIELD_VALUE_SIZE) {
                if (!isOrderByAppeared_) {
                    sql += "ORDER BY ";
                }
                sql += GetFieldShape(queryNode);
                sql += queryNode.fieldValue[0].boolValue ? "ASC," : "DESC,";
                orderByCounts_--;
                if (orderByCounts_ == 0) {
                    sql.pop_back();
                }
                isOrderByAppeared_ = true;
            }
            return sql;
        case QueryObjType::LIMIT:
            if (hasPrefixKey_ && !hasOrderBy_ && isNeedOrderbyKey_) {
                sql += "ORDER BY key ASC ";
            }
            sql += " LIMIT " + std::to_string(queryNode.fieldValue[0].integerValue);
            sql += " OFFSET " + std::to_string(queryNode.fieldValue[1].integerValue);
            return sql;
        default:
            return "";
    }
}

std::string SqliteQueryHelper::MapValueToSql(const QueryObjNode &queryNode, bool placeholder) const
{
    std::string resultSql;
    for (size_t i = 0; i < queryNode.fieldValue.size(); i++) {
        if (i != 0) {
            resultSql += ", ";
        }
        resultSql += placeholder ? "? " : FieldValue2String(queryNode.fieldValue[i], queryNode.type);
    }
    return resultSql;
}

static bool IsNeedCastWitEmptyValue(const QueryObjNode &queryNode)
{
    return (queryNode.operFlag == QueryObjType::IS_NULL || queryNode.operFlag == QueryObjType::IS_NOT_NULL ||
        queryNode.operFlag == QueryObjType::IN || queryNode.operFlag == QueryObjType::NOT_IN);
}

std::string SqliteQueryHelper::MapCastFuncSql(const QueryObjNode &queryNode, const std::string &accessStr)
{
    std::string resultSql;
    if (queryNode.fieldValue.empty() && !IsNeedCastWitEmptyValue(queryNode)) {
        return resultSql;
    }
    // fieldPath and isQueryable had been checked ok in the previous code, So here parse path and get type won't fail.
    FieldPath fieldPath;
    SchemaUtils::ParseAndCheckFieldPath(queryNode.fieldName, fieldPath);
    FieldType fieldType = FieldType::LEAF_FIELD_INTEGER;
    schema_.CheckQueryableAndGetFieldType(fieldPath, fieldType);
    resultSql += SchemaObject::GenerateExtractSQL(schema_.GetSchemaType(), fieldPath, fieldType,
        schema_.GetSkipSize(), accessStr);
    isNeedOrderbyKey_ = false; // When index by value, No need order by key!
    return resultSql;
}

int SqliteQueryHelper::BindFieldValue(sqlite3_stmt *statement, const QueryObjNode &queryNode, int &index) const
{
    SymbolType symbolType = GetSymbolType(queryNode.operFlag);
    if (symbolType != SymbolType::COMPARE_SYMBOL && symbolType != SymbolType::RELATIONAL_SYMBOL &&
        symbolType != SymbolType::RANGE_SYMBOL) {
        return E_OK;
    }

    int errCode = SQLITE_OK;
    for (size_t i = 0; i < queryNode.fieldValue.size(); i++) {
        if (queryNode.type == QueryValueType::VALUE_TYPE_BOOL) {
            errCode = sqlite3_bind_int(statement, index, queryNode.fieldValue[i].boolValue);
        } else if (queryNode.type == QueryValueType::VALUE_TYPE_INTEGER) {
            errCode = sqlite3_bind_int(statement, index, queryNode.fieldValue[i].integerValue);
        } else if (queryNode.type == QueryValueType::VALUE_TYPE_LONG) {
            errCode = sqlite3_bind_int64(statement, index, queryNode.fieldValue[i].longValue);
        } else if (queryNode.type == QueryValueType::VALUE_TYPE_DOUBLE) {
            errCode = sqlite3_bind_double(statement, index, queryNode.fieldValue[i].doubleValue);
        } else {
            if (queryNode.fieldValue[i].stringValue.size() > MAX_SQLITE_BIND_SIZE) {
                return -E_MAX_LIMITS;
            }
            errCode = sqlite3_bind_text(statement, index, queryNode.fieldValue[i].stringValue.c_str(),
                queryNode.fieldValue[i].stringValue.size(), SQLITE_TRANSIENT);
        }
        if (errCode != SQLITE_OK) {
            break;
        }
        index++;
    }
    return SQLiteUtils::MapSQLiteErrno(errCode);
}

std::string SqliteQueryHelper::GetFieldShape(const QueryObjNode &queryNode, const std::string &accessStr)
{
    if (isRelationalQuery_) {
        // For relational query, $. prefix is not permitted, so need not extract json. Return directly will be OK.
        return "a.'" + queryNode.fieldName + "' ";
    }
    return MapCastFuncSql(queryNode, accessStr);
}

int SqliteQueryHelper::ParseQueryExpression(const QueryObjNode &queryNode, std::string &querySql,
    const std::string &accessStr, bool placeholder)
{
    SymbolType symbolType = GetSymbolType(queryNode.operFlag);
    if (symbolType == SymbolType::RANGE_SYMBOL && queryNode.fieldValue.size() > MAX_CONDITIONS_SIZE) {
        LOGE("[Query][Parse][Expression] conditions is too many!");
        return -E_MAX_LIMITS;
    }

    if (symbolType == SymbolType::COMPARE_SYMBOL || symbolType == SymbolType::RELATIONAL_SYMBOL ||
        symbolType == SymbolType::RANGE_SYMBOL) {
        querySql += GetFieldShape(queryNode, accessStr);
        querySql += MapRelationalSymbolToSql(queryNode, placeholder);
    } else if (symbolType == SymbolType::LOGIC_SYMBOL || symbolType == SymbolType::LINK_SYMBOL) {
        querySql += MapLogicSymbolToSql(queryNode);
    } else {
        querySql += MapKeywordSymbolToSql(queryNode);
    }

    if (querySql.size() > MAX_SQL_LEN) {
        LOGE("[Query][Parse][Expression] Sql is too long!");
        return -E_MAX_LIMITS;
    }
    return E_OK;
}

std::string SqliteQueryHelper::AssembleSqlForSuggestIndex(const std::string &baseSql, const std::string &filter) const
{
    std::string formatIndex = CheckAndFormatSuggestIndex();
    if (formatIndex.empty()) {
        return baseSql + filter;
    }

    return baseSql + USING_INDEX + "'" + formatIndex + "' " + filter;
}

std::string SqliteQueryHelper::CheckAndFormatSuggestIndex() const
{
    if (suggestIndex_.empty()) {
        return "";
    }
    IndexName indexName;
    int errCode = SchemaUtils::ParseAndCheckFieldPath(suggestIndex_, indexName);
    if (errCode != E_OK) {
        LOGW("Check and format suggest index failed! %d", errCode);
        return "";
    }

    if (!schema_.IsIndexExist(indexName)) {
        LOGW("The suggest index not exist!");
        return "";
    }
    return SchemaUtils::FieldPathString(indexName);
}

std::string SqliteQueryHelper::MapKeysInSubCondition(const std::string &accessStr) const
{
    std::string resultStr = "hex(" + accessStr + "key) IN (";
    for (auto iter = keys_.begin(); iter != keys_.end(); iter++) {
        if (iter != keys_.begin()) {
            resultStr += ", ";
        }
        resultStr += "'" + DBCommon::VectorToHexString(*iter) + "' ";
    }
    resultStr += ")";
    return resultStr;
}

int SqliteQueryHelper::GetSubscribeCondition(const std::string &accessStr, std::string &conditionStr)
{
    if (queryObjNodes_.empty()) {
        conditionStr += " (1 = 1) ";
        return E_OK;
    }

    bool hasQueryByValue = std::any_of(queryObjNodes_.begin(), queryObjNodes_.end(), [](const QueryObjNode &it) {
        return GetSymbolType(it.operFlag) == SymbolType::COMPARE_SYMBOL ||
            GetSymbolType(it.operFlag) == SymbolType::RELATIONAL_SYMBOL ||
            GetSymbolType(it.operFlag) == SymbolType::RANGE_SYMBOL;
    });
    if (hasQueryByValue) {
        // json_extract_by_path function will return error when value is empty, check it before when query by value
        conditionStr += "((length(" + accessStr + "value) != 0 AND " + accessStr + "value IS NOT NULL) AND ";
    } else {
        conditionStr += "(";
    }

    if (hasPrefixKey_) {
        conditionStr += "(hex(" + accessStr + "key) LIKE '" + DBCommon::VectorToHexString(prefixKey_) + "%')";
    }

    if (!keys_.empty()) {
        if (hasPrefixKey_) {
            conditionStr += " AND ";
        }
        conditionStr += "(" + MapKeysInSubCondition(accessStr) + ")";
    }

    bool isNeedEndBracket = FilterSymbolToAddBracketLink(conditionStr, hasPrefixKey_ || !keys_.empty());
    int errCode = E_OK;
    for (const QueryObjNode &objNode : queryObjNodes_) {
        SymbolType symbolType = GetSymbolType(objNode.operFlag);
        if (symbolType == SymbolType::SPECIAL_SYMBOL && isNeedEndBracket) {
            conditionStr += ") ";
            isNeedEndBracket = false;
        }
        errCode = ParseQueryExpression(objNode, conditionStr, accessStr, false);
        if (errCode != E_OK) {
            conditionStr.clear();
            return errCode;
        }
    }

    if (isNeedEndBracket) {
        conditionStr += ") ";
    }
    conditionStr += ")";
    return errCode;
}

int SqliteQueryHelper::GetSubscribeSql(TriggerModeEnum mode, std::string &subscribeCondition)
{
    if (!isValid_) {
        return -E_INVALID_QUERY_FORMAT;
    }
    int errCode = E_OK;
    switch (mode) {
        case TriggerModeEnum::INSERT:
            errCode = GetSubscribeCondition(DBConstant::TRIGGER_REFERENCES_NEW, subscribeCondition);
            break;
        case TriggerModeEnum::UPDATE:
            errCode = GetSubscribeCondition(DBConstant::TRIGGER_REFERENCES_OLD, subscribeCondition);
            if (errCode != E_OK) {
                break;
            }
            subscribeCondition += " OR ";
            errCode = GetSubscribeCondition(DBConstant::TRIGGER_REFERENCES_NEW, subscribeCondition);
            break;
        case TriggerModeEnum::DELETE:
            errCode = GetSubscribeCondition(DBConstant::TRIGGER_REFERENCES_OLD, subscribeCondition);
            break;
        default:
            errCode = -INVALID_ARGS;
    }
    if (errCode != E_OK) {
        LOGD("Get subscribe query condition failed. %d", errCode);
    }
    return errCode;
}

int SqliteQueryHelper::GetRelationalMissQuerySql(const std::vector<std::string> &fieldNames, std::string &sql)
{
    if (!isValid_) {
        return -E_INVALID_QUERY_FORMAT;
    }

    if (hasPrefixKey_) {
        LOGE("For relational DB query, prefix key is not supported.");
        return -E_NOT_SUPPORT;
    }

    sql = GetSelectAndFromClauseForRDB(tableName_, fieldNames);
    sql += GetMissQueryFlagClauseForRDB();
    sql += GetTimeRangeClauseForRDB();
    sql += "ORDER BY " + DBConstant::TIMESTAMP_ALIAS + " ASC;";
    return E_OK;
}

int SqliteQueryHelper::GetRelationalSyncDataQuerySql(std::string &sql, bool hasSubQuery,
    const std::vector<std::string> &fieldNames)
{
    if (!isValid_) {
        return -E_INVALID_QUERY_FORMAT;
    }

    if (hasPrefixKey_) {
        LOGE("For relational DB query, prefix key is not supported.");
        return -E_NOT_SUPPORT;
    }

    sql = AssembleSqlForSuggestIndex(GetSelectAndFromClauseForRDB(tableName_, fieldNames), GetFlagClauseForRDB());
    sql = hasSubQuery ? sql : (sql + GetTimeRangeClauseForRDB());

    querySql_.clear(); // clear local query sql format
    int errCode = ToQuerySyncSql(hasSubQuery, true);
    if (errCode != E_OK) {
        LOGE("To query sql fail! errCode[%d]", errCode);
        return errCode;
    }
    sql += querySql_;
    if (hasSubQuery) {
        // The last timestamp in one query will be stored in continue token and used for next query.
        // Therefore all query data must be ordered by timestamp.
        // When there is limit in SQL, data should be ordered by key in sub query, and timestamp is ordered by outside.
        sql = GetOuterQueryClauseForRDB(sql);
    }
    return errCode;
}

int SqliteQueryHelper::GetRelationalMissQueryStatement(sqlite3 *dbHandle, uint64_t beginTime, uint64_t endTime,
    const std::vector<std::string> &fieldNames, sqlite3_stmt *&statement)
{
    std::string sql;
    int errCode = GetRelationalMissQuerySql(fieldNames, sql);
    if (errCode != E_OK) {
        LOGE("[Query] Get SQL fail!");
        return -E_INVALID_QUERY_FORMAT;
    }

    errCode = SQLiteUtils::GetStatement(dbHandle, sql, statement);
    if (errCode != E_OK) {
        LOGE("[Query] Get statement fail!");
        return -E_INVALID_QUERY_FORMAT;
    }

    int index = 1; // begin with 1.
    return BindTimeRange(statement, index, beginTime, endTime);
}

namespace {
std::string GetRelationalSyncDataQueryHeader(const std::vector<std::string> &fieldNames)
{
    std::string sql = "SELECT b.data_key,"
        "b.device,"
        "b.ori_device,"
        "b.timestamp as " + DBConstant::TIMESTAMP_ALIAS + ","
        "b.wtimestamp,"
        "b.flag,"
        "b.hash_key,";
    if (fieldNames.empty()) {  // For query check. If column count changed, can be discovered.
        sql += "a.*";
    } else {
        for (const auto &fieldName : fieldNames) {  // For query data.
            sql += "a." + fieldName + ",";
        }
        sql.pop_back();
    }
    return sql;
}

std::string GetRelationalCloudSyncDataQueryHeader(const std::vector<Field> &fields)
{
    std::string sql = "SELECT b.data_key,"
        "b.device,"
        "b.ori_device,"
        "b.timestamp as " + DBConstant::TIMESTAMP_ALIAS + ","
        "b.wtimestamp,"
        "b.flag,"
        "b.hash_key,"
        "b.cloud_gid,"
        "b.version,"
        "b.status,";
    if (fields.empty()) {  // For query check. If column count changed, can be discovered.
        sql += "a.*";
    } else {
        for (const auto &field : fields) {  // For query data.
            sql += "a." + field.colName + ",";
        }
        sql.pop_back();
    }
    return sql;
}
}

int SqliteQueryHelper::GetRelationalSyncDataQuerySqlWithLimit(const std::vector<std::string> &fieldNames,
    std::string &sql)
{
    if (!isValid_) {
        return -E_INVALID_QUERY_FORMAT;
    }

    if (hasPrefixKey_) {
        LOGE("For relational DB query, prefix key is not supported.");
        return -E_NOT_SUPPORT;
    }
    sql = GetRelationalSyncDataQueryHeader(fieldNames);
    sql += " FROM '" + tableName_ + "' AS a INNER JOIN ";
    sql += DBConstant::RELATIONAL_PREFIX + tableName_ + "_log";
    sql += " AS b ON (a." + std::string(DBConstant::SQLITE_INNER_ROWID) + " = b.data_key)";
    sql += " WHERE (b.flag&0x03=0x02)";

    querySql_.clear(); // clear local query sql format
    int errCode = ToQuerySyncSql(true, true);
    if (errCode != E_OK) {
        LOGE("To query sql fail! errCode[%d]", errCode);
        return errCode;
    }
    sql += querySql_;
    return E_OK;
}

int SqliteQueryHelper::GetRelationalQueryStatement(sqlite3 *dbHandle, uint64_t beginTime, uint64_t endTime,
    const std::vector<std::string> &fieldNames, sqlite3_stmt *&statement)
{
    int errCode = E_OK;
    bool hasSubQuery = false;
    std::string sql;
    if (hasLimit_ && !hasOrderBy_) { // Query with limit and no order by
        hasSubQuery = true; // Need sub query.
        errCode = GetRelationalSyncDataQuerySqlWithLimit(fieldNames, sql);
    } else if (hasLimit_ || hasOrderBy_) {
        hasSubQuery = true; // Need sub query.
        errCode = GetRelationalSyncDataQuerySql(sql, hasSubQuery, fieldNames);
    } else {
        isNeedOrderbyKey_ = false; // Need order by timestamp.
        errCode = GetRelationalSyncDataQuerySql(sql, hasSubQuery, fieldNames);
    }
    if (errCode != E_OK) {
        LOGE("[Query] Get SQL fail!");
        return -E_INVALID_QUERY_FORMAT;
    }

    errCode = SQLiteUtils::GetStatement(dbHandle, sql, statement);
    if (errCode != E_OK) {
        LOGE("[Query] Get statement fail!");
        return -E_INVALID_QUERY_FORMAT;
    }

    int index = 1; // begin with 1.
    if (hasSubQuery) {
        /**
         *   SELECT b.data_key,b.device,b.ori_device,b.timestamp as naturalbase_rdb_timestamp,
         *          b.wtimestamp,b.flag,b.hash_key,a.*
         *   FROM tableName AS a INNER JOIN naturalbase_rdb_log AS b
         *   ON a.rowid=b.data_key
         *   WHERE (b.flag&0x03=0x02)
         *   ORDER BY a.xxx
         *   LIMIT ? OFFSET ? )
         */
        errCode = BindObjNodes(statement, index);
        if (errCode != E_OK) {
            return errCode;
        }
        if (!hasLimit_ || hasOrderBy_) {
            errCode = BindTimeRange(statement, index, beginTime, endTime);
        }
    } else {
        /**
         * SELECT b.data_key,b.device,b.ori_device,b.timestamp as naturalbase_rdb_timestamp,
         *        b.wtimestamp,b.flag,b.hash_key,a.*
         * FROM tableName AS a INNER JOIN naturalbase_rdb_log AS b
         * ON a.rowid=b.data_key
         * WHERE (b.flag&0x03=0x02) AND (naturalbase_rdb_timestamp>=? AND naturalbase_rdb_timestamp<?)
         * ORDER BY naturalbase_rdb_timestamp ASC;
         */
        errCode = BindTimeRange(statement, index, beginTime, endTime);
        if (errCode != E_OK) {
            return errCode;
        }
        errCode = BindObjNodes(statement, index);
    }
    return errCode;
}

std::string SqliteQueryHelper::MapKeysInToSql(size_t keysNum) const
{
    std::string resultSql = "key IN (";
    for (size_t i = 0; i < keysNum; i++) {
        if (i != 0) {
            resultSql += ", ";
        }
        resultSql += "? ";
    }
    resultSql += ") ";
    return resultSql;
}

int SqliteQueryHelper::BindKeysToStmt(const std::set<Key> &keys, sqlite3_stmt *&statement, int &index) const
{
    if (!keys_.empty()) {
        int errCode = E_OK;
        for (const auto &key : keys) {
            errCode = SQLiteUtils::BindBlobToStatement(statement, index, key);
            if (errCode != E_OK) {
                LOGE("[Query] Get statement when bind keys failed, errCode = %d", errCode);
                return errCode;
            }
            index++;
        }
    }
    return E_OK;
}

std::string SqliteQueryHelper::GetRelationalCloudQuerySql(const std::vector<Field> &fields,
    const bool &isCloudForcePush, bool isCompensatedTask, CloudWaterType mode)
{
    std::string sql = GetRelationalCloudSyncDataQueryHeader(fields);
    AppendCloudQuery(isCloudForcePush, isCompensatedTask, sql, mode);
    AppendCloudQueryToGetDiffData(sql, mode);
    return sql;
}

std::string SqliteQueryHelper::GetCountRelationalCloudQuerySql(bool isCloudForcePush, bool isCompensatedTask,
    CloudWaterType mode)
{
    std::string sql = "SELECT COUNT(*) ";
    AppendCloudQuery(isCloudForcePush, isCompensatedTask, sql, mode);
    return sql;
}

std::string SqliteQueryHelper::GetGidRelationalCloudQuerySql(const std::vector<Field> &fields,
    bool isCloudForcePush, bool isCompensatedTask)
{
    std::string sql = GetRelationalCloudSyncDataQueryHeader(fields);
    AppendCloudGidQuery(isCloudForcePush, isCompensatedTask, sql);
    return sql;
}

void SqliteQueryHelper::AppendCloudQuery(bool isCloudForcePush, bool isCompensatedTask, std::string &sql,
    CloudWaterType mode)
{
    sql += CloudStorageUtils::GetLeftJoinLogSql(tableName_, false);
    sql += " WHERE ";
    // let data after remove device data at flag_only and logic delete mode and deleted by others to upload to cloud.
    if (mode == CloudWaterType::INSERT) {
        sql += "(b.cloud_gid == '' and (b.flag & 0x20 != 0) and (b.flag & 0x02 = 0) and (b.flag & 0x08 != 0x08) and";
        sql += " (b.flag & 0x01 = 0) and (b.status = 0)) OR ";
    }
    if (mode == CloudWaterType::DELETE && isCompensatedTask) {
        // deleted data does not have primary key, requires gid to compensate sync
        sql += "(b.status = 1 AND (b.flag & 0x01 = 0x01) AND b.cloud_gid != '') OR ";
    }
    if (mode == CloudWaterType::DELETE || mode == CloudWaterType::UPDATE) {
        if (queryObjNodes_.empty() && isCompensatedTask) {
            sql += "0 ";
            return;
        }
        if (!isCompensatedTask) {
            sql += "(b.status != 1) AND ";
        }
    }
    if (isCloudForcePush) {
        sql += " (b.flag & 0x04 != 0x04)";
    } else {
        sql += "(b.flag & 0x02 = 0x02)";
        if (!isCompensatedTask) {
            // local data and flag is not upload finished.
            sql += " AND (b.flag & 0x400 != 0x400)";
        }
    }
    sql += " AND (b.flag & 0x08 != 0x08) AND (b.cloud_gid != '' or"; // actually, b.cloud_gid will not be null.
    sql += " (b.cloud_gid == '' and (b.flag & 0x01 = 0))) ";
}

void SqliteQueryHelper::AppendCloudQueryToGetDiffData(std::string &sql, const CloudWaterType mode, bool isKv)
{
    switch (mode) {
        case DistributedDB::CloudWaterType::DELETE:
            sql += isKv ? " AND (flag & 0x01 == 0x01) " :
                " AND (b.flag & 0x01 == 0x01) ";
            break;
        case DistributedDB::CloudWaterType::UPDATE:
            sql += isKv ? " AND (flag & 0x01 == 0 AND cloud_gid != '' AND cloud_gid IS NOT NULL) " :
                " AND (b.flag & 0x01 == 0 AND b.cloud_gid != '') ";
            break;
        case DistributedDB::CloudWaterType::INSERT:
            sql += isKv ? " AND (flag & 0x01 == 0 AND (cloud_gid == '' OR cloud_gid IS NULL)) " :
                " AND (b.flag & 0x01 == 0 AND b.cloud_gid == '') ";
            break;
        default:
            break;
    }
}

void SqliteQueryHelper::AppendCloudGidQuery(bool isCloudForcePush, bool isCompensatedTask, std::string &sql)
{
    sql += CloudStorageUtils::GetLeftJoinLogSql(tableName_, false);
    sql += " WHERE ";
    if (isCompensatedTask) {
        // deleted data does not have primary key, requires gid to compensate sync
        sql += "(b.status = 1 AND (b.flag & 0x01 = 0x01)) ";
        if (queryObjNodes_.empty()) {
            return;
        }
        sql += "OR ";
    }
    // actually, b.cloud_gid will not be null.
    sql += isCloudForcePush ? " (b.flag & 0x04 != 0x04) AND (b.cloud_gid != '') " : " (b.cloud_gid != '') ";
}

int SqliteQueryHelper::GetCloudQueryStatement(bool useTimestampAlias, sqlite3 *dbHandle, std::string &sql,
    sqlite3_stmt *&statement)
{
    querySql_.clear(); // clear local query sql format
    int errCode = ToQuerySyncSql(false, useTimestampAlias);
    if (errCode != E_OK) {
        LOGE("To query sql fail! errCode[%d]", errCode);
        return errCode;
    }
    sql += querySql_;
    errCode = SQLiteUtils::GetStatement(dbHandle, sql, statement);
    if (errCode != E_OK) {
        LOGE("[Query] Get statement fail!");
        return -E_INVALID_QUERY_FORMAT;
    }
    int index = 1;
    errCode = BindObjNodes(statement, index);
    if (errCode != E_OK) {
        LOGE("[Query] BindObjNodes failed %d", errCode);
        int resetRet = E_OK;
        SQLiteUtils::ResetStatement(statement, true, resetRet);
        if (resetRet != E_OK) {
            LOGW("[Query] reset statement failed %d", resetRet);
        }
    }
    return errCode;
}

std::pair<int, sqlite3_stmt *> SqliteQueryHelper::GetKvCloudQueryStmt(sqlite3 *db, bool forcePush,
    const CloudWaterType mode, int64_t timeStamp, const std::string &user)
{
    std::pair<int, sqlite3_stmt *> res;
    sqlite3_stmt *&stmt = res.second;
    int &errCode = res.first;
    std::string sql = GetKvCloudQuerySql(false, forcePush);
    AppendCloudQueryToGetDiffData(sql, mode, true);
    AppendKvQueryObjectOnSql(sql);
    sql += "order by modify_time asc";
    errCode = SQLiteUtils::GetStatement(db, sql, stmt);
    if (errCode != E_OK) {
        LOGE("[SqliteQueryHelper] Get kv cloud query stmt failed %d", errCode);
        return res;
    }
    int ret = E_OK;
    errCode = SQLiteUtils::BindTextToStatement(stmt, BIND_CLOUD_USER, user);
    if (errCode != E_OK) {
        SQLiteUtils::ResetStatement(stmt, true, ret);
        LOGE("[SqliteQueryHelper] Bind user failed %d reset %d", errCode, ret);
        return res;
    }
    errCode = SQLiteUtils::BindInt64ToStatement(stmt, BIND_CLOUD_TIMESTAMP, timeStamp);
    if (errCode != E_OK) {
        SQLiteUtils::ResetStatement(stmt, true, ret);
        LOGE("[SqliteQueryHelper] Bind begin time failed %d reset %d", errCode, ret);
        return res;
    }
    int index = BIND_CLOUD_TIMESTAMP + 1;
    errCode = BindKeysToStmt(keys_, stmt, index);
    if (errCode != E_OK) {
        SQLiteUtils::ResetStatement(stmt, true, ret);
        LOGE("[SqliteQueryHelper] Bind user failed %d reset %d", errCode, ret);
    }
    return res;
}

std::string SqliteQueryHelper::GetKvCloudQuerySql(bool countOnly, bool forcePush)
{
    std::string sql;
    if (countOnly) {
        sql = QUERY_COUNT_HEAD;
    } else {
        sql = QUERY_CLOUD_SYNC_DATA_HEAD;
    }
    sql += QUERY_CLOUD_SYNC_DATA_DETAIL;
    sql += QUERY_CLOUD_SYNC_DATA_CONDITION;
    sql += " AND flag & 0x02 != 0 "; // get all data which is local
    if (forcePush) {
        sql += " AND flag & 0x04 != 0x04 "; // get all data which hasn't pushed
    } else {
        sql += " AND (cloud_flag is null OR cloud_flag & " +
            std::to_string(static_cast<uint32_t>(LogInfoFlag::FLAG_UPLOAD_FINISHED)) + " = 0) ";
    }
    return sql;
}

std::string SqliteQueryHelper::GetKvCloudRecordSql()
{
    std::string sql = QUERY_CLOUD_SYNC_DATA_HEAD;
    sql += QUERY_CLOUD_SYNC_DATA_DETAIL;
    sql += QUERY_CLOUD_VERSION_RECORD_CONDITION;
    return sql;
}

std::string SqliteQueryHelper::GetCloudVersionRecordSql(bool isDeviceEmpty)
{
    std::string sql = QUERY_CLOUD_VERSION_RECORD_SQL_HEAD;
    if (!isDeviceEmpty) {
        sql += QUERY_CLOUD_VERSION_RECORD_SQL_DEVICE_CONDITION;
    } else {
        sql += QUERY_CLOUD_VERSION_RECORD_SQL_EMPTY_DEVICE_CONDITION;
    }
    return sql;
}

int SqliteQueryHelper::GetAndBindGidKvCloudQueryStatement(const std::string &user, sqlite3 *dbHandle,
    sqlite3_stmt *&stmt)
{
    std::string sql = SELECT_CLOUD_GID_SQL;
    AppendKvQueryObjectOnSql(sql);
    int errCode = SQLiteUtils::GetStatement(dbHandle, sql, stmt);
    if (errCode != E_OK) {
        LOGE("[SqliteQueryHelper] Get gid stmt failed %d", errCode);
        return errCode;
    }
    int index = 1;
    int ret = E_OK;
    errCode = SQLiteUtils::BindTextToStatement(stmt, index++, user);
    if (errCode != E_OK) {
        SQLiteUtils::ResetStatement(stmt, true, ret);
        LOGE("[SqliteQueryHelper] Bind user failed %d when query gid", errCode);
        return errCode;
    }
    errCode = BindKeysToStmt(keys_, stmt, index);
    if (errCode != E_OK) {
        LOGE("[SqliteQueryHelper] Bind keys to query gid stmt failed %d", errCode);
        SQLiteUtils::ResetStatement(stmt, true, ret);
    }
    return errCode;
}

int SqliteQueryHelper::GetCountKvCloudDataStatement(sqlite3 *db, bool forcePush, const CloudWaterType mode,
    sqlite3_stmt *&stmt)
{
    std::string sql = SqliteQueryHelper::GetKvCloudQuerySql(true, forcePush);
    SqliteQueryHelper::AppendCloudQueryToGetDiffData(sql, mode, true);
    AppendKvQueryObjectOnSql(sql);
    int errCode = SQLiteUtils::GetStatement(db, sql, stmt);
    if (errCode != E_OK) {
        LOGE("[SqliteQueryHelper] Count data stmt failed %d", errCode);
    }
    return errCode;
}

std::pair<int, int64_t> SqliteQueryHelper::BindCountKvCloudDataStatement(sqlite3 *db, bool isMemory,
    const Timestamp &timestamp, const std::string &user, sqlite3_stmt *&stmt)
{
    ResFinalizer finalizer([stmt]() {
        sqlite3_stmt *statement = stmt;
        int ret = E_OK;
        SQLiteUtils::ResetStatement(statement, true, ret);
        if (ret != E_OK) {
            LOGW("[SqliteCloudKvExecutorUtils] Reset log stmt failed %d when get upload count", ret);
        }
    });
    std::pair<int, int64_t> res = { E_OK, 0 };
    auto &[errCode, count] = res;
    errCode = SQLiteUtils::BindTextToStatement(stmt, BIND_CLOUD_USER, user);
    if (errCode != E_OK) {
        LOGE("[SqliteQueryHelper] Bind user failed %d when get upload count", errCode);
        return res;
    }
    errCode = SQLiteUtils::BindInt64ToStatement(stmt, BIND_CLOUD_TIMESTAMP, static_cast<int64_t>(timestamp));
    if (errCode != E_OK) {
        LOGE("[SqliteQueryHelper] Bind begin time failed %d when get upload count", errCode);
        return res;
    }
    int keysIndex = BIND_CLOUD_TIMESTAMP + 1;
    errCode = BindKeysToStmt(keys_, stmt, keysIndex);
    if (errCode != E_OK) {
        LOGE("[SqliteQueryHelper] Bind keys failed %d when get upload count", errCode);
        return res;
    }
    errCode = SQLiteUtils::StepNext(stmt, isMemory);
    if (errCode == -E_FINISHED) {
        count = 0;
        return res;
    }
    count = sqlite3_column_int64(stmt, CLOUD_QUERY_COUNT_INDEX);
    LOGD("[SqliteCloudKvExecutorUtils] Get total upload count %" PRId64, count);
    return res;
}

void SqliteQueryHelper::AppendKvQueryObjectOnSql(std::string &sql)
{
    if (!keys_.empty()) {
        sql += " AND " + MapKeysInToSql(keys_.size());
    }
}
}