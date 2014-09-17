/* UI-only test
 * g++ -O2 ui_test.cxx fl_munt_ui.cxx fl_lcd_display.cpp -I/opt/ntk/include/ntk -pthread  `pkg-config --libs --cflags ntk` -o ui_test
 */

#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>

#include "fl_munt_ui.h"

FLMuntUI *ui;

class TestUIController: public MuntUIController
{
public:
    ~TestUIController()
    {
    }
    void test1()
    {
        ui->display->setData(0, 5, (const uint8_t*)"BOOH\x80");
    }
    void test2()
    {
        ui->display->setData(0, 5, (const uint8_t*)"BLUB ");
    }
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
