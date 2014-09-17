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

#include <assert.h>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <signal.h>
#include <string>
#include <unistd.h>

#include "lv2/lv2plug.in/ns/ext/atom/atom.h"
#include "lv2/lv2plug.in/ns/ext/atom/util.h"
#include "lv2/lv2plug.in/ns/ext/midi/midi.h"
#include "lv2/lv2plug.in/ns/ext/urid/urid.h"
#include "lv2/lv2plug.in/ns/ext/log/log.h"
#include "lv2/lv2plug.in/ns/lv2core/lv2.h"
#include "lv2/lv2plug.in/ns/extensions/ui/ui.h"
#include "lv2/lv2_external_ui.h"

#include <mt32emu/mt32emu.h>

#include "fl_munt_ui.h"
#include "munt_ui_controller.h"

#define MUNT_URI "http://github.com/munt/munt"
#define MUNT_URI_UI "http://github.com/munt/munt/gui"
#define MUNT_URI_EXTERNAL_UI "http://github.com/munt/munt/ui#external"

class LV2PluginUI
{
public:
    virtual ~LV2PluginUI() {}
    virtual void port_event(uint32_t port_index, uint32_t buffer_size, uint32_t format, const void* buffer) = 0;
    virtual int idle() = 0;
};

class MuntPluginUI: public LV2PluginUI, public MuntUIController
{
public:
    MuntPluginUI(const char* bundle_path, LV2UI_Write_Function write_function,
               LV2UI_Controller controller, LV2UI_Widget* widget, const LV2_Feature* const* features);
    ~MuntPluginUI();

    void port_event(uint32_t port_index, uint32_t buffer_size, uint32_t format, const void* buffer);
    int idle();

    struct Features
    {
        LV2_URID_Map* map;
    };
    struct URIs
    {
        //LV2_URID midi_MidiEvent;
    };
private:
    Features m_features;
    URIs m_uris;

    std::string m_bundlePath;
    LV2UI_Write_Function m_writeFunction;
    LV2UI_Controller m_controller;

    FLMuntUI *m_ui;
};

MuntPluginUI::MuntPluginUI(const char* bundle_path, LV2UI_Write_Function write_function,
               LV2UI_Controller controller, LV2UI_Widget* widget, const LV2_Feature* const* features):
    m_ui(0)
{
    LV2UI_Widget parentWindow = 0;
    LV2UI_Resize *resize = 0;
    memset(&m_features, 0, sizeof(m_features));
    memset(&m_uris, 0, sizeof(m_uris));

    for (int i = 0; features[i]; ++i)
    {
        if (!strcmp(features[i]->URI, LV2_URID__map))
            m_features.map = (LV2_URID_Map*)features[i]->data;
        if (!strcmp(features[i]->URI, LV2_UI__parent))
            parentWindow = features[i]->data;
        if (!strcmp(features[i]->URI, LV2_UI__resize))
            resize = (LV2UI_Resize*)features[i]->data;
    }

    if (m_features.map)
    {
        //m_uris.midi_MidiEvent = m_features.map->map(m_features.map->handle, LV2_MIDI__MidiEvent);
    }

    m_bundlePath = bundle_path;
    m_writeFunction = write_function;
    m_controller = controller;

    if (!parentWindow)
        printf("Warning: no parent window given\n");

    m_ui = new FLMuntUI(this, (void*)parentWindow);
    *widget = (LV2UI_Widget)fl_xid(m_ui->w);

    if (resize)
        resize->ui_resize(resize->handle, m_ui->w->w(), m_ui->w->h());
    else
        printf("Warning: Host doesn't support resize extension.");
}

void MuntPluginUI::port_event(uint32_t port_index, uint32_t buffer_size, uint32_t format, const void* buffer)
{
}

int MuntPluginUI::idle()
{
    Fl::check();
    Fl::flush();
    return 0;
}

MuntPluginUI::~MuntPluginUI()
{
    delete m_ui;
}

//////////////////////////////////////////
// LV2UI plugin C++ wrapper

static LV2UI_Handle instantiate(const struct _LV2UI_Descriptor* descriptor,
                            const char*                     plugin_uri,
                            const char*                     bundle_path,
                            LV2UI_Write_Function            write_function,
                            LV2UI_Controller                controller,
                            LV2UI_Widget*                   widget,
                            const LV2_Feature* const*       features)
{
    if (!strcmp(descriptor->URI, MUNT_URI_UI) && !strcmp(plugin_uri, MUNT_URI))
        return static_cast<LV2_Handle>(
            new MuntPluginUI(bundle_path, write_function, controller, widget, features));
    return NULL;
}

static void
cleanup(LV2UI_Handle ui)
{
    delete static_cast<LV2PluginUI*>(ui);
}

static void
port_event(LV2UI_Handle ui,
           uint32_t     port_index,
           uint32_t     buffer_size,
           uint32_t     format,
           const void*  buffer)
{
    static_cast<LV2PluginUI*>(ui)->port_event(port_index, buffer_size, format, buffer);
}

static int
idle(LV2UI_Handle ui)
{
    return static_cast<LV2PluginUI*>(ui)->idle();
}

static const LV2UI_Idle_Interface idle_interface = {
    idle
};

static const void*
external_extension_data(const char* uri)
{
    if (strcmp(uri, LV2_UI__idleInterface) == 0)
        return (void *) &idle_interface;
    return NULL;
}

static const LV2UI_Descriptor descriptor = {
    MUNT_URI_UI,
    instantiate,
    cleanup,
    port_event,
    external_extension_data
};

LV2_SYMBOL_EXPORT
__attribute__ ((visibility ("default")))
const LV2UI_Descriptor *lv2ui_descriptor ( uint32_t index )
{
    switch (index)
    {
    case 0: return &descriptor;
    default: return NULL;
    }
}
