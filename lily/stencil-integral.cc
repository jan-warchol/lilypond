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

/*
tools for transform-matrices following the standard at
http://www.w3.org/TR/SVG/coords.html

a list in the form
(list a b c d e f g)
becomes this matrix:
[ a c e ]
[ b d f ]
[ 0 0 1 ]
when this transforms a point (x,y), the point is written as matrix:
[ x ]
[ y ]
[ 1 ]
*/

#include <pango/pango-matrix.h>
#include <complex>
#include "box.hh"
#include "bezier.hh"
#include "font-metric.hh"
#include "grob.hh"
#include "interval.hh"
#include "freetype.hh"
#include "misc.hh"
#include "offset.hh"
#include "modified-font-metric.hh"
#include "open-type-font.hh"
#include "pango-font.hh"
#include "pointer-group-interface.hh"
#include "lily-guile.hh"
#include "real.hh"
#include "stencil.hh"
#include "string-convert.hh"
#include "skyline.hh"
#include "skyline-pair.hh"
using namespace std;

Real QUANTIZATION_UNIT = 0.2;
typedef Drul_array<Offset> Line;

void create_path_cap (vector<Box> &boxes, vector<Drul_array<Offset> > &buildings, PangoMatrix trans, Offset pt, Real rad, Real slope, Direction d);

struct Transform_matrix_and_expression {
  PangoMatrix tm_;
  SCM expr_;

  Transform_matrix_and_expression (PangoMatrix tm, SCM expr);
};


Transform_matrix_and_expression::Transform_matrix_and_expression (PangoMatrix tm, SCM expr)
{
  tm_ = tm;
  expr_ = expr;
}

PangoMatrix
make_transform_matrix (Real p0, Real p1, Real p2, Real p3, Real p4, Real p5)
{
  PangoMatrix out;
  out.xx = p0;
  out.xy = p1;
  out.yx = p2;
  out.yy = p3;
  out.x0 = p4;
  out.y0 = p5;
  return out;
}

//// UTILITY FUNCTIONS

/*
  map x's placement between orig_l and orig_r onto
  the interval final_l final_r
*/
Real
linear_map (Real final_l, Real final_r, Real orig_l, Real orig_r, Real x)
{
  return final_l + ((final_r - final_l) * ((x - orig_l) / (orig_r - orig_l)));
}

/*
  from a nested SCM list, return the first list of numbers
  useful for polygons
*/
SCM
get_number_list (SCM l)
{
  if (scm_is_pair (l))
    {
      if (scm_is_number (scm_car (l)))
        return l;
      SCM res = get_number_list (scm_car (l));
      if (res == SCM_BOOL_F)
        return get_number_list (scm_cdr (l));
      return res;
    }
  return SCM_BOOL_F;
}

/*
  from a nested SCM list, return the first list of numbers
  useful for paths
*/
SCM
get_path_list (SCM l)
{
  if (scm_is_pair (l))
    {
      if (scm_memv (scm_car (l),
                    scm_list_n (ly_symbol2scm ("moveto"),
                                ly_symbol2scm ("rmoveto"),
                                ly_symbol2scm ("lineto"),
                                ly_symbol2scm ("rlineto"),
                                ly_symbol2scm ("curveto"),
                                ly_symbol2scm ("rcurveto"),
                                ly_symbol2scm ("closepath"),
                                SCM_UNDEFINED))
          != SCM_BOOL_F)
        return l;
      SCM res = get_path_list (scm_car (l));
      if (res == SCM_BOOL_F)
        return get_path_list (scm_cdr (l));
      return res;
    }
  return SCM_BOOL_F;
}

Real
perpendicular_slope (Real s)
{
  if (s == 0.0)
    return infinity_f;
  if (s == infinity_f)
    return 0.0;
  return -1.0 / s;
}

// Returns a vector of lines that outline a thickened version
// of the original lines.
vector<Line>
thicken_and_transform_lines (vector<Line> const& lines, PangoMatrix const& trans, Real thick)
{
  return lines; // TODO
}

vector<Line>
outline_polyline (vector<Line> const& lines, PangoMatrix const& trans, Real thick, bool connect)
{
  // TODO
}

//// END UTILITY FUNCTIONS

/*
  below, for all of the functions make_X_boxes, the expression
  is always unpacked into variables.
  then, after a line of /////, there are manipulations of these variables
  (there may be no manipulations necessary depending on the function)
  afterwards, there is another ///// followed by the creation of points
  and boxes
*/

vector<Line>
outline_line (PangoMatrix const& trans, SCM expr)
{
  Real thick = robust_scm2double (scm_car (expr), 0.0);
  expr = scm_cdr (expr);
  Real x0 = robust_scm2double (scm_car (expr), 0.0);
  expr = scm_cdr (expr);
  Real y0 = robust_scm2double (scm_car (expr), 0.0);
  expr = scm_cdr (expr);
  Real x1 = robust_scm2double (scm_car (expr), 0.0);
  expr = scm_cdr (expr);
  Real y1 = robust_scm2double (scm_car (expr), 0.0);
  Real slope = x1 == x0 ? infinity_f : (y1 - y0) / (x1 - x0);

  Offset left (x0, y0);
  Offset right (x1, y1);

  vector<Offset> ret;
  ret.push_back (left);
  ret.push_back (right);
  return outline_polyline (ret, trans, thick, false);
}

vector<Line>
outline_partial_ellipse (PangoMatrix const& trans, SCM expr)
{
  Real x_rad = robust_scm2double (scm_car (expr), 0.0);
  expr = scm_cdr (expr);
  Real y_rad = robust_scm2double (scm_car (expr), 0.0);
  expr = scm_cdr (expr);
  Real start = robust_scm2double (scm_car (expr), 0.0);
  expr = scm_cdr (expr);
  Real end = robust_scm2double (scm_car (expr), 0.0);
  expr = scm_cdr (expr);
  Real thick = robust_scm2double (scm_car (expr), 0.0);
  expr = scm_cdr (expr);
  bool connect = to_boolean (scm_car (expr));
  expr = scm_cdr (expr);
  bool fill = to_boolean (scm_car (expr));

  Real start_radians = start * M_PI / 180;
  Real end_radians = end * M_PI / 180;
  vector<Offset> points;
  int quantization = max (1, (int) (((x_rad * trans.xx) + (y_rad * trans.yy)) * M_PI / QUANTIZATION_UNIT));
  for (vsize i = 0; i < 1 + quantization; i++)
    {
      Real angle = linear_map (start_radians, end_radians, 0, quantization, i);
      points.push_back (Offset (x_rad * cos (angle), y_rad + sin (angle)));
    }

  return outline_polyline (points, trans, thick, connect || fill || abs (start - end) == 360);
}

vector<Line>
outline_round_filled_box (PangoMatrix const& trans, SCM expr)
{
  Real left = robust_scm2double (scm_car (expr), 0.0);
  expr = scm_cdr (expr);
  Real right = robust_scm2double (scm_car (expr), 0.0);
  expr = scm_cdr (expr);
  Real bottom = robust_scm2double (scm_car (expr), 0.0);
  expr = scm_cdr (expr);
  Real top = robust_scm2double (scm_car (expr), 0.0);
  expr = scm_cdr (expr);
  Real thick = robust_scm2double (scm_car (expr), 0.0);

  vector<Offset> points;
  points.push_back (-left, -bottom);
  points.push_back (right, -bottom);
  points.push_back (right, top);
  points.push_back (-left, top);
  return outline_polyline (points, trans, thick, true);
}

// TODO: put this in outline_polyline
#if 0
void
create_path_cap (vector<Box> &boxes, vector<Drul_array<Offset> > &buildings, PangoMatrix trans, Offset pt, Real rad, Real slope, Direction d)
{
  Real angle = atan (slope) * 180 / M_PI;
  Real other = angle > 180 ? angle - 180 : angle + 180;
  if (angle < other)
    {
      Real holder = other;
      other = angle;
      angle = holder;
    }
  other = (slope >= 0 && d == DOWN) || (slope < 0 && d == UP)
          ? other + 360.0
          : other;
  PangoMatrix new_trans (trans);
  pango_matrix_translate (&new_trans, pt[X_AXIS], pt[Y_AXIS]);
  make_partial_ellipse_boxes (boxes, buildings, new_trans,
                                     scm_list_n (scm_from_double (rad),
                                                 scm_from_double (rad),
                                                 scm_from_double (angle),
                                                 scm_from_double (other),
                                                 scm_from_double (0.0),
                                                 SCM_BOOL_F,
                                                 SCM_BOOL_F,
                                                 SCM_UNDEFINED));
}
#endif

void
outline_bezier (PangoMatrix const& trans, SCM expr)
{
  Real thick = robust_scm2double (scm_car (expr), 0.0);
  expr = scm_cdr (expr);
  Real x0 = robust_scm2double (scm_car (expr), 0.0);
  expr = scm_cdr (expr);
  Real y0 = robust_scm2double (scm_car (expr), 0.0);
  expr = scm_cdr (expr);
  Real x1 = robust_scm2double (scm_car (expr), 0.0);
  expr = scm_cdr (expr);
  Real y1 = robust_scm2double (scm_car (expr), 0.0);
  expr = scm_cdr (expr);
  Real x2 = robust_scm2double (scm_car (expr), 0.0);
  expr = scm_cdr (expr);
  Real y2 = robust_scm2double (scm_car (expr), 0.0);
  expr = scm_cdr (expr);
  Real x3 = robust_scm2double (scm_car (expr), 0.0);
  expr = scm_cdr (expr);
  Real y3 = robust_scm2double (scm_car (expr), 0.0);

  Bezier curve;
  curve.control_[0] = Offset (x0, y0);
  curve.control_[1] = Offset (x1, y1);
  curve.control_[2] = Offset (x2, y2);
  curve.control_[3] = Offset (x3, y3);
  Offset p0 (x0, y0);
  Offset p1 (x1, y1);
  Offset p2 (x2, y2);
  Offset p3 (x3, y3);

  vector<Offset> points;
  int quantization = int (((p1 - p0).length ()
                           + (p2 - p1).length ()
                           + (p3 - p2).length ())
                          / QUANTIZATION_UNIT * max (trans.xx, trans.yy));
  for (vsize i = 0; i <= quantization; i++)
    {
      Real t = (double) i / quantization;
      points.push_back (curve.point (t));
    }

  return outline_polyline (points, trans, thick, false);
}

/*
  converts a path into lists of 4 (line) or 8 (curve) absolute coordinates
  for example:
  '(moveto 1 2 lineto 3 4 rlineto -1 -1 curveto 3 3 5 5 6 6 rcurveto -1 -1 -1 -1 -1 -1 closepath)
  becomes
  '((1 2 3 4)
    (3 4 2 3)
    (2 3 3 3 5 5 6 6)
    (6 6 5 5 4 4 3 3)
    (3 3 1 2))
*/

SCM
absolute_path (SCM expr)
{
  SCM out = SCM_EOL;
  Offset start (0, 0);
  Offset current (0, 0);
  bool first = true;
  while (scm_is_pair (expr))
    {
      if (scm_car (expr) == ly_symbol2scm ("moveto")
          || (scm_car (expr) == ly_symbol2scm ("rmoveto") && first))
        {
          Real x = robust_scm2double (scm_cadr (expr), 0.0);
          Real y = robust_scm2double (scm_caddr (expr), 0.0);
          start = Offset (x, y);
          current = start;
          expr = scm_cdddr (expr);
        }
      if (scm_car (expr) == ly_symbol2scm ("rmoveto"))
        {
          Real x = robust_scm2double (scm_cadr (expr), 0.0);
          Real y = robust_scm2double (scm_caddr (expr), 0.0);
          start = (Offset (x, y) + current);
          current = start;
          expr = scm_cdddr (expr);
        }
      else if (scm_car (expr) == ly_symbol2scm ("lineto"))
        {
          Real x = robust_scm2double (scm_cadr (expr), 0.0);
          Real y = robust_scm2double (scm_caddr (expr), 0.0);
          out = scm_cons (scm_list_4 (scm_from_double (current[X_AXIS]),
                                      scm_from_double (current[Y_AXIS]),
                                      scm_from_double (x),
                                      scm_from_double (y)),
                          out);
          current = Offset (x, y);
          expr = scm_cdddr (expr);
        }
      else if (scm_car (expr) == ly_symbol2scm ("rlineto"))
        {
          Real x = robust_scm2double (scm_cadr (expr), 0.0);
          Real y = robust_scm2double (scm_caddr (expr), 0.0);
          out = scm_cons (scm_list_4 (scm_from_double (current[X_AXIS]),
                                      scm_from_double (current[Y_AXIS]),
                                      scm_from_double (x + current[X_AXIS]),
                                      scm_from_double (y + current[Y_AXIS])),
                          out);
          current = (Offset (x, y) + current);
          expr = scm_cdddr (expr);
        }
      else if (scm_car (expr) == ly_symbol2scm ("curveto"))
        {
          Real x1 = robust_scm2double (scm_cadr (expr), 0.0);
          expr = scm_cddr (expr);
          Real y1 = robust_scm2double (scm_car (expr), 0.0);
          expr = scm_cdr (expr);
          Real x2 = robust_scm2double (scm_car (expr), 0.0);
          expr = scm_cdr (expr);
          Real y2 = robust_scm2double (scm_car (expr), 0.0);
          expr = scm_cdr (expr);
          Real x3 = robust_scm2double (scm_car (expr), 0.0);
          expr = scm_cdr (expr);
          Real y3 = robust_scm2double (scm_car (expr), 0.0);
          expr = scm_cdr (expr);
          out = scm_cons (scm_list_n (scm_from_double (current[X_AXIS]),
                                      scm_from_double (current[Y_AXIS]),
                                      scm_from_double (x1),
                                      scm_from_double (y1),
                                      scm_from_double (x2),
                                      scm_from_double (y2),
                                      scm_from_double (x3),
                                      scm_from_double (y3),
                                      SCM_UNDEFINED),
                            out);
          current = Offset (x3, y3);
        }
      else if (scm_car (expr) == ly_symbol2scm ("rcurveto"))
        {
          Real x1 = robust_scm2double (scm_cadr (expr), 0.0);
          expr = scm_cddr (expr);
          Real y1 = robust_scm2double (scm_car (expr), 0.0);
          expr = scm_cdr (expr);
          Real x2 = robust_scm2double (scm_car (expr), 0.0);
          expr = scm_cdr (expr);
          Real y2 = robust_scm2double (scm_car (expr), 0.0);
          expr = scm_cdr (expr);
          Real x3 = robust_scm2double (scm_car (expr), 0.0);
          expr = scm_cdr (expr);
          Real y3 = robust_scm2double (scm_car (expr), 0.0);
          expr = scm_cdr (expr);
          out = scm_cons (scm_list_n (scm_from_double (current[X_AXIS]),
                                      scm_from_double (current[Y_AXIS]),
                                      scm_from_double (x1 + current[X_AXIS]),
                                      scm_from_double (y1 + current[Y_AXIS]),
                                      scm_from_double (x2 + current[X_AXIS]),
                                      scm_from_double (y2 + current[Y_AXIS]),
                                      scm_from_double (x3 + current[X_AXIS]),
                                      scm_from_double (y3 + current[Y_AXIS]),
                                      SCM_UNDEFINED),
                          out);
          current = (Offset (x3, y3) + current);
        }
      else if (scm_car (expr) == ly_symbol2scm ("closepath"))
        {
          if ((current[X_AXIS] != start[X_AXIS]) || (current[Y_AXIS] != start[Y_AXIS]))
            {
              out = scm_cons (scm_list_4 (scm_from_double (current[X_AXIS]),
                                          scm_from_double (current[Y_AXIS]),
                                          scm_from_double (start[X_AXIS]),
                                          scm_from_double (start[Y_AXIS])),
                              out);
              current = start;
            }
          expr = scm_cdr (expr);
        }
      else
        {
          warning ("Malformed path for path stencil.");
          return out;
        }
      first = false;
    }
  return scm_reverse_x (out, SCM_EOL);
}

vector<Line>
outline_path (PangoMatrix const& trans, SCM expr)
{
  SCM thick = scm_car (expr);
  expr = scm_cdr (expr);
  SCM path = absolute_path (expr);
  // note that expr has more stuff that we don't need after this - simply ignore it

  vector<Line> ret;
  for (SCM s = path; scm_is_pair (s); s = scm_cdr (s))
    {
      vector<Line> part;
      if (scm_to_int (scm_length (scm_car (s))) == 4)
        part = outline_line (trans, scm_cons (blot, scm_car (s)));
      else
        part = outline_bezier (trans, scm_cons (blot, scm_car (s)));
      ret.insert (ret.end (), part.begin (), part.end ());
    }

  return ret;
}

vector<Line>
outline_polygon (PangoMatrix const& trans, SCM expr)
{
  // FIXME: probably don't need get_number_list...
  SCM coords = get_number_list (scm_car (expr));
  expr = scm_cdr (expr);
  SCM thick = scm_car (expr);

  vector<Offset> points;
  for (SCM s = coords; scm_is_pair (s); s = scm_cddr (s))
    {
      Real x = robust_scm2double (scm_car (s), 0.0);
      Real y = robust_scm2double (scm_cadr (s), 0.0);
      points.push_back (Offset (x, y));
    }

  return outline_polyline (points, trans, thick, true);
}

void
make_named_glyph_boxes (vector<Box> &boxes, vector<Drul_array<Offset> > &buildings, PangoMatrix trans, SCM expr)
{
  SCM fm_scm = scm_car (expr);
  Font_metric *fm = unsmob_metrics (fm_scm);
  expr = scm_cdr (expr);
  SCM glyph = scm_car (expr);
  string glyph_s = ly_scm2string (glyph);

  Open_type_font *open_fm =
    dynamic_cast<Open_type_font *>
      (dynamic_cast<Modified_font_metric *>(fm)->original_font ());
  SCM_ASSERT_TYPE (open_fm, fm_scm, SCM_ARG1, __FUNCTION__, "OpenType font");

  size_t gidx = open_fm->name_to_index (glyph_s);
  Box bbox = open_fm->get_unscaled_indexed_char_dimensions (gidx);
  SCM outline = open_fm->get_glyph_outline (gidx);
  Box real_bbox = fm->get_indexed_char_dimensions (gidx);

  /*
    Because extents for named glyphs are cached, the value of
    real_bbox may not be the one that freetype calculates.

    They should be close, though.
    A workaround below is to use the max of the two, which may
    slightly overshoot an extent but generally doesn't.
  */
  Real xlen = real_bbox[X_AXIS].length () / bbox[X_AXIS].length ();
  Real ylen = real_bbox[Y_AXIS].length () / bbox[Y_AXIS].length ();
  assert (abs (xlen - ylen) < 10e-3);

  pango_matrix_scale (&trans, max (xlen, ylen), max (xlen, ylen));

  vector<Line> ret;
  for (SCM s = outline; scm_is_pair (s); s = scm_cdr (s))
    {
      vector<Line> part;
      SCM expr = scm_cons (scm_from_double (0), scm_car (s));
      if (scm_to_int (scm_length (scm_car (s))) == 4)
        part = outline_line (trans, expr);
      else
        part = outline_bezier (trans, expr);

      ret.insert (ret.end (), part.begin (), part.end ());
    }

  return ret;
}

vector<Line>
make_glyph_string_boxes (PangoMatrix const& trans, SCM expr)
{
  Font_metric *fm = unsmob_metrics (scm_car (fm_scm));
  expr = scm_cdr (expr);
  expr = scm_cdr (expr); // font-name
  expr = scm_cdr (expr); // size
  expr = scm_cdr (expr); // cid?
  SCM whxy = scm_cadar (expr);

  vector<Real> widths;
  vector<Interval> heights;
  vector<Real> x_offsets;
  vector<Real> y_offsets;
  vector<string> char_ids;
  Pango_font *pango_fm = dynamic_cast<Pango_font *> (fm);
  SCM_ASSERT_TYPE (pango_fm, fm_scm, SCM_ARG1, __FUNCTION__, "Pango font");

  for (SCM s = whxy; scm_is_pair (s); s = scm_cdr (s))
    {
      SCM now = scm_car (s);
      widths.push_back (robust_scm2double (scm_car (now), 0.0));
      now = scm_cdr (now);
      heights.push_back (robust_scm2interval (scm_car (now), Interval (0,0)));
      now = scm_cdr (now);
      x_offsets.push_back (robust_scm2double (scm_car (now), 0.0));
      now = scm_cdr (now);
      y_offsets.push_back (robust_scm2double (scm_car (now), 0.0));
      now = scm_cdr (now);
      char_ids.push_back (robust_scm2string (scm_car (now), ""));
    }
  Real cumulative_x = 0.0;
  for (vsize i = 0; i < widths.size (); i++)
    {
      PangoMatrix transcopy (trans);
      Offset pt0 (cumulative_x + xos[i], heights[i][DOWN] + yos[i]);
      Offset pt1 (cumulative_x + widths[i] + xos[i], heights[i][UP] + yos[i]);
      cumulative_x += widths[i];

      Box kerned_bbox;
      kerned_bbox.add_point (pt0);
      kerned_bbox.add_point (pt1);
      size_t gidx = pango_fm->name_to_index (char_ids[i]);
      Box real_bbox = pango_fm->get_scaled_indexed_char_dimensions (gidx);
      Box bbox = pango_fm->get_unscaled_indexed_char_dimensions (gidx);
      SCM outline = pango_fm->get_glyph_outline (gidx);

      // scales may have rounding error but should be close
      Real xlen = real_bbox[X_AXIS].length () / bbox[X_AXIS].length ();
      Real ylen = real_bbox[Y_AXIS].length () / bbox[Y_AXIS].length ();

      /*
        TODO:

        The value will be nan for whitespace, in which case we just want
        filler, so the kerned bbox is ok.

        However, if the value is inf, this likely means that LilyPond is
        using a font that is currently difficult to get the measurements
        from the Pango_font.  This should eventually be fixed.  The solution
        for now is just to use the bounding box.
      */
      if (isnan (xlen) || isnan (ylen) || isinf (xlen) || isinf (ylen))
        outline = box_to_scheme_lines (kerned_bbox);
      else
        {
          assert (abs (xlen - ylen) < 10e-3);

          Real scale_factor = max (xlen, ylen);
          // the three operations below move the stencil from its original coordinates to current coordinates
          pango_matrix_translate (&transcopy, kerned_bbox[X_AXIS][LEFT], kerned_bbox[Y_AXIS][DOWN] - real_bbox[Y_AXIS][DOWN]);
          pango_matrix_translate (&transcopy, real_bbox[X_AXIS][LEFT], real_bbox[Y_AXIS][DOWN]);
          pango_matrix_scale (&transcopy, scale_factor, scale_factor);
          pango_matrix_translate (&transcopy, -bbox[X_AXIS][LEFT], -bbox[Y_AXIS][DOWN]);
        }
      //////////////////////
      for (SCM s = outline;
           scm_is_pair (s);
           s = scm_cdr (s))
        {
          scm_to_int (scm_length (scm_car (s))) == 4
                      ? make_draw_line_boxes (boxes, buildings, transcopy, scm_cons (scm_from_double (0), scm_car (s)), false)
                      : make_draw_bezier_boxes (boxes, buildings, transcopy, scm_cons (scm_from_double (0), scm_car (s)));
        }
    }
}


// Returns a vector of straight lines that approximates the outline
// of the given stencil.
vector<Line>
outline_simple_stencil (PangoMatrix trans, SCM expr)
{
  if (not scm_is_pair (expr))
    return;
  if (scm_car (expr) == ly_symbol2scm ("draw-line"))
    return outline_line (trans, scm_cdr (expr));
  else if (scm_car (expr) == ly_symbol2scm ("dashed-line"))
    {
      expr = scm_cdr (expr);
      SCM thickness = scm_car (expr);
      expr = scm_cdr (expr);
      expr = scm_cdr (expr); // on
      expr = scm_cdr (expr); // off
      SCM x1 = scm_car (expr);
      expr = scm_cdr (expr);
      SCM x2 = scm_car (expr);
      return outline_line (trans, scm_list_5 (thickness,
                                              scm_from_double (0.0),
                                              scm_from_double (0.0),
                                              x1, x2));
    }
  else if (scm_car (expr) == ly_symbol2scm ("circle"))
    {
      expr = scm_cdr (expr);
      SCM radius = scm_car (expr);
      expr = scm_cdr (expr);
      SCM thickness = scm_car (expr);
      return outline_partial_ellipse (trans,
                                      scm_list_n (radius,
                                                  radius,
                                                  scm_from_double (0.0),
                                                  scm_from_double (360.0),
                                                  thickness,
                                                  SCM_BOOL_F,
                                                  SCM_BOOL_T,
                                                  SCM_UNDEFINED));
    }
  else if (scm_car (expr) == ly_symbol2scm ("ellipse"))
    {
      expr = scm_cdr (expr);
      SCM x_radius = scm_car (expr);
      expr = scm_cdr (expr);
      SCM y_radius = scm_car (expr);
      expr = scm_cdr (expr);
      SCM thickness = scm_car (expr);
      return outline_partial_ellipse (trans,
                                      scm_list_n (x_radius,
                                                  y_radius,
                                                  scm_from_double (0.0),
                                                  scm_from_double (360.0),
                                                  thickness,
                                                  SCM_BOOL_F,
                                                  SCM_BOOL_T,
                                                  SCM_UNDEFINED));
    }
  else if (scm_car (expr) == ly_symbol2scm ("partial-ellipse"))
    return outline_partial_ellipse (trans, scm_cdr (expr));
  else if (scm_car (expr) == ly_symbol2scm ("round-filled-box"))
    return outline_round_filled_box (trans, scm_cdr (expr));
  else if (scm_car (expr) == ly_symbol2scm ("named-glyph"))
    return outline_named_glyph (trans, scm_cdr (expr));
  else if (scm_car (expr) == ly_symbol2scm ("polygon"))
    return outline_polygon (trans, scm_cdr (expr));
  else if (scm_car (expr) == ly_symbol2scm ("path"))
    return outline_path (trans, scm_cdr (expr));
  else if (scm_car (expr) == ly_symbol2scm ("glyph-string"))
    return outline_glyph_string (trans, scm_cdr (expr));
  else
    {
      programming_error ("Stencil expression not supported by the vertical skylines.");
    }
}

/*
  traverses a stencil expression, returning a vector of Transform_matrix_and_expression
  the struct Transform_matrix_and_expression contains two members,
  a Transform_matrix that indicates where to move a stencil and the stencil expression
  to show how to construct the stencil
*/
vector<Transform_matrix_and_expression>
stencil_traverser (PangoMatrix trans, SCM expr)
{
  if (scm_is_null (expr))
    return vector<Transform_matrix_and_expression> ();
  else if (expr == ly_string2scm (""))
    return vector<Transform_matrix_and_expression> ();
  else if (scm_car (expr) == ly_symbol2scm ("combine-stencil"))
    {
      vector<Transform_matrix_and_expression> out;
      for (SCM s = scm_cdr (expr); scm_is_pair (s); s = scm_cdr (s))
        {
          vector<Transform_matrix_and_expression> res = stencil_traverser (trans, scm_car (s));
          out.insert (out.end (), res.begin (), res.end ());
        }
      return out;
    }
  else if (scm_car (expr) == ly_symbol2scm ("footnote"))
    return vector<Transform_matrix_and_expression> ();
  else if (scm_car (expr) == ly_symbol2scm ("translate-stencil"))
    {
      Real x = robust_scm2double (scm_caadr (expr), 0.0);
      Real y = robust_scm2double (scm_cdadr (expr), 0.0);
      pango_matrix_translate (&trans, x, y);
      return stencil_traverser (trans, scm_caddr (expr));
    }
  else if (scm_car (expr) == ly_symbol2scm ("scale-stencil"))
    {
      Real x = robust_scm2double (scm_caadr (expr), 0.0);
      Real y = robust_scm2double (scm_cadadr (expr), 0.0);
      pango_matrix_scale (&trans, x, y);
      return stencil_traverser (trans, scm_caddr (expr));
    }
  else if (scm_car (expr) == ly_symbol2scm ("rotate-stencil"))
    {
      Real ang = robust_scm2double (scm_caadr (expr), 0.0);
      Real x = robust_scm2double (scm_car (scm_cadadr (expr)), 0.0);
      Real y = robust_scm2double (scm_cdr (scm_cadadr (expr)), 0.0);
      pango_matrix_translate (&trans, x, y);
      pango_matrix_rotate (&trans, -ang);
      pango_matrix_translate (&trans, -x, -y);
      return stencil_traverser (trans, scm_caddr (expr));
    }
  else if (scm_car (expr) == ly_symbol2scm ("delay-stencil-evaluation"))
    return stencil_traverser (trans, scm_force (scm_cadr (expr)));
  else if (scm_car (expr) == ly_symbol2scm ("grob-cause"))
    return stencil_traverser (trans, scm_caddr (expr));
  else if (scm_car (expr) == ly_symbol2scm ("color"))
    return stencil_traverser (trans, scm_caddr (expr));
  else if (scm_car (expr) == ly_symbol2scm ("id"))
    return stencil_traverser (trans, scm_caddr (expr));
  else
    {
      vector<Transform_matrix_and_expression> out;
      out.push_back (Transform_matrix_and_expression (trans, expr));
      return out;
    }
  warning ("Stencil expression not supported by the veritcal skylines.");
  return vector<Transform_matrix_and_expression> ();
}

SCM
Grob::internal_simple_skylines_from_stencil (SCM smob, Axis a)
{
  Grob *me = unsmob_grob (smob);

  if (to_boolean (me->get_property ("cross-staff")))
    return Skyline_pair ().smobbed_copy ();

  extract_grob_set (me, "elements", elts);
  if (elts.size ())
    return internal_skylines_from_element_stencils (smob, a);

  Stencil *s = unsmob_stencil (me->get_property ("stencil"));
  if (!s)
    return Skyline_pair ().smobbed_copy();

  vector<Box> boxes;
  boxes.push_back (Box (s->extent (X_AXIS), s->extent (Y_AXIS)));
  return Skyline_pair (boxes, a).smobbed_copy ();
}

MAKE_SCHEME_CALLBACK (Grob, simple_vertical_skylines_from_stencil, 1);
SCM
Grob::simple_vertical_skylines_from_stencil (SCM smob)
{
  return internal_simple_skylines_from_stencil (smob, X_AXIS);
}

MAKE_SCHEME_CALLBACK (Grob, simple_horizontal_skylines_from_stencil, 1);
SCM
Grob::simple_horizontal_skylines_from_stencil (SCM smob)
{
  return internal_simple_skylines_from_stencil (smob, Y_AXIS);
}

SCM
Stencil::skylines_from_stencil (SCM sten, Real pad, Axis a)
{
  Stencil *s = unsmob_stencil (sten);
  if (!s)
    return Skyline_pair ().smobbed_copy ();

  vector<Transform_matrix_and_expression> data =
    stencil_traverser (make_transform_matrix (1.0,0.0,0.0,1.0,0.0,0.0),
                       s->expr ());
  vector<Box> boxes;
  vector<Drul_array<Offset> > buildings;
  for (vsize i = 0; i < data.size (); i++)
    stencil_dispatcher (boxes, buildings, data[i].tm_, data[i].expr_);

  // we use the bounding box if there are no boxes
  if (!boxes.size () && !buildings.size ())
    boxes.push_back (Box (s->extent (X_AXIS), s->extent (Y_AXIS)));

  Skyline_pair out (boxes, a);
  out.merge (Skyline_pair (buildings, a));

  for (DOWN_and_UP (d))
    out[d] = out[d].padded (pad);

  out.deholify ();
  return out.smobbed_copy ();
}

MAKE_SCHEME_CALLBACK (Grob, vertical_skylines_from_stencil, 1);
SCM
Grob::vertical_skylines_from_stencil (SCM smob)
{
  Grob *me = unsmob_grob (smob);

  Real pad = robust_scm2double (me->get_property ("skyline-horizontal-padding"), 0.0);
  SCM out = Stencil::skylines_from_stencil (me->get_property ("stencil"), pad, X_AXIS);

  return out;
}

MAKE_SCHEME_CALLBACK (Grob, horizontal_skylines_from_stencil, 1);
SCM
Grob::horizontal_skylines_from_stencil (SCM smob)
{
  Grob *me = unsmob_grob (smob);

  Real pad = robust_scm2double (me->get_property ("skyline-vertical-padding"), 0.0);
  SCM out = Stencil::skylines_from_stencil (me->get_property ("stencil"), pad, Y_AXIS);

  return out;
}

SCM
Grob::internal_skylines_from_element_stencils (SCM smob, Axis a)
{
  Grob *me = unsmob_grob (smob);

  extract_grob_set (me, "elements", elts);
  vector<Real> x_pos;
  vector<Real> y_pos;
  Grob *x_common = common_refpoint_of_array (elts, me, X_AXIS);
  Grob *y_common = common_refpoint_of_array (elts, me, Y_AXIS);
  for (vsize i = 0; i < elts.size (); i++)
    {
      x_pos.push_back (elts[i]->relative_coordinate (x_common, X_AXIS));
      y_pos.push_back (elts[i]->relative_coordinate (y_common, Y_AXIS));
    }
  Real my_x = me->relative_coordinate (x_common, X_AXIS);
  Real my_y = me->relative_coordinate (y_common, Y_AXIS);
  Skyline_pair res;
  for (vsize i = 0; i < elts.size (); i++)
    {
      Skyline_pair *skyp = Skyline_pair::unsmob (elts[i]->get_property (a == X_AXIS ? "vertical-skylines" : "horizontal-skylines"));
      if (skyp)
        {
          /*
            Here, copying is essential.  Otherwise, the skyline pair will
            get doubly shifted!
          */
          /*
            It took Mike about 6 months of his life to add the `else' clause
            below.  For horizontal skylines, the raise and shift calls need
            to be reversed.  This is what was causing the problems in the
            shifting with all of the tests. RIP 6 months!
          */
          Skyline_pair copy = Skyline_pair (*skyp);
          if (a == X_AXIS)
            {
              copy.shift (x_pos[i] - my_x);
              copy.raise (y_pos[i] - my_y);
            }
          else
            {
              copy.raise (x_pos[i] - my_x);
              copy.shift (y_pos[i] - my_y);
            }
          res.merge (copy);
        }
    }
  return res.smobbed_copy ();
}

MAKE_SCHEME_CALLBACK (Grob, vertical_skylines_from_element_stencils, 1);
SCM
Grob::vertical_skylines_from_element_stencils (SCM smob)
{
  return internal_skylines_from_element_stencils (smob, X_AXIS);
}

MAKE_SCHEME_CALLBACK (Grob, horizontal_skylines_from_element_stencils, 1);
SCM
Grob::horizontal_skylines_from_element_stencils (SCM smob)
{
  return internal_skylines_from_element_stencils (smob, Y_AXIS);
}
