/*
 * OPN2 Bank Editor by Wohlstand, a free tool for music bank editing
 * Copyright (c) 2017-2022 Vitaly Novichkov <admin@wohlnet.ru>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "generator.h"
#include <qendian.h>
#include <cmath>

#include "chips/gens_opn2.h"
#include "chips/nuked_opn2.h"
#include "chips/mame_opn2.h"
#include "chips/gx_opn2.h"
#include "chips/np2_opna.h"
#include "chips/mame_opna.h"
#include "chips/pmdwin_opna.h"

#define USED_CHANNELS_4OP       6

/***************************************************************
 *                    Volume model tables                      *
 ***************************************************************/

// Mapping from MIDI volume level to OPL level value.

static const uint_fast32_t s_dmx_volume_model[128] =
{
    0,  1,  3,  5,  6,  8,  10, 11,
    13, 14, 16, 17, 19, 20, 22, 23,
    25, 26, 27, 29, 30, 32, 33, 34,
    36, 37, 39, 41, 43, 45, 47, 49,
    50, 52, 54, 55, 57, 59, 60, 61,
    63, 64, 66, 67, 68, 69, 71, 72,
    73, 74, 75, 76, 77, 79, 80, 81,
    82, 83, 84, 84, 85, 86, 87, 88,
    89, 90, 91, 92, 92, 93, 94, 95,
    96, 96, 97, 98, 99, 99, 100, 101,
    101, 102, 103, 103, 104, 105, 105, 106,
    107, 107, 108, 109, 109, 110, 110, 111,
    112, 112, 113, 113, 114, 114, 115, 115,
    116, 117, 117, 118, 118, 119, 119, 120,
    120, 121, 121, 122, 122, 123, 123, 123,
    124, 124, 125, 125, 126, 126, 127, 127,
};

static const uint_fast32_t W9X_volume_mapping_table[32] =
{
    63, 63, 40, 36, 32, 28, 23, 21,
    19, 17, 15, 14, 13, 12, 11, 10,
    9,  8,  7,  6,  5,  5,  4,  4,
    3,  3,  2,  2,  1,  1,  0,  0
};


QString GeneratorDebugInfo::toStr()
{
    return QString("Channels:\n"
                   "4-op: %1")
        .arg(this->chan4op);
}

Generator::Generator(uint32_t sampleRate, OPN_Chips initialChip)
{
    m_rate = sampleRate;
    note = 60;
    lfo_reg = 0x00;
    m_patch =
    {
        {
            { {0x71, 0x23, 0x5F, 0x05, 0x02, 0x11, 0x00} },
            { {0x0D, 0x2D, 0x99, 0x05, 0x02, 0x11, 0x00} },
            { {0x33, 0x26, 0x5F, 0x05, 0x02, 0x11, 0x00} },
            { {0x01, 0x00, 0x94, 0x07, 0x02, 0xA6, 0x00} },
        },
        0x32,
        0x00,
        +12,
        60,
    };

    memset(m_ins, 0, sizeof(uint16_t) * NUM_OF_CHANNELS);
    memset(m_pit, 0, sizeof(uint8_t) * NUM_OF_CHANNELS);
    memset(m_pan_lfo, 0, sizeof(uint8_t) * NUM_OF_CHANNELS);

    switchChip(initialChip);

    //Send the null patch to initialize the OPL stuff
    changePatch(FmBank::emptyInst(), false);
    m_isInstrumentLoaded = false;//Reset the flag to false as no instruments loaded
}

Generator::~Generator()
{}

void Generator::initChip()
{
    //Init chip //7670454
    chip->setRate(m_rate, chip->nativeClockRate());
    WriteReg(0, 0x22, lfo_reg);   //LFO off
    WriteReg(0, 0x27, 0x0 );   //Channel 3 mode normal

    //Shut up all channels
    WriteReg(0, 0x28, 0x00 );   //Note Off 0 channel
    WriteReg(0, 0x28, 0x01 );   //Note Off 1 channel
    WriteReg(0, 0x28, 0x02 );   //Note Off 2 channel
    WriteReg(0, 0x28, 0x04 );   //Note Off 3 channel
    WriteReg(0, 0x28, 0x05 );   //Note Off 4 channel
    WriteReg(0, 0x28, 0x06 );   //Note Off 5 channel

    //Disable DAC
    WriteReg(0, 0x2B, 0x0 );   //DAC off

    for(uint16_t chn = 0; chn < 6; chn++)
    {
        uint8_t ch   = chn % 3;
        uint8_t port = (chn <= 2) ? 0 : 1;
        WriteReg(port, 0x30 + ch, 0x71);   //Detune/Frequency Multiple operator 1
        WriteReg(port, 0x34 + ch, 0x0D);   //Detune/Frequency Multiple operator 2
        WriteReg(port, 0x38 + ch, 0x33);   //Detune/Frequency Multiple operator 3
        WriteReg(port, 0x3C + ch, 0x01);   //Detune/Frequency Multiple operator 4

        WriteReg(port, 0x40 + ch, 0x23);   //Total Level operator 1
        WriteReg(port, 0x44 + ch, 0x2D);   //Total Level operator 2
        WriteReg(port, 0x48 + ch, 0x26);   //Total Level operator 3
        WriteReg(port, 0x4C + ch, 0x00);   //Total Level operator 4

        WriteReg(port, 0x50 + ch, 0x5F);   //RS/AR operator 1
        WriteReg(port, 0x54 + ch, 0x99);   //RS/AR operator 2
        WriteReg(port, 0x58 + ch, 0x5F);   //RS/AR operator 3
        WriteReg(port, 0x5C + ch, 0x94);   //RS/AR operator 4

        WriteReg(port, 0x60 + ch, 0x05);   //AM/D1R operator 1
        WriteReg(port, 0x64 + ch, 0x05);   //AM/D1R operator 2
        WriteReg(port, 0x68 + ch, 0x05);   //AM/D1R operator 3
        WriteReg(port, 0x6C + ch, 0x07);   //AM/D1R operator 4

        WriteReg(port, 0x70 + ch, 0x02);   //D2R operator 1
        WriteReg(port, 0x74 + ch, 0x02);   //D2R operator 2
        WriteReg(port, 0x78 + ch, 0x02);   //D2R operator 3
        WriteReg(port, 0x7C + ch, 0x02);   //D2R operator 4

        WriteReg(port, 0x80 + ch, 0x11);   //D1L/RR Operator 1
        WriteReg(port, 0x84 + ch, 0x11);   //D1L/RR Operator 2
        WriteReg(port, 0x88 + ch, 0x11);   //D1L/RR Operator 3
        WriteReg(port, 0x8C + ch, 0xA6);   //D1L/RR Operator 4

        WriteReg(port, 0x90 + ch, 0x00);   //Proprietary shit
        WriteReg(port, 0x94 + ch, 0x00);   //Proprietary shit
        WriteReg(port, 0x98 + ch, 0x00);   //Proprietary shit
        WriteReg(port, 0x9C + ch, 0x00);   //Proprietary shit

        WriteReg(port, 0xB0 + ch, 0x32);   //Feedback/Algorithm

        m_pan_lfo[ch] = 0xC0;
        WriteReg(port, 0xB4 + ch, m_pan_lfo[ch]);   //Panorame (toggle on both speakers)

        WriteReg(port, 0x28, 0x00 + ch);   //Key off to channel

        WriteReg(port, 0xA4 + ch, 0x68);   //Set frequency and octave
        WriteReg(port, 0xA0 + ch, 0xFF);
    }

    // OPN END
    Silence();
}

void Generator::switchChip(Generator::OPN_Chips chipId, int family)
{
    m_chipFamily = static_cast<OPNFamily>(family);

    switch(chipId)
    {
    case CHIP_GENS:
        chip.reset(new GensOPN2(m_chipFamily));
        break;
    default:
    case CHIP_Nuked:
        chip.reset(new NukedOPN2(m_chipFamily));
        break;
    case CHIP_MAME:
        chip.reset(new MameOPN2(m_chipFamily));
        break;
    case CHIP_GX:
        chip.reset(new GXOPN2(m_chipFamily));
        break;
    case CHIP_NP2:
        chip.reset(new NP2OPNA<>(m_chipFamily));
        break;
    case CHIP_MAMEOPNA:
        chip.reset(new MameOPNA(m_chipFamily));
        break;
    case CHIP_PMDWIN:
        chip.reset(new PMDWinOPNA(m_chipFamily));
        break;
    }
    initChip();
}

void Generator::WriteReg(uint8_t port, uint16_t address, uint8_t byte)
{
    chip->writeReg(port, address, byte);
}

void Generator::NoteOff(uint32_t c)
{
    uint8_t cc = static_cast<uint8_t>(c % 6);
    WriteReg(0, 0x28, (c <= 2) ? cc : cc + 1);
}

void Generator::NoteOn(uint32_t c, double hertz) // Hertz range: 0..131071
{
    uint8_t  cc     = uint8_t(c % 3);
    uint8_t  port   = uint8_t((c <= 2) ? 0 : 1);
    uint32_t octave = 0, ftone = 0;
    uint32_t mul_offset = 0;

    if(hertz < 0) // Avoid infinite loop
        return;

    double coef;
    switch(m_chipFamily)
    {
    case OPNChip_OPN2: default:
        coef = 321.88557; break;
    case OPNChip_OPNA:
        coef = 309.12412; break;
    }
    hertz *= coef;

    //Basic range until max of octaves reaching
    while((hertz >= 1023.75) && (octave < 0x3800))
    {
        hertz /= 2.0;    // Calculate octave
        octave += 0x800;
    }
    //Extended range, rely on frequency multiplication increment
    while(hertz >= 2036.75)
    {
        hertz /= 2.0;    // Calculate octave
        mul_offset++;
    }
    ftone = octave + static_cast<uint32_t>(hertz + 0.5);

    for(size_t op = 0; op < 4; op++)
    {
        uint32_t reg = m_patch.OPS[op].data[0];
        uint16_t address = 0x30 + (op * 4) + cc;
        if(mul_offset > 0) // Increase frequency multiplication value
        {
            uint32_t dt  = reg & 0xF0;
            uint32_t mul = reg & 0x0F;
            if((mul + mul_offset) > 0x0F)
            {
                mul_offset = 0;
                mul = 0x0F;
            }
            WriteReg(port, address, uint8_t(dt | (mul + mul_offset)));
        }
        else
        {
            WriteReg(port, address, uint8_t(reg));
        }
    }

    WriteReg(port, 0xA4 + cc, (ftone >> 8) & 0xFF);//Set frequency and octave
    WriteReg(port, 0xA0 + cc,  ftone & 0xFF);
    WriteReg(0, 0x28, 0xF0 + uint8_t((c <= 2) ? c : c + 1));
}

void Generator::touchNote(uint32_t c,
                          uint32_t velocity,
                          uint8_t channelVolume,
                          uint8_t channelExpression,
                          uint32_t brightness)
{
    uint8_t cc   =  c % 3;
    uint8_t port = (c <= 2) ? 0 : 1;

    uint_fast32_t volume = 0;

    uint8_t op_vol[4] =
    {
        m_patch.OPS[OPERATOR1].data[1],
        m_patch.OPS[OPERATOR2].data[1],
        m_patch.OPS[OPERATOR3].data[1],
        m_patch.OPS[OPERATOR4].data[1],
    };

    bool alg_do[8][4] =
    {
        /*
         * Yeah, Operator 2 and 3 are seems swapped
         * which we can see in the algorithm 4
         */
        //OP1   OP3   OP2    OP4
        //30    34    38     3C
        {false,false,false,true},//Algorithm #0:  W = 1 * 2 * 3 * 4
        {false,false,false,true},//Algorithm #1:  W = (1 + 2) * 3 * 4
        {false,false,false,true},//Algorithm #2:  W = (1 + (2 * 3)) * 4
        {false,false,false,true},//Algorithm #3:  W = ((1 * 2) + 3) * 4
        {false,false,true, true},//Algorithm #4:  W = (1 * 2) + (3 * 4)
        {false,true ,true ,true},//Algorithm #5:  W = (1 * (2 + 3 + 4)
        {false,true ,true ,true},//Algorithm #6:  W = (1 * 2) + 3 + 4
        {true ,true ,true ,true},//Algorithm #7:  W = 1 + 2 + 3 + 4
    };


    switch(m_volumeScale)
    {
    default:
    case VOLUME_Generic:
    {
        volume = velocity * 127 *
                 channelVolume * channelExpression;

        /* If the channel has arpeggio, the effective volume of
             * *this* instrument is actually lower due to timesharing.
             * To compensate, add extra volume that corresponds to the
             * time this note is *not* heard.
             * Empirical tests however show that a full equal-proportion
             * increment sounds wrong. Therefore, using the square root.
             */
        //volume = (int)(volume * std::sqrt( (double) ch[c].users.size() ));
        const double c1 = 11.541560327111707;
        const double c2 = 1.601379199767093e+02;
        const uint_fast32_t minVolume = 1108075; // 8725 * 127

        // The formula below: SOLVE(V=127^4 * 2^( (A-63.49999) / 8), A)
        if(volume > minVolume)
        {
            double lv = std::log(static_cast<double>(volume));
            volume = static_cast<uint_fast32_t>(lv * c1 - c2) * 2;
        }
        else
            volume = 0;
    }
    break;

    case VOLUME_CMF:
    {
        volume = velocity * channelVolume * channelExpression;
        //volume = volume * m_masterVolume / (127 * 127 * 127) / 2;
        volume = ((volume * 127) / 4096766);

        if(volume > 0)
            volume += 64;//OPN has 0~127 range. As 0...63 is almost full silence, but at 64 to 127 is very closed to OPL3, just add 64.
    }
    break;

    case VOLUME_DMX:
    {
        volume = (channelVolume * channelExpression * 127) / 16129;
        volume = (s_dmx_volume_model[volume] + 1) << 1;
        volume = (s_dmx_volume_model[(velocity < 128) ? velocity : 127] * volume) >> 9;

        if(volume > 0)
            volume += 64;//OPN has 0~127 range. As 0...63 is almost full silence, but at 64 to 127 is very closed to OPL3, just add 64.
    }
    break;

    case VOLUME_APOGEE:
    {
        volume = (channelVolume * channelExpression * 127 / 16129);
        volume = ((64 * (velocity + 0x80)) * volume) >> 15;
        //volume = ((63 * (vol + 0x80)) * Ch[MidCh].volume) >> 15;
        if(volume > 0)
            volume += 64;//OPN has 0~127 range. As 0...63 is almost full silence, but at 64 to 127 is very closed to OPL3, just add 64.
    }
    break;

    case VOLUME_9X:
    {
        //volume = 63 - W9X_volume_mapping_table[(((vol * Ch[MidCh].volume /** Ch[MidCh].expression*/) * 127 / 16129 /*2048383*/) >> 2)];
        volume = 63 - W9X_volume_mapping_table[((velocity * channelVolume * channelExpression * 127 / 2048383) >> 2)];
        //volume = W9X_volume_mapping_table[vol >> 2] + volume;
        if(volume > 0)
            volume += 64;//OPN has 0~127 range. As 0...63 is almost full silence, but at 64 to 127 is very closed to OPL3, just add 64.
    }
    break;
    }

    if(volume > 127)
        volume = 127;

    uint8_t alg = m_patch.fbalg & 0x07;

    for(uint8_t op = 0; op < 4; op++)
    {
        bool do_op = alg_do[alg][op];
        uint32_t x = op_vol[op];
        uint32_t vol_res = do_op ? (127 - (static_cast<uint32_t>(volume) * (127 - (x & 127))) / 127) : x;
        if(brightness != 127)
        {
            brightness = static_cast<uint32_t>(::round(127.0 * ::sqrt((static_cast<double>(brightness)) * (1.0 / 127.0))));
            if(!do_op)
                vol_res = (127 - (brightness * (127 - (static_cast<uint32_t>(vol_res) & 127))) / 127);
        }
        WriteReg(port, 0x40 + cc + (4 * op), vol_res);
    }
    // Correct formula (ST3, AdPlug):
    //   63-((63-(instrvol))/63)*chanvol
    // Reduces to (tested identical):
    //   63 - chanvol + chanvol*instrvol/63
    // Also (slower, floats):
    //   63 + chanvol * (instrvol / 63.0 - 1)
}

void Generator::Patch(uint32_t c)
{
    uint8_t port = (c <= 2) ? 0 : 1;
    uint8_t cc   = c % 3;
    for(uint8_t op = 0; op < 4; op++)
    {
        WriteReg(port, 0x30 + (op * 4) + cc, m_patch.OPS[op].data[0]);
        WriteReg(port, 0x40 + (op * 4) + cc, m_patch.OPS[op].data[1]);
        WriteReg(port, 0x50 + (op * 4) + cc, m_patch.OPS[op].data[2]);
        WriteReg(port, 0x60 + (op * 4) + cc, m_patch.OPS[op].data[3]);
        WriteReg(port, 0x70 + (op * 4) + cc, m_patch.OPS[op].data[4]);
        WriteReg(port, 0x80 + (op * 4) + cc, m_patch.OPS[op].data[5]);
        WriteReg(port, 0x90 + (op * 4) + cc, m_patch.OPS[op].data[6]);
    }
    m_pan_lfo[c] = (m_pan_lfo[c] & 0xC0) | (m_patch.lfosens & 0x3F);
    WriteReg(port, 0xB0 + cc, m_patch.fbalg);
    WriteReg(port, 0xB4 + cc, m_pan_lfo[c]);
}

void Generator::Pan(uint32_t c, uint8_t value)
{
    uint8_t cc = c % 3;
    uint8_t port = (c <= 2) ? 0 : 1;
    m_pan_lfo[c] = (value & 0xC0) | (m_patch.lfosens & 0x3F);
    WriteReg(port, 0xB4 + cc, m_pan_lfo[c]);
}

void Generator::PlayNoteF(int noteID, uint32_t volume, uint8_t ccvolume, uint8_t ccexpr)
{
    if(!m_isInstrumentLoaded)
        return;//Deny playing notes without instrument loaded

    bool replace;
    int ch = m_noteManager.noteOn(noteID, volume, ccvolume, ccexpr, &replace);

    if(replace)
    {
        //if it replaces an old note, shut up the old one first
        //this lets the sustain take over with a fresh envelope
        NoteOff(ch);
    }

    PlayNoteCh(ch);
}

void Generator::PlayNoteCh(int ch, bool patch)
{
    if(!m_isInstrumentLoaded)
        return;//Deny playing notes without instrument loaded

    int tone;
    const NotesManager::Note &channel = m_noteManager.channel(ch);

    if(m_patch.tone)
    {
        tone = m_patch.tone;
        if(tone > 128)
            tone -= 128;
    }
    else
    {
        tone = channel.note;
    }

    m_debug.chan4op = int32_t(ch);

    double bend = 0.0;
    double phase = 0.0;

    if(patch)
    {
        Patch(ch);
        Pan(ch, 0xC0);
    }

    touchNote(ch, channel.volume, channel.ccvolume, channel.ccexpr);

    bend  = m_bend + m_patch.finetune;
    NoteOn(ch, std::exp(0.057762265 * (tone + bend + phase)));
}

void Generator::StopNoteF(int noteID)
{
    if(!m_isInstrumentLoaded)
        return;//Deny playing notes without instrument loaded

    int ch = m_noteManager.findNoteOffChannel(noteID);
    if (ch == -1)
        return;

    StopNoteCh(ch);
}

void Generator::StopNoteCh(int ch)
{
    if(!m_isInstrumentLoaded)
        return;//Deny playing notes without instrument loaded

    if(m_hold)
    {
        m_noteManager.hold(ch, true);  // stop later after hold is over
        return;
    }

    m_noteManager.channelOff(ch);

    NoteOff(ch);
}

void Generator::PitchBend(int bend)
{
    if(!m_isInstrumentLoaded)
        return;//Deny playing notes without instrument loaded

    m_bend = bend * m_bendsense;

    int channels = m_noteManager.channelCount();
    for(int ch = 0; ch < channels; ++ch)
    {
        const NotesManager::Note &channel = m_noteManager.channel(ch);
        if(channel.note != -1)
            PlayNoteCh(ch, false);  // updates frequency
    }
}

void Generator::PlayDrum(uint8_t drum, int noteID)
{
    if(!m_isInstrumentLoaded)
        return;//Deny playing notes without instrument loaded

    int tone = noteID;

    if(m_patch.tone)
    {
        tone = m_patch.tone;
        if(tone > 128)
            tone -= 128;
    }

    uint32_t adlchannel = 18 + drum;
    //Patch(adlchannel);
    Pan(adlchannel, 0xC0);
    touchNote(adlchannel, 127, 127, 127, 127);
    double bend = 0.0;
    double phase = 0.0;
    bend  = m_bend + m_patch.finetune;
    NoteOn(adlchannel, std::exp(0.057762265 * (tone + bend + phase)));
}

void Generator::Silence()
{
    //Shutup!
    for(uint32_t c = 0; c < NUM_OF_CHANNELS; ++c)
    {
        NoteOff(c);
        touchNote(c, 0, 0, 0);
    }

    m_noteManager.clearNotes();
}

void Generator::NoteOffAllChans()
{
    if(m_hold)
    {
        // mark all channels held for later key-off
        int channels = m_noteManager.channelCount();
        for(int ch = 0; ch < channels; ++ch)
        {
            const NotesManager::Note &channel = m_noteManager.channel(ch);
            if(channel.note != -1)
                m_noteManager.hold(ch, true);
        }
        return;
    }

    for(uint32_t c = 0; c < NUM_OF_CHANNELS; ++c)
        NoteOff(c);

    m_noteManager.clearNotes();
}



void Generator::PlayNote(uint32_t volume, uint8_t ccvolume, uint8_t ccexpr)
{
    PlayNoteF(note, volume, ccvolume, ccexpr);
}

void Generator::PlayMajorChord(int n, uint32_t volume, uint8_t ccvolume, uint8_t ccexpr)
{
    PlayNoteF(n - 12, volume, ccvolume, ccexpr);
    PlayNoteF(n, volume, ccvolume, ccexpr);
    PlayNoteF(n + 4, volume, ccvolume, ccexpr);
    PlayNoteF(n - 5, volume, ccvolume, ccexpr);
}

void Generator::PlayMinorChord(int n, uint32_t volume, uint8_t ccvolume, uint8_t ccexpr)
{
    PlayNoteF(n - 12, volume, ccvolume, ccexpr);
    PlayNoteF(n, volume, ccvolume, ccexpr);
    PlayNoteF(n + 3, volume, ccvolume, ccexpr);
    PlayNoteF(n - 5, volume, ccvolume, ccexpr);
}

void Generator::PlayAugmentedChord(int n, uint32_t volume, uint8_t ccvolume, uint8_t ccexpr)
{
    PlayNoteF(n - 12, volume, ccvolume, ccexpr);
    PlayNoteF(n, volume, ccvolume, ccexpr);
    PlayNoteF(n + 4, volume, ccvolume, ccexpr);
    PlayNoteF(n - 4, volume, ccvolume, ccexpr);
}

void Generator::PlayDiminishedChord(int n, uint32_t volume, uint8_t ccvolume, uint8_t ccexpr)
{
    PlayNoteF(n - 12, volume, ccvolume, ccexpr);
    PlayNoteF(n, volume, ccvolume, ccexpr);
    PlayNoteF(n + 3, volume, ccvolume, ccexpr);
    PlayNoteF(n - 6, volume, ccvolume, ccexpr);
}

void Generator::PlayMajor7Chord(int n, uint32_t volume, uint8_t ccvolume, uint8_t ccexpr)
{
    PlayNoteF(n - 12, volume, ccvolume, ccexpr);
    PlayNoteF(n - 2, volume, ccvolume, ccexpr);
    PlayNoteF(n, volume, ccvolume, ccexpr);
    PlayNoteF(n + 4, volume, ccvolume, ccexpr);
    PlayNoteF(n - 5, volume, ccvolume, ccexpr);
}

void Generator::PlayMinor7Chord(int n, uint32_t volume, uint8_t ccvolume, uint8_t ccexpr)
{
    PlayNoteF(n - 12, volume, ccvolume, ccexpr);
    PlayNoteF(n - 2, volume, ccvolume, ccexpr);
    PlayNoteF(n, volume, ccvolume, ccexpr);
    PlayNoteF(n + 3, volume, ccvolume, ccexpr);
    PlayNoteF(n - 5, volume, ccvolume, ccexpr);
}

void Generator::StopNote()
{
    StopNoteF(note);
}

void Generator::PitchBendSensitivity(int cents)
{
    m_bendsense = cents * (1e-2 / 8192);
}

void Generator::Hold(bool held)
{
    if(!m_isInstrumentLoaded)
        return;//Deny playing notes without instrument loaded

    if (m_hold == held)
        return;
    m_hold = held;

    if (!held)
    {
        // key-off all held notes now
        int channels = m_noteManager.channelCount();
        for(int ch = 0; ch < channels; ++ch)
        {
            const NotesManager::Note &channel = m_noteManager.channel(ch);
            if(channel.note != -1 && channel.held)
                StopNoteCh(ch);
        }
    }
}

void Generator::changePatch(const FmBank::Instrument &instrument, bool isDrum)
{
    //Shutup everything
    Silence();
    m_bend = 0.0;
    m_bendsense = 2.0 / 8192;
    //m_hold = false;
    {
        for(int op = 0; op < 4; op++)
        {
            m_patch.OPS[op].data[0] = instrument.getRegDUMUL(op);
            m_patch.OPS[op].data[1] = instrument.getRegLevel(op);
            m_patch.OPS[op].data[2] = instrument.getRegRSAt(op);
            m_patch.OPS[op].data[3] = instrument.getRegAMD1(op);
            m_patch.OPS[op].data[4] = instrument.getRegD2(op);
            m_patch.OPS[op].data[5] = instrument.getRegSysRel(op);
            m_patch.OPS[op].data[6] = instrument.getRegSsgEg(op);
        }
        m_patch.fbalg    = instrument.getRegFbAlg();
        m_patch.lfosens  = instrument.getRegLfoSens();
        m_patch.finetune = static_cast<int8_t>(instrument.note_offset1);
        m_patch.tone     = 0;

        if(isDrum || instrument.is_fixed_note)
            m_patch.tone = instrument.percNoteNum;
    }

    m_noteManager.allocateChannels(USED_CHANNELS_4OP);

    m_isInstrumentLoaded = true;//Mark instrument as loaded
}

void Generator::changeNote(int newnote)
{
    note = int32_t(newnote);
}



void Generator::changeLFO(bool enabled)
{
    lfo_enable = uint8_t(enabled);
    lfo_reg = (((lfo_enable << 3)&0x08) | (lfo_freq & 0x07)) & 0x0F;
    WriteReg(0, 0x22, lfo_reg);
}

void Generator::changeLFOfreq(int freq)
{
    lfo_freq = uint8_t(freq);
    lfo_reg = (((lfo_enable << 3)&0x08) | (lfo_freq & 0x07)) & 0x0F;
    WriteReg(0, 0x22, lfo_reg);
}

void Generator::changeVolumeModel(int volmodel)
{
    m_volumeScale = volmodel;
}

void Generator::generate(int16_t *frames, unsigned nframes)
{
    chip->generate(frames, nframes);
    // 2x Gain by default
    for(size_t i = 0; i < nframes * 2; ++i)
        frames[i] *= 2;
}

Generator::NotesManager::NotesManager()
{
    channels.reserve(USED_CHANNELS_4OP);
}

Generator::NotesManager::~NotesManager()
{}

void Generator::NotesManager::allocateChannels(int count)
{
    channels.clear();
    channels.resize(count);
    cycle = 0;
}

uint8_t Generator::NotesManager::noteOn(int note, uint32_t volume, uint8_t ccvolume, uint8_t ccexpr, bool *r)
{
    uint8_t beganAt = cycle;
    uint8_t chan = 0;

    // Increase age of all working notes;
    for(Note &ch : channels)
    {
        if(note >= 0)
            ch.age++;
    }

    bool replace = true;

    do
    {
        chan = cycle++;
        // Rotate cycles
        if(cycle == channels.size())
            cycle = 0;

        if(channels[chan].note == -1)
        {
            channels[chan].note = note;
            channels[chan].volume = volume;
            channels[chan].ccvolume = ccvolume;
            channels[chan].ccexpr = ccexpr;
            channels[chan].held = false;
            channels[chan].age = 0;
            replace = false;
            break;
        }

        if (cycle == beganAt) // If no free channels found
        {
            int age = -1;
            int oldest = -1;
            // Find oldest note
            for(uint8_t c = 0; c < channels.size(); c++)
            {
                if((channels[c].note >= 0) && ((age == -1) || (channels[c].age > age)))
                {
                    oldest = c;
                    age = channels[c].age;
                }
            }

            if(age >= 0)
            {
                chan = (uint8_t)oldest;
                channels[chan].note = note;
                channels[chan].volume = volume;
                channels[chan].ccvolume = ccvolume;
                channels[chan].ccexpr = ccexpr;
                channels[chan].held = false;
                channels[chan].age = 0;
            }
            break;
        }
    } while(1);

    if(r)
        *r = replace;

    return chan;
}

int8_t Generator::NotesManager::noteOff(int note)
{
    int8_t chan = findNoteOffChannel(note);
    if(chan != -1)
        channelOff(chan);
    return chan;
}

void Generator::NotesManager::channelOff(int ch)
{
    channels[ch].note = -1;
}

int8_t Generator::NotesManager::findNoteOffChannel(int note)
{
    // find the first active note not in held state (delayed noteoff)
    for(uint8_t chan = 0; chan < channels.size(); chan++)
    {
        if(channels[chan].note == note && !channels[chan].held)
            return (int8_t)chan;
    }
    return -1;
}

void Generator::NotesManager::hold(int ch, bool h)
{
    channels[ch].held = h;
}

void Generator::NotesManager::clearNotes()
{
    for(uint8_t chan = 0; chan < channels.size(); chan++)
        channels[chan].note = -1;
}
