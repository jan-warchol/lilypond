/*
  This file is part of LilyPond, the GNU music typesetter.

  Copyright (C) 2005--2012 Han-Wen Nienhuys <hanwen@xs4all.nl>


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

#include "tie-configuration.hh"

#include "warn.hh"
#include "tie-formatting-problem.hh"
#include "bezier.hh"

int
Tie_configuration::compare (Tie_configuration const &a,
                            Tie_configuration const &b)
{
  if (a.position_ - b.position_)
    return sign (a.position_ - b.position_);
  return sign (a.dir_ - b.dir_);
}

Tie_configuration::Tie_configuration ()
{
  dir_ = CENTER;
  position_ = 0;
  delta_y_ = 0.0;
  score_ = 0.0;
  scored_ = false;
  column_ranks_ = Drul_array<int> (0, 0);
}

void
Tie_configuration::center_tie_vertically (Tie_details const &details)
{
  Bezier b = get_untransformed_bezier (details);
  Offset middle = b.curve_point (0.5);
  Offset edge = b.curve_point (0.0);
  Real center = (edge[Y_AXIS] + middle[Y_AXIS]) / 2.0;

  delta_y_ = - dir_ * center;
}

/*
  Apparently, things happen in the following way:
  - we determine exact postions of the tie ends (i.e. attachment_x_ and delta_y_)
  - we calc tie length l (from attachment_x_) and we draw a "default-shaped tie"
    of length l and height based on some general settings
  - we flip the tie if the direction is down
  - we translate the tie to the actual attachment points.
  */
Bezier
Tie_configuration::get_transformed_bezier (Tie_details const &details) const
{
  Bezier b (get_untransformed_bezier (details));

  b.scale (1, dir_);
  b.translate (Offset (attachment_x_[LEFT],
                       delta_y_ + details.staff_space_ * 0.5 * position_));

  return b;
}

/*
  Get bezier with left control at (0,0)
 */
Bezier
Tie_configuration::get_untransformed_bezier (Tie_details const &details) const
{
  Real l = attachment_x_.length ();
  if (isinf (l) || isnan (l))
    {
      programming_error ("Inf or NaN encountered");
      l = 1.0;
    }
  return slur_shape (l,
                     details.height_limit_,
                     details.ratio_);
}

int
Tie_configuration::column_span_length () const
{
  return column_ranks_[RIGHT] - column_ranks_[LEFT];
}

Real
Tie_configuration::distance (Tie_configuration const &a,
                             Tie_configuration const &b)
{

  Real d = 3 * (a.position_ - b.position_);
  if (d < 0)
    return d + (2 + (b.dir_ - a.dir_));
  else
    return d + (2 + (a.dir_ - b.dir_));
}

//score == uglyness points
void
Tie_configuration::add_score (Real s, string desc)
{
  assert (!scored_);
  score_ += s;
//#if DEBUG_SLUR_SCORING
  if (s)
    score_card_ += to_string ("%s=%.2f ", desc.c_str (), s);
  ///< That is a kind of efficiency catastrophe.
  /*!
   * If that was only in debug version, it would be OK, but it is present in normal, release
   * version of Lilypond. What's more, it wouldn't compile without DEBUG_SLUR_SCORING
   * (because of #if in header) - easiest solution is another #if here, which i added in comment.
   * By the way, passing std::string by value is very inefficient and that problem is
   * very common in Lilypond code - i believe it can have significant influence on Lilypond speed.
   *
   * And results of collecting these score_cards can (now) be accessed only through debugger.
   */
//#endif
}

/// Height of tie, measured between tip-points and top, or more specificly,
/// height of curve in its middle point (half of length).

// wouldn't it be simpler to have a direct tie_height(tie_length) function,
// instead of calculating whole bezier curve in the process?
// Current approach seems to be very broken to me.

Real
Tie_configuration::height (Tie_details const &details) const
{
  Real l = attachment_x_.length ();

  return slur_shape (l,
                     details.height_limit_,
                     details.ratio_).curve_point (0.5)[Y_AXIS];
}

Ties_configuration::Ties_configuration ()
{
  score_ = 0.0;
  scored_ = false;
}

void
Ties_configuration::reset_score ()
{
  score_ = 0.0;
  scored_ = false;
  /// Another thing that wouldn't compile without #if DEBUG_SLUR_SCORING
  score_card_ = "";
  tie_score_cards_.clear ();
}

void
Ties_configuration::add_tie_score (Real s, int i, string desc)
{
  assert (!scored_);
  score_ += s;
  if (s)
    {
      while (tie_score_cards_.size () < size ())
        tie_score_cards_.push_back ("");

      /// And this is another costly collected description accessible only through debugger
      tie_score_cards_[i] += to_string ("%s=%.2f ", desc.c_str (), s);
    }
}

void
Ties_configuration::add_score (Real s, string desc)
{
  assert (!scored_);
  score_ += s;
  /// Another thing that wouldn't compile without #if DEBUG_SLUR_SCORING
  if (s)
    score_card_ += to_string ("%s=%.2f ", desc.c_str (), s);
}

Real
Ties_configuration::score () const
{
  return score_;
}

string
Ties_configuration::complete_tie_card (vsize i) const
{
  string s;
  s += to_string ("%d (%.2f) %c: ", (*this)[i].position_, (*this)[i].delta_y_,
                  ((*this)[i].dir_ == UP ? 'u' : 'd'))
       + (*this)[i].card () + (*this).tie_card (i);

  /*
    this is a little awkward, but we must decide where to put
    aggregrates.
   */
  if (i == 0)
    s += card ();

  if (i + 1 == size ())
    s += to_string ("TOTAL=%.2f", score ());

  return s;
}

/* for use inside GDB */
string
Ties_configuration::complete_score_card () const
{
  string s;
  for (vsize i = 0; i < size (); i++)
    {
      s += complete_tie_card (i);
    }

  return s;
}

string
Ties_configuration::card () const
{
  /// Another thing that wouldn't compile without #if DEBUG_SLUR_SCORING
  return score_card_;
}

