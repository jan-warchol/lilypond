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

#include "slur-configuration.hh"

#include "item.hh"
#include "libc-extension.hh"
#include "misc.hh"
#include "pointer-group-interface.hh"
#include "slur-scoring.hh"
#include "slur.hh"
#include "spanner.hh"
#include "staff-symbol-referencer.hh"
#include "stem.hh"
#include "tie.hh"
#include "warn.hh"

// Move slur's "belly" (the middle section, where it becomes flat)
// so that it avoids the stafflines. There are two distinct cases:
//
// belly above line:
//
//    _          _..------.._
//  x |     _.-''            ''-._
// ___|___.'______________________'._____staffline
//    ^ .'                          '.
//     /                              \
//
//
// belly below line:
//
// __________.___________________________staffline
//         x |
//           ^   _..------.._
//          _.-''            ''-._
//        .'                      '.
//      .'                          '.
//
//
// We call x (distance between slur and staffline) "clearance" or "gap".
// In the "belly above" case we need more clearance than in the other case.
//
// The function is implemented so that we're more likely to increase curvature
// rather than decrease, because we want to avoid too flat slurs.
//
// Note that we also want the slur tips to avoid stafflines, but that's done
// in another function.

Bezier
avoid_staff_line (Slur_score_state const &state,
                  Bezier bez)
{
  // Get the point(s) where the curve is horizontal (i.e. the belly):
  Offset horiz (1, 0);
  vector<Real> ts = bez.solve_derivative (horiz);

  /* TODO: handle case of broken slur.  */
  if (!ts.empty ()
      && (state.extremes_[LEFT].staff_ == state.extremes_[RIGHT].staff_)
      && state.extremes_[LEFT].staff_ && state.extremes_[RIGHT].staff_)
    {
      // There could be more than one solution if we supported S-slurs.
      Real t = ts[0];
      Real y = bez.curve_point (t)[Y_AXIS];

      Grob *staff = state.extremes_[LEFT].staff_;
      Real const staff_th = Staff_symbol_referencer::line_thickness (staff);
      Real const slur_th = state.thickness_ * staff_th * 10;
      Interval const min_gap = Interval (state.parameters_.min_gap_below_staffline_,
                                         state.parameters_.min_gap_above_staffline_);

      // normalized to staffspaces and upward slurs.
      Real norm_y = (y - staff->relative_coordinate (state.common_[Y_AXIS], Y_AXIS))
                    / state.staff_space_
                    * state.dir_;

      // TODO: to handle weird staves well, we should round to staffline positions.
      Real const round = my_round (norm_y);
      Real const gap = fabs (norm_y - round) - (slur_th + staff_th) / 2;
      Direction which_side = (round <= norm_y) ? UP : DOWN;

      if (gap < min_gap [which_side]
          && Staff_symbol_referencer::on_staff_line (staff, int (2 * round)))
        {
          Real correction = (min_gap [which_side] - gap) * which_side;
          correction *= state.staff_space_ * state.dir_;
          /*
            We apply the correction in two steps: first we move the whole
            slur by a fraction of the correction, and then we increase
            the curvature by moving middle control-points (We could have
            moved only the middle control-points, but that could disturb
            the curvature too much):
          */
          bez.translate (Offset (0, 0.4 * correction));
          /*
            Now, the rest of the correction is applied by increasing curvature.
            The belly position is given by the following equation
            b = P0*(1-t)^3 + P1*t*(1-t)^2 + P2*(1-t)*t^2 + P3*t^3
            where P0-P3 are the control points - see
            http://en.wikipedia.org/wiki/B%C3%A9zier_curve#Higher-order_curves
            P0 and P3 will remain unchanged, so
            correction = dP1*3*t*(t-1)^2 + dP2*3*(1-t)*t^2
            and dP1=dP2.
          */
          Real mid_pts_cor = 0.6 * correction / (3 * (t - (t * t)));
          bez.control_[1][Y_AXIS] += mid_pts_cor;
          bez.control_[2][Y_AXIS] += mid_pts_cor;
        }
    }
  return bez;
}

Real
fit_factor (Offset dz_unit, Offset dz_perp, Real close_to_edge_length,
            Bezier curve, Direction d, vector<Offset> const &avoid)
{
  Real fit_factor = 0.0;
  Offset x0 = curve.control_[0];
  curve.translate (-x0);
  curve.rotate (-dz_unit.arg ());
  curve.scale (1, d);

  Interval curve_xext;
  curve_xext.add_point (curve.control_[0][X_AXIS]);
  curve_xext.add_point (curve.control_[3][X_AXIS]);

  for (vsize i = 0; i < avoid.size (); i++)
    {
      Offset z = (avoid[i] - x0);
      Offset p (dot_product (z, dz_unit),
                d * dot_product (z, dz_perp));

      bool close_to_edge = false;
      for (LEFT_and_RIGHT (d))
        close_to_edge = close_to_edge || -d * (p[X_AXIS] - curve_xext[d]) < close_to_edge_length;

      if (close_to_edge)
        continue;

      Real eps = 0.01;
      Interval pext = eps * Interval (-1, 1) + p[X_AXIS];
      pext.intersect (curve_xext);

      if (pext.is_empty () || pext.length () <= 1.999 * eps)
        continue;

      Real y = curve.get_other_coordinate (X_AXIS, p[X_AXIS]);
      if (y)
        fit_factor = max (fit_factor, (p[Y_AXIS] / y));
    }
  return fit_factor;
}

void
Slur_configuration::generate_curve (Slur_score_state const &state,
                                    Real r_0, Real h_inf,
                                    vector<Offset> const &avoid)
{
  Offset dz = attachment_[RIGHT] - attachment_[LEFT];;
  Offset dz_unit = dz;
  dz_unit *= 1 / dz.length ();
  Offset dz_perp = dz_unit * Offset (0, 1);

  Real indent, height;
  get_slur_indent_height (&indent, &height, dz.length (), h_inf, r_0);

  Real len = dz.length ();

  /* This condition,

  len^2 > 4h^2 +  3 (i + 1/3len)^2  - 1/3 len^2

  is equivalent to:

  |bez' (0)| < | bez' (.5)|

  when (control2 - control1) has the same direction as
  (control3 - control0).  */

  Real max_indent = len / 3.1;
  indent = min (indent, max_indent);

  Real a1 = sqr (len) / 3.0;
  Real a2 = 0.75 * sqr (indent + len / 3.0);
  Real max_h = a1 - a2;

  if (max_h < 0)
    {
      programming_error ("slur indent too small");
      max_h = len / 3.0;
    }
  else
    max_h = sqrt (max_h);

  Real eccentricity = robust_scm2double (state.slur_->get_property ("eccentricity"), 0);

  Real x1 = (eccentricity + indent);
  Real x2 = (eccentricity - indent);

  Bezier curve;
  curve.control_[0] = attachment_[LEFT];
  curve.control_[1] = attachment_[LEFT] + dz_perp * height * state.dir_
                      + dz_unit * x1;
  curve.control_[2] = attachment_[RIGHT] + dz_perp * height * state.dir_
                      + dz_unit * x2;
  curve.control_[3] = attachment_[RIGHT];

  Real ff = fit_factor (dz_unit, dz_perp, state.parameters_.close_to_edge_length_,
                        curve, state.dir_, avoid);

  height = max (height, min (height * ff, max_h));

  curve.control_[0] = attachment_[LEFT];
  curve.control_[1] = attachment_[LEFT] + dz_perp * height * state.dir_
                      + dz_unit * x1;
  curve.control_[2] = attachment_[RIGHT] + dz_perp * height * state.dir_
                      + dz_unit * x2;
  curve.control_[3] = attachment_[RIGHT];

  curve_ = avoid_staff_line (state, curve);
  height_ = height;
}

Slur_configuration::Slur_configuration ()
{
  score_ = 0.0;
  index_ = -1;
};

void
Slur_configuration::add_score (Real s, const string &desc)
{
  if (s < 0)
    {
      programming_error ("Negative demerits found for slur.  Ignoring");
      s = 0.0;
    }

  if (s)
    {
      if (score_card_.length () > 0)
        score_card_ += ", ";
      score_card_ += to_string ("%s=%.2f", desc.c_str (), s);
      score_ += s;
    }
}

void
Slur_configuration::score_encompass (Slur_score_state const &state)
{
  Bezier const &bez (curve_);
  Real demerit = 0.0;

  /*
    Distances for heads that are between slur and line between
    attachment points.
  */
  vector<Real> convex_head_distances;
  for (vsize j = 0; j < state.encompass_infos_.size (); j++)
    {
      Real x = state.encompass_infos_[j].x_;

      bool l_edge = j == 0;
      bool r_edge = j == state.encompass_infos_.size () - 1;
      bool edge = l_edge || r_edge;

      if (! (x < attachment_[RIGHT][X_AXIS]
             && x > attachment_[LEFT][X_AXIS]))
        continue;

      Real y = bez.get_other_coordinate (X_AXIS, x);
      if (!edge)
        {
          Real head_dy = (y - state.encompass_infos_[j].head_);
          if (state.dir_ * head_dy < 0)
            {
              demerit += state.parameters_.head_encompass_penalty_;
              convex_head_distances.push_back (0.0);
            }
          else
            {
              Real hd = (head_dy)
                        ? (1 / fabs (head_dy) - 1 / state.parameters_.free_head_distance_)
                        : state.parameters_.head_encompass_penalty_;
              hd = min (max (hd, 0.0), state.parameters_.head_encompass_penalty_);

              demerit += hd;
            }

          Real line_y = linear_interpolate (x,
                                            attachment_[RIGHT][X_AXIS],
                                            attachment_[LEFT][X_AXIS],
                                            attachment_[RIGHT][Y_AXIS],
                                            attachment_[LEFT][Y_AXIS]);

          if (1) // state.dir_ * state.encompass_infos_[j].get_point (state.dir_) > state.dir_ *line_y )
            {

              Real closest
                = state.dir_ * max (state.dir_ * state.encompass_infos_[j].get_point (state.dir_), state.dir_ * line_y);
              Real d = fabs (closest - y);

              convex_head_distances.push_back (d);
            }
        }

      if (state.dir_ * (y - state.encompass_infos_[j].stem_) < 0)
        {
          Real stem_dem = state.parameters_.stem_encompass_penalty_;
          if ((l_edge && state.dir_ == UP)
              || (r_edge && state.dir_ == DOWN))
            stem_dem /= 5;

          demerit += stem_dem;
        }
    }
  add_score (demerit, "encompass");

  if (vsize n = convex_head_distances.size ())
    {
      Real avg_distance = 0.0;
      Real min_dist = infinity_f;

      for (vsize j = 0; j < n; j++)
        {
          min_dist = min (min_dist, convex_head_distances[j]);
          avg_distance += convex_head_distances[j];
        }

      /*
        For slurs over 3 or 4 heads, the average distance is not a
        good normalizer.
      */
      if (n <= 2)
        {
          Real fact = 1.0;
          avg_distance += height_ * fact;
          ++n;
        }

      /*
        TODO: maybe it's better to use (avgdist - mindist)*factor
        as penalty.
      */
      avg_distance /= n;
      Real variance_penalty = state.parameters_.head_slur_distance_max_ratio_;
      if (min_dist > 0.0)
        variance_penalty
          = min ((avg_distance / (min_dist + state.parameters_.absolute_closeness_measure_) - 1.0), variance_penalty);

      variance_penalty = max (variance_penalty, 0.0);
      variance_penalty *= state.parameters_.head_slur_distance_factor_;

      add_score (variance_penalty, "variance");
    }
}

void
Slur_configuration::score_extra_encompass (Slur_score_state const &state)
{
  // we find forbidden attachments
  vector<Offset> forbidden_attachments;
  for (vsize i = 0; i < state.extra_encompass_infos_.size (); i++)
    if (Tie::has_interface (state.extra_encompass_infos_[i].grob_))
      {
        Grob *t = state.extra_encompass_infos_[i].grob_;
        Grob *common_x = Grob::get_vertical_axis_group (t);
        Real rp = t->relative_coordinate (common_x, X_AXIS);
        SCM cp = t->get_property ("control-points");

        Bezier b;
        int j = 0;
        for (SCM s = cp; scm_is_pair (s); s = scm_cdr (s))
          {
            b.control_[j] = ly_scm2offset (scm_car (s));
            j++;
          }
        forbidden_attachments.push_back (Offset (b.control_[0]) + Offset (rp, 0));
        forbidden_attachments.push_back (Offset (b.control_[3]) + Offset (rp, 0));
      }

  bool too_close = false;
  for (vsize k = 0; k < forbidden_attachments.size (); k++)
    for (LEFT_and_RIGHT (side))
      if ((forbidden_attachments[k] - attachment_[side]).length () < state.parameters_.slur_tie_extrema_min_distance_)
        {
          too_close = true;
          break;
        }

  if (too_close)
    add_score (state.parameters_.slur_tie_extrema_min_distance_penalty_, "extra");

  for (vsize j = 0; j < state.extra_encompass_infos_.size (); j++)
    {
      Drul_array<Offset> attachment = attachment_;
      Extra_collision_info const &info (state.extra_encompass_infos_[j]);

      Interval slur_wid (attachment[LEFT][X_AXIS], attachment[RIGHT][X_AXIS]);

      /*
        to prevent numerical inaccuracies in
        Bezier::get_other_coordinate ().
      */

      bool found = false;
      Real y = 0.0;

      for (LEFT_and_RIGHT (d))
        {
          /*
            We need to check for the bound explicitly, since the
            slur-ending can be almost vertical, making the Y
            coordinate a bad approximation of the object-slur
            distance.
          */
          Item *as_item = dynamic_cast<Item *> (state.extra_encompass_infos_[j].grob_);
          if (!as_item)
            continue;

          Interval item_x = as_item->extent (state.common_[X_AXIS], X_AXIS);
          item_x.intersect (state.extremes_[d].slur_head_x_extent_);
          if (!item_x.is_empty ())
            {
              y = attachment[d][Y_AXIS];
              found = true;
            }

        }

      if (!found)
        {
          Real x = info.extents_[X_AXIS].linear_combination (info.idx_);

          if (!slur_wid.contains (x))
            continue;

          y = curve_.get_other_coordinate (X_AXIS, x);
        }

      Real dist = 0.0;
      if (info.type_ == ly_symbol2scm ("around"))
        dist = info.extents_[Y_AXIS].distance (y);

      /*
        Have to score too: the curve enumeration is limited in its
        shape, and may produce curves which collide anyway.
       */
      else if (info.type_ == ly_symbol2scm ("inside"))
        dist = state.dir_ * (y - info.extents_[Y_AXIS][state.dir_]);
      else
        programming_error ("unknown avoidance type");

      dist = max (dist, 0.0);

      Real penalty = info.penalty_ * peak_around (0.1 * state.parameters_.extra_encompass_free_distance_,
                                                  state.parameters_.extra_encompass_free_distance_,
                                                  dist);

      add_score (penalty, "extra");
    }
}

void
Slur_configuration::score_edges (Slur_score_state const &state)
{

  Offset dz = attachment_[RIGHT]
              - attachment_[LEFT];
  Real slope = dz[Y_AXIS] / dz[X_AXIS];
  for (LEFT_and_RIGHT (d))
    {
      Real y = attachment_[d][Y_AXIS];
      Real dy = fabs (y - state.base_attachments_[d][Y_AXIS]);

      Real factor = state.parameters_.edge_attraction_factor_;
      Real demerit = factor * dy;
      if (state.extremes_[d].stem_
          && state.extremes_[d].stem_dir_ == state.dir_
          // TODO - Stem::get_beaming() should be precomputed.
          && !Stem::get_beaming (state.extremes_[d].stem_, -d))
        demerit /= 5;

      demerit *= exp (state.dir_ * d * slope
                      * state.parameters_.edge_slope_exponent_);

      string dir_str = d == LEFT ? "L" : "R";
      add_score (demerit, dir_str + " edge");
    }
}

void
Slur_configuration::score_slopes (Slur_score_state const &state)
{
  Real dy = state.musical_dy_;
  Offset slur_dz = attachment_[RIGHT] - attachment_[LEFT];
  Real slur_dy = slur_dz[Y_AXIS];
  Real demerit = 0.0;

  demerit += max ((fabs (slur_dy / slur_dz[X_AXIS])
                   - state.parameters_.max_slope_), 0.0)
             * state.parameters_.max_slope_factor_;

  /* 0.2: account for staffline offset. */
  Real max_dy = (fabs (dy) + 0.2);
  if (state.edge_has_beams_)
    max_dy += 1.0;

  if (!state.is_broken_)
    demerit += state.parameters_.steeper_slope_factor_
               * (max (fabs (slur_dy) - max_dy, 0.0));

  demerit += max ((fabs (slur_dy / slur_dz[X_AXIS])
                   - state.parameters_.max_slope_), 0.0)
             * state.parameters_.max_slope_factor_;

  if (sign (dy) == 0
      && sign (slur_dy) != 0
      && !state.is_broken_)
    demerit += state.parameters_.non_horizontal_penalty_;

  if (sign (dy)
      && !state.is_broken_
      && sign (slur_dy)
      && sign (slur_dy) != sign (dy))
    demerit += state.edge_has_beams_
               ? state.parameters_.same_slope_penalty_ / 10
               : state.parameters_.same_slope_penalty_;

  add_score (demerit, "slope");
}

// This is a temporary hack to see how much we can gain by using a
// priority queue on the beams to score.
static int score_count = 0;
LY_DEFINE (ly_slur_score_count, "ly:slur-score-count", 0, 0, 0,
           (),
           "count number of slur scores.")
{
  return scm_from_int (score_count);
}

void
Slur_configuration::run_next_scorer (Slur_score_state const &state)
{
  switch (next_scorer_todo)
    {
    case EXTRA_ENCOMPASS:
      score_extra_encompass (state);
      break;
    case SLOPE:
      score_slopes (state);
      break;
    case EDGES:
      score_edges (state);
      break;
    case ENCOMPASS:
      score_encompass (state);
      break;
    default:
      assert (false);
    }
  next_scorer_todo++;
  score_count++;
}

bool
Slur_configuration::done () const
{
  return next_scorer_todo >= NUM_SCORERS;
}

Slur_configuration *
Slur_configuration::new_config (Drul_array<Offset> const &offs, int idx)
{
  Slur_configuration *conf = new Slur_configuration;
  conf->attachment_ = offs;
  conf->index_ = idx;
  conf->next_scorer_todo = INITIAL_SCORE + 1;
  return conf;
}
