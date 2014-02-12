% INSPIRATIONAL HEADER FOR LILYPOND DOCUMENTATION fretted-strings %
% Passage from Johann Kaspar Mertz "Opern Revue, Op. 8, no. 17"   %
% on melodies from Bellini's "Norma"                              %
%*****************************************************************%

\version "2.17.30"

\header {
  lsrtags = "headword"
  texidoc = ""
  doctitle = "headword"
}


\layout {
  \context {
    \Score
    \remove "Bar_number_engraver"
  }
}

%%%% shortcuts
% fingering orientations
sfol = \set fingeringOrientations = #'(left)
sfor = \set fingeringOrientations = #'(right)
sfod = \set fingeringOrientations = #'(down)
sfou = \set fingeringOrientations = #'(up)

% string number orientations
ssnol = \set stringNumberOrientations = #'(left)  %(down right up)
ssnou = \set stringNumberOrientations = #'(up)
ssnod = \set stringNumberOrientations = #'(down)
ssnor = \set stringNumberOrientations = #'(right)

% define fingering offset
FO = #(define-music-function (parser location offsetX offsetY) (number? number?)
#{
  \once \override Voice.Fingering.extra-offset = #(cons offsetX offsetY)
#})

% markups
rit = \markup \center-align { \bold { \italic { " rit." } } }
dimin = \markup \center-align { \italic { " dim." } }
andantino = \markup \left-align { \italic { \bold { \fontsize #2.5 { "Andantino" } } } }
benmarcato = \markup { \italic { \bold { "il canto ben marcato" } } }
pdolce = #(make-dynamic-script (markup #:line (#:dynamic "p" #:normal-text #:italic "dol.")))

%%% THE MUSIC %%%

melody = \relative c {
  \clef "treble_8"
  \key d \major
  \time 4/4
  \voiceOne
  \sfol
  e,32 a' c e
  e, a c e
  e,, a' c e
  e, a c e
  f4\rest <e'-4>4-> | % m. 1

  e,,,32 gis' b e
  e, gis b e
  e,, gis' b e
  e, gis b e
  f4\rest \FO #'0.4 #'0.5 <gis-1 e'-4>4 | % m. 2

  d4\rest <b e>-> d4\rest^\rit <b e>4-> | % m. 3
  <gis b e>1 | % m. 4

  \bar "||"
  \key a \minor
  R1 % m. 5

  e'4^\benmarcato e8. d16-4
  d4-4 \tuplet 3/2 { \sfou \FO #'-0.3 #'0.6 <c-2>4 b8 } | % end of m. 6

  \FO #'-0.3 #'0.3
  <a-3>4 \tuplet 3/2 { c4 b8 } a4 e'8. e16 | % m. 7

  \FO #'-0.3 #'0.3
  <g-4>4 \tuplet 3/2 { \sfol \FO #'0.3 #'0.0 <f-1>4 e8 } e4  % beg of m. 8
  \tuplet 3/2 { \sfou <d-4>4 c8 } | % end of m. 8

  b4 \tuplet 3/2 { d4-4 c8 } \sfou \FO #'-1.7 #'-1.5 <b-0>4 e | % end of m. 9

  e4 e8. d16-4 d4 \tuplet 3/2 { c4 b8 } | % m. 10

  \tuplet 3/2 { a4 a8 b4 c8 } % beg of m. 11
  \sfou \FO #'-0.3 #'0.3
  <d-4>4^\< \tuplet 3/2 { e4 <d f>8\! } | % end of m. 11
}

bass = \relative c {
  \key d \major
  \time 4/4
  \voiceTwo

  e,8\fp[ e'] e,[ e'] e, \sfol <c''-1> <a'-2> c, | % m. 1

  e,,8\fp[ e'] e,[ e'] e, \sfod \FO #'0.2 #'-0.2 <b''-1>  % beg m. 2
  \sfol \FO #'0.3 #'0.0 <e-1> b | % end m. 2

  e,,8 e' gis e e, e' gis_\dimin e | % m. 3

  e,1 | % m. 4

  %% new section starts here in A minor
  \set Score.beamExceptions = #'()
  \once \override TextScript.staff-padding = #1.7
  \tuplet 3/2 { a8\p^\andantino e' a c a e a, e' a c a e } | % m. 5

  \tuplet 3/2 { a,8\pdolce e' a c a e } % beg m. 6
  \tuplet 3/2 { e,8 \sfou <e'-3> <gis-1> c gis e } | % end m. 6

  \tuplet 3/2 { a,8 <e'-2> a c e, b' a, e' a c a e } | % m. 7

  \tuplet 3/2 { f,8 f' a \sfol \FO #'0.3 #'-0.5 <d-4> a f fis, d' a' d a d, } | % m. 8

  \tuplet 3/2 { <g,-3>8 d' g d' g, d % beg m. 9
    \sfod \FO #'0.0 #'-2.0 <gis,-4> \sfou <e'-2> <gis-1> b gis e } | % end m. 9

  \tuplet 3/2 { a,8 e' a c a e e, e' gis c gis e } | % m. 10

  \tuplet 3/2 { a,8 e' a b a e f, f' a d a f } | % m. 11
}

\score {
  \new Staff = "guitar" <<
    \context Voice = "upper" { \melody }
    \context Voice = "lower" { \bass }
  >>
  \layout {
    \context {
      \Score
      \override Fingering.staff-padding = #'()
      \omit TupletNumber
      \override TupletBracket.bracket-visibility = ##f
    }
  }
  \midi { }
}
