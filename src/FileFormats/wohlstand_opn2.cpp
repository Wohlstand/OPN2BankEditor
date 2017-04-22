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

#include "wohlstand_opn2.h"
#include "../common.h"

static const char *wopn2_magic = "WOPN2-BANK\0";

bool WohlstandOPN2::detect(char *magic)
{
    return (strncmp(magic, wopn2_magic, 11) == 0);
}

int WohlstandOPN2::loadFile(QString filePath, FmBank &bank)
{
    unsigned short count_melodic_banks     = 1;
    unsigned short count_percusive_banks   = 1;
    char magic[32];
    memset(magic, 0, 32);

    QFile file(filePath);
    if(!file.open(QIODevice::ReadOnly))
        return ERR_NOFILE;

    bank.reset();
    if(file.read(magic, 11) != 11)
        return ERR_BADFORMAT;

    if(strncmp(magic, wopn2_magic, 11) != 0)
        return ERR_BADFORMAT;

    uint8_t head[5];
    memset(head, 0, 5);
    if(file.read(char_p(head), 5) != 5)
        return ERR_BADFORMAT;

    count_melodic_banks     = toUint16BE(head);
    count_percusive_banks   = toUint16BE(head + 2);
    bank.setRegLFO(head[4]);

    unsigned short total = 128 * count_melodic_banks;
    bool readPercussion = false;

tryAgain:
    for(unsigned short i = 0; i < total; i++)
    {
        FmBank::Instrument &ins = (readPercussion) ? bank.Ins_Percussion[i] : bank.Ins_Melodic[i];
        unsigned char idata[65];
        if(file.read(char_p(idata), 65) != 65)
        {
            bank.reset();
            return ERR_BADFORMAT;
        }
        strncpy(ins.name, char_p(idata), 32);
        ins.note_offset1 = toSint16BE(idata + 32);
        ins.percNoteNum  = idata[34];
        ins.setRegFbAlg(idata[35]);
        ins.setRegLfoSens(idata[36]);

        for(int op = 0; op < 4; op++)
        {
            int off = 37 + op * 7;
            ins.setRegDUMUL(op, idata[off + 0]);
            ins.setRegLevel(op, idata[off + 1]);
            ins.setRegRSAt(op,  idata[off + 2]);
            ins.setRegAMD1(op,  idata[off + 3]);
            ins.setRegD2(op,    idata[off + 4]);
            ins.setRegSysRel(op,idata[off + 5]);
            ins.setRegSsgEg(op, idata[off + 6]);
        }
    }
    if(!readPercussion)
    {
        total = 128 * count_percusive_banks;
        readPercussion = true;
        goto tryAgain;
    }
    file.close();

    return ERR_OK;
}

int WohlstandOPN2::saveFile(QString filePath, FmBank &bank)
{
    FmBank::Instrument null;
    memset(&null, 0, sizeof(FmBank::Instrument));

    unsigned short count_melodic_banks     = 1;
    unsigned short count_percusive_banks   = 1;

    QFile file(filePath);
    if(!file.open(QIODevice::WriteOnly))
        return ERR_NOFILE;

    //Write header
    file.write(char_p(wopn2_magic), 11);
    uint8_t head[5];
    fromUint16BE(count_melodic_banks, head);
    fromUint16BE(count_percusive_banks, head + 2);
    head[4] = bank.getRegLFO();
    file.write(char_p(head), 5);

    unsigned short total = 128 * count_melodic_banks;
    bool wrtiePercussion = false;

tryAgain:
    for(uint16_t i = 0; i < total; i++)
    {
        FmBank::Instrument &ins = (wrtiePercussion) ? bank.Ins_Percussion[i] : bank.Ins_Melodic[i];
        uint8_t odata[65];
        memset(odata, 0, 65);
        strncpy(char_p(odata), ins.name, 32);
        fromSint16BE(ins.note_offset1, odata + 32);
        odata[34] = ins.percNoteNum;
        odata[35] = ins.getRegFbAlg();
        odata[36] = ins.getRegLfoSens();

        for(int op = 0; op < 4; op++)
        {
            int off = 37 + op * 7;
            odata[off + 0] = ins.getRegDUMUL(op);
            odata[off + 1] = ins.getRegLevel(op);
            odata[off + 2] = ins.getRegRSAt(op);
            odata[off + 3] = ins.getRegAMD1(op);
            odata[off + 4] = ins.getRegD2(op);
            odata[off + 5] = ins.getRegSysRel(op);
            odata[off + 6] = ins.getRegSsgEg(op);
        }
        if(file.write(char_p(odata), 65) != 65)
            return ERR_BADFORMAT;
    }
    if(!wrtiePercussion)
    {
        total = 128 * count_percusive_banks;
        wrtiePercussion = true;
        goto tryAgain;
    }

    file.close();

    return ERR_OK;
}
