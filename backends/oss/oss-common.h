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

#ifndef OSS_COMMON_H
#define OSS_COMMON_H

#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "config.h"

#ifdef HAVE_SYS_SOUNDCARD_H
#include <sys/soundcard.h>
#elif HAVE_SOUNDCARD_H
#include <soundcard.h>
#elif HAVE_MACHINE_SOUNDCARD_H
#include <machine/soundcard.h>
#else
#error "No OSS header file present"
#endif

#endif /* OSS_COMMON_H */
