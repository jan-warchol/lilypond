\version "2.19.13"
#(set-global-staff-size 30)

\header {
  texidoc = "Objects like articulations, lyrics, dynamics etc. are
aligned correctly even when they aren't attached directly to notes.

Object's parent may be a @code{PaperColumn} (instead of a more usual
@code{NoteHead}) e.g. when a DynamicText is attached to a spacer rest,
or when a Lyrics context doesn't have an @code{associatedVoice}.
In that case, LilyPond should find noteheads belonging to this
@code{PaperColumn} and align the object on these noteheads.  If there
are no noteheads in the @code{PaperColumn}, the object should be aligned
using a placeholder extent (which should be equivalent to the extent of
the previous notehead), to ensure consistent spacing between objects
attached to PaperColumns and NoteHeads.

Note that the placeholder extent shouldn't be used if there are any
noteheads in respective @code{PaperColumn}, even if they have empty
stencils.

In the test cases below @code{PaperColumn} location has been marked
with a blue line, and the placeholder extent has been marked with a
double-headed arrow."
}

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%% stuff for displaying PaperColumns and placeholder extent

\layout {
  \context {
    \Voice
    \consists "Grid_point_engraver"
    % added twice so that even when there's one voice it will work
    \consists "Grid_point_engraver"
    gridInterval = #(ly:make-moment 1/4)
    \override GridPoint.Y-extent = #'(-1 . 3)
  }
  \context {
    \Staff
    \consists "Grid_line_span_engraver"
    \override GridLine.color = #'(0.2 0.4 1)
    \override GridLine.layer = #2
  }
}

#(define (draw-two-headed-arrow grob len thick)
   (ly:stencil-add
    (grob-interpret-markup grob
      (markup #:scale '(0.4 . 0.8) #:arrow-head X LEFT #t))
    (make-line-stencil thick thick 0 (- len thick) 0)
    (ly:stencil-translate
     (grob-interpret-markup grob
       (markup #:scale '(0.4 . 0.8) #:arrow-head X RIGHT #t))
     `(,len . 0))))

drawPlaceholderNoteWidth = {
  \override Staff.GridLine.stencil =
  #(lambda (grob)
     (ly:stencil-add
      (ly:grid-line-interface::print grob)
      (draw-two-headed-arrow grob
        (cdr placeholder-notehead-extent)
        0.3)))
}

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%% actual tests

\layout {
  ragged-right = ##f
}

\markup { Distances between dynamics should be identical: }
\score {
  \new Staff <<
    \new Voice { a'2 a' }
    \new Voice {
      s4\p
      \once \drawPlaceholderNoteWidth
      s\p
      s\p
      \once \drawPlaceholderNoteWidth
      s\p
    }
  >>
  \layout {
    line-width = 7.5\cm
  }
}

\markup { Dynamics should be centered on note columns: }
\score {
  \new Staff <<
    \omit Staff.GridLine
    \new Voice {
      <f' a' c''>8
      \once \omit NoteHead q
      <g' b' d''>
      \once \omit NoteHead q
    }
    \new Voice { s8\p s\p s\p s\p }
  >>
  \layout {
    line-width = 9\cm
  }
}

\markup { Turn should be centered between noteheads: }
\score {
  \new Staff <<
    \new Voice { e''2 e''2 }
    \new Voice { s4 \once \drawPlaceholderNoteWidth s\turn }
  >>
  \layout {
    line-width = 6.5\cm
  }
}


lyrExample = <<
  \new Staff { d'2 d' <f' g'>1 \drawPlaceholderNoteWidth s1 }
  \new Lyrics { \lyricmode { foo2 bar mmmm1 l2 bom } }
>>

\markup { Lyrics default-aligned (centered): }
\lyrExample

\markup { Lyrics right-aligned: }
{
  \override Score.LyricText.self-alignment-X = #RIGHT
  \lyrExample
}
