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
#include <sstream>
#include <unistd.h>
#include <vector>
#include <limits>
#include <set>

#include "lv2/lv2plug.in/ns/ext/atom/atom.h"
#include "lv2/lv2plug.in/ns/ext/atom/forge.h"
#include "lv2/lv2plug.in/ns/ext/atom/util.h"
#include "lv2/lv2plug.in/ns/ext/midi/midi.h"
#include "lv2/lv2plug.in/ns/ext/urid/urid.h"
#include "lv2/lv2plug.in/ns/ext/log/log.h"
#include "lv2/lv2plug.in/ns/lv2core/lv2.h"
#include "lv2/lv2plug.in/ns/extensions/ui/ui.h"

#include <mt32emu/mt32emu.h>

#include "fl_munt_ui.h"
#include <FL/Fl_File_Chooser.H>
#include "munt_ui_controller.h"
#include "mt32emu_lv2_common.h"

/** MT-32 constants for GUI */
const unsigned int NUM_PARTS = 9;
static const unsigned int NUM_BANKS = 4;
static const char* BANK_NAMES[] = {"Synth-1", "Synth-2", "Memory ", "Rhythm "};

/** Convert an LV2_Atom_String to a std::string */
inline std::string atomToString(const LV2_Atom_String* message)
{
    return std::string((const char*)LV2_ATOM_BODY(message), message->atom.size);
}

class MuntPluginUI: public MuntUIController
{
public:
    static const char *URI;
    static LV2UI_Handle instantiate(const struct _LV2UI_Descriptor* descriptor,
        const char* plugin_uri, const char* bundle_path, LV2UI_Write_Function write_function,
        LV2UI_Controller controller, LV2UI_Widget* widget, const LV2_Feature* const* features);
    ~MuntPluginUI();

    /* Implementations of LV2 functions */
    void port_event(uint32_t port_index, uint32_t buffer_size, uint32_t format, const void* buffer);
    int idle();

    /* Send MIDI message to DSP */
    size_t sendMidi(void *data_in, size_t size_in);

    /* Signals from NTK ui */
    void resetSynth();
    void loadSyx(const char *filename);

    struct Features
    {
        Features(const LV2_Feature* const* features);
        /** Return true if all required features are present */
        bool validate();

        LV2_URID_Map* map;
        LV2UI_Widget parentWindow;
        LV2UI_Resize* resize;
    };
    struct URIs
    {
        URIs(LV2_URID_Map* map);

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
        LV2_URID munt_evt_onDeviceReset;
        LV2_URID munt_evt_onSysExReceived;
        LV2_URID munt_cmd_resetSynth;
        // Event arguments
        LV2_URID munt_arg_message;
        LV2_URID munt_arg_partNum;
        LV2_URID munt_arg_bankNum;
        LV2_URID munt_arg_patchName;
        LV2_URID munt_arg_numPolys;
        LV2_URID munt_arg_numPolysNonReleasing;
        LV2_URID munt_arg_addr;
        LV2_URID munt_arg_len;
    };
private:
    MuntPluginUI(const char* bundle_path, LV2UI_Write_Function write_function,
               LV2UI_Controller controller, LV2UI_Widget* widget, const Features &features);

    Features m_features;
    URIs m_uris;

    std::string m_bundlePath;
    LV2UI_Write_Function m_writeFunction;
    LV2UI_Controller m_controller;
    LV2_Atom_Forge m_forge;

    FLMuntUI *m_ui;

    /** Display handling */
    int m_polyState[NUM_PARTS];
    /** Display state: this is in reverse order of priority,
     * lower states override higher ones.
     */
    enum DisplayState: uint8_t {
        RESET = 0,
        PROGRESS,
        MESSAGE,
        PROGRAM_CHANGE,
        PARTS,
        NumStates
    };
    /** Bitfield of active states */
    uint32_t m_displayStates;
    double m_displayProgress;
    struct DisplayStateData
    {
        DisplayState id;
        MuntPluginUI *parent;
        std::string displayMessage;
    } m_displayStatesData[DisplayState::NumStates];

    /** Initialize display data */
    void initDisplay();
    /** Main display drawing/update function */
    void updateDisplay();
    /** Display a message on the display for a certain time */
    void displayMessage(DisplayState newState, const std::string &newMessage, double timeout);
    /** Display timeout function: revert to PARTS state */
    static void displayTimeout(void *self);

    /** Syx loading.
     * We need to distribute loading over a longer time to avoid filling up the hosts queue
     * and dropping events...
     */
    std::vector<uint8_t> m_syxData;
    size_t m_syxOffset;
    size_t m_syxStart;
    /** Keep track of which events have been sent to the DSP, this is necessary
     * because some hosts may drop events and want to warn of this.
     */
    std::set<std::pair<uint32_t,uint32_t> > m_syxSent; // [addr,len]
    /** Send queued syx events to DSP.
     * Return true when finished, false if more events have to be processed.
     */
    bool syxProcessEvents(size_t numBytes);
    /** Check whether all syx events have been received.
     */
    static void checkSyxFinished(void *self);
};

const char *MuntPluginUI::URI = MUNT_URI_UI;

MuntPluginUI::URIs::URIs(LV2_URID_Map* map)
{
    memset(this, 0, sizeof(*this));

    midi_MidiEvent = map->map(map->handle, LV2_MIDI__MidiEvent);
    atom_Object = map->map(map->handle, LV2_ATOM__Object);
    atom_String = map->map(map->handle, LV2_ATOM__String);
    atom_Int = map->map(map->handle, LV2_ATOM__Int);
    atom_URID = map->map(map->handle, LV2_ATOM__URID);
    atom_eventTransfer = map->map(map->handle, LV2_ATOM__eventTransfer);
    munt_eventType = map->map(map->handle, MUNT_URI__eventType);

    munt_evt_showLCDMessage = map->map(map->handle, MUNT_URI__evt_showLCDMessage);
    munt_evt_onPolyStateChanged = map->map(map->handle, MUNT_URI__evt_onPolyStateChanged);
    munt_evt_onProgramChanged = map->map(map->handle, MUNT_URI__evt_onProgramChanged);
    munt_evt_onDeviceReset = map->map(map->handle, MUNT_URI__evt_onDeviceReset);
    munt_evt_onSysExReceived = map->map(map->handle, MUNT_URI__evt_onSysExReceived);
    munt_cmd_resetSynth = map->map(map->handle, MUNT_URI__cmd_resetSynth);
    munt_arg_message = map->map(map->handle, MUNT_URI__arg_message);
    munt_arg_partNum = map->map(map->handle, MUNT_URI__arg_partNum);
    munt_arg_bankNum = map->map(map->handle, MUNT_URI__arg_bankNum);
    munt_arg_patchName = map->map(map->handle, MUNT_URI__arg_patchName);
    munt_arg_numPolys = map->map(map->handle, MUNT_URI__arg_numPolys);
    munt_arg_numPolysNonReleasing = map->map(map->handle, MUNT_URI__arg_numPolysNonReleasing);
    munt_arg_addr = map->map(map->handle, MUNT_URI__arg_addr);
    munt_arg_len = map->map(map->handle, MUNT_URI__arg_len);
}

MuntPluginUI::Features::Features(const LV2_Feature* const* features)
{
    memset(this, 0, sizeof(*this));

    for (int i = 0; features[i]; ++i) {
        if (!strcmp(features[i]->URI, LV2_URID__map))
            map = (LV2_URID_Map*)features[i]->data;
        if (!strcmp(features[i]->URI, LV2_UI__parent))
            parentWindow = (LV2UI_Widget)features[i]->data;
        if (!strcmp(features[i]->URI, LV2_UI__resize))
            resize = (LV2UI_Resize*)features[i]->data;
    }
}

bool MuntPluginUI::Features::validate()
{
    return map != 0;
}

LV2UI_Handle MuntPluginUI::instantiate(const struct _LV2UI_Descriptor* descriptor,
    const char* plugin_uri, const char* bundle_path, LV2UI_Write_Function write_function,
    LV2UI_Controller controller, LV2UI_Widget* widget, const LV2_Feature* const* features)
{
    if (strcmp(descriptor->URI, MUNT_URI_UI) != 0 || strcmp(plugin_uri, MUNT_URI) != 0)
        return nullptr;
    Features l_features(features);
    if (!l_features.validate())
    {
        fprintf(stderr, "mt32emu_lv2ui: required features are missing\n");
        return nullptr;
    }
    return static_cast<LV2_Handle>(new MuntPluginUI(bundle_path, write_function, controller, widget, l_features));
}

MuntPluginUI::MuntPluginUI(const char* bundle_path, LV2UI_Write_Function write_function,
               LV2UI_Controller controller, LV2UI_Widget* widget, const Features &features):
    m_features(features), m_uris(features.map), m_ui(0), m_displayStates(1<<DisplayState::PARTS)
{
    m_bundlePath = bundle_path;
    m_writeFunction = write_function;
    m_controller = controller;

    if (!m_features.parentWindow)
        printf("Warning: no parent window given\n");

    m_ui = new FLMuntUI(this, (void*)m_features.parentWindow);
    *widget = (LV2UI_Widget)fl_xid(m_ui->w);

    if (m_features.resize)
        m_features.resize->ui_resize(m_features.resize->handle, m_ui->w->w(), m_ui->w->h());
    else
        printf("Warning: Host doesn't support resize extension, window may have malformed size.");

    /// forge for UI->DSP events
    lv2_atom_forge_init(&m_forge, m_features.map);

    for (unsigned int i=0; i<NUM_PARTS; ++i)
        m_polyState[i] = 0;
    initDisplay();
}

void MuntPluginUI::port_event(uint32_t port_index, uint32_t buffer_size, uint32_t format, const void* buffer)
{
    UNUSED(buffer_size);
    UNUSED(port_index);
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
            if (!message || message->atom.type != m_uris.atom_String) {
                fprintf(stderr, "mt32emu_lv2ui: invalid showLCDMessage message\n");
                fflush(stderr);
                return;
            }
            displayMessage(DisplayState::MESSAGE, atomToString(message), 3.0);
        } else if (eventType->body == m_uris.munt_evt_onPolyStateChanged) {
            const LV2_Atom_Int* partNum = NULL;
            const LV2_Atom_Int* numPolys = NULL;
            const LV2_Atom_Int* numPolysNonReleasing = NULL;
            lv2_atom_object_get(obj, m_uris.munt_arg_partNum, &partNum,
                                     m_uris.munt_arg_numPolys, &numPolys,
                                     m_uris.munt_arg_numPolysNonReleasing, &numPolysNonReleasing,
                                     0);
            if (!partNum || !numPolys || !numPolysNonReleasing ||
                 partNum->atom.type != m_uris.atom_Int || numPolys->atom.type != m_uris.atom_Int || numPolysNonReleasing->atom.type != m_uris.atom_Int ||
                 partNum->body < 0 || partNum->body >= int(NUM_PARTS)) {
                fprintf(stderr, "mt32emu_lv2ui: invalid onProgramChanged message\n");
                fflush(stderr);
                return;
            }
            //fprintf(stdout, "mt32emu_lv2ui: poly state change %i %i %i\n",
            //        partNum->body, numPolys->body, numPolysNonReleasing->body);
            m_polyState[partNum->body] = numPolysNonReleasing->body;
            updateDisplay();
        } else if (eventType->body == m_uris.munt_evt_onProgramChanged) {
            const LV2_Atom_Int* partNum = NULL;
            const LV2_Atom_Int* bankNum = NULL;
            const LV2_Atom_String* patchName = NULL;
            lv2_atom_object_get(obj, m_uris.munt_arg_partNum, &partNum,
                                     m_uris.munt_arg_bankNum, &bankNum,
                                     m_uris.munt_arg_patchName, &patchName,
                                     0);
            if (!partNum || !bankNum || !patchName ||
                 partNum->atom.type != m_uris.atom_Int || bankNum->atom.type != m_uris.atom_Int || patchName->atom.type != m_uris.atom_String ||
                 partNum->body < 0 || partNum->body >= int(NUM_PARTS) ||
                 bankNum->body < 0 || bankNum->body >= int(NUM_BANKS)) {
                fprintf(stderr, "mt32emu_lv2ui: invalid onProgramChanged message\n");
                fflush(stderr);
                return;
            }
            // fprintf(stdout, "mt32emu_lv2ui: program change %i %i %s\n", partNum->body, bankNum->body, (const char*)LV2_ATOM_BODY(patchName));

            std::stringstream s;
            s << partNum->body + 1 << "|" << BANK_NAMES[bankNum->body] << "|" << atomToString(patchName);
            displayMessage(DisplayState::PROGRAM_CHANGE, s.str(), 1.0);
        } else if (eventType->body == m_uris.munt_evt_onDeviceReset) {
            displayMessage(DisplayState::RESET, "", 0.25);
        } else if (eventType->body == m_uris.munt_evt_onSysExReceived) {
            const LV2_Atom_Int* addr = NULL;
            const LV2_Atom_Int* len = NULL;
            lv2_atom_object_get(obj, m_uris.munt_arg_addr, &addr,
                                     m_uris.munt_arg_len, &len,
                                     0);
            if (!addr || !len ||
                 addr->atom.type != m_uris.atom_Int || len->atom.type != m_uris.atom_Int) {
                fprintf(stderr, "mt32emu_lv2ui: invalid onSysExReceived message\n");
                fflush(stderr);
                return;
            }
            // Mark <addr,len> as done
            m_syxSent.erase(std::make_pair(addr->body, len->body));
        } else {
            fprintf(stderr, "mt32emu_lv2ui: unknown UI notification eventType=%i\n", eventType->body);
        }
    }
    Fl::unlock();
    Fl::awake();
}

// MT32Emu::Synth::calcSysexChecksum(const Bit8u *data, Bit32u len, Bit8u checksum)
void MuntPluginUI::resetSynth()
{
    uint8_t obj_buf[1024];
    lv2_atom_forge_set_buffer(&m_forge, obj_buf, 1024);
    LV2_Atom_Forge_Frame set_frame;
    LV2_Atom* set = (LV2_Atom*)lv2_atom_forge_object(&m_forge, &set_frame, 0, m_uris.atom_eventTransfer);
    lv2_atom_forge_key(&m_forge, m_uris.munt_eventType);
    lv2_atom_forge_urid(&m_forge, m_uris.munt_cmd_resetSynth);
    lv2_atom_forge_pop(&m_forge, &set_frame);
    m_writeFunction(m_controller, PortIndex::CONTROL, m_forge.offset, m_uris.atom_eventTransfer, set);
}

size_t MuntPluginUI::sendMidi(void *data_in, size_t size_in)
{
    if (sizeof(LV2_Atom) + size_in >= 1024) {
        fprintf(stdout, "mt32emu_lv2ui: Sending MIDI event too large (%i)\n", (int)size_in);
        return 0;
    }
    uint8_t obj_buf[1024];
    lv2_atom_forge_set_buffer(&m_forge, obj_buf, 1024);
    LV2_Atom *atom = (LV2_Atom*)lv2_atom_forge_atom(&m_forge, size_in, m_uris.midi_MidiEvent);
    lv2_atom_forge_write(&m_forge, data_in, size_in);
    m_writeFunction(m_controller, PortIndex::CONTROL, m_forge.offset, m_uris.atom_eventTransfer, atom);
    return m_forge.offset;
}

const size_t SYX_BUFFER_SIZE = 64*1024;
void MuntPluginUI::loadSyx(const char *filename)
{
    if (!m_syxData.empty()) // Still loading a previous syx
        return;
    FILE *f = fopen(filename, "rb");
    if (!f) {
        printf("Unable to open file %s\n", filename);
        return;
    }
    uint8_t buffer[SYX_BUFFER_SIZE];
    size_t nread = fread(buffer, 1, SYX_BUFFER_SIZE, f);
    fclose(f);
    printf("Read %i bytes from syx file\n", (int)nread);
    m_syxData = std::vector<uint8_t>(buffer, buffer+nread);
    m_syxOffset = 0;
    m_syxStart = std::numeric_limits<size_t>::max();
    m_syxSent.clear();

    m_displayProgress = 0.0;
    displayMessage(DisplayState::PROGRESS, std::string("Loading ") + fl_filename_name(filename) + "...", 0.0);
}

bool MuntPluginUI::syxProcessEvents(size_t numBytes)
{
    // Read and process SysEx events
    size_t numSent = 0;
    // printf("Processevents... %u %u\n", (unsigned)m_syxOffset, (unsigned)end);
    while (m_syxOffset < m_syxData.size() && numSent < numBytes) {
        if (m_syxData[m_syxOffset] == 0xf0)
            m_syxStart = m_syxOffset;
        if (m_syxData[m_syxOffset] == 0xf7 && m_syxStart != std::numeric_limits<size_t>::max()) {
            m_displayProgress = (double)m_syxOffset / m_syxData.size();
            updateDisplay();
            numSent += sendMidi(&m_syxData[m_syxStart], m_syxOffset-m_syxStart+1);
            // Mark (addr,len) of event as sent
            uint32_t addr, len;
            if (getSysExInfo(&m_syxData[m_syxStart], m_syxOffset-m_syxStart+1, &addr, &len))
                m_syxSent.insert(std::make_pair(addr, len));
        }
        ++m_syxOffset;
    }
    if (m_syxOffset == m_syxData.size()) {
        m_displayStates &= ~(1 << DisplayState::PROGRESS);
        updateDisplay();
        m_syxData.clear();
        // wait a bit, then if m_syxSent is not empty, signal transfer error
        Fl::add_timeout(0.25, checkSyxFinished, this);
        return true;
    } else {
        return false;
    }
}

void MuntPluginUI::checkSyxFinished(void *self_)
{
    MuntPluginUI *self = static_cast<MuntPluginUI*>(self_);
    if (!self->m_syxSent.empty())
    {
        fl_alert("There was a problem transferring the syx data: %u events were lost",
                (unsigned int)self->m_syxSent.size());
    }
}

void MuntPluginUI::initDisplay()
{
    for (unsigned int i=0; i<DisplayState::NumStates; ++i) {
        m_displayStatesData[i].id = DisplayState(i);
        m_displayStatesData[i].parent = this;
    }
    updateDisplay();
}

void MuntPluginUI::displayMessage(DisplayState newState, const std::string &newMessage, double timeout)
{
    DisplayStateData &stateData = m_displayStatesData[newState];
    stateData.displayMessage = newMessage;
    if (m_displayStates & (1 << newState))
        Fl::remove_timeout(displayTimeout, &m_displayStatesData[newState]);
    m_displayStates |= (1 << newState);
    updateDisplay();
    if (timeout != 0.0)
        Fl::add_timeout(timeout, displayTimeout, &m_displayStatesData[newState]);
}

void MuntPluginUI::updateDisplay()
{
    uint8_t display[LCDDisplay::NUMCHARS] = {0};
    int n;
    int curStateId = ffs(m_displayStates) - 1;
    if (curStateId < 0 || curStateId >= int(DisplayState::NumStates))
        return;
    const DisplayStateData &curStateData = m_displayStatesData[curStateId];
    const std::string &displayMessage = curStateData.displayMessage;
    switch (curStateId) {
    case DisplayState::PARTS:
        for (unsigned int i=0; i<9; ++i) {
            if (m_polyState[i])
                display[i*2] = 128;
            else
                display[i*2] = i==8 ? 'R' : ('1' + i);
        }
        break;
    case DisplayState::PROGRESS:
        n = int(LCDDisplay::NUMCHARS+1) * m_displayProgress;
        if (n < 0) n = 0;
        if (n > int(LCDDisplay::NUMCHARS)) n = int(LCDDisplay::NUMCHARS);
        strncpy((char*)display, displayMessage.c_str(), LCDDisplay::NUMCHARS);
        for (int u=0; u<n; ++u)
            display[u] = 128;
        break;
    case DisplayState::MESSAGE:
    case DisplayState::PROGRAM_CHANGE:
        memcpy((char*)display, displayMessage.data(), std::min(LCDDisplay::NUMCHARS, displayMessage.size()));
        break;
    case DisplayState::RESET:
        memset((char*)display, 128, LCDDisplay::NUMCHARS);
        break;
    }
    m_ui->display->setData(0, LCDDisplay::NUMCHARS, display);
}

void MuntPluginUI::displayTimeout(void *self_)
{
    MuntPluginUI::DisplayStateData *self = static_cast<MuntPluginUI::DisplayStateData*>(self_);
    self->parent->m_displayStates &= ~(1 << self->id);
    self->parent->updateDisplay();
}

int MuntPluginUI::idle()
{
    Fl::check();
    Fl::flush();
    if (!m_syxData.empty())
        syxProcessEvents(1000);
    return 0;
}

MuntPluginUI::~MuntPluginUI()
{
    delete m_ui;
}

/** C++ LV2UI wrapper functions */
template <class T> class LV2UIWrapper
{
private:
    static void cleanup(LV2UI_Handle ui)
    {
        delete static_cast<T*>(ui);
    }
    static void port_event(LV2UI_Handle ui, uint32_t port_index, uint32_t buffer_size,
               uint32_t format, const void* buffer)
    {
        static_cast<T*>(ui)->port_event(port_index, buffer_size, format, buffer);
    }
    static int idle(LV2UI_Handle ui)
    {
        return static_cast<T*>(ui)->idle();
    }
    static const void* external_extension_data(const char* uri)
    {
        static const LV2UI_Idle_Interface idle_interface = {
            idle
        };
        if (strcmp(uri, LV2_UI__idleInterface) == 0)
            return (void *) &idle_interface;
        return NULL;
    }
public:
    static const LV2UI_Descriptor *get_descriptor()
    {
        static const LV2UI_Descriptor descriptor = {
            T::URI,
            T::instantiate,
            cleanup,
            port_event,
            external_extension_data
        };
        return &descriptor;
    }
};

LV2_SYMBOL_EXPORT
__attribute__ ((visibility ("default")))
const LV2UI_Descriptor *lv2ui_descriptor ( uint32_t index )
{
    switch (index) {
    case 0: return LV2UIWrapper<MuntPluginUI>::get_descriptor();
    default: return NULL;
    }
}
