/**
 * (C) 2010-2011 Alibaba Group Holding Limited.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 * 
 * Version: $Id$
 *
 * ob_schema_reader.cpp for ...
 *
 * Authors:
 *   qushan <qushan@taobao.com>
 *
 */

#include <tbsys.h>

#include "common/ob_schema.h"
using namespace oceanbase::common;
int main(int argc, char* argv[])
{
  if (argc != 2)
  {
    printf("usage %s schema_file\n", argv[0]);
    return 0;
  }
  tbsys::CConfig c1;
  ObSchemaManager mm(1);
  if ( mm.parse_from_file(argv[1], c1))
  {
    mm.print_info();
  }
  else 
  {
    printf("parse file %s error\n",argv[1]);
  }
  return 0;

}

