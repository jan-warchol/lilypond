/*
  This file is part of LilyPond, the GNU music typesetter.

  Copyright (C) 2004--2012 Han-Wen Nienhuys <hanwen@xs4all.nl>

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

#include "self-alignment-interface.hh"

#include "directional-element-interface.hh"
#include "grob.hh"
#include "grob-array.hh"
#include "interval-minefield.hh"
#include "note-column.hh"
#include "paper-column.hh"
#include "pointer-group-interface.hh"
#include "warn.hh"

MAKE_SCHEME_CALLBACK (Self_alignment_interface, y_aligned_on_self, 1);
SCM
Self_alignment_interface::y_aligned_on_self (SCM element)
{
  return aligned_on_self (unsmob_grob (element), Y_AXIS, false, 0, 0);
}

MAKE_SCHEME_CALLBACK (Self_alignment_interface, x_aligned_on_self, 1);
SCM
Self_alignment_interface::x_aligned_on_self (SCM element)
{
  return aligned_on_self (unsmob_grob (element), X_AXIS, false, 0, 0);
}

MAKE_SCHEME_CALLBACK (Self_alignment_interface, pure_y_aligned_on_self, 3);
SCM
Self_alignment_interface::pure_y_aligned_on_self (SCM smob, SCM start, SCM end)
{
  return aligned_on_self (unsmob_grob (smob), Y_AXIS, true, robust_scm2int (start, 0), robust_scm2int (end, INT_MAX));
}

SCM
Self_alignment_interface::aligned_on_self (Grob *me, Axis a, bool pure, int start, int end)
{
  SCM sym = (a == X_AXIS) ? ly_symbol2scm ("self-alignment-X")
            : ly_symbol2scm ("self-alignment-Y");

  SCM align (me->internal_get_property (sym));
  if (scm_is_number (align))
    {
      Interval ext (me->maybe_pure_extent (me, a, pure, start, end));
      // Empty extent doesn't mean an error - we simply don't align such grobs.
      // However, empty extent and non-empty stencil would be suspicious.
      if (!ext.is_empty ())
        return scm_from_double (- ext.linear_combination (scm_to_double (align)));
      else if (me->get_stencil ())
        warning (me->name () + " has empty extent and non-empty stencil.");
    }
  return scm_from_double (0.0);
}

SCM
Self_alignment_interface::centered_on_object (Grob *him, Axis a)
{
  return scm_from_double (robust_relative_extent (him, him, a).center ());
}

MAKE_SCHEME_CALLBACK (Self_alignment_interface, centered_on_x_parent, 1);
SCM
Self_alignment_interface::centered_on_x_parent (SCM smob)
{
  return centered_on_object (unsmob_grob (smob)->get_parent (X_AXIS), X_AXIS);
}

MAKE_SCHEME_CALLBACK (Self_alignment_interface, centered_on_y_parent, 1);
SCM
Self_alignment_interface::centered_on_y_parent (SCM smob)
{
  return centered_on_object (unsmob_grob (smob)->get_parent (Y_AXIS), Y_AXIS);
}

MAKE_SCHEME_CALLBACK (Self_alignment_interface, x_align_grob, 1);
SCM
Self_alignment_interface::x_align_grob (SCM smob)
{
  Grob *me = unsmob_grob (smob);
  return align_grob (me, X_AXIS, false, 0, 0);
}

MAKE_SCHEME_CALLBACK (Self_alignment_interface, y_align_grob, 1);
SCM
Self_alignment_interface::y_align_grob (SCM smob)
{
  Grob *me = unsmob_grob (smob);
  return align_grob (me, Y_AXIS, false, 0, 0);
}

/*
  Positioning of grobs is done relative to their parents in respective axes.
  Grob properties [XY]-offset measure the displacement between grob's reference
  point and the reference point of grob's parent in [XY]_AXIS (in staffspaces).

  To align a particular point of the grob with a particular point of its parent
  one has to calculate what this offset should be, based on dimensions (extents)
  of both objects.

  There was an idea to pass the reference grob as another argument (to allow
  aligning 'me' on something else than its parent), but that'd be a wrong
  approach.  If we want to align me on another grob, what we need to do is
  change appropriate parent to be that grob. (TODO: did i get this right? --jw)

  TODO: Also, Mike Solomon suggested to make a more generic funciton that would
  accept a vector of grobs to align with (instead of simply grabbing a parent).
  His idea was inspired by the fact that sometimes we need to grab a notecolumn
  and align on it (for example when the parent is a PaperColumn and we cannot
  use him), so maybe we could allow aligning on arbitrary set of grobs.
  However, i don't think this is a good idea.  Aligning on NoteColumns is an
  exception, and we generally align to a single grob - aligning to multiple grobs
  seems to be something strange.  Thoughts? --jw
  see https://codereview.appspot.com/7768043#msg12
*/
SCM
Self_alignment_interface::align_grob (Grob *me, Axis a, bool pure, int start, int end)
{
  Real offset = 0.0;
  Grob *him = me->get_parent (a);

  SCM alignment = (a == X_AXIS)
                  ? me->internal_get_property (ly_symbol2scm ("self-alignment-X"))
                  : me->internal_get_property (ly_symbol2scm ("self-alignment-Y"));

  SCM my_alignment, his_alignment;
  if (scm_is_pair (alignment))
    {
      my_alignment = scm_car (alignment);
      his_alignment = scm_cdr (alignment);
    }
  else
    {
      my_alignment = alignment;
      his_alignment = alignment;
    }

  // calculate offset related to grob's own dimensions
  if (scm_is_number (my_alignment))
    {
      Interval my_ext = me->maybe_pure_extent (me, a, pure, start, end);

      // Empty extent doesn't mean an error - we simply don't align such grobs.
      // However, empty extent and non-empty stencil would be suspicious.
      if (!my_ext.is_empty ())
        offset -= my_ext.linear_combination (scm_to_double (my_alignment));
      else if (me->get_stencil ())
        warning (me->name () + " has empty extent and non-empty stencil.");
    }

  // calculate offset related to grob's parent dimensions
  if (scm_is_number (his_alignment))
    {
      Interval his_ext;
      if (Paper_column::has_interface (him))
        /*
          PaperColumn extents aren't reliable (they depend on size and alignment
          of PaperColumn's children), so we cannot use them. Instead, we extract
          the extent of a respective NoteColumn and align on it.
          This situation (i.e. having PaperColumn as parent) happens for example
          for unassociated lyrics (i.e. lyrics without associatedVoice set),
          or dynamics attached to spacers.
        */
        his_ext = Paper_column::get_generic_interface_extent
                  (him, ly_symbol2scm ("note-column-interface"), a);
      else
        his_ext = him->maybe_pure_extent (him, a, pure, start, end);

      // Empty extent doesn't mean an error - we simply don't align such grobs.
      // However, empty extent and non-empty stencil would be suspicious.
      if (!his_ext.is_empty ())
        offset += his_ext.linear_combination (scm_to_double (his_alignment));
      else if (him->get_stencil ())
        warning (him->name () + " has empty extent and non-empty stencil.");
    }

  return scm_from_double (offset);
}

ADD_INTERFACE (Self_alignment_interface,
               "Align a particular point of this object on"
               " a particular point of its parent, for example"
               " object's reference point on its parent's center."
               " Alignment is read from properties @code{self-alignment-X}"
               " and @code{self-alignment-Y}.",

               /* properties */
               "collision-bias "
               "collision-padding "
               "potential-X-colliding-grobs "
               "self-alignment-X "
               "self-alignment-Y "
               "X-colliding-grobs "
               "Y-colliding-grobs "
              );
