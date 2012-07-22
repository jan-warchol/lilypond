# show_skyline_command.py -- a GDB plugin to aid in debugging skylines
#
# This file is part of LilyPond, the GNU music typesetter.
#
# Copyright (C) 2012 Joe Neeman <joeneeman@gmail.com>
#
# LilyPond is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# LilyPond is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with LilyPond.  If not, see <http://www.gnu.org/licenses/>.

# To use this command, you will need a python-enabled version of
# GDB. Copy this file to ~/.show_skyline_command.py and add
#
# source .show_skyline_command.py
#
# to your .gdbinit file.

import gdb
from threading import Thread
from math import isinf
import gtk
import gobject
import goocanvas

class GtkSkylineCanvas (goocanvas.Canvas):
    """A Canvas for displaying skylines."""
    def __init__ (self):
        super (GtkSkylineCanvas, self).__init__ ()
        self.connect ('size-allocate', GtkSkylineCanvas.rescale)
        self.x_min = float ('inf')
        self.x_max = float ('-inf')
        self.y_min = float ('inf')
        self.y_max = float ('-inf')

        self.colors = ('black', 'red', 'green', 'blue', 'maroon', 'olive', 'teal')
        self.cur_color_index = 0

    def rescale (self, allocation):
        width = (self.x_max - self.x_min + 1) * 1.1
        height = (self.y_max - self.y_min + 1) * 1.1
        if width <= 0 or height <= 0:
            return

        scale_x = allocation.width / width
        scale_y = allocation.height / height
        scale = min (scale_x, scale_y)
        self.set_scale (scale)

        center_x = (self.x_max + self.x_min) / 2
        center_y = (self.y_max + self.y_min) / 2
        actual_width = allocation.width / scale
        actual_height = allocation.height / scale
        actual_min_x = center_x - actual_width / 2
        actual_max_x = center_x + actual_width / 2
        actual_min_y = center_y - actual_height / 2
        actual_max_y = center_y + actual_height / 2

        self.set_bounds (actual_min_x, actual_min_y, actual_max_x, actual_max_y)
        self.scroll_to (actual_min_x, actual_min_y)

    def add_skyline (self, lines):
        """Adds a skyline to the current canvas, in a new color.

        The canvas will be rescaled, if necessary, to make room for the
        new skyline."""
        # Flip vertically, because goocanvas thinks higher numbers are
        # further down, while lilypond thinks they're further up.
        lines = [(x1, -y1, x2, -y2) for (x1, y1, x2, y2) in lines]

        color = self.colors[self.cur_color_index]
        self.cur_color_index = (self.cur_color_index + 1) % len (self.colors)

        # Update the bounding box of the skylines.
        x_vals = [s[0] for s in lines] + [s[2] for s in lines]
        y_vals = [s[1] for s in lines] + [s[3] for s in lines]
        self.x_min = min ([self.x_min] + x_vals)
        self.x_max = max ([self.x_max] + x_vals)
        self.y_min = min ([self.y_min] + y_vals)
        self.y_max = max ([self.y_max] + y_vals)

        # Add the lines to the canvas.
        root = self.get_root_item ()
        for (x1, y1, x2, y2) in lines:
            goocanvas.polyline_new_line (root, x1, y1, x2, y2,
                    stroke_color=color,
                    line_width=0.05)
        self.rescale (self.get_allocation ())

# We want to run the gtk main loop in a separate thread so that
# the gdb main loop is still responsive.
class SkylineWindowThread (Thread):
    """A thread that runs a Gtk.Window displaying a skyline."""

    def run (self):
        gtk.gdk.threads_init ()
        self.window = None
        self.canvas = None
        gtk.main ()

    # This should only be called from the Gtk main loop.
    def _destroy_window (self, window):
        self.window = None
        self.canvas = None

    # This should only be called from the Gtk main loop.
    def _setup_window (self):
        if self.window is None:
            self.window = gtk.Window ()
            self.canvas = GtkSkylineCanvas ()
            self.window.add (self.canvas)
            self.window.connect ("destroy", self._destroy_window)
            self.window.show_all ()

    # This should only be called from the Gtk main loop.
    def _add_skyline (self, lines):
        self._setup_window ()
        self.canvas.add_skyline (lines)

    def add_skyline (self, lines):
        # Copy the lines, just in case someone modifies them.
        gobject.idle_add (self._add_skyline, list (lines))


# Function taken from GCC
def find_type(orig, name):
    typ = orig.strip_typedefs()
    while True:
        search = str(typ) + '::' + name
        try:
            return gdb.lookup_type(search)
        except RuntimeError:
            pass
        # The type was not found, so try the superclass.    We only need
        # to check the first superclass, so we don't bother with
        # anything fancier here.
        field = typ.fields()[0]
        if not field.is_base_class:
            raise ValueError, "Cannot find type %s::%s" % (str(orig), name)
        typ = field.type

# Class adapted from GCC
class ListIterator:
    def __init__ (self, val):
        self.val = val
        self.nodetype = find_type (val.type, '_Node')
        self.nodetype = self.nodetype.strip_typedefs ().pointer ()

        head = val['_M_impl']['_M_node']
        self.base = head['_M_next']
        self.head = head.address

    def __iter__ (self):
        return self

    def next (self):
        if self.base == self.head:
                raise StopIteration
        elt = self.base.cast (self.nodetype).dereference ()
        self.base = elt['_M_next']
        return elt['_M_data']

def to_list (list_val):
    return list (ListIterator (list_val))

thread = SkylineWindowThread ()
thread.setDaemon (True)
thread.start ()

def skyline_to_lines (sky_value):
    """Turns a gdb.Value into a list of line segments."""
    sky_d = int (sky_value['sky_'])
    buildings = to_list (sky_value['buildings_'])

    def bld_to_line (bld):
        y_intercept = float (bld['y_intercept_']) * sky_d
        slope = float (bld['slope_']) * sky_d
        x1 = float (bld['start_'])
        x2 = float (bld['end_'])

        if isinf (y_intercept) or isinf (x1) or isinf (x2):
            return None
        return (x1, y_intercept + slope * x1, x2, y_intercept + slope * x2)

    ret = map (bld_to_line, buildings)
    return [r for r in ret if r is not None]

class ShowSkylineCommand (gdb.Command):
    "Show a skyline graphically."

    def __init__ (self, command_name):
        super (ShowSkylineCommand, self).__init__ (command_name,
                                                 gdb.COMMAND_DATA,
                                                 gdb.COMPLETE_SYMBOL, False)

    def to_lines (self, skyline):
        pass

    def invoke (self, arg, from_tty):
        global plot

        val = gdb.parse_and_eval (arg)
        typ = val.type

        # If they passed in a reference or pointer to a skyline,
        # dereference it.
        if typ.code == gdb.TYPE_CODE_PTR or typ.code == gdb.TYPE_CODE_REF:
            val = val.dereference ()
            typ = val.type

        if typ.tag == 'Skyline_pair':
            sky = val['skylines_']
            arr = sky['array_']
            thread.add_skyline (self.to_lines (arr[0]))
            thread.add_skyline (self.to_lines (arr[1]))

        elif typ.tag == 'Skyline':
            thread.add_skyline (self.to_lines (val))

class ShowVSkylineCommand (ShowSkylineCommand):
    "Show a vertical skyline."

    def __init__ (self):
        super (ShowVSkylineCommand, self).__init__ ("vsky")

    def to_lines (self, skyline):
        return skyline_to_lines (skyline)

class ShowHSkylineCommand (ShowSkylineCommand):
    "Show a horizontal skyline."

    def __init__ (self):
        super (ShowHSkylineCommand, self).__init__ ("hsky")

    def to_lines (self, skyline):
        lines = skyline_to_lines (skyline)
        # Because it is a horizontal skyline, reflect around the line y=x.
        return [(y1, x1, y2, x2) for (x1, y1, x2, y2) in lines]

ShowHSkylineCommand()
ShowVSkylineCommand()

