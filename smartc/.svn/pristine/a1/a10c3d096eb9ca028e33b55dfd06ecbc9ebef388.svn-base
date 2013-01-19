
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
 *		tangtang 1/6/2007
 *
 * Contributor(s):
 *
 */
#ifndef LOG_H
#define LOG_H	

#define s_error(fmt, ...) \
		     vlog_debug(config->log.error_log, "[%s %d] error "fmt, __FILE__, __LINE__, __VA_ARGS__)

#define warning(fmt, ...) \
		     vlog_debug(config->log.warning_log, "[%s %d] warning "fmt, __FILE__, __LINE__, __VA_ARGS__)

#define debug(fmt, ...) \
		     log_debug(__FILE__, __LINE__, fmt, __VA_ARGS__)

extern void log_init(void);
extern void log_debug(const char *file, const int line, const char *format,...);

#endif
