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

(set! %load-path
      (cons* "/ipfs/QmafuNSaiqymsEvRECwQaz7TSz5hYMWiJ3X7Z3QGwuThPb/guix-cocfree_v0.0.0-37-g291d4eb"
             %load-path))

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
             (guix-cocfree utils))

(define %source-dir (dirname (current-filename)))

(define-public galapix-0.3
  (package
   (name "galapix-0.3")
   (version (version-from-source %source-dir))
   (source (local-file %source-dir
                       #:recursive? #t
                       #:select? (source-select-function %source-dir
                                                         '(""
                                                           "/external/benchmark"
                                                           "/external/glm"
                                                           "/external/googletest"
                                                           "/external/logmich"
                                                           "/external/sexp-cpp"
                                                           "/external/uitest"))))
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
   (license gpl3+)))

galapix-0.3

;; EOF ;;
