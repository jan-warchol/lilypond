/*
  This file is part of LilyPond, the GNU music typesetter.

  Copyright (C) 2012 Mike Solomon <mike@apollinemike.com>

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

#include "box.hh"

LY_DEFINE (ly_make_box, "ly:make-box",
           1, 2, 0, (SCM points, SCM widen_x, SCM widen_y),
           "Create a box containing points @var{points} with optional"
           " widening @var{widen_x} and @var{widen_y}.")
{
  LY_ASSERT_TYPE (ly_is_list, points, 1);
  Real wide_x = 0.0;
  
  if (widen_x != SCM_UNDEFINED)
    {
      LY_ASSERT_TYPE (scm_is_number, widen_x, 2);
      wide_x = scm_to_double (widen_x);
    }

  Real wide_y = 0.0;
  if (widen_y != SCM_UNDEFINED)
    {
      LY_ASSERT_TYPE (scm_is_number, widen_y, 2);
      wide_y = scm_to_double (widen_y);
    }

  Box b;
  for (SCM pts = points; scm_is_pair (pts); pts = scm_cdr (pts))
    {
      LY_ASSERT_TYPE (scm_is_pair, scm_car (pts), 1);
      b.add_point (robust_scm2offset (scm_car (pts), Offset (0,0)));
    }

  b[X_AXIS].widen (wide_x);
  b[Y_AXIS].widen (wide_y);

  return b.smobbed_copy ();
}

LY_DEFINE (ly_box_translate_x, "ly:box-translate!",
           2, 0, 0, (SCM box, SCM offset),
           "Translate box @var{box} by offset @var{offset}.")
{
  LY_ASSERT_SMOB (Box, box, 1);
  LY_ASSERT_TYPE (is_number_pair, offset, 2);
  Offset o = ly_scm2offset (offset);

  unsmob_box (box)->translate (o);

  return SCM_BOOL_T;
}

LY_DEFINE (ly_box_translate, "ly:box-translate",
           2, 0, 0, (SCM box, SCM offset),
           "Translate box @var{box} by offset @var{offset},"
           " returning a new box.")
{
  LY_ASSERT_SMOB (Box, box, 1);
  LY_ASSERT_TYPE (is_number_pair, offset, 2);
  Offset o = ly_scm2offset (offset);

  SCM b = unsmob_box (box)->smobbed_copy ();
  unsmob_box (b)->translate (o);

  return b;
}
