%% DO NOT EDIT this file manually; it is automatically
%% generated from LSR http://lsr.dsi.unimi.it
%% Make any changes in LSR itself, or in Documentation/snippets/new/ ,
%% and then run scripts/auxiliar/makelsr.py
%%
%% This file is in the public domain.
\version "2.14.2"

\header {
  lsrtags = "specific-notation, winds"

%%%    Translation of GIT committish: b482c3e5b56c3841a88d957e0ca12964bd3e64fa
  texidoces = "
El fragmento de música que aparece a continuación presenta todos los
diagramas de viento madera que se encuentran definidos en LilyPond por
el momento.

"
  doctitlees = "Listado de los diagramas para viento madera"

%%%    Translation of GIT committish: ab9e3136d78bfaf15cc6d77ed1975d252c3fe506


  texidocde="
Folgende Noten zeige alle Holzbläserdiagramme, die für LilyPond
definiert sind.

"
  doctitlede = "Liste der Holzbläserdiagramme"


%% Translation of GIT committish: d5307870fe0ad47904daba73792c7e17b813737f
  texidocfr = "
Voici les différents instruments à vent de la section des bois pour
lesquels LilyPond peut, à ce jour, afficher des doigtés.

"
  doctitlefr = "Liste des diagrammes de doigtés pour bois"

  texidoc = "
The following music shows all of the woodwind diagrams currently
defined in LilyPond.

"
  doctitle = "Woodwind diagrams listing"
} % begin verbatim

\relative c' {
  \textLengthOn
  c1^
  \markup {
    \center-column {
      'piccolo
      " "
       \woodwind-diagram
                  #'piccolo
                  #'()
    }
  }

  c1^
  \markup {
    \center-column {
       'flute
       " "
       \woodwind-diagram
          #'flute
          #'()
    }
  }
  c1^\markup {
    \center-column {
      'oboe
      " "
      \woodwind-diagram
        #'oboe
        #'()
    }
  }

  c1^\markup {
    \center-column {
      'clarinet
      " "
      \woodwind-diagram
        #'clarinet
        #'()
    }
  }

  c1^\markup {
    \center-column {
      'bass-clarinet
      " "
      \woodwind-diagram
        #'bass-clarinet
        #'()
    }
  }

  c1^\markup {
    \center-column {
      'saxophone
      " "
      \woodwind-diagram
        #'saxophone
        #'()
    }
  }

  c1^\markup {
    \center-column {
      'bassoon
      " "
      \woodwind-diagram
        #'bassoon
        #'()
    }
  }

  c1^\markup {
    \center-column {
      'contrabassoon
      " "
      \woodwind-diagram
        #'contrabassoon
        #'()
    }
  }
}
