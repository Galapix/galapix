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
  (cons* "/ipfs/QmZdLjyRm29uL4Eh4HqkZHvwMMus6zjwQ8EdBtp5JUPT99/guix-cocfree_0.0.0-52-ga8e1798"
         %load-path))

(use-modules (guix build-system scons)
             ((guix licenses) #:prefix license:)
             (guix packages)
             (gnu packages base)
             (gnu packages mcrypt)
             (gnu packages boost)
             (gnu packages compression)
             (gnu packages curl)
             (gnu packages gcc)
             (gnu packages gl)
             (gnu packages imagemagick)
             (gnu packages pkg-config)
             (gnu packages photo)
             (gnu packages sdl)
             (gnu packages sqlite)
             (guix-cocfree utils))

(define %source-dir (dirname (current-filename)))

(define-public galapix-0.2
  (package
   (name "galapix")
   (version (version-from-source %source-dir))
   (source  (source-from-source %source-dir))
   (arguments
    `(#:tests? #f
      #:scons-flags (list "GALAPIX_GTK=False")
      #:phases (modify-phases
                %standard-phases
                (add-before 'build 'fixgcc9
                            (lambda _
                              (unsetenv "C_INCLUDE_PATH")
                              (unsetenv "CPLUS_INCLUDE_PATH")
                              #t))
                (replace 'install
                         (lambda* (#:key outputs #:allow-other-keys)
                           (let* ((out (assoc-ref outputs "out"))
                                  (bindir (string-append out "/bin")))
                             (mkdir-p bindir)
                             (copy-file "build/galapix.sdl" (string-append bindir "/galapix.sdl")))
                           #t))
                )))
   (build-system scons-build-system)
   (native-inputs
    `(("pkg-config" ,pkg-config)))
   (inputs
    `(("gcc" ,gcc-9)
      ("sdl2" ,sdl2)
      ("sdl2-image" ,sdl2-image)
      ("mesa" ,mesa)
      ("glew" ,glew)
      ("curl" ,curl)
      ("boost" ,boost)
      ("boost-signals2" ,boost-signals2)
      ("sqlite" ,sqlite)
      ("libmhash" ,libmhash)
      ("imagemagick" ,imagemagick)
      ("libexif" ,libexif)
      ))
   (propagated-inputs
    `(("p7zip" ,p7zip)
      ("tar" ,tar)
      ("unzip" ,unzip)))
   (synopsis "An image viewer for large image collections")
   (description "An image viewer for large image collections")
   (home-page "https://gitlab.com/galapix/galapix")
   (license license:gpl3+)))

galapix-0.2

;; EOF ;;
