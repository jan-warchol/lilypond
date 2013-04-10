#include "grob.hh"
#include "simple-closure.hh"
#include "unpure-pure-container.hh"

SCM
axis_offset_symbol (Axis a)
{
  return a == X_AXIS
         ? ly_symbol2scm ("X-offset")
         : ly_symbol2scm ("Y-offset");
}

SCM
axis_parent_positioning (Axis a)
{
  return (a == X_AXIS)
         ? Grob::x_parent_positioning_proc
         : Grob::y_parent_positioning_proc;
}

/*
  replace

  (orig-proc GROB)

  by

  (PROC GROB (orig-proc GROB))
*/
void
chain_callback (Grob *g, SCM proc, SCM sym)
{
  SCM data = g->get_property_data (sym);

  if (ly_is_procedure (data) || is_unpure_pure_container (data))
    data = ly_make_simple_closure (scm_list_1 (data));
  else if (is_simple_closure (data))
    data = simple_closure_expression (data);
  else
    /*
      Data may be nonnumber. In that case, it is assumed to be
      undefined.
    */

    data = SCM_UNDEFINED;

  SCM expr = scm_list_2 (proc, data);
  g->set_property (sym,

                   // twice: one as a wrapper for grob property routines,
                   // once for the actual delayed binding.
                   ly_make_simple_closure (ly_make_simple_closure (expr)));
}

void
chain_offset_callback (Grob *g, SCM proc, Axis a)
{
  chain_callback (g, proc, axis_offset_symbol (a));
}
