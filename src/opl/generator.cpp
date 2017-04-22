/*
 * OPL Bank Editor by Wohlstand, a free tool for music bank editing
 * Copyright (c) 2016 Vitaly Novichkov <admin@wohlnet.ru>
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

#define BEND_COEFFICIENT 172.4387

#define USED_CHANNELS_2OP       18
#define USED_CHANNELS_2OP_PS4   9
#define USED_CHANNELS_4OP       6

Generator::Generator(uint32_t sampleRate,
                     QObject *parent)
    :   QIODevice(parent)
{
    note = 60;
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

    //Init chip
    chip2.set_rate(sampleRate, 6350400.0);

    lfo_reg = 0x00;
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

        WriteReg(port, 0x60 + ch, 0x5 );   //AM/D1R operator 1
        WriteReg(port, 0x64 + ch, 0x5 );   //AM/D1R operator 2
        WriteReg(port, 0x68 + ch, 0x5 );   //AM/D1R operator 3
        WriteReg(port, 0x6C + ch, 0x7 );   //AM/D1R operator 4

        WriteReg(port, 0x70 + ch, 0x2 );   //D2R operator 1
        WriteReg(port, 0x74 + ch, 0x2 );   //D2R operator 2
        WriteReg(port, 0x78 + ch, 0x2 );   //D2R operator 3
        WriteReg(port, 0x7C + ch, 0x2 );   //D2R operator 4

        WriteReg(port, 0x80 + ch, 0x11);   //D1L/RR Operator 1
        WriteReg(port, 0x84 + ch, 0x11);   //D1L/RR Operator 2
        WriteReg(port, 0x88 + ch, 0x11);   //D1L/RR Operator 3
        WriteReg(port, 0x8C + ch, 0xA6);   //D1L/RR Operator 4

        WriteReg(port, 0x90 + ch, 0x0 );   //Proprietary shit
        WriteReg(port, 0x94 + ch, 0x0 );   //Proprietary shit
        WriteReg(port, 0x98 + ch, 0x0 );   //Proprietary shit
        WriteReg(port, 0x9C + ch, 0x0 );   //Proprietary shit

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

Generator::~Generator()
{}

void Generator::WriteReg(uint8_t port, uint16_t address, uint8_t byte)
{
    switch(port)
    {
    case 0:
        chip2.write0(address, byte);
        break;
    case 1:
        chip2.write1(address, byte);
        break;
    }
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

    if(hertz < 0 || hertz > 131071) // Avoid infinite loop
        return;

    while(hertz >= 1023.5)
    {
        hertz /= 2.0;    // Calculate octave
        x2 += 0x800;
    }
    x2 += static_cast<uint32_t>(hertz + 0.5);
    WriteReg(port, 0xA0 + cc,  x2 & 0xFF);
    WriteReg(port, 0xA4 + cc, (x2>>8) & 0xFF);//Set frequency and octave
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
        m_patch.OPS[0].data[1],
        m_patch.OPS[1].data[1],
        m_patch.OPS[2].data[1],
        m_patch.OPS[3].data[1],
    };

    bool alg_do[8][4] =
    {
        //OP1   OP2   OP3   OP4
        {false,false,false,true},
        {false,false,false,true},
        {false,false,false,true},
        {false,false,false,true},
        {false,true, true, true},
        {false,true, true, true},
        {false,true, true, true},
        {true, true, true, true},
    };
    uint8_t alg = m_patch.fbalg & 0x07;
    for(uint8_t op = 0; op < 4; op++)
    {
        uint8_t x = op_vol[op];
        WriteReg(port,
                 0x40 + cc + (4 * op),
                 (alg_do[alg][op]) ? uint8_t((x|127) - volume + volume * (x&127)/127) : x
                 );
    }
    // Correct formula (ST3, AdPlug):
    //   63-((63-(instrvol))/63)*chanvol
    // Reduces to (tested identical):
    //   63 - chanvol + chanvol*instrvol/63
    // Also (slower, floats):
    //   63 + chanvol * (instrvol / 63.0 - 1)
}

void Generator::Touch(unsigned c, unsigned volume) // Volume maxes at 127*127*127
{
    // The formula below: SOLVE(V=127^3 * 2^( (A-63.49999) / 8), A)
    //Touch_Real(c, static_cast<unsigned int>(volume > 8725  ? std::log(volume) * 11.541561 + (0.5 - 104.22845) : 0));
    Touch_Real(c, volume / 3u);
    // The incorrect formula below: SOLVE(V=127^3 * (2^(A/63)-1), A)
    //Touch_Real(c, volume>11210 ? 91.61112 * std::log(4.8819E-7*volume + 1.0)+0.5 : 0);
}

void Generator::Patch(unsigned c)
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

void Generator::Pan(unsigned c, uint8_t value)
{
    uint8_t cc = c % 3;
    uint8_t port = (c <= 2) ? 0 : 1;
    m_pan_lfo[c] = (value & 0xC0) | (m_patch.lfosens & 0x3F);
    WriteReg(port, 0xB4 + cc, m_pan_lfo[c]);
}

void Generator::PlayNoteF(int noteID)
{
    static uint32_t chan4op = 5;
    static struct DebugInfo
    {
        int32_t chan4op;
        QString toStr()
        {
            return QString("Channels:\n"
                           "4-op: %1")
                   .arg(this->chan4op);
        }
    } _debug {-1};

    int tone = noteID + 2;//Seems OPN lacks one tone, let's add two half-tones to fix that

    if(m_patch.tone)
    {
        if(m_patch.tone < 20)
            tone += m_patch.tone;
        else if(m_patch.tone < 128)
            tone = m_patch.tone;
        else
            tone -= m_patch.tone - 128;
    }

    chan4op++;
    if(chan4op > (USED_CHANNELS_4OP - 1))
        chan4op = 0;
    _debug.chan4op = chan4op;

    emit debugInfo(_debug.toStr());
    double bend = 0.0;
    double phase = 0.0;

    Patch(chan4op);
    Pan(chan4op, 0xC0);
    Touch_Real(chan4op, 127);

    bend  = 0.0 + m_patch.finetune;
    NoteOn(chan4op, BEND_COEFFICIENT * std::exp(0.057762265 * (tone + bend + phase)));
}

void Generator::PlayDrum(uint8_t drum, int noteID)
{
    int tone = noteID;

    if(m_patch.tone)
    {
        if(m_patch.tone < 20)
            tone += m_patch.tone;
        else if(m_patch.tone < 128)
            tone = m_patch.tone;
        else
            tone -= m_patch.tone - 128;
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
    for(unsigned c = 0; c < NUM_OF_CHANNELS; ++c)
    {
        NoteOff(c);
        Touch_Real(c, 0);
    }
}

void Generator::NoteOffAllChans()
{
    for(unsigned c = 0; c < NUM_OF_CHANNELS; ++c)
        NoteOff(c);
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



void Generator::changePatch(FmBank::Instrument &instrument, bool isDrum)
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
        {
            if(instrument.percNoteNum && instrument.percNoteNum < 20)
            {
                uchar nnum = instrument.percNoteNum;
                while(nnum && nnum < 20)
                {
                    nnum += 12;
                    m_patch.finetune -= 12;
                }
            }
        }
    }
}

void Generator::changeNote(int32_t newnote)
{
    note = newnote;
}



void Generator::changeLFO(bool enabled)
{
    lfo_enable = uint8_t(enabled);
    lfo_reg = (((lfo_enable << 3)&0x08) | (lfo_freq & 0x07)) & 0x0F;
    chip2.write0(0x22, lfo_reg);
}

void Generator::changeLFOfreq(int freq)
{
    lfo_freq = uint8_t(freq);
    lfo_reg = (((lfo_enable << 3)&0x08) | (lfo_freq & 0x07)) & 0x0F;
    chip2.write0(0x22, lfo_reg);
}



void Generator::start()
{
    open(QIODevice::ReadOnly);
}

void Generator::stop()
{
    close();
}

qint64 Generator::readData(char *data, qint64 len)
{
    int16_t *_out = reinterpret_cast<short *>(data);
    len -= len % 4; //must be multiple 4!
    uint32_t lenS = (static_cast<uint32_t>(len) / 4);
    memset(_out, 0, len);
    chip2.run(lenS, _out);
    return len;
}

qint64 Generator::writeData(const char *data, qint64 len)
{
    Q_UNUSED(data);
    Q_UNUSED(len);
    return 0;
}

qint64 Generator::bytesAvailable() const
{
    return 4096;// + QIODevice::bytesAvailable();
}
