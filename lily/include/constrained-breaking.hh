/*
  This file is part of LilyPond, the GNU music typesetter.

  Copyright (C) 2006--2014 Joe Neeman <joeneeman@gmail.com>

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

#ifndef CONSTRAINED_BREAKING_HH
#define CONSTRAINED_BREAKING_HH

#include "lily-guile.hh"
#include "matrix.hh"
#include "prob.hh"

/*
 * Begin/rest-of-line hack.  This geometrical shape is a crude approximation
 * of Skyline, but it is better than a rectangle.
 */
struct Line_shape
{
  Interval begin_;
  Interval rest_;

  Line_shape ()
  {
  }
  Line_shape (Interval begin, Interval rest);
  Line_shape piggyback (Line_shape mount, Real padding) const;
};

struct Line_details
{
  Grob *last_column_;
  Real force_;
  Line_shape shape_;
  vector<Real> footnote_heights_; /* The footnotes at the bottom of the
                                   page, where each stencil represents
                                   a different footnote. */
  vector<Real> in_note_heights_; /* The in-notes under a system,
                                   where each stencil represents
                                   a different in-note. */
  Interval refpoint_extent_; /* The refpoints of the first and last
                                spaceable staff in this line.  min-distance
                                should be measured from the bottom
                                refpoint_extent of one line to the
                                top refpoint_extent of the next. */
  Real tallness_; /* Y-extent, adjusted according to begin/rest-of-line*/

  Real padding_;  /* compulsory space after this system (if we're not
                     last on a page) */
  Real title_padding_;
  Real min_distance_;
  Real title_min_distance_;
  Real bottom_padding_;
  Real space_;    /* spring length */
  Real title_space_;
  Real inverse_hooke_;

  SCM break_permission_;
  SCM page_permission_;
  SCM turn_permission_;
  Real break_penalty_;
  Real page_penalty_;
  Real turn_penalty_;

  bool title_;

  /* The page-breaker deals with forbidden page breaks by "compressing"
     two Line_detailses into one. The following fields are used by the
     page-breaker to keep track of this. If the number of fields needed
     by the page-breaker grows, it might be a good idea to create a separate
     class. */
  int compressed_lines_count_;
  int compressed_nontitle_lines_count_;
  bool last_markup_line_;
  bool first_markup_line_;
  bool tight_spacing_;

  Line_details ()
  {
    last_column_ = 0;
    force_ = infinity_f;
    padding_ = 0;
    title_padding_ = 0;
    bottom_padding_ = 0;
    min_distance_ = 0;
    title_min_distance_ = 0;
    space_ = 0;
    title_space_ = 0;
    inverse_hooke_ = 1;
    tight_spacing_ = false;
    break_permission_ = ly_symbol2scm ("allow");
    page_permission_ = ly_symbol2scm ("allow");
    turn_permission_ = ly_symbol2scm ("allow");
    break_penalty_ = 0;
    page_penalty_ = 0;
    turn_penalty_ = 0;
    title_ = false;
    compressed_lines_count_ = 1;
    compressed_nontitle_lines_count_ = 1;
    last_markup_line_ = false;
    first_markup_line_ = false;
    tallness_ = 0;
    refpoint_extent_ = Interval (0, 0);
  }

  Line_details (Prob *pb, Output_def *paper);
  Real full_height () const;
  Real tallness () const;
  Real spring_length (Line_details const &next_line) const;
};

/*
   Helper to trace back an optimal path
*/
struct Constrained_break_node
{
  /* the number of bars in all the systems before this one
  */
  int prev_;

  /* unlike the Gourlay breaker, this is the sum of all demerits up to,
   * and including, this line */
  Real demerits_;
  struct Line_details details_;

  Constrained_break_node ()
  {
    prev_ = -1;
    demerits_ = infinity_f;
  }

  void print () const
  {
    printf ("prev break %d, demerits %f\n", prev_, demerits_);
  }
};

/*
   A dynamic programming solution to breaking scores into lines
*/
class Constrained_breaking
{
public:
  vector<Column_x_positions> solve (vsize start, vsize end, vsize sys_count);
  vector<Column_x_positions> best_solution (vsize start, vsize end);
  vector<Line_details> line_details (vsize start, vsize end, vsize sys_count);

  Constrained_breaking (Paper_score *ps);
  Constrained_breaking (Paper_score *ps, vector<vsize> const &start_col_posns);

  int max_system_count (vsize start, vsize end);
  int min_system_count (vsize start, vsize end);

private:
  Paper_score *pscore_;
  vsize valid_systems_;
  vsize systems_;
  bool ragged_right_;
  bool ragged_last_;

  Real system_system_min_distance_;
  Real system_system_padding_;
  Real system_system_space_;
  Real system_markup_space_;
  Real score_system_min_distance_;
  Real score_system_padding_;
  Real score_markup_min_distance_;
  Real score_markup_padding_;

  /* the (i,j)th entry is the configuration for breaking between
    columns i and j */
  Matrix<Line_details> lines_;

  /* the [i](j,k)th entry is the score for fitting the first k bars onto the
    first j systems, starting at the i'th allowed starting column */
  vector<Matrix<Constrained_break_node> > state_;

  vector<vsize> start_;         /* the columns at which we might be asked to start breaking */
  vector<vsize> starting_breakpoints_; /* the corresponding index in breaks_ */

  vector<Grob *> all_;
  vector<vsize> breaks_;

  void initialize ();
  void resize (vsize systems);

  Column_x_positions space_line (vsize start_col, vsize end_col);
  vsize prepare_solution (vsize start, vsize end, vsize sys_count);

  Real combine_demerits (Real force, Real prev_force);

  bool calc_subproblem (vsize start, vsize systems, vsize max_break_index);
  void fill_line_details (Line_details *, vsize, vsize);
};
#endif /* CONSTRAINED_BREAKING_HH */
