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

#include "format_vgm_import.h"
#include "../common.h"

#include <QSet>
#include <QByteArray>
#include <algorithm>

const char magic_vgm[4] = {0x56, 0x67, 0x6D, 0x20};

bool VGM_Importer::detect(const QString &, char *magic)
{
    return (memcmp(magic_vgm, magic, 4) == 0);
}

FfmtErrCode VGM_Importer::loadFile(QString filePath, FmBank &bank)
{
    uint8_t ymram[2][0xFF];
    char magic[4];
    bool keys[6];
    memset(magic, 0, 4);
    memset(ymram[0], 0, 0xFF);
    memset(ymram[1], 0, 0xFF);
    memset(keys, 0, sizeof(bool)*6);
    uint8_t numb[4];

    QSet<QByteArray> cache;

    QFile file(filePath);
    if(!file.open(QIODevice::ReadOnly))
        return FfmtErrCode::ERR_NOFILE;

    bank.reset();
    if(file.read(magic, 4) != 4)
        return FfmtErrCode::ERR_BADFORMAT;

    if(memcmp(magic, magic_vgm, 4) != 0)
        return FfmtErrCode::ERR_BADFORMAT;

    file.seek(0x34);
    file.read(char_p(numb), 4);

    uint32_t data_offset = toUint32LE(numb);
    if(data_offset == 0x0C)
        data_offset = 0x40;
    file.seek(data_offset);

    bank.Ins_Melodic_box.clear();

    int insCount = 0;
    uint32_t pcm_offset = 0;
    bool end = false;
    bool dacOn = false;
    while(!end && !file.atEnd())
    {
        uint8_t cmd = 0x00;
        uint8_t reg = 0x00;
        uint8_t val = 0x00;
        file.read(char_p(&cmd), 1);
        switch(cmd)
        {
        case 0x52://Write port 0
            file.read(char_p(&reg), 1);
            file.read(char_p(&val), 1);
            //Get useful-only registers
            if( ((reg >= 0x30) && (reg <= 0x9F)) ||
                ((reg >= 0xB0) && (reg <= 0xB6)) )
                ymram[0][reg] = val;
            if(reg == 0x28)
            {
                switch(val&0x0F)
                {
                case 0: case 1: case 2: case 3:
                    keys[val&0x0F] = ((val>>4)&0x0F) != 0;
                    break;
                case 4: case 5: case 6:
                    keys[(val&0x0F) - 1] = ((val>>4)&0x0F) != 0;
                    break;
                }
            }
            if(reg == 0x22)
                bank.setRegLFO(val);
            if(reg == 0x2B)
                dacOn = (val != 0);
            break;
        case 0x53://Write port 1
            file.read(char_p(&reg), 1);
            file.read(char_p(&val), 1);
            //Get useful-only registers
            if( ((reg >= 0x30) && (reg <= 0x9F)) ||
                ((reg >= 0xB0) && (reg <= 0xB6)) )
                ymram[1][reg] = val;
            break;
        case 0x61://Wait samples
        case 0x62://Wait samples
        case 0x63://Wait samples
        case 0x70:
        case 0x71:
        case 0x72:
        case 0x73:
        case 0x74:
        case 0x75:
        case 0x76:
        case 0x77:
        case 0x78:
        case 0x79:
        case 0x7A:
        case 0x7B:
        case 0x7C:
        case 0x7D:
        case 0x7E:
        case 0x7F:
            if(cmd == 0x61)
                file.seek(file.pos() + 2);
            /* Analyze dumps and take the instruments */
            for(size_t i = 0; i < 2; i++)
            {
                for(uint8_t ch = 0; ch < 3; ch++)
                {
                    if(!keys[ch + 3*i])
                        continue;//Skip if key is not pressed
                    if(dacOn && (ch+ 3*i == 5))
                        continue;//Skip DAC channel
                    QByteArray insRaw;//Raw instrument
                    FmBank::Instrument ins = FmBank::emptyInst();
                    for(uint8_t op = 0; op < 4; op++)
                    {
                        ins.setRegDUMUL(op, ymram[i][0x30 + (op*4) + ch]);
                        ins.setRegLevel(op, ymram[i][0x40 + (op*4) + ch]);
                        ins.setRegRSAt(op,  ymram[i][0x50 + (op*4) + ch]);
                        ins.setRegAMD1(op,  ymram[i][0x60 + (op*4) + ch]);
                        ins.setRegD2(op,    ymram[i][0x70 + (op*4) + ch]);
                        ins.setRegSysRel(op,ymram[i][0x80 + (op*4) + ch]);
                        ins.setRegSsgEg(op, ymram[i][0x90 + (op*4) + ch]);
                        insRaw.push_back((char)ins.getRegDUMUL(op));
                        insRaw.push_back((char)ins.getRegLevel(op));
                        insRaw.push_back((char)ins.getRegRSAt(op));
                        insRaw.push_back((char)ins.getRegAMD1(op));
                        insRaw.push_back((char)ins.getRegD2(op));
                        insRaw.push_back((char)ins.getRegSysRel(op));
                        insRaw.push_back((char)ins.getRegSsgEg(op));
                    }

                    ins.setRegLfoSens(ymram[i][0xB4 + ch]);
                    ins.setRegFbAlg(ymram[i][0xB0 + ch]);

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

                    if(!cache.contains(insRaw))
                    {
                        snprintf(ins.name, 32, "Ins %d, channel %d", insCount++, (int)(ch + (3 * i)));
                        bank.Ins_Melodic_box.push_back(ins);
                        bank.Ins_Melodic = bank.Ins_Melodic_box.data();
                        cache.insert(insRaw);
                    }
                }
            }

            break;
        case 0x66://End of sound data
            end = 1;
            break;

        case 0x67://Data block to skip
            file.seek(file.pos() + 2);
            file.read(char_p(numb), 4);
            pcm_offset = toUint32LE(numb);
            file.seek(file.pos() + pcm_offset);
            break;
        case 0x50:
            file.seek(file.pos() + 1);
            //printf("PSG (SN76489/SN76496) write value, skip\n");
            break;
        case 0xE0:
            file.seek(file.pos() + 4);
            //printf("PCM offset, skip\n");
            break;
        case 0x80:
            file.seek(file.pos() + 1);
            //printf("PCM sample\n");
            break;
        case 0x4F:
            file.seek(file.pos() + 1);
            //printf("PCM sample\n");
            break;
        }
    }

    file.close();

    return FfmtErrCode::ERR_OK;
}

int VGM_Importer::formatCaps() const
{
    return (int)FormatCaps::FORMAT_CAPS_IMPORT;
}

QString VGM_Importer::formatName() const
{
    return "Video Game Music";
}

QString VGM_Importer::formatModuleName() const
{
    return "Video Game Music importer";
}

QString VGM_Importer::formatExtensionMask() const
{
    return "*.vgm";
}

BankFormats VGM_Importer::formatId() const
{
    return BankFormats::FORMAT_VGM_IMPORTER;
}
