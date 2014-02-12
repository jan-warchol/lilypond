/*
  This file is part of LilyPond, the GNU music typesetter.

  Copyright (C) 2004--2014 Jan Nieuwenhuizen <janneke@gnu.org>

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

#include "paper-system.hh"
#include "international.hh"
#include "item.hh"

Prob *
make_paper_system (SCM immutable_init)
{
  Prob *prob = new Prob (ly_symbol2scm ("paper-system"), immutable_init);
  return prob;
}

/*
  TODO
  it might be interesting to split off the footnotes as well, ie.

  get_footnotes(SCM expr, SCM* footnotes, SCM* cleaned)

  by doing it this way and overwriting the old expr in the caller,
  you can make sure nobody tries to handle footnotes differently
  downstream.
*/
SCM
get_footnotes (SCM expr)
{
  if (!scm_is_pair (expr))
    return SCM_EOL;

  SCM head = scm_car (expr);

  if (head == ly_symbol2scm ("delay-stencil-evaluation"))
    {
      // we likely need to do something here...just don't know what...
      return SCM_EOL;
    }

  if (head == ly_symbol2scm ("combine-stencil"))
    {
      SCM out = SCM_EOL;
      SCM *tail = &out;

      for (SCM x = scm_cdr (expr); scm_is_pair (x); x = scm_cdr (x))
        {
          SCM footnote = get_footnotes (scm_car (x));
          if (SCM_EOL != footnote)
            {
              *tail = scm_cons (footnote, SCM_EOL);
              tail = SCM_CDRLOC (*tail);
            }
        }
      return scm_append (out);
    }
  if (head == ly_symbol2scm ("translate-stencil"))
    return get_footnotes (scm_caddr (expr));

  if (head == ly_symbol2scm ("footnote"))
    return scm_list_1 (scm_cdr (expr));

  return SCM_EOL;
}

void
paper_system_set_stencil (Prob *prob, Stencil s)
{
  SCM yext = prob->get_property ("Y-extent");

  if (is_number_pair (yext))
    {
      Box b = s.extent_box ();
      b[Y_AXIS] = ly_scm2interval (yext);

      s = Stencil (b, s.expr ());
    }

  prob->set_property ("stencil", s.smobbed_copy ());
}
