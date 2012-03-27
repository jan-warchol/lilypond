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
#include "skyline-pair.hh"

static Skyline_pair
tower_skyline (Real width)
{
  return Skyline_pair (Box (Interval (-width, width), Interval (0, 100)), X_AXIS);
}

static Skyline_pair
teeth_skyline ()
{
  vector<Box> boxes;
  boxes.push_back (Box (Interval (-3, -2), Interval (-3, 0)));
  boxes.push_back (Box (Interval (-1, 0), Interval (-2, 0)));
  boxes.push_back (Box (Interval (1, 2), Interval (-1, 0)));
  return Skyline_pair (boxes, X_AXIS);
}

FUNC (skyline_pair_smallest_shift)
{
  Skyline_pair tower = tower_skyline (0.4);
  Skyline_pair teeth = teeth_skyline ();

  EQUAL (0.4, tower.smallest_shift (teeth, RIGHT));
  EQUAL (1.4, teeth.smallest_shift (tower, RIGHT));
  EQUAL (-1.4, tower.smallest_shift (teeth, LEFT));
  EQUAL (-0.4, teeth.smallest_shift (tower, LEFT));

  tower = tower_skyline (1.0);
  EQUAL (3, tower.smallest_shift (teeth, RIGHT));
  EQUAL (-4, tower.smallest_shift (teeth, LEFT));
}
