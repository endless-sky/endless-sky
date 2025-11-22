/*-*- Mode: C; c-basic-offset: 8 -*-*/

#ifndef foortkithfoo
#define foortkithfoo

/***
        Copyright 2009 Lennart Poettering
        Copyright 2010 David Henningsson <diwic@ubuntu.com>

        Permission is hereby granted, free of charge, to any person
        obtaining a copy of this software and associated documentation files
        (the "Software"), to deal in the Software without restriction,
        including without limitation the rights to use, copy, modify, merge,
        publish, distribute, sublicense, and/or sell copies of the Software,
        and to permit persons to whom the Software is furnished to do so,
        subject to the following conditions:

        The above copyright notice and this permission notice shall be
        included in all copies or substantial portions of the Software.

        THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
        EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
        MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
        NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
        BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
        ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
        CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
        SOFTWARE.
***/

#include <sys/types.h>

#include "dbus_wrap.h"

/* This is the reference implementation for a client for
 * RealtimeKit. You don't have to use this, but if do, just copy these
 * sources into your repository */

#define RTKIT_SERVICE_NAME "org.freedesktop.RealtimeKit1"
#define RTKIT_OBJECT_PATH "/org/freedesktop/RealtimeKit1"

/* This is mostly equivalent to sched_setparam(thread, SCHED_RR, {
 * .sched_priority = priority }). 'thread' needs to be a kernel thread
 * id as returned by gettid(), not a pthread_t! If 'thread' is 0 the
 * current thread is used. The returned value is a negative errno
 * style error code, or 0 on success. */
int rtkit_make_realtime(DBusConnection *system_bus, pid_t thread, int priority);

/* This is mostly equivalent to setpriority(PRIO_PROCESS, thread,
 * nice_level). 'thread' needs to be a kernel thread id as returned by
 * gettid(), not a pthread_t! If 'thread' is 0 the current thread is
 * used. The returned value is a negative errno style error code, or 0
 * on success.*/
int rtkit_make_high_priority(DBusConnection *system_bus, pid_t thread, int nice_level);

/* Return the maximum value of realtime priority available. Realtime requests
 * above this value will fail. A negative value is an errno style error code.
 */
int rtkit_get_max_realtime_priority(DBusConnection *system_bus);

/* Retreive the minimum value of nice level available. High prio requests
 * below this value will fail. The returned value is a negative errno
 * style error code, or 0 on success.*/
int rtkit_get_min_nice_level(DBusConnection *system_bus, int *min_nice_level);

/* Return the maximum value of RLIMIT_RTTIME to set before attempting a
 * realtime request. A negative value is an errno style error code.
 */
long long rtkit_get_rttime_usec_max(DBusConnection *system_bus);

#endif
