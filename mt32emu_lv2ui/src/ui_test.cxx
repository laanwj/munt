/* UI-only test
 * g++ -O2 ui_test.cxx fl_munt_ui.cxx fl_lcd_display.cpp -I/opt/ntk/include/ntk -pthread  `pkg-config --libs --cflags ntk` -o ui_test
 */
#include <string>

#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/filename.H>

#include "fl_munt_ui.h"

FLMuntUI *ui;

class TestUIController: public MuntUIController
{
public:
    ~TestUIController()
    {
    }
    static void progress_timeout(void *self_)
    {
        TestUIController *self = static_cast<TestUIController*>(self_);
        self->show_progress(self->message.c_str(), self->progress);
        self->progress += 0.025;
        if (self->progress < 1.0)
            Fl::add_timeout(0.05, progress_timeout, self_);
    }
    void show_progress(const char *message, float percentage)
    {
        uint8_t data[LCDDisplay::NUMCHARS];
        int n = (LCDDisplay::NUMCHARS+1) * percentage;
        if (n < 0) n = 0;
        if (n > LCDDisplay::NUMCHARS) n = LCDDisplay::NUMCHARS;
        memset(data, 0, sizeof(data));
        strncpy((char*)data, (const char*)message, LCDDisplay::NUMCHARS);
        for (unsigned u=0; u<n; ++u)
            data[u] = 128;
        ui->display->setData(0, LCDDisplay::NUMCHARS, data);
    }
    void test1()
    {
        uint8_t data[5];
        for (unsigned u=0; u<5; ++u)
            data[u] = 32 + random()%97;
        ui->display->setData(0, 5, data);
    }
    void loadSyx(const char *filename)
    {
        message = std::string("Loading ") + fl_filename_name(filename) + "...";
        progress = 0;
        progress_timeout(this);
    }

    double progress;
    std::string message;
};

int main()
{
  TestUIController *controller = new TestUIController();
  ui = new FLMuntUI(controller);
  ui->display->setData(0, 4, (const uint8_t*)"TEST");
  Fl::run();
  delete ui;
  delete controller;
  return 0;
}
