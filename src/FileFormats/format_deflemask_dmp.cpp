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

#include "format_deflemask_dmp.h"
#include "../common.h"

#include <QSet>
#include <QFileInfo>
#include <QByteArray>
#include <algorithm>

bool DefleMask::detectInst(const QString &filePath, char * /*magic*/)
{
    //By name extension
    if(filePath.endsWith(".dmp", Qt::CaseInsensitive))
        return true;

    //By file size :-P (it is going between 49 and 51 bytes)
    QFileInfo f(filePath);
    if((f.size() >= 49) && (f.size() <= 51))
        return true;

    return false;
}

static uint8_t opn1_to_opn2[7]
{
    7, 6, 5,
    0,
    1, 2, 3
};

static uint8_t opn2_to_opn1[8]
{
    3,
    4, 5, 6,
    3,
    2, 1, 0
};

static uint8_t detuneToOPN2(uint8_t d)
{
    if(d > 7) return 0;
    return opn1_to_opn2[d];
}

static uint8_t detuneFromOPN2(uint8_t d)
{
    if(d > 8) return 3;
    return opn2_to_opn1[d];
}

FfmtErrCode DefleMask::loadFileInst(QString filePath, FmBank::Instrument &inst, bool *isDrum)
{
    uint8_t idata[51];
    QFile file(filePath);

    if(!file.open(QIODevice::ReadOnly))
        return FfmtErrCode::ERR_NOFILE;

    memset(idata, 0, 51);
    if(file.read(char_p(idata), 51) < 48)
        return FfmtErrCode::ERR_BADFORMAT;

    memset(&inst, 0, sizeof(FmBank::Instrument));
    size_t head_offset = 3;
    size_t data_offset = 7;

    switch(idata[0])
    {
    case 0x00: // Oldest format
        head_offset = 1;
        data_offset = 5;
        break;

    case 0x01: case 0x02: case 0x03: case 0x04: case 0x05:
    case 0x06: case 0x07: case 0x08:
        /* FIXME: Check validy of those versions!!! */

    case 0x0A:
        head_offset = 2;
        data_offset = 6;
        if(idata[1] != 0x01) // FM only is supported, STANDARD is not supported
            return FfmtErrCode::ERR_UNSUPPORTED_FORMAT;
        break;

    case 0x09:
        head_offset = 3;
        data_offset = 7;
        if(idata[1] != 0x01) // FM only is supported, STANDARD is not supported
            return FfmtErrCode::ERR_UNSUPPORTED_FORMAT;
        break;

    case 0x0B:
        head_offset = 3;
        data_offset = 7;
        if(idata[1] != 0x02) // Genesis type is only supported
            return FfmtErrCode::ERR_UNSUPPORTED_FORMAT;
        if(idata[2] != 0x01) // FM only is supported, STANDARD is not supported
            return FfmtErrCode::ERR_UNSUPPORTED_FORMAT;
        break;
    default:
        return FfmtErrCode::ERR_UNSUPPORTED_FORMAT;
    }

    //3: 1 Byte: LFO (FMS on YM2612, PMS on YM2151)
    inst.fm = idata[head_offset + 0] & 0x07;
    //4: 1 Byte: FB
    inst.feedback  = idata[head_offset + 1] & 0x07;
    //5: 1 Byte: ALG
    inst.algorithm = idata[head_offset + 2] & 0x07;
    //6: 1 Byte: LFO2 (AMS on YM2612, AMS on YM2151)
    inst.am = idata[head_offset + 3] & 0x03;

    for(size_t op = 0; op < 4; op++)
    {
        uint8_t *op_off = idata + data_offset + (op * 11);
        //1 Byte: MULT
        inst.OP[op].fmult = op_off[0] & 0x0F;
        //1 Byte: TL
        inst.OP[op].level = op_off[1] & 0x7F;
        //1 Byte: AR
        inst.OP[op].attack = op_off[2] & 0x7F;
        //1 Byte: DR
        inst.OP[op].decay1 = op_off[3] & 0x7F;
        //1 Byte: SL
        inst.OP[op].sustain = op_off[4] & 0x0F;
        //1 Byte: RR
        inst.OP[op].release = op_off[5] & 0x0F;
        //1 Byte: AM
        inst.OP[op].am_enable = (op_off[6] != 0);
        //1 Byte: RS
        inst.OP[op].ratescale = op_off[7] & 0x03;
        //1 Byte: DT (DT2<<4 | DT on YM2151)
        inst.OP[op].detune = detuneToOPN2(op_off[8] & 0x0F);
        //1 Byte: D2R
        inst.OP[op].decay2 = op_off[9] & 0x7F;
        //1 Byte: SSGEG_Enabled <<3 | SSGEG
        inst.OP[op].ssg_eg = op_off[10] & 0x0F;
    }

    if(isDrum)
        *isDrum = false;

    QByteArray name = QFileInfo(filePath).baseName().toUtf8();
    strncpy(inst.name, name.data(), name.size() < 32 ? (size_t)name.size() : 32);

    file.close();

    return FfmtErrCode::ERR_OK;
}

FfmtErrCode DefleMask::saveFileInst(QString filePath, FmBank::Instrument &inst, bool /*isDrum*/)
{
    uint8_t idata[51];
    QFile file(filePath);

    if(!file.open(QIODevice::WriteOnly))
        return FfmtErrCode::ERR_NOFILE;

    memset(idata, 0, 51);
    idata[0] = 0x0B;//DefleMask v. 0.12.0
    idata[1] = 0x02;//SYSTEM_GENESIS
    idata[2] = 0x01;//Instrument Mode: 1=FM

    //3: 1 Byte: LFO (FMS on YM2612, PMS on YM2151)
    idata[3] = inst.fm;
    //4: 1 Byte: FB
    idata[4] = inst.feedback;
    //5: 1 Byte: ALG
    idata[5] = inst.algorithm;
    //6: 1 Byte: LFO2 (AMS on YM2612, AMS on YM2151)
    idata[6] = inst.am;

    for(size_t op = 0; op < 4; op++)
    {
        uint8_t *op_off = idata + 7 + (op * 11);
        //1 Byte: MULT
        op_off[0] = inst.OP[op].fmult;
        //1 Byte: TL
        op_off[1] = inst.OP[op].level;
        //1 Byte: AR
        op_off[2] = inst.OP[op].attack;
        //1 Byte: DR
        op_off[3] = inst.OP[op].decay1;
        //1 Byte: SL
        op_off[4] = inst.OP[op].sustain;
        //1 Byte: RR
        op_off[5] = inst.OP[op].release;
        //1 Byte: AM
        op_off[6] = inst.OP[op].am_enable;
        //1 Byte: RS
        op_off[7] = inst.OP[op].ratescale;
        //1 Byte: DT (DT2<<4 | DT on YM2151)
        op_off[8] = detuneFromOPN2(inst.OP[op].detune);
        //1 Byte: D2R
        op_off[9] = inst.OP[op].decay2;
        //1 Byte: SSGEG_Enabled <<3 | SSGEG
        op_off[10] = inst.OP[op].ssg_eg;
    }

    if(file.write(char_p(idata), 51) != 51)
        return FfmtErrCode::ERR_BADFORMAT;

    file.close();

    return FfmtErrCode::ERR_OK;
}

int DefleMask::formatInstCaps() const
{
    return (int)FormatCaps::FORMAT_CAPS_EVERYTHING;
}

QString DefleMask::formatInstName() const
{
    return "DefleMask Preset Format instrument";
}

QString DefleMask::formatInstExtensionMask() const
{
    return "*.dmp";
}

InstFormats DefleMask::formatInstId() const
{
    return FORMAT_INST_DM_DMP;
}
