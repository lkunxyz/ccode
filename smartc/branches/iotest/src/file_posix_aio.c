
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
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <aio.h>
#include <signal.h>
#include <glib.h>

#include "file_aio.h"
#include "log.h"

#ifdef __USE_GNU
static struct aioinit aio_config;
#endif
static struct job *job_list;
static int job_count;

static void job_list_init(void);
static void job_append(struct job *item);
static struct job* job_remove(struct job *item);
static struct job *job_new(WPF *handler, void *data, struct aiocb *cb);
static void job_clean(struct job *job);
#if USE_SIGIO
static void posix_aio_completion_handler(int signo, siginfo_t *sigval , void *context);
#else 
static void posix_aio_completion_handler(union sigval foo);
#endif

void file_aio_loop(void) 
{
	if(!io_queue) 
		return;
	
	struct job *job = NULL;
	struct job *prev = NULL;
	
	g_async_queue_ref(io_queue);

	if((job = g_async_queue_try_pop(io_queue))) {
		debug("posix_aio_comletion_handler pop %p\n", job);
		assert(job != prev);
		int error;
		int res;
		WPF *func = job->handler;
		void *data = job->data;
		struct aiocb *iocb = job->iocb;
#if USE_SIGIO	
		if(sigval->si_signo != SIGIO) 
			return;
#endif
		if(iocb == NULL) 
			return;

		if(0 != (error = aio_error(iocb))) {
				debug("warning! posix_aio_completion_handler %s\n", strerror(errno));
				job_clean(job);
				safe_free(iocb);
				return ;
		}
		
		res = aio_return(iocb);
		prev = job;
		job_clean(job);
		safe_free(iocb);
	
		func(error, data, res);
	}
	
	g_async_queue_unref(io_queue);
}

int file_aio_init(int max) 
{
	int threads = max > 16 ? max : 16;
#ifdef __USE_GNU
	memset(&aio_config, 0, sizeof(struct aioinit));
	aio_config.aio_threads = threads;
	aio_config.aio_num = 16;
	aio_config.aio_idle_time = 0;

#endif
#if USE_SIGIO
	struct sigaction sig_act;		  
	int ret;
	/* Set up the signal handler */
	sigemptyset(&sig_act.sa_mask);
	sig_act.sa_flags = SA_SIGINFO;
	sig_act.sa_sigaction = posix_aio_completion_handler;

	ret = sigaction(SIGUSR1, &sig_act, NULL); 
	if(ret == -1) {
		error("file_aio_read  %s\n", strerror(errno));
		return -1;
	}
#endif
	if (!g_thread_supported ()) g_thread_init (NULL);

	io_queue =  g_async_queue_new(); 
	if(io_queue == NULL) {
		abort();
	}
	g_async_queue_ref(io_queue);
	job_list = NULL;
	return 1;	
}

int file_aio_write(int fd, struct string *s, long long offset, WPF *handler, void *data)
{
	struct job *job;
	struct aiocb *iocb = calloc(1, sizeof(*iocb));
	assert(iocb);
	iocb->aio_fildes = fd;
	iocb->aio_buf    = s->buf;
	iocb->aio_nbytes = s->offset;
	iocb->aio_offset = offset;
	
	job = job_new(handler, data, iocb);
	assert(job);

#if USE_SIGIO
	iocb->aio_sigevent.sigev_notify = SIGEV_SIGNAL;
	iocb->aio_sigevent.sigev_signo = SIGIO;
	iocb->aio_sigevent.sigev_value.sival_ptr = job;
#else 
	iocb->aio_sigevent.sigev_notify_function = posix_aio_completion_handler;
	iocb->aio_sigevent.sigev_notify_attributes = NULL;
	iocb->aio_sigevent.sigev_value.sival_ptr = job;
	iocb->aio_sigevent.sigev_notify = SIGEV_THREAD;	
#endif

	if(0 != aio_write(iocb)) {
		error("file_aio_write %s\n", strerror(errno));
		job_clean(job);
		safe_free(iocb);
		return -1;
	}
	
	//job_append(job);
	return 0;
}

int file_aio_read(int fd, void *start, int size, long long offset, WPF *handler, void *data)
{	
	struct job *job;
	struct aiocb *iocb = calloc(1, sizeof(*iocb));
	assert(iocb);

	iocb->aio_fildes = fd;
	iocb->aio_buf    = start;
	iocb->aio_nbytes = size;
	iocb->aio_offset = offset;
	
	job = job_new(handler, data, iocb);
	assert(job);
#if USE_SIGIO
	/* Link the AIO request with the Signal Handler */
	iocb->aio_sigevent.sigev_notify = SIGEV_SIGNAL;
	iocb->aio_sigevent.sigev_signo = SIGIO;
	iocb->aio_sigevent.sigev_value.sival_ptr = job;
#else
	iocb->aio_sigevent.sigev_notify_function = posix_aio_completion_handler;
	iocb->aio_sigevent.sigev_notify_attributes = NULL;
	iocb->aio_sigevent.sigev_value.sival_ptr = job;
	iocb->aio_sigevent.sigev_notify = SIGEV_THREAD;
#endif	

	if(0 != aio_read(iocb)) {
		error("file_aio_write %s\n", strerror(errno));
		job_clean(job);
		safe_free(iocb);
		return -1;
	}

	//job_append(job);
	return 0;	
}

#if USE_SIGIO
static void posix_aio_completion_handler(int signo, siginfo_t *sigval , void *context)
#else 
static void posix_aio_completion_handler(union sigval foo)
#endif
{
#if USE_SIGIO
	struct job *job = sigval->si_value.sival_ptr;
#else 
	struct job *job = foo.sival_ptr;
#endif
	GAsyncQueue *outq; 

	g_async_queue_ref(io_queue);
	
	outq = io_queue;

	g_async_queue_push(outq, job);
	
	g_async_queue_unref(io_queue);
}

static void job_list_init(void) 
{
	if(!job_list) {
		job_list = calloc(1, sizeof(*job_list));
		assert(job_list);
		INIT_LIST_HEAD(&job_list->list);
	}
}

static struct job *job_new(WPF *handler, void *data, struct aiocb *cb) 
{
	struct job * job = calloc(1, sizeof(*job));
	if(!job) 
		return NULL;
	
	job->handler = handler;
	job->data = data;
	job->iocb = cb;
	INIT_LIST_HEAD(&job->list);

	return job;
}

static void job_append(struct job *job) 
{
	if(!job)
		return ;
	if(!job_list) {
		job_list = job;
		INIT_LIST_HEAD(&job_list->list);
		return ;
	}

	//list_add_tail(&job->list, &job_list->list);
	job_count++;
}

static void job_clean(struct job *job) 
{
	if(!job) 
		return ;
	safe_free(job);
}

static struct job * job_remove(struct job *job)
{
	if(!job_list) return NULL;

	if(!job) return NULL;
	if(job == job_list) {
		struct job *head = job_list;
		struct job *next = list_entry(&head->list.next, struct job, list);
		list_del(&head->list);
		job_list = next;
		if(head == next) 
			job_list = NULL;
		job_count--;
		return head;
	}
	else {
		list_del(&job->list);
		job_count--;
		return job;
	}
}

