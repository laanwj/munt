/* Copyright (C) 2014 Wladimir J. van der Laan
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation, either version 2.1 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef FL_LCD_DISPLAY_H
#define FL_LCD_DISPLAY_H

#include <FL/Fl.H>
#include <FL/Fl_Widget.H>
#include <stdint.h>

/*** Simple LCD character display with fade */
class LCDDisplay: public Fl_Widget
{
public:
    static const size_t NUMCHARS = 20;

    LCDDisplay(int _x, int _y, int _w, int _h, const char *_label=0 );
    ~LCDDisplay();

    void draw();
    void setData(size_t addr, size_t size, const uint8_t *data);
private:
    static const unsigned int CHAR_WIDTH = 12;
    static const unsigned int CHAR_HEIGHT = 18;
    static const unsigned int WIDTH = 12 * NUMCHARS - 2;
    static const unsigned int HEIGHT = 18;
    uint8_t display[NUMCHARS];
    cairo_surface_t *surface;
    int settle_counter;

    void refreshDisplay();
    static void refresh_timeout(void *);
};

#endif // FL_LCD_DISPLAY_H
