;; Galapix - an image viewer for large image collections
;; Copyright (C) 2008-2019 Ingo Ruhnke <grumbel@gmail.com>
;;
;; This program is free software: you can redistribute it and/or modify
;; it under the terms of the GNU General Public License as published by
;; the Free Software Foundation, either version 3 of the License, or
;; (at your option) any later version.
;;
;; This program is distributed in the hope that it will be useful,
;; but WITHOUT ANY WARRANTY; without even the implied warranty of
;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;; GNU General Public License for more details.
;;
;; You should have received a copy of the GNU General Public License
;; along with this program.  If not, see <http://www.gnu.org/licenses/>.

;; Install with:
;; guix package --install-from-file=guix.scm

(use-modules (ice-9 popen)
             (ice-9 rdelim)
             (guix build utils)
             (guix build-system cmake)
             (guix gexp)
             (guix git-download)
             (guix licenses)
             (guix packages)
             (gnu packages base)
             (gnu packages mcrypt)
             (gnu packages boost)
             (gnu packages curl)
             (gnu packages gcc)
             (gnu packages gl)
             (gnu packages imagemagick)
             (gnu packages pkg-config)
             (gnu packages photo)
             (gnu packages python)
             (gnu packages sdl)
             (gnu packages sqlite)
             )

(define %source-dir (dirname (current-filename)))

(define current-commit
  (with-directory-excursion %source-dir
   (let* ((port   (open-input-pipe "git describe --tags"))
          (output (read-line port)))
     (close-pipe port)
     (string-trim-right output #\newline))))

;;           (add-before 'configure 'fixgcc7
;;             (lambda _
;;               (unsetenv "C_INCLUDE_PATH")
;;               (unsetenv "CPLUS_INCLUDE_PATH")))

(define (source-predicate . dirs)
  (let ((preds (map (lambda (p)
                      (git-predicate (string-append %source-dir p)))
                    dirs)))
    (lambda (file stat)
      (let loop ((f (car preds))
                 (rest (cdr preds)))
        (if (f file stat)
            #t
            (if (not (nil? rest))
                (loop (car rest) (cdr rest))
                #f))))))

(package
 (name "galapix")
 (version current-commit)
 (source (local-file %source-dir
                     #:recursive? #t
                     #:select? (source-predicate ""
                                                 "/external/benchmark"
                                                 "/external/glm"
                                                 "/external/googletest"
                                                 "/external/logmich"
                                                 "/external/sexp-cpp"
                                                 "/external/uitest")))
 (arguments
  `(#:tests? #f
    #:configure-flags '()
    #:phases (modify-phases
              %standard-phases
              (add-before 'configure 'fixgcc9
                          (lambda _
                            (unsetenv "C_INCLUDE_PATH")
                            (unsetenv "CPLUS_INCLUDE_PATH")))
              (add-before 'configure 'patch-python
                          (lambda _
                            (substitute* "bin2h.py"
                                         (("/usr/bin/env python") (which "python3")))
                            #t))

              )))
 (build-system cmake-build-system)
 (inputs `(("python" ,python)
           ("sdl2" ,sdl2)
           ("sdl2-image" ,sdl2-image)
           ("sdl2-mixer" ,sdl2-mixer)
           ("mesa" ,mesa)
           ("glew" ,glew)
           ("curl" ,curl)
           ("boost" ,boost)
           ("sqlite" ,sqlite)
           ("libmhash" ,libmhash)
           ("imagemagick" ,imagemagick)
           ("libexif" ,libexif)
           ("coreutils" ,coreutils)
           ))
 (native-inputs `(("pkg-config" ,pkg-config)
                  ("gcc" ,gcc-9)))
 (synopsis "An image viewer for large image collections")
 (description "An image viewer for large image collections")
 (home-page "https://gitlab.com/galapix/galapix")
 (license gpl3+))

;; EOF ;;
