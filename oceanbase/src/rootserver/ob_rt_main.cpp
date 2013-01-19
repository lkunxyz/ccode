/**
 * (C) 2010-2011 Alibaba Group Holding Limited.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 * 
 * Version: $Id$
 *
 * ob_rt_main.cpp for ...
 *
 * Authors:
 *   daoan <daoan@taobao.com>
 *
 */

#include <malloc.h>
#include "rootserver/ob_root_main.h"
#include "common/ob_malloc.h"
using namespace oceanbase::rootserver;
using namespace oceanbase::common;

namespace  
{ 
  static const int DEFAULT_MMAP_MAX_VAL = 1024*1024*1024; 
}; 


int main(int argc, char *argv[])
{
  mallopt(M_MMAP_MAX, DEFAULT_MMAP_MAX_VAL);  
  ob_init_memory_pool();
  
  BaseMain* pmain = ObRootMain::get_instance();
  if (pmain == NULL){
    perror("not enought mem, exit \n");
  }
  else{
    pmain->start(argc, argv, "root_server");
    pmain->destroy();
  }
  
  return 0;
}

