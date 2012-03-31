/*
  This file is part of LilyPond, the GNU music typesetter.

  Copyright (C) 2012 Joe Neeman <joeneeman@gmail.com>

  LilyPond is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  LilyPond is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with LilyPond.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <cstdlib>
#include "yaffut.hh"
#include "skyline.hh"

static Skyline
tower_skyline (Real width)
{
  return Skyline (Box (Interval (-width, width), Interval (0, 100)), X_AXIS, UP);
}

static Skyline
teeth_skyline ()
{
  vector<Box> boxes;
  boxes.push_back (Box (Interval (-3, -2), Interval (-3, 0)));
  boxes.push_back (Box (Interval (-1, 0), Interval (-2, 0)));
  boxes.push_back (Box (Interval (1, 2), Interval (-1, 0)));
  return Skyline (boxes, X_AXIS, DOWN);
}

static Skyline
segment_skyline (Real x1, Real y1, Real x2, Real y2, Direction d)
{
  Offset p1 = Offset (x1, y1);
  Offset p2 = Offset (x2, y2);
  vector<Drul_array<Offset> > segs;
  segs.push_back (Drul_array<Offset> (p1, p2));
  return Skyline (segs, X_AXIS, d);
}

FUNC (skyline_distance)
{
  Skyline tower = tower_skyline (0.9);
  Skyline teeth = teeth_skyline ();

  EQUAL (102, tower.distance (teeth));
  tower.shift (1);
  EQUAL (101, tower.distance (teeth));
  tower.shift (-3);
  EQUAL (103, tower.distance (teeth));
  tower.shift (-2);
  EQUAL (-infinity_f, tower.distance (teeth));
}

FUNC (skyline_smallest_shift)
{
  Skyline tower = tower_skyline (0.4);
  Skyline teeth = teeth_skyline ();

  EQUAL (0.4, tower.smallest_shift (teeth, RIGHT));
  EQUAL (-1.4, tower.smallest_shift (teeth, LEFT));

  tower = tower_skyline (1.0);
  EQUAL (3, tower.smallest_shift (teeth, RIGHT));
  EQUAL (-4, tower.smallest_shift (teeth, LEFT));

  // Parallel slopes
  Skyline slope1 = segment_skyline (0, 0, 1, 1, UP);
  Skyline slope2 = segment_skyline (0, 0, 1, 1, DOWN);
  EQUAL (0, slope1.smallest_shift (slope2, RIGHT));
  EQUAL (0, slope1.smallest_shift (slope2, LEFT));
  slope2.shift (0.5);
  EQUAL (0.5, slope1.smallest_shift (slope2, RIGHT));
  EQUAL (-0.5, slope1.smallest_shift (slope2, LEFT));

  // Perpendicular slopes
  slope2 = segment_skyline (0, 1, 1, 0, DOWN);
  EQUAL (1, slope1.smallest_shift (slope2, RIGHT));
  EQUAL (-1, slope1.smallest_shift (slope2, LEFT));
}

FUNC (area_between)
{
  Skyline flat = segment_skyline (-10, 0, 10, 0, UP);
  Skyline teeth = teeth_skyline ();

  EQUAL (-6, flat.area_between (teeth, -3, 2, 0, 0));
  EQUAL (-6, teeth.area_between (flat, -3, 2, 0, 0));
  EQUAL (-3, flat.area_between (teeth, -2, 2, 0, 0));
  EQUAL (-5, flat.area_between (teeth, -2, 2, 0, -1));

  EQUAL (-10, flat.area_between (teeth, -3, 2, 0, 0, 0.5));
}
