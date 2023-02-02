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

#ifndef DISTRIBUTED_RDB_SERVICE_H
#define DISTRIBUTED_RDB_SERVICE_H

#include <memory>
#include <string>
#include "iremote_broker.h"
#include "iremote_object.h"
#include "rdb_types.h"
#include "visibility.h"

namespace OHOS {
namespace DistributedRdb {
class API_EXPORT RdbService {
public:
    enum {
        RDB_SERVICE_CMD_OBTAIN_TABLE,
        RDB_SERVICE_CMD_INIT_NOTIFIER,
        RDB_SERVICE_CMD_SET_DIST_TABLE,
        RDB_SERVICE_CMD_SYNC,
        RDB_SERVICE_CMD_ASYNC,
        RDB_SERVICE_CMD_SUBSCRIBE,
        RDB_SERVICE_CMD_UNSUBSCRIBE,
        RDB_SERVICE_CMD_REMOTE_QUERY,
        RDB_SERVICE_CREATE_RDB_TABLE,
        RDB_SERVICE_DESTROY_RDB_TABLE,
        RDB_SERVICE_CMD_MAX
    };
    DECLARE_INTERFACE_DESCRIPTOR(u"OHOS.DistributedRdb.IRdbService");
    virtual std::string ObtainDistributedTableName(const std::string &device, const std::string &table) = 0;

    virtual int32_t SetDistributedTables(const RdbSyncerParam &param, const std::vector<std::string> &tables) = 0;

    virtual int32_t Sync(const RdbSyncerParam &param, const SyncOption &option, const RdbPredicates &predicates,
        const SyncCallback &callback) = 0;

    virtual int32_t Subscribe(const RdbSyncerParam &param, const SubscribeOption &option,
        RdbStoreObserver *observer) = 0;

    virtual int32_t UnSubscribe(const RdbSyncerParam &param, const SubscribeOption &option,
        RdbStoreObserver *observer) = 0;

    virtual int32_t RemoteQuery(const RdbSyncerParam &param, const std::string &device, const std::string &sql,
        const std::vector<std::string> &selectionArgs, sptr<IRemoteObject> &resultSet) = 0;
    virtual int32_t CreateRDBTable(const RdbSyncerParam &param, const std::string &writePermission,
        const std::string &readPermission) = 0;

    virtual int32_t DestroyRDBTable(const RdbSyncerParam &param) = 0;

    virtual int32_t InitNotifier(const RdbSyncerParam &param, const sptr<IRemoteObject> notifier) = 0;

protected:
    virtual int32_t DoSync(const RdbSyncerParam &param, const SyncOption &option, const RdbPredicates &predicates,
        SyncResult &result) = 0;

    virtual int32_t DoAsync(const RdbSyncerParam &param, uint32_t seqNum, const SyncOption &option,
        const RdbPredicates &predicates) = 0;

    virtual int32_t DoSubscribe(const RdbSyncerParam &param) = 0;

    virtual int32_t DoUnSubscribe(const RdbSyncerParam &param) = 0;
};
}
} // namespace OHOS::DistributedRdb
#endif
