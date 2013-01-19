/**
 * (C) 2010-2011 Alibaba Group Holding Limited.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 * 
 * Version: $Id$
 *
 * ob_groupby_operator.h for ...
 *
 * Authors:
 *   wushi <wushi.ly@taobao.com>
 *
 */
#ifndef MERGESERVER_OB_GROUPBY_OPERATOR_H_ 
#define MERGESERVER_OB_GROUPBY_OPERATOR_H_
#include "common/ob_cell_array.h"
#include "common/hash/ob_hashmap.h"
#include "common/ob_groupby.h"
namespace oceanbase
{
  namespace mergeserver
  {
    class ObGroupByOperator : public oceanbase::common::ObCellArray
    {
    public:
      ObGroupByOperator();
      ~ObGroupByOperator();

      void clear();

      int init(const oceanbase::common::ObGroupByParam & param, const int64_t max_avail_mem_size); 
      /// add an org row [row_beg,row_end]
      int add_row(const oceanbase::common::ObCellArray & org_cells, const int64_t row_beg, const int64_t row_end);
      int init_all_in_one_group_row();
    private:
      static const int64_t HASH_SLOT_NUM = 1024*16;
      const oceanbase::common::ObGroupByParam *param_;
      oceanbase::common::hash::ObHashMap<oceanbase::common::ObGroupKey,int64_t,oceanbase::common::hash::NoPthreadDefendMode> group_hash_map_;
      bool inited_;
      int64_t max_avail_mem_size_;
    };
  }
}  
#endif /* MERGESERVER_OB_GROUPBY_OPERATOR_H_ */


