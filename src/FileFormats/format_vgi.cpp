/*
 * OPN2 Bank Editor by Wohlstand, a free tool for music bank editing
 * Copyright (c) 2017-2021 Vitaly Novichkov <admin@wohlnet.ru>
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

#include "format_vgi.h"
#include "../common.h"
#include <QFileInfo>

bool VGM_MM::detectInst(const QString &filePath, char* magic)
{
    Q_UNUSED(magic);

    //By name extension
    if(filePath.endsWith(".vgi", Qt::CaseInsensitive))
        return true;

    //By file size :-P
    QFileInfo f(filePath);
    if(f.size() == 43)
        return true;

    return false;
}

FfmtErrCode VGM_MM::loadFileInst(QString filePath, FmBank::Instrument &inst, bool *isDrum)
{
    Q_UNUSED(isDrum);

    uint8_t idata[43];
    QFile file(filePath);

    if(!file.open(QIODevice::ReadOnly))
        return FfmtErrCode::ERR_NOFILE;

    if(file.read(char_p(idata), 43) != 43)
        return FfmtErrCode::ERR_BADFORMAT;

    inst.algorithm = idata[0] & 7;
    inst.feedback = idata[1] & 7;
    inst.setRegLfoSens(idata[2]);

    for (unsigned op = 0; op < 4; ++op) {
        FmBank::Operator &oper = inst.OP[op];
        uint8_t *opdata = idata + 3 + op * 10;
        oper.fmult = opdata[0] & 15;
        oper.detune = opdata[1] & 7;
        oper.level = opdata[2] & 127;
        oper.ratescale = opdata[3] & 3;
        oper.attack = opdata[4] & 31;
        oper.decay1 = opdata[5] & 31;
        oper.am_enable = opdata[5] >> 7;
        oper.decay2 = opdata[6] & 31;
        oper.release = opdata[7] & 15;
        oper.sustain = opdata[8] & 15;
        oper.ssg_eg = opdata[9] & 15;
    }

    return FfmtErrCode::ERR_OK;
}

FfmtErrCode VGM_MM::saveFileInst(QString filePath, FmBank::Instrument &inst, bool isDrum)
{
    Q_UNUSED(isDrum);

    uint8_t idata[43];

    idata[0] = inst.algorithm;
    idata[1] = inst.feedback;
    idata[2] = inst.getRegLfoSens();

    for (unsigned op = 0; op < 4; ++op) {
        uint8_t *opdata = idata + 3 + op * 10;
        const FmBank::Operator &oper = inst.OP[op];
        opdata[0] = oper.fmult;
        opdata[1] = oper.detune;
        opdata[2] = oper.level;
        opdata[3] = oper.ratescale;
        opdata[4] = oper.attack;
        opdata[5] = oper.decay1 | (oper.am_enable << 7);
        opdata[6] = oper.decay2;
        opdata[7] = oper.release;
        opdata[8] = oper.sustain;
        opdata[9] = oper.ssg_eg;
    }

    QFile file(filePath);
    if(!file.open(QIODevice::WriteOnly))
        return FfmtErrCode::ERR_NOFILE;

    if(file.write(char_p(idata), 43) != 43 || !file.flush())
        return FfmtErrCode::ERR_BADFORMAT;

    return FfmtErrCode::ERR_OK;
}

int VGM_MM::formatInstCaps() const
{
    return (int)FormatCaps::FORMAT_CAPS_EVERYTHING;
}

QString VGM_MM::formatInstName() const
{
    return "VGM Music Maker instrument";
}

QString VGM_MM::formatInstExtensionMask() const
{
    return "*.vgi";
}

InstFormats VGM_MM::formatInstId() const
{
    return FORMAT_INST_VGM_MM;
}
