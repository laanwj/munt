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

void LCDDisplay::refreshDisplay()
{
    unsigned char *data = cairo_image_surface_get_data(surface);
    /// Fade out old contents
    for (unsigned w=0; w<WIDTH*HEIGHT; ++w)
        data[w] = data[w]/2;
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
                if (((fval >> m) & 1) != 0)
                {
                    data[yat*stride + xat] = 255;
                    data[yat*stride + xat + 1] = 210;
                    data[yat*stride + xat + stride] = 180;
                    data[yat*stride + xat + stride + 1] = 150;
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
        cairo_set_source_rgb(cr, 98/255.0, 127/255.0, 0/255.0);
        cairo_fill(cr);
        //cairo_rectangle(cr, _x, _y, _w, _h);
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
        Fl::add_timeout(0.05, refresh_timeout, self_);
    }
}

void LCDDisplay::setData(size_t addr, size_t size, const uint8_t *data)
{
    for (size_t ptr=0; ptr<size; ++ptr)
        if ((addr+ptr) < NUMCHARS)
            display[addr+ptr] = data[ptr];
    refreshDisplay();
    if (!settle_counter)
        Fl::add_timeout(0.05, refresh_timeout, this);
    settle_counter = 5;
}

