#include "config.h"

#include <gst/gst.h>

#include "gstrtponvifmetapay.h"
#include "gstrtponvifparse.h"
#include "gstrtponviftimestamp.h"

static gboolean plugin_init(GstPlugin *plugin) {
    if (!gst_element_register(plugin, "rtponviftimestamp", GST_RANK_NONE, GST_TYPE_RTP_ONVIF_TIMESTAMP))
        return FALSE;
    if (!gst_element_register(plugin, "rtponvifparse", GST_RANK_NONE, GST_TYPE_RTP_ONVIF_PARSE))
        return FALSE;
    if (!gst_element_register(plugin, "rtponvifmetapay", GST_RANK_NONE, GST_TYPE_RTP_ONVIF_METAPAY))
        return FALSE;

    return TRUE;
}

GST_PLUGIN_DEFINE(GST_VERSION_MAJOR,
                  GST_VERSION_MINOR,
                  rtponvif,
                  "ONVIF Streaming features",
                  plugin_init,
                  "0.0.1",
                  GST_LICENSE_UNKNOWN,
                  "gst-rtp-onvif",
                  "gst-rtp-onvif")
