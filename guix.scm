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
  (cons* "/ipfs/QmdHSWX34MXZeAgMuDuFwrYzSPf4fknX7E7cYJ7HDxaLrZ/guix-cocfree_0.0.0-62-g3b27118"
         %load-path))

(use-modules (guix build-system cmake)
             ((guix licenses) #:prefix license:)
             (guix packages)
             (gnu packages base)
             (gnu packages benchmark)
             (gnu packages mcrypt)
             (gnu packages check)
             (gnu packages curl)
             (gnu packages gcc)
             (gnu packages gl)
             (gnu packages glib)
             (gnu packages gtk)
             (gnu packages image)
             (gnu packages imagemagick)
             (gnu packages maths)
             (gnu packages pkg-config)
             (gnu packages photo)
             (gnu packages python)
             (gnu packages pretty-print)
             (gnu packages sdl)
             (gnu packages sqlite)
             (gnu packages serialization)
             (gnu packages tls)
             (guix-cocfree utils))

(define %source-dir (dirname (current-filename)))

(define-public galapix-0.3
  (package
   (name "galapix-0.3")
   (version (version-from-source %source-dir))
   (source (source-from-source %source-dir #:version version))
   (arguments
    `(#:tests? #f ; no network
      #:configure-flags '("-DCMAKE_BUILD_TYPE=Release"
                          "-DBUILD_GALAPIX_SDL=ON"
                          "-DBUILD_GALAPIX_GTK=ON"
                          "-DBUILD_TESTS=ON"
                          "-DBUILD_BENCHMARKS=ON"
                          "-DBUILD_EXTRAS=ON")))
   (build-system cmake-build-system)
   (native-inputs
    `(("pkg-config" ,pkg-config)
      ("gcc" ,gcc-10) ; for <filesystem>
      ))
   (inputs
    `(("benchmark" ,benchmark)
      ("curl" ,curl)
      ("fmt" ,fmt)
      ("glew" ,glew)
      ("glm" ,glm)
      ("googletest" ,googletest)
      ("gtkmm" ,gtkmm)
      ("imagemagick" ,imagemagick)
      ("jsoncpp" ,jsoncpp)
      ("libexif" ,libexif)
      ("libjpeg-turbo" ,libjpeg-turbo)
      ("libpng" ,libpng)
      ("libsigc++" ,libsigc++)
      ("mesa" ,mesa)
      ("openssl" ,openssl)
      ("python" ,python)
      ("sdl2" ,sdl2)
      ("sqlite" ,sqlite)))
   (synopsis (synopsis-from-source %source-dir))
   (description (description-from-source %source-dir))
   (home-page (homepage-from-source %source-dir))
   (license license:gpl3+)))

galapix-0.3

;; EOF ;;
