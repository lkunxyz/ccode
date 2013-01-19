/**
 * (C) 2010-2011 Alibaba Group Holding Limited.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 * 
 * Version: $Id$
 *
 * ob_root_fetch_thread.h for ...
 *
 * Authors:
 *   qushan <qushan@taobao.com>
 *
 */
#ifndef OCEANBASE_ROOTSERVER_FETCH_THREAD_H_
#define OCEANBASE_ROOTSERVER_FETCH_THREAD_H_

#include "common/ob_fetch_runnable.h"
#include "rootserver/ob_root_log_manager.h"

namespace oceanbase
{
  namespace rootserver
  {
    class ObRootFetchThread : public common::ObFetchRunnable
    {
      const static int64_t WAIT_INTERVAL;

      public:
        ObRootFetchThread();

        int wait_recover_done();

        int got_ckpt(uint64_t ckpt_id);

        void set_log_manager(ObRootLogManager* log_manager);
    
      private:
        int recover_ret_;
        bool is_recover_done_;
        ObRootLogManager* log_manager_;
    };
  } /* rootserver */
} /* oceanbase */

#endif /* end of include guard: OCEANBASE_ROOTSERVER_FETCH_THREAD_H_ */
#include "common/ob_fetch_runnable.h"


