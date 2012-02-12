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

(map ly:load '("stencil-integral.scm"
               "stencil.scm"))

(use-modules (ice-9 regex))

(define pre-defined-box-data
'((".notdef" . ((paths . ()) (mmx . (0 . 0)) (mmy . (0 . 0))))
  ("space" . ((paths . ()) (mmx . (0 . 0)) (mmy . (0 . 0))))))

(define (make-curves-for-glyph d font name)
  (let* ((sten (ly:font-get-glyph font name))
         (ex (ly:stencil-extent sten X))
         (ey (ly:stencil-extent sten Y))
         (alc (parse-d-into-absolute-lines-and-curves d))
         ; min-max returned as '((minx maxx) (miny maxy))
         (mm (map (lambda (x)
                    (apply (if (eq? (length x) 4)
                               line-min-max
                               bezier-min-max)
                           x))
                  alc))
         (mmx (cons (apply min (map (lambda (x) (caar x)) mm))
                    (apply max (map (lambda (x) (cadar x)) mm))))
         (mmy (cons (apply min (map (lambda (x) (caadr x)) mm))
                    (apply max (map (lambda (x) (cadadr x)) mm)))))
    `((paths . ,alc) (mmx . ,mmx) (mmy . ,mmy))))

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

(define (extract-glyph-boxes font all-glyphs name)
  (let* ((new-name (regexp-quote name))
         (regexp (regexp-exec (glyph-element-regexp new-name) all-glyphs))
         (glyph (match:substring regexp))
         (d-attr (regexp-exec glyph-path-regexp glyph))
         (d-attr? (regexp-match? d-attr))
         (d-attr-value (if d-attr? (match:substring d-attr 1) "")))
    (if (not (equal? d-attr-value ""))
        (make-curves-for-glyph d-attr-value font name)
        (ly:error (_ "Cannot make integral for this glyph glyph: ~A") name))))

(define (svg-defs svg-font)
  (let ((start (string-contains svg-font "<defs>"))
	(end (string-contains svg-font "</defs>")))
    (substring svg-font (+ start 7) (- end 1))))

(define (get-box-data font svg-font glyph)
  (let ((all-glyphs (svg-defs (cached-file-contents svg-font))))
    (extract-glyph-boxes font all-glyphs glyph)))

(define (make-named-glyph-boxes font glyph)
  (if (assoc-get glyph pre-defined-box-data #f)
      (assoc-get glyph pre-defined-box-data)
    (let* ((name-style (font-name-style font))
           (font-file (ly:find-file (string-append name-style ".svg"))))
      (if font-file
          (get-box-data font
                        font-file
                        glyph)
          (ly:error (_ "cannot find SVG font ~S") font-file)))))

(define (box-data-for-font font-name)
  (let* ((font (ly:system-font-load font-name))
         (info (map (lambda (glyph)
                      (cons glyph (make-named-glyph-boxes font glyph)))
                    (ly:otf-glyph-list font))))
  (format #t "(define boxes-for-~a (make-hash-table ~a))\n" font-name (length info))
  (for-each (lambda (x) (format #t "(hashq-set! boxes-for-~a '~a '~a)\n" font-name (car x) (cdr x))) info)
  (format #t "(hashq-set! box-hash '~a boxes-for-~a)\n" font-name font-name)))

(define font-list
  '("emmentaler-11"
    "emmentaler-13"
    "emmentaler-14"
    "emmentaler-16"
    "emmentaler-18"
    "emmentaler-20"
    "emmentaler-23"
    "emmentaler-26"
    "emmentaler-brace"))

(format #t "(define box-hash (make-hash-table ~a))\n" (length font-list))

(for-each
  (lambda (x)
    (box-data-for-font x))
  font-list)