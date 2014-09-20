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
