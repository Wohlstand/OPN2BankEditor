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

#ifndef YM2151_TO_WOPI_H
#define YM2151_TO_WOPI_H

#include <stdint.h>
#include <memory>

#include "../bank.h"
#include "ym2612_to_wopi.h" // instrument sharing

class RawYm2151ToWopi
{
    typedef RawYm2612ToWopi::InstrumentData InstrumentData;
    uint8_t m_keys[8];
    uint8_t m_ymram[0xFF];
    std::shared_ptr<InstrumentData> m_insdata;

public:
    RawYm2151ToWopi();
    void reset();
    template <class Ym> void shareInstruments(Ym &other) { m_insdata = other.m_insdata; }
    void passReg(uint8_t reg, uint8_t val);
    void doAnalyzeState();
    const QList<FmBank::Instrument> &caughtInstruments();
};

#endif // YM2151_TO_WOPI_H
