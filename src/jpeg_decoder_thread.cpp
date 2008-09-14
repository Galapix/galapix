/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2008 Ingo Ruhnke <grumbel@gmx.de>
**
**  This program is free software: you can redistribute it and/or modify
**  it under the terms of the GNU General Public License as published by
**  the Free Software Foundation, either version 3 of the License, or
**  (at your option) any later version.
**
**  This program is distributed in the hope that it will be useful,
**  but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**  GNU General Public License for more details.
**
**  You should have received a copy of the GNU General Public License
**  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "software_surface.hpp"
#include "jpeg_decoder_thread.hpp"

JPEGDecoderThread* JPEGDecoderThread::current_ = 0;

JPEGDecoderThread::JPEGDecoderThread()
  : Thread("JPEGDecoderThread"),
    quit(false)
{
  current_ = 0;
}

void
JPEGDecoderThread::request_decode(const Blob& blob,
                                  const boost::function<void (const SoftwareSurface&)>& callback)
{
  JPEGDecoderThreadMessage msg;
  msg.blob     = blob;
  msg.callback = callback;
  queue.push(msg);
}

void 
JPEGDecoderThread::stop()
{
  quit = true;
}

int
JPEGDecoderThread::run()
{
  quit = false;
  while (!quit)
    {
      while(!queue.empty())
        {
          JPEGDecoderThreadMessage msg = queue.front();
          queue.pop();
          //msg.callback(SoftwareSurface::from_jpeg_data(msg.blob));
        }
      queue.wait();
    }

  return 0;
}

/* EOF */
