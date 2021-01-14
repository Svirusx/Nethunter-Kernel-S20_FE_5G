#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched/clock.h>
#include <linux/timer.h>
#include <linux/time.h>
#include <linux/notifier.h>
#include <linux/list.h>
#include <linux/netdevice.h>
#include <linux/inet_diag.h>
#include <linux/sock_diag.h>
#include <linux/ip.h>
#include <linux/ipv6.h>
#include <linux/rtc.h>
#include <net/sock.h>
#include <net/tcp.h>
#include <net/analytics.h>

/* print format 
*
* u(user) tcpRxBytes tcpTxBytes udpRxBytes udpTxBytes
* ue(user err) tcpRxCounts tcpTxCounts udpRxCounts udpTxCounts
*
* d(rmnet_data) i(rmnet_ipa) m(rmnet_mhi) r(rmnet), etc ,,
* d(rmnet_data) rxBytes rxPackets rxDrops txBytes txPackets txDrops
*/

#ifdef CONFIG_IPC_LOGGING
#include <linux/ipc_logging.h>

#define IPC_LOG_PAGE_CNT 50
static void *ipc_log_ctx;
#define IPC_LOG(x, ...) \
		if (ipc_log_ctx) \
			ipc_log_string(ipc_log_ctx, x, ##__VA_ARGS__)
#endif

#define DEBUG_NETIO_STAT_PRINT BIT(1)

static int verbose_log __read_mostly = DEBUG_NETIO_STAT_PRINT;
module_param(verbose_log, int, 0644);
MODULE_PARM_DESC(verbose_log, "turn on/off log on dmesg");

static atomic64_t netio_tick;

struct usr_io {
	atomic64_t stream_rx;
	atomic64_t stream_tx;
	atomic64_t dgram_rx;
	atomic64_t dgram_tx;
	__u64 o_stream_rx;
	__u64 o_stream_tx;
	__u64 o_dgram_rx;
	__u64 o_dgram_tx;
	atomic64_t stream_err_rx;
	atomic64_t stream_err_tx;
	atomic64_t dgram_err_rx;
	atomic64_t dgram_err_tx;
	__u64 o_stream_err_rx;
	__u64 o_stream_err_tx;
	__u64 o_dgram_err_rx;
	__u64 o_dgram_err_tx;
};
static struct usr_io usr_stat;

static struct hrtimer netio_timer;
static __u64 netio_utc_interval;

struct netdev_name {
	char name[IFNAMSIZ];
	char alias[IFNAMSIZ];
	int off;
};

static struct netdev_name name_alias[] = {
	{ "rmnet_data", "d", 10 },
	{ "rmnet_ipa", "i", 9 },
	{ "rmnet_mhi", "m", 9 },
	{ "rmnet", "r", 5 }, /* should be last rmnet */

	{ "rndis", "rnd", 5 },
	{ "v4-rmnet", "4r", 7 },
	{ "wlan", "w", 4 },
	{ "swlan", "sw", 5 },
	{ "epdg", "e", 4 },
	{ "dummy", "dm", 4 },
	{ "lo", "l", 1 },
};

void net_usr_tx(struct sock *sk, int len)
{
	/* per sk operation first */

	// sk reference -> usr tx += len << does not need to be atomic var.

	/* now count total bytes */
	if (len < 0) {
		if (sk->sk_type == SOCK_STREAM)
			atomic64_inc(&usr_stat.stream_err_tx);
		else if (sk->sk_type == SOCK_DGRAM)
			atomic64_inc(&usr_stat.dgram_err_tx);
	} else {
		if (sk->sk_type == SOCK_STREAM)
			atomic64_add(len, &usr_stat.stream_tx);
		else if (sk->sk_type == SOCK_DGRAM)
			atomic64_add(len, &usr_stat.dgram_tx);
	}
}

void net_usr_rx(struct sock *sk, int len)
{
	/* per sk operation first */

	// sk reference -> usr tx += len

	/* now count total bytes */
	if (len < 0) {
		if (sk->sk_type == SOCK_STREAM)
			atomic64_inc(&usr_stat.stream_err_rx);
		else if (sk->sk_type == SOCK_DGRAM)
			atomic64_inc(&usr_stat.dgram_err_rx);	
	} else {
		if (sk->sk_type == SOCK_STREAM)
			atomic64_add(len, &usr_stat.stream_rx);
		else if (sk->sk_type == SOCK_DGRAM)
			atomic64_add(len, &usr_stat.dgram_rx);
	}
}

/** ptype handler to get each packet information **/

#define IFF_NETIO BIT(30)
static int netio_ptype_rcv(struct sk_buff *skb, struct net_device *dev,
		       struct packet_type *ptype, struct net_device *orig_dev)
{
	/* start timer for first tick */
	if (atomic64_inc_return(&netio_tick) == 1) {
		hrtimer_start(&netio_timer,
			      ktime_add(ktime_get(), ktime_set(1, 0)),
			      HRTIMER_MODE_ABS);
	}
	/* hack for each net device recognition */
	dev->flags |= IFF_NETIO;

	consume_skb(skb);

	return 0;
}

static struct packet_type netio_hook ____cacheline_aligned_in_smp = {
	.type = cpu_to_be16(ETH_P_ALL),
	.func = netio_ptype_rcv,
};


/** net device up / down list management **/

struct netdev_info {
	struct list_head list;
	struct net_device *dev;
	char alias[IFNAMSIZ];
	struct rtnl_link_stats64 stats;
};

static void get_devname_alias(const char *devname, char *alias)
{
	int arr_size = ARRAY_SIZE(name_alias);
	int i;

	for (i = 0; i < arr_size; i++) {
		if (!strncmp(devname, name_alias[i].name, name_alias[i].off)) {
			snprintf(alias, IFNAMSIZ, "%s%s",
				 name_alias[i].alias,
				 devname + name_alias[i].off);
			return;
		}
	}
	snprintf(alias, IFNAMSIZ, "%s", devname);
}

static LIST_HEAD(updev_list);

static int netio_ndev_event_cb(struct notifier_block *nb,
			       unsigned long event, void *ptr)
{
	struct net_device *dev = netdev_notifier_info_to_dev(ptr);
	struct netdev_info *ndev_i, *temp;

	if (!dev)
		return NOTIFY_DONE;

	if (event == NETDEV_DOWN) {
		list_for_each_entry_safe(ndev_i, temp, &updev_list, list) {
			if (ndev_i->dev == dev) {
				/* lockless handing, cancel timer prevents
				 * accessing the changing list
				 */
				hrtimer_cancel(&netio_timer);
				//remove from the list
				list_del(&ndev_i->list);
				kfree(ndev_i);
				hrtimer_start(&netio_timer,
					ktime_add(ktime_get(), ktime_set(1, 0)),
					 HRTIMER_MODE_ABS);
				return NOTIFY_DONE;
			}
		}
	}

	if (event == NETDEV_UP) {
		if (!strncmp(dev->name, "ppp", 3))
			return NOTIFY_DONE;

		list_for_each_entry_safe(ndev_i, temp, &updev_list, list) {
			if (ndev_i->dev == dev) {
				return NOTIFY_DONE;
			}
		}
		// add to the list
		ndev_i = kzalloc(sizeof(struct netdev_info), GFP_KERNEL);
		if (ndev_i) {
			INIT_LIST_HEAD(&ndev_i->list);
			ndev_i->dev = dev;
			get_devname_alias(dev->name, ndev_i->alias);
			/* lockless handing, cancel timer prevents
			 * accessing the changing list
			 */
			hrtimer_cancel(&netio_timer);
			list_add_tail(&ndev_i->list, &updev_list);
			hrtimer_start(&netio_timer,
			      ktime_add(ktime_get(), ktime_set(1, 0)),
			      HRTIMER_MODE_ABS);
		}
	}

	return NOTIFY_DONE;
}

static struct notifier_block netio_ndev_nb = {
	.notifier_call = netio_ndev_event_cb,
};

/***********************************************/

/** print out to kernel log netio stat **/
static char *log_buf;

struct stats_diff {
	__u64	rx_packets;		/* total packets received	*/
	__u64	tx_packets;		/* total packets transmitted	*/
	__u64	rx_bytes;		/* total bytes received 	*/
	__u64	tx_bytes;		/* total bytes transmitted	*/
	__u64	rx_drop_errs;		/* bad packets received	+ dropped */
	__u64	tx_drop_errs;		/* packet transmit problems + dropped */
};

static void get_stats_diff(struct stats_diff *st_diff,
		      struct rtnl_link_stats64 *o_stats,
		      struct rtnl_link_stats64 *n_stats)
{
	st_diff->rx_packets = n_stats->rx_packets - o_stats->rx_packets;
	st_diff->tx_packets = n_stats->tx_packets - o_stats->tx_packets;
	st_diff->rx_bytes = n_stats->rx_bytes - o_stats->rx_bytes;
	st_diff->tx_bytes = n_stats->tx_bytes - o_stats->tx_bytes;
	st_diff->rx_drop_errs = n_stats->rx_errors - o_stats->rx_errors;
	st_diff->rx_drop_errs += (n_stats->rx_dropped - o_stats->rx_dropped);
	st_diff->rx_drop_errs +=
		(n_stats->rx_missed_errors - o_stats->rx_missed_errors);
	st_diff->tx_drop_errs = n_stats->tx_errors - o_stats->tx_errors;
	st_diff->tx_drop_errs += (n_stats->tx_dropped - o_stats->tx_dropped);
}

#define PRINT_DIFF
/* print format 
*
* u(user) tcpRxBytes tcpTxBytes udpRxBytes udpTxBytes
* ue(user err) tcpRxCounts tcpTxCounts udpRxCounts udpTxCounts
*
* d(rmnet_data) i(rmnet_ipa) m(rmnet_mhi) r(rmnet), etc ,,
* d(rmnet_data) rxBytes rxPackets rxDrops txBytes txPackets txDrops
*/
static void netio_printout(void)
{
	int off = 0;
	int remain = PAGE_SIZE;
	int written;
	struct netdev_info *ndev_i, *temp;
	struct rtnl_link_stats64 o_stats, *n_stats;
	struct stats_diff st_diff;
	__u64 s_rx, s_tx, d_rx, d_tx;
	__u64 s_rx_delta, s_tx_delta, d_rx_delta, d_tx_delta;
	__u64 s_erx, s_etx, d_erx, d_etx;
	__u64 s_erx_delta, s_etx_delta, d_erx_delta, d_etx_delta;
	struct timespec ts;
	struct rtc_time tm;

	if (!log_buf)
		return;

	s_rx = atomic64_read(&usr_stat.stream_rx);
	s_tx = atomic64_read(&usr_stat.stream_tx);
	d_rx = atomic64_read(&usr_stat.dgram_rx);
	d_tx = atomic64_read(&usr_stat.dgram_tx);

	s_rx_delta = s_rx - usr_stat.o_stream_rx;
	s_tx_delta = s_tx - usr_stat.o_stream_tx;
	d_rx_delta = d_rx - usr_stat.o_dgram_rx;
	d_tx_delta = d_tx - usr_stat.o_dgram_tx;

	usr_stat.o_stream_rx = s_rx;
	usr_stat.o_stream_tx = s_tx;
	usr_stat.o_dgram_rx = d_rx;
	usr_stat.o_dgram_tx = d_tx;
	
	s_erx = atomic64_read(&usr_stat.stream_err_rx);
	s_etx = atomic64_read(&usr_stat.stream_err_tx);
	d_erx = atomic64_read(&usr_stat.dgram_err_rx);
	d_etx = atomic64_read(&usr_stat.dgram_err_tx);

	s_erx_delta = s_erx - usr_stat.o_stream_err_rx;
	s_etx_delta = s_etx - usr_stat.o_stream_err_tx;
	d_erx_delta = d_erx - usr_stat.o_dgram_err_rx;
	d_etx_delta = d_etx - usr_stat.o_dgram_err_tx;

	usr_stat.o_stream_err_rx = s_erx;
	usr_stat.o_stream_err_tx = s_etx;
	usr_stat.o_dgram_err_rx = d_erx;
	usr_stat.o_dgram_err_tx = d_etx;

	if (s_rx_delta || s_tx_delta ||	d_rx_delta || d_tx_delta) {
#ifdef PRINT_DIFF
		written = scnprintf(log_buf + off, remain,
				    "u %llu %llu %llu %llu ",
				    s_rx_delta, s_tx_delta,
				    d_rx_delta, d_tx_delta);
#else
		written = scnprintf(log_buf + off, remain,
				    "u %llu %llu %llu %llu ",
				    s_rx, s_tx, d_rx, d_tx);
#endif
		off += written;
		remain -= written;
	}

	if (s_erx_delta || s_etx_delta ||	d_erx_delta || d_etx_delta) {
#ifdef PRINT_DIFF
		written = scnprintf(log_buf + off, remain,
				    "ue %llu %llu %llu %llu ",
				    s_erx_delta, s_etx_delta,
				    d_erx_delta, d_etx_delta);
#else
		written = scnprintf(log_buf + off, remain,
				    "ue %llu %llu %llu %llu ",
				    s_erx, s_etx, d_erx, d_etx);
#endif
		off += written;
		remain -= written;
	}
	
	list_for_each_entry_safe(ndev_i, temp, &updev_list, list) {
		if (!ndev_i->dev || !(ndev_i->dev->flags & IFF_NETIO))
			continue;

		ndev_i->dev->flags &= ~(IFF_NETIO & 0xFFFFFFFF);
		n_stats = &ndev_i->stats;
		memcpy(&o_stats, n_stats, sizeof(struct rtnl_link_stats64));
		dev_get_stats(ndev_i->dev, n_stats);

		get_stats_diff(&st_diff, &o_stats, n_stats);

#ifdef PRINT_DIFF
		written = scnprintf(log_buf + off, remain,
				    "%s %llu %llu %llu %llu %llu %llu ",
				    ndev_i->alias,
				    st_diff.rx_bytes, st_diff.rx_packets,
				    st_diff.rx_drop_errs,
				    st_diff.tx_bytes, st_diff.tx_packets,
				    st_diff.tx_drop_errs);
#else
		written = scnprintf(log_buf + off, remain,
				    "%s %llu %llu %llu %llu %llu %llu ",
				    ndev_i->alias,
				    n_stats->rx_bytes, n_stats->rx_packets,
				    n_stats->rx_errors + n_stats->rx_dropped + n_stats->rx_missed_errors,
				    n_stats->tx_bytes, n_stats->tx_packets,
				    n_stats->tx_errors + n_stats->tx_dropped);
#endif

		off += written;
		remain -= written;
	}

	if (off == 0)
		return;

	if ((netio_utc_interval++ % 60) == 0) {
		getnstimeofday(&ts);
		rtc_time_to_tm(ts.tv_sec, &tm);
		written = scnprintf(log_buf + off, remain,
				    "> %02d-%02d %02d:%02d:%02d.%06lu ",
				    tm.tm_mon + 1, tm.tm_mday, tm.tm_hour,
				    tm.tm_min, tm.tm_sec, ts.tv_nsec / 1000);
		off += written;
		remain -= written;
	}

	if (verbose_log & DEBUG_NETIO_STAT_PRINT)
		pr_err("netio %s\n", log_buf);

#ifdef CONFIG_IPC_LOGGING
	IPC_LOG("%s\n", log_buf);
#endif
}

static enum hrtimer_restart netio_timer_handler(struct hrtimer *hrtimer)
{
	if (atomic64_read(&netio_tick) > 0) {
		hrtimer_start(&netio_timer,
			      ktime_add(ktime_get(), ktime_set(1, 0)),
			      HRTIMER_MODE_ABS);
		atomic64_set(&netio_tick, 0);
	} else {
		netio_utc_interval = 0;
	}

	netio_printout();

	return HRTIMER_NORESTART;
}

/***********************************************/

static int __init alts_logger_init(void)
{
	int ret = 0;
	if (!log_buf)
		log_buf = kmalloc(PAGE_SIZE, GFP_KERNEL);

	/* error */
	if (!log_buf) {
		pr_err("%s() fail to alloc log buffer\n", __func__);
		return -ENOMEM;
	}

#ifdef CONFIG_IPC_LOGGING
	if (!ipc_log_ctx)
		ipc_log_ctx = ipc_log_context_create(IPC_LOG_PAGE_CNT,
						     "net_analytics", 0);
#endif

	ret = register_netdevice_notifier(&netio_ndev_nb);
	if (ret < 0) {
		kfree(log_buf);
	}

	hrtimer_init(&netio_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	netio_timer.function = netio_timer_handler;

	dev_add_pack(&netio_hook);

	return ret;
}

static void __exit alts_logger_exit(void)
{
	struct netdev_info *ndev_i, *temp;

	__dev_remove_pack(&netio_hook);
	unregister_netdevice_notifier(&netio_ndev_nb);

	list_for_each_entry_safe(ndev_i, temp, &updev_list, list) {
		list_del(&ndev_i->list);
		kfree(ndev_i);
	}

#ifdef CONFIG_IPC_LOGGING
	if (!ipc_log_ctx)
		ipc_log_context_destroy(ipc_log_ctx);
#endif
	kfree(log_buf);
}

module_init(alts_logger_init);
module_exit(alts_logger_exit);
MODULE_LICENSE("GPL");