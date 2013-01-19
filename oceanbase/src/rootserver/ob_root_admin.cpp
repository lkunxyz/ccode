/**
 * (C) 2010-2011 Alibaba Group Holding Limited.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 * 
 * Version: $Id$
 *
 * ob_root_admin.cpp for ...
 *
 * Authors:
 *   qushan <qushan@taobao.com>
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "rootserver/ob_root_signal.h"
int main (int argc, char *argv[])
{
  if (argc != 3) 
  {
    fprintf(stderr, "usage:\n"
        "    %s pid command\n\n"
        "command :\n"
        "   log_move_to_debug;\n"
        "   log_move_to_error;\n"
        "   start_merge;\n"
        "   dump_root_table;\n"
        "   dump_server_info;\n"
        "   switch_schema;\n"
        "   reload_config;\n"
        "   do_check_point;\n"
        ,argv[0]);

  }
  else
  {
    char cmd[200];
    int sig = 0;
    if (strcmp(argv[2], "log_move_to_debug") == 0)
    {
      sig = LOG_MOVE_TO_DEBUG;
    }else if (strcmp(argv[2], "log_move_to_error") == 0)
    {
      sig = LOG_MOVE_TO_ERROR;
    }else if (strcmp(argv[2], "start_merge") == 0)
    {
      sig = START_MERGE_SIG;
    }else if (strcmp(argv[2], "dump_root_table") == 0)
    {
      sig = DUMP_ROOT_TABLE_TO_LOG;
    }else if (strcmp(argv[2], "dump_server_info") == 0)
    {
      sig = DUMP_AVAILABLE_SEVER_TO_LOG;
    }else if (strcmp(argv[2], "switch_schema") == 0)
    {
      sig = SWITCH_SCHEMA;
    }else if (strcmp(argv[2], "reload_config") == 0)
    {
      sig = RELOAD_CONFIG;
    } else if (strcmp(argv[2], "do_check_point") == 0)
    {
      sig = DO_CHECK_POINT;
    }
    int pid = 0;
    pid =atoi(argv[1]);
    if (sig != 0 && pid != 0)
    {
      sprintf(cmd, "kill -%d %d",sig, pid);
      system(cmd);
      fprintf(stderr, "command %s was launched \n", cmd);
    }
  }
  return 0;
}

