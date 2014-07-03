/*
 * Copyright (C) 2014 Michal Ratajsky <michal.ratajsky@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the licence, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include <glib.h>
#include <glib-object.h>
#include <string.h>

#include <libmatemixer/matemixer-client-stream.h>
#include <libmatemixer/matemixer-stream.h>

#include <pulse/pulseaudio.h>

#include "pulse-connection.h"
#include "pulse-client-stream.h"
#include "pulse-monitor.h"
#include "pulse-stream.h"
#include "pulse-source.h"
#include "pulse-source-output.h"

static void pulse_source_output_class_init (PulseSourceOutputClass *klass);
static void pulse_source_output_init       (PulseSourceOutput      *output);

G_DEFINE_TYPE (PulseSourceOutput, pulse_source_output, PULSE_TYPE_CLIENT_STREAM);

static gboolean      source_output_set_mute       (MateMixerStream       *stream,
                                                   gboolean               mute);
static gboolean      source_output_set_volume     (MateMixerStream       *stream,
                                                   pa_cvolume            *volume);
static gboolean      source_output_set_parent     (MateMixerClientStream *stream,
                                                   MateMixerStream       *parent);
static gboolean      source_output_remove         (MateMixerClientStream *stream);
static PulseMonitor *source_output_create_monitor (MateMixerStream       *stream);

static void
pulse_source_output_class_init (PulseSourceOutputClass *klass)
{
    PulseStreamClass       *stream_class;
    PulseClientStreamClass *client_class;

    stream_class = PULSE_STREAM_CLASS (klass);

    stream_class->set_mute       = source_output_set_mute;
    stream_class->set_volume     = source_output_set_volume;
    stream_class->create_monitor = source_output_create_monitor;

    client_class = PULSE_CLIENT_STREAM_CLASS (klass);

    client_class->set_parent = source_output_set_parent;
    client_class->remove     = source_output_remove;
}

static void
pulse_source_output_init (PulseSourceOutput *output)
{
}

PulseStream *
pulse_source_output_new (PulseConnection             *connection,
                         const pa_source_output_info *info,
                         PulseStream                 *parent)
{
    PulseSourceOutput *output;

    g_return_val_if_fail (PULSE_IS_CONNECTION (connection), NULL);
    g_return_val_if_fail (info != NULL, NULL);

    /* Consider the sink input index as unchanging parameter */
    output = g_object_new (PULSE_TYPE_SOURCE_OUTPUT,
                           "connection", connection,
                           "index", info->index,
                           NULL);

    /* Other data may change at any time, so let's make a use of our update function */
    pulse_source_output_update (PULSE_STREAM (output), info, parent);

    return PULSE_STREAM (output);
}

gboolean
pulse_source_output_update (PulseStream                 *stream,
                            const pa_source_output_info *info,
                            PulseStream                 *parent)
{
    MateMixerStreamFlags flags = MATE_MIXER_STREAM_INPUT |
                                 MATE_MIXER_STREAM_CLIENT;
    gchar *name;

    const gchar *prop;
    const gchar *description = NULL;

    g_return_val_if_fail (PULSE_IS_SOURCE_OUTPUT (stream), FALSE);

    /* Let all the information update before emitting notify signals */
    g_object_freeze_notify (G_OBJECT (stream));

    /* Many other mixer applications query the Pulse client list and use the
     * client name here, but we use the name only as an identifier, so let's avoid
     * this unnecessary overhead and use a custom name.
     * Also make sure to make the name unique by including the Pulse index. */
    name = g_strdup_printf ("pulse-stream-client-input-%lu", (gulong) info->index);

    pulse_stream_update_name (stream, name);
    g_free (name);

    prop = pa_proplist_gets (info->proplist, PA_PROP_APPLICATION_NAME);
    if (prop != NULL)
        pulse_client_stream_update_app_name (PULSE_CLIENT_STREAM (stream), prop);

    prop = pa_proplist_gets (info->proplist, PA_PROP_APPLICATION_ID);
    if (prop != NULL)
        pulse_client_stream_update_app_id (PULSE_CLIENT_STREAM (stream), prop);

    prop = pa_proplist_gets (info->proplist, PA_PROP_APPLICATION_VERSION);
    if (prop != NULL)
        pulse_client_stream_update_app_version (PULSE_CLIENT_STREAM (stream), prop);

    prop = pa_proplist_gets (info->proplist, PA_PROP_APPLICATION_ICON_NAME);
    if (prop != NULL)
        pulse_client_stream_update_app_icon (PULSE_CLIENT_STREAM (stream), prop);

    prop = pa_proplist_gets (info->proplist, PA_PROP_MEDIA_ROLE);

    if (prop != NULL && !strcmp (prop, "event")) {
        /* The event description seems to provide much better readable
         * description for event streams */
        prop = pa_proplist_gets (info->proplist, PA_PROP_EVENT_DESCRIPTION);

        if (G_LIKELY (prop != NULL))
            description = prop;

        flags |= MATE_MIXER_STREAM_EVENT;
    }
    if (description == NULL)
        description = info->name;

    pulse_stream_update_description (stream, description);

    if (info->client != PA_INVALID_INDEX)
        flags |= MATE_MIXER_STREAM_APPLICATION;

    if (pa_channel_map_can_balance (&info->channel_map))
        flags |= MATE_MIXER_STREAM_CAN_BALANCE;
    if (pa_channel_map_can_fade (&info->channel_map))
        flags |= MATE_MIXER_STREAM_CAN_FADE;

#if PA_CHECK_VERSION(1, 0, 0)
    if (info->has_volume) {
        flags |= MATE_MIXER_STREAM_HAS_VOLUME;
        if (info->volume_writable)
            flags |= MATE_MIXER_STREAM_CAN_SET_VOLUME;
    }
    flags |= MATE_MIXER_STREAM_HAS_MUTE;

    pulse_stream_update_flags (stream, flags);
    pulse_stream_update_mute (stream, info->mute ? TRUE : FALSE);

    if (info->has_volume)
        pulse_stream_update_volume (stream, &info->volume, &info->channel_map, 0);
    else
        pulse_stream_update_volume (stream, NULL, &info->channel_map, 0);
#else
    pulse_stream_update_flags (stream, flags);
    pulse_stream_update_volume (stream, NULL, &info->channel_map, 0);
#endif

    if (G_LIKELY (parent != NULL))
        pulse_client_stream_update_parent (PULSE_CLIENT_STREAM (stream),
                                           MATE_MIXER_STREAM (parent));
    else
        pulse_client_stream_update_parent (PULSE_CLIENT_STREAM (stream), NULL);

    // XXX needs to fix monitor if parent changes

    g_object_thaw_notify (G_OBJECT (stream));
    return TRUE;
}

static gboolean
source_output_set_mute (MateMixerStream *stream, gboolean mute)
{
    PulseStream *pulse;

    g_return_val_if_fail (PULSE_IS_SOURCE_OUTPUT (stream), FALSE);

    pulse = PULSE_STREAM (stream);

    return pulse_connection_set_source_output_mute (pulse_stream_get_connection (pulse),
                                                    pulse_stream_get_index (pulse),
                                                    mute);
}

static gboolean
source_output_set_volume (MateMixerStream *stream, pa_cvolume *volume)
{
    PulseStream *pulse;

    g_return_val_if_fail (PULSE_IS_SOURCE_OUTPUT (stream), FALSE);
    g_return_val_if_fail (volume != NULL, FALSE);

    pulse = PULSE_STREAM (stream);

    return pulse_connection_set_source_output_volume (pulse_stream_get_connection (pulse),
                                                      pulse_stream_get_index (pulse),
                                                      volume);
}

static gboolean
source_output_set_parent (MateMixerClientStream *stream, MateMixerStream *parent)
{
    PulseStream *pulse;

    g_return_val_if_fail (PULSE_IS_SOURCE_OUTPUT (stream), FALSE);

    if (G_UNLIKELY (!PULSE_IS_SOURCE (parent))) {
        g_warning ("Could not change stream parent to %s: not a parent input stream",
                   mate_mixer_stream_get_name (parent));
        return FALSE;
    }

    pulse = PULSE_STREAM (stream);

    return pulse_connection_move_sink_input (pulse_stream_get_connection (pulse),
                                             pulse_stream_get_index (pulse),
                                             pulse_stream_get_index (PULSE_STREAM (parent)));
}

static gboolean
source_output_remove (MateMixerClientStream *stream)
{
    PulseStream *pulse;

    g_return_val_if_fail (PULSE_IS_SOURCE_OUTPUT (stream), FALSE);

    pulse = PULSE_STREAM (stream);

    return pulse_connection_kill_source_output (pulse_stream_get_connection (pulse),
                                                pulse_stream_get_index (pulse));
}

static PulseMonitor *
source_output_create_monitor (MateMixerStream *stream)
{
    MateMixerStream *parent;

    g_return_val_if_fail (PULSE_IS_SOURCE_OUTPUT (stream), NULL);

    parent = mate_mixer_client_stream_get_parent (MATE_MIXER_CLIENT_STREAM (stream));
    if (G_UNLIKELY (parent == NULL)) {
        g_debug ("Not creating monitor for client stream %s as it is not available",
                 mate_mixer_stream_get_name (stream));
        return NULL;
    }

    return pulse_connection_create_monitor (pulse_stream_get_connection (PULSE_STREAM (stream)),
                                            pulse_stream_get_index (PULSE_STREAM (parent)),
                                            PA_INVALID_INDEX);
}
