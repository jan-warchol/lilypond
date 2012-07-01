\version "2.15.41"
#(ly:expect-warning (_ "system with empty extent"))
#(ly:expect-warning (_ "didn't find a vertical alignment in this system"))

\header{
  texidoc = "
A score with @code{skipTypesetting} set for the whole score
will not segfault.
"
}


{
  \set Score.skipTypesetting = ##t
  c'4
}

