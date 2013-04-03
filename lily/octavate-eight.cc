/*
  This file is part of LilyPond, the GNU music typesetter.

  Copyright (C) 2013--2013 Janek Warchoł <lemniskata.bernoullego@gmail.com>

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

#include "item.hh"

struct Octavate_eight
{
  DECLARE_SCHEME_CALLBACK (calc_alignment, (SCM));
  DECLARE_GROB_INTERFACE ();
};

MAKE_SCHEME_CALLBACK (Octavate_eight, calc_alignment, 1)
SCM
Octavate_eight::calc_alignment (SCM smob)
{
  Grob *me = unsmob_grob (smob);
  Grob *clef = me->get_parent (X_AXIS);
  string full_clef_name = ly_scm2string (clef->get_property ("glyph"));

  int separator_position = full_clef_name.find ('.');
  string clef_type = full_clef_name.substr (separator_position + 1,
                                            separator_position + 2);

  // find entry with keyname clef_type in octavate-eight-alignments
  SCM alist_entry = scm_assq (ly_symbol2scm (clef_type.c_str ()),
                              me->get_property ("octavate-eight-alignments"));

  if (scm_is_pair (alist_entry))
    {
      SCM entry_value = scm_cdr (alist_entry);
      // the value should be a pair of numbers - first is the alignment
      // for octavations below the clef, second for those above.
      if (scm_is_pair (entry_value))
        if (robust_scm2dir (me->get_property ("direction"), DOWN) == DOWN)
          return scm_car (entry_value);
        else
          return scm_cdr (entry_value);

      else // default alignment = centered
        return scm_from_double (0);
    }
  else // default alignment = centered
    return scm_from_double (0);
}

ADD_INTERFACE (Octavate_eight,
               "The number describing transposition of the clef, placed below\n"
               "or above clef sign. Usually this is 8 (octave transposition)\n"
               "or 15 (two octaves), but LilyPond allows any integer here.",

               /* properties */
               "octavate-eight-alignments "
              );

