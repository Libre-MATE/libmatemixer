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

// XXX
// - make sure all functions work correctly with flags
// - make sure all functions notify
// - figure out whether functions should g_warning on errors
// - distinguish MateMixer and Pulse variable names

#include <string.h>
#include <glib.h>
#include <glib-object.h>

#include <libmatemixer/matemixer-device.h>
#include <libmatemixer/matemixer-enums.h>
#include <libmatemixer/matemixer-stream.h>
#include <libmatemixer/matemixer-port.h>

#include <pulse/pulseaudio.h>

#include "pulse-connection.h"
#include "pulse-helpers.h"
#include "pulse-monitor.h"
#include "pulse-stream.h"

struct _PulseStreamPrivate
{
    guint32                index;
    guint32                index_device;
    gchar                 *name;
    gchar                 *description;
    MateMixerDevice       *device;
    MateMixerStreamFlags   flags;
    MateMixerStreamState   state;
    gboolean               mute;
    pa_cvolume             volume;
    pa_volume_t            base_volume;
    pa_channel_map         channel_map;
    gfloat                 balance;
    gfloat                 fade;
    GList                 *ports;
    MateMixerPort         *port;
    PulseConnection       *connection;
    PulseMonitor          *monitor;
};

enum {
    PROP_0,
    PROP_NAME,
    PROP_DESCRIPTION,
    PROP_DEVICE,
    PROP_FLAGS,
    PROP_STATE,
    PROP_MUTE,
    PROP_NUM_CHANNELS,
    PROP_VOLUME,
    PROP_BALANCE,
    PROP_FADE,
    PROP_PORTS,
    PROP_ACTIVE_PORT,
    PROP_INDEX,
    PROP_CONNECTION,
    N_PROPERTIES
};

static void mate_mixer_stream_interface_init (MateMixerStreamInterface  *iface);
static void pulse_stream_class_init          (PulseStreamClass          *klass);
static void pulse_stream_init                (PulseStream               *stream);
static void pulse_stream_dispose             (GObject                   *object);
static void pulse_stream_finalize            (GObject                   *object);

G_DEFINE_ABSTRACT_TYPE_WITH_CODE (PulseStream, pulse_stream, G_TYPE_OBJECT,
                                  G_IMPLEMENT_INTERFACE (MATE_MIXER_TYPE_STREAM,
                                                         mate_mixer_stream_interface_init))

/* Interface implementation */
static const gchar *            stream_get_name               (MateMixerStream          *stream);
static const gchar *            stream_get_description        (MateMixerStream          *stream);
static MateMixerDevice *        stream_get_device             (MateMixerStream          *stream);
static MateMixerStreamFlags     stream_get_flags              (MateMixerStream          *stream);
static MateMixerStreamState     stream_get_state              (MateMixerStream          *stream);
static gboolean                 stream_get_mute               (MateMixerStream          *stream);
static gboolean                 stream_set_mute               (MateMixerStream          *stream,
                                                               gboolean                  mute);
static guint                    stream_get_num_channels       (MateMixerStream          *stream);
static guint                    stream_get_volume             (MateMixerStream          *stream);
static gboolean                 stream_set_volume             (MateMixerStream          *stream,
                                                               guint                     volume);
static gdouble                  stream_get_decibel            (MateMixerStream          *stream);
static gboolean                 stream_set_decibel            (MateMixerStream          *stream,
                                                               gdouble                   decibel);
static MateMixerChannelPosition stream_get_channel_position   (MateMixerStream          *stream,
                                                               guint                     channel);
static guint                    stream_get_channel_volume     (MateMixerStream          *stream,
                                                               guint                     channel);
static gboolean                 stream_set_channel_volume     (MateMixerStream          *stream,
                                                               guint                     channel,
                                                               guint                     volume);
static gdouble                  stream_get_channel_decibel    (MateMixerStream          *stream,
                                                               guint                     channel);
static gboolean                 stream_set_channel_decibel    (MateMixerStream          *stream,
                                                               guint                     channel,
                                                               gdouble                   decibel);
static gboolean                 stream_has_position           (MateMixerStream          *stream,
                                                               MateMixerChannelPosition  position);
static guint                    stream_get_position_volume    (MateMixerStream          *stream,
                                                               MateMixerChannelPosition  position);
static gboolean                 stream_set_position_volume    (MateMixerStream          *stream,
                                                               MateMixerChannelPosition  position,
                                                               guint                     volume);
static gdouble                  stream_get_position_decibel   (MateMixerStream          *stream,
                                                               MateMixerChannelPosition  position);
static gboolean                 stream_set_position_decibel   (MateMixerStream          *stream,
                                                               MateMixerChannelPosition  position,
                                                               gdouble                   decibel);
static gfloat                   stream_get_balance            (MateMixerStream          *stream);
static gboolean                 stream_set_balance            (MateMixerStream          *stream,
                                                               gfloat                    balance);
static gfloat                   stream_get_fade               (MateMixerStream          *stream);
static gboolean                 stream_set_fade               (MateMixerStream          *stream,
                                                               gfloat                    fade);
static gboolean                 stream_suspend                (MateMixerStream          *stream);
static gboolean                 stream_resume                 (MateMixerStream          *stream);

static gboolean                 stream_monitor_start          (MateMixerStream          *stream);
static void                     stream_monitor_stop           (MateMixerStream          *stream);
static gboolean                 stream_monitor_is_running     (MateMixerStream          *stream);
static void                     stream_monitor_value          (PulseMonitor             *monitor,
                                                               gdouble                   value,
                                                               MateMixerStream          *stream);

static const GList *            stream_list_ports             (MateMixerStream          *stream);
static MateMixerPort *          stream_get_active_port        (MateMixerStream          *stream);
static gboolean                 stream_set_active_port        (MateMixerStream          *stream,
                                                               const gchar              *port_name);

static guint                    stream_get_min_volume         (MateMixerStream          *stream);
static guint                    stream_get_max_volume         (MateMixerStream          *stream);
static guint                    stream_get_normal_volume      (MateMixerStream          *stream);
static guint                    stream_get_base_volume        (MateMixerStream          *stream);

static gboolean                 stream_set_cvolume            (MateMixerStream          *stream,
                                                               pa_cvolume               *volume);
static gint                     stream_compare_ports          (gconstpointer             a,
                                                               gconstpointer             b);

static void
mate_mixer_stream_interface_init (MateMixerStreamInterface *iface)
{
    iface->get_name                 = stream_get_name;
    iface->get_description          = stream_get_description;
    iface->get_device               = stream_get_device;
    iface->get_flags                = stream_get_flags;
    iface->get_state                = stream_get_state;
    iface->get_mute                 = stream_get_mute;
    iface->set_mute                 = stream_set_mute;
    iface->get_num_channels         = stream_get_num_channels;
    iface->get_volume               = stream_get_volume;
    iface->set_volume               = stream_set_volume;
    iface->get_decibel              = stream_get_decibel;
    iface->set_decibel              = stream_set_decibel;
    iface->get_channel_position     = stream_get_channel_position;
    iface->get_channel_volume       = stream_get_channel_volume;
    iface->set_channel_volume       = stream_set_channel_volume;
    iface->get_channel_decibel      = stream_get_channel_decibel;
    iface->set_channel_decibel      = stream_set_channel_decibel;
    iface->has_position             = stream_has_position;
    iface->get_position_volume      = stream_get_position_volume;
    iface->set_position_volume      = stream_set_position_volume;
    iface->get_position_decibel     = stream_get_position_decibel;
    iface->set_position_decibel     = stream_set_position_decibel;
    iface->get_balance              = stream_get_balance;
    iface->set_balance              = stream_set_balance;
    iface->get_fade                 = stream_get_fade;
    iface->set_fade                 = stream_set_fade;
    iface->suspend                  = stream_suspend;
    iface->resume                   = stream_resume;
    iface->monitor_start            = stream_monitor_start;
    iface->monitor_stop             = stream_monitor_stop;
    iface->monitor_is_running       = stream_monitor_is_running;
    iface->list_ports               = stream_list_ports;
    iface->get_active_port          = stream_get_active_port;
    iface->set_active_port          = stream_set_active_port;
    iface->get_min_volume           = stream_get_min_volume;
    iface->get_max_volume           = stream_get_max_volume;
    iface->get_normal_volume        = stream_get_normal_volume;
    iface->get_base_volume          = stream_get_base_volume;
}

static void
pulse_stream_get_property (GObject    *object,
                           guint       param_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
    PulseStream *stream;

    stream = PULSE_STREAM (object);

    switch (param_id) {
    case PROP_NAME:
        g_value_set_string (value, stream->priv->name);
        break;
    case PROP_DESCRIPTION:
        g_value_set_string (value, stream->priv->description);
        break;
    case PROP_DEVICE:
        g_value_set_object (value, stream->priv->device);
        break;
    case PROP_FLAGS:
        g_value_set_flags (value, stream->priv->flags);
        break;
    case PROP_STATE:
        g_value_set_enum (value, stream->priv->state);
        break;
    case PROP_MUTE:
        g_value_set_boolean (value, stream->priv->mute);
        break;
    case PROP_NUM_CHANNELS:
        g_value_set_uint (value, stream_get_num_channels (MATE_MIXER_STREAM (stream)));
        break;
    case PROP_VOLUME:
        g_value_set_int64 (value, stream_get_volume (MATE_MIXER_STREAM (stream)));
        break;
    case PROP_BALANCE:
        g_value_set_float (value, stream->priv->balance);
        break;
    case PROP_FADE:
        g_value_set_float (value, stream->priv->fade);
        break;
    case PROP_PORTS:
        g_value_set_pointer (value, stream->priv->ports);
        break;
    case PROP_ACTIVE_PORT:
        g_value_set_object (value, stream->priv->port);
        break;
    case PROP_INDEX:
        g_value_set_uint (value, stream->priv->index);
        break;
    case PROP_CONNECTION:
        g_value_set_object (value, stream->priv->connection);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
        break;
    }
}

static void
pulse_stream_set_property (GObject      *object,
                           guint         param_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
    PulseStream *stream;

    stream = PULSE_STREAM (object);

    switch (param_id) {
    case PROP_INDEX:
        stream->priv->index = g_value_get_uint (value);
        break;
    case PROP_CONNECTION:
        /* Construct-only object */
        stream->priv->connection = g_value_dup_object (value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
        break;
    }
}

static void
pulse_stream_class_init (PulseStreamClass *klass)
{
    GObjectClass *object_class;

    object_class = G_OBJECT_CLASS (klass);
    object_class->dispose      = pulse_stream_dispose;
    object_class->finalize     = pulse_stream_finalize;
    object_class->get_property = pulse_stream_get_property;
    object_class->set_property = pulse_stream_set_property;

    g_object_class_install_property (object_class,
                                     PROP_INDEX,
                                     g_param_spec_uint ("index",
                                                        "Index",
                                                        "Stream index",
                                                        0,
                                                        G_MAXUINT,
                                                        0,
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_STATIC_STRINGS));

    g_object_class_install_property (object_class,
                                     PROP_CONNECTION,
                                     g_param_spec_object ("connection",
                                                          "Connection",
                                                          "PulseAudio connection",
                                                          PULSE_TYPE_CONNECTION,
                                                          G_PARAM_CONSTRUCT_ONLY |
                                                          G_PARAM_READWRITE |
                                                          G_PARAM_STATIC_STRINGS));

    g_object_class_override_property (object_class, PROP_NAME, "name");
    g_object_class_override_property (object_class, PROP_DESCRIPTION, "description");
    g_object_class_override_property (object_class, PROP_DEVICE, "device");
    g_object_class_override_property (object_class, PROP_FLAGS, "flags");
    g_object_class_override_property (object_class, PROP_STATE, "state");
    g_object_class_override_property (object_class, PROP_MUTE, "mute");
    g_object_class_override_property (object_class, PROP_NUM_CHANNELS, "num-channels");
    g_object_class_override_property (object_class, PROP_VOLUME, "volume");
    g_object_class_override_property (object_class, PROP_BALANCE, "balance");
    g_object_class_override_property (object_class, PROP_FADE, "fade");
    g_object_class_override_property (object_class, PROP_PORTS, "ports");
    g_object_class_override_property (object_class, PROP_ACTIVE_PORT, "active-port");

    g_type_class_add_private (object_class, sizeof (PulseStreamPrivate));
}

static void
pulse_stream_init (PulseStream *stream)
{
    stream->priv = G_TYPE_INSTANCE_GET_PRIVATE (stream,
                                                PULSE_TYPE_STREAM,
                                                PulseStreamPrivate);
}

static void
pulse_stream_dispose (GObject *object)
{
    PulseStream *stream;

    stream = PULSE_STREAM (object);

    if (stream->priv->ports) {
        g_list_free_full (stream->priv->ports, g_object_unref);
        stream->priv->ports = NULL;
    }

    g_clear_object (&stream->priv->port);
    g_clear_object (&stream->priv->device);
    g_clear_object (&stream->priv->monitor);
    g_clear_object (&stream->priv->connection);

    G_OBJECT_CLASS (pulse_stream_parent_class)->dispose (object);
}

static void
pulse_stream_finalize (GObject *object)
{
    PulseStream *stream;

    stream = PULSE_STREAM (object);

    g_free (stream->priv->name);
    g_free (stream->priv->description);

    G_OBJECT_CLASS (pulse_stream_parent_class)->finalize (object);
}

guint32
pulse_stream_get_index (PulseStream *stream)
{
    g_return_val_if_fail (PULSE_IS_STREAM (stream), 0);

    return stream->priv->index;
}

PulseConnection *
pulse_stream_get_connection (PulseStream *stream)
{
    g_return_val_if_fail (PULSE_IS_STREAM (stream), NULL);

    return stream->priv->connection;
}

PulseMonitor *
pulse_stream_get_monitor (PulseStream *stream)
{
    g_return_val_if_fail (PULSE_IS_STREAM (stream), NULL);

    return stream->priv->monitor;
}

gboolean
pulse_stream_update_name (PulseStream *stream, const gchar *name)
{
    g_return_val_if_fail (PULSE_IS_STREAM (stream), FALSE);

    /* Allow the name to be NULL */
    if (g_strcmp0 (name, stream->priv->name)) {
        g_free (stream->priv->name);
        stream->priv->name = g_strdup (name);

        g_object_notify (G_OBJECT (stream), "name");
    }
    return TRUE;
}

gboolean
pulse_stream_update_description (PulseStream *stream, const gchar *description)
{
    g_return_val_if_fail (PULSE_IS_STREAM (stream), FALSE);

    /* Allow the description to be NULL */
    if (g_strcmp0 (description, stream->priv->description)) {
        g_free (stream->priv->description);
        stream->priv->description = g_strdup (description);

        g_object_notify (G_OBJECT (stream), "description");
    }
    return TRUE;
}

gboolean
pulse_stream_update_device (PulseStream *stream, MateMixerDevice *device)
{
    g_return_val_if_fail (PULSE_IS_STREAM (stream), FALSE);

    if (stream->priv->device != device) {
        g_clear_object (&stream->priv->device);

        if (G_LIKELY (device != NULL))
            stream->priv->device = g_object_ref (device);

        g_object_notify (G_OBJECT (stream), "device");
    }
    return TRUE;
}

gboolean
pulse_stream_update_flags (PulseStream *stream, MateMixerStreamFlags flags)
{
    g_return_val_if_fail (PULSE_IS_STREAM (stream), FALSE);

    if (stream->priv->flags != flags) {
        stream->priv->flags = flags;
        g_object_notify (G_OBJECT (stream), "flags");
    }
    return TRUE;
}

gboolean
pulse_stream_update_state (PulseStream *stream, MateMixerStreamState state)
{
    g_return_val_if_fail (PULSE_IS_STREAM (stream), FALSE);

    if (stream->priv->state != state) {
        stream->priv->state = state;
        g_object_notify (G_OBJECT (stream), "state");
    }
    return TRUE;
}

gboolean
pulse_stream_update_mute (PulseStream *stream, gboolean mute)
{
    g_return_val_if_fail (PULSE_IS_STREAM (stream), FALSE);

    if (stream->priv->mute != mute) {
        stream->priv->mute = mute;
        g_object_notify (G_OBJECT (stream), "mute");
    }
    return TRUE;
}

gboolean
pulse_stream_update_volume (PulseStream          *stream,
                            const pa_cvolume     *volume,
                            const pa_channel_map *map,
                            pa_volume_t           base_volume)
{
    gfloat fade = 0.0f;
    gfloat balance = 0.0f;

    g_return_val_if_fail (PULSE_IS_STREAM (stream), FALSE);

    /* The channel map should be always present, but volume is not always
     * supported and might be NULL */
    if (G_LIKELY (map != NULL)) {
        if (!pa_channel_map_equal (&stream->priv->channel_map, map))
            stream->priv->channel_map = *map;
    }

    if (volume != NULL) {
        if (!pa_cvolume_equal (&stream->priv->volume, volume)) {
            stream->priv->volume = *volume;

            g_object_notify (G_OBJECT (stream), "volume");
        }

        stream->priv->base_volume = (base_volume > 0)
            ? base_volume
            : PA_VOLUME_NORM;

        /* Fade and balance need a valid channel map and volume, otherwise
         * compare against the default values */
        fade = pa_cvolume_get_fade (volume, &stream->priv->channel_map);
        balance = pa_cvolume_get_balance (volume, &stream->priv->channel_map);
    } else {
        stream->priv->base_volume = PA_VOLUME_NORM;
    }

    if (stream->priv->balance != balance) {
        stream->priv->balance = balance;
        g_object_notify (G_OBJECT (stream), "balance");
    }

    if (stream->priv->fade != fade) {
        stream->priv->fade = fade;
        g_object_notify (G_OBJECT (stream), "fade");
    }
    return TRUE;
}

gboolean
pulse_stream_update_ports (PulseStream *stream, GList *ports)
{
    g_return_val_if_fail (PULSE_IS_STREAM (stream), FALSE);

    if (stream->priv->ports)
        g_list_free_full (stream->priv->ports, g_object_unref);

    if (ports)
        stream->priv->ports = g_list_sort (ports, stream_compare_ports);
    else
        stream->priv->ports = NULL;

    g_object_notify (G_OBJECT (stream), "ports");
    return TRUE;
}

gboolean
pulse_stream_update_active_port (PulseStream *stream, const gchar *port_name)
{
    GList         *list;
    MateMixerPort *port = NULL;

    g_return_val_if_fail (PULSE_IS_STREAM (stream), FALSE);

    list = stream->priv->ports;
    while (list) {
        port = MATE_MIXER_PORT (list->data);

        if (!g_strcmp0 (mate_mixer_port_get_name (port), port_name))
            break;

        port = NULL;
        list = list->next;
    }

    if (stream->priv->port != port) {
        if (stream->priv->port)
            g_clear_object (&stream->priv->port);
        if (port)
            stream->priv->port = g_object_ref (port);

        g_object_notify (G_OBJECT (stream), "active-port");
    }
    return TRUE;
}

static const gchar *
stream_get_name (MateMixerStream *stream)
{
    g_return_val_if_fail (PULSE_IS_STREAM (stream), NULL);

    return PULSE_STREAM (stream)->priv->name;
}

static const gchar *
stream_get_description (MateMixerStream *stream)
{
    g_return_val_if_fail (PULSE_IS_STREAM (stream), NULL);

    return PULSE_STREAM (stream)->priv->description;
}

static MateMixerDevice *
stream_get_device (MateMixerStream *stream)
{
    g_return_val_if_fail (PULSE_IS_STREAM (stream), NULL);

    return PULSE_STREAM (stream)->priv->device;
}

static MateMixerStreamFlags
stream_get_flags (MateMixerStream *stream)
{
    g_return_val_if_fail (PULSE_IS_STREAM (stream), MATE_MIXER_STREAM_NO_FLAGS);

    return PULSE_STREAM (stream)->priv->flags;
}

static MateMixerStreamState
stream_get_state (MateMixerStream *stream)
{
    g_return_val_if_fail (PULSE_IS_STREAM (stream), MATE_MIXER_STREAM_UNKNOWN_STATE);

    return PULSE_STREAM (stream)->priv->state;
}

static gboolean
stream_get_mute (MateMixerStream *stream)
{
    g_return_val_if_fail (PULSE_IS_STREAM (stream), FALSE);

    return PULSE_STREAM (stream)->priv->mute;
}

static gboolean
stream_set_mute (MateMixerStream *stream, gboolean mute)
{
    PulseStream *pulse;

    g_return_val_if_fail (PULSE_IS_STREAM (stream), FALSE);

    pulse = PULSE_STREAM (stream);

    if (pulse->priv->mute != mute) {
        if (PULSE_STREAM_GET_CLASS (stream)->set_mute (stream, mute) == FALSE)
            return FALSE;

        pulse->priv->mute = mute;
        g_object_notify (G_OBJECT (stream), "mute");
    }
    return TRUE;
}

static guint
stream_get_num_channels (MateMixerStream *stream)
{
    g_return_val_if_fail (PULSE_IS_STREAM (stream), 0);

    return PULSE_STREAM (stream)->priv->volume.channels;
}

static guint
stream_get_volume (MateMixerStream *stream)
{
    g_return_val_if_fail (PULSE_IS_STREAM (stream), 0);

    return (guint) pa_cvolume_max (&PULSE_STREAM (stream)->priv->volume);
}

static gboolean
stream_set_volume (MateMixerStream *stream, guint volume)
{
    PulseStream  *pulse;
    pa_cvolume    cvolume;

    g_return_val_if_fail (PULSE_IS_STREAM (stream), FALSE);

    if (!(mate_mixer_stream_get_flags (stream) & MATE_MIXER_STREAM_CAN_SET_VOLUME))
        return FALSE;

    pulse = PULSE_STREAM (stream);
    cvolume = pulse->priv->volume;

    if (pa_cvolume_scale (&cvolume, (pa_volume_t) volume) == NULL)
        return FALSE;

    return stream_set_cvolume (stream, &cvolume);
}

static gdouble
stream_get_decibel (MateMixerStream *stream)
{
    gdouble value;

    g_return_val_if_fail (PULSE_IS_STREAM (stream), -MATE_MIXER_INFINITY);

    if (!(mate_mixer_stream_get_flags (stream) & MATE_MIXER_STREAM_HAS_DECIBEL_VOLUME))
        return -MATE_MIXER_INFINITY;

    value = pa_sw_volume_to_dB (stream_get_volume (stream));

    return (value == PA_DECIBEL_MININFTY) ? -MATE_MIXER_INFINITY : value;
}

static gboolean
stream_set_decibel (MateMixerStream *stream, gdouble decibel)
{
    g_return_val_if_fail (PULSE_IS_STREAM (stream), FALSE);

    if (!(mate_mixer_stream_get_flags (stream) & MATE_MIXER_STREAM_HAS_DECIBEL_VOLUME) ||
        !(mate_mixer_stream_get_flags (stream) & MATE_MIXER_STREAM_CAN_SET_VOLUME))
        return FALSE;

    return stream_set_volume (stream, pa_sw_volume_from_dB (decibel));
}

static MateMixerChannelPosition
stream_get_channel_position (MateMixerStream *stream, guint channel)
{
    PulseStream *pulse;

    g_return_val_if_fail (PULSE_IS_STREAM (stream), MATE_MIXER_CHANNEL_UNKNOWN_POSITION);

    pulse = PULSE_STREAM (stream);

    if (channel >= pulse->priv->channel_map.channels)
        return MATE_MIXER_CHANNEL_UNKNOWN_POSITION;

    return pulse_convert_position_to_pulse (pulse->priv->channel_map.map[channel]);
}

static guint
stream_get_channel_volume (MateMixerStream *stream, guint channel)
{
    PulseStream *pulse;

    g_return_val_if_fail (PULSE_IS_STREAM (stream), 0);

    pulse = PULSE_STREAM (stream);

    if (channel >= pulse->priv->volume.channels)
        return stream_get_min_volume (stream);

    return (guint) pulse->priv->volume.values[channel];
}

static gboolean
stream_set_channel_volume (MateMixerStream *stream, guint channel, guint volume)
{
    PulseStream  *pulse;
    pa_cvolume    cvolume;

    g_return_val_if_fail (PULSE_IS_STREAM (stream), FALSE);

    pulse = PULSE_STREAM (stream);
    cvolume = pulse->priv->volume;

    if (channel >= pulse->priv->volume.channels)
        return FALSE;

    cvolume.values[channel] = (pa_volume_t) volume;

    return stream_set_cvolume (stream, &cvolume);
}

static gdouble
stream_get_channel_decibel (MateMixerStream *stream, guint channel)
{
    PulseStream *pulse;
    gdouble      value;

    g_return_val_if_fail (PULSE_IS_STREAM (stream), -MATE_MIXER_INFINITY);

    if (!(mate_mixer_stream_get_flags (stream) & MATE_MIXER_STREAM_HAS_DECIBEL_VOLUME))
        return -MATE_MIXER_INFINITY;

    pulse = PULSE_STREAM (stream);

    if (channel >= pulse->priv->volume.channels)
        return -MATE_MIXER_INFINITY;

    value = pa_sw_volume_to_dB (pulse->priv->volume.values[channel]);

    return (value == PA_DECIBEL_MININFTY) ? -MATE_MIXER_INFINITY : value;
}

static gboolean
stream_set_channel_decibel (MateMixerStream *stream,
                            guint            channel,
                            gdouble          decibel)
{
    g_return_val_if_fail (PULSE_IS_STREAM (stream), FALSE);

    if (!(mate_mixer_stream_get_flags (stream) & MATE_MIXER_STREAM_HAS_DECIBEL_VOLUME) ||
        !(mate_mixer_stream_get_flags (stream) & MATE_MIXER_STREAM_CAN_SET_VOLUME))
        return FALSE;

    return stream_set_channel_volume (stream, channel, pa_sw_volume_from_dB (decibel));
}

static gboolean
stream_has_position (MateMixerStream *stream, MateMixerChannelPosition position)
{
    PulseStream *pulse;

    g_return_val_if_fail (PULSE_IS_STREAM (stream), FALSE);

    pulse = PULSE_STREAM (stream);

    return pa_channel_map_has_position (&pulse->priv->channel_map,
                                        pulse_convert_position_to_pulse (position));
}

static guint
stream_get_position_volume (MateMixerStream          *stream,
                            MateMixerChannelPosition  position)
{
    PulseStream *pulse;

    g_return_val_if_fail (PULSE_IS_STREAM (stream), 0);

    pulse = PULSE_STREAM (stream);

    return pa_cvolume_get_position (&pulse->priv->volume,
                                    &pulse->priv->channel_map,
                                    pulse_convert_position_to_pulse (position));
}

static gboolean
stream_set_position_volume (MateMixerStream          *stream,
                            MateMixerChannelPosition  position,
                            guint                     volume)
{
    PulseStream *pulse;
    pa_cvolume   cvolume;

    g_return_val_if_fail (PULSE_IS_STREAM (stream), FALSE);

    pulse = PULSE_STREAM (stream);
    cvolume = pulse->priv->volume;

    if (!pa_cvolume_set_position (&cvolume,
                                  &pulse->priv->channel_map,
                                  pulse_convert_position_to_pulse (position),
                                  (pa_volume_t) volume))
        return FALSE;

    return stream_set_cvolume (stream, &cvolume);
}

static gdouble
stream_get_position_decibel (MateMixerStream          *stream,
                             MateMixerChannelPosition  position)
{
    gdouble value;

    g_return_val_if_fail (PULSE_IS_STREAM (stream), -MATE_MIXER_INFINITY);

    if (!(mate_mixer_stream_get_flags (stream) & MATE_MIXER_STREAM_HAS_DECIBEL_VOLUME))
        return -MATE_MIXER_INFINITY;

    value = pa_sw_volume_to_dB (stream_get_position_volume (stream, position));

    return (value == PA_DECIBEL_MININFTY) ? -MATE_MIXER_INFINITY : value;
}

static gboolean
stream_set_position_decibel (MateMixerStream          *stream,
                             MateMixerChannelPosition  position,
                             gdouble                   decibel)
{
    g_return_val_if_fail (PULSE_IS_STREAM (stream), FALSE);

    if (!(mate_mixer_stream_get_flags (stream) & MATE_MIXER_STREAM_HAS_DECIBEL_VOLUME) ||
        !(mate_mixer_stream_get_flags (stream) & MATE_MIXER_STREAM_CAN_SET_VOLUME))
        return FALSE;

    return stream_set_position_volume (stream, position, pa_sw_volume_from_dB (decibel));
}

static gfloat
stream_get_balance (MateMixerStream *stream)
{
    g_return_val_if_fail (PULSE_IS_STREAM (stream), 0.0f);

    return PULSE_STREAM (stream)->priv->balance;
}

static gboolean
stream_set_balance (MateMixerStream *stream, gfloat balance)
{
    PulseStream *pulse;
    pa_cvolume   cvolume;

    g_return_val_if_fail (PULSE_IS_STREAM (stream), FALSE);

    pulse = PULSE_STREAM (stream);
    cvolume = pulse->priv->volume;

    if (pa_cvolume_set_balance (&cvolume, &pulse->priv->channel_map, balance) == NULL)
        return FALSE;

    return stream_set_cvolume (stream, &cvolume);
}

static gfloat
stream_get_fade (MateMixerStream *stream)
{
    g_return_val_if_fail (PULSE_IS_STREAM (stream), 0.0f);

    return PULSE_STREAM (stream)->priv->fade;
}

static gboolean
stream_set_fade (MateMixerStream *stream, gfloat fade)
{
    PulseStream *pulse;
    pa_cvolume   cvolume;

    g_return_val_if_fail (PULSE_IS_STREAM (stream), FALSE);

    pulse = PULSE_STREAM (stream);
    cvolume = pulse->priv->volume;

    if (pa_cvolume_set_fade (&cvolume, &pulse->priv->channel_map, fade) == NULL)
        return FALSE;

    return stream_set_cvolume (stream, &cvolume);
}

static gboolean
stream_suspend (MateMixerStream *stream)
{
    g_return_val_if_fail (PULSE_IS_STREAM (stream), FALSE);

    if (!(PULSE_STREAM (stream)->priv->flags & MATE_MIXER_STREAM_CAN_SUSPEND))
        return FALSE;

    return PULSE_STREAM_GET_CLASS (stream)->suspend (stream);
}

static gboolean
stream_resume (MateMixerStream *stream)
{
    g_return_val_if_fail (PULSE_IS_STREAM (stream), FALSE);

    if (!(PULSE_STREAM (stream)->priv->flags & MATE_MIXER_STREAM_CAN_SUSPEND))
        return FALSE;

    return PULSE_STREAM_GET_CLASS (stream)->resume (stream);
}

// XXX allow to provide custom translated monitor name
static gboolean
stream_monitor_start (MateMixerStream *stream)
{
    PulseStream *pulse;

    g_return_val_if_fail (PULSE_IS_STREAM (stream), FALSE);

    pulse = PULSE_STREAM (stream);

    if (!pulse->priv->monitor) {
        pulse->priv->monitor = PULSE_STREAM_GET_CLASS (stream)->create_monitor (stream);

        if (G_UNLIKELY (pulse->priv->monitor == NULL))
            return FALSE;

        g_signal_connect (G_OBJECT (pulse->priv->monitor),
                          "value",
                          G_CALLBACK (stream_monitor_value),
                          stream);
    }
    return pulse_monitor_enable (pulse->priv->monitor);
}

static void
stream_monitor_stop (MateMixerStream *stream)
{
    PulseStream *pulse;

    g_return_if_fail (PULSE_IS_STREAM (stream));

    pulse = PULSE_STREAM (stream);

    if (pulse->priv->monitor)
        pulse_monitor_disable (pulse->priv->monitor);
}

static gboolean
stream_monitor_is_running (MateMixerStream *stream)
{
    PulseStream *pulse;

    g_return_val_if_fail (PULSE_IS_STREAM (stream), FALSE);

    pulse = PULSE_STREAM (stream);

    if (pulse->priv->monitor)
        return pulse_monitor_is_enabled (pulse->priv->monitor);

    return FALSE;
}

static void
stream_monitor_value (PulseMonitor *monitor, gdouble value, MateMixerStream *stream)
{
    g_signal_emit_by_name (G_OBJECT (stream),
                           "monitor-value",
                           value);
}

static const GList *
stream_list_ports (MateMixerStream *stream)
{
    g_return_val_if_fail (PULSE_IS_STREAM (stream), NULL);

    return (const GList *) PULSE_STREAM (stream)->priv->ports;
}

static MateMixerPort *
stream_get_active_port (MateMixerStream *stream)
{
    g_return_val_if_fail (PULSE_IS_STREAM (stream), NULL);

    return PULSE_STREAM (stream)->priv->port;
}

static gboolean
stream_set_active_port (MateMixerStream *stream, const gchar *port_name)
{
    PulseStream   *pulse;
    GList         *list;
    MateMixerPort *port = NULL;

    g_return_val_if_fail (PULSE_IS_STREAM (stream), FALSE);
    g_return_val_if_fail (port_name != NULL, FALSE);

    pulse = PULSE_STREAM (stream);
    list  = pulse->priv->ports;
    while (list) {
        port = MATE_MIXER_PORT (list->data);

        if (!g_strcmp0 (mate_mixer_port_get_name (port), port_name))
            break;

        port = NULL;
        list = list->next;
    }

    if (port == NULL ||
        PULSE_STREAM_GET_CLASS (stream)->set_active_port (stream, port_name) == FALSE)
        return FALSE;

    if (pulse->priv->port)
        g_object_unref (pulse->priv->port);

    pulse->priv->port = g_object_ref (port);

    g_object_notify (G_OBJECT (stream), "active-port");
    return TRUE;
}

static guint
stream_get_min_volume (MateMixerStream *stream)
{
    return (guint) PA_VOLUME_MUTED;
}

static guint
stream_get_max_volume (MateMixerStream *stream)
{
    return (guint) PA_VOLUME_UI_MAX;
}

static guint
stream_get_normal_volume (MateMixerStream *stream)
{
    return (guint) PA_VOLUME_NORM;
}

static guint
stream_get_base_volume (MateMixerStream *stream)
{
    g_return_val_if_fail (PULSE_IS_STREAM (stream), 0);

    return PULSE_STREAM (stream)->priv->base_volume;
}

static gboolean
stream_set_cvolume (MateMixerStream *stream, pa_cvolume *volume)
{
    PulseStream *pulse;

    if (!pa_cvolume_valid (volume))
        return FALSE;

    pulse = PULSE_STREAM (stream);

    if (!pa_cvolume_equal (volume, &pulse->priv->volume)) {
        if (PULSE_STREAM_GET_CLASS (stream)->set_volume (stream, volume) == FALSE)
            return FALSE;

        pulse->priv->volume = *volume;
        g_object_notify (G_OBJECT (stream), "volume");

        // XXX notify fade and balance
    }
    return TRUE;
}

static gint
stream_compare_ports (gconstpointer a, gconstpointer b)
{
    MateMixerPort *p1 = MATE_MIXER_PORT (a);
    MateMixerPort *p2 = MATE_MIXER_PORT (b);

    gint ret = (gint) (mate_mixer_port_get_priority (p2) -
                       mate_mixer_port_get_priority (p1));
    if (ret != 0)
        return ret;
    else
        return strcmp (mate_mixer_port_get_name (p1),
                       mate_mixer_port_get_name (p2));
}
