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

//! Typedef to signed character pointer
typedef char         *char_p;

FmBank::FmBank()
{
    reset();
}

FmBank::FmBank(const FmBank &fb)
{
    reset();
    Ins_Melodic_box     = fb.Ins_Melodic_box;
    Ins_Percussion_box  = fb.Ins_Percussion_box;
    Ins_Melodic         = Ins_Melodic_box.data();
    Ins_Percussion      = Ins_Percussion_box.data();
    Banks_Melodic       = fb.Banks_Melodic;
    Banks_Percussion    = fb.Banks_Percussion;
    lfo_enabled         = fb.lfo_enabled;
    lfo_frequency       = fb.lfo_frequency;
}

FmBank &FmBank::operator=(const FmBank &fb)
{
    if(this == &fb)
        return *this;

    reset();
    Ins_Melodic_box     = fb.Ins_Melodic_box;
    Ins_Percussion_box  = fb.Ins_Percussion_box;
    Ins_Melodic         = Ins_Melodic_box.data();
    Ins_Percussion      = Ins_Percussion_box.data();
    Banks_Melodic       = fb.Banks_Melodic;
    Banks_Percussion    = fb.Banks_Percussion;
    lfo_enabled         = fb.lfo_enabled;
    lfo_frequency       = fb.lfo_frequency;
    return *this;
}

bool FmBank::operator==(const FmBank &fb)
{
    bool res = true;
    res &= (lfo_enabled == fb.lfo_enabled);
    res &= (lfo_frequency == fb.lfo_frequency);
    res &= (Ins_Melodic_box.size() == fb.Ins_Melodic_box.size());
    res &= (Ins_Percussion_box.size() == fb.Ins_Percussion_box.size());
    res &= (Banks_Melodic.size() == fb.Banks_Melodic.size());
    res &= (Banks_Percussion.size() == fb.Banks_Percussion.size());
    if(res)
    {
        int size = Ins_Melodic_box.size() * static_cast<int>(sizeof(Instrument));
        res &= (memcmp(Ins_Melodic,      fb.Ins_Melodic,    static_cast<size_t>(size)) == 0);
        size = Ins_Percussion_box.size() * static_cast<int>(sizeof(Instrument));
        res &= (memcmp(Ins_Percussion,   fb.Ins_Percussion, static_cast<size_t>(size)) == 0);
        size = Banks_Melodic.size() * static_cast<int>(sizeof(MidiBank));
        res &= (memcmp(Banks_Melodic.data(),   fb.Banks_Melodic.data(), static_cast<size_t>(size)) == 0);
        size = Banks_Percussion.size() * static_cast<int>(sizeof(MidiBank));
        res &= (memcmp(Banks_Percussion.data(),   fb.Banks_Percussion.data(), static_cast<size_t>(size)) == 0);
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
    size_t banksnum = insnum / 128;
    size_t size = sizeof(Instrument) * insnum;
    Ins_Melodic_box.resize(static_cast<int>(insnum));
    Ins_Percussion_box.resize(static_cast<int>(insnum));
    Ins_Melodic     = Ins_Melodic_box.data();
    Ins_Percussion  = Ins_Percussion_box.data();
    Banks_Melodic.resize(static_cast<int>(banksnum));
    Banks_Percussion.resize(static_cast<int>(banksnum));
    memset(Ins_Melodic,    0, size);
    memset(Ins_Percussion, 0, size);
    size = sizeof(MidiBank) * banksnum;
    memset(Banks_Melodic.data(), 0, size);
    memset(Banks_Percussion.data(), 0, size);
    lfo_enabled     = false;
    lfo_frequency   = 0;
}

void FmBank::reset(uint16_t melodic_banks, uint16_t percussion_banks)
{
    size_t insnum = 128;
    size_t size = sizeof(Instrument) * insnum;
    Ins_Melodic_box.resize(static_cast<int>(insnum * melodic_banks));
    Ins_Percussion_box.resize(static_cast<int>(insnum * percussion_banks));
    Ins_Melodic     = Ins_Melodic_box.data();
    Ins_Percussion  = Ins_Percussion_box.data();
    Banks_Melodic.resize(static_cast<int>(melodic_banks));
    Banks_Percussion.resize(static_cast<int>(percussion_banks));
    memset(Ins_Melodic,    0, size * melodic_banks);
    memset(Ins_Percussion, 0, size * percussion_banks);
    size = sizeof(MidiBank) * melodic_banks;
    memset(Banks_Melodic.data(), 0, size);
    size = sizeof(MidiBank) * percussion_banks;
    memset(Banks_Percussion.data(), 0, size);
    lfo_enabled     = false;
    lfo_frequency   = 0;
}

uint8_t FmBank::getRegLFO() const
{
    uint8_t out = 0;
    out |= 0x08 & (uint8_t(lfo_enabled) << 3);
    out |= 0x07 & uint8_t(lfo_frequency);
    return out;
}

void FmBank::setRegLFO(uint8_t lfo_reg)
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

FmBank::MidiBank FmBank::emptyBank(uint16_t index)
{
    FmBank::MidiBank bank;
    memset(&bank, 0, sizeof(FmBank::MidiBank));
    bank.lsb = ((index >> 0) & 0xFF);
    bank.msb = ((index >> 8) & 0xFF);
    bank.name[0] = '\0';
    return bank;
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

uint8_t FmBank::Instrument::getRegDUMUL(int OpID) const
{
    uint8_t out = 0;
    out |= 0x70 & (uint8_t(OP[OpID].detune) << 4);
    out |= 0x0F & (uint8_t(OP[OpID].fmult));
    return out;
}

void FmBank::Instrument::setRegDUMUL(int OpID, uint8_t reg_dumul)
{
    OP[OpID].detune = (reg_dumul >> 4) & 0x07;
    OP[OpID].fmult = (reg_dumul) & 0x0F;
}

uint8_t FmBank::Instrument::getRegLevel(int OpID) const
{
    uint8_t out = 0;
    out = OP[OpID].level & 0x7F;
    return out;
}

void FmBank::Instrument::setRegLevel(int OpID, uint8_t reg_level)
{
    OP[OpID].level = reg_level & 0x7F;
}

uint8_t FmBank::Instrument::getRegRSAt(int OpID) const
{
    uint8_t out = 0;
    out |= 0xC0 & (uint8_t(OP[OpID].ratescale) << 6);
    out |= 0x1F & (uint8_t(OP[OpID].attack));
    return out;
}

void FmBank::Instrument::setRegRSAt(int OpID, uint8_t reg_rsat)
{
    OP[OpID].ratescale = (reg_rsat >> 6) & 0x03;
    OP[OpID].attack    = (reg_rsat) & 0x1F;
}

uint8_t FmBank::Instrument::getRegAMD1(int OpID) const
{
    uint8_t out = 0;
    out |= 0x80 & (uint8_t(OP[OpID].am_enable) << 7);
    out |= 0x1F & (uint8_t(OP[OpID].decay1));
    return out;
}

void FmBank::Instrument::setRegAMD1(int OpID, uint8_t reg_amd1)
{
    OP[OpID].am_enable = (reg_amd1 >> 7) & 0x01;
    OP[OpID].decay1 = (reg_amd1) & 0x1F;
}

uint8_t FmBank::Instrument::getRegD2(int OpID) const
{
    uint8_t out = 0;
    out |= 0x1F & (uint8_t(OP[OpID].decay2));
    return out;
}

void FmBank::Instrument::setRegD2(int OpID, uint8_t reg_d2)
{
    OP[OpID].decay2 = reg_d2 & 0x1F;
}

uint8_t FmBank::Instrument::getRegSysRel(int OpID) const
{
    uint8_t out = 0;
    out |= 0xF0 & (uint8_t(OP[OpID].sustain) << 4);
    out |= 0x0F & (uint8_t(OP[OpID].release));
    return out;
}

void FmBank::Instrument::setRegSysRel(int OpID, uint8_t reg_sysrel)
{
    OP[OpID].sustain    = (reg_sysrel >> 4) & 0x0F;
    OP[OpID].release    = (reg_sysrel) & 0x0F;
}

uint8_t FmBank::Instrument::getRegSsgEg(int OpID) const
{
    uint8_t out = 0;
    out |= 0x0F & (uint8_t(OP[OpID].ssg_eg));
    return out;
}

void FmBank::Instrument::setRegSsgEg(int OpID, uint8_t reg_ssgeg)
{
    OP[OpID].ssg_eg = reg_ssgeg & 0x0F;
}

uint8_t FmBank::Instrument::getRegFbAlg() const
{
    uint8_t out = 0;
    out |= 0x38 & (uint8_t(feedback) << 3);
    out |= 0x07 & (uint8_t(algorithm));
    return out;
}

void FmBank::Instrument::setRegFbAlg(uint8_t reg_ssgeg)
{
    feedback    = (reg_ssgeg >> 3) & 0x07;
    algorithm   = (reg_ssgeg) & 0x07;
}

uint8_t FmBank::Instrument::getRegLfoSens() const
{
    uint8_t out = 0;
    out |= 0x30 & (uint8_t(am) << 4);
    out |= 0x07 & (uint8_t(fm));
    return out;
}

void FmBank::Instrument::setRegLfoSens(uint8_t reg_lfosens)
{
    am = (reg_lfosens >> 4) & 0x03;
    fm = (reg_lfosens) & 0x07;
}

TmpBank::TmpBank(FmBank &bank, int minMelodic, int minPercusive)
{
    insMelodic = bank.Ins_Melodic;
    insPercussion = bank.Ins_Percussion;
    if(bank.Ins_Melodic_box.size() < minMelodic)
    {
        tmpMelodic = bank.Ins_Melodic_box;
        tmpMelodic.reserve(128 - tmpMelodic.size());
        while(tmpMelodic.size() < 128)
            tmpMelodic.push_back(FmBank::emptyInst());
        insMelodic = tmpMelodic.data();
    }

    if(bank.Ins_Percussion_box.size() < minPercusive)
    {
        tmpPercussion = bank.Ins_Percussion_box;
        tmpPercussion.reserve(128 - tmpPercussion.size());
        while(tmpPercussion.size() < 128)
            tmpPercussion.push_back(FmBank::emptyInst());
        insPercussion = tmpPercussion.data();
    }
}
