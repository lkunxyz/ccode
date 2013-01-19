
/*
 * Version: MPL 1.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is HongWei.Peng code.
 *
 * The Initial Developer of the Original Code is
 * HongWei.Peng.
 * Portions created by the Initial Developer are Copyright (C) 2007
 * the Initial Developer. All Rights Reserved.
 * 
 * Author:
 *		tangtang 9/2/2008
 *
 * Contributor(s):
 *
 */

#ifndef CACHE_CONTROL_H 
#define CACHE_CONTROL_H

#include "cache_file.h"
#include "http.h"

extern int cache_able(struct cache_file *cache, struct request_t *req, struct reply_t *reply);
extern int freshness_check(struct cache_file *cache, struct request_t *req);
extern void cache_control_set(struct cache_control *cc, struct header_t *header);
extern time_t cache_freshness_lifetime_server(struct cache_file *cache);
extern time_t cache_age(struct cache_file *cache);
#endif
