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
#include "font-interface.hh"
#include "output-def.hh"
#include "stencil.hh"
#include "international.hh"

class Lyric_text
{
public:
  DECLARE_GROB_INTERFACE ();
  DECLARE_SCHEME_CALLBACK (calc_core_extent, (SCM element));
};

MAKE_SCHEME_CALLBACK (Lyric_text, calc_core_extent, 1)
SCM
Lyric_text::calc_core_extent (SCM smob)
{
  Grob *me = unsmob_grob (smob);
  Interval result_extent = me->extent (me, X_AXIS);

  SCM lyric_text_scm = me->get_property ("text");

  if (scm_is_string (lyric_text_scm))
    {
      string lyric_text = ly_scm2string (lyric_text_scm);
      string prefix = "";
      string suffix = "";
      size_t prefix_index = lyric_text.find ('&');
      size_t suffix_index = lyric_text.find ('$');

      if ((int) prefix_index >= 0)
        prefix = lyric_text.substr (0, prefix_index++);
      else
        prefix_index = 0;

      if ((int) suffix_index >= 0)
        suffix = lyric_text.substr (suffix_index-- + 1, lyric_text.size ());
      else
        suffix_index = lyric_text.size () - 1;

      SCM font_chain = Font_interface::text_font_alist_chain (me);
      SCM punctuation_list = ly_chain_assoc_get (ly_symbol2scm ("punctuation-list"),
                                                 font_chain,
                                                 SCM_EOL);

      for (vsize i = prefix_index; i < lyric_text.size (); i++)
        {
          string addon = "";
          SCM punct = punctuation_list;
          do
            {
              if (lyric_text.substr (i, 1) == robust_scm2string (scm_car (punct), ""))
                {
                  addon = lyric_text.substr (i, 1);
                  break;
                }
            }
          while ((punct = scm_cdr (punct)) != SCM_EOL);

          if (addon != "")
            prefix = prefix + addon;
          else
            break;
        }

      for (vsize i = suffix_index; i >= 0; i--)
        {
          string addon = "";
          SCM punct = punctuation_list;
          do
            {
              if (lyric_text.substr (i, 1) == robust_scm2string (scm_car (punct), ""))
                {
                  addon = lyric_text.substr (i, 1);
                  break;
                }
            }
          while ((punct = scm_cdr (punct)) != SCM_EOL);

          if (addon != "")
            suffix = addon + suffix;
          else
            break;
        }

      Output_def *layout = unsmob_output_def (me->layout ()->self_scm ());
      Font_metric *fm = select_encoded_font (layout, font_chain);

      Stencil prefix_stencil = fm->text_stencil (layout, prefix, false);
      Stencil suffix_stencil = fm->text_stencil (layout, suffix, false);
      result_extent[LEFT] += prefix_stencil.extent (X_AXIS).length ();
      result_extent[RIGHT] -= suffix_stencil.extent (X_AXIS).length ();
    }
  return ly_interval2scm (result_extent);
}

ADD_INTERFACE (Lyric_text,
               "blah.",

               /* properties */
               "X-extent "
               "X-core-extent "
               "punctuation-list "
              );
