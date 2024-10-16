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

#include "format_gym_import.h"
#include "ym2612_to_wopi.h"
#include "../common.h"

#include <QSet>
#include <QByteArray>
#include <algorithm>

bool GYM_Importer::detect(const QString &filePath, char *)
{
    //By name extension
    if(filePath.endsWith(".gym", Qt::CaseInsensitive))
        return true;
    return false;
}

FfmtErrCode GYM_Importer::loadFile(QString filePath, FmBank &bank)
{
    RawYm2612ToWopi pseudoChip;

    QFile file(filePath);
    if(!file.open(QIODevice::ReadOnly))
        return FfmtErrCode::ERR_NOFILE;

    bank.Ins_Melodic_box.clear();

    while(!file.atEnd())
    {
        uint8_t cmd = 0x00;
        uint8_t reg = 0x00;
        uint8_t val = 0x00;
        if( file.read(char_p(&cmd), 1) != 1)
            break;//End has been reached!

        switch(cmd)
        {
        case 0x01://Write port 0
            file.read(char_p(&reg), 1);
            file.read(char_p(&val), 1);
            pseudoChip.passReg(0, reg, val);
            break;
        case 0x02://Write port 1
            file.read(char_p(&reg), 1);
            file.read(char_p(&val), 1);
            pseudoChip.passReg(1, reg, val);
            break;
        case 0x03://Write PSG
            break;
        case 0x00://Wait samples
            pseudoChip.doAnalyzeState();
            break;
        }
    }

    const QList<FmBank::Instrument> &insts = pseudoChip.caughtInstruments();
    bank.Ins_Melodic_box.reserve(insts.size());
    for(const FmBank::Instrument &inst : insts)
        bank.Ins_Melodic_box.push_back(inst);
    bank.Ins_Melodic = bank.Ins_Melodic_box.data();

    file.close();

    return FfmtErrCode::ERR_OK;
}

int GYM_Importer::formatCaps() const
{
    return (int)FormatCaps::FORMAT_CAPS_IMPORT;
}

QString GYM_Importer::formatName() const
{
    return "GYM Music";
}

QString GYM_Importer::formatModuleName() const
{
    return "GYM importer";
}

QString GYM_Importer::formatExtensionMask() const
{
    return "*.gym";
}

BankFormats GYM_Importer::formatId() const
{
    return BankFormats::FORMAT_VGM_IMPORTER;
}
