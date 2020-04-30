# pidvisualizer

A simple utility library to help visualizer the behavior of
[PID controllers](https://en.wikipedia.org/wiki/PID_controller).
This is meant to be used together with
[libacfutils's](https://github.com/skiselkov/libacfutils) `pid_ctl_t` facility.
See `acfutils/pid_ctl.h` for more details on the implementation.

To use `pidvisualizer`, you simply call `pidvis_new` with a short name (which
will be shown as the name of the PID controller in the window's title),
a pointer to the `pid_ctl_t` you wish to visualize and an optional
`mt_cairo_uploader_t` (see `acfutils/mt_cairo_render.h` for that, or pass `NULL`
if you don't want to use an async uploader). This will give you a `pid_vis_t`
object. You can then call `pidvis_open` to open the window. The PID visualizer
continuously samples the PID controller 30x per second, even if the window isn't
visible. If you want to destroy the PID visualizer when the window is closed,
call `pidvis_is_open` to determine if the user has closed the window and call
`pidvis_destroy` to destroy the visualizer completely.
