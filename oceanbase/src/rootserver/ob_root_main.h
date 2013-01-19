/**
 * (C) 2010-2011 Alibaba Group Holding Limited.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 * 
 * Version: $Id$
 *
 * ob_root_main.h for ...
 *
 * Authors:
 *   qushan <qushan@taobao.com>
 *
 */

#ifndef OCEANBASE_ROOTSERVER_OB_ROOT_MAIN_H_
#define OCEANBASE_ROOTSERVER_OB_ROOT_MAIN_H_
#include "common/ob_packet_factory.h"
#include "common/base_main.h"
#include "common/ob_define.h"
#include "rootserver/ob_root_worker.h"
namespace oceanbase 
{ 
namespace rootserver
{ 
class ObRootMain : public common::BaseMain
{
public:
  static common::BaseMain* get_instance();
  int do_work();
  void do_signal(const int sig);
private:
  virtual void print_version();
  ObRootMain();
  OBRootWorker worker;
  common::ObPacketFactory packet_factory_;
};
}
}
#endif

