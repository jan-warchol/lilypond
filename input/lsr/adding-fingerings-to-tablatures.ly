%% Do not edit this file; it is auto-generated from LSR http://lsr.dsi.unimi.it
%% This file is in the public domain.
\version "2.13.1"

\header {
  lsrtags = "fretted-strings"

%% Translation of GIT committish: c8446d6eb2fadbf8931a867741572582759935ad
  texidocfr = "Ajout de doigtés à des tablatures"

  doctitlefr = "
L'ajout de doigtés à des tablatures s'obtient en conjuguant des
@code{\\markup} et des @code{\\finger}.
"

  texidoc = "
To add fingerings to tablatures, use a combination of @code{\\markup}
and @code{\\finger}. 

"
  doctitle = "Adding fingerings to tablatures"
} % begin verbatim

one = \markup { \finger 1 }
two = \markup { \finger 2 }
threeTwo = \markup {
  \override #'(baseline-skip . 2)
  \column {
    \finger 3
    \finger 2
  }
}
threeFour = \markup {
  \override #'(baseline-skip . 2)
  \column {
    \finger 3
    \finger 4
  }
}

\score {
  \new TabStaff {
    \stemUp
    e8\4^\one b\2 <e, g\3 e'\1>^>[ b\2 e\4]
    <a\3 fis'\1>^>^\threeTwo[ b\2 e\4]
  }
}

