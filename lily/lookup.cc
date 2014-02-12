/*
  This file is part of LilyPond, the GNU music typesetter.

  Copyright (C) 1997--2014 Han-Wen Nienhuys <hanwen@xs4all.nl>

  Jan Nieuwenhuizen <janneke@gnu.org>

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

#include "lookup.hh"

#include <cmath>
#include <cctype>
using namespace std;

#include "line-interface.hh"
#include "warn.hh"
#include "international.hh"
#include "dimensions.hh"
#include "bezier.hh"
#include "file-path.hh"
#include "main.hh"
#include "lily-guile.hh"

Stencil
Lookup::beam (Real slope, Real width, Real thick, Real blot)
{
  Box b;

  Offset p;

  p = Offset (0, thick / 2);
  b.add_point (p);
  p += Offset (1, -1) * (blot / 2);

  SCM points = SCM_EOL;

  points = scm_cons (scm_from_double (p[X_AXIS]),
                     scm_cons (scm_from_double (p[Y_AXIS]),
                               points));

  p = Offset (0, -thick / 2);
  b.add_point (p);
  p += Offset (1, 1) * (blot / 2);

  points = scm_cons (scm_from_double (p[X_AXIS]),
                     scm_cons (scm_from_double (p[Y_AXIS]),
                               points));

  p = Offset (width, width * slope - thick / 2);
  b.add_point (p);
  p += Offset (-1, 1) * (blot / 2);

  points = scm_cons (scm_from_double (p[X_AXIS]),
                     scm_cons (scm_from_double (p[Y_AXIS]),
                               points));

  p = Offset (width, width * slope + thick / 2);
  b.add_point (p);
  p += Offset (-1, -1) * (blot / 2);

  points = scm_cons (scm_from_double (p[X_AXIS]),
                     scm_cons (scm_from_double (p[Y_AXIS]),
                               points));

  SCM expr = scm_list_n (ly_symbol2scm ("polygon"),
                         ly_quote_scm (points),
                         scm_from_double (blot),
                         SCM_BOOL_T,
                         SCM_UNDEFINED);

  return Stencil (b, expr);
}

Stencil
Lookup::rotated_box (Real slope, Real width, Real thick, Real blot)
{
  vector<Offset> pts;
  Offset rot (1, slope);

  thick -= 2 * blot;
  width -= 2 * blot;
  rot /= sqrt (1 + slope * slope);
  pts.push_back (Offset (0, -thick / 2) * rot);
  pts.push_back (Offset (width, -thick / 2) * rot);
  pts.push_back (Offset (width, thick / 2) * rot);
  pts.push_back (Offset (0, thick / 2) * rot);
  return Lookup::round_filled_polygon (pts, blot);
}

Stencil
Lookup::horizontal_line (Interval w, Real th)
{
  SCM at = scm_list_n (ly_symbol2scm ("draw-line"),
                       scm_from_double (th),
                       scm_from_double (w[LEFT]),
                       scm_from_double (0),
                       scm_from_double (w[RIGHT]),
                       scm_from_double (0),
                       SCM_UNDEFINED);

  Box box;
  box[X_AXIS] = w;
  box[Y_AXIS] = Interval (-th / 2, th / 2);

  return Stencil (box, at);
}

Stencil
Lookup::blank (Box b)
{
  return Stencil (b, scm_from_locale_string (""));
}

Stencil
Lookup::circle (Real rad, Real thick, bool filled)
{
  Box b (Interval (-rad, rad), Interval (-rad, rad));
  return Stencil (b, scm_list_4 (ly_symbol2scm ("circle"),
                                 scm_from_double (rad),
                                 scm_from_double (thick),
                                 scm_from_bool (filled)));
}

Stencil
Lookup::filled_box (Box b)
{
  return round_filled_box (b, 0.0);
}

/*
 * round filled box:
 *
 *   __________________________________
 *  /     \  ^           /     \      ^
 * |         |blot              |     |
 * |       | |dia       |       |     |
 * |         |meter             |     |
 * |\ _ _ /  v           \ _ _ /|     |
 * |                            |     |
 * |                            |     | Box
 * |                    <------>|     | extent
 * |                      blot  |     | (Y_AXIS)
 * |                    diameter|     |
 * |                            |     |
 * |  _ _                  _ _  |     |
 * |/     \              /     \|     |
 * |                            |     |
 * |       |            |       |     |
 * |                            |     |
 * x\_____/______________\_____/|_____v
 * |(0, 0)                       |
 * |                            |
 * |                            |
 * |<-------------------------->|
 *       Box extent (X_AXIS)
 */
Stencil
Lookup::round_filled_box (Box b, Real blotdiameter)
{
  Real width = b.x ().delta ();
  blotdiameter = min (blotdiameter, width);
  Real height = b.y ().delta ();
  blotdiameter = min (blotdiameter, height);

  if (blotdiameter < 0.0)
    {
      if (!isinf (blotdiameter))
        warning (_f ("Not drawing a box with negative dimension, %.2f by %.2f.",
                     width, height));
      return Stencil (b, SCM_EOL);
    }

  SCM at = (scm_list_n (ly_symbol2scm ("round-filled-box"),
                        scm_from_double (-b[X_AXIS][LEFT]),
                        scm_from_double (b[X_AXIS][RIGHT]),
                        scm_from_double (-b[Y_AXIS][DOWN]),
                        scm_from_double (b[Y_AXIS][UP]),
                        scm_from_double (blotdiameter),
                        SCM_UNDEFINED));

  return Stencil (b, at);
}

/*
 * Create Stencil that represents a filled polygon with round edges.
 *
 * LIMITATIONS:
 *
 * (a) Only outer (convex) edges are rounded.
 *
 * (b) This algorithm works as expected only for polygons whose edges
 * do not intersect.  For example, the polygon ((0, 0), (q, 0), (0,
 * q), (q, q)) has an intersection at point (q/2, q/2) and therefore
 * will give a strange result.  Even non-adjacent edges that just
 * touch each other will in general not work as expected for non-null
 * blotdiameter.
 *
 * (c) Given a polygon ((x0, y0), (x1, y1), ... , (x (n-1), y (n-1))),
 * if there is a natural number k such that blotdiameter is greater
 * than the maximum of { | (x (k mod n), y (k mod n)) - (x ((k+1) mod n),
 * y ((k+1) mod n)) |, | (x (k mod n), y (k mod n)) - (x ((k+2) mod n),
 * y ((k+2) mod n)) |, | (x ((k+1) mod n), y ((k+1) mod n)) - (x ((k+2)
 * mod n), y ((k+2) mod n)) | }, then the outline of the rounded
 * polygon will exceed the outline of the core polygon.  In other
 * words: Do not draw rounded polygons that have a leg smaller or
 * thinner than blotdiameter (or set blotdiameter to a sufficiently
 * small value -- maybe even 0.0)!
 *
 * NOTE: Limitations (b) and (c) arise from the fact that round edges
 * are made by moulding sharp edges to round ones rather than adding
 * to a core filled polygon.  For details of these two different
 * approaches, see the thread upon the ledger lines patch that started
 * on March 25, 2002 on the devel mailing list.  The below version of
 * round_filled_polygon () sticks to the moulding model, which the
 * majority of the list participants finally voted for.  This,
 * however, results in the above limitations and a much increased
 * complexity of the algorithm, since it has to compute a shrinked
 * polygon -- which is not trivial define precisely and unambigously.
 * With the other approach, one simply could move a circle of size
 * blotdiameter along all edges of the polygon (which is what the
 * postscript routine in the backend effectively does, but on the
 * shrinked polygon). --jr
 */
Stencil
Lookup::round_filled_polygon (vector<Offset> const &points,
                              Real blotdiameter)
{
  /* TODO: Maybe print a warning if one of the above limitations
     applies to the given polygon.  However, this is quite complicated
     to check. */

  const Real epsilon = 0.01;

#ifndef NDEBUG
  /* remove consecutive duplicate points */
  for (vsize i = 0; i < points.size (); i++)
    {
      int next = (i + 1) % points.size ();
      Real d = (points[i] - points[next]).length ();
      if (d < epsilon)
        programming_error ("Polygon should not have duplicate points");
    }
#endif

  /* special cases: degenerated polygons */
  if (points.size () == 0)
    return Stencil ();
  if (points.size () == 1)
    {
      Stencil circ = circle (0.5 * blotdiameter, 0, true);
      circ.translate (points[0]);
      return circ;
    }
  if (points.size () == 2)
    return Line_interface::make_line (blotdiameter, points[0], points[1]);

  /* shrink polygon in size by 0.5 * blotdiameter */
  vector<Offset> shrunk_points;
  shrunk_points.resize (points.size ());
  bool ccw = 1; // true, if three adjacent points are counterclockwise ordered
  for (vsize i = 0; i < points.size (); i++)
    {
      int i0 = i;
      int i1 = (i + 1) % points.size ();
      int i2 = (i + 2) % points.size ();
      Offset p0 = points[i0];
      Offset p1 = points[i1];
      Offset p2 = points[i2];
      Offset p10 = p0 - p1;
      Offset p12 = p2 - p1;
      if (p10.length () != 0.0)
        {
          // recompute ccw
          Real phi = p10.arg ();
          // rotate (p2 - p0) by (-phi)
          Offset q = complex_multiply (p2 - p0, complex_exp (Offset (1.0, -phi)));

          if (q[Y_AXIS] > 0)
            ccw = 1;
          else if (q[Y_AXIS] < 0)
            ccw = 0;
          else {} // keep ccw unchanged
        }
      else {} // keep ccw unchanged
      Offset p10n = (1.0 / p10.length ()) * p10; // normalize length to 1.0
      Offset p12n = (1.0 / p12.length ()) * p12;
      Offset p13n = 0.5 * (p10n + p12n);
      Offset p14n = 0.5 * (p10n - p12n);
      Offset p13;
      Real d = p13n.length () * p14n.length (); // distance p3n to line (p1..p0)
      if (d < epsilon)
        // special case: p0, p1, p2 are on a single line => build
        // vector orthogonal to (p2-p0) of length 0.5 blotdiameter
        {
          p13[X_AXIS] = p10[Y_AXIS];
          p13[Y_AXIS] = -p10[X_AXIS];
          p13 = (0.5 * blotdiameter / p13.length ()) * p13;
        }
      else
        p13 = (0.5 * blotdiameter / d) * p13n;
      shrunk_points[i1] = p1 + ((ccw) ? p13 : -p13);
    }

  /* build scm expression and bounding box */
  SCM shrunk_points_scm = SCM_EOL;
  Box box;
  for (vsize i = 0; i < shrunk_points.size (); i++)
    {
      SCM x = scm_from_double (shrunk_points[i][X_AXIS]);
      SCM y = scm_from_double (shrunk_points[i][Y_AXIS]);
      shrunk_points_scm = scm_cons (x, scm_cons (y, shrunk_points_scm));
      box.add_point (points[i]);
    }
  SCM polygon_scm = scm_list_n (ly_symbol2scm ("polygon"),
                                ly_quote_scm (shrunk_points_scm),
                                scm_from_double (blotdiameter),
                                SCM_BOOL_T,
                                SCM_UNDEFINED);

  Stencil polygon = Stencil (box, polygon_scm);
  shrunk_points.clear ();
  return polygon;
}

/*
  TODO: deprecate?
*/
Stencil
Lookup::frame (Box b, Real thick, Real blot)
{
  Stencil m;
  for (Axis a = X_AXIS; a < NO_AXES; a = Axis (a + 1))
    {
      Axis o = Axis ((a + 1) % NO_AXES);
      for (LEFT_and_RIGHT (d))
        {
          Box edges;
          edges[a] = b[a][d] + 0.5 * thick * Interval (-1, 1);
          edges[o][DOWN] = b[o][DOWN] - thick / 2;
          edges[o][UP] = b[o][UP] + thick / 2;

          m.add_stencil (round_filled_box (edges, blot));
        }
    }
  return m;
}

/*
  Make a smooth curve along the points
*/
Stencil
Lookup::slur (Bezier curve, Real curvethick, Real linethick,
              SCM dash_details)
{
  Stencil return_value;

  /*
      calculate the offset for the two beziers that make the sandwich
      for the slur
  */
  Real alpha = (curve.control_[3] - curve.control_[0]).arg ();
  Bezier back = curve;
  Offset perp = curvethick * complex_exp (Offset (0, alpha + M_PI / 2)) * 0.5;
  back.control_[1] += perp;
  back.control_[2] += perp;

  curve.control_[1] -= perp;
  curve.control_[2] -= perp;

  if (!scm_is_pair (dash_details))
    {
      /* solid slur  */
      return_value = bezier_sandwich (back, curve, linethick);
    }
  else
    {
      /* dashed or combination slur */
      int num_segments = scm_to_int (scm_length (dash_details));
      for (int i = 0; i < num_segments; i++)
        {
          SCM dash_pattern = scm_list_ref (dash_details, scm_from_int (i));
          Real t_min = robust_scm2double (scm_car (dash_pattern), 0);
          Real t_max = robust_scm2double (scm_cadr (dash_pattern), 1.0);
          Real dash_fraction
            = robust_scm2double (scm_caddr (dash_pattern), 1.0);
          Real dash_period
            = robust_scm2double (scm_cadddr (dash_pattern), 0.75);
          Bezier back_segment = back.extract (t_min, t_max);
          Bezier curve_segment = curve.extract (t_min, t_max);
          if (dash_fraction == 1.0)
            return_value.add_stencil (bezier_sandwich (back_segment,
                                                       curve_segment,
                                                       linethick));
          else
            {
              Bezier back_dash, curve_dash;
              Real seg_length = (back_segment.control_[3]
                                 - back_segment.control_[0]).length ();
              int pattern_count = (int) (seg_length / dash_period);
              Real pattern_length = 1.0 / (pattern_count + dash_fraction);
              Real start_t, end_t;
              for (int p = 0; p <= pattern_count; p++)
                {
                  start_t = p * pattern_length;
                  end_t = (p + dash_fraction) * pattern_length;
                  back_dash
                    = back_segment.extract (start_t, end_t);
                  curve_dash
                    = curve_segment.extract (start_t, end_t);
                  return_value.add_stencil (bezier_sandwich (back_dash,
                                                             curve_dash,
                                                             linethick));
                }
            }
        }
    }
  return return_value;
}

/*
 * Bezier Sandwich:
 *
 *                               .|
 *                        .       |
 *              top .             |
 *              . curve           |
 *          .                     |
 *       .                        |
 *     .                          |
 *    |                           |
 *    |                          .|
 *    |                     .
 *    |         bottom .
 *    |            . curve
 *    |         .
 *    |      .
 *    |   .
 *    | .
 *    |.
 *    |
 *
 */
Stencil
Lookup::bezier_sandwich (Bezier top_curve, Bezier bottom_curve, Real thickness)
{
  SCM commands = scm_list_n (ly_symbol2scm ("moveto"),
                             scm_from_double (top_curve.control_[0][X_AXIS]),
                             scm_from_double (top_curve.control_[0][Y_AXIS]),
                             ly_symbol2scm ("curveto"),
                             scm_from_double (top_curve.control_[1][X_AXIS]),
                             scm_from_double (top_curve.control_[1][Y_AXIS]),
                             scm_from_double (top_curve.control_[2][X_AXIS]),
                             scm_from_double (top_curve.control_[2][Y_AXIS]),
                             scm_from_double (top_curve.control_[3][X_AXIS]),
                             scm_from_double (top_curve.control_[3][Y_AXIS]),
                             ly_symbol2scm ("curveto"),
                             scm_from_double (bottom_curve.control_[2][X_AXIS]),
                             scm_from_double (bottom_curve.control_[2][Y_AXIS]),
                             scm_from_double (bottom_curve.control_[1][X_AXIS]),
                             scm_from_double (bottom_curve.control_[1][Y_AXIS]),
                             scm_from_double (bottom_curve.control_[0][X_AXIS]),
                             scm_from_double (bottom_curve.control_[0][Y_AXIS]),
                             ly_symbol2scm ("closepath"),
                             SCM_UNDEFINED);

  SCM horizontal_bend = scm_list_n (ly_symbol2scm ("path"),
                                    scm_from_double (thickness),
                                    ly_quote_scm (commands),
                                    ly_quote_scm (ly_symbol2scm ("round")),
                                    ly_quote_scm (ly_symbol2scm ("round")),
                                    SCM_BOOL_T,
                                    SCM_UNDEFINED);

  Interval x_extent = top_curve.extent (X_AXIS);
  x_extent.unite (bottom_curve.extent (X_AXIS));
  Interval y_extent = top_curve.extent (Y_AXIS);
  y_extent.unite (bottom_curve.extent (Y_AXIS));
  Box b (x_extent, y_extent);

  b.widen (0.5 * thickness, 0.5 * thickness);
  return Stencil (b, horizontal_bend);
}

Stencil
Lookup::repeat_slash (Real w, Real s, Real t)
{

  Real x_width = sqrt ((t * t) + ((t / s) * (t / s)));
  Real height = w * s;

  SCM controls = scm_list_n (ly_symbol2scm ("moveto"),
                             scm_from_double (0),
                             scm_from_double (0),
                             ly_symbol2scm ("rlineto"),
                             scm_from_double (x_width),
                             scm_from_double (0),
                             ly_symbol2scm ("rlineto"),
                             scm_from_double (w),
                             scm_from_double (height),
                             ly_symbol2scm ("rlineto"),
                             scm_from_double (-x_width),
                             scm_from_double (0),
                             ly_symbol2scm ("closepath"),
                             SCM_UNDEFINED);

  SCM slashnodot = scm_list_n (ly_symbol2scm ("path"),
                               scm_from_double (0),
                               ly_quote_scm (controls),
                               ly_quote_scm (ly_symbol2scm ("round")),
                               ly_quote_scm (ly_symbol2scm ("round")),
                               SCM_BOOL_T,
                               SCM_UNDEFINED);

  Box b (Interval (0, w + sqrt (sqr (t / s) + sqr (t))),
         Interval (0, w * s));

  return Stencil (b, slashnodot); //  http://slashnodot.org
}

Stencil
Lookup::bracket (Axis a, Interval iv, Real thick, Real protrude, Real blot)
{
  Box b;
  Axis other = Axis ((a + 1) % 2);
  b[a] = iv;
  b[other] = Interval (-1, 1) * thick * 0.5;

  Stencil m = round_filled_box (b, blot);

  b[a] = Interval (iv[UP] - thick, iv[UP]);
  Interval oi = Interval (-thick / 2, thick / 2 + fabs (protrude));
  oi *= sign (protrude);
  b[other] = oi;
  m.add_stencil (round_filled_box (b, blot));
  b[a] = Interval (iv[DOWN], iv[DOWN] + thick);
  m.add_stencil (round_filled_box (b, blot));

  return m;
}

Stencil
Lookup::triangle (Interval iv, Real thick, Real protrude)
{
  Box b;
  b[X_AXIS] = Interval (0, iv.length ());
  b[Y_AXIS] = Interval (min (0., protrude), max (0.0, protrude));

  vector<Offset> points;
  points.push_back (Offset (iv[LEFT], 0));
  points.push_back (Offset (iv[RIGHT], 0));
  points.push_back (Offset (iv.center (), protrude));
  points.push_back (Offset (iv[LEFT], 0));  // close triangle

  return points_to_line_stencil (thick, points);

}

Stencil
Lookup::points_to_line_stencil (Real thick, vector<Offset> const &points)
{
  Stencil ret;
  for (vsize i = 1; i < points.size (); i++)
    {
      if (points[i - 1].is_sane () && points[i].is_sane ())
        {
          Stencil line
            = Line_interface::make_line (thick, points[i - 1], points[i]);
          ret.add_stencil (line);
        }
    }
  return ret;
}
