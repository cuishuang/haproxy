/*
 * include/haproxy/stream_interface.h
 * This file contains stream_interface function prototypes
 *
 * Copyright (C) 2000-2014 Willy Tarreau - w@1wt.eu
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation, version 2.1
 * exclusively.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef _HAPROXY_STREAM_INTERFACE_H
#define _HAPROXY_STREAM_INTERFACE_H

#include <haproxy/api.h>
#include <haproxy/applet.h>
#include <haproxy/channel.h>
#include <haproxy/connection.h>
#include <haproxy/conn_stream.h>
#include <haproxy/cs_utils.h>
#include <haproxy/obj_type.h>

extern struct cs_app_ops cs_app_embedded_ops;
extern struct cs_app_ops cs_app_conn_ops;
extern struct cs_app_ops cs_app_applet_ops;
extern struct data_cb si_conn_cb;
extern struct data_cb check_conn_cb;

struct stream_interface *si_new(struct conn_stream *cs);
void si_free(struct stream_interface *si);

/* main event functions used to move data between sockets and buffers */
int conn_si_send_proxy(struct connection *conn, unsigned int flag);
void si_applet_wake_cb(struct stream_interface *si);
void si_update_rx(struct stream_interface *si);
void si_update_tx(struct stream_interface *si);
struct task *si_cs_io_cb(struct task *t, void *ctx, unsigned int state);
void si_update_both(struct stream_interface *si_f, struct stream_interface *si_b);
int si_sync_recv(struct stream_interface *si);
void si_sync_send(struct stream_interface *si);

/* returns the channel which receives data from this stream interface (input channel) */
static inline struct channel *si_ic(struct stream_interface *si)
{
	struct stream *strm = __cs_strm(si->cs);

	return ((si->flags & SI_FL_ISBACK) ? &(strm->res) : &(strm->req));
}

/* returns the channel which feeds data to this stream interface (output channel) */
static inline struct channel *si_oc(struct stream_interface *si)
{
	struct stream *strm = __cs_strm(si->cs);

	return ((si->flags & SI_FL_ISBACK) ? &(strm->req) : &(strm->res));
}

/* returns the buffer which receives data from this stream interface (input channel's buffer) */
static inline struct buffer *si_ib(struct stream_interface *si)
{
	return &si_ic(si)->buf;
}

/* returns the buffer which feeds data to this stream interface (output channel's buffer) */
static inline struct buffer *si_ob(struct stream_interface *si)
{
	return &si_oc(si)->buf;
}

/* returns the stream associated to a stream interface */
static inline struct stream *si_strm(struct stream_interface *si)
{
	return __cs_strm(si->cs);
}

/* returns the task associated to this stream interface */
static inline struct task *si_task(struct stream_interface *si)
{
	struct stream *strm = __cs_strm(si->cs);

	return strm->task;
}

/* returns the stream interface on the other side. Used during forwarding. */
static inline struct stream_interface *si_opposite(struct stream_interface *si)
{
	struct stream *strm = __cs_strm(si->cs);

	return ((si->flags & SI_FL_ISBACK) ? strm->csf->si : strm->csb->si);
}

/* initializes a stream interface and create the event
 * tasklet.
 */
static inline int si_init(struct stream_interface *si)
{
	si->flags         &= SI_FL_ISBACK;
	si->cs             = NULL;
	return 0;
}

/* Returns non-zero if the stream interface's Rx path is blocked */
static inline int si_rx_blocked(const struct stream_interface *si)
{
	return !!(si->flags & SI_FL_RXBLK_ANY);
}


/* Returns non-zero if the stream interface's Rx path is blocked because of lack
 * of room in the input buffer.
 */
static inline int si_rx_blocked_room(const struct stream_interface *si)
{
	return !!(si->flags & SI_FL_RXBLK_ROOM);
}

/* Returns non-zero if the stream interface's endpoint is ready to receive */
static inline int si_rx_endp_ready(const struct stream_interface *si)
{
	return !(si->flags & SI_FL_RX_WAIT_EP);
}

/* The stream interface announces it is ready to try to deliver more data to the input buffer */
static inline void si_rx_endp_more(struct stream_interface *si)
{
	si->flags &= ~SI_FL_RX_WAIT_EP;
}

/* The stream interface announces it doesn't have more data for the input buffer */
static inline void si_rx_endp_done(struct stream_interface *si)
{
	si->flags |=  SI_FL_RX_WAIT_EP;
}

/* Tell a stream interface the input channel is OK with it sending it some data */
static inline void si_rx_chan_rdy(struct stream_interface *si)
{
	si->flags &= ~SI_FL_RXBLK_CHAN;
}

/* Tell a stream interface the input channel is not OK with it sending it some data */
static inline void si_rx_chan_blk(struct stream_interface *si)
{
	si->flags |=  SI_FL_RXBLK_CHAN;
}

/* Tell a stream interface the other side is connected */
static inline void si_rx_conn_rdy(struct stream_interface *si)
{
	si->flags &= ~SI_FL_RXBLK_CONN;
}

/* Tell a stream interface it must wait for the other side to connect */
static inline void si_rx_conn_blk(struct stream_interface *si)
{
	si->flags |=  SI_FL_RXBLK_CONN;
}

/* The stream interface just got the input buffer it was waiting for */
static inline void si_rx_buff_rdy(struct stream_interface *si)
{
	si->flags &= ~SI_FL_RXBLK_BUFF;
}

/* The stream interface failed to get an input buffer and is waiting for it.
 * Since it indicates a willingness to deliver data to the buffer that will
 * have to be retried, we automatically clear RXBLK_ENDP to be called again
 * as soon as RXBLK_BUFF is cleared.
 */
static inline void si_rx_buff_blk(struct stream_interface *si)
{
	si->flags |=  SI_FL_RXBLK_BUFF;
}

/* Tell a stream interface some room was made in the input buffer */
static inline void si_rx_room_rdy(struct stream_interface *si)
{
	si->flags &= ~SI_FL_RXBLK_ROOM;
}

/* The stream interface announces it failed to put data into the input buffer
 * by lack of room. Since it indicates a willingness to deliver data to the
 * buffer that will have to be retried, we automatically clear RXBLK_ENDP to
 * be called again as soon as RXBLK_ROOM is cleared.
 */
static inline void si_rx_room_blk(struct stream_interface *si)
{
	si->flags |=  SI_FL_RXBLK_ROOM;
}

/* The stream interface announces it will never put new data into the input
 * buffer and that it's not waiting for its endpoint to deliver anything else.
 * This function obviously doesn't have a _rdy equivalent.
 */
static inline void si_rx_shut_blk(struct stream_interface *si)
{
	si->flags |=  SI_FL_RXBLK_SHUT;
}

/* Returns non-zero if the stream interface's Rx path is blocked */
static inline int si_tx_blocked(const struct stream_interface *si)
{
	return !!(si->flags & SI_FL_WAIT_DATA);
}

/* Returns non-zero if the stream interface's endpoint is ready to transmit */
static inline int si_tx_endp_ready(const struct stream_interface *si)
{
	return (si->flags & SI_FL_WANT_GET);
}

/* Report that a stream interface wants to get some data from the output buffer */
static inline void si_want_get(struct stream_interface *si)
{
	si->flags |= SI_FL_WANT_GET;
}

/* Report that a stream interface failed to get some data from the output buffer */
static inline void si_cant_get(struct stream_interface *si)
{
	si->flags |= SI_FL_WANT_GET | SI_FL_WAIT_DATA;
}

/* Report that a stream interface doesn't want to get data from the output buffer */
static inline void si_stop_get(struct stream_interface *si)
{
	si->flags &= ~SI_FL_WANT_GET;
}

/* Report that a stream interface won't get any more data from the output buffer */
static inline void si_done_get(struct stream_interface *si)
{
	si->flags &= ~(SI_FL_WANT_GET | SI_FL_WAIT_DATA);
}

/* Try to allocate a buffer for the stream-int's input channel. It relies on
 * channel_alloc_buffer() for this so it abides by its rules. It returns 0 on
 * failure, non-zero otherwise. If no buffer is available, the requester,
 * represented by the <wait> pointer, will be added in the list of objects
 * waiting for an available buffer, and SI_FL_RXBLK_BUFF will be set on the
 * stream-int and SI_FL_RX_WAIT_EP cleared. The requester will be responsible
 * for calling this function to try again once woken up.
 */
static inline int si_alloc_ibuf(struct stream_interface *si, struct buffer_wait *wait)
{
	int ret;

	ret = channel_alloc_buffer(si_ic(si), wait);
	if (!ret)
		si_rx_buff_blk(si);
	return ret;
}

/* Sends a shutr to the endpoint using the data layer */
static inline void cs_shutr(struct conn_stream *cs)
{
	cs->ops->shutr(cs);
}

/* Sends a shutw to the endpoint using the data layer */
static inline void cs_shutw(struct conn_stream *cs)
{
	cs->ops->shutw(cs);
}

/* This is to be used after making some room available in a channel. It will
 * return without doing anything if the conn-stream's RX path is blocked.
 * It will automatically mark the stream interface as busy processing the end
 * point in order to avoid useless repeated wakeups.
 * It will then call ->chk_rcv() to enable receipt of new data.
 */
static inline void cs_chk_rcv(struct conn_stream *cs)
{
	if (cs->si->flags & SI_FL_RXBLK_CONN && cs_state_in(cs_opposite(cs)->state, CS_SB_RDY|CS_SB_EST|CS_SB_DIS|CS_SB_CLO))
		si_rx_conn_rdy(cs->si);

	if (si_rx_blocked(cs->si) || !si_rx_endp_ready(cs->si))
		return;

	if (!cs_state_in(cs->state, CS_SB_RDY|CS_SB_EST))
		return;

	cs->si->flags |= SI_FL_RX_WAIT_EP;
	cs->ops->chk_rcv(cs);
}

/* Calls chk_snd on the endpoint using the data layer */
static inline void cs_chk_snd(struct conn_stream *cs)
{
	cs->ops->chk_snd(cs);
}

/* Combines both si_update_rx() and si_update_tx() at once */
static inline void si_update(struct stream_interface *si)
{
	si_update_rx(si);
	si_update_tx(si);
}

#endif /* _HAPROXY_STREAM_INTERFACE_H */

/*
 * Local variables:
 *  c-indent-level: 8
 *  c-basic-offset: 8
 * End:
 */
