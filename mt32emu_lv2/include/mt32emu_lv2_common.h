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

/** Common data between LV2 UI and DSP */
#ifndef H_MT32EMU_LV2_COMMON
#define H_MT32EMU_LV2_COMMON

#define UNUSED(x) (void)(x)

/** Plugin URI for DSP */
#define MUNT_URI "http://github.com/munt/munt"
/** Plugin URI for UI */
#define MUNT_URI_UI "http://github.com/munt/munt/gui"

/** Sub-URIs for state save/restore */
#define MUNT_URI__PatchTempMemoryRegion    MUNT_URI "#PatchTempMemoryRegion"
#define MUNT_URI__RhythmTempMemoryRegion   MUNT_URI "#RhythmTempMemoryRegion"
#define MUNT_URI__TimbreTempMemoryRegion   MUNT_URI "#TimbreTempMemoryRegion"
#define MUNT_URI__PatchesMemoryRegion      MUNT_URI "#PatchesMemoryRegion"
#define MUNT_URI__TimbresMemoryRegion      MUNT_URI "#TimbresMemoryRegion"
#define MUNT_URI__SystemMemoryRegion       MUNT_URI "#SystemMemoryRegion"
/** UI event type property */
#define MUNT_URI__eventType                MUNT_URI "#eventType"
/** UI event types */
#define MUNT_URI__evt_showLCDMessage       MUNT_URI "#evt_showLCDMessage"
#define MUNT_URI__evt_onPolyStateChanged   MUNT_URI "#evt_onPolyStateChanged"
#define MUNT_URI__evt_onProgramChanged     MUNT_URI "#evt_onProgramChanged"
#define MUNT_URI__evt_onDeviceReset        MUNT_URI "#evt_onDeviceReset"
#define MUNT_URI__evt_onSysExReceived      MUNT_URI "#evt_onSysExReceived"
/** DSP event types */
#define MUNT_URI__cmd_resetSynth           MUNT_URI "#cmd_resetSynth"
/** Event arguments */
#define MUNT_URI__arg_message              MUNT_URI "#arg_message"
#define MUNT_URI__arg_partNum              MUNT_URI "#arg_partNum"
#define MUNT_URI__arg_bankNum              MUNT_URI "#arg_bankNum"
#define MUNT_URI__arg_patchName            MUNT_URI "#arg_patchName"
#define MUNT_URI__arg_numPolys             MUNT_URI "#arg_numPolys"
#define MUNT_URI__arg_numPolysNonReleasing MUNT_URI "#arg_numPolysNonReleasing"
#define MUNT_URI__arg_addr                 MUNT_URI "#arg_addr"
#define MUNT_URI__arg_len                  MUNT_URI "#arg_len"

/** Port indices */
enum PortIndex: uint32_t
{
    CONTROL = 0,
    NOTIFY = 1,
    OUT_L = 2,
    OUT_R = 3,
    REVERB_ENABLE = 4,
    REVERB_OVERRIDE = 5,
    REVERB_MODE = 6,
    REVERB_TIME = 7,
    REVERB_LEVEL = 8,
    OUTPUT_GAIN = 9,
    REVERB_OUTPUT_GAIN = 10
};

bool getSysExInfo(const uint8_t *evdata, uint32_t size, uint32_t *addr_out, uint32_t *len_out)
{
    if (size > 10)
    {
        // TODO check that this is really a Roland SysEx
        if (addr_out)
            *addr_out = (evdata[5] << 24)|(evdata[6]<<16)|evdata[7];
        if (len_out)
            *len_out = size - 10; // F1 <hdr:7> <data:N> <checksum:1> F7
        return true;
    }
    return false;
}

#endif
