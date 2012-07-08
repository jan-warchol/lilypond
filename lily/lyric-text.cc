/*
  This file is part of LilyPond, the GNU music typesetter.

  Copyright (C) 2012 Jan Warchoł <janek.lilypond@gmail.com>

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
#include "self-alignment-interface.hh"
#include "lyric-hyphen.hh"

class Lyric_text
{
public:
  DECLARE_SCHEME_CALLBACK (calc_x_offset, (SCM));
  DECLARE_GROB_INTERFACE ();
};

/* Restrict position of default-aligned long syllables
 * so that they don't start earlier than minimum-X-offset from the note.
 * (doesn't affect syllables with explicitly specified X-alignment)
 */
MAKE_SCHEME_CALLBACK (Lyric_text, calc_x_offset, 1);
SCM
Lyric_text::calc_x_offset (SCM smob)
{
  Grob *me = unsmob_grob (smob);
  SCM align_prop (me->get_property (ly_symbol2scm ("X-alignment")));

  Real restriction = (align_prop != SCM_EOL)
                     ? -infinity_f
                     : robust_scm2double (me->get_property ("minimum-X-offset"), -2.2);

  if (align_prop == SCM_EOL)
    me->set_property ("X-alignment", me->get_property ("default-X-alignment"));

  Real offset = scm_to_double (Self_alignment_interface::general_x_alignment (smob));
  message (to_string (Lyric_hyphen::ziomal ()));
  Grob *hyph = unsmob_grob (me->get_object ("LyricHyphen"));
  message (hyph->name ());
  return scm_from_double (max (offset, restriction));
}

ADD_INTERFACE (Lyric_text,
               "blah.",

               /* properties */
               "default-X-alignment "
               "minimum-X-offset "
              );
