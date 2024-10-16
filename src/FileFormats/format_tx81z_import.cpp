/*
 * OPN2 Bank Editor by Wohlstand, a free tool for music bank editing
 * Copyright (c) 2017-2024 Vitaly Novichkov <admin@wohlnet.ru>
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

#include "format_tx81z_import.h"
#include "../common.h"
#include <math.h>

bool TX81Z_Importer::detect(const QString & /*filePath*/, char *magic)
{
    return (uint8_t)magic[0] == 0xf0 && (uint8_t)magic[1] == 0x43 &&
        (uint8_t)magic[3] == 0x04;
}

static unsigned convertMultiplier(unsigned mul)
{
    if (mul > 0) {
        mul = (unsigned)lround(mul * 0.25);
        mul = (mul < 15) ? mul : 15;
    }
    return mul;
}

static unsigned convertDetune(unsigned det)
{
    switch(det)
    {
    default: return 0;
    case 0: return 7;
    case 1: return 6;
    case 2: return 5;
    case 3: return 4;
    case 4: return 1;
    case 5: return 2;
    case 6: return 3;
    }
}

FfmtErrCode TX81Z_Importer::loadFile(QString filePath, FmBank &bank)
{
    QFile file(filePath);

    if(!file.open(QIODevice::ReadOnly))
        return FfmtErrCode::ERR_NOFILE;

    uint8_t magic[6];
    if (file.read((char *)magic, 6) != 6)
        return FfmtErrCode::ERR_BADFORMAT;

    if (magic[0] != 0xf0 || magic[1] != 0x43 || magic[3] != 0x04)
        return FfmtErrCode::ERR_BADFORMAT;

    unsigned voiceCount = magic[4];
    if (voiceCount >= 128)
        return FfmtErrCode::ERR_BADFORMAT;

    bank.reset(1, 1);
    for(unsigned i = 0; i < 128; ++i)
    {
        bank.Ins_Melodic[i].is_blank = i >= voiceCount;
        bank.Ins_Percussion[i].is_blank = true;
    }

    for(unsigned i = 0; i < voiceCount; ++i)
    {
        FmBank::Instrument &inst = bank.Ins_Melodic[i];

        uint8_t idata[128];
        if (file.read((char *)idata, 128) != 128)
            return FfmtErrCode::ERR_BADFORMAT;

        char name[11];
        memcpy(name, &idata[57], 10);
        name[10] = 0;

        size_t namelen = strlen(name);
        while(namelen > 0 && name[namelen - 1] == ' ')
            name[--namelen] = '\0';
        strcpy(inst.name, name);

        inst.setRegFbAlg(idata[40]);
        for(unsigned j = 0; j < 4; ++j)
        {
            const unsigned opnum[] = {OPERATOR1_HR, OPERATOR3_HR, OPERATOR2_HR, OPERATOR4_HR};
            const unsigned op = opnum[j];
            unsigned o = j * 10;
            inst.setRegDUMUL(op, convertMultiplier(idata[o + 8] & 63) | (convertDetune(idata[o + 9] & 7) << 4));
            inst.setRegRSAt(op, (idata[o] & 31) | ((idata[o + 9] & 24) << 3));
            inst.setRegAMD1(op, (31 - (idata[o + 1] & 31)) | ((idata[o + 6] & 64) << 1));
            inst.setRegD2(op, idata[o + 2] & 31);
            inst.setRegSysRel(op, (idata[o + 3] & 15) | ((15 - idata[o + 4]) << 4));
            inst.setRegLevel(op, (uint8_t)std::max(0, 99 - (idata[o + 7] & 127)));
        }
    }

    return FfmtErrCode::ERR_OK;
}

int TX81Z_Importer::formatCaps() const
{
    return (int)FormatCaps::FORMAT_CAPS_OPEN|(int)FormatCaps::FORMAT_CAPS_IMPORT;
}

QString TX81Z_Importer::formatName() const
{
    return "TX81Z bank";
}

QString TX81Z_Importer::formatExtensionMask() const
{
    return "*.syx";
}

BankFormats TX81Z_Importer::formatId() const
{
    return BankFormats::FORMAT_TX81Z_IMPORTER;
}
