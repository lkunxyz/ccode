/*
 * Copyright (C) 2007-2011 Taobao Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Description here
 *
 * Version: $Id$
 *
 * Authors:
 *   Zhifeng YANG <zhuweng.yzf@taobao.com>
 *     - some work details here
 */

#include "rootserver/ob_root_stat_key.h"
#include <cstdlib>

const char* oceanbase::rootserver::OB_RS_STAT_KEYSTR[]= 
{
  "reserve",                    // 0
  "common",                     // 1
  "start_time",                 // 2
  "prog_version",               // 3
  "pid",                        // 4
  "local_time",                 // 5
  "mem",                        // 6
  "rs_status",                  // 7
  "frozen_version",             // 8
  "schema_version",             // 9
  "log_id",                     // 10
  "log_file_id",                // 11
  "table_num",                  // 12
  "tablet_num",                 // 13
  "replicas_num",               // 14
  "cs_num",                     // 15
  "ms_num",                     // 16
  "cs",                         // 17
  "ms",                         // 18
  "ups",                        // 19
  "rs_slave",                   // 20
  "ops_get",                    // 21
  "ops_scan",                   // 22
  "rs_slave_num",               // 23
  "frozen_time",                // 24
  "client_conf",                // 25
  "sstable_dist",               // 26
  NULL
};
