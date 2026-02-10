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

/**
 * SECTION:element-rtponvifmetapay
 * @title: rtponvifmetapay
 *
 * Payload an RTP-payloaded ONVIF metadata into RTP packets
 * according to point 5.1.2.1.1 (RTP for Metadata stream) of ONVIF
 * Streaming Specification.
 * For detailed information see:
 * https://www.onvif.org/specs/stream/ONVIF-Streaming-Spec.pdf
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/rtp/gstrtpbuffer.h>
#include <stdlib.h>
#include <string.h>

#include "gstrtponvifmetapay.h"

GST_DEBUG_CATEGORY_STATIC(rtponvifmetapay_debug);
#define GST_CAT_DEFAULT (rtponvifmetapay_debug)

static GstStaticPadTemplate gst_rtp_onvif_metapay_sink_template =
    GST_STATIC_PAD_TEMPLATE("sink",
                            GST_PAD_SINK,
                            GST_PAD_ALWAYS,
                            GST_STATIC_CAPS("application/x-onvif-metadata"));

static GstStaticPadTemplate gst_rtp_onvif_metapay_src_template =
    GST_STATIC_PAD_TEMPLATE("src",
                            GST_PAD_SRC,
                            GST_PAD_ALWAYS,
                            GST_STATIC_CAPS("application/x-rtp, "
                                            "media = (string) {\"application\"}, "
                                            "clock-rate = (int) 1000, "
                                            "encoding-name = (string) \"VND.ONVIF.METADATA\""));

static GstFlowReturn gst_rtp_onvif_metapay_handle_buffer(GstRTPBasePayload *rtppay, GstBuffer *buffer);
static gboolean gst_rtp_onvif_metapay_set_caps(GstRTPBasePayload *rtppay, GstCaps *caps);

#define gst_rtp_onvif_metapay_parent_class parent_class
G_DEFINE_TYPE(GstRtpOnvifMetapay, gst_rtp_onvif_metapay, GST_TYPE_RTP_BASE_PAYLOAD);

static void gst_rtp_onvif_metapay_init(GstRtpOnvifMetapay *rtponvifmetapay) {
    g_print("gst_rtp_onvif_metapay_init\n");
    rtponvifmetapay->clock = gst_system_clock_obtain();
    rtponvifmetapay->last_frame_timestamp = rand();
    rtponvifmetapay->first_frame = 1;
}

static void gst_rtp_onvif_metapay_finalize(GObject *object) {
    g_print("gst_rtp_onvif_metapay_finalize\n");
    G_OBJECT_CLASS(parent_class)->finalize(object);
}

static void gst_rtp_onvif_metapay_class_init(GstRtpOnvifMetapayClass *klass) {
    g_print("gst_rtp_onvif_metapay_class_init\n");
    GObjectClass *gobject_class;
    GstElementClass *gstelement_class;
    GstRTPBasePayloadClass *gstbasertppayload_class;

    gobject_class = (GObjectClass *)klass;
    gstelement_class = (GstElementClass *)klass;
    gstbasertppayload_class = (GstRTPBasePayloadClass *)klass;

    gobject_class->finalize = gst_rtp_onvif_metapay_finalize;

    gstbasertppayload_class->handle_buffer = gst_rtp_onvif_metapay_handle_buffer;
    gstbasertppayload_class->set_caps = gst_rtp_onvif_metapay_set_caps;

    gst_element_class_add_static_pad_template(gstelement_class, &gst_rtp_onvif_metapay_sink_template);
    gst_element_class_add_static_pad_template(gstelement_class, &gst_rtp_onvif_metapay_src_template);
    gst_element_class_set_static_metadata(gstelement_class, "RTP ONVIF metadata payloader", "Codec/Payloader/Network",
                                          "Payload-encodes ONVIF metadata into RTP packets (MS_RTSP)",
                                          "Aleksandr Muravev <aleksandrm8@gmail.com>");

    GST_DEBUG_CATEGORY_INIT(rtponvifmetapay_debug, "rtponvifmetapay", 0, "ONVIF metadata RTP Payloader");
}

static gboolean gst_rtp_onvif_metapay_set_caps(GstRTPBasePayload *rtppay, GstCaps *caps) {
    g_print("gst_rtp_onvif_metapay_set_caps\n");
    /* FIXME change application for the actual content */
    gst_rtp_base_payload_set_options(rtppay, "application", TRUE, "VND.ONVIF.METADATA", 1000);
    return TRUE;
}

static GstFlowReturn gst_rtp_onvif_metapay_handle_buffer(GstRTPBasePayload *rtppay_, GstBuffer *buffer) {
    GstRtpOnvifMetapay *rtponvifmetapay = GST_RTP_ONVIF_METAPAY_CAST(rtppay_);
    GstRTPBasePayload *rtppay;
    guint8 *data;
    guint32 buf_size;
    GstFlowReturn ret = GST_FLOW_OK;
    GstBuffer *current;
    guint packet_len;
    guint payload_len;
    GstRTPBuffer rtp = GST_RTP_BUFFER_INIT;
    GstClockTime now = gst_clock_get_time(rtponvifmetapay->clock);

    buf_size = gst_buffer_get_size(buffer);
    packet_len = gst_rtp_buffer_calc_packet_len(buf_size, 0, 0);
    current = gst_rtp_buffer_new_allocate_len(packet_len, 0, 0);

    gst_rtp_buffer_map(current, GST_MAP_READWRITE, &rtp);

    rtppay = GST_RTP_BASE_PAYLOAD(rtponvifmetapay);

    data = gst_rtp_buffer_get_payload(&rtp);
    payload_len = gst_rtp_buffer_get_payload_len(&rtp);
    memset(data, 0, payload_len);

    gst_buffer_extract(buffer, 0, data, buf_size);

    gst_rtp_buffer_set_ssrc(&rtp, rtppay->current_ssrc);
    gst_rtp_buffer_set_marker(&rtp, TRUE);
    gst_rtp_buffer_set_payload_type(&rtp, GST_RTP_BASE_PAYLOAD_PT(rtppay));
    gst_rtp_buffer_set_seq(&rtp, rtppay->seqnum + 1);
    if (rtponvifmetapay->first_frame) {
        rtponvifmetapay->first_frame = 0;
    } else {
        rtponvifmetapay->last_frame_timestamp += (now - rtponvifmetapay->last_frame_time) / 1000000;
    }
    gst_rtp_buffer_set_timestamp(&rtp, rtponvifmetapay->last_frame_timestamp);

    gst_rtp_buffer_unmap(&rtp);

    GST_BUFFER_TIMESTAMP(current) = GST_BUFFER_TIMESTAMP(buffer);

    rtppay->seqnum++;
    rtppay->timestamp = rtponvifmetapay->last_frame_timestamp;

    GST_DEBUG_OBJECT(rtponvifmetapay, "Pushing rtp buffer");
    ret = gst_rtp_base_payload_push(rtppay, current);
    if (ret != GST_FLOW_OK) {
        gst_buffer_unref(buffer);
        return ret;
    }

    rtponvifmetapay->last_frame_time = now;

    gst_buffer_unref(buffer);
    return ret;
}

gboolean gst_rtp_onvif_metapay_plugin_init(GstPlugin *plugin) {
    g_print("Plugin init\n");
    return gst_element_register(plugin, "rtponvifmetapay", GST_RANK_NONE, GST_TYPE_RTP_ONVIF_METAPAY);
}
