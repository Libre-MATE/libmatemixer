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

#ifndef MATEMIXER_ENUMS_H
#define MATEMIXER_ENUMS_H

/*
 * GTypes are not generated by glib-mkenums, see:
 * https://bugzilla.gnome.org/show_bug.cgi?id=621942
 */

/**
 * MateMixerState:
 * @MATE_MIXER_STATE_IDLE:
 *     Not connected.
 * @MATE_MIXER_STATE_CONNECTING:
 *     Connection is in progress.
 * @MATE_MIXER_STATE_READY:
 *     Connected.
 * @MATE_MIXER_STATE_FAILED:
 *     Connection has failed.
 * @MATE_MIXER_STATE_UNKNOWN:
 *     Unknown state. This state is used as an error indicator.
 *
 * State of a connection to a sound system.
 */
typedef enum {
  MATE_MIXER_STATE_IDLE,
  MATE_MIXER_STATE_CONNECTING,
  MATE_MIXER_STATE_READY,
  MATE_MIXER_STATE_FAILED,
  MATE_MIXER_STATE_UNKNOWN
} MateMixerState;

/**
 * MateMixerBackendType:
 * @MATE_MIXER_BACKEND_UNKNOWN:
 *     Unknown or undefined sound system backend type.
 * @MATE_MIXER_BACKEND_PULSEAUDIO:
 *     PulseAudio sound system backend. It has the highest priority and
 *     will be the first one to try when you call mate_mixer_context_open(),
 *     unless you select a specific sound system to connect to.
 * @MATE_MIXER_BACKEND_ALSA:
 *     The Advanced Linux Sound Architecture sound system.
 * @MATE_MIXER_BACKEND_OSS:
 *     The Open Sound System.
 * @MATE_MIXER_BACKEND_NULL:
 *     Fallback backend which never fails to initialize, but provides no
 *     functionality. This backend has the lowest priority and will be used
 *     if you do not select a specific backend and it isn't possible to use
 *     any of the other backends.
 *
 * Constants identifying a sound system backend.
 */
typedef enum {
  MATE_MIXER_BACKEND_UNKNOWN,
  MATE_MIXER_BACKEND_PULSEAUDIO,
  MATE_MIXER_BACKEND_ALSA,
  MATE_MIXER_BACKEND_OSS,
  MATE_MIXER_BACKEND_NULL
} MateMixerBackendType;

/**
 * MateMixerBackendFlags:
 * @MATE_MIXER_BACKEND_NO_FLAGS:
 *     No flags.
 * @MATE_MIXER_BACKEND_HAS_APPLICATION_CONTROLS:
 *     The sound system backend includes support for application stream
 * controls, allowing per-application volume control.
 * @MATE_MIXER_BACKEND_HAS_STORED_CONTROLS:
 *     The sound system backend includes support for stored controls. See the
 *     #MateMixerStoredControl description for more information.
 *     The presence of this flag does not guarantee that this feature is enabled
 *     in the sound system's configuration.
 * @MATE_MIXER_BACKEND_CAN_SET_DEFAULT_INPUT_STREAM:
 *     The sound system backend is able to change the current default input
 * stream using the mate_mixer_context_set_default_input_stream() function.
 * @MATE_MIXER_BACKEND_CAN_SET_DEFAULT_OUTPUT_STREAM:
 *     The sound system backend is able to change the current default output
 * stream using the mate_mixer_context_set_default_output_stream() function.
 *
 * Flags describing capabilities of a sound system.
 */
typedef enum { /*< flags >*/
               MATE_MIXER_BACKEND_NO_FLAGS = 0,
               MATE_MIXER_BACKEND_HAS_APPLICATION_CONTROLS = 1 << 0,
               MATE_MIXER_BACKEND_HAS_STORED_CONTROLS = 1 << 1,
               MATE_MIXER_BACKEND_CAN_SET_DEFAULT_INPUT_STREAM = 1 << 2,
               MATE_MIXER_BACKEND_CAN_SET_DEFAULT_OUTPUT_STREAM = 1 << 3
} MateMixerBackendFlags;

/**
 * MateMixerDirection:
 * @MATE_MIXER_DIRECTION_UNKNOWN:
 *     Unknown direction.
 * @MATE_MIXER_DIRECTION_INPUT:
 *     Input direction (recording).
 * @MATE_MIXER_DIRECTION_OUTPUT:
 *     Output direction (playback).
 *
 * Sound stream direction.
 */
typedef enum {
  MATE_MIXER_DIRECTION_UNKNOWN,
  MATE_MIXER_DIRECTION_INPUT,
  MATE_MIXER_DIRECTION_OUTPUT,
} MateMixerDirection;

/**
 * MateMixerStreamControlFlags:
 * @MATE_MIXER_STREAM_CONTROL_NO_FLAGS:
 *     No flags.
 * @MATE_MIXER_STREAM_CONTROL_MUTE_READABLE:
 *     The stream control includes a mute toggle and allows reading the mute
 * state.
 * @MATE_MIXER_STREAM_CONTROL_MUTE_WRITABLE:
 *     The stream control includes a mute toggle and allows changing the mute
 * state.
 * @MATE_MIXER_STREAM_CONTROL_VOLUME_READABLE:
 *     The stream control includes a volume control and allows reading the
 * volume.
 * @MATE_MIXER_STREAM_CONTROL_VOLUME_WRITABLE:
 *     The stream control includes a volume control and allows changing the
 * volume.
 * @MATE_MIXER_STREAM_CONTROL_CAN_BALANCE:
 *     The stream control includes the necessary channel positions to allow
 * left/right volume balancing.
 * @MATE_MIXER_STREAM_CONTROL_CAN_FADE:
 *     The stream control includes the necessary channel positions to allow
 * front/back volume fading.
 * @MATE_MIXER_STREAM_CONTROL_MOVABLE:
 *     It is possible to move the stream control to a different stream using the
 *     mate_mixer_stream_control_set_stream() function. See the function
 * description for details.
 * @MATE_MIXER_STREAM_CONTROL_HAS_DECIBEL:
 *     The stream controls supports decibel values and it is possible to
 * successfully use the functions which operate on decibel values.
 * @MATE_MIXER_STREAM_CONTROL_HAS_MONITOR:
 *     The stream control supports peak level monitoring.
 * @MATE_MIXER_STREAM_CONTROL_STORED:
 *     The stream control is a #MateMixerStoredControl.
 *
 * Flags describing capabilities and properties of a stream control.
 */
typedef enum {
  MATE_MIXER_STREAM_CONTROL_NO_FLAGS = 0,
  MATE_MIXER_STREAM_CONTROL_MUTE_READABLE = 1 << 0,
  MATE_MIXER_STREAM_CONTROL_MUTE_WRITABLE = 1 << 1,
  MATE_MIXER_STREAM_CONTROL_VOLUME_READABLE = 1 << 2,
  MATE_MIXER_STREAM_CONTROL_VOLUME_WRITABLE = 1 << 3,
  MATE_MIXER_STREAM_CONTROL_CAN_BALANCE = 1 << 4,
  MATE_MIXER_STREAM_CONTROL_CAN_FADE = 1 << 5,
  MATE_MIXER_STREAM_CONTROL_MOVABLE = 1 << 6,
  MATE_MIXER_STREAM_CONTROL_HAS_DECIBEL = 1 << 7,
  MATE_MIXER_STREAM_CONTROL_HAS_MONITOR = 1 << 8,
  MATE_MIXER_STREAM_CONTROL_STORED = 1 << 9
} MateMixerStreamControlFlags;

/**
 * MateMixerStreamControlRole:
 * @MATE_MIXER_STREAM_CONTROL_ROLE_UNKNOWN:
 *     Unknown role.
 * @MATE_MIXER_STREAM_CONTROL_ROLE_MASTER:
 *     Master volume control.
 * @MATE_MIXER_STREAM_CONTROL_ROLE_APPLICATION:
 *     Application volume control.
 * @MATE_MIXER_STREAM_CONTROL_ROLE_PCM:
 *     PCM volume control.
 * @MATE_MIXER_STREAM_CONTROL_ROLE_SPEAKER:
 *     Speaker volume control.
 * @MATE_MIXER_STREAM_CONTROL_ROLE_MICROPHONE:
 *     Microphone volume control.
 * @MATE_MIXER_STREAM_CONTROL_ROLE_PORT:
 *     Volume control for a connector of a sound device.
 * @MATE_MIXER_STREAM_CONTROL_ROLE_BOOST:
 *     Boost control (for example a microphone boost or bass boost).
 * @MATE_MIXER_STREAM_CONTROL_ROLE_BASS:
 *     Bass control.
 * @MATE_MIXER_STREAM_CONTROL_ROLE_TREBLE:
 *     Treble control.
 * @MATE_MIXER_STREAM_CONTROL_ROLE_CD:
 *     CD input volume control.
 * @MATE_MIXER_STREAM_CONTROL_ROLE_VIDEO:
 *     Video volume control.
 * @MATE_MIXER_STREAM_CONTROL_ROLE_MUSIC:
 *     Music volume control.
 */
typedef enum {
  MATE_MIXER_STREAM_CONTROL_ROLE_UNKNOWN,
  MATE_MIXER_STREAM_CONTROL_ROLE_MASTER,
  MATE_MIXER_STREAM_CONTROL_ROLE_APPLICATION,
  MATE_MIXER_STREAM_CONTROL_ROLE_PCM,
  MATE_MIXER_STREAM_CONTROL_ROLE_SPEAKER,
  MATE_MIXER_STREAM_CONTROL_ROLE_MICROPHONE,
  MATE_MIXER_STREAM_CONTROL_ROLE_PORT,
  MATE_MIXER_STREAM_CONTROL_ROLE_BOOST,
  MATE_MIXER_STREAM_CONTROL_ROLE_BASS,
  MATE_MIXER_STREAM_CONTROL_ROLE_TREBLE,
  MATE_MIXER_STREAM_CONTROL_ROLE_CD,
  MATE_MIXER_STREAM_CONTROL_ROLE_VIDEO,
  MATE_MIXER_STREAM_CONTROL_ROLE_MUSIC
} MateMixerStreamControlRole;

/**
 * MateMixerStreamControlMediaRole:
 * @MATE_MIXER_STREAM_CONTROL_MEDIA_ROLE_UNKNOWN:
 *     Unknown media role.
 * @MATE_MIXER_STREAM_CONTROL_MEDIA_ROLE_VIDEO:
 *     Video role.
 * @MATE_MIXER_STREAM_CONTROL_MEDIA_ROLE_MUSIC:
 *     Music role.
 * @MATE_MIXER_STREAM_CONTROL_MEDIA_ROLE_GAME:
 *     Game role.
 * @MATE_MIXER_STREAM_CONTROL_MEDIA_ROLE_EVENT:
 *     Event sounds.
 * @MATE_MIXER_STREAM_CONTROL_MEDIA_ROLE_PHONE:
 * @MATE_MIXER_STREAM_CONTROL_MEDIA_ROLE_ANIMATION:
 * @MATE_MIXER_STREAM_CONTROL_MEDIA_ROLE_PRODUCTION:
 * @MATE_MIXER_STREAM_CONTROL_MEDIA_ROLE_A11Y:
 * @MATE_MIXER_STREAM_CONTROL_MEDIA_ROLE_TEST:
 * @MATE_MIXER_STREAM_CONTROL_MEDIA_ROLE_ABSTRACT:
 * @MATE_MIXER_STREAM_CONTROL_MEDIA_ROLE_FILTER:
 *
 * Constants describing a media role of a control. These constants are mapped
 * to PulseAudio media role property and therefore are only available when using
 * the PulseAudio sound system.
 *
 * Media roles are commonly set by applications to indicate what kind of sound
 * input/output they provide and may be the defining property of stored controls
 * (for example an event role stored control can be used to provide a volume
 * slider for event sounds).
 *
 * See the PulseAudio documentation for more detailed information about media
 * roles.
 */
typedef enum {
  MATE_MIXER_STREAM_CONTROL_MEDIA_ROLE_UNKNOWN,
  MATE_MIXER_STREAM_CONTROL_MEDIA_ROLE_VIDEO,
  MATE_MIXER_STREAM_CONTROL_MEDIA_ROLE_MUSIC,
  MATE_MIXER_STREAM_CONTROL_MEDIA_ROLE_GAME,
  MATE_MIXER_STREAM_CONTROL_MEDIA_ROLE_EVENT,
  MATE_MIXER_STREAM_CONTROL_MEDIA_ROLE_PHONE,
  MATE_MIXER_STREAM_CONTROL_MEDIA_ROLE_ANIMATION,
  MATE_MIXER_STREAM_CONTROL_MEDIA_ROLE_PRODUCTION,
  MATE_MIXER_STREAM_CONTROL_MEDIA_ROLE_A11Y,
  MATE_MIXER_STREAM_CONTROL_MEDIA_ROLE_TEST,
  MATE_MIXER_STREAM_CONTROL_MEDIA_ROLE_ABSTRACT,
  MATE_MIXER_STREAM_CONTROL_MEDIA_ROLE_FILTER
} MateMixerStreamControlMediaRole;

/**
 * MateMixerDeviceSwitchRole:
 * @MATE_MIXER_DEVICE_SWITCH_ROLE_UNKNOWN:
 *     Unknown device switch role.
 * @MATE_MIXER_DEVICE_SWITCH_ROLE_PROFILE:
 *     The switch changes the active sound device profile.
 */
typedef enum {
  MATE_MIXER_DEVICE_SWITCH_ROLE_UNKNOWN,
  MATE_MIXER_DEVICE_SWITCH_ROLE_PROFILE,
} MateMixerDeviceSwitchRole;

/**
 * MateMixerStreamSwitchRole:
 * @MATE_MIXER_STREAM_SWITCH_ROLE_UNKNOWN:
 *     Unknown stream switch role.
 * @MATE_MIXER_STREAM_SWITCH_ROLE_PORT:
 *     The switch changes the active port.
 * @MATE_MIXER_STREAM_SWITCH_ROLE_BOOST:
 *     The switch changes the boost value.
 */
typedef enum {
  MATE_MIXER_STREAM_SWITCH_ROLE_UNKNOWN,
  MATE_MIXER_STREAM_SWITCH_ROLE_PORT,
  MATE_MIXER_STREAM_SWITCH_ROLE_BOOST
} MateMixerStreamSwitchRole;

/**
 * MateMixerStreamSwitchFlags:
 * @MATE_MIXER_STREAM_SWITCH_NO_FLAGS:
 *     No flags.
 * @MATE_MIXER_STREAM_SWITCH_TOGGLE:
 *     The switch is a #MateMixerStreamToggle.
 */
typedef enum { /*< flags >*/
               MATE_MIXER_STREAM_SWITCH_NO_FLAGS = 0,
               MATE_MIXER_STREAM_SWITCH_TOGGLE = 1 << 0,
} MateMixerStreamSwitchFlags;

/**
 * MateMixerChannelPosition:
 * @MATE_MIXER_CHANNEL_UNKNOWN:
 *     Unknown channel position.
 * @MATE_MIXER_CHANNEL_MONO:
 *     Mono channel. Only used for single-channel controls.
 * @MATE_MIXER_CHANNEL_FRONT_LEFT:
 *     Front left channel.
 * @MATE_MIXER_CHANNEL_FRONT_RIGHT:
 *     Front right channel.
 * @MATE_MIXER_CHANNEL_FRONT_CENTER:
 *     Front center channel.
 * @MATE_MIXER_CHANNEL_LFE:
 *     Low-frequency effects channel (subwoofer).
 * @MATE_MIXER_CHANNEL_BACK_LEFT:
 *     Back (rear) left channel.
 * @MATE_MIXER_CHANNEL_BACK_RIGHT:
 *     Back (rear) right channel.
 * @MATE_MIXER_CHANNEL_BACK_CENTER:
 *     Back (rear) center channel.
 * @MATE_MIXER_CHANNEL_FRONT_LEFT_CENTER:
 *     Front left of center channel.
 * @MATE_MIXER_CHANNEL_FRONT_RIGHT_CENTER:
 *     Front right of center channel.
 * @MATE_MIXER_CHANNEL_SIDE_LEFT:
 *     Side left channel.
 * @MATE_MIXER_CHANNEL_SIDE_RIGHT:
 *     Side right channel.
 * @MATE_MIXER_CHANNEL_TOP_FRONT_LEFT:
 *     Top front left channel.
 * @MATE_MIXER_CHANNEL_TOP_FRONT_RIGHT:
 *     Top front right channel.
 * @MATE_MIXER_CHANNEL_TOP_FRONT_CENTER:
 *     Top front center channel.
 * @MATE_MIXER_CHANNEL_TOP_CENTER:
 *     Top center channel.
 * @MATE_MIXER_CHANNEL_TOP_BACK_LEFT:
 *     Top back (rear) left channel.
 * @MATE_MIXER_CHANNEL_TOP_BACK_RIGHT:
 *     Top back (rear) right channel.
 * @MATE_MIXER_CHANNEL_TOP_BACK_CENTER:
 *     Top back (rear) center channel.
 */
typedef enum {
  MATE_MIXER_CHANNEL_UNKNOWN = 0,
  MATE_MIXER_CHANNEL_MONO,
  MATE_MIXER_CHANNEL_FRONT_LEFT,
  MATE_MIXER_CHANNEL_FRONT_RIGHT,
  MATE_MIXER_CHANNEL_FRONT_CENTER,
  MATE_MIXER_CHANNEL_LFE,
  MATE_MIXER_CHANNEL_BACK_LEFT,
  MATE_MIXER_CHANNEL_BACK_RIGHT,
  MATE_MIXER_CHANNEL_BACK_CENTER,
  MATE_MIXER_CHANNEL_FRONT_LEFT_CENTER,
  MATE_MIXER_CHANNEL_FRONT_RIGHT_CENTER,
  MATE_MIXER_CHANNEL_SIDE_LEFT,
  MATE_MIXER_CHANNEL_SIDE_RIGHT,
  MATE_MIXER_CHANNEL_TOP_FRONT_LEFT,
  MATE_MIXER_CHANNEL_TOP_FRONT_RIGHT,
  MATE_MIXER_CHANNEL_TOP_FRONT_CENTER,
  MATE_MIXER_CHANNEL_TOP_CENTER,
  MATE_MIXER_CHANNEL_TOP_BACK_LEFT,
  MATE_MIXER_CHANNEL_TOP_BACK_RIGHT,
  MATE_MIXER_CHANNEL_TOP_BACK_CENTER,
  /*< private >*/
  MATE_MIXER_CHANNEL_MAX
} MateMixerChannelPosition;

#endif /* MATEMIXER_ENUMS_H */
