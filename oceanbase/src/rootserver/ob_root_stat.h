/**
 * (C) 2010-2011 Alibaba Group Holding Limited.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 * 
 * Version: $Id$
 *
 * ob_root_stat.h for ...
 *
 * Authors:
 *   qushan <qushan@taobao.com>
 *
 */

#ifndef OCEANBASE_ROOTSERVER_OB_ROOT_STAT_H_
#define OCEANBASE_ROOTSERVER_OB_ROOT_STAT_H_
#include "common/ob_statistics.h"
namespace oceanbase
{
  namespace rootserver
  {
    class ObRootStatManager :public common::ObStatManager
    {
      public:
        enum
        {
          INDEX_SUCCESS_GET_COUNT = 0,
          INDEX_SUCCESS_SCAN_COUNT = 1,
          INDEX_FAIL_GET_COUNT = 2,
          INDEX_FAIL_SCAN_COUNT = 3,
          INDEX_GET_OBI_ROLE_COUNT = 4,
          INDEX_MIGRATE_COUNT = 5,
          INDEX_COPY_COUNT = 6,
        };
        enum
        {
          ROOT_TABLE_ID = 1,
        };
        ObRootStatManager() :ObStatManager(common::ObStatManager::SERVER_TYPE_ROOT) {}
    };
  }
}
#endif

