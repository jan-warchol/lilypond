\version "2.13.10"

\header {
  lsrtags = "expressive-marks, editorial-annotations"
  texidoc = "
Any graphical object can be printed over a white backgound to
mask parts of objects that lie beneath.  This can be useful to
improve the appearance of collisions in complex situations when
repositioning objects is impractical.  It is necessary to explicitly
set the @code{layer} property to control which objects are masked
by the white background.

In this example the collision of the tie with the time signature is
improved by masking out the part of the tie that crosses the time
signature by setting the @code{whiteout} property of @code{TimeSignature}.
To do this @code{TimeSignature} is moved to a layer above @code{Tie}, which
is left in the default layer of 1, and @code{StaffSymbol} is moved to a
layer above @code{TimeSignature} so it is not masked.
"
  doctitle = "Using the whiteout property"
}

{
  \override Score.StaffSymbol #'layer = #4
  \override Staff.TimeSignature #'layer = #3
  b'2 b'~
  \once \override Staff.TimeSignature #'whiteout = ##t
  \time 3/4
  b' r4
}