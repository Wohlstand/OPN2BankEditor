/*
 * OPN2 Bank Editor by Wohlstand, a free tool for music bank editing
 * Copyright (c) 2017-2024 Vitaly Novichkov <admin@wohlnet.ru>
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

#include "ym2151_to_wopi.h"
#include "vgm_import_options.h"
#include <cstring>

RawYm2151ToWopi::RawYm2151ToWopi()
{
    m_insdata.reset(new InstrumentData);
    reset();
}

void RawYm2151ToWopi::reset()
{
    InstrumentData &insdata = *m_insdata;
    insdata.cache.clear();
    insdata.caughtInstruments.clear();

    std::memset(m_keys, 0, sizeof(m_keys));
    std::memset(m_ymram, 0, sizeof(m_ymram));
}

void RawYm2151ToWopi::passReg(uint8_t reg, uint8_t val)
{
    if(reg == 0x08)
        m_keys[val & 3] = (val >> 3) & 15;

    m_ymram[reg] = val;
}

void RawYm2151ToWopi::doAnalyzeState()
{
    InstrumentData &insdata = *m_insdata;

    /* Analyze dumps and take the instruments */
    for(uint8_t ch = 0; ch < 8; ch++)
    {
        if(m_keys[ch] == 0)
            continue;//Skip if key is not pressed

        QByteArray insRaw;//Raw instrument
        FmBank::Instrument ins = FmBank::emptyInst();
        for(uint8_t op = 0; op < 4; op++)
        {
            ins.setRegDUMUL(op, m_ymram[0x40 + (op * 8) + ch]);
            ins.setRegLevel(op, m_ymram[0x60 + (op * 8) + ch]);
            ins.setRegRSAt(op,  m_ymram[0x80 + (op * 8) + ch]);
            ins.setRegAMD1(op,  m_ymram[0xa0 + (op * 8) + ch]);
            ins.setRegD2(op,    m_ymram[0xc0 + (op * 8) + ch]);
            ins.setRegSysRel(op,m_ymram[0xe0 + (op * 8) + ch]);
            insRaw.push_back((char)ins.getRegDUMUL(op));
            insRaw.push_back((char)ins.getRegLevel(op));
            insRaw.push_back((char)ins.getRegRSAt(op));
            insRaw.push_back((char)ins.getRegAMD1(op));
            insRaw.push_back((char)ins.getRegD2(op));
            insRaw.push_back((char)ins.getRegSysRel(op));
            insRaw.push_back((char)ins.getRegSsgEg(op));
        }

        ins.am = m_ymram[0x38] & 3;
        ins.fm = (m_ymram[0x38] >> 4) & 7;
        ins.algorithm = m_ymram[0x20] & 7;
        ins.feedback = (m_ymram[0x20] >> 3) & 7;

        if(g_vgmImportOptions.ignoreLfoAmplitudeChanges)
            ins.am = 0;

        if(g_vgmImportOptions.ignoreLfoFrequencyChanges)
            ins.fm = 0;

        insRaw.push_back((char)ins.getRegLfoSens());
        insRaw.push_back((char)ins.getRegFbAlg());

        if(g_vgmImportOptions.maximiseVolume)
        {
            /* Maximize key volume */
            uint8_t olevels[4] =
            {
                ins.OP[OPERATOR1].level,
                ins.OP[OPERATOR2].level,
                ins.OP[OPERATOR3].level,
                ins.OP[OPERATOR4].level
            };

            uint8_t dec = 0;
            switch(ins.algorithm)
            {
            case 0:case 1: case 2: case 3:
                ins.OP[OPERATOR4].level = 0;
                break;
            case 4:
                dec = std::min({olevels[OPERATOR3], olevels[OPERATOR4]});
                ins.OP[OPERATOR3].level = olevels[OPERATOR3] - dec;
                ins.OP[OPERATOR4].level = olevels[OPERATOR4] - dec;
                break;
            case 5:
                dec = std::min({olevels[OPERATOR2], olevels[OPERATOR3], olevels[OPERATOR4]});
                ins.OP[OPERATOR2].level = olevels[OPERATOR2] - dec;
                ins.OP[OPERATOR3].level = olevels[OPERATOR3] - dec;
                ins.OP[OPERATOR4].level = olevels[OPERATOR4] - dec;
                break;
            case 6:
                dec = std::min({olevels[OPERATOR2], olevels[OPERATOR3], olevels[OPERATOR4]});
                ins.OP[OPERATOR2].level = olevels[OPERATOR2] - dec;
                ins.OP[OPERATOR3].level = olevels[OPERATOR3] - dec;
                ins.OP[OPERATOR4].level = olevels[OPERATOR4] - dec;
                break;
            case 7:
                dec = std::min({olevels[OPERATOR1], olevels[OPERATOR2], olevels[OPERATOR3], olevels[OPERATOR4]});
                ins.OP[OPERATOR1].level = olevels[OPERATOR1] - dec;
                ins.OP[OPERATOR2].level = olevels[OPERATOR2] - dec;
                ins.OP[OPERATOR3].level = olevels[OPERATOR3] - dec;
                ins.OP[OPERATOR4].level = olevels[OPERATOR4] - dec;
                break;
            }

            //Encode volume bytes back
            insRaw[1 + (OPERATOR1 * 7)] = (char)ins.getRegLevel(OPERATOR1);
            insRaw[1 + (OPERATOR2 * 7)] = (char)ins.getRegLevel(OPERATOR2);
            insRaw[1 + (OPERATOR3 * 7)] = (char)ins.getRegLevel(OPERATOR3);
            insRaw[1 + (OPERATOR4 * 7)] = (char)ins.getRegLevel(OPERATOR4);
        }

        if(!insdata.cache.contains(insRaw))
        {
            std::snprintf(ins.name, 32,
                          "Ins %d, channel %d",
                          insdata.caughtInstruments.size(),
                          (int)ch);
            insdata.caughtInstruments.push_back(ins);
            insdata.cache.insert(insRaw);
        }
    }
}

const QList<FmBank::Instrument> &RawYm2151ToWopi::caughtInstruments()
{
    return m_insdata->caughtInstruments;
}
