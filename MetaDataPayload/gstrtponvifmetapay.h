/* ONVIF metadata RTP Payloader plugin for GStreamer
 * Copyright (C) 2019 Aleksandr Muravev <aleksandrm8@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef __GST_RTP_ONVIF_METAPAY_H__
#define __GST_RTP_ONVIF_METAPAY_H__

#include <gst/base/gstadapter.h>
#include <gst/gst.h>
#include <gst/rtp/gstrtpbasepayload.h>
#include <gst/rtp/gstrtpbuffer.h>

G_BEGIN_DECLS
#define GST_TYPE_RTP_ONVIF_METAPAY (gst_rtp_onvif_metapay_get_type())
#define GST_RTP_ONVIF_METAPAY(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_RTP_ONVIF_METAPAY, GstRtpOnvifMetapay))
#define GST_RTP_ONVIF_METAPAY_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_RTP_ONVIF_METAPAY, GstRtpOnvifMetapayClass))
#define GST_IS_RTP_ONVIF_METAPAY(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_RTP_ONVIF_METAPAY))
#define GST_IS_RTP_ONVIF_METAPAY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_RTP_ONVIF_METAPAY))
#define GST_RTP_ONVIF_METAPAY_CAST(obj) ((GstRtpOnvifMetapay *)(obj))
enum GstRtpOnvifMetapayState { META_NOT_STARTED, META_DATA_OBJECT, META_PACKETS, META_END };

typedef struct _GstRtpOnvifMetapay GstRtpOnvifMetapay;
typedef struct _GstRtpOnvifMetapayClass GstRtpOnvifMetapayClass;

struct _GstRtpOnvifMetapay {
    GstRTPBasePayload rtppay;

    enum GstRtpOnvifMetapayState state;

    GstClock *clock;
    GstClockTime last_frame_time;
    guint32 last_frame_timestamp;
    gint first_frame;
};

struct _GstRtpOnvifMetapayClass {
    GstRTPBasePayloadClass parent_class;
};

GType gst_rtp_onvif_metapay_get_type(void);
gboolean gst_rtp_onvif_metapay_plugin_init(GstPlugin *plugin);

G_END_DECLS
#endif /* __GST_RTP_ONVIF_METAPAY_H__ */
