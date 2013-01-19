/**
 * (C) 2010-2011 Alibaba Group Holding Limited.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 * 
 * Version: $Id$
 *
 * ob_trace_log.h for ...
 *
 * Authors:
 *   yubai <yubai.lk@taobao.com>
 *
 */
#ifndef  OCEANBASE_COMMON_FILL_LOG_H_
#define  OCEANBASE_COMMON_FILL_LOG_H_
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <pthread.h>
#include <new>
#include <algorithm>
#include "ob_define.h"

#define INC_TRACE_LOG_LEVEL() oceanbase::common::TraceLog::inc_log_level()
#define DEC_TRACE_LOG_LEVEL() oceanbase::common::TraceLog::dec_log_level()
#define SET_TRACE_LOG_LEVEL(level) oceanbase::common::TraceLog::set_log_level(level)
#define CLEAR_TRACE_LOG() oceanbase::common::TraceLog::clear_log()
#define FILL_TRACE_LOG(_fmt_, args...) oceanbase::common::TraceLog::fill_log("f=[%s] " _fmt_, __FUNCTION__, ##args)
#define PRINT_TRACE_LOG() \
  { \
    TBSYS_LOGGER.logMessage(TBSYS_LOG_NUM_LEVEL(oceanbase::common::TraceLog::get_log_level()), "[%ld][%ld][%ld] %stotal_timeu=%ld", \
                            pthread_self(), tbsys::CLogger::get_cur_tv().tv_sec, tbsys::CLogger::get_cur_tv().tv_usec, \
                            oceanbase::common::TraceLog::get_logbuffer().buffer, \
                            tbsys::CTimeUtil::getTime() - oceanbase::common::TraceLog::get_logbuffer().start_timestamp); \
    CLEAR_TRACE_LOG(); \
  }
#define GET_TRACE_TIMEU() (tbsys::CTimeUtil::getTime() - oceanbase::common::TraceLog::get_logbuffer().start_timestamp)
#define TBSYS_TRACE_LOG(_fmt_, args...) \
    TBSYS_LOGGER.logMessage(TBSYS_LOG_NUM_LEVEL(oceanbase::common::TraceLog::get_log_level()), "[%ld][%ld][%ld] " _fmt_, \
                            pthread_self(), tbsys::CLogger::get_cur_tv().tv_sec, tbsys::CLogger::get_cur_tv().tv_usec, ##args)

namespace oceanbase
{
  namespace common
  {
    class TraceLog
    {
      static const char *const LOG_LEVEL_ENV_KEY;
      struct LogBuffer
      {
        static const int64_t LOG_BUFFER_SIZE = 4 * 1024;
        int64_t cur_pos;
        int64_t prev_timestamp;
        int64_t start_timestamp;
        char buffer[LOG_BUFFER_SIZE];
      };
      public:
        static void clear_log();
        static void fill_log(const char *fmt, ...);
        static void print_log();
        static LogBuffer &get_logbuffer();
        static int set_log_level(const char *log_level_str);
        static int get_log_level();
        static int dec_log_level();
        static int inc_log_level();
      private:
        static const char *const level_strs_[];
        static volatile int log_level_;
        static bool got_env_;
    };
  }
}

#endif //OCEANBASE_COMMON_FILL_LOG_H_


