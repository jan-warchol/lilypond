%% DO NOT EDIT this file manually; it is automatically
%% generated from LSR http://lsr.dsi.unimi.it
%% Make any changes in LSR itself, or in Documentation/snippets/new/ ,
%% and then run scripts/auxiliar/makelsr.py
%%
%% This file is in the public domain.
\version "2.14.2"

\header {
  lsrtags = "tweaks-and-overrides, scheme-language, really-cool, editorial-annotations"

%% Translation of GIT committish: d5307870fe0ad47904daba73792c7e17b813737f
  texidocfr = "
Lorsqu'il est impossible d'obtenir facilement une allure particulière
pour les têtes de note en recourant à la technique du @code{\\markup}, un
code PostScript peut vous tirer d'embarras.  Voici comment générer des
têtes ressemblant à des parallélogrammes.

"
  doctitlefr = "Utilisation de PostScript pour générer des têtes de note à l'allure particulière"

  texidoc = "
When a note head with a special shape cannot easily be generated with
graphic markup, PostScript code can be used to generate the shape.
This example shows how a parallelogram-shaped note head is generated.

"
  doctitle = "Using PostScript to generate special note head shapes"
} % begin verbatim


parallelogram =
  #(ly:make-stencil (list 'embedded-ps
    "gsave
      currentpoint translate
      newpath
      0 0.25 moveto
      1.3125 0.75 lineto
      1.3125 -0.25 lineto
      0 -0.75 lineto
      closepath
      fill
      grestore" )
    (cons 0 1.3125)
    (cons -.75 .75))

myNoteHeads = \override NoteHead #'stencil = \parallelogram
normalNoteHeads = \revert NoteHead #'stencil

\relative c'' {
  \myNoteHeads
  g4 d'
  \normalNoteHeads
  <f, \tweak #'stencil \parallelogram b e>4 d
}
