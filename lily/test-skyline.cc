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

FUNC (skyline_horizontal_distance)
{
  Skyline tower = tower_skyline (0.4);
  Skyline teeth = teeth_skyline ();

  EQUAL (0.4, tower.horizontal_distance (teeth, RIGHT));
  EQUAL (-1.4, tower.horizontal_distance (teeth, LEFT));

  tower = tower_skyline (1.0);
  EQUAL (3, tower.horizontal_distance (teeth, RIGHT));
  EQUAL (-4, tower.horizontal_distance (teeth, LEFT));

  // Parallel slopes
  Skyline slope1 = segment_skyline (0, 0, 1, 1, UP);
  Skyline slope2 = segment_skyline (0, 0, 1, 1, DOWN);
  EQUAL (0, slope1.horizontal_distance (slope2, RIGHT));
  EQUAL (0, slope1.horizontal_distance (slope2, LEFT));
  slope2.shift (0.5);
  EQUAL (0.5, slope1.horizontal_distance (slope2, RIGHT));
  EQUAL (-0.5, slope1.horizontal_distance (slope2, LEFT));

  // Perpendicular slopes
  slope2 = segment_skyline (0, 1, 1, 0, DOWN);
  EQUAL (1, slope1.horizontal_distance (slope2, RIGHT));
  EQUAL (-1, slope1.horizontal_distance (slope2, LEFT));
}
