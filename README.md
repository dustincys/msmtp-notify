# msmtp-notify

## What? ##

This program is inspired in Du Yang's [notifier program](https://code.google.com/p/msmtp-notify/). But instead of polling on msmtp's log file and compare sizes to see if there any changes, it uses the kernel's `inotify` feature. Also, the notifications are permanent: the user is required to manually close them. This is to avoid having them disappear before he could read them.

## How? ##

Requires a kernel that supports inotify and, [libnotify](http://library.gnome.org/devel/notification-spec/), and [pkg-config](http://pkgconfig.freedesktop.org/wiki/). Clone to a directory of your choice, run make, and then run program. Default log file name is "./msmtp.log". To change, use the `-l` flag.
