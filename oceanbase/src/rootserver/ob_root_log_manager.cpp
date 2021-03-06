/**
 * (C) 2010-2011 Alibaba Group Holding Limited.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 * 
 * Version: $Id$
 *
 * ob_root_log_manager.cpp for ...
 *
 * Authors:
 *   qushan <qushan@taobao.com>
 *
 */
#include "common/ob_define.h"
#include "common/ob_log_dir_scanner.h"
#include "common/file_utils.h"
#include "common/ob_log_reader.h"
#include "common/file_directory_utils.h"
#include "common/ob_log_entry.h"
#include "ob_root_log_manager.h"

namespace
{
  const int64_t DEFAULT_LOG_SIZE_MB = 64; // 64MB
  const int64_t DEFAULT_LOG_SYNC_TYPE = 0;

  const char* STR_ROOT_SECTION = "root_server";
  const char* OBRT_LOG_DIR_PATH = "log_dir_path";
  const char* OBRT_LOG_SIZE_MB = "log_size_mb";
  const char* OBRT_LOG_SYNC_TYPE = "log_sync_type";
}

namespace oceanbase
{
  namespace rootserver
  {
    using namespace common;
    const int ObRootLogManager::UINT64_MAX_LEN = 32;
    ObRootLogManager::ObRootLogManager() : ckpt_id_(0), replay_point_(0), max_log_id_(0), rt_server_status_(0), is_initialized_(false), is_log_dir_empty_(false)
    {
    }

    ObRootLogManager::~ObRootLogManager()
    {
    }

    int ObRootLogManager::init(ObRootServer2* root_server, common::ObSlaveMgr* slave_mgr)
    {
      int ret = OB_SUCCESS;

      if (is_initialized_)
      {
        TBSYS_LOG(ERROR, "rootserver's log manager has been initialized");
        ret = OB_INIT_TWICE;
      }

      root_server_ = root_server;
      log_worker_.set_root_server(root_server_);
      log_worker_.set_log_manager(this);

      const char* log_dir = TBSYS_CONFIG.getString(STR_ROOT_SECTION, OBRT_LOG_DIR_PATH);
      if (OB_SUCCESS == ret)
      {
        if (NULL == log_dir)
        {
          TBSYS_LOG(ERROR, "log directory is null, section: %s, name: %s", STR_ROOT_SECTION, OBRT_LOG_DIR_PATH);
          ret = OB_INVALID_ARGUMENT;
        }
        else
        {
          int log_dir_len = strlen(log_dir);
          if (log_dir_len >= OB_MAX_FILE_NAME_LENGTH)
          {
            TBSYS_LOG(ERROR, "Argument is invalid[log_dir_len=%d log_dir=%s]", log_dir_len, log_dir);
            ret = OB_INVALID_ARGUMENT;
          }
          else
          {
            strncpy(log_dir_, log_dir, log_dir_len);
            log_dir_[log_dir_len] = '\0';
          }
        }
      }

      if (OB_SUCCESS == ret)
      {
        ObLogDirScanner scanner;

        ret = scanner.init(log_dir_);
        if (OB_SUCCESS != ret && OB_DISCONTINUOUS_LOG != ret)
        {
          TBSYS_LOG(ERROR, "ObLogDirScanner init error");
        }
        else
        {
          if (!scanner.has_ckpt())
          {
            // no check point
            replay_point_ = 1;
            if (!scanner.has_log())
            {
              max_log_id_ = 1;
              is_log_dir_empty_ = true;
            }
            else
            {
              ret = scanner.get_max_log_id(max_log_id_);
              if (OB_SUCCESS != ret)
              {
                TBSYS_LOG(ERROR, "ObLogDirScanner get_max_log_id error[ret=%d]", ret);
              }
            }
            if (OB_SUCCESS == ret)
            {
              TBSYS_LOG(INFO, "check point does not exist, take replay_point[replay_point_=%lu][ckpt_id=%lu]", replay_point_, ckpt_id_);
            }
          }
          else
          {
            // has checkpoint
            uint64_t min_log_id;

            ret = scanner.get_max_ckpt_id(ckpt_id_);

            if (ret == OB_SUCCESS)
            {
              ret = scanner.get_min_log_id(min_log_id);
            }

            if (OB_SUCCESS != ret)
            {
              TBSYS_LOG(ERROR, "get_min_log_id error[ret=%d]", ret);
              ret = OB_ERROR;
            }
            else
            {
              if (min_log_id > ckpt_id_ + 1)
              {
                TBSYS_LOG(ERROR, "missing log file[min_log_id=%lu ckeck_point=%lu", min_log_id, ckpt_id_);
                ret = OB_ERROR;
              }
              else
              {
                replay_point_ = ckpt_id_ + 1; // replay from next log file
              }
            }

            if (OB_SUCCESS == ret)
            {
              ret = scanner.get_max_log_id(max_log_id_);
              if (OB_SUCCESS != ret)
              {
                TBSYS_LOG(ERROR, "get_max_log_id error[ret=%d]", ret);
                ret = OB_ERROR;
              }
            }
          }
        }
      }

      if (OB_SUCCESS == ret)
      {
        int64_t log_file_max_size = TBSYS_CONFIG.getInt(STR_ROOT_SECTION, OBRT_LOG_SIZE_MB, DEFAULT_LOG_SIZE_MB) * 1000 * 1000;
        int64_t log_sync_type = TBSYS_CONFIG.getInt(STR_ROOT_SECTION, OBRT_LOG_SYNC_TYPE, DEFAULT_LOG_SYNC_TYPE);

        ret = ObLogWriter::init(log_dir, log_file_max_size, slave_mgr, log_sync_type);
        if (OB_SUCCESS != ret)
        {
          TBSYS_LOG(ERROR, "ObLogWriter init failed[ret=%d]", ret);
        }
        else
        {
          is_initialized_ = true;
          TBSYS_LOG(INFO, "ObRootLogMgr[this=%p] init succ[replay_point_=%lu max_log_id_=%lu]", this, replay_point_, max_log_id_);
        }
      }

      return ret;
    }

    int ObRootLogManager::add_slave(const common::ObServer& server, uint64_t &new_log_file_id)
    {
      int ret = OB_SUCCESS;

      ObSlaveMgr* slave_mgr = get_slave_mgr();
      if (slave_mgr == NULL)
      {
        TBSYS_LOG(ERROR, "slave manager is NULL");
        ret = OB_ERROR;
      }
      else
      {
        ret = slave_mgr->add_server(server);
        if (ret != OB_SUCCESS)
        {
          TBSYS_LOG(ERROR, "add slave manager failed [ret=%d]", ret);
        }
        else
        {
          ret = switch_log_file(new_log_file_id);
          if (ret != OB_SUCCESS)
          {
            TBSYS_LOG(ERROR, "switch log file error [ret=%d]", ret);
          }
        }
      }

      return ret;
    }

    int ObRootLogManager::do_after_recover_check_point()
    {
      TBSYS_LOG(INFO, "[NOTICE] after recovery checkpointing");
      root_server_->start_threads();
      root_server_->wait_init_finished();
      TBSYS_LOG(INFO, "[NOTICE] background threads started and finished init");
      return OB_SUCCESS;
    }

    // called by start_as_master
    int ObRootLogManager::replay_log()
    {
      int ret = OB_SUCCESS;

      ObLogReader log_reader;

      char* log_data = NULL;
      int64_t log_length = 0;
      LogCommand cmd = OB_LOG_UNKNOWN;
      uint64_t seq = 0;

      TBSYS_LOG(INFO, "begin replay log");
      ret = check_inner_stat();

      if (ret == OB_SUCCESS && !is_log_dir_empty_)
      {
        if (ckpt_id_ > 0)
        {
          ret = load_server_status();
          if (ret == OB_SUCCESS)
          {
            ret = root_server_->recover_from_check_point(rt_server_status_, ckpt_id_);
            if (ret != OB_SUCCESS)
            {
              TBSYS_LOG(ERROR, "recover from check point failed, err=%d", ret);
            }
          }
          else
          {
            TBSYS_LOG(ERROR, "load server status failed, so recover from check point failed");
          }
        }
        if (ret == OB_SUCCESS)
        {
          ret = do_after_recover_check_point();
        }
        if (ret == OB_SUCCESS)
        {
          ret = log_reader.init(log_dir_, replay_point_, 0, false);
          if (ret != OB_SUCCESS)
          {
            TBSYS_LOG(ERROR, "ObLogReader init failed, ret=%d", ret);
          }
          else
          {
            ret = log_reader.read_log(cmd, seq, log_data, log_length);
            if (OB_FILE_NOT_EXIST != ret && OB_READ_NOTHING != ret && OB_SUCCESS != ret)
            {
              TBSYS_LOG(ERROR, "ObLogReader read error[ret=%d]", ret);
            }

            while (ret == OB_SUCCESS)
            {
              set_cur_log_seq(seq);

              if (OB_LOG_NOP != cmd)
              {
                ret = log_worker_.apply(cmd, log_data, log_length);

                if (ret != OB_SUCCESS)
                {
                  TBSYS_LOG(ERROR, "apply log failed, command: %d", cmd);
                }
              }

              ret = log_reader.read_log(cmd, seq, log_data, log_length);
              if (OB_FILE_NOT_EXIST != ret && OB_READ_NOTHING != ret && OB_SUCCESS != ret)
              {
                TBSYS_LOG(ERROR, "ObLogReader read error[ret=%d]", ret);
              }
            } // end while

            // handle exception, when the last log file contain SWITCH_LOG entry
            // but the next log file is missing
            if (OB_FILE_NOT_EXIST == ret)
            {
              max_log_id_++;
              ret = OB_SUCCESS;
            }

            // reach the end of commit log
            if (OB_READ_NOTHING == ret)
            {
              ret = OB_SUCCESS;
            }
          }
        }
      }
      else if(ret == OB_SUCCESS)
      {
        ret = do_after_recover_check_point();
      }

      if (OB_SUCCESS == ret)
      {
        ret = start_log(max_log_id_, get_cur_log_seq());
        if (OB_SUCCESS != ret)
        {
          TBSYS_LOG(ERROR, "ObLogWriter start_log[max_log_id_=%lu get_cur_log_seq()=%lu", max_log_id_, get_cur_log_seq());
        }
        else
        {
          TBSYS_LOG(INFO, "start log [%d] done, cur_log_seq=%lu", max_log_id_, get_cur_log_seq());
        }
      }
      return ret;
    }

    // called by slave's fetch_thread when downloaded the remote ckpt file
    int ObRootLogManager::recover_checkpoint(const uint64_t checkpoint_id)
    {
      int ret = OB_SUCCESS;

      ckpt_id_ = checkpoint_id;

      if (ckpt_id_ > 0)
      {
        ret = load_server_status();
        if (ret == OB_SUCCESS)
        {
          ret = root_server_->recover_from_check_point(rt_server_status_, ckpt_id_);
          if (ret != OB_SUCCESS)
          {
            TBSYS_LOG(ERROR, "recover from check point [%d] failed, err=%d", ckpt_id_, ret);
          }
          else
          {
            TBSYS_LOG(INFO, "recover from check point [%d] done", ckpt_id_);
          }
        }
        else
        {
          TBSYS_LOG(ERROR, "load server status failed, so recover from check point failed");
        }
      }

      return ret;
    }

    int ObRootLogManager::do_check_point(const uint64_t checkpoint_id/* = 0 */)
    {
      // do check point
      int ret = OB_SUCCESS;

      uint64_t ckpt_id = checkpoint_id;
      if (ckpt_id == 0) {
        ret = write_checkpoint_log(ckpt_id);
      }

      if (ret == OB_SUCCESS)
      {
        // write server status into default checkpoint file
        char filename[OB_MAX_FILE_NAME_LENGTH];
        int err = 0;
        FileUtils ckpt_file;
        err = snprintf(filename, OB_MAX_FILE_NAME_LENGTH, "%s/%lu.%s", log_dir_, ckpt_id, DEFAULT_CKPT_EXTENSION);
        if (err < 0 || err >= OB_MAX_FILE_NAME_LENGTH)
        {
          TBSYS_LOG(ERROR, "generate check point file name failed, error: %s", strerror(errno));
          ret = OB_ERROR;
        }
        else
        {
          err = ckpt_file.open(filename, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH); 
          if (err < 0)
          {
            TBSYS_LOG(ERROR, "open check point file[%s] failed: %s", filename, strerror(errno));
            ret = OB_ERROR;
          }
          else
          {
            char st_str[UINT64_MAX_LEN];
            int st_str_len = 0;
            int server_status = root_server_->get_server_status();
            st_str_len = snprintf(st_str, UINT64_MAX_LEN, "%d", server_status);
            if (st_str_len < 0)
            {
              TBSYS_LOG(ERROR, "snprintf st_str error[%s][server_status=%d]", strerror(errno), server_status);
              ret = OB_ERROR;
            }
            else if (st_str_len >= UINT64_MAX_LEN)
            {
              TBSYS_LOG(ERROR, "st_str is too long[st_str_len=%d UINT64_MAX_LEN=%d", st_str_len, UINT64_MAX_LEN);
              ret = OB_ERROR;
            }
            else
            {
              err = ckpt_file.write(st_str, st_str_len);
              if (err < 0)
              {
                TBSYS_LOG(ERROR, "write error[%s]", strerror(errno));
                ret = OB_ERROR;
              }
            }

            ckpt_file.close();
          }
        }
      }

      if (ret == OB_SUCCESS)
      {
        ret = root_server_->do_check_point(ckpt_id);
      }

      if (ret == OB_SUCCESS)
      {
        TBSYS_LOG(INFO, "do check point success, check point id: %lu => %lu", ckpt_id_, ckpt_id);
        ckpt_id_ = ckpt_id;
        replay_point_ = ckpt_id;
      }

      return ret;
    }

    int ObRootLogManager::load_server_status()
    {
      int ret = OB_SUCCESS;

      int err = 0;
      char st_str[UINT64_MAX_LEN];
      int st_str_len = 0;

      char ckpt_fn[OB_MAX_FILE_NAME_LENGTH];
      err = snprintf(ckpt_fn, OB_MAX_FILE_NAME_LENGTH, "%s/%lu.%s", log_dir_, ckpt_id_, DEFAULT_CKPT_EXTENSION);
      if (err < 0 || err >= OB_MAX_FILE_NAME_LENGTH)
      {
        TBSYS_LOG(ERROR, "generate check point file name [%s] failed, error: %s", ckpt_fn, strerror(errno));
        ret = OB_ERROR;
      }

      if (ret == OB_SUCCESS && !FileDirectoryUtils::exists(ckpt_fn))
      {
        TBSYS_LOG(INFO, "check point file[\"%s\"] does not exist", ckpt_fn);
        ret = OB_FILE_NOT_EXIST;
      }
      else
      {
        FileUtils ckpt_file;
        err = ckpt_file.open(ckpt_fn, O_RDONLY);
        if (err < 0)
        {
          TBSYS_LOG(ERROR, "open file[\"%s\"] error[%s]", ckpt_fn, strerror(errno));
          ret = OB_ERROR;
        }
        else
        {
          st_str_len = ckpt_file.read(st_str, UINT64_MAX_LEN);
          TBSYS_LOG(DEBUG, "st_str_len = %d", st_str_len);
          st_str[st_str_len] = '\0';
          if (st_str_len < 0)
          {
            TBSYS_LOG(ERROR, "read file error[%s]", strerror(errno));
            ret = OB_ERROR;
          }
          else if ((st_str_len >= UINT64_MAX_LEN) || (st_str_len == 0))
          {
            TBSYS_LOG(ERROR, "data contained in ckeck point file is invalid[ckpt_str_len=%d]", st_str_len);
            ret = OB_ERROR;
          }
          else
          {
            int rc = sscanf(st_str, "%d", &rt_server_status_);
            if (rc != 1)
            {
              TBSYS_LOG(ERROR, "server status load failed.");
              ret = OB_ERROR;
            }
          }
        }
      }

      if (OB_SUCCESS == ret)
      {
        TBSYS_LOG(INFO, "load server status succ[server_status=%d] from file[\"%s\"]", rt_server_status_, ckpt_fn);
      }

      return ret;
    }
  } /* rootserver */
} /* oceanbase */

