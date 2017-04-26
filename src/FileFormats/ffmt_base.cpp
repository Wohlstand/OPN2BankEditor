/*
 * OPL Bank Editor by Wohlstand, a free tool for music bank editing
 * Copyright (c) 2016 Vitaly Novichkov <admin@wohlnet.ru>
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

#include "../common.h"

#include "wohlstand_opn2.h"
#include "vgm_import.h"

static const char *openFilters[]
{
    "Standard OPN2 Bank by Wohlstand (*.wopn)",
    "Import from VGM file (*.vgm)",
};

static const char *saveFilters[]
{
    openFilters[0],
};

#include "ffmt_base.h"


QString FmBankFormatBase::getSaveFiltersList()
{
    return  QString()
            +  saveFilters[FORMAT_WOHLSTAND_OPN2] /*+ ";;"*/;
}

QString FmBankFormatBase::getOpenFiltersList()
{
    return  QString("Supported bank files (*.wopn *.vgm);;") +
            +  openFilters[FORMAT_WOHLSTAND_OPN2] + ";;" +
            +  openFilters[FORMAT_VGM_IMPORTER] + ";;" +
            +  "All files (*.*)";
}

FmBankFormatBase::Formats FmBankFormatBase::getFormatFromFilter(QString filter)
{
    for(int i = (int)FORMATS_BEGIN; i < (int)FORMATS_END; i++)
    {
        if(filter == saveFilters[i])
            return (Formats)i;
    }
    return FORMAT_UNKNOWN;
}

QString FmBankFormatBase::getFilterFromFormat(FmBankFormatBase::Formats format)
{
    if(format >= FORMATS_END)
        return "UNKNOWN";
    if(format < FORMATS_BEGIN)
        return "UNKNOWN";

    return saveFilters[format];
}

int FmBankFormatBase::OpenBankFile(QString filePath, FmBank &bank, Formats *recent)
{
    char magic[32];
    getMagic(filePath, magic, 32);

    int err = FmBankFormatBase::ERR_UNSUPPORTED_FORMAT;
    Formats fmt = FORMAT_UNKNOWN;

    //Check out for Wohlstand OPN2 file format
    if(WohlstandOPN2::detect(magic))
    {
        err = WohlstandOPN2::loadFile(filePath, bank);
        fmt = FORMAT_WOHLSTAND_OPN2;
    }

    //Check out for Wohlstand OPN2 file format
    if(VGM_Importer::detect(magic))
    {
        err = VGM_Importer::loadFile(filePath, bank);
        fmt = FORMAT_VGM_IMPORTER;
    }

    if(recent)
        *recent = fmt;

    return err;
}

int FmBankFormatBase::OpenInstrumentFile(QString filePath, FmBank::Instrument &ins, FmBankFormatBase::InsFormats *recent, bool *isDrum)
{
    char magic[32];
    getMagic(filePath, magic, 32);

    int err = FmBankFormatBase::ERR_UNSUPPORTED_FORMAT;
    InsFormats fmt = FORMAT_INST_UNKNOWN;
    //    if(SbIBK::detectInst(magic))
//    {
//        err = SbIBK::loadFileInst(filePath, ins, isDrum);
//        fmt = FORMAT_INST_OPN2;
//    }

    if(recent)
        *recent = fmt;

    return err;
}
