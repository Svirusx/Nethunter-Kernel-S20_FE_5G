#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>

#include <linux/ipc_logging.h>


#define LOG_CTX_PAGE_CNT 150
static void *log_ctx;
#define MAX_LINE_SIZE 512
static char *buf;

void net_log(const char *fmt, ...)
{
	va_list arg_list;

	va_start(arg_list, fmt);
	if (log_ctx && buf) {
		vscnprintf(buf, MAX_LINE_SIZE, fmt, arg_list);
		ipc_log_string(log_ctx, "%s", buf);
	}
	va_end(arg_list);
}
EXPORT_SYMBOL(net_log);

static int __init net_ipc_log_init(void)
{

	if (!log_ctx)
		log_ctx = ipc_log_context_create(LOG_CTX_PAGE_CNT,
							"net_log", 0);
	if (!buf)
		buf = kmalloc(MAX_LINE_SIZE, GFP_KERNEL);

	// error ????
	//
	return 0;
}

static void __exit net_ipc_log_exit(void)
{
	if (log_ctx)
		ipc_log_context_destroy(log_ctx);
	if (buf)
		kfree(buf);
}

module_init(net_ipc_log_init);
module_exit(net_ipc_log_exit);
MODULE_LICENSE("GPL");
