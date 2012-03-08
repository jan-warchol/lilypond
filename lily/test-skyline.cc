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
}
