#include"logprintf.h"
#include<mtutil/mutex.h>
#include<mtutil/mtcommon.h>

#include<stdio.h>
#include<stdint.h>
#include<time.h>

#define LOG_FILE "latest.log"
#ifndef LOG_TRUNCATE
#define LOG_TRUNCATE NDEBUG
#endif

static atomic_int g_log_new_session = 0;
static mutex_t g_log_lock = 0;
static char g_log_last[4096];

typedef struct output_funclist_struct
{
	log_on_output_fp fp;
	void *userdata;
	struct output_funclist_struct *prev;
	struct output_funclist_struct *next;
}output_funclist_t, *output_funclist_p;

static output_funclist_p g_on_output_fpll = NULL;
static output_funclist_p g_ll_tail = NULL;

#define GLE_COUNT 8
#define GLE_BUFSIZE 256
TLS_VAR char g_strbuf_GetLastError[GLE_COUNT][GLE_BUFSIZE];
TLS_VAR size_t g_strbuf_GetLastError_Index = 0;

static void log_enter_lock()
{
	mutex_acquire(&g_log_lock);
}

static void log_leave_lock()
{
	mutex_unacquire(&g_log_lock);
}

static FILE *log_get_fp()
{
	FILE *fp = NULL;
#if LOG_TRUNCATE
	if(atomic_load(&g_log_new_session)) fp = fopen(LOG_FILE, "a");
	else fp = fopen(LOG_FILE, "w");
#else // !LOG_TRUNCATE
	fp = fopen(LOG_FILE, "a");
#endif // !LOG_TRUNCATE
	if(!fp) fp = stdout;
	
	return fp;
}

static void log_free_fp(FILE *fp)
{
	if(fp != stdout) fclose(fp);
}

static void log_session_check(FILE *fp)
{
	if(!atomic_load(&g_log_new_session))
	{
		fprintf(fp, "\n\t======== NEW SESSION ========\n\n");
		atomic_store(&g_log_new_session, 1);
	}
}

void log_push_cb(log_on_output_fp on_output, void *userdata)
{
	output_funclist_p np = calloc(1, sizeof *np);
	if(!np)
	{
		logprintf_e("Push callback function for log system failed: (%d) %s.\n", errno, strerror(errno));
		return;
	}
	
	np->fp = on_output;
	np->userdata = userdata;
	
	log_enter_lock();
	if(!g_on_output_fpll) g_on_output_fpll = np;
	if(g_ll_tail)
	{
		g_ll_tail->next = np;
		np->prev = g_ll_tail;
	}
	g_ll_tail = np;
	log_leave_lock();
}

void log_pop_cb()
{
	log_enter_lock();
	if(g_ll_tail)
	{
		output_funclist_p t = g_ll_tail;
		g_ll_tail = t->prev;
		if(g_ll_tail) g_ll_tail->next = NULL;
		else g_on_output_fpll = NULL;
		free(t);
	}
	else g_on_output_fpll = NULL;
	
	log_leave_lock();
}

void logvprintf_type(const char *type, const char *fmt, va_list ap)
{
	time_t curtime;
	char *str_time;
	FILE *fp;
	int nch;
	
	log_enter_lock();
	curtime = time(NULL);
	fp = log_get_fp();
	log_session_check(fp);
	
	str_time = asctime(gmtime(&curtime));
	
	fprintf(fp, "[%.24s][%s] ", str_time, type);
	vfprintf(fp, fmt, ap);
	
	nch = sprintf(g_log_last, "[%.24s][%s] ", str_time, type);
	if(nch < sizeof g_log_last) vsprintf(&g_log_last[nch], fmt, ap);
	
	if(g_on_output_fpll)
	{
		output_funclist_p n = g_on_output_fpll;
		while(n)
		{
			n->fp(g_log_last, n->userdata);
			n = n->next;
		}
	}
	
	log_free_fp(fp);
	log_leave_lock();
}

void logprintf_type(const char *type, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	logvprintf_type(type, fmt, ap);
	va_end(ap);
}

void logprintf(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	logvprintf_type("INFO", fmt, ap);
	va_end(ap);
}

void logprintf_w(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	logvprintf_type("WARN", fmt, ap);
	va_end(ap);
}

void logprintf_e(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	logvprintf_type("ERROR", fmt, ap);
	va_end(ap);
}

#ifdef _WIN32
char *str_GetLastError()
{
	char *retbuf = g_strbuf_GetLastError[(g_strbuf_GetLastError_Index ++) % GLE_COUNT];
	DWORD last_error = GetLastError();
	DWORD ret = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, NULL, last_error, 0,
		retbuf, GLE_BUFSIZE, NULL);
	if(!ret)
	{
		sprintf(retbuf, "GetLastError() = %u; And FormatMessageA() failed with error %u.", (uint32_t)last_error, (uint32_t)GetLastError());
	}
	return retbuf;
}
#endif
