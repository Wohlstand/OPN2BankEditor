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

#include "format_wohlstand_opn2.h"
#include "../common.h"

static const char *wopn2_magic = "WOPN2-BANK\0";
static const char *wopni_magic = "WOPN2-INST\0";

bool WohlstandOPN2::detect(const QString &, char *magic)
{
    return (strncmp(magic, wopn2_magic, 11) == 0);
}

bool WohlstandOPN2::detectInst(const QString &, char *magic)
{
    return (strncmp(magic, wopni_magic, 11) == 0);
}

static bool readInstrument(QFile &file, FmBank::Instrument &ins)
{
    uint8_t idata[65];
    if(file.read(char_p(idata), 65) != 65)
        return false;
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
    return true;
}

static bool writeInstrument(QFile &file, FmBank::Instrument &ins)
{
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
    return (file.write(char_p(odata), 65) == 65);
}

FfmtErrCode WohlstandOPN2::loadFile(QString filePath, FmBank &bank)
{
    uint16_t count_melodic_banks     = 1;
    uint16_t count_percusive_banks   = 1;

    char magic[32];
    memset(magic, 0, 32);

    QFile file(filePath);
    if(!file.open(QIODevice::ReadOnly))
        return FfmtErrCode::ERR_NOFILE;

    bank.reset();
    if(file.read(magic, 11) != 11)
        return FfmtErrCode::ERR_BADFORMAT;

    if(strncmp(magic, wopn2_magic, 11) != 0)
        return FfmtErrCode::ERR_BADFORMAT;

    uint8_t head[5];
    memset(head, 0, 5);
    if(file.read(char_p(head), 5) != 5)
        return FfmtErrCode::ERR_BADFORMAT;

    count_melodic_banks     = toUint16BE(head);
    count_percusive_banks   = toUint16BE(head + 2);

    if((count_melodic_banks < 1) || (count_percusive_banks < 1))
        return FfmtErrCode::ERR_BADFORMAT;

    bank.reset(count_melodic_banks, count_percusive_banks);

    bank.setRegLFO(head[4]);

    uint16_t total = 128 * count_melodic_banks;
    bool readPercussion = false;

tryAgain:
    for(uint16_t i = 0; i < total; i++)
    {
        FmBank::Instrument &ins = (readPercussion) ? bank.Ins_Percussion[i] : bank.Ins_Melodic[i];
        if(!readInstrument(file, ins))
        {
            bank.reset();
            return FfmtErrCode::ERR_BADFORMAT;
        }
    }

    if(!readPercussion)
    {
        total = 128 * count_percusive_banks;
        readPercussion = true;
        goto tryAgain;
    }
    file.close();

    return FfmtErrCode::ERR_OK;
}

FfmtErrCode WohlstandOPN2::saveFile(QString filePath, FmBank &bank)
{
    FmBank::Instrument null;
    memset(&null, 0, sizeof(FmBank::Instrument));

    uint16_t count_melodic_banks     = uint16_t(((bank.countMelodic() - 1)/ 128) + 1);
    uint16_t count_percusive_banks   = uint16_t(((bank.countDrums() - 1)/ 128) + 1);

    QFile file(filePath);
    if(!file.open(QIODevice::WriteOnly))
        return FfmtErrCode::ERR_NOFILE;

    //Write header
    file.write(char_p(wopn2_magic), 11);
    uint8_t head[5];
    fromUint16BE(count_melodic_banks, head);
    fromUint16BE(count_percusive_banks, head + 2);
    head[4] = bank.getRegLFO();
    file.write(char_p(head), 5);

    uint16_t total = 128 * count_melodic_banks;
    uint16_t total_insts = uint16_t(bank.Ins_Melodic_box.size());
    bool wrtiePercussion = false;
    FmBank::Instrument *insts = bank.Ins_Melodic;

tryAgain:
    for(uint16_t i = 0; i < total; i++)
    {
        if(i < total_insts)
        {
            FmBank::Instrument &ins = insts[i];
            if(!writeInstrument(file, ins))
                return FfmtErrCode::ERR_BADFORMAT;
        }
        else
        {
            if(!writeInstrument(file, null))
                return FfmtErrCode::ERR_BADFORMAT;
        }
    }

    if(!wrtiePercussion)
    {
        total = 128 * count_percusive_banks;
        insts = bank.Ins_Percussion;
        total_insts = uint16_t(bank.Ins_Percussion_box.size());
        wrtiePercussion = true;
        goto tryAgain;
    }

    file.close();

    return FfmtErrCode::ERR_OK;
}

int WohlstandOPN2::formatCaps()
{
    return (int)FormatCaps::FORMAT_CAPS_EVERYTHING;
}

QString WohlstandOPN2::formatName()
{
    return "Standard OPN2 bank by Wohlstand";
}

QString WohlstandOPN2::formatExtensionMask()
{
    return "*.wopn";
}

BankFormats WohlstandOPN2::formatId()
{
    return BankFormats::FORMAT_WOHLSTAND_OPN2;
}



FfmtErrCode WohlstandOPN2::loadFileInst(QString filePath, FmBank::Instrument &inst, bool *isDrum)
{
    char magic[32];
    memset(magic, 0, 32);
    uint8_t isDrumFlag = 0;
    QFile file(filePath);

    if(!file.open(QIODevice::ReadOnly))
        return FfmtErrCode::ERR_NOFILE;
    if(file.read(magic, 11) != 11)
        return FfmtErrCode::ERR_BADFORMAT;
    if(strncmp(magic, wopni_magic, 11) != 0)
        return FfmtErrCode::ERR_BADFORMAT;
    if(file.read(char_p(&isDrumFlag), 1) != 1)
        return FfmtErrCode::ERR_BADFORMAT;
    if(isDrum)
        *isDrum = bool(isDrumFlag);
    if(!readInstrument(file, inst))
        return FfmtErrCode::ERR_BADFORMAT;
    file.close();

    return FfmtErrCode::ERR_OK;
}

FfmtErrCode WohlstandOPN2::saveFileInst(QString filePath, FmBank::Instrument &inst, bool isDrum)
{
    uint8_t isDrumFlag = uint8_t(isDrum);
    QFile file(filePath);

    if(!file.open(QIODevice::WriteOnly))
        return FfmtErrCode::ERR_NOFILE;
    if(file.write(char_p(wopni_magic), 11) != 11)
        return FfmtErrCode::ERR_BADFORMAT;
    if(file.write(char_p(&isDrumFlag), 1) != 1)
        return FfmtErrCode::ERR_BADFORMAT;
    if(!writeInstrument(file, inst))
        return FfmtErrCode::ERR_BADFORMAT;
    file.close();

    return FfmtErrCode::ERR_OK;
}

int WohlstandOPN2::formatInstCaps()
{
    return (int)FormatCaps::FORMAT_CAPS_EVERYTHING;
}

QString WohlstandOPN2::formatInstName()
{
    return "Standard OPN2 instrument by Wohlstand";
}

QString WohlstandOPN2::formatInstExtensionMask()
{
    return "*.opni";
}

InstFormats WohlstandOPN2::formatInstId()
{
    return InstFormats::FORMAT_INST_WOPN2;
}
