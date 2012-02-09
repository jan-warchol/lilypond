;;;; This file is part of LilyPond, the GNU music typesetter.
;;;;
;;;; Copyright (C) 2012 Mike Solomon <mike@apollinemike.com>
;;;;
;;;; LilyPond is free software: you can redistribute it and/or modify
;;;; it under the terms of the GNU General Public License as published by
;;;; the Free Software Foundation, either version 3 of the License, or
;;;; (at your option) any later version.
;;;;
;;;; LilyPond is distributed in the hope that it will be useful,
;;;; but WITHOUT ANY WARRANTY; without even the implied warranty of
;;;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;;;; GNU General Public License for more details.
;;;;
;;;; You should have received a copy of the GNU General Public License
;;;; along with LilyPond.  If not, see <http://www.gnu.org/licenses/>.

;; tools for transform-matrices following the standard at
;; http://www.w3.org/TR/SVG/coords.html

;; a list in the form
;; (list a b c d e f g)
;; becomes this matrix:
;; [ a c e ]
;; [ b d f ]
;; [ 0 0 1 ]
;; when this transforms a point (x,y), the point is written as matrix:
;; [ x ]
;; [ y ]
;; [ 1 ]

(define post-processing-box-cache '())

(define curve-quantization 10)

(define ellipse-quantization 20)

(define (multiply-transform-matrices x y)
  "Multiply transform matrices x and y to get a new transform
   matrix.  Equivalent to 3x3 matrix multiplication when the
   matrix is expanded using the convention explained above."
  (let ((x0 (car x))
        (x1 (cadr x))
        (x2 (caddr x))
        (x3 (caddr (reverse x)))
        (x4 (cadr (reverse x)))
        (x5 (car (reverse x)))
        (y0 (car y))
        (y1 (cadr y))
        (y2 (caddr y))
        (y3 (caddr (reverse y)))
        (y4 (cadr (reverse y)))
        (y5 (car (reverse y))))
    (list (+ (* x0 y0) (* x2 y1))
          (+ (* x1 y0) (* x3 y1))
          (+ (* x0 y2) (* x2 y3))
          (+ (* x1 y2) (* x3 y3))
          (+ (* x0 y4) (* x2 y5) x4)
          (+ (* x1 y4) (* x3 y5) x5))))

(define (multiply-transform-matrix-and-point m p)
  "Multiply transform matrix m and point p to get a new point."
  (let ((m0 (car m))
        (m1 (cadr m))
        (m2 (caddr m))
        (m3 (caddr (reverse m)))
        (m4 (cadr (reverse m)))
        (m5 (car (reverse m)))
        (x (car p))
        (y (cdr p)))
    (cons (+ (* m0 x) (* m2 y) m4)
          (+ (* m1 x) (* m3 y) m5))))

(define (linear-map final-l final-r orig-l orig-r x)
  (+ final-l
     (* (- final-r final-l)
        (/ (- x orig-l)
           (- orig-r orig-l)))))

; turns a transform matrix into a translate, scale, and rotate
(define (make-translate-scale-rotate m)
  (let* ((m0 (car m))
         (m1 (cadr m))
         (m2 (caddr m))
         (m3 (caddr (reverse m)))
         (m4 (cadr (reverse m)))
         (m5 (car (reverse m)))
         (translate (cons m4 m5))
         (rotate (atan (- m2) m0))
         (scale (if (eqv? (cos rotate) 0.0)
                    (cons (/ (- m2) (sin rotate)) (/ m1 (sin rotate)))
                    (cons (/ m0 (cos rotate)) (/ m3 (cos rotate))))))
    (list translate scale rotate)))

(define (make-draw-line-boxes trans th x0 y0 x1 y1)
  (let* ((points (map (lambda (x)
                   (multiply-transform-matrix-and-point
                     trans
                     (cons (linear-map x0 x1 0 curve-quantization x)
                           (linear-map y0 y1 0 curve-quantization x))))
                   (iota (1+ curve-quantization))))
         (thicks (coord-scale (cons th th)
                              (cadr (make-translate-scale-rotate trans)))))
    (map (lambda (b0 b1)
           (ly:make-box (list b0 b1)
                        (/ (abs (car thicks)) 2)
                        (/ (abs (cdr thicks)) 2)))
         (reverse (cdr (reverse points)))
         (cdr points))))

(define (make-partial-ellipse-boxes trans x-rad y-rad start end th connect fill)
  (let* ((start (* PI-OVER-180 (angle-0-360 start)))
         (end (* PI-OVER-180 (angle-0-360 end)))
         (end (if (eqv? end start) (+ end TWO-PI) end))
         (sunit (make-polar 1 start))
         (eunit (make-polar 1 end))
         (sp (cons (* (real-part sunit) x-rad)
                   (* (imag-part sunit) y-rad)))
         (ep (cons (* (real-part eunit) x-rad)
                   (* (imag-part eunit) y-rad)))
         (points (map (lambda (x)
                   (let* ((ang (linear-map start end 0 ellipse-quantization x))
                          (coord (make-polar 1 ang)))
                     (multiply-transform-matrix-and-point
                       trans
                       (cons (* (real-part coord) x-rad)
                             (* (imag-part coord) y-rad)))))
                   (iota (1+ ellipse-quantization))))
         (thicks (coord-scale (cons th th)
                              (cadr (make-translate-scale-rotate trans)))))
    (append
      (if connect
          (make-draw-line-boxes trans th (car sp) (cdr sp) (car ep) (cdr ep))
          '())
      (map (lambda (b0 b1)
             (ly:make-box (list b0 b1)
                          (/ (abs (car thicks)) 2)
                          (/ (abs (cdr thicks)) 2)))
           (reverse (cdr (reverse points)))
           (cdr points)))))

(define (make-draw-bezier-boxes trans th x0 y0 x1 y1 x2 y2 x3 y3)
  (let* ((bez (make-bezier `(,x0 . ,y0) `(,x1 . ,y1) `(,x2 . ,y2) `(,x3 . ,y3)))
         (points (map (lambda (x)
                   (multiply-transform-matrix-and-point
                     trans
                     (car (cdr (split-bezier bez (/ x curve-quantization))))))
                   (cdr (iota curve-quantization))))
         (points (append `(,(multiply-transform-matrix-and-point trans `(,x0 . ,y0)))
                           points
                         `(,(multiply-transform-matrix-and-point trans `(,x3 . ,y3)))))
         (thicks (coord-scale (cons th th)
                              (cadr (make-translate-scale-rotate trans)))))
    (map (lambda (b0 b1)
           (ly:make-box (list b0 b1)
                        (/ (abs (car thicks)) 2)
                        (/ (abs (cdr thicks)) 2)))
         (reverse (cdr (reverse points)))
         (cdr points))))

(define (make-round-filled-box-boxes trans left right bottom top th)
  (let* ((p0 (cons (- left) (- bottom)))
         (p1 (cons right top))
         (points (map (lambda (x)
                   (multiply-transform-matrix-and-point
                     trans
                     x))
                   (list p0 p1)))
         (thicks (coord-scale (cons th th)
                              (cadr (make-translate-scale-rotate trans)))))
    (list (ly:make-box points
                       (/ (abs (car thicks)) 2)
                       (/ (abs (cdr thicks)) 2)))))

(define (group-tokens-into-commands tokens out temp convert-number)
  (cond
    ((null? tokens)
     (filter (lambda (x) (not (null? x))) (append out (list (reverse temp)))))
    ((not ((if convert-number string->number number?) (car tokens)))
     (group-tokens-into-commands (cdr tokens)
                                 (append out (list (reverse temp)))
                                 (list (car tokens))
                                 convert-number))
    (else
      (group-tokens-into-commands (cdr tokens)
                                  out
                                  (cons ((if convert-number
                                             string->number
                                             values)
                                         (car tokens))
                                        temp)
                                  convert-number))))

(define command-to-token-alist
  '((moveto . "M")
    (rmoveto . "m")
    (lineto . "L")
    (rlineto . "l")
    (curveto . "C")
    (rcurveto . "c")
    (closepath . "z")))

(define (get-path-from-quoted-material l)
  (cond
    ((not (pair? l))
     #f)
    ((memv (car l) '(moveto rmoveto lineto rlineto curveto rcurveto closepath))
     l)
    (else
      (flatten-list (filter values (list (get-path-from-quoted-material (car l))
                                         (get-path-from-quoted-material (cdr l))))))))

(define (get-coords-from-quoted-material l)
  (cond
    ((not (pair? l))
     #f)
    ((number? (car l))
     l)
    (else
      (flatten-list (filter values (list (get-coords-from-quoted-material (car l))
                                         (get-coords-from-quoted-material (cdr l))))))))

; a bit kludgy, as we go from number to string and back again
; doesn't seem to slow things down too much

(define (transform-path-into-command-tokens path)
  (map (lambda (x)
         (if (assoc-get x command-to-token-alist #f)
             (assoc-get x command-to-token-alist)
             x))
       ; this is really ugly...
       ; probably will fail in certain contexts...
       (get-path-from-quoted-material path)))

(define (split-into-sublists l n)
  (if (eq? n 0)
      '(())
      (map (lambda (x) (list-head (list-tail l (* x n)) n))
         (iota (/ (length l) n)))))

(define n-for-command
  '(("m" . 2) ("c" . 6) ("s" . 4) ("l" . 2) ("v" . 1) ("h" . 1) ("z" . 0)
    ("M" . 2) ("C" . 6) ("S" . 4) ("L" . 2) ("V" . 1) ("H" . 1) ("Z" . 0)))

(define (expand-command c)
  (map (lambda (x) (cons (car c) x))
       (split-into-sublists (cdr c) (assoc-get (car c) n-for-command))))

(define (update-coords l x y)
  (map (lambda (idx) (+ (list-ref l idx)
                        (if (eq? 0 (modulo idx 2)) x y)))
       (iota (length l))))

(define (all-commands-to-absolute commands
                                  out
                                  first?
                                  start-x
                                  start-y
                                  cur-x
                                  cur-y)
  (cond
    ((null? commands)
     (reverse out))
    ; a close command
    ((member (caar commands) '("Z" "z"))
     (all-commands-to-absolute (cdr commands)
                               (cons '("Z") out)
                               #f
                               start-x
                               start-y
                               start-x
                               start-y))
    ; an absolute moveto or the first moveto, abs or relative
    ((or (equal? (caar commands) "M")
         (and (equal? (caar commands) "m") first?))
     (all-commands-to-absolute (cdr commands)
                               (cons (cons "M" (cdar commands)) out)
                               #f
                               (cadar commands)
                               (caddar commands)
                               (cadar commands)
                               (caddar commands)))
    ; an absolute vertical line
    ((equal? (caar commands) "V")
     (all-commands-to-absolute (cdr commands)
                               (cons (car commands) out)
                               #f
                               start-x
                               start-y
                               cur-x
                               (cadar commands)))
    ; an absolute horizontal line
    ((equal? (caar commands) "H")
     (all-commands-to-absolute (cdr commands)
                               (cons (car commands) out)
                               #f
                               start-x
                               start-y
                               (cadar commands)
                               cur-y))
    ; a relative vertical line
    ((equal? (caar commands) "v")
     (all-commands-to-absolute (cdr commands)
                               (cons `("V" ,(+ cur-y (cadar commands))) out)
                               #f
                               start-x
                               start-y
                               cur-x
                               (+ cur-y (cadar commands))))
    ; a relative horizontal line
    ((equal? (caar commands) "h")
     (all-commands-to-absolute (cdr commands)
                               (cons `("H" ,(+ cur-x (cadar commands))) out)
                               #f
                               start-x
                               start-y
                               (+ cur-x (cadar commands))
                               cur-y))
    ; a relative moveto after the first
    ((equal? (caar commands) "m")
     (let* ((new-pts (update-coords (cdar commands) cur-x cur-y)))
       (all-commands-to-absolute (cdr commands)
                                 (cons (cons "M" new-pts) out)
                                 #f
                                 (car new-pts)
                                 (cadr new-pts)
                                 (car new-pts)
                                 (cadr new-pts))))
    ; a relative curve, smooth curve, or line
    ((member (caar commands) '("s" "l" "c"))
     (let* ((new-pts (update-coords (cdar commands) cur-x cur-y)))
       (all-commands-to-absolute (cdr commands)
                                 (cons (cons (string-upcase (caar commands)) new-pts) out)
                                 #f
                                 start-x
                                 start-y
                                 (cadr (reverse new-pts))
                                 (car (reverse new-pts)))))
    ; an absolute coordinate
    ; no manipulation necessary
    (else
     (all-commands-to-absolute (cdr commands)
                               (cons (car commands) out)
                               #f
                               start-x
                               start-y
                               (cadr (reverse (car commands)))
                               (car (reverse (car commands)))))))

(define (flip-over to-reflect origin)
  (- (* 2 origin) to-reflect))

(define (absolute-commands-to-coords commands
                                     out
                                     start-x
                                     start-y
                                     cur-x
                                     cur-y)
  (cond
    ((null? commands)
     (reverse out))
    ; a close command
    ((equal? (caar commands) "Z")
     (absolute-commands-to-coords (cdr commands)
                                  (cons `(,cur-x ,cur-y ,start-x ,start-y) out)
                                  start-x
                                  start-y
                                  start-x
                                  start-y))
    ; a moveto
    ((equal? (caar commands) "M")
     (absolute-commands-to-coords (cdr commands)
                                  out
                                  (cadar commands)
                                  (caddar commands)
                                  (cadar commands)
                                  (caddar commands)))
    ; a vertical line
    ((equal? (caar commands) "V")
     (absolute-commands-to-coords (cdr commands)
                                  (cons `(,cur-x ,cur-y ,cur-x ,(cadar commands)) out)
                                  start-x
                                  start-y
                                  cur-x
                                  (cadar commands)))
    ; a horizontal line
    ((equal? (caar commands) "H")
     (absolute-commands-to-coords (cdr commands)
                                  (cons `(,cur-x ,cur-y ,(cadar commands) ,cur-y) out)
                                  start-x
                                  start-y
                                  (cadar commands)
                                  cur-y))
    ; a smooth curve
    ((equal? (caar commands) "S")
     (let* ((last-item (car out))
            (origin (list-tail last-item (- (length last-item) 2)))
            (to-reflect (list-head (list-tail last-item (- (length last-item) 4)) 2)))
       (absolute-commands-to-coords (cdr commands)
                                    (cons (append `(,cur-x ,cur-y) (map flip-over to-reflect origin) (cdar commands)) out)
                                    start-x
                                    start-y
                                    (cadr (reverse (car commands)))
                                    (car (reverse (car commands))))))
    ; a line or curve
    (else
     (absolute-commands-to-coords (cdr commands)
                                  (cons (append `(,cur-x ,cur-y) (cdar commands)) out)
                                  start-x
                                  start-y
                                  (cadr (reverse (car commands)))
                                  (car (reverse (car commands)))))))

(define (chop-string-at-tokens str tokens out)
  (let ((idx (string-index str (string-ref (car tokens) 0))))
    (if (eq? (length tokens) 1)
      (reverse (filter (lambda (x) (not (equal? x "")))
                       (append `(,(substring str (1+ idx))
                                 ,(car tokens)
                                 ,(substring str 0 idx))
                               out)))
      (chop-string-at-tokens (substring str (1+ idx))
                             (cdr tokens)
                             (cons (car tokens)
                                   (cons (substring str 0 idx)
                                         out))))))

(define (numbers-and-letters d)
  (let* ((tokens (map match:substring
                      (list-matches "[MZHVLSCmzhvlsc]" d))))
  (chop-string-at-tokens d tokens '())))

(define (not-vestigial-closing-line l)
  (if (= (length l) 4)
      (not (and (= (car l) (caddr l)) (= (cadr l) (cadddr l))))
      #t))

(define (parse-d-tokens-into-absolute-lines-and-curves d-l convert-number)
  (let* ((commands (group-tokens-into-commands d-l '() '() convert-number))
         (commands (reverse (reduce append '() (map expand-command commands))))
         (commands (all-commands-to-absolute commands
                                             '()
                                             #t
                                             0
                                             0
                                             0
                                             0))
         (commands (absolute-commands-to-coords commands
                                                '()
                                                0
                                                0
                                                0
                                                0)))
  (filter not-vestigial-closing-line commands)))

(define (parse-path-into-absolute-lines-and-curves path)
  (let* ((d-l (transform-path-into-command-tokens path)))
    (parse-d-tokens-into-absolute-lines-and-curves d-l #f)))

(define (parse-d-into-absolute-lines-and-curves d)
  (let* ((d (regexp-substitute/global #f "[\n]+" d 'pre " " 'post))
         (d-l (filter (lambda (x) (not (equal? x "")))
                     (reduce append
                             '()
                             (reverse (map (lambda (y) (string-split y #\space))
                                      (numbers-and-letters d)))))))
    (parse-d-tokens-into-absolute-lines-and-curves d-l #t)))

(define (set-assoc-cache-to-null-and-return-null name-style name)
  (set! box-cache (assoc-set! box-cache
                              (string-append name-style name)
                              '()))
  '())

(define (make-polygon-boxes trans coords blot-diameter is-filled)
  (let* ((my-coords (get-coords-from-quoted-material coords))
         (commands (flatten-list (map (lambda (x)
                                        (cons (if (= x 0) 'moveto 'lineto)
                                              (list-head (list-tail my-coords
                                                                    (* x 2))
                                                         2)))
                                      (iota (/ (length my-coords) 2))))))
    (make-path-boxes trans blot-diameter (append commands '(closepath)))))

(define (make-path-boxes trans th commands)
  (let* ((alc (parse-path-into-absolute-lines-and-curves commands)))
    (flatten-list (map (lambda (x)
                       (apply
                         (if (eq? (length x) 4)
                             make-draw-line-boxes
                             make-draw-bezier-boxes)
                         (append `(,trans ,th) x)))
                       alc))))

(define (lines-and-curves-to-extent l mmx mmy ex ey)
  (map (lambda (idx)
         (if (= (modulo idx 2) 0)
             (linear-map (car ex) (cdr ex) (car mmx) (cdr mmx) (list-ref l idx))
             (linear-map (car ey) (cdr ey) (car mmy) (cdr mmy) (list-ref l idx))))
       (iota (length l))))

(define (make-curves-for-glyph-and-cache d font name-style name)
  (let* ((sten (ly:font-get-glyph font name))
         (ex (ly:stencil-extent sten X))
         (ey (ly:stencil-extent sten Y)) (dummy (format #t "EX EY N ~a ~a ~a ~a\n" name-style name ey ex))
         (alc (parse-d-into-absolute-lines-and-curves d))
         ; min-max returned as '((minx maxx) (miny maxy))
         (mm (map (lambda (x)
                    (apply (if (eq? (length x) 4)
                               line-min-max
                               bezier-min-max)
                           x))
                  alc))
         (mmx (cons (apply min (map (lambda (x) (caar x)) mm)) (apply max (map (lambda (x) (cadar x)) mm))))
         (mmy (cons (apply min (map (lambda (x) (caadr x)) mm)) (apply max (map (lambda (x) (cadadr x)) mm))))
         (alc (map (lambda (x) (lines-and-curves-to-extent x mmx mmy ex ey)) alc)))
    (set! box-cache (assoc-set! box-cache
                                (string-append name-style name)
                                alc))
    alc))

;;; code dup from output-svg.scm
(define glyph-path-regexp
  (make-regexp "d=\"([-MmZzLlHhVvCcSsQqTt0-9.\n ]*)\""))

(define (glyph-element-regexp name)
  (make-regexp (string-append "<glyph"
			      "(([[:space:]]+[-a-z]+=\"[^\"]*\")+)?"
			      "[[:space:]]+glyph-name=\"("
			      name
			      ")\""
			      "(([[:space:]]+[-a-z]+=\"[^\"]*\")+)?"
			      "([[:space:]]+)?"
			      "/>")))

(define (get-gylph-from-font-cache font name-style glyph)
  (let* ((sten (ly:font-get-glyph font glyph))
               (ex (ly:stencil-extent sten X))
               (ey (ly:stencil-extent sten Y))
               (path-info (hashq-ref (hashq-ref box-hash
                                                (string->symbol name-style))
                          (string->symbol glyph))))
          (map (lambda (x)
                 (lines-and-curves-to-extent
                   x
                   (assoc-get 'mmx path-info)
                   (assoc-get 'mmy path-info)
                   ex
                   ey))
               (assoc-get 'paths path-info))))

(define (make-named-glyph-boxes trans font glyph)
  (let* ((name-style (font-name-style font))
         (font-file (ly:find-file (string-append name-style ".svg"))))
    (if font-file
        (flatten-list (map (lambda (x)
                             (apply
                               (if (eq? (length x) 4)
                                   make-draw-line-boxes
                                   make-draw-bezier-boxes)
                               (append `(,trans 0) x)))
                           (get-gylph-from-font-cache font name-style glyph)))
        (begin
          (ly:warning (_ "cannot find SVG font ~S") font-file)
          '()))))

(define (make-glyph-string-boxes trans
                                 font-name
                                 size
                                 cid?
                                 w-h-x-y-named-glyphs)
  (let* ((widths (map (lambda (x) (car x)) (cadr w-h-x-y-named-glyphs)))
         (heights (map (lambda (x) (cadr x)) (cadr w-h-x-y-named-glyphs)))
         (xoffs (map (lambda (x) (reduce + 0 (list-head widths x)))
                     (iota (1+ (length (cadr w-h-x-y-named-glyphs))))))
         (pts (map (lambda (x0 x1 y)
                     (list (cons x0 (car y)) (cons x1 (cdr y))))
                   (reverse (cdr (reverse xoffs))) (cdr xoffs) heights))
         (pts (map (lambda (pair)
                     (map (lambda (pt)
                            (multiply-transform-matrix-and-point
                              trans
                              pt))
                          pair))
                   pts)))
    (map (lambda (pair) (ly:make-box pair)) pts)))

(define (make-box-from-stencil t s)
  (cond
    ((eq? (car s)
          'draw-line)
     (apply make-draw-line-boxes (cons t (cdr s))))
    ((eq? (car s)
          'dashed-line)
     (apply make-draw-line-boxes (cons t (list (cadr s) 0 0 (list-ref s 4) (list-ref s 5)))))
    ; double the radius
    ((eq? (car s)
          'circle)
     (let ((rad (cadr s))
           (th (caddr s)))
       (make-partial-ellipse-boxes t rad rad 0.0 360.0 th #f #t)))
    ((eq? (car s)
          'ellipse)
     (let ((x-rad (cadr s))
           (y-rad (caddr s))
           (th (cadddr s)))
       (make-partial-ellipse-boxes t x-rad y-rad 0.0 360.0 th #f #t)))
    ((eq? (car s)
          'partial-ellipse)
     (let ((x-rad (cadr s))
           (y-rad (caddr s))
           (start (cadddr s))
           (end (list-ref s 4))
           (th (list-ref s 5))
           (connect (list-ref s 6))
           (fill (list-ref s 7)))
       (make-partial-ellipse-boxes t
                                   x-rad
                                   y-rad
                                   start
                                   end
                                   th
                                   (or connect fill)
                                   (or connect fill))))
    ((eq? (car s)
          'round-filled-box)
     (apply make-round-filled-box-boxes (cons t (cdr s))))
    ((eq? (car s)
          'named-glyph)
     (apply make-named-glyph-boxes (cons t (cdr s))))
    ((eq? (car s)
          'polygon)
     (apply make-polygon-boxes (cons t (cdr s))))
    ((eq? (car s)
          'path)
     (apply make-path-boxes (cons t (list-head (cdr s) 2))))
    ((eq? (car s)
          'glyph-string)
     (apply make-glyph-string-boxes (cons t (cdr s))))
    (else
     '())))
 
(define (stencil-traverser expr head out)
  (cond
    ((null? expr)
     '())
    ((equal? "" expr)
     '())
    ((eq? (car expr)
          'combine-stencil) 
     (append out (apply append
                        (map (lambda (x)
                               (stencil-traverser x head '()))
                             (cdr expr)))))
    ((eq? (car expr)
          'footnote)
     '())
    ((eq? (car expr)
          'translate-stencil)
     (stencil-traverser (caddr expr)
                        (cons `(1 0 0 1 ,(caadr expr) ,(cdadr expr)) head)
                        out))
    ((eq? (car expr)
          'scale-stencil)
     (stencil-traverser (caddr expr)
                        (cons `(,(caadr expr) 0 0 ,(cadadr expr) 0 0) head)
                        out))
    ((eq? (car expr)
          'rotate-stencil)
     (stencil-traverser (caddr expr)
                        (append `((1 0 0 1 ,(- (car (cadadr expr))) ,(- (cdr (cadadr expr))))
                                  (,(cos (caadr expr))
                                   ,(sin (caadr expr))
                                   ,(- (sin (caadr expr)))
                                   ,(cos (caadr expr))
                                   0
                                   0)
                                  (1 0 0 1 ,(car (cadadr expr)) ,(cdr (cadadr expr))))
                                head)
                        out))
    ((eq? (car expr)
          'delay-stencil-evaluation)
     (stencil-traverser (force (cadr expr)) head out))
    ((eq? (car expr)
          'grob-cause)
     (stencil-traverser (caddr expr) head out))
    ((eq? (car expr)
          'color)
     (stencil-traverser (caddr expr) head out))
    ((eq? (car expr)
          'id)
     (stencil-traverser (caddr expr) head out))
    (else
     (list (cons expr head)))))

(define-public (grob::vertical-skylines-from-stencil grob)
  (if (not (ly:grob-property grob 'stencil #f))
      (ly:make-skyline-pair '() X)
      (let* ((s (ly:grob-property grob 'stencil))
             (sinfo (stencil-traverser (ly:stencil-expr s) '() '()))
             (bxs (map (lambda (x)
                         (make-box-from-stencil (reduce multiply-transform-matrices
                                                        '(1 0 0 1 0 0)
                                                        (reverse (cdr x)))
                                                (car x)))
                       sinfo))
            (bxs (reduce append '() bxs)))
        (ly:make-skyline-pair bxs X 0.0))))

(define-public (grob::vertical-skylines-from-element-stencils grob)
  (let* ((elts (ly:grob-array->list (ly:grob-object grob 'elements)))
         (refp (ly:grob-common-refpoint-of-array grob
                                                 (ly:grob-object grob
                                                                 'elements)
                                                 X))
         (x-pos (map (lambda (x) (ly:grob-relative-coordinate x refp X)) elts))
         (x-pos (map (lambda (x) (- x (car x-pos))) x-pos))
         (vertical-skylines (map (lambda (x)
                                   (let ((sky (ly:grob-property x 'vertical-skylines)))
                                     (if (ly:skyline-pair? sky)
                                         sky
                                         (ly:make-skyline-pair '() 0 X))))
                                 elts))
         (vertical-skylines (map (lambda (sky xp)
                                   (ly:skyline-pair-shift sky xp))
                                 vertical-skylines
                                 x-pos))
         (vertical-skyline (reduce ly:skyline-pair-merge
                                   (ly:make-skyline-pair '() X 0.0)
                                   vertical-skylines)))
  vertical-skyline))
