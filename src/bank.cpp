/*
 * OPN2 Bank Editor by Wohlstand, a free tool for music bank editing
 * Copyright (c) 2017 Vitaly Novichkov <admin@wohlnet.ru>
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

#include "bank.h"
#include <memory.h>

//! Typedef to unsigned char
typedef unsigned char uchar;

//! Typedef to signed character pointer
typedef char         *char_p;

//! Typedef to unsigned integer
typedef unsigned int uint;

FmBank::FmBank()
{
    reset();
}

FmBank::FmBank(const FmBank &fb)
{
    reset();
    lfo_enabled     = fb.lfo_enabled;
    lfo_frequency   = fb.lfo_frequency;
    Ins_Melodic_box     = fb.Ins_Melodic_box;
    Ins_Percussion_box  = fb.Ins_Percussion_box;
    //int size = sizeof(Instrument)*128;
    //memcpy(Ins_Melodic,    fb.Ins_Melodic,    size);
    //memcpy(Ins_Percussion, fb.Ins_Percussion, size);
}

FmBank &FmBank::operator=(const FmBank &fb)
{
    if(this == &fb)
        return *this;

    reset();
    lfo_enabled     = fb.lfo_enabled;
    lfo_frequency   = fb.lfo_frequency;
    Ins_Melodic_box     = fb.Ins_Melodic_box;
    Ins_Percussion_box  = fb.Ins_Percussion_box;
    Ins_Melodic     = Ins_Melodic_box.data();
    Ins_Percussion  = Ins_Percussion_box.data();
    return *this;
}

bool FmBank::operator==(const FmBank &fb)
{
    bool res = true;
    res &= (Ins_Melodic_box.size() == fb.Ins_Melodic_box.size());
    res &= (Ins_Percussion_box.size() == fb.Ins_Percussion_box.size());
    lfo_enabled     = fb.lfo_enabled;
    lfo_frequency   = fb.lfo_frequency;
    if(res)
    {
        int size = Ins_Melodic_box.size() * static_cast<int>(sizeof(Instrument));
        res &= (memcmp(Ins_Melodic,      fb.Ins_Melodic,    static_cast<size_t>(size)) == 0);
        size = Ins_Percussion_box.size() * static_cast<int>(sizeof(Instrument));
        res &= (memcmp(Ins_Percussion,   fb.Ins_Percussion, static_cast<size_t>(size)) == 0);
    }

    return res;
}

bool FmBank::operator!=(const FmBank &fb)
{
    return !this->operator==(fb);
}

void FmBank::reset()
{
    size_t insnum = 128;
    size_t size = sizeof(Instrument) * insnum;
    lfo_enabled     = false;
    lfo_frequency   = 0;
    Ins_Melodic_box.resize(static_cast<int>(insnum));
    Ins_Percussion_box.resize(static_cast<int>(insnum));
    Ins_Melodic     = Ins_Melodic_box.data();
    Ins_Percussion  = Ins_Percussion_box.data();
    memset(Ins_Melodic,    0, size);
    memset(Ins_Percussion, 0, size);
}

unsigned char FmBank::getRegLFO()
{
    uchar out = 0;
    out |= 0x08 & (uchar(lfo_enabled) << 3);
    out |= 0x07 & uchar(lfo_frequency);
    return out;
}

void FmBank::setRegLFO(unsigned char lfo_reg)
{
    lfo_enabled     = ((lfo_reg & 0x08) >> 3);
    lfo_frequency   = 0x07 & lfo_reg;
}

FmBank::Instrument FmBank::emptyInst()
{
    FmBank::Instrument inst;
    memset(&inst, 0, sizeof(FmBank::Instrument));
    return inst;
}

bool FmBank::Instrument::operator==(const FmBank::Instrument &fb)
{
    if(memcmp(OP, fb.OP, sizeof(Operator)*4) != 0)
        return false;
    if(feedback != fb.feedback)
        return false;
    if(algorithm != fb.algorithm)
        return false;
    if(percNoteNum != fb.percNoteNum)
        return false;
    if(note_offset1 != fb.note_offset1)
        return false;
    if(fm != fb.fm)
        return false;
    if(am != fb.am)
        return false;
    return true;
}

bool FmBank::Instrument::operator!=(const FmBank::Instrument &fb)
{
    return !this->operator==(fb);
}

uint8_t FmBank::Instrument::getRegDUMUL(int OpID)
{
    uchar out = 0;
    out |= 0x70 & (uchar(OP[OpID].detune) << 4);
    out |= 0x0F & (uchar(OP[OpID].fmult));
    return out;
}

void FmBank::Instrument::setRegDUMUL(int OpID, uint8_t reg_dumul)
{
    OP[OpID].detune = (reg_dumul >> 4) & 0x07;
    OP[OpID].fmult = (reg_dumul) & 0x0F;
}

uint8_t FmBank::Instrument::getRegLevel(int OpID)
{
    uchar out = 0;
    out = OP[OpID].level & 0x7F;
    return out;
}

void FmBank::Instrument::setRegLevel(int OpID, uint8_t reg_level)
{
    OP[OpID].level = reg_level & 0x7F;
}

uint8_t FmBank::Instrument::getRegRSAt(int OpID)
{
    uchar out = 0;
    out |= 0xC0 & (uchar(OP[OpID].ratescale) << 6);
    out |= 0x1F & (uchar(OP[OpID].attack));
    return out;
}

void FmBank::Instrument::setRegRSAt(int OpID, uint8_t reg_rsat)
{
    OP[OpID].ratescale = (reg_rsat >> 6) & 0x03;
    OP[OpID].attack    = (reg_rsat) & 0x1F;
}

uint8_t FmBank::Instrument::getRegAMD1(int OpID)
{
    uchar out = 0;
    out |= 0x80 & (uchar(OP[OpID].am_enable) << 7);
    out |= 0x1F & (uchar(OP[OpID].decay1));
    return out;
}

void FmBank::Instrument::setRegAMD1(int OpID, uint8_t reg_amd1)
{
    OP[OpID].am_enable = (reg_amd1 >> 7) & 0x01;
    OP[OpID].decay1 = (reg_amd1) & 0x1F;
}

uint8_t FmBank::Instrument::getRegD2(int OpID)
{
    uchar out = 0;
    out |= 0x1F & (uchar(OP[OpID].decay2));
    return out;
}

void FmBank::Instrument::setRegD2(int OpID, uint8_t reg_d2)
{
    OP[OpID].decay2 = reg_d2 & 0x1F;
}

uint8_t FmBank::Instrument::getRegSysRel(int OpID)
{
    uchar out = 0;
    out |= 0xF0 & (uchar(OP[OpID].sustain) << 4);
    out |= 0x0F & (uchar(OP[OpID].release));
    return out;
}

void FmBank::Instrument::setRegSysRel(int OpID, uint8_t reg_sysrel)
{
    OP[OpID].sustain    = (reg_sysrel >> 4) & 0x0F;
    OP[OpID].release    = (reg_sysrel) & 0x0F;
}

uint8_t FmBank::Instrument::getRegSsgEg(int OpID)
{
    uchar out = 0;
    out |= 0x0F & (uchar(OP[OpID].ssg_eg));
    return out;
}

void FmBank::Instrument::setRegSsgEg(int OpID, uint8_t reg_ssgeg)
{
    OP[OpID].ssg_eg = reg_ssgeg & 0x0F;
}

uint8_t FmBank::Instrument::getRegFbAlg()
{
    uint8_t out = 0;
    out |= 0x38 & (uchar(feedback) << 3);
    out |= 0x07 & (uchar(algorithm));
    return out;
}

void FmBank::Instrument::setRegFbAlg(uint8_t reg_ssgeg)
{
    feedback    = (reg_ssgeg >> 3) & 0x07;
    algorithm   = (reg_ssgeg) & 0x07;
}

uint8_t FmBank::Instrument::getRegLfoSens()
{
    uchar out = 0;
    out |= 0x30 & (uchar(am) << 4);
    out |= 0x07 & (uchar(fm));
    return out;
}

void FmBank::Instrument::setRegLfoSens(uint8_t reg_lfosens)
{
    am = (reg_lfosens >> 4) & 0x03;
    fm = (reg_lfosens) & 0x07;
}
