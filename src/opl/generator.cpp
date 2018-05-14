/*
 * OPN2 Bank Editor by Wohlstand, a free tool for music bank editing
 * Copyright (c) 2017-2018 Vitaly Novichkov <admin@wohlnet.ru>
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

#define BEND_COEFFICIENT 321.88557

#define USED_CHANNELS_4OP       6

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
    chip->setRate(m_rate, 7670454);
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

        WriteReg(port, 0x28 + ch, 0x00);   //Key off to channel

        WriteReg(port, 0xA4 + ch, 0x68);   //Set frequency and octave
        WriteReg(port, 0xA0 + ch, 0xFF);
    }

    // OPN END
    Silence();
}

void Generator::switchChip(Generator::OPN_Chips chipId)
{
    switch(chipId)
    {
    case CHIP_GENS:
        chip.reset(new GensOPN2());
        break;
    case CHIP_Nuked:
        chip.reset(new NukedOPN2());
        break;
    case CHIP_MAME:
        chip.reset(new MameOPN2());
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
    uint8_t  cc     = c % 3;
    uint8_t  port   = (c <= 2) ? 0 : 1;
    uint16_t x2 = 0x0000;

    if(hertz < 0 || hertz > 262143) // Avoid infinite loop
        return;

    while((hertz >= 1023.75) && (x2 < 0x3800))
    {
        hertz /= 2.0;    // Calculate octave
        x2 += 0x800;
    }
    x2 += static_cast<uint32_t>(hertz + 0.5);
    WriteReg(port, 0xA4 + cc, (x2>>8) & 0xFF);//Set frequency and octave
    WriteReg(port, 0xA0 + cc,  x2 & 0xFF);
    WriteReg(0, 0x28, 0xF0 + uint8_t((c <= 2) ? c : c + 1));
}

void Generator::Touch_Real(uint32_t c, uint32_t volume)
{
    if(volume > 127)
        volume = 127;
    uint8_t cc   =  c % 3;
    uint8_t port = (c <= 2) ? 0 : 1;

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
    uint8_t alg = m_patch.fbalg & 0x07;
    for(uint8_t op = 0; op < 4; op++)
    {
        uint8_t x = op_vol[op];
        uint8_t vol_res = (alg_do[alg][op]) ? uint8_t(127 - (volume * (volume - (x&127)))/127) : x;
        WriteReg(port, 0x40 + cc + (4 * op), vol_res);
    }
    // Correct formula (ST3, AdPlug):
    //   63-((63-(instrvol))/63)*chanvol
    // Reduces to (tested identical):
    //   63 - chanvol + chanvol*instrvol/63
    // Also (slower, floats):
    //   63 + chanvol * (instrvol / 63.0 - 1)
}

void Generator::Touch(uint32_t c, uint32_t volume) // Volume maxes at 127*127*127
{
    // The formula below: SOLVE(V=127^3 * 2^( (A-63.49999) / 8), A)
    //Touch_Real(c, static_cast<uint32_t>(volume > 8725  ? std::log(volume) * 11.541561 + (0.5 - 104.22845) : 0));
    Touch_Real(c, volume);
    // The incorrect formula below: SOLVE(V=127^3 * (2^(A/63)-1), A)
    //Touch_Real(c, volume>11210 ? 91.61112 * std::log(4.8819E-7*volume + 1.0)+0.5 : 0);
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

void Generator::PlayNoteF(int noteID)
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

    int ch = m_noteManager.noteOn(noteID);
    m_debug.chan4op = int32_t(ch);

    double bend = 0.0;
    double phase = 0.0;

    Patch(ch);
    Pan(ch, 0xC0);
    Touch_Real(ch, 127);

    bend  = 0.0 + m_patch.finetune;
    NoteOn(ch, BEND_COEFFICIENT * std::exp(0.057762265 * (tone + bend + phase)));
}

void Generator::StopNoteF(int noteID)
{
    int ch = m_noteManager.noteOff(noteID);
    if (ch == -1)
        return;

    NoteOff(ch);
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
    Touch_Real(adlchannel, 127);
    double bend = 0.0;
    double phase = 0.0;
    bend  = 0.0 + m_patch.finetune;
    NoteOn(adlchannel, BEND_COEFFICIENT * std::exp(0.057762265 * (tone + bend + phase)));
}

void Generator::Silence()
{
    //Shutup!
    for(uint32_t c = 0; c < NUM_OF_CHANNELS; ++c)
    {
        NoteOff(c);
        Touch_Real(c, 0);
    }

    m_noteManager.clearNotes();
}

void Generator::NoteOffAllChans()
{
    for(uint32_t c = 0; c < NUM_OF_CHANNELS; ++c)
        NoteOff(c);

    m_noteManager.clearNotes();
}



void Generator::PlayNote()
{
    PlayNoteF(note);
}

void Generator::PlayMajorChord()
{
    PlayNoteF(note - 12);
    PlayNoteF(note);
    PlayNoteF(note + 4);
    PlayNoteF(note - 5);
}

void Generator::PlayMinorChord()
{
    PlayNoteF(note - 12);
    PlayNoteF(note);
    PlayNoteF(note + 3);
    PlayNoteF(note - 5);
}

void Generator::PlayAugmentedChord()
{
    PlayNoteF(note - 12);
    PlayNoteF(note);
    PlayNoteF(note + 4);
    PlayNoteF(note - 4);
}

void Generator::PlayDiminishedChord()
{
    PlayNoteF(note - 12);
    PlayNoteF(note);
    PlayNoteF(note + 3);
    PlayNoteF(note - 6);
}

void Generator::PlayMajor7Chord()
{
    PlayNoteF(note - 12);
    PlayNoteF(note - 2);
    PlayNoteF(note);
    PlayNoteF(note + 4);
    PlayNoteF(note - 5);
}

void Generator::PlayMinor7Chord()
{
    PlayNoteF(note - 12);
    PlayNoteF(note - 2);
    PlayNoteF(note);
    PlayNoteF(note + 3);
    PlayNoteF(note - 5);
}

void Generator::StopNote()
{
    StopNoteF(note);
}



void Generator::changePatch(const FmBank::Instrument &instrument, bool isDrum)
{
    //Shutup everything
    Silence();
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

        if(isDrum)
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

void Generator::generate(int16_t *frames, unsigned nframes)
{
    chip->generate(frames, nframes);
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

uint8_t Generator::NotesManager::noteOn(int note)
{
    uint8_t beganAt = cycle;
    uint8_t chan = 0;

    // Increase age of all working notes;
    for(Note &ch : channels)
    {
        if(note >= 0)
            ch.age++;
    }

    do
    {
        chan = cycle++;
        // Rotate cycles
        if(cycle == channels.size())
            cycle = 0;

        if(channels[chan].note == -1)
        {
            channels[chan].note = note;
            channels[chan].age = 0;
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
                channels[chan].age = 0;
            }
            break;
        }
    } while(1);

    return chan;
}

int8_t Generator::NotesManager::noteOff(int note)
{
    for(uint8_t chan = 0; chan < channels.size(); chan++)
    {
        if(channels[chan].note == note)
        {
            channels[chan].note = -1;
            return (int8_t)chan;
        }
    }
    return -1;
}

void Generator::NotesManager::clearNotes()
{
    for(uint8_t chan = 0; chan < channels.size(); chan++)
        channels[chan].note = -1;
}
