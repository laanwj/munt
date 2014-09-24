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
/** DSP event types */
#define MUNT_URI__cmd_resetSynth           MUNT_URI "#cmd_resetSynth"
/** Event arguments */
#define MUNT_URI__arg_message              MUNT_URI "#arg_message"
#define MUNT_URI__arg_partNum              MUNT_URI "#arg_partNum"
#define MUNT_URI__arg_bankNum              MUNT_URI "#arg_bankNum"
#define MUNT_URI__arg_patchName            MUNT_URI "#arg_patchName"
#define MUNT_URI__arg_numPolys             MUNT_URI "#arg_numPolys"
#define MUNT_URI__arg_numPolysNonReleasing MUNT_URI "#arg_numPolysNonReleasing"

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

#endif
