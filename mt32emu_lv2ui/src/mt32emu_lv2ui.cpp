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
#include "lv2/lv2plug.in/ns/ext/atom/forge.h"
#include "lv2/lv2plug.in/ns/ext/atom/util.h"
#include "lv2/lv2plug.in/ns/ext/midi/midi.h"
#include "lv2/lv2plug.in/ns/ext/urid/urid.h"
#include "lv2/lv2plug.in/ns/ext/log/log.h"
#include "lv2/lv2plug.in/ns/lv2core/lv2.h"
#include "lv2/lv2plug.in/ns/extensions/ui/ui.h"
#include "lv2/lv2_external_ui.h"

#include <mt32emu/mt32emu.h>

#include "fl_munt_ui.h"
#include <FL/Fl_File_Chooser.H> 
#include "munt_ui_controller.h"

#define MUNT_URI "http://github.com/munt/munt"
#define MUNT_URI_UI "http://github.com/munt/munt/gui"
#define MUNT_URI_EXTERNAL_UI "http://github.com/munt/munt/ui#external"

namespace PortIndex
{
    enum PortIndex
    {
        CONTROL = 0,
        NOTIFY  = 1,
        OUT_L   = 2,
        OUT_R   = 3
    };
}

class LV2PluginUI
{
public:
    virtual ~LV2PluginUI() {}
    virtual void port_event(uint32_t port_index, uint32_t buffer_size, uint32_t format, const void* buffer) = 0;
    virtual int idle() = 0;
};
const unsigned int NUM_PARTS=9;

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
        LV2_URID midi_MidiEvent;
        LV2_URID atom_Object;
        LV2_URID atom_URID;
        LV2_URID atom_String;
        LV2_URID atom_Int;
        LV2_URID atom_eventTransfer;
        LV2_URID munt_eventType;
        // Event types
        LV2_URID munt_evt_showLCDMessage;
        LV2_URID munt_evt_onPolyStateChanged;
        LV2_URID munt_evt_onProgramChanged;
        // Event arguments
        LV2_URID munt_arg_message;
        LV2_URID munt_arg_partNum;
        LV2_URID munt_arg_bankNum;
        LV2_URID munt_arg_patchName;
        LV2_URID munt_arg_numPolys;
        LV2_URID munt_arg_numPolysNonReleasing;
    };

    /* Send MIDI message to DSP */
    void sendMidi(void *data_in, size_t size_in);

    /* Signals from NTK ui */
    void test1();
    void test2();

private:
    Features m_features;
    URIs m_uris;

    std::string m_bundlePath;
    LV2UI_Write_Function m_writeFunction;
    LV2UI_Controller m_controller;
    LV2_Atom_Forge m_forge;

    FLMuntUI *m_ui;

    int m_polyState[NUM_PARTS];

    void updateDisplay();
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
        LV2_URID_Map* map = m_features.map;
        m_uris.midi_MidiEvent = map->map(map->handle, LV2_MIDI__MidiEvent);
        m_uris.atom_Object = map->map(map->handle, LV2_ATOM__Object);
        m_uris.atom_String = map->map(map->handle, LV2_ATOM__String);
        m_uris.atom_Int = map->map(map->handle, LV2_ATOM__Int);
        m_uris.atom_URID = map->map(map->handle, LV2_ATOM__URID);
        m_uris.atom_eventTransfer = map->map(map->handle, LV2_ATOM__eventTransfer);
        m_uris.munt_eventType = map->map(map->handle, MUNT_URI"#eventType");

        m_uris.munt_evt_showLCDMessage = map->map(map->handle, MUNT_URI"#evt_showLCDMessage");
        m_uris.munt_evt_onPolyStateChanged = map->map(map->handle, MUNT_URI"#evt_onPolyStateChanged");
        m_uris.munt_evt_onProgramChanged = map->map(map->handle, MUNT_URI"#evt_onProgramChanged");
        m_uris.munt_arg_message = map->map(map->handle, MUNT_URI"#arg_message");
        m_uris.munt_arg_partNum = map->map(map->handle, MUNT_URI"#arg_partNum");
        m_uris.munt_arg_bankNum = map->map(map->handle, MUNT_URI"#arg_bankNum");
        m_uris.munt_arg_patchName = map->map(map->handle, MUNT_URI"#arg_patchName");
        m_uris.munt_arg_numPolys = map->map(map->handle, MUNT_URI"#arg_numPolys");
        m_uris.munt_arg_numPolysNonReleasing = map->map(map->handle, MUNT_URI"#arg_numPolysNonReleasing");
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

    /// forge for UI->DSP events
    lv2_atom_forge_init(&m_forge, m_features.map);

    for (unsigned i=0; i<NUM_PARTS; ++i)
        m_polyState[i] = 0;
    updateDisplay();
}

void MuntPluginUI::port_event(uint32_t port_index, uint32_t buffer_size, uint32_t format, const void* buffer)
{
    Fl::lock();
    if (format == m_uris.atom_eventTransfer) {
        LV2_Atom* atom = (LV2_Atom*)buffer;
        if (atom->type != m_uris.atom_Object) {
            fprintf(stderr, "mt32emu_lv2ui: atom->type != atom_Object\n");
            return;
        }
        LV2_Atom_Object* obj = (LV2_Atom_Object*)atom;
        const LV2_Atom_URID* eventType = NULL;
        lv2_atom_object_get(obj, m_uris.munt_eventType, &eventType, 0);
        if (eventType == NULL || eventType->atom.type != m_uris.atom_URID) {
            fprintf(stderr, "mt32emu_lv2ui: eventType missing or has invalid type\n");
            fflush(stderr);
            return;
        }
        if (eventType->body == m_uris.munt_evt_showLCDMessage) {
            const LV2_Atom_String* message = NULL;
            lv2_atom_object_get(obj, m_uris.munt_arg_message, &message, 0);
            if (!message || message->atom.type != m_uris.atom_String)
            {
                fprintf(stderr, "mt32emu_lv2ui: invalid showLCDMessage message\n");
                fflush(stderr);
                return;
            }
            fprintf(stdout, "mt32emu_lv2ui: LCD message: %s\n", (const char*)LV2_ATOM_BODY(message));
            fflush(stdout);
        } else if(eventType->body == m_uris.munt_evt_onPolyStateChanged) {
            const LV2_Atom_Int* partNum = NULL;
            const LV2_Atom_Int* numPolys = NULL;
            const LV2_Atom_Int* numPolysNonReleasing = NULL;
            lv2_atom_object_get(obj, m_uris.munt_arg_partNum, &partNum,
                                     m_uris.munt_arg_numPolys, &numPolys,
                                     m_uris.munt_arg_numPolysNonReleasing, &numPolysNonReleasing,
                                     0);
            if (!partNum || !numPolys || !numPolysNonReleasing ||
                 partNum->atom.type != m_uris.atom_Int || numPolys->atom.type != m_uris.atom_Int || numPolysNonReleasing->atom.type != m_uris.atom_Int ||
                 partNum->body < 0 || partNum->body >= int(NUM_PARTS))
            {
                fprintf(stderr, "mt32emu_lv2ui: invalid onProgramChanged message\n");
                fflush(stderr);
                return;
            }
            //fprintf(stdout, "mt32emu_lv2ui: poly state change %i %i %i\n",
            //        partNum->body, numPolys->body, numPolysNonReleasing->body);
            m_polyState[partNum->body] = numPolysNonReleasing->body;
            updateDisplay();
        } else if(eventType->body == m_uris.munt_evt_onProgramChanged) {
            const LV2_Atom_Int* partNum = NULL;
            const LV2_Atom_Int* bankNum = NULL;
            const LV2_Atom_String* patchName = NULL;
            lv2_atom_object_get(obj, m_uris.munt_arg_partNum, &partNum,
                                     m_uris.munt_arg_bankNum, &bankNum,
                                     m_uris.munt_arg_patchName, &patchName,
                                     0);
            if (!partNum || !bankNum || !patchName ||
                 partNum->atom.type != m_uris.atom_Int || bankNum->atom.type != m_uris.atom_Int || patchName->atom.type != m_uris.atom_String)
            {
                fprintf(stderr, "mt32emu_lv2ui: invalid onProgramChanged message\n");
                fflush(stderr);
                return;
            }
            fprintf(stdout, "mt32emu_lv2ui: program change %i %i %s\n", partNum->body, bankNum->body, (const char*)LV2_ATOM_BODY(patchName));
        } else {
            fprintf(stderr, "mt32emu_lv2ui: unknown UI notification eventType=%i\n", eventType->body);
        }
    }
    Fl::unlock();
    Fl::awake();
}

// MT32Emu::Synth::calcSysexChecksum(const Bit8u *data, Bit32u len, Bit8u checksum)
void MuntPluginUI::test1()
{
    printf("test1...\n");
    uint8_t obj_buf[1024];
    lv2_atom_forge_set_buffer(&m_forge, obj_buf, 1024);
    LV2_Atom_Forge_Frame set_frame;
    LV2_Atom* set = (LV2_Atom*)lv2_atom_forge_object(&m_forge, &set_frame, 0, m_uris.atom_eventTransfer);

    lv2_atom_forge_pop(&m_forge, &set_frame);
    m_writeFunction(m_controller, PortIndex::CONTROL, lv2_atom_total_size(set), m_uris.atom_eventTransfer, set);
}

void MuntPluginUI::sendMidi(void *data_in, size_t size_in)
{
    if(sizeof(LV2_Atom) + size_in >= 1024)
    {
        printf("mt32emu_lv2ui: Sending MIDI event too large (%i)\n", (int)size_in);
        return;
    }
    uint8_t obj_buf[1024];
    LV2_Atom* hdr = (LV2_Atom*)obj_buf;
    uint8_t* data = (uint8_t *)LV2_ATOM_BODY(hdr);
    hdr->type = m_uris.midi_MidiEvent;
    hdr->size = size_in;
    memcpy(data, data_in, size_in);
    //data[0] = 0x91;
    //data[1] = 60;
    //data[2] = 127;
    size_t padded_size = lv2_atom_pad_size(lv2_atom_total_size(hdr));
    m_writeFunction(m_controller, PortIndex::CONTROL, padded_size, m_uris.atom_eventTransfer, hdr);
    usleep(padded_size*100);
}

const size_t SYX_BUFFER_SIZE = 64*1024;
void MuntPluginUI::test2()
{
    char *filename = fl_file_chooser("Select .syx file to load", "*.syx", NULL, 1);
    if (!filename)
        return;
    FILE *f = fopen(filename, "rb");
    if (!f)
    {
        printf("Unable to open file %s\n", filename);
        return;
    }
    uint8_t buffer[SYX_BUFFER_SIZE];
    size_t nread = fread(buffer, 1, SYX_BUFFER_SIZE, f);
    fclose(f);
    printf("Read %i bytes from syx file\n", (int)nread);
    // 0xf0..0xf7
    // Read events
    size_t offset = 0;
    size_t start = 0;
    while (offset < nread) {
        if (buffer[offset] == 0xf0)
            start = offset;
        if (buffer[offset] == 0xf7 && start != 0)
        {
            printf("  Event start=%u size=%u\n", (unsigned)start, (unsigned)(offset-start+1));
            sendMidi(&buffer[start], offset-start+1);
        }
        ++offset;
    }

}

void MuntPluginUI::updateDisplay()
{
    uint8_t display[LCDDisplay::NUMCHARS] = {0};
    for (unsigned int i=0; i<9; ++i)
    {
        if (m_polyState[i])
            display[i*2] = 128;
        else
            display[i*2] = i==8 ? 'R' : ('1' + i);
    }
    m_ui->display->setData(0, LCDDisplay::NUMCHARS, display);
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
