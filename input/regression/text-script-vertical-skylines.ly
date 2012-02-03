\version "2.15.28"

\header {
  texidoc = "@code{TextScript} vertical skylines should allow for vertical
kerning.
"
}

\relative c' {
  a^\markup { \filled-box #'(0 . 2) #'(0 . 20) #0 hello}
  a^\markup { world }
}