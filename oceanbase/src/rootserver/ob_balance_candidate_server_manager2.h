/**
 * (C) 2010-2011 Alibaba Group Holding Limited.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 * 
 * Version: $Id$
 *
 * ob_balance_candidate_server_manager2.h for ...
 *
 * Authors:
 *   qushan <qushan@taobao.com>
 *
 */

#ifndef OCEANBASE_ROOTSERVER_OB_CANDIDATE_SERVER_MANAGER2_H_
#define OCEANBASE_ROOTSERVER_OB_CANDIDATE_SERVER_MANAGER2_H_
#include "common/ob_server.h"
#include "common/ob_array_helper.h"
#include "rootserver/ob_chunk_server_manager.h"
#include "rootserver/ob_root_table2.h"
namespace oceanbase
{
  namespace rootserver
  {
    //we will use this class to find the top CANDIDATE_SERVER_COUNT cs who has the most free disk
    class ObCandidateServerByDiskManager
    {
      public:
        enum 
        {
          CANDIDATE_SERVER_COUNT = 3,
        };
        ObCandidateServerByDiskManager();
        void reset();
        int add_server(ObServerStatus* server_status);
        int32_t get_length() const;
        ObServerStatus* get_server(const int32_t index);

      private:
        void insert(const int32_t index, ObServerStatus* server_status);

        ObServerStatus* data_holder[CANDIDATE_SERVER_COUNT];
        int32_t length_;
    };

    
    //we will use this class to caculate the count of shared tablets between cs
    class ObCandidateServerBySharedManager2
    {
      public:
        enum 
        {
          CANDIDATE_SERVER_COUNT = ObChunkServerManager::MAX_SERVER_COUNT,
        };
        struct effectiveServer
        {
          effectiveServer();
          int32_t server_indexes_[common::OB_SAFE_COPY_COUNT];
          bool is_not_in(int32_t server_index) const;
        };
        struct sharedInfo
        {
          sharedInfo();
          int32_t server_index_;
          int32_t shared_count_;
          bool operator < (const sharedInfo& rv) const;
        };
        ObCandidateServerBySharedManager2();
        void set_effective_server(const effectiveServer& effective_server);
        void init(const effectiveServer& effective_server, const ObChunkServerManager* server_manager);
        void sort();
        void scan_root_meta(ObRootTable2::const_iterator it);
        void scan_root_table(ObRootTable2* roor_table);

        const sharedInfo* begin() const;
        const sharedInfo* end() const;
        sharedInfo* begin() ;
        sharedInfo* end() ;
        sharedInfo* find(int32_t server_index);

      private:
        effectiveServer effective_server_;
        sharedInfo data_holder_[CANDIDATE_SERVER_COUNT];
        common::ObArrayHelper<sharedInfo> shared_infos_;

    };
  }
}
#endif

