/*
 * OPN2 Bank Editor by Wohlstand, a free tool for music bank editing
 * Copyright (c) 2017-2022 Vitaly Novichkov <admin@wohlnet.ru>
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

#include "format_gens_y12.h"
#include "../common.h"

#include <QFileInfo>

bool Gens_Y12::detectInst(const QString &filePath, char *magic)
{
    (void)magic;

    //By name extension
    if(filePath.endsWith(".y12", Qt::CaseInsensitive))
        return true;

    //By file size :-P
    QFileInfo f(filePath);
    if(f.size() == 128)
        return true;

    return false;
}

FfmtErrCode Gens_Y12::loadFileInst(QString filePath, FmBank::Instrument &inst, bool *isDrum)
{
    uint8_t idata[128];
    QFile file(filePath);
    (void)isDrum;

    if(!file.open(QIODevice::ReadOnly))
        return FfmtErrCode::ERR_NOFILE;

    if(file.read((char *)idata, 128) != 128)
        return FfmtErrCode::ERR_BADFORMAT;

    for(unsigned i = 0; i < 4; ++i)
    {
        unsigned op = i;
        inst.setRegDUMUL(op, idata[i * 16 + 0]);
        inst.setRegLevel(op, idata[i * 16 + 1]);
        inst.setRegRSAt(op, idata[i * 16 + 2]);
        inst.setRegAMD1(op, idata[i * 16 + 3]);
        inst.setRegD2(op, idata[i * 16 + 4]);
        inst.setRegSysRel(op, idata[i * 16 + 5]);
        inst.setRegSsgEg(op, idata[i * 16 + 6]);
    }

    inst.algorithm = idata[4 * 16 + 0];
    inst.feedback = idata[4 * 16 + 1];

    memcpy(inst.name, &idata[5 * 16], 16);

    return FfmtErrCode::ERR_OK;
}

FfmtErrCode Gens_Y12::saveFileInst(QString filePath, FmBank::Instrument &inst, bool isDrum)
{
    uint8_t idata[128];
    QFile file(filePath);
    (void)isDrum;

    if(!file.open(QIODevice::WriteOnly))
        return FfmtErrCode::ERR_NOFILE;

    memset(idata, 0, 128);

    for(unsigned i = 0; i < 4; ++i)
    {
        unsigned op = i;
        idata[i * 16 + 0] = inst.getRegDUMUL(op);
        idata[i * 16 + 1] = inst.getRegLevel(op);
        idata[i * 16 + 2] = inst.getRegRSAt(op);
        idata[i * 16 + 3] = inst.getRegAMD1(op);
        idata[i * 16 + 4] = inst.getRegD2(op);
        idata[i * 16 + 5] = inst.getRegSysRel(op);
        idata[i * 16 + 6] = inst.getRegSsgEg(op);
    }

    idata[4 * 16 + 0] = inst.algorithm;
    idata[4 * 16 + 1] = inst.feedback;

    size_t namelen = strlen(inst.name);
    namelen = (namelen < 16) ? namelen : 16;

    memcpy(&idata[5 * 16], inst.name, namelen);

    const char dumper[] = "OPN2 Bank Editor";
    const char game[] = "Y12 File Writer";
    memcpy(&idata[6 * 16], dumper, sizeof(dumper) - 1);
    memcpy(&idata[7 * 16], game, sizeof(game) - 1);

    if(file.write((char *)idata, 128) != 128 || !file.flush())
        return FfmtErrCode::ERR_BADFORMAT;

    return FfmtErrCode::ERR_OK;
}

int Gens_Y12::formatInstCaps() const
{
    return (int)FormatCaps::FORMAT_CAPS_EVERYTHING;
}

QString Gens_Y12::formatInstName() const
{
    return "Gens KMod voice dump";
}

QString Gens_Y12::formatInstExtensionMask() const
{
    return "*.y12";
}

QString Gens_Y12::formatInstDefaultExtension() const
{
    return "y12";
}

InstFormats Gens_Y12::formatInstId() const
{
    return FORMAT_INST_Y12;
}
