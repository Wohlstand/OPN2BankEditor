/*
 * OPN2 Bank Editor by Wohlstand, a free tool for music bank editing
 * Copyright (c) 2017-2019 Vitaly Novichkov <admin@wohlnet.ru>
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

#ifndef YM2612_TO_WOPI_H
#define YM2612_TO_WOPI_H

#include <stdint.h>
#include <memory>
#include <QSet>
#include <QList>

#include "../bank.h"

class RawYm2612ToWopi
{
    struct InstrumentData
    {
        QSet<QByteArray> cache;
        QList<FmBank::Instrument> caughtInstruments;
    };

    uint8_t m_ymram[2][0xFF];
    char m_magic[4];
    bool m_keys[6];
    uint8_t m_lfoVal = 0;
    bool m_dacOn = false;
    std::shared_ptr<InstrumentData> m_insdata;

public:
    RawYm2612ToWopi();
    void reset();
    void shareInstruments(RawYm2612ToWopi &other);
    void passReg(uint8_t port, uint8_t reg, uint8_t val);
    void doAnalyzeState();
    const QList<FmBank::Instrument> &caughtInstruments();
};

#endif // YM2612_TO_WOPI_H
