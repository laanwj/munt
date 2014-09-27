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

#include "fl_lcd_display.h"

#include "font_6x8.h"

LCDDisplay::LCDDisplay(int _x, int _y, int _w, int _h, const char *_label):
    Fl_Widget(_x, _y, _w, _h, _label), surface(0),
    settle_counter(0)
{
    memset(display, 0, NUMCHARS);
    surface = cairo_image_surface_create(CAIRO_FORMAT_A8, WIDTH, HEIGHT);
}

LCDDisplay::~LCDDisplay()
{
    cairo_surface_destroy(surface);
}

inline void fade(uint8_t &d, uint8_t nv)
{
    if (d < nv)
        d = nv;
    else
        d = (d + nv) >> 1;
}

void LCDDisplay::refreshDisplay()
{
    unsigned char *data = cairo_image_surface_get_data(surface);
    int stride = cairo_image_surface_get_stride(surface);
    int xat, xstart, yat;
    xstart = 0;
    yat = 0;
    for (unsigned i = 0; i < NUMCHARS; i++) {
        uint8_t c = display[i];
        // Don't render characters we don't have mapped
        if (c < 0x20) c = 0x20;
        if (c > 0x80) c = 0x20;
        c -= 0x20;

        yat = 0;
        for (int t = 0; t < 8; t++) {
            xat = xstart;
            unsigned char fval = Font_6x8[c][t];
            for (int m = 4; m >= 0; --m) {
                if (((fval >> m) & 1) != 0) {
                    fade(data[yat*stride + xat], 255);
                    fade(data[yat*stride + xat + 1], 210);
                    fade(data[yat*stride + xat + stride], 180);
                    fade(data[yat*stride + xat + stride + 1], 150);
                } else {
                    fade(data[yat*stride + xat], 20);
                    fade(data[yat*stride + xat + 1], 20);
                    fade(data[yat*stride + xat + stride], 20);
                    fade(data[yat*stride + xat + stride + 1], 20);
                }
                xat += 2;
            }
            yat += 2;
            if (t == 6) yat += 2;
        }
        xstart += 12;
    }
    cairo_surface_mark_dirty(surface);

    redraw();
}

void LCDDisplay::draw()
{
    if (damage() & FL_DAMAGE_ALL) {
        cairo_t *cr = Fl::cairo_cc();
        cairo_save(cr);
        int _x = x(), _y = y(), _w = w(), _h = h();

        cairo_rectangle(cr, _x, _y, _w, _h);
        cairo_set_source_rgb(cr, 12/255.0, 66/255.0, 4/255.0);
        cairo_fill(cr);
        cairo_set_source_rgb(cr, 232/255.0, 254/255.0, 0/255.0);
        cairo_mask_surface(cr, surface, _x + _w/2 - WIDTH/2, _y + _h/2 - HEIGHT/2);
        cairo_restore(cr);
    }
}

void LCDDisplay::refresh_timeout(void *self_)
{
    LCDDisplay *self = static_cast<LCDDisplay*>(self_);
    self->refreshDisplay();
    if (self->settle_counter)
    {
        self->settle_counter--;
        Fl::repeat_timeout(0.05, refresh_timeout, self_);
    }
}

void LCDDisplay::setData(size_t addr, size_t size, const uint8_t *data)
{
    for (size_t ptr=0; ptr<size; ++ptr)
        if ((addr+ptr) < NUMCHARS)
            display[addr+ptr] = data[ptr];
    if (!settle_counter)
    {
        refreshDisplay();
        Fl::add_timeout(0.05, refresh_timeout, this);
    }
    settle_counter = 5;
}

