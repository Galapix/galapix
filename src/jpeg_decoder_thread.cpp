/*  $Id$
**   __      __ __             ___        __   __ __   __
**  /  \    /  \__| ____    __| _/_______/  |_|__|  | |  |   ____
**  \   \/\/   /  |/    \  / __ |/  ___/\   __\  |  | |  | _/ __ \
**   \        /|  |   |  \/ /_/ |\___ \  |  | |  |  |_|  |_\  ___/
**    \__/\  / |__|___|  /\____ /____  > |__| |__|____/____/\___  >
**         \/          \/      \/    \/                         \/
**  Copyright (C) 2007 Ingo Ruhnke <grumbel@gmx.de>
**
**  This program is free software; you can redistribute it and/or
**  modify it under the terms of the GNU General Public License
**  as published by the Free Software Foundation; either version 2
**  of the License, or (at your option) any later version.
**
**  This program is distributed in the hope that it will be useful,
**  but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**  GNU General Public License for more details.
** 
**  You should have received a copy of the GNU General Public License
**  along with this program; if not, write to the Free Software
**  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
**  02111-1307, USA.
*/

#include "software_surface.hpp"
#include "jpeg_decoder_thread.hpp"

JPEGDecoderThread* JPEGDecoderThread::current_ = 0;

JPEGDecoderThread::JPEGDecoderThread()
  : quit(false)
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
          msg.callback(SoftwareSurface::from_data(msg.blob));
        }
      queue.wait();
    }

  return 0;
}

/* EOF */
