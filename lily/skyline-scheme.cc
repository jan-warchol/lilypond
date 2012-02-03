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

#include "skyline.hh"
#include "skyline-pair.hh"

LY_DEFINE (ly_make_skyline_pair, "ly:make-skyline-pair",
           2, 1, 0, (SCM boxes, SCM axis, SCM padding),
           "Create a skyline-pair using the boxes in @var{boxes} along"
           " axis @var{axis} with optional horizontal padding @var{horizontal_padding}.")
{
  LY_ASSERT_TYPE (ly_is_list, boxes, 1);
  LY_ASSERT_TYPE (is_axis, axis, 2);

  Axis ax = Axis (scm_to_int (axis));
  Real pad = 0.0;  
  if (padding != SCM_UNDEFINED)
    {
      LY_ASSERT_TYPE (scm_is_number, padding, 2);
      pad = scm_to_double (padding);
    }

  vector<Box> vboxes;
  for (SCM bxs = boxes; scm_is_pair (bxs); bxs = scm_cdr (bxs))
    {
      LY_ASSERT_SMOB (Box, scm_car (bxs), 1);
      vboxes.push_back (*unsmob_box (scm_car (bxs)));
    }

  return Skyline_pair (vboxes, pad, ax).smobbed_copy ();
}

LY_DEFINE (ly_skyline_pair_raise_x, "ly:skyline-pair-raise!",
           2, 0, 0, (SCM skyline_pair, SCM amount),
           "Raise skyline-pair @var{skyline_pair} by amount @var{amount}.")
{
  LY_ASSERT_SMOB (Skyline_pair, skyline_pair, 1);
  LY_ASSERT_TYPE (scm_is_number, amount, 2);

  Real real_amount = scm_to_double (amount);
  Skyline_pair::unsmob (skyline_pair)->raise (real_amount);

  return SCM_BOOL_T;
}

LY_DEFINE (ly_skyline_pair_shift_x, "ly:skyline-pair-shift!",
           2, 0, 0, (SCM skyline_pair, SCM amount),
           "Shift skyline-pair @var{skyline_pair} by amount @var{amount}.")
{
  LY_ASSERT_SMOB (Skyline_pair, skyline_pair, 1);
  LY_ASSERT_TYPE (scm_is_number, amount, 2);

  Real real_amount = scm_to_double (amount);
  Skyline_pair::unsmob (skyline_pair)->shift (real_amount);

  return SCM_BOOL_T;
}

LY_DEFINE (ly_skyline_pair_merge_x, "ly:skyline-pair-merge!",
           2, 0, 0, (SCM skyline_pair, SCM other),
           "Merge skyline-pair @var{skyline_pair} with @var{other}.")
{
  LY_ASSERT_SMOB (Skyline_pair, skyline_pair, 1);
  LY_ASSERT_SMOB (Skyline_pair, other, 2);

  Skyline_pair::unsmob (skyline_pair)->merge (*Skyline_pair::unsmob (other));

  return SCM_BOOL_T;
}

LY_DEFINE (ly_skyline_pair_raise, "ly:skyline-pair-raise",
           2, 0, 0, (SCM skyline_pair, SCM amount),
           "Raise skyline-pair @var{skyline_pair} by amount @var{amount},"
           " returning a new skyline pair.")
{
  LY_ASSERT_SMOB (Skyline_pair, skyline_pair, 1);
  LY_ASSERT_TYPE (scm_is_number, amount, 2);

  Real real_amount = scm_to_double (amount);

  SCM new_skp = Skyline_pair::unsmob (skyline_pair)->smobbed_copy ();
  Skyline_pair::unsmob (new_skp)->shift (real_amount);

  return new_skp;
}


LY_DEFINE (ly_skyline_pair_shift, "ly:skyline-pair-shift",
           2, 0, 0, (SCM skyline_pair, SCM amount),
           "Shift skyline-pair @var{skyline_pair} by amount @var{amount},"
           " returning a new skyline pair.")
{
  LY_ASSERT_SMOB (Skyline_pair, skyline_pair, 1);
  LY_ASSERT_TYPE (scm_is_number, amount, 2);

  Real real_amount = scm_to_double (amount);

  SCM new_skp = Skyline_pair::unsmob (skyline_pair)->smobbed_copy ();
  Skyline_pair::unsmob (new_skp)->shift (real_amount);

  return new_skp;
}

LY_DEFINE (ly_skyline_pair_merge, "ly:skyline-pair-merge",
           2, 0, 0, (SCM skyline_pair, SCM other),
           "Merge skyline-pair @var{skyline_pair} with @var{other},"
           " returning a new skyline pair.")
{
  LY_ASSERT_SMOB (Skyline_pair, skyline_pair, 1);
  LY_ASSERT_SMOB (Skyline_pair, other, 2);

  Skyline_pair skyp (vector<Box> (), 0, X_AXIS);
  skyp.merge (*Skyline_pair::unsmob (skyline_pair));
  skyp.merge (*Skyline_pair::unsmob (other));

  return skyp.smobbed_copy ();
}

LY_DEFINE (ly_skyline_pair_print, "ly:skyline-pair-print",
           1, 0, 0, (SCM skyline_pair),
           "Print skyline-pair @var{skyline_pair}.")
{
  LY_ASSERT_SMOB (Skyline_pair, skyline_pair, 1);

  Skyline_pair::unsmob (skyline_pair)->print ();

  return SCM_BOOL_T;
}

LY_DEFINE (ly_skyline_pair_print_points, "ly:skyline-pair-print-points",
           1, 0, 0, (SCM skyline_pair),
           "Print points of skyline-pair @var{skyline_pair}.")
{
  LY_ASSERT_SMOB (Skyline_pair, skyline_pair, 1);

  Skyline_pair::unsmob (skyline_pair)->print_points ();

  return SCM_BOOL_T;
}

