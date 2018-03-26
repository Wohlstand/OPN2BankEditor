/*
 * OPN2 Bank Editor by Wohlstand, a free tool for music bank editing
 * Copyright (c) 2017-2018 Vitaly Novichkov <admin@wohlnet.ru>
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

//!Initial version
static const char *wopn2_magic1 = "WOPN2-BANK\0";
static const char *wopni_magic1 = "WOPN2-INST\0";

//!All next versions
static const char *wopn2_magic2 = "WOPN2-B2NK\0";
static const char *wopni_magic2 = "WOPN2-IN2T\0";

#define WOPL_INST_SIZE_V1 65
#define WOPL_INST_SIZE_V2 69

static const uint16_t latest_version = 2;

bool WohlstandOPN2::detect(const QString &, char *magic)
{
    return (strncmp(magic, wopn2_magic1, 11) == 0) || (strncmp(magic, wopn2_magic2, 11) == 0);
}

bool WohlstandOPN2::detectInst(const QString &, char *magic)
{
    return (strncmp(magic, wopni_magic1, 11) == 0) || (strncmp(magic, wopni_magic2, 11) == 0);
}

static bool readInstrument(QFile &file, FmBank::Instrument &ins, uint16_t &version, bool hasSoundKoefficients = true)
{
    uint8_t idata[WOPL_INST_SIZE_V2];
    if(version >= 2)
    {
        if(file.read(char_p(idata), WOPL_INST_SIZE_V2) != WOPL_INST_SIZE_V2)
            return false;
    }
    else
    {
        if(file.read(char_p(idata), WOPL_INST_SIZE_V1) != WOPL_INST_SIZE_V1)
            return false;
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
    if(version >= 2 && hasSoundKoefficients)
    {
        ins.ms_sound_kon  = toUint16BE(idata + 65);
        ins.ms_sound_koff = toUint16BE(idata + 67);
        ins.is_blank = (ins.ms_sound_kon == 0) && (ins.ms_sound_koff == 0);
    }
    return true;
}

static bool writeInstrument(QFile &file, FmBank::Instrument &ins, bool hasSoundKoefficients = true)
{
    uint8_t odata[WOPL_INST_SIZE_V2];
    memset(odata, 0, WOPL_INST_SIZE_V2);
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

    if(hasSoundKoefficients)
    {
        if(ins.is_blank)
        {
            fromUint16BE(0, odata + 65);
            fromUint16BE(0, odata + 67);
        }
        else
        {
            fromUint16BE(ins.ms_sound_kon,  odata + 65);
            fromUint16BE(ins.ms_sound_koff, odata + 67);
        }
        return (file.write(char_p(odata), WOPL_INST_SIZE_V2) == WOPL_INST_SIZE_V2);
    } else
        return (file.write(char_p(odata), WOPL_INST_SIZE_V1) == WOPL_INST_SIZE_V1);
}

FfmtErrCode WohlstandOPN2::loadFile(QString filePath, FmBank &bank)
{
    uint16_t count_melodic_banks     = 1;
    uint16_t count_percusive_banks   = 1;
    uint16_t version = 1;

    char magic[32];
    memset(magic, 0, 32);

    QFile file(filePath);
    if(!file.open(QIODevice::ReadOnly))
        return FfmtErrCode::ERR_NOFILE;

    bank.reset();
    if(file.read(magic, 11) != 11)
        return FfmtErrCode::ERR_BADFORMAT;

    bool is1 = strncmp(magic, wopn2_magic1, 11) == 0;
    bool is2 = strncmp(magic, wopn2_magic2, 11) == 0;
    if(!is1 && !is2)
        return FfmtErrCode::ERR_BADFORMAT;
    if(is2)
    {
        uint8_t ver[2];
        if(file.read(char_p(ver), 2) != 2)
            return FfmtErrCode::ERR_BADFORMAT;
        version = toUint16LE(ver);
        if(version < 2 || version > latest_version)
            return FfmtErrCode::ERR_UNSUPPORTED_FORMAT;
    }

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

    if(version >= 2)//Read bank meta-entries
    {
        for(int i = 0; i < bank.Banks_Melodic.size(); i++)
        {
            FmBank::MidiBank &bankMeta = bank.Banks_Melodic[i];
            uint8_t bank_meta[34];
            if(file.read(char_p(bank_meta), 34) != 34)
                return FfmtErrCode::ERR_BADFORMAT;
            strncpy(bankMeta.name, char_p(bank_meta), 32);
            bankMeta.lsb = bank_meta[32];
            bankMeta.msb = bank_meta[33];
        }

        for(int i = 0; i < bank.Banks_Percussion.size(); i++)
        {
            FmBank::MidiBank &bankMeta = bank.Banks_Percussion[i];
            uint8_t bank_meta[34];
            if(file.read(char_p(bank_meta), 34) != 34)
                return FfmtErrCode::ERR_BADFORMAT;
            strncpy(bankMeta.name, char_p(bank_meta), 32);
            bankMeta.lsb = bank_meta[32];
            bankMeta.msb = bank_meta[33];
        }
    }

    uint16_t total = 128 * count_melodic_banks;
    bool readPercussion = false;

tryAgain:
    for(uint16_t i = 0; i < total; i++)
    {
        FmBank::Instrument &ins = (readPercussion) ? bank.Ins_Percussion[i] : bank.Ins_Melodic[i];
        if(!readInstrument(file, ins, version))
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
    file.write(char_p(wopn2_magic2), 11);
    writeLE(file, latest_version);
    uint8_t head[5];
    fromUint16BE(count_melodic_banks, head);
    fromUint16BE(count_percusive_banks, head + 2);
    head[4] = bank.getRegLFO();
    file.write(char_p(head), 5);

    //For Version 2: BEGIN
    //Write melodic banks meta-data
    for(int i = 0; i < bank.Banks_Melodic.size(); i++)
    {
        const FmBank::MidiBank &bankMeta = bank.Banks_Melodic[i];
        uint8_t bank_meta[34];
        strncpy(char_p(bank_meta), bankMeta.name, 32);
        bank_meta[32] = bankMeta.lsb;
        bank_meta[33] = bankMeta.msb;
        if(file.write(char_p(bank_meta), 34) != 34)
            return FfmtErrCode::ERR_BADFORMAT;
    }

    //Write percussion banks meta-data
    for(int i = 0; i < bank.Banks_Percussion.size(); i++)
    {
        const FmBank::MidiBank &bankMeta = bank.Banks_Percussion[i];
        uint8_t bank_meta[34];
        strncpy(char_p(bank_meta), bankMeta.name, 32);
        bank_meta[32] = bankMeta.lsb;
        bank_meta[33] = bankMeta.msb;
        if(file.write(char_p(bank_meta), 34) != 34)
            return FfmtErrCode::ERR_BADFORMAT;
    }
    //For Version 2: END

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

int WohlstandOPN2::formatCaps() const
{
    return (int)FormatCaps::FORMAT_CAPS_EVERYTHING;
}

QString WohlstandOPN2::formatName() const
{
    return "Standard OPN2 bank by Wohlstand";
}

QString WohlstandOPN2::formatExtensionMask() const
{
    return "*.wopn";
}

BankFormats WohlstandOPN2::formatId() const
{
    return BankFormats::FORMAT_WOHLSTAND_OPN2;
}



FfmtErrCode WohlstandOPN2::loadFileInst(QString filePath, FmBank::Instrument &inst, bool *isDrum)
{
    char magic[32];
    memset(magic, 0, 32);
    uint8_t isDrumFlag = 0;
    uint16_t version = 1;
    QFile file(filePath);

    if(!file.open(QIODevice::ReadOnly))
        return FfmtErrCode::ERR_NOFILE;
    if(file.read(magic, 11) != 11)
        return FfmtErrCode::ERR_BADFORMAT;
    bool is1 = (strncmp(magic, wopni_magic1, 11) == 0);
    bool is2 = (strncmp(magic, wopni_magic2, 11) == 0);
    if(!is1 && !is2)
        return FfmtErrCode::ERR_BADFORMAT;
    if(is2)
    {
        uint8_t ver[2];
        if(file.read(char_p(ver), 2) != 2)
            return FfmtErrCode::ERR_BADFORMAT;
        version = toUint16LE(ver);
        if(version < 2 || version > latest_version)
            return FfmtErrCode::ERR_UNSUPPORTED_FORMAT;
    }
    if(file.read(char_p(&isDrumFlag), 1) != 1)
        return FfmtErrCode::ERR_BADFORMAT;
    if(isDrum)
        *isDrum = bool(isDrumFlag);
    if(!readInstrument(file, inst, version, false))
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
    if(file.write(char_p(wopni_magic2), 11) != 11)
        return FfmtErrCode::ERR_BADFORMAT;
    if(writeLE(file, latest_version) != 2)
        return FfmtErrCode::ERR_BADFORMAT;
    if(file.write(char_p(&isDrumFlag), 1) != 1)
        return FfmtErrCode::ERR_BADFORMAT;
    if(!writeInstrument(file, inst, false))
        return FfmtErrCode::ERR_BADFORMAT;
    file.close();

    return FfmtErrCode::ERR_OK;
}

int WohlstandOPN2::formatInstCaps() const
{
    return (int)FormatCaps::FORMAT_CAPS_EVERYTHING;
}

QString WohlstandOPN2::formatInstName() const
{
    return "Standard OPN2 instrument by Wohlstand";
}

QString WohlstandOPN2::formatInstExtensionMask() const
{
    return "*.opni";
}

InstFormats WohlstandOPN2::formatInstId() const
{
    return InstFormats::FORMAT_INST_WOPN2;
}
