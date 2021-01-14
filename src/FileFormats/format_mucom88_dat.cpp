/*
 * OPN2 Bank Editor by Wohlstand, a free tool for music bank editing
 * Copyright (c) 2018-2021 Vitaly Novichkov <admin@wohlnet.ru>
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

#include "format_mucom88_dat.h"
#include "../common.h"
#include <QFileInfo>

bool Mucom88_DAT::detect(const QString &filePath, char* magic)
{
    Q_UNUSED(magic);

    QFileInfo f(filePath);
    uint64_t size = f.size();
    return filePath.endsWith(".dat", Qt::CaseInsensitive) && size == 8192;
}

FfmtErrCode Mucom88_DAT::loadFile(QString filePath, FmBank &bank)
{
    QFile file(filePath);

    if(!file.open(QIODevice::ReadOnly))
        return FfmtErrCode::ERR_NOFILE;

    uint8_t idata[32];
    const uint8_t iempty[32] = { 0 };

    QVector<FmBank::Instrument> insts;
    insts.reserve(256);

    unsigned nthInst = 0;
    while(file.read((char *)idata, 32) == 32)
    {
        if(memcmp(idata, iempty, 32) == 0)
            continue;

        FmBank::Instrument inst = FmBank::emptyInst();
        ++nthInst;

        char name[7];
        memcpy(name, (char *)&idata[26], 6);
        name[6] = '\0';

        unsigned namelen = (unsigned)strlen(name);
        while(namelen > 0 && name[namelen - 1] == ' ')
            --namelen;
        if(namelen > 0)
            memcpy(inst.name, name, namelen);
        else
            snprintf(inst.name, 32, "ins%03u", nthInst);

        unsigned off = 0;

        ++off; //00: 1??? byte

        for(unsigned op = 0; op < 4; ++op)
        {
            unsigned j = 0;
            inst.setRegDUMUL(op, idata[off + op + (j++ * 4)]);
            inst.setRegLevel(op, idata[off + op + (j++ * 4)]);
            inst.setRegRSAt(op, idata[off + op + (j++ * 4)]);
            inst.setRegAMD1(op, idata[off + op + (j++ * 4)]);
            inst.setRegD2(op, idata[off + op + (j++ * 4)]);
            inst.setRegSysRel(op, idata[off + op + (j++ * 4)]);
        }
        off += 4 * 6;

        inst.setRegFbAlg(idata[off++]);

        insts.push_back(inst);
    }

    unsigned insCount = insts.size();
    unsigned bankCount = (insCount + 127) / 128;
    if(bankCount > (128 * 128))
        return FfmtErrCode::ERR_BADFORMAT;

    bank.reset(bankCount, 1);

    for(unsigned i = 0; i < bankCount; ++i)
    {
        FmBank::MidiBank &bankMeta = bank.Banks_Melodic[i];
        bankMeta.msb = i / 128;
        bankMeta.lsb = i % 128;
    }

    for(unsigned i = 0, n = bank.countMelodic(); i < n; ++i)
        bank.Ins_Melodic[i].is_blank = true;
    for(unsigned i = 0, n = bank.countDrums(); i < n; ++i)
        bank.Ins_Percussion[i].is_blank = true;

    std::copy(insts.begin(), insts.end(), bank.Ins_Melodic_box.begin());

    return FfmtErrCode::ERR_OK;
}

int Mucom88_DAT::formatCaps() const
{
    return (int)FormatCaps::FORMAT_CAPS_OPEN|(int)FormatCaps::FORMAT_CAPS_IMPORT;
}

QString Mucom88_DAT::formatName() const
{
    return "Mucom88 voice data";
}

QString Mucom88_DAT::formatExtensionMask() const
{
    return "*.dat";
}

BankFormats Mucom88_DAT::formatId() const
{
    return BankFormats::FORMAT_MUCOM88_DAT;
}
