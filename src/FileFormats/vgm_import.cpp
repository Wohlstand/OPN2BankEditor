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

#include "vgm_import.h"
#include "../common.h"

const char magic_vgm[4] = {0x56, 0x67, 0x6D, 0x20};


bool VGM_Importer::detect(char *magic)
{
    return (memcmp(magic_vgm, magic, 4) == 0);
}

int VGM_Importer::loadFile(QString filePath, FmBank &bank)
{
    uint8_t ymram[2][0xFF];
    uint8_t ymram_prev[2][0xFF];
    char magic[4];
    memset(magic, 0, 4);
    memset(ymram[0], 0, 0xFF);
    memset(ymram[1], 0, 0xFF);
    memset(ymram_prev[0], 0, 0xFF);
    memset(ymram_prev[1], 0, 0xFF);
    uint8_t numb[4];

    QFile file(filePath);
    if(!file.open(QIODevice::ReadOnly))
        return ERR_NOFILE;

    bank.reset();
    if(file.read(magic, 4) != 4)
        return ERR_BADFORMAT;

    if(memcmp(magic, magic_vgm, 4) != 0)
        return ERR_BADFORMAT;

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
                if(memcmp(ymram[i], ymram_prev[i], 0xFF) != 0)
                {
                    for(uint8_t ch = 0; ch < 3; ch++)
                    {
                        FmBank::Instrument ins = FmBank::emptyInst();
                        snprintf(ins.name, 32, "Ins %d, channel %d", insCount++, (int)(ch + (3 * i)));
                        for(uint8_t op = 0; op < 4; op++)
                        {
                            ins.setRegDUMUL(op, ymram[i][0x30 + (op*4) + ch]);
                            ins.setRegLevel(op, ymram[i][0x40 + (op*4) + ch]);
                            ins.setRegRSAt(op, ymram[i][0x50 + (op*4) + ch]);
                            ins.setRegAMD1(op, ymram[i][0x60 + (op*4) + ch]);
                            ins.setRegD2(op, ymram[i][0x70 + (op*4) + ch]);
                            ins.setRegSysRel(op, ymram[i][0x80 + (op*4) + ch]);
                            ins.setRegSsgEg(op, ymram[i][0x90 + (op*4) + ch]);
                        }
                        ins.setRegFbAlg(ymram[i][0xB0 + ch]);
                        ins.setRegLfoSens(ymram[i][0xB4 + ch]);

                        bank.Ins_Melodic_box.push_back(ins);
                        bank.Ins_Melodic = bank.Ins_Melodic_box.data();
                    }
                    memcpy(ymram_prev[i], ymram[i], 0xFF);
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

    return ERR_OK;
}

int VGM_Importer::saveFile(QString, FmBank &)
{
    return ERR_UNSUPPORTED_FORMAT;
}


