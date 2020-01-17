/*
 * OPN2 Bank Editor by Wohlstand, a free tool for music bank editing
 * Copyright (c) 2018-2020 Vitaly Novichkov <admin@wohlnet.ru>
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

#include "format_tomsoft_gin.h"
#include "../common.h"

#include <QSet>
#include <QFileInfo>
#include <QByteArray>
#include <algorithm>

static const char *magic_tomsoft_gin = "Tomsoft Studio.SEGA Genesys Instrument.1.00a. E-mail:tomsoft@cmmail.com";

bool Tomsoft_GIN::detect(const QString &/*filePath*/, char * magic)
{
    return (memcmp(magic_tomsoft_gin, magic, 4) == 0);
}

FfmtErrCode Tomsoft_GIN::loadFile(QString filePath, FmBank &bank)
{
    uint8_t idata[0x10d2];
    QFile file(filePath);

    if(!file.open(QIODevice::ReadOnly))
        return FfmtErrCode::ERR_NOFILE;

    if(file.size() < 0x10d2)
        return FfmtErrCode::ERR_BADFORMAT;

    if(file.read(char_p(idata), 0x10d2) != 0x10d2)
        return FfmtErrCode::ERR_BADFORMAT;

    bank.reset(1, 0);

    if(memcmp(idata, magic_tomsoft_gin, 0x48))
        return FfmtErrCode::ERR_BADFORMAT;

    for(unsigned i = 0; i < 128; ++i)
    {
        FmBank::Instrument &ins = bank.Ins_Melodic[i];
        memset(&ins, 0, sizeof(ins));

        FfmtErrCode err = loadMemInst(&idata[83 + i * 33], ins);
        if(err != FfmtErrCode::ERR_OK)
            ins.is_blank = true;
    }

    return FfmtErrCode::ERR_OK;
}

int Tomsoft_GIN::formatCaps() const
{
    return (int)FormatCaps::FORMAT_CAPS_OPEN|(int)FormatCaps::FORMAT_CAPS_IMPORT;
}

QString Tomsoft_GIN::formatName() const
{
    return "Tomsoft GIN bank";
}

QString Tomsoft_GIN::formatExtensionMask() const
{
    return "*.gin";
}

BankFormats Tomsoft_GIN::formatId() const
{
    return BankFormats::FORMAT_GEMS_BNK;
}

FfmtErrCode Tomsoft_GIN::loadMemInst(const uint8_t idata[80], FmBank::Instrument &inst)
{
    memset(&inst, 0, sizeof(FmBank::Instrument));

    inst.setRegFbAlg(idata[4]);
    for(unsigned i = 0; i < 4; ++i)
    {
        const unsigned opnum[] = {OPERATOR1_HR, OPERATOR3_HR, OPERATOR2_HR, OPERATOR4_HR};
        const unsigned op = opnum[i];
        const unsigned ins_offset = 4;
        inst.setRegDUMUL(op,  idata[ins_offset + 0 + (i * 7)]);
        inst.setRegLevel(op,  idata[ins_offset + 1 + (i * 7)]);
        inst.setRegRSAt(op,   idata[ins_offset + 2 + (i * 7)]);
        inst.setRegAMD1(op,   idata[ins_offset + 3 + (i * 7)]);
        inst.setRegD2(op,     idata[ins_offset + 4 + (i * 7)]);
        inst.setRegSysRel(op, idata[ins_offset + 5 + (i * 7)]);
        inst.setRegSsgEg(op,  idata[ins_offset + 6 + (i * 7)]);
    }

    return FfmtErrCode::ERR_OK;
}
