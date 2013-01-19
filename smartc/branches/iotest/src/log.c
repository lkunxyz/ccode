
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
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <errno.h>
#include <string.h>

#include "server.h"
#include "log.h"

static const char *debug_log_path = DEFAULT_DEBUG_LOG;
static const char *error_log_path = DEFAULT_ERROR_LOG;
static const char *warning_log_path = DEFAULT_WARN_LOG;

void log_init(void) 
{
	assert(config);
	
	if(!config->log.error_log) {
		config->log.error_log = fopen(error_log_path, "w");
		assert(config->log.error_log);
		setvbuf(config->log.error_log, NULL,  _IONBF, 0);
	}
	if(!config->log.warning_log) {
		config->log.warning_log = fopen(warning_log_path, "w");
		assert(config->log.warning_log);
		setvbuf(config->log.warning_log, NULL,  _IONBF, 0);
	}
	if(!config->log.debug_log) {
		config->log.debug_log = fopen(debug_log_path, "w");
		if(config->log.debug_log == NULL) {
			fprintf(stderr, "%s %d: %s\n", __FILE__, __LINE__, strerror(errno));
			abort();
		}
		setvbuf(config->log.debug_log, NULL,  _IONBF, 0);
	}

	log_debug(__FILE__, __LINE__, "log_init finish!\n");
}

void log_debug(const char *file, const int line, const char *format,...)
{
	assert(config);

	if(!config->server.debug_mode) 
		return;

	assert(config->log.debug_log);
	
	char new_format[MAX_LINE];
	
	va_list  va;
	va_start(va, format);
	
	snprintf(new_format, BUF_SIZE - 1 ,  "[%s] | [%s %d] %s", smart_time(), file, line, format);

	if(vfprintf(config->log.debug_log, new_format, va) < 0)
		fprintf(stderr, "Warning. [%s %d] %s\n", __FILE__, __LINE__, strerror(errno));

	va_end(va);
}

void vlog_debug(FILE *file, const char *format,...)
{
	assert(config);

	if(!file) 
		return;

	static char new_format[MAX_LINE];
	
	va_list  va;
	va_start(va, format);
	
	snprintf(new_format, BUF_SIZE - 1 ,  "[%s] | %s", smart_time(), format);

	if(vfprintf(file, new_format, va) < 0)
		fprintf(stderr, "Warning. [%s %d] %s\n", __FILE__, __LINE__, strerror(errno));

	va_end(va);
}
