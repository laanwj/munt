/* UI-only test
 * g++ -O2 ui_test.cxx fl_munt_ui.cxx -I/opt/ntk/include/ntk -pthread  `pkg-config --libs --cflags ntk` -o ui_test
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
};

int main()
{
  TestUIController *controller = new TestUIController();
  ui = new FLMuntUI(controller);
  Fl::run();
  delete ui;
  delete controller;
  return 0;
}
