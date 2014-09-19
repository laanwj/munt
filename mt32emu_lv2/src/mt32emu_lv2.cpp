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

#include <mt32emu/mt32emu.h>
#include "resample/SampleRateConverter.h"

#define MUNT_URI "http://github.com/munt/munt"

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

class ReportHandler_LV2;

class MuntPlugin
{
public:
    MuntPlugin(const LV2_Descriptor* descriptor, double rate, const char* bundle_path,
        const LV2_Feature* const* features);
    ~MuntPlugin();

    void connect_port(uint32_t port, void* data);
    void activate();
    void run(uint32_t sample_count);
    void deactivate();

    struct Features
    {
        LV2_URID_Map* map;
        LV2_Log_Log* log;
    };
    struct URIs
    {
        LV2_URID midi_MidiEvent;
        LV2_URID log_Error;
        LV2_URID log_Note;
        LV2_URID log_Trace;
        LV2_URID log_Warning;
        LV2_URID atom_eventTransfer;
        // Event type property
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
private:
    Features m_features;
    URIs m_uris;
    struct Ports
    {
        const LV2_Atom_Sequence* control;
        LV2_Atom_Sequence* notify;
        float *out[2];
    } m_ports;

    std::string m_bundlePath;
    double m_rate;

    ReportHandler_LV2 *m_reportHandler;
    MT32Emu::Synth *m_synth;
    const MT32Emu::ROMImage *m_controlROMImage;
    const MT32Emu::ROMImage *m_pcmROMImage;
    uint64_t m_timestamp;
    /// Ratio scaling from real samples to MT32 samples
    double m_rateRatio;
    SampleRateConverter *m_converter;
    LV2_Atom_Forge m_forge;
    LV2_Atom_Forge_Frame m_notify_frame;
};

/** Reporthandler that passes on events to UI through
 * notification port.
 */
class ReportHandler_LV2: public MT32Emu::ReportHandler
{
public:
    ReportHandler_LV2(const MuntPlugin::Features &features, const MuntPlugin::URIs &uris, LV2_Atom_Forge &m_forge):
        m_features(features), m_uris(uris), m_forge(m_forge), m_synth(0) {}
    ~ReportHandler_LV2() {}

    void setSynth(MT32Emu::Synth *synth) { m_synth = synth; }

protected:
    // Callback for debug messages, in vprintf() format
    void printDebug(const char *fmt, va_list list)
    {
        //m_features.log->vprintf(m_features.log->handle, m_uris.log_Note, fmt, list);
        //m_features.log->printf(m_features.log->handle, m_uris.log_Note, "\n");
        vfprintf(stderr, fmt, list);
        fputs("\n", stderr);
        fflush(stderr);
    }

    // Callbacks for reporting various errors and information
    void onErrorControlROM()
    {
        //m_features.log->printf(m_features.log->handle, m_uris.log_Error, "Error loading control ROM\n");
        fprintf(stderr, "Error loading control ROM\n");
        fflush(stderr);
    }
    void onErrorPCMROM()
    {
        //m_features.log->printf(m_features.log->handle, m_uris.log_Error, "Error loading PCM ROM\n");
        fprintf(stderr, "Error loading PCM ROM\n");
        fflush(stderr);
    }
    void showLCDMessage(const char *message)
    {
        //m_features.log->printf(m_features.log->handle, m_uris.log_Note, "LCD message: %s\n", message);
        fprintf(stderr, "LCD message: %s\n", message);
        fflush(stderr);
        LV2_Atom_Forge_Frame frame;
        lv2_atom_forge_frame_time(&m_forge, 0);
        lv2_atom_forge_object(&m_forge, &frame, 0, m_uris.atom_eventTransfer);
        lv2_atom_forge_key(&m_forge, m_uris.munt_eventType);
        lv2_atom_forge_urid(&m_forge, m_uris.munt_evt_showLCDMessage);
        lv2_atom_forge_key(&m_forge, m_uris.munt_arg_message);
        lv2_atom_forge_string(&m_forge, message, strlen(message));
        lv2_atom_forge_pop(&m_forge, &frame);
    }
    void onMIDIMessagePlayed() {}
    void onDeviceReset() {}
    void onDeviceReconfig() {}
    void onNewReverbMode(MT32Emu::Bit8u /*mode*/) {}
    void onNewReverbTime(MT32Emu::Bit8u /*time*/) {}
    void onNewReverbLevel(MT32Emu::Bit8u /*level*/) {}
    void onPolyStateChanged(int partNum)
    {
        const MT32Emu::Part *part = m_synth->getPart(partNum);
        const MT32Emu::Poly *poly = part->getFirstActivePoly();
        // Count number of polys, as well as non-releasing polys, and
        // send statistics
        int numPolys = 0;
        int numPolysNonReleasing = 0;
        while (poly != NULL) {
            if (poly->getState() != MT32Emu::POLY_Releasing)
                numPolysNonReleasing += 1;
            numPolys += 1;
            poly = poly->getNext();
        }
        LV2_Atom_Forge_Frame frame;
        lv2_atom_forge_frame_time(&m_forge, 0);
        lv2_atom_forge_object(&m_forge, &frame, 0, m_uris.atom_eventTransfer);
        lv2_atom_forge_key(&m_forge, m_uris.munt_eventType);
        lv2_atom_forge_urid(&m_forge, m_uris.munt_evt_onPolyStateChanged);
        lv2_atom_forge_key(&m_forge, m_uris.munt_arg_partNum);
        lv2_atom_forge_int(&m_forge, partNum);
        lv2_atom_forge_key(&m_forge, m_uris.munt_arg_numPolys);
        lv2_atom_forge_int(&m_forge, numPolys);
        lv2_atom_forge_key(&m_forge, m_uris.munt_arg_numPolysNonReleasing);
        lv2_atom_forge_int(&m_forge, numPolysNonReleasing);
        lv2_atom_forge_pop(&m_forge, &frame);
    }
    void onProgramChanged(int partNum, int bankNum, const char *patchName)
    {
        LV2_Atom_Forge_Frame frame;
        lv2_atom_forge_frame_time(&m_forge, 0);
        lv2_atom_forge_object(&m_forge, &frame, 0, m_uris.atom_eventTransfer);
        lv2_atom_forge_key(&m_forge, m_uris.munt_eventType);
        lv2_atom_forge_urid(&m_forge, m_uris.munt_evt_onProgramChanged);
        lv2_atom_forge_key(&m_forge, m_uris.munt_arg_partNum);
        lv2_atom_forge_int(&m_forge, partNum);
        lv2_atom_forge_key(&m_forge, m_uris.munt_arg_bankNum);
        lv2_atom_forge_int(&m_forge, bankNum);
        lv2_atom_forge_key(&m_forge, m_uris.munt_arg_patchName);
        lv2_atom_forge_string(&m_forge, patchName, strlen(patchName));
        lv2_atom_forge_pop(&m_forge, &frame);
    }

private:
    const MuntPlugin::Features &m_features;
    const MuntPlugin::URIs &m_uris;
    LV2_Atom_Forge &m_forge;
    MT32Emu::Synth *m_synth;
};

MuntPlugin::MuntPlugin(const LV2_Descriptor* /*descriptor*/, double rate, const char* bundle_path,
    const LV2_Feature* const* features):
    m_rate(rate), m_reportHandler(0), m_synth(0), m_controlROMImage(0), m_pcmROMImage(0),
    m_timestamp(0), m_rateRatio(0), m_converter(0)
{
    memset(&m_features, 0, sizeof(m_features));
    memset(&m_uris, 0, sizeof(m_uris));
    memset(&m_ports, 0, sizeof(m_ports));

    for (int i = 0; features[i]; ++i)
    {
        if (!strcmp(features[i]->URI, LV2_URID__map))
            m_features.map = (LV2_URID_Map*)features[i]->data;
        if (!strcmp(features[i]->URI, LV2_LOG__log))
            m_features.log = (LV2_Log_Log*)features[i]->data;
    }

    if (m_features.map)
    {
        LV2_URID_Map* map = m_features.map;
        m_uris.midi_MidiEvent = map->map(map->handle, LV2_MIDI__MidiEvent);
        m_uris.log_Error = map->map(map->handle, LV2_LOG__Error);
        m_uris.log_Note = map->map(map->handle, LV2_LOG__Note);
        m_uris.log_Trace = map->map(map->handle, LV2_LOG__Trace);
        m_uris.log_Warning = map->map(map->handle, LV2_LOG__Warning);
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
    /// forge for DSP->UI events
    lv2_atom_forge_init(&m_forge, m_features.map);

    m_bundlePath = bundle_path;
}

MuntPlugin::~MuntPlugin()
{
}

void MuntPlugin::connect_port(uint32_t port, void* data)
{
    switch(port)
    {
    case PortIndex::CONTROL:
        m_ports.control = static_cast<const LV2_Atom_Sequence*>(data);
        break;
    case PortIndex::NOTIFY:
        m_ports.notify = static_cast<LV2_Atom_Sequence*>(data);
        break;
    case PortIndex::OUT_L:
        m_ports.out[0] = static_cast<float*>(data);
        break;
    case PortIndex::OUT_R:
        m_ports.out[1] = static_cast<float*>(data);
        break;
    }
}

void deleteROMImage(const MT32Emu::ROMImage *image)
{
    if (!image)
        return;
    delete image->getFile();
    MT32Emu::ROMImage::freeROMImage(image);
}

const MT32Emu::ROMImage *loadROMImage(const std::string &filename)
{
    MT32Emu::FileStream *file = new MT32Emu::FileStream();
    if (!file->open(filename.c_str()))
    {
        fprintf(stderr, "Unable to open ROM image %s\n", filename.c_str());
        fflush(stderr);
        delete file;
        return NULL;
    }

    const MT32Emu::ROMImage *image = MT32Emu::ROMImage::makeROMImage(file);
    if (image->getROMInfo() == NULL)
    {
        fprintf(stderr, "Could not identify ROM image %s\n", filename.c_str());
        fflush(stderr);
        deleteROMImage(image);
        return NULL;
    }
    return image;
}

void MuntPlugin::activate()
{
    m_reportHandler = new ReportHandler_LV2(m_features, m_uris, m_forge);
    m_synth = new MT32Emu::Synth(m_reportHandler);
    m_reportHandler->setSynth(m_synth);

    m_controlROMImage = loadROMImage(m_bundlePath + "/control.rom");
    m_pcmROMImage = loadROMImage(m_bundlePath + "/pcm.rom");
    if (!m_controlROMImage || !m_pcmROMImage)
    {
        fprintf(stderr, "Unable to open ROM images, not activating\n");
        fflush(stderr);
        delete m_synth; m_synth = 0;
        return;
    }
    printf("Control ROM: %s\n", m_controlROMImage->getROMInfo()->description);
    printf("PCM ROM: %s\n", m_pcmROMImage->getROMInfo()->description);

    if (!m_synth->open(*m_controlROMImage, *m_pcmROMImage, MT32Emu::DEFAULT_MAX_PARTIALS))
    {
        fprintf(stderr, "Unable to open synth, not activating\n");
        fflush(stderr);
        return;
    }
    m_timestamp = 0;
    if (m_rate != MT32Emu::SAMPLE_RATE)
    {
        m_converter = SampleRateConverter::createSampleRateConverter(m_synth, m_rate);
        fprintf(stdout, "Converting sample rate from %f to %f\n", (double)MT32Emu::SAMPLE_RATE, m_rate);
        fflush(stdout);
    }
    m_rateRatio = MT32Emu::SAMPLE_RATE / m_rate;
}

void MuntPlugin::run(uint32_t sample_count)
{
    if (!m_ports.control || !m_ports.notify || !m_ports.out[0] || !m_ports.out[1] || !m_synth)
        return;

    // Set up forge to write to notify port
    lv2_atom_forge_set_buffer(&m_forge, (uint8_t*)m_ports.notify, m_ports.notify->atom.size);
    lv2_atom_forge_sequence_head(&m_forge, &m_notify_frame, 0);

    LV2_ATOM_SEQUENCE_FOREACH(m_ports.control, ev)
    {
        if (ev->body.type == m_uris.midi_MidiEvent && ev->body.size > 0)
        {
            const uint8_t *evdata = (uint8_t *)LV2_ATOM_BODY(&ev->body);
            uint64_t timeTarget = m_timestamp + ev->time.frames;
            uint64_t timeScaled = timeTarget * m_rateRatio;
            if (evdata[0] != LV2_MIDI_MSG_SYSTEM_EXCLUSIVE)
            {
                MT32Emu::Bit32u msg = 0;
                for (unsigned i=0; i<ev->body.size; ++i)
                    msg |= evdata[i] << (i*8);
                m_synth->playMsg(msg, timeScaled);
            }
            else
            {
                m_synth->playSysex(evdata, ev->body.size, timeScaled);
                printf("sysex t=%08x st=%08x size=%08x\n", (unsigned)timeTarget, (unsigned)timeScaled, ev->body.size);
                fflush(stdout);
            }
        }
        else
        {
            printf("mt32emu_lv2: Unknown event type %i\n", ev->body.type);
            fflush(stdout);
        }
    }

    MT32Emu::Sample samples[MT32Emu::MAX_SAMPLES_PER_RUN*2];
    uint32_t offset = 0;
    while(offset < sample_count)
    {
        uint32_t framesToRender = std::min(sample_count - offset, MT32Emu::MAX_SAMPLES_PER_RUN);

        if (m_converter)
            m_converter->getOutputSamples(samples, framesToRender);
        else
            m_synth->render(samples, framesToRender);

        for(unsigned x=0; x<framesToRender; ++x)
        {
#if MT32EMU_USE_FLOAT_SAMPLES
            m_ports.out[0][offset] = samples[x*2+0];
            m_ports.out[1][offset] = samples[x*2+1];
#else
            m_ports.out[0][offset] = samples[x*2+0] / 10240.0;
            m_ports.out[1][offset] = samples[x*2+1] / 10240.0;
#endif
            offset += 1;
        }
    }
    m_timestamp += sample_count;
}

void MuntPlugin::deactivate()
{
    delete m_converter;
    if (m_synth)
        m_synth->close();
    delete m_synth; m_synth = 0;

    deleteROMImage(m_controlROMImage);
    deleteROMImage(m_pcmROMImage);
    m_controlROMImage = m_pcmROMImage = 0;
}

//////////////////////////////////////////
// LV2 plugin C++ wrapper

static LV2_Handle
instantiate(const LV2_Descriptor*     descriptor,
            double                    rate,
            const char*               bundle_path,
            const LV2_Feature* const* features)
{
    if (!strcmp(descriptor->URI, MUNT_URI))
        return static_cast<LV2_Handle>(
            new MuntPlugin(descriptor, rate, bundle_path, features));
    return NULL;
}

static void
connect_port(LV2_Handle instance,
             uint32_t   port,
             void*      data)
{
    static_cast<MuntPlugin*>(instance)->connect_port(port, data);
}

static void
activate(LV2_Handle instance)
{
    static_cast<MuntPlugin*>(instance)->activate();
}

static void
run(LV2_Handle instance, uint32_t sample_count)
{
    static_cast<MuntPlugin*>(instance)->run(sample_count);
}

static void
deactivate(LV2_Handle instance)
{
    static_cast<MuntPlugin*>(instance)->deactivate();
}

static void
cleanup(LV2_Handle instance)
{
    delete static_cast<MuntPlugin*>(instance);
}

static const void*
extension_data(const char* uri)
{
    return NULL;
}

static const LV2_Descriptor descriptor = {
    MUNT_URI,
    instantiate,
    connect_port,
    activate,
    run,
    deactivate,
    cleanup,
    extension_data
};

LV2_SYMBOL_EXPORT
__attribute__ ((visibility ("default")))
const LV2_Descriptor*
lv2_descriptor(uint32_t index)
{
    switch (index)
    {
    case 0: return &descriptor;
    default: return NULL;
    }
}
