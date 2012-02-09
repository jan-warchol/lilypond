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

#include <complex>
#include "box.hh"
#include "bezier.hh"
#include "font-metric.hh"
#include "grob.hh"
#include "interval.hh"
#include "offset.hh"
#include "pointer-group-interface.hh"
#include "lily-guile.hh"
#include "real.hh"
#include "stencil.hh"
#include "string-convert.hh"
#include "skyline-pair.hh"
using namespace std;

Real CURVE_QUANTIZATION =  10;

Real ELLIPSE_QUANTIZATION = 20;


struct Translate_scale_rotate {
  Offset translate_;
  Offset scale_;
  Real rotate_;
};

Translate_scale_rotate
make_translate_scale_rotate (Offset translate, Offset scale, Real rotate)
{
  Translate_scale_rotate out;
  out.translate_ = translate;
  out.scale_ = scale;
  out.rotate_ = rotate;
  return out;
}

struct Transform_matrix {
  Real p0_;
  Real p1_;
  Real p2_;
  Real p3_;
  Real p4_;
  Real p5_;

  static Transform_matrix multiply_matrix (Transform_matrix x, Transform_matrix y);
  static Offset multiply_point (Transform_matrix x, Offset p);
  static Translate_scale_rotate to_translate_scale_rotate (Transform_matrix m);
};

struct Transform_matrix_and_expression {
  Transform_matrix tm_;
  SCM expr_;

  Transform_matrix_and_expression (Transform_matrix tm, SCM expr);
};


Transform_matrix_and_expression::Transform_matrix_and_expression (Transform_matrix tm, SCM expr)
{
  tm_ = tm;
  expr_ = expr;
}

Transform_matrix
make_transform_matrix (Real p0, Real p1, Real p2, Real p3, Real p4, Real p5)
{
  Transform_matrix out;
  out.p0_ = p0;
  out.p1_ = p1;
  out.p2_ = p2;
  out.p3_ = p3;
  out.p4_ = p4;
  out.p5_ = p5;
  return out;
}

/*
   Multiply transform matrices x and y to get a new transform
   matrix.  Equivalent to 3x3 matrix multiplication when the
   matrix is expanded using the convention explained above.
*/

Transform_matrix
Transform_matrix::multiply_matrix (Transform_matrix x, Transform_matrix y)
{
  return make_transform_matrix  ((x.p0_ * y.p0_) + (x.p2_ * y.p1_),
                                 (x.p1_ * y.p0_) + (x.p3_ * y.p1_),
                                 (x.p0_ * y.p2_) + (x.p2_ * y.p3_),
                                 (x.p1_ * y.p2_) + (x.p3_ * y.p3_),
                                 (x.p0_ * y.p4_) + (x.p2_ * y.p5_) + x.p4_,
                                 (x.p1_ * y.p4_) + (x.p3_ * y.p5_) + x.p5_);
}

/*
   Multiply transform matrix x and point p to get a new point.
*/
Offset
Transform_matrix::multiply_point (Transform_matrix x, Offset p)
{
  return Offset ((x.p0_ * p[X_AXIS]) + (x.p2_ * p[Y_AXIS]) + x.p4_,
                 (x.p1_ * p[X_AXIS]) + (x.p3_ * p[Y_AXIS]) + x.p5_);
}

/*
   Turns a transform matrix into a translate, scale, and rotate.
   The order is important (rotate first, then scale, then translate).
*/

Translate_scale_rotate
Transform_matrix::to_translate_scale_rotate (Transform_matrix x)
{
  Real rotate = atan2 (-x.p2_, x.p0_);
  bool use_sin = cos (rotate) == 0.0;

  return make_translate_scale_rotate (Offset (x.p4_, x.p5_),
                                      Offset (use_sin ? -x.p2_ / sin (rotate) : x.p0_ / cos (rotate),
                                              use_sin ? x.p1_ / sin (rotate) : x.p3_ / cos (rotate)),
                                      rotate);
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

//// END UTILITY FUNCTIONS

/*
  below, for all of the functions make_X_boxes, the expression
  is always unpacked into variables.
  then, after a line of /////, there are manipulations of these variables
  (there may be no manipulations necessary depending on the function)
  afterwards, there is another ///// followed by the creation of points
  and boxes
*/

vector<Box>
make_draw_line_boxes (Transform_matrix trans, SCM expr)
{
  vector<Box> boxes;
  Real thick = robust_scm2double (scm_car (expr), 0.0);
  expr = scm_cdr (expr);
  Real x0 = robust_scm2double (scm_car (expr), 0.0);
  expr = scm_cdr (expr);
  Real y0 = robust_scm2double (scm_car (expr), 0.0);
  expr = scm_cdr (expr);
  Real x1 = robust_scm2double (scm_car (expr), 0.0);
  expr = scm_cdr (expr);
  Real y1 = robust_scm2double (scm_car (expr), 0.0);
  //////////////////////
  vector<Offset> points;
  for (vsize i = 0; i < 1 + CURVE_QUANTIZATION; i++)
    {
      Offset pt (linear_map (x0, x1, 0, CURVE_QUANTIZATION, i),
                 linear_map (y0, y1, 0, CURVE_QUANTIZATION, i));
      points.push_back (Transform_matrix::multiply_point (trans, pt));
    }
  Translate_scale_rotate tsr = Transform_matrix::to_translate_scale_rotate (trans);
  for (vsize i = 0; i < CURVE_QUANTIZATION; i++)
    {
      Box b;
      b.add_point (points[i]);
      b.add_point (points[i + 1]);
      b.widen (abs (thick * tsr.scale_[X_AXIS]) / 2, abs (thick * tsr.scale_[Y_AXIS]) / 2);
      boxes.push_back (b);
    }
  return boxes;
}

vector<Box>
make_partial_ellipse_boxes (Transform_matrix trans, SCM expr)
{
  vector<Box> boxes;
  Real x_rad = robust_scm2double (scm_car (expr), 0.0);
  expr = scm_cdr (expr);
  Real y_rad = robust_scm2double (scm_car (expr), 0.0);
  expr = scm_cdr (expr);
  Real start = robust_scm2double (scm_car (expr), 0.0);
  expr = scm_cdr (expr);
  Real end = robust_scm2double (scm_car (expr), 0.0);
  expr = scm_cdr (expr);
  Real th = robust_scm2double (scm_car (expr), 0.0);
  expr = scm_cdr (expr);
  bool connect = to_boolean (scm_car (expr));
  expr = scm_cdr (expr);
  bool fill = to_boolean (scm_car (expr));
  //////////////////////
  start = M_PI * start / 180;
  end = M_PI * end / 180;
  if (end == start)
    end += (2 * M_PI);
  complex<Real> sunit = polar (1.0, start);
  complex<Real> eunit = polar (1.0, end);
  Offset sp (real (sunit) * x_rad, imag (sunit) * y_rad);
  Offset ep (real (eunit) * x_rad, imag (eunit) * y_rad);
  //////////////////////
  vector<Offset> points;
  for (vsize i = 0; i < 1 + CURVE_QUANTIZATION; i++)
    {
      Real ang = linear_map (start, end, 0, ELLIPSE_QUANTIZATION, i);
      complex<Real> coord = polar (1.0, ang);
      Offset pt (real (coord) * x_rad,
                 imag (coord) * y_rad);
      points.push_back (Transform_matrix::multiply_point (trans, pt));
    }
  Translate_scale_rotate tsr = Transform_matrix::to_translate_scale_rotate (trans);
  for (vsize i = 0; i < CURVE_QUANTIZATION; i++)
    {
      Box b;
      b.add_point (points[i]);
      b.add_point (points[i + 1]);
      b.widen (abs (th * tsr.scale_[X_AXIS]) / 2, abs (th * tsr.scale_[Y_AXIS]) / 2);
      boxes.push_back (b);
    }
  if (connect || fill)
    {
      vector<Box> db = make_draw_line_boxes (trans, scm_list_5(scm_from_double (th),
                                                               scm_from_double (sp[X_AXIS]),
                                                               scm_from_double (sp[Y_AXIS]),
                                                               scm_from_double (ep[X_AXIS]),
                                                               scm_from_double (ep[Y_AXIS])));
      boxes.insert (boxes.end (), db.begin (), db.end ());
    }
  return boxes;
}

vector<Box>
make_round_filled_box_boxes (Transform_matrix trans, SCM expr)
{
  vector<Box> boxes;
  Real left = robust_scm2double (scm_car (expr), 0.0);
  expr = scm_cdr (expr);
  Real right = robust_scm2double (scm_car (expr), 0.0);
  expr = scm_cdr (expr);
  Real bottom = robust_scm2double (scm_car (expr), 0.0);
  expr = scm_cdr (expr);
  Real top = robust_scm2double (scm_car (expr), 0.0);
  expr = scm_cdr (expr);
  Real th = robust_scm2double (scm_car (expr), 0.0);
  //////////////////////
  vector<Offset> points;
  Box b;
  b.add_point (Transform_matrix::multiply_point (trans, Offset (-left, -bottom)));
  b.add_point (Transform_matrix::multiply_point (trans, Offset (right, top)));
  Translate_scale_rotate tsr = Transform_matrix::to_translate_scale_rotate (trans);
  b.widen (abs (th * tsr.scale_[X_AXIS]) / 2, abs (th * tsr.scale_[Y_AXIS]) / 2);
  boxes.push_back (b);
  return boxes;
}


vector<Box>
make_draw_bezier_boxes (Transform_matrix trans, SCM expr)
{
  vector<Box> boxes;
  Real th = robust_scm2double (scm_car (expr), 0.0);
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
  //////////////////////
  Bezier curve;
  curve.control_[0] = Offset (x0, y0);
  curve.control_[1] = Offset (x1, y1);
  curve.control_[2] = Offset (x2, y2);
  curve.control_[3] = Offset (x3, y3);
  //////////////////////
  vector<Offset> points;
  points.push_back (Transform_matrix::multiply_point (trans, curve.control_[0]));
  for (vsize i = 1; i < CURVE_QUANTIZATION; i++)
    points.push_back (Transform_matrix::multiply_point (trans, curve.curve_point ((i * 1.0) / CURVE_QUANTIZATION)));
  points.push_back (Transform_matrix::multiply_point (trans, curve.control_[3]));
  Translate_scale_rotate tsr = Transform_matrix::to_translate_scale_rotate (trans);
  for (vsize i = 0; i < CURVE_QUANTIZATION; i++)
    {
      Box b;
      b.add_point (points[i]);
      b.add_point (points[i + 1]);
      b.widen (abs (th * tsr.scale_[X_AXIS]) / 2, abs (th * tsr.scale_[Y_AXIS]) / 2);
      boxes.push_back (b);
    }
  return boxes;
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
all_commands_to_absolute_and_group (SCM expr)
{
  SCM out = SCM_EOL;
  SCM *tail = &out;
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
          *tail = scm_cons (scm_list_4 (scm_from_double (current[X_AXIS]),
                                        scm_from_double (current[Y_AXIS]),
                                        scm_from_double (x),
                                        scm_from_double (y)),
                            SCM_EOL);
          tail = SCM_CDRLOC (*tail);
          current = Offset (x, y);
          expr = scm_cdddr (expr);
        }
      else if (scm_car (expr) == ly_symbol2scm ("rlineto"))
        {
          Real x = robust_scm2double (scm_cadr (expr), 0.0);
          Real y = robust_scm2double (scm_caddr (expr), 0.0);
          *tail = scm_cons (scm_list_4 (scm_from_double (current[X_AXIS]),
                                        scm_from_double (current[Y_AXIS]),
                                        scm_from_double (x + current[X_AXIS]),
                                        scm_from_double (y + current[Y_AXIS])),
                            SCM_EOL);
          tail = SCM_CDRLOC (*tail);
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
          *tail = scm_cons (scm_list_n (scm_from_double (current[X_AXIS]),
                                        scm_from_double (current[Y_AXIS]),
                                        scm_from_double (x1),
                                        scm_from_double (y1),
                                        scm_from_double (x2),
                                        scm_from_double (y2),
                                        scm_from_double (x3),
                                        scm_from_double (y3),
                                        SCM_UNDEFINED),
                            SCM_EOL);
          tail = SCM_CDRLOC (*tail);
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
          *tail = scm_cons (scm_list_n (scm_from_double (current[X_AXIS]),
                                        scm_from_double (current[Y_AXIS]),
                                        scm_from_double (x1 + current[X_AXIS]),
                                        scm_from_double (y1 + current[Y_AXIS]),
                                        scm_from_double (x2 + current[X_AXIS]),
                                        scm_from_double (y2 + current[Y_AXIS]),
                                        scm_from_double (x3 + current[X_AXIS]),
                                        scm_from_double (y3 + current[Y_AXIS]),
                                        SCM_UNDEFINED),
                            SCM_EOL);
          tail = SCM_CDRLOC (*tail);
          current = (Offset (x3, y3) + current);
        }
      else if (scm_car (expr) == ly_symbol2scm ("closepath"))
        {
          if ((current[X_AXIS] != start[X_AXIS]) || (current[Y_AXIS] != start[Y_AXIS]))
            {
              *tail = scm_cons (scm_list_4 (scm_from_double (current[X_AXIS]),
                                            scm_from_double (current[Y_AXIS]),
                                            scm_from_double (start[X_AXIS]),
                                            scm_from_double (start[Y_AXIS])),
                                SCM_EOL);
              tail = SCM_CDRLOC (*tail);
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
  return out;
}

vector<Box>
make_path_boxes (Transform_matrix trans, SCM expr)
{
  vector<Box> boxes;
  SCM blot = scm_car (expr);
  expr = scm_cdr (expr);
  SCM path = all_commands_to_absolute_and_group (get_path_list (scm_car (expr)));
  // note that expr has more stuff that we don't need after this - simply ignore it
  //////////////////////
  for (SCM s = path; scm_is_pair (s); s = scm_cdr (s))
    {
      vector<Box> bxs = scm_to_int (scm_length (scm_car (s))) == 4
                        ? make_draw_line_boxes (trans, scm_cons (blot, scm_car (s)))
                        : make_draw_bezier_boxes (trans, scm_cons (blot, scm_car (s)));
      boxes.insert (boxes.end (), bxs.begin (), bxs.end ());
    }
  return boxes;
}

vector<Box>
make_polygon_boxes (Transform_matrix trans, SCM expr)
{
  vector<Box> boxes;
  SCM coords = get_number_list (scm_car (expr));
  expr = scm_cdr (expr);
  SCM blot_diameter = scm_car (expr);
  //////////////////////
  bool first = true;
  SCM l = SCM_EOL;
  SCM *tail = &l;
  for (SCM s = coords; scm_is_pair (s); s = scm_cddr (s))
    {
      *tail = scm_cons (first ? ly_symbol2scm ("moveto") : ly_symbol2scm ("lineto"), SCM_EOL);
      tail = SCM_CDRLOC (*tail);
      *tail = scm_cons (scm_car (s), SCM_EOL);
      tail = SCM_CDRLOC (*tail);
      *tail = scm_cons (scm_cadr (s), SCM_EOL);
      tail = SCM_CDRLOC (*tail);
      first = false;
    }
  *tail = scm_cons (ly_symbol2scm ("closepath"), SCM_EOL);
  tail = SCM_CDRLOC (*tail);
  return make_path_boxes (trans, scm_cons (blot_diameter, l));
}

vector<Box>
make_named_glyph_boxes (Transform_matrix trans, SCM expr)
{
  vector<Box> boxes;
  Font_metric *fm = unsmob_metrics (scm_car (expr));
  expr = scm_cdr (expr);
  SCM glyph = scm_car (expr);
  //////////////////////
  string font_name = String_convert::to_lower (fm->font_name ());
  SCM box_table = ly_lily_module_constant ("box-hash");
  SCM font_list = scm_hashq_ref (box_table, ly_symbol2scm (font_name.c_str ()), SCM_EOL);
  SCM glyph_info = scm_hashq_ref (font_list, scm_string_to_symbol (glyph), SCM_EOL);
  Stencil m = fm->find_by_name (ly_scm2string (glyph));
  // mmx and mmy give the bounding box for the original stencil
  Interval mmx = robust_scm2interval (ly_assoc_get (ly_symbol2scm ("mmx"), glyph_info, SCM_EOL), Interval (0,0));
  Interval mmy = robust_scm2interval (ly_assoc_get (ly_symbol2scm ("mmy"), glyph_info, SCM_EOL), Interval (0,0));
  // xex and yex give the bounding box for the current stencil
  Interval xex = m.extent (X_AXIS);
  Interval yex = m.extent (Y_AXIS);
  Offset scale (xex.length () / mmx.length (), yex.length () / mmy.length ());
  // the three operations below move the stencil from its original coordinates to current coordinates
  trans = Transform_matrix::multiply_matrix (trans, make_transform_matrix (1.0, 0.0, 0.0, 1.0, xex[LEFT], yex[DOWN]));
  trans = Transform_matrix::multiply_matrix (trans, make_transform_matrix (scale[X_AXIS], 0.0, 0.0, scale[Y_AXIS], 0.0, 0.0));
  trans = Transform_matrix::multiply_matrix (trans, make_transform_matrix (1.0, 0.0, 0.0, 1.0, -mmx[LEFT], -mmy[DOWN]));
  //////////////////////
  for (SCM s = ly_assoc_get (ly_symbol2scm ("paths"), glyph_info, SCM_EOL);
       scm_is_pair (s);
       s = scm_cdr (s))
    {
      vector<Box> bxs = scm_to_int (scm_length (scm_car (s))) == 4
                        ? make_draw_line_boxes (trans, scm_cons (scm_from_double (0), scm_car (s)))
                        : make_draw_bezier_boxes (trans, scm_cons (scm_from_double (0), scm_car (s)));
      boxes.insert (boxes.end (), bxs.begin (), bxs.end ());
    }
  return boxes;
}

vector<Box>
make_glyph_string_boxes (Transform_matrix trans, SCM expr)
{
  vector<Box> boxes;
  expr = scm_cdr (expr); // font-name
  expr = scm_cdr (expr); // size
  expr = scm_cdr (expr); // cid?
  SCM whxy = scm_cadar (expr);
  vector<Real> widths;
  vector<Interval> heights;
  vector<Real> xos;
  vector<Real> yos;
  //////////////////////
  for (SCM s = whxy; scm_is_pair (s); s = scm_cdr (s))
    {
      SCM now = scm_car (s);
      widths.push_back (robust_scm2double (scm_car (now), 0.0));
      now = scm_cdr (now);
      heights.push_back (robust_scm2interval (scm_car (now), Interval (0,0)));
      now = scm_cdr (now);
      xos.push_back (robust_scm2double (scm_car (now), 0.0));
      now = scm_cdr (now);
      yos.push_back (robust_scm2double (scm_car (now), 0.0));
    }
  Real cumulative_x = 0.0;
  for (vsize i = 0; i < widths.size (); i++)
    {
      Offset pt0 (cumulative_x + xos[i], heights[i][DOWN] + yos[i]);
      Offset pt1 (cumulative_x + widths[i] + xos[i], heights[i][UP] + yos[i]);
      Box b;
      b.add_point (Transform_matrix::multiply_point (trans, pt0));
      b.add_point (Transform_matrix::multiply_point (trans, pt1));
      boxes.push_back (b);
      cumulative_x += widths[i];
    }
  return boxes;
}

/*
  receives a stencil expression and a transform matrix
  depending on the stencil name, dispatches it to the appropriate function
*/

vector<Box>
stencil_dispatcher (Transform_matrix trans, SCM expr)
{
  if (not scm_is_pair (expr))
    return vector<Box> ();
  if (scm_car (expr) == ly_symbol2scm ("draw-line"))
    return make_draw_line_boxes (trans, scm_cdr (expr));
  else if (scm_car (expr) == ly_symbol2scm ("dashed-line"))
    {
      expr = scm_cdr (expr);
      SCM th = scm_car (expr);
      expr = scm_cdr (expr);
      expr = scm_cdr (expr); // on
      expr = scm_cdr (expr); // off
      SCM x1 = scm_car (expr);
      expr = scm_cdr (expr);
      SCM x2 = scm_car (expr);
      return make_draw_line_boxes (trans, scm_list_5 (th, scm_from_double (0.0), scm_from_double (0.0), x1, x2));
    }
  else if (scm_car (expr) == ly_symbol2scm ("circle"))
    {
      expr = scm_cdr (expr);
      SCM rad = scm_car (expr);
      expr = scm_cdr (expr);
      SCM th = scm_car (expr);
      return make_partial_ellipse_boxes (trans,
                                         scm_list_n (rad,
                                                     rad,
                                                     scm_from_double (0.0),
                                                     scm_from_double (360.0),
                                                     th,
                                                     SCM_BOOL_F,
                                                     SCM_BOOL_T,
                                                     SCM_UNDEFINED));
    }
  else if (scm_car (expr) == ly_symbol2scm ("ellipse"))
    {
      expr = scm_cdr (expr);
      SCM x_rad = scm_car (expr);
      expr = scm_cdr (expr);
      SCM y_rad = scm_car (expr);
      expr = scm_cdr (expr);
      SCM th = scm_car (expr);
      return make_partial_ellipse_boxes (trans,
                                         scm_list_n (x_rad,
                                                     y_rad,
                                                     scm_from_double (0.0),
                                                     scm_from_double (360.0),
                                                     th,
                                                     SCM_BOOL_F,
                                                     SCM_BOOL_T,
                                                     SCM_UNDEFINED));
    }
  else if (scm_car (expr) == ly_symbol2scm ("partial-ellipse"))
    return make_partial_ellipse_boxes (trans, scm_cdr (expr));
  else if (scm_car (expr) == ly_symbol2scm ("round-filled-box"))
    return make_round_filled_box_boxes (trans, scm_cdr (expr));
  else if (scm_car (expr) == ly_symbol2scm ("named-glyph"))
    return make_named_glyph_boxes (trans, scm_cdr (expr));
  else if (scm_car (expr) == ly_symbol2scm ("polygon"))
    return make_polygon_boxes (trans, scm_cdr (expr));
  else if (scm_car (expr) == ly_symbol2scm ("path"))
    return make_path_boxes (trans, scm_cdr (expr));
  else if (scm_car (expr) == ly_symbol2scm ("glyph-string"))
    return make_glyph_string_boxes (trans, scm_cdr (expr));
  else
    {
      warning ("Stencil expression not supported by the veritcal skylines.");
      return vector<Box> ();
    }
}

/*
  traverses a stencil expression, returning a vector of Transform_matrix_and_expression
  the struct Transform_matrix_and_expression contains two members,
  a Transform_matrix that indicates where to move a stencil and the stencil expression
  to show how to construct the stencil
*/
vector<Transform_matrix_and_expression>
stencil_traverser (Transform_matrix trans, SCM expr)
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
      trans = Transform_matrix::multiply_matrix (trans, make_transform_matrix (1.0,0.0,0.0,1.0,x,y));
      return stencil_traverser (trans, scm_caddr (expr));
    }
  else if (scm_car (expr) == ly_symbol2scm ("scale-stencil"))
    {
      Real x = robust_scm2double (scm_caadr (expr), 0.0);
      Real y = robust_scm2double (scm_cadadr (expr), 0.0);
      trans = Transform_matrix::multiply_matrix (trans, make_transform_matrix (x,0.0,0.0,y,0.0,0.0));
      return stencil_traverser (trans, scm_caddr (expr));
    }
  else if (scm_car (expr) == ly_symbol2scm ("rotate-stencil"))
    {
      Real ang = robust_scm2double (scm_caadr (expr), 0.0);
      Real x = robust_scm2double (scm_car (scm_cadadr (expr)), 0.0);
      Real y = robust_scm2double (scm_cdr (scm_cadadr (expr)), 0.0);
      trans = Transform_matrix::multiply_matrix (trans, make_transform_matrix (1.0,0.0,0.0,1.0,x,y));
      trans = Transform_matrix::multiply_matrix (trans, make_transform_matrix (cos (ang),sin (ang),-sin (ang),cos (ang),0.0,0.0));
      trans = Transform_matrix::multiply_matrix (trans, make_transform_matrix (1.0,0.0,0.0,1.0,-x,-y));
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

MAKE_SCHEME_CALLBACK (Grob, vertical_skylines_from_stencil, 1);
SCM
Grob::vertical_skylines_from_stencil (SCM smob)
{
  Grob *me = unsmob_grob (smob);
  Stencil *s = unsmob_stencil (me->get_property ("stencil"));
  if (!s)
    return Skyline_pair ().smobbed_copy ();
  vector<Transform_matrix_and_expression> data =
    stencil_traverser (make_transform_matrix (1.0,0.0,0.0,1.0,0.0,0.0),
                       s->expr ());
  vector<Box> boxes;
  for (vsize i = 0; i < data.size (); i++)
    {
      vector<Box> bxs = stencil_dispatcher (data[i].tm_, data[i].expr_);
      boxes.insert (boxes.end (), bxs.begin (), bxs.end ());
    }
  return Skyline_pair (boxes, 0.0, X_AXIS).smobbed_copy ();
}

MAKE_SCHEME_CALLBACK (Grob, vertical_skylines_from_element_stencils, 1);
SCM
Grob::vertical_skylines_from_element_stencils (SCM smob)
{
  Grob *me = unsmob_grob (smob);
  extract_grob_set (me, "elements", elts);
  Grob *refp = common_refpoint_of_array (elts, me, X_AXIS);
  vector<Real> x_pos;
  bool first = true;
  Real diff = 0.0;
  for (vsize i = 0; i < elts.size (); i++)
    {
      x_pos.push_back (elts[i]->relative_coordinate (refp, X_AXIS));
      if (first)
        diff = x_pos[0];
      first = false;
      x_pos[i] -= diff;
    }
  Skyline_pair res;
  for (vsize i = 0; i < elts.size (); i++)
    {
      Skyline_pair *skyp = Skyline_pair::unsmob (elts[i]->get_property ("vertical-skylines"));
      if (skyp)
        {
          skyp->shift (x_pos[i]);
          res.merge (*skyp);
        }
    }
  return res.smobbed_copy ();
}
