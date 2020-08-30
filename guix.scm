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
  (cons* "/ipfs/QmdrwuaArAXmKGPY3oEoNpxTp7EUSKetXAVGBt67nqo54X/guix-cocfree_0.0.0-59-gd79b2bf"
         %load-path))

(use-modules (guix build-system cmake)
             ((guix licenses) #:prefix license:)
             (guix packages)
             (gnu packages base)
             (gnu packages mcrypt)
             (gnu packages boost)
             (gnu packages curl)
             (gnu packages gcc)
             (gnu packages gl)
             (gnu packages image)
             (gnu packages imagemagick)
             (gnu packages maths)
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
   (source (source-from-source %source-dir #:version version))
   (arguments
    `(#:tests? #f))
   (build-system cmake-build-system)
   (native-inputs
    `(("pkg-config" ,pkg-config)
      ("gcc" ,gcc-10) ; for <filesystem>
      ))
   (inputs
    `(("python" ,python)
      ("sdl2" ,sdl2)
      ("mesa" ,mesa)
      ("glew" ,glew)
      ("glm" ,glm)
      ("curl" ,curl)
      ("boost" ,boost)
      ("fmt" ,fmt)
      ("libjpeg-turbo" ,libjpeg-turbo)
      ("libpng" ,libpng)
      ("sqlite" ,sqlite)
      ("libmhash" ,libmhash)
      ("imagemagick" ,imagemagick)
      ("libexif" ,libexif)))
   (synopsis (synopsis-from-source %source-dir))
   (description (description-from-source %source-dir))
   (home-page (homepage-from-source %source-dir))
   (license license:gpl3+)))

galapix-0.3

;; EOF ;;
