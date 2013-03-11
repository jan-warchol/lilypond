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

// (yet) unavoidable exeptions

MAKE_SCHEME_CALLBACK (Self_alignment_interface, pure_y_aligned_on_self, 3);
SCM
Self_alignment_interface::pure_y_aligned_on_self (SCM smob, SCM start, SCM end)
{
  Grob *me = unsmob_grob (smob);
  SCM align (me->internal_get_property (ly_symbol2scm ("self-alignment-Y")));
  if (scm_is_number (align))
    {
      Interval ext (me->maybe_pure_extent (me, Y_AXIS, true, robust_scm2int (start, 0), robust_scm2int (end, INT_MAX)));
      if (!ext.is_empty ())
        return scm_from_double (- ext.linear_combination (scm_to_double (align)));
    }
  return scm_from_double (0.0);
}

MAKE_SCHEME_CALLBACK (Self_alignment_interface, centered_on_note_columns, 1);
SCM
Self_alignment_interface::centered_on_note_columns (SCM smob)
{
  Item *it = unsmob_item (smob)->get_column ();
  if (!it)
    return scm_from_double (0.0);

  extract_grob_set (it, "elements", elts);
  vector<Grob *> ncs;
  Interval centers;
  for (vsize i = 0; i < elts.size (); i++)
    if (Note_column::has_interface (elts[i]))
      centers.add_point (robust_relative_extent (elts[i], elts[i], X_AXIS).center ());

  if (centers.is_empty ())
    return scm_from_double (0.0);

  return scm_from_double (centers.center ());
}

// wrappers around general-alignment for old ways of doing things

void
Self_alignment_interface::convert (Grob *me, Axis a, SCM align_val, bool selfish)
{
  SCM align_sym = (a == X_AXIS)
                  ? ly_symbol2scm ("X-alignment")
                  : ly_symbol2scm ("Y-alignment");
  SCM ext_sym = (a == X_AXIS)
                ? ly_symbol2scm ("X-extent")
                : ly_symbol2scm ("Y-extent");
  me->set_property (align_sym,
                    scm_list_3 (scm_cons (ext_sym, align_val),
                                selfish ? SCM_EOL : scm_cons (ext_sym, align_val),
                                scm_from_double (0)));
}

MAKE_SCHEME_CALLBACK (Self_alignment_interface, fofo, 1);
SCM
Self_alignment_interface::fofo (SCM smob)
{
  Grob *me = unsmob_grob (smob);
  return general_alignment (me, unsmob_grob(me->get_object ("stem")), X_AXIS);
}

MAKE_SCHEME_CALLBACK (Self_alignment_interface, centered_on_x_parent, 1);
SCM
Self_alignment_interface::centered_on_x_parent (SCM smob)
{
  Grob *me = unsmob_grob (smob);
  convert (me, X_AXIS, scm_from_double (0), true);
  return general_alignment (me, me->get_parent (X_AXIS), X_AXIS);
}

MAKE_SCHEME_CALLBACK (Self_alignment_interface, centered_on_y_parent, 1);
SCM
Self_alignment_interface::centered_on_y_parent (SCM smob)
{
  Grob *me = unsmob_grob (smob);
  convert (me, Y_AXIS, scm_from_double (0), true);
  return general_alignment (me, me->get_parent (Y_AXIS), Y_AXIS);
}

MAKE_SCHEME_CALLBACK (Self_alignment_interface, x_centered_on_y_parent, 1);
SCM
Self_alignment_interface::x_centered_on_y_parent (SCM smob)
{
  Grob *me = unsmob_grob (smob);
  convert (me, X_AXIS, scm_from_double (0), true);
  return general_alignment (me, me->get_parent (Y_AXIS), X_AXIS);
}

MAKE_SCHEME_CALLBACK (Self_alignment_interface, x_aligned_on_self, 1);
SCM
Self_alignment_interface::x_aligned_on_self (SCM element)
{
  Grob *me = unsmob_grob (element);
  convert (me, X_AXIS, me->internal_get_property (ly_symbol2scm ("self-alignment-X")), true);
  return general_alignment (me, me->get_parent (X_AXIS), X_AXIS);
}

MAKE_SCHEME_CALLBACK (Self_alignment_interface, y_aligned_on_self, 1);
SCM
Self_alignment_interface::y_aligned_on_self (SCM element)
{
  Grob *me = unsmob_grob (element);
  convert (me, Y_AXIS, me->internal_get_property (ly_symbol2scm ("self-alignment-Y")), true);
  return general_alignment (me, me->get_parent (Y_AXIS), Y_AXIS);
}

MAKE_SCHEME_CALLBACK (Self_alignment_interface, aligned_on_x_parent, 1);
SCM
Self_alignment_interface::aligned_on_x_parent (SCM smob)
{
  Grob *me = unsmob_grob (smob);
  convert (me, X_AXIS, me->internal_get_property (ly_symbol2scm ("self-alignment-X")), false);
  return general_alignment (me, me->get_parent (X_AXIS), X_AXIS);
}

MAKE_SCHEME_CALLBACK (Self_alignment_interface, aligned_on_y_parent, 1);
SCM
Self_alignment_interface::aligned_on_y_parent (SCM smob)
{
  Grob *me = unsmob_grob (smob);
  convert (me, Y_AXIS, me->internal_get_property (ly_symbol2scm ("self-alignment-Y")), false);
  return general_alignment (me, me->get_parent (Y_AXIS), Y_AXIS);
}

// brand-new general alignment

MAKE_SCHEME_CALLBACK (Self_alignment_interface, general_x_alignment, 1)
SCM
Self_alignment_interface::general_x_alignment (SCM smob)
{
  Grob *me = unsmob_grob (smob);
  return general_alignment (me, me->get_parent (X_AXIS), X_AXIS);
}

MAKE_SCHEME_CALLBACK (Self_alignment_interface, general_y_alignment, 1)
SCM
Self_alignment_interface::general_y_alignment (SCM smob)
{
  Grob *me = unsmob_grob (smob);
  return general_alignment (me, me->get_parent (Y_AXIS), Y_AXIS);
}

SCM
Self_alignment_interface::general_alignment (Grob *me, Grob *him, Axis a)
{
  //him is usually a parent of me in respective axis.

  Real offset = 0.0;

  SCM which_alignment = (a == X_AXIS)
                        ? ly_symbol2scm ("X-alignment")
                        : ly_symbol2scm ("Y-alignment");

  SCM alignment_property_list (me->internal_get_property (which_alignment));

  // which_*_extent allows to choose a property used for calculating extents.
  // If this remains SCM_EOL, the reference point of the stencil will be used.
  SCM which_grob_extent = SCM_EOL;
  SCM which_parent_extent = SCM_EOL;
  Real grob_alignment = 0;
  Real parent_alignment = 0;
  Real extra_offset = 0;

  if (alignment_property_list == SCM_EOL)
    return scm_from_int (0);
  else
    {
      SCM grob_pair = robust_list_ref (0, alignment_property_list);
      if (scm_is_pair (grob_pair))
        {
          which_grob_extent = scm_car (grob_pair);
          grob_alignment = robust_scm2double (scm_cdr (grob_pair), 0.0);
        }

      SCM parent_pair = robust_list_ref (1, alignment_property_list);
      if (scm_is_pair (parent_pair))
        {
          which_parent_extent = scm_car (parent_pair);
          parent_alignment = robust_scm2double (scm_cdr (parent_pair), 0.0);
        }

      extra_offset += scm_to_double (robust_list_ref (2, alignment_property_list));
    }

  // PaperColumn extents are weird, so we don't use them.
  if (Paper_column::has_interface (him))
    which_parent_extent = SCM_EOL;

  SCM grob_property = me->get_property (which_grob_extent);
  Interval grob_extent = (scm_is_pair (grob_property))
                         ? ly_scm2interval (grob_property)
                         : me->extent (me, a);

  // TODO add error when empty extent
  //if (ext.is_empty ())
  //  programming_error ("cannot align on self: empty element");

  if (!grob_extent.is_empty () && (which_grob_extent != SCM_EOL))
    offset -= grob_extent.linear_combination (grob_alignment);

  SCM parent_property = him->get_property (which_parent_extent);
  Interval parent_extent = (scm_is_pair (parent_property))
                           ? ly_scm2interval (parent_property)
                           : him->extent (him, a);

  if (!parent_extent.is_empty () && (which_parent_extent != SCM_EOL))
    offset += parent_extent.linear_combination (parent_alignment);

  offset += extra_offset;

  return scm_from_double (offset);
}

void
Self_alignment_interface::set_center_parent (Grob *me, Axis a)
{
  add_offset_callback (me,
                       (a == X_AXIS) ? centered_on_x_parent_proc : centered_on_y_parent_proc,
                       a);
}

void
Self_alignment_interface::set_align_self (Grob *me, Axis a)
{
  add_offset_callback (me,
                       (a == X_AXIS) ? x_aligned_on_self_proc : y_aligned_on_self_proc,
                       a);
}

ADD_INTERFACE (Self_alignment_interface,
               "Position this object on itself and/or on its parent.  To this"
               " end, the following functions are provided:\n"
               "\n"
               "@table @code\n"
               "@item Self_alignment_interface::[xy]_aligned_on_self\n"
               "Align self on reference point, using"
               " @code{self-alignment-X} and @code{self-alignment-Y}."
               "@item Self_alignment_interface::aligned_on_[xy]_parent\n"
               "@item Self_alignment_interface::centered_on_[xy]_parent\n"
               "Shift the object so its own reference point is centered on"
               " the extent of the parent\n"
               "@item Self_alignment_interface::general_[xy]_alignment\n"
               "Align object's reference point on parent's reference point;\n"
               " reference points are taken from @code{X-alignment} or\n"
               " @code{Y-alignment} properties, respectively\n"
               "@end table\n",

               /* properties */
               "collision-bias "
               "collision-padding "
               "potential-X-colliding-grobs "
               "self-alignment-X "
               "self-alignment-Y "
               "X-alignment "
               "Y-alignment "
               "X-colliding-grobs "
               "Y-colliding-grobs "
              );
