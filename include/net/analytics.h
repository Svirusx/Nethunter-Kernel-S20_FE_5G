#ifndef NET_ANALYTICS_H
#define NET_ANALYTICS_H

#ifdef CONFIG_NET_ANALYTICS
#ifndef __COUNT_ARGS24
/* This counts to 24. Any more, it will return 25th argument. */
#define __COUNT_ARGS24(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10,	\
		       _11, _12, _13, _14, _15, _16, _17, _18, _19, _20,\
		       _21, _22, _23, _24, _n, X...) _n

#define CNT_ARGS24(X...) __COUNT_ARGS24(, ##X, 24, 23, 22, 21, 20,	\
					19, 18, 17, 16, 15, 14, 13, 12, 11,\
					10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)
#endif /* __COUNT_ARGS24 */

void net_usr_tx(struct sock *sk, int len);
void net_usr_rx(struct sock *sk, int len);
#else
#endif

#endif /* NET_ANALYTICS_H */
