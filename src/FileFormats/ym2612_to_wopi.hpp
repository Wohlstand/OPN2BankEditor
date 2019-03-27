
#include <stdint.h>
#include <memory>
#include <cstring>
#include <QSet>
#include <QList>

#include "../bank.h"

#ifndef YM2612_TO_WOPI_HPP
#define YM2612_TO_WOPI_HPP

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
    RawYm2612ToWopi()
    {
        m_insdata.reset(new InstrumentData);
        reset();
    }

    void reset()
    {
        InstrumentData &insdata = *m_insdata;
        insdata.cache.clear();
        insdata.caughtInstruments.clear();

        std::memset(m_magic, 0, 4);
        std::memset(m_ymram[0], 0, 0xFF);
        std::memset(m_ymram[1], 0, 0xFF);
        std::memset(m_keys, 0, sizeof(bool) * 6);
        m_lfoVal = 0;
        m_dacOn = false;
    }

    void shareInstruments(RawYm2612ToWopi &other)
    {
        m_insdata = other.m_insdata;
    }

    void passReg(uint8_t port, uint8_t reg, uint8_t val)
    {
        if(port == 0)
        {
        //Get useful-only registers
        if( ((reg >= 0x30) && (reg <= 0x9F)) ||
            ((reg >= 0xB0) && (reg <= 0xB6)) )
            m_ymram[port][reg] = val;
        if(reg == 0x28)
        {
            switch(val&0x0F)
            {
            case 0: case 1: case 2: case 3:
                m_keys[val&0x0F] = ((val>>4)&0x0F) != 0;
                break;
            case 4: case 5: case 6:
                m_keys[(val&0x0F) - 1] = ((val>>4)&0x0F) != 0;
                break;
            }
        }
        if(reg == 0x22)
            m_lfoVal = val;
        if(reg == 0x2B)
            m_dacOn = (val != 0);
        }
        else
        if(port == 1)
        {
            //Get useful-only registers
            if( ((reg >= 0x30) && (reg <= 0x9F)) ||
                ((reg >= 0xB0) && (reg <= 0xB6)) )
                m_ymram[1][reg] = val;
        }
    }

    void doAnalyzeState()
    {
        InstrumentData &insdata = *m_insdata;

        /* Analyze dumps and take the instruments */
        for(size_t i = 0; i < 2; i++)
        {
            for(uint8_t ch = 0; ch < 3; ch++)
            {
                if(!m_keys[ch + 3*i])
                    continue;//Skip if key is not pressed
                if(m_dacOn && (ch+ 3*i == 5))
                    continue;//Skip DAC channel
                QByteArray insRaw;//Raw instrument
                FmBank::Instrument ins = FmBank::emptyInst();
                for(uint8_t op = 0; op < 4; op++)
                {
                    ins.setRegDUMUL(op, m_ymram[i][0x30 + (op * 4) + ch]);
                    ins.setRegLevel(op, m_ymram[i][0x40 + (op * 4) + ch]);
                    ins.setRegRSAt(op,  m_ymram[i][0x50 + (op * 4) + ch]);
                    ins.setRegAMD1(op,  m_ymram[i][0x60 + (op * 4) + ch]);
                    ins.setRegD2(op,    m_ymram[i][0x70 + (op * 4) + ch]);
                    ins.setRegSysRel(op,m_ymram[i][0x80 + (op * 4) + ch]);
                    ins.setRegSsgEg(op, m_ymram[i][0x90 + (op * 4) + ch]);
                    insRaw.push_back((char)ins.getRegDUMUL(op));
                    insRaw.push_back((char)ins.getRegLevel(op));
                    insRaw.push_back((char)ins.getRegRSAt(op));
                    insRaw.push_back((char)ins.getRegAMD1(op));
                    insRaw.push_back((char)ins.getRegD2(op));
                    insRaw.push_back((char)ins.getRegSysRel(op));
                    insRaw.push_back((char)ins.getRegSsgEg(op));
                }

                ins.setRegLfoSens(m_ymram[i][0xB4 + ch]);
                ins.setRegFbAlg(m_ymram[i][0xB0 + ch]);

                insRaw.push_back((char)ins.getRegLfoSens());
                insRaw.push_back((char)ins.getRegFbAlg());

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
                insRaw[1 + OPERATOR1*7] = (char)ins.getRegLevel(OPERATOR1);
                insRaw[1 + OPERATOR2*7] = (char)ins.getRegLevel(OPERATOR2);
                insRaw[1 + OPERATOR3*7] = (char)ins.getRegLevel(OPERATOR3);
                insRaw[1 + OPERATOR4*7] = (char)ins.getRegLevel(OPERATOR4);

                if(!insdata.cache.contains(insRaw))
                {
                    std::snprintf(ins.name, 32,
                                  "Ins %d, channel %d",
                                  insdata.caughtInstruments.size(),
                                  (int)(ch + (3 * i))
                    );
                    insdata.caughtInstruments.push_back(ins);
                    insdata.cache.insert(insRaw);
                }
            }
        }
    }

    const QList<FmBank::Instrument> &caughtInstruments()
    {
        return m_insdata->caughtInstruments;
    }
};

#endif // YM2612_TO_WOPI_HPP
