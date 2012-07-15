%% DO NOT EDIT this file manually; it is automatically
%% generated from LSR http://lsr.dsi.unimi.it
%% Make any changes in LSR itself, or in Documentation/snippets/new/ ,
%% and then run scripts/auxiliar/makelsr.py
%%
%% This file is in the public domain.
\version "2.14.2"

\header {
  lsrtags = "tweaks-and-overrides, pitches"

%% Translation of GIT committish: 28097cf54698db364afeb75658e4c8e0e0ccd716
  texidocfr = "
Les altérations supprimées lors d'un changement de tonalité sont par
défaut accolés à la nouvelle armure.  Ce comportement peut s'adapter
grâce à la propriété @code{'break-align-orders} de l'objet
@code{BreakAlignment}.

La valeur de @code{'break-align-orders} est constituée d'un vecteur à
trois composantes listant l'ordre des éléments à l'occasion d'une
rupture.  Seule la deuxième liste est ici modifiée,
@code{key-cancellation} étant déplacé avant @code{staff-bar}.  En ne
modifiant que la deuxième liste, cette modification dans l'ordre
d'apparition des éléments sera effective seulement en cours de système,
et non en début ou fin de ligne.

"
  doctitlefr = "Séparation entre altérations annulées et nouvelle armure"

  texidoc = "
By default, the accidentals used for key cancellations are placed
adjacent to those for key signature changes.  This behavior can be
changed by overriding the @code{'break-align-orders} property of the
@code{BreakAlignment} grob.


The value of @code{'break-align-orders} is a vector of length 3, with
quoted lists of breakable items as elements.  This example only
modifies the second list, moving @code{key-cancellation} before
@code{staff-bar}; by modifying the second list, break alignment
behavior only changes in the middle of a system, not at the beginning
or the end.

"
  doctitle = "Separating key cancellations from key signature changes"
} % begin verbatim

\new Staff {
  \override Score.BreakAlignment #'break-align-orders =
    #'#((left-edge ambitus breathing-sign clef staff-bar
                   key-cancellation key-signature time-signature custos)

        (left-edge ambitus breathing-sign clef key-cancellation
                   staff-bar key-signature time-signature custos)

        (left-edge ambitus breathing-sign clef key-cancellation
                   key-signature staff-bar time-signature custos))

  \key des \major
  c'1
  \bar "||"
  \key bes \major
  c'1
}
