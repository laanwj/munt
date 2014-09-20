mt32emu_lv2
============

mt32emu_lv2 is a LV2 plugin that approximately emulates the Roland MT-32, CM-32L and LAPC-I synthesizer modules.
It uses the Munt MT-32 emulator (which is included in the repository). Like the famous Roland D-50, the MT-32
generates sound with a combination of Linear Arithmetic synthesis and samples for the attack transients.

As a LV2 plugin it can be used in plugin hosts such as [Carla](http://kxstudio.sourceforge.net/Applications:Carla),
or directly in some sequencers such as [QTractor](http://qtractor.sourceforge.net/qtractor-index.html). It has been
tested in the aforementioned programs, but should work with others as well.

It has a basic GUI that shows the current part status (similar to mt32emu_qt) and can be used to load syx files.

![Status](mt32emu_lv2ui/screenshots/parts.png)

![Loading syx file](mt32emu_lv2ui/screenshots/loadsyx.png)

![SysEx LCD message](mt32emu_lv2ui/screenshots/message.png)

Known issues
--------------

- GUI is very basic (I could use help with design)
- Aimed at Linux. The underlying Munt library is portable, and as NTK is used for the UI, portability to Windows or MacOSX should be possible, but was never tested.
- No way to edit instrument settings in the GUI; however it is possible to load syx banks provided by an external SysEx librarion

Install
---------

This will build and install the plugin:

```bash
mkdir build
cd build
cmake ..
make
sudo make install
```

### Cmake options

These options can be passed on the `cmake` command line to configure the build

```
-DCMAKE_BUILD_TYPE:STRING=RelWithDebInfo|Release|Debug 

Build in Release with debug info (default)/Release/Debug mode
```

```
-DCMAKE_INSTALL_PREFIX:PATH=/opt/music

To install to an alternative prefix
```

Copying the ROMS
-----------------

Like Munt standalone, this plugin needs a copy of the MT-32 (or CM-32) control
and PCM ROMs. These ROMs need to be installed manually to the LV2 bundle path
(`$PREFIX/lib/lv2/mt32emu.lv2`), otherwise there will be no sound output. Their
names should be `control.rom` and `pcm.rom` respectively.

For example:
```bash
cp MT32_CONTROL.ROM $PREFIX/lib/lv2/mt32emu.lv2/control.rom
cp MT32_PCM.ROM $PREFIX/lib/lv2/mt32emu.lv2/pcm.rom
```

If everything goes alright, the plugin will log the following while loading:

    Control ROM: MT-32 Control v1.07
    PCM ROM: MT-32 PCM ROM

Otherwise it will print, along with some more verbose information

    Unable to open ROM images, not activating

SysEx banks
------------
Many preset patch banks in SysEx (.syx) format can be found for the MT-32, for
example at [Quest Studios](http://www.queststudios.com/roland/banks.html).
Just like the standalone Munt emulator, the LV2 plugin can accept these as inline SysEx events or
when loaded through the GUI.

Important
----------
This LV2 plugin but behaves quite differently from most other LV2 synths,
which use only MIDI channel 1 or ignore MIDI channels altogether. For now I have decided
to keep the 'multi-timbral' part of the emulation - maybe a one-MT32-part plugin could be useful at
some point.

The default setting the MT-32 has parts on MIDI channel 2-9, as well as a rhythm
part on MIDI channel 10. **Sending events on MIDI channel 1 will thus have no effect**.

Another difference with other plugins is that this plugin accepts program changes per channel. 
Some hosts, for example Carla, may have to be configued to let those through instead of mapping them
(uncheck "Map Program Changes", check "Send Bank/Program Changes").

TODOs
------

- Expose Reverb settings through port parameters (synth allows overriding these, see setReverbOverridden)
- How useful can this be made without the MT-32 roms, can we make a standalone
  LA synth? Is it 'just' a matter of providing our own presets and samples?
- Figure out whether it makes sense to pass `MT32EMU_USE_FLOAT_SAMPLES` to render to floats directly

