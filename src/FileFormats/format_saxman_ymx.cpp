/*
 * OPN2 Bank Editor by Wohlstand, a free tool for music bank editing
 * Copyright (c) 2017-2020 Vitaly Novichkov <admin@wohlnet.ru>
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

#include "format_saxman_ymx.h"
#include "../common.h"
#include <QDebug>

bool Saxman_YMX::detect(const QString & /*filePath*/ , char *magic)
{
    return !memcmp(magic, "YM2612", 6);
}

FfmtErrCode Saxman_YMX::loadFile(QString filePath, FmBank &bank)
{
    QFile file(filePath);

    if(!file.open(QIODevice::ReadOnly))
        return FfmtErrCode::ERR_NOFILE;

    char magic[6] = {};
    file.read((char *)&magic, 6);
    if(memcmp(magic, "YM2612", 6))
        return FfmtErrCode::ERR_BADFORMAT;

    uint8_t revision;
    if (file.read((char *)&revision, 1) != 1)
        return FfmtErrCode::ERR_BADFORMAT;

    switch(revision)
    {
    case 0:
        return loadRevision0(file, bank);
    // TODO the revision 1
    default:
        return FfmtErrCode::ERR_BADFORMAT;
    }
}

FfmtErrCode Saxman_YMX::loadRevision0(QFile &file, FmBank &bank)
{
    uint8_t voiceCount; // count supposed to be N-1, but some files have it as N
    if (file.read((char *)&voiceCount, 1) != 1)
        return FfmtErrCode::ERR_BADFORMAT;

    struct RawVoice {
        char name[11];
        uint8_t idata[25];
    };

    std::vector<RawVoice> voices;
    voices.reserve(voiceCount);

    for(unsigned i = 0; i < static_cast<unsigned>(voiceCount + 1); ++i)
    {
        file.seek(8 + i * 35);

        RawVoice v;

        if (file.read((char *)v.idata, 25) != 25) {
            if(i == voiceCount)
                break;  // voice count workaround
            return FfmtErrCode::ERR_BADFORMAT;
        }

        if (file.read((char *)v.name, 10) != 10)
            return FfmtErrCode::ERR_BADFORMAT;
        v.name[10] = 0;

        size_t namelen = strlen(v.name);
        while(namelen > 0 && v.name[namelen - 1] == ' ')
            v.name[--namelen] = '\0';

        voices.push_back(v);
    }

    if (voices.empty())
        return FfmtErrCode::ERR_BADFORMAT;

    bank.reset((voices.size() + 127) / 128, 1);
    for(unsigned i = 0, n = bank.Ins_Melodic_box.size(); i < n; ++i)
        bank.Ins_Melodic_box[i].is_blank = true;
    for(unsigned i = 0, n = bank.Ins_Percussion_box.size(); i < n; ++i)
        bank.Ins_Percussion_box[i].is_blank = true;

    for(unsigned j = 0, n = (unsigned)voices.size(); j < n; ++j)
    {
        FmBank::Instrument &inst = bank.Ins_Melodic[j];
        inst.is_blank = false;
        strcpy(inst.name, voices[j].name);

        const uint8_t *idata = voices[j].idata;
        inst.setRegFbAlg(idata[0]);

        for(unsigned i = 0; i < 4; ++i)
        {
            const unsigned opnum[] = {OPERATOR1_HR, OPERATOR3_HR, OPERATOR2_HR, OPERATOR4_HR};
            const unsigned op = opnum[i];

            inst.setRegDUMUL(op, idata[i + 1]);
            inst.setRegRSAt(op, idata[i + 1 + 4]);
            inst.setRegAMD1(op, idata[i + 1 + 8]);
            inst.setRegD2(op, idata[i + 1 + 12]);
            inst.setRegSysRel(op, idata[i + 1 + 16]);
            inst.setRegLevel(op, idata[i + 1 + 20]);
        }
    }

    return FfmtErrCode::ERR_OK;
}

FfmtErrCode Saxman_YMX::saveFile(QString filePath, FmBank &bank)
{
    QFile file(filePath);

    if(!file.open(QIODevice::WriteOnly))
        return FfmtErrCode::ERR_NOFILE;

    uint8_t revision = 0;
    file.write("YM2612", 6);
    file.write((char *)&revision, 1);

    // store up to the 128 first instruments
    uint8_t instCount = 0;
    const FmBank::Instrument *insts[128];
    for(unsigned i = 0, n = bank.countMelodic(); i < n && instCount < 128; ++i)
    {
        if(!bank.Ins_Melodic[i].is_blank)
            insts[instCount++] = &bank.Ins_Melodic[i];
    }
    for(unsigned i = 0, n = bank.countDrums(); i < n && instCount < 128; ++i)
    {
        if(!bank.Ins_Percussion[i].is_blank)
            insts[instCount++] = &bank.Ins_Percussion[i];
    }

    uint8_t instCountStored = instCount - 1;
    file.write((char *)&instCountStored, 1);

    for(unsigned i = 0; i < instCount; ++i)
    {
        const FmBank::Instrument &inst = *insts[i];

        uint8_t idata[25];
        idata[0] = inst.getRegFbAlg();

        for(unsigned i = 0; i < 4; ++i)
        {
            const unsigned opnum[] = {OPERATOR1_HR, OPERATOR3_HR, OPERATOR2_HR, OPERATOR4_HR};
            const unsigned op = opnum[i];

            idata[i + 1] = inst.getRegDUMUL(op);
            idata[i + 1 + 4] = inst.getRegRSAt(op);
            idata[i + 1 + 8] = inst.getRegAMD1(op);
            idata[i + 1 + 12] = inst.getRegD2(op);
            idata[i + 1 + 16] = inst.getRegSysRel(op);
            idata[i + 1 + 20] = inst.getRegLevel(op);
        }

        file.write((char *)idata, 25);

        char name[11];
        memcpy(name, inst.name, 10);
        name[10] = '\0';
        for(size_t n = strlen(name); n < 10; ++n)
            name[n] = ' ';

        file.write(name, 10);
    }

    if(!file.flush()) {
        file.remove();
        return FfmtErrCode::ERR_NOFILE;
    }

    return FfmtErrCode::ERR_OK;
}

int Saxman_YMX::formatCaps() const
{
    return (int)FormatCaps::FORMAT_CAPS_EVERYTHING_GM;
}

QString Saxman_YMX::formatName() const
{
    return "YMX bank";
}

QString Saxman_YMX::formatExtensionMask() const
{
    return "*.ymx";
}

BankFormats Saxman_YMX::formatId() const
{
    return BankFormats::FORMAT_SAXMAN_YMX;
}
