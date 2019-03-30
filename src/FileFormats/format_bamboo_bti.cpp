/*
 * OPN2 Bank Editor by Wohlstand, a free tool for music bank editing
 * Copyright (c) 2017-2019 Vitaly Novichkov <admin@wohlnet.ru>
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

#include "format_bamboo_bti.h"
#include "../common.h"
#include <cstring>

static const char bti_magic[] = "BambooTrackerIst";

bool Bamboo_BTI::detectInst(const QString &filePath, char *magic)
{
    Q_UNUSED(filePath);

    //By magic
    return std::memcmp(magic, bti_magic, 16) == 0;
}

static constexpr uint32_t btVersion(uint8_t x, uint8_t y, uint8_t z)
{
    return (x << 16) | (y << 8) | z;
}

FfmtErrCode Bamboo_BTI::loadFileInst(QString filePath, FmBank::Instrument &inst, bool *isDrum)
{
    Q_UNUSED(isDrum);

    QFile file(filePath);
    if(!file.open(QFile::ReadOnly))
        return FfmtErrCode::ERR_NOFILE;

    char magic[16];
    if(file.read(magic, 16) != 16 || std::memcmp(magic, bti_magic, 16) != 0)
        return FfmtErrCode::ERR_BADFORMAT;

    uint8_t temp[8];
    file.seek(file.pos() + 4); // EOF offset
    if(file.read((char *)temp, 4) != 4)
        return FfmtErrCode::ERR_BADFORMAT;

    uint32_t file_ver = toUint32LE(temp);

    if(file_ver < btVersion(1, 0, 0) || file_ver > btVersion(1, 1, 0))
        return FfmtErrCode::ERR_BADFORMAT;

    // - Instrument Section -
    if(file.read((char *)temp, 8) != 8 || std::memcmp(temp, "INSTRMNT", 8) != 0)
        return FfmtErrCode::ERR_BADFORMAT;

    qint64 pos_end_inst_section = file.pos();

    if(file.read((char *)temp, 8) != 8)
        return FfmtErrCode::ERR_BADFORMAT;

    uint32_t inst_end_offset = toUint32LE(temp);
    uint32_t inst_name_length = toUint32LE(temp + 4);

    pos_end_inst_section += inst_end_offset;

    uint32_t inst_name_limited = std::min(32u, inst_name_length);
    if(file.read(inst.name, inst_name_limited) != inst_name_limited)
        return FfmtErrCode::ERR_BADFORMAT;
    file.seek(file.pos() + (inst_name_length - inst_name_limited));

    uint8_t inst_type;
    if(file.read((char *)&inst_type, 1) != 1)
        return FfmtErrCode::ERR_BADFORMAT;

    if(inst_type == 0x00) // FM
    {
        uint8_t envelope_reset;
        if(file.read((char *)&envelope_reset, 1) != 1)
            return FfmtErrCode::ERR_BADFORMAT;
    }
    else
        return FfmtErrCode::ERR_BADFORMAT;

    // - Instrument Property Section -
    file.seek(pos_end_inst_section);
    if(file.read((char *)temp, 8) != 8 || std::memcmp(temp, "INSTPROP", 8) != 0)
        return FfmtErrCode::ERR_BADFORMAT;

    qint64 pos_end_iprop_section = file.pos();

    if(file.read((char *)temp, 4) != 4)
        return FfmtErrCode::ERR_BADFORMAT;

    uint32_t insp_end_offset = toUint32LE(temp);
    pos_end_iprop_section += insp_end_offset;

    while(file.pos() < pos_end_iprop_section)
    {
        // - Instrument Property Subsection -
        uint8_t subsec_type;
        if(file.read((char *)&subsec_type, 1) != 1)
            return FfmtErrCode::ERR_BADFORMAT;

        qint64 pos_end_subsec = file.pos();

        uint32_t subsec_end_offset;
        if(subsec_type < 0x02) // FM envelope and LFO
        {
            if(file.read((char *)temp, 1) != 1)
                return FfmtErrCode::ERR_BADFORMAT;
            subsec_end_offset = temp[0];
        }
        else
        { // one of the sequence types
            if(file.read((char *)temp, 2) != 2)
                return FfmtErrCode::ERR_BADFORMAT;
            subsec_end_offset = toUint16LE(temp);
        }

        pos_end_subsec += subsec_end_offset;

        switch(subsec_type)
        {
        case 0x00: // FM envelope
        {
            uint8_t idata[1 + 4 * 6];
            if(file.read((char *)idata, sizeof(idata)) != sizeof(idata))
                return FfmtErrCode::ERR_BADFORMAT;

            inst.feedback = idata[0] & 7;
            inst.algorithm = (idata[0] >> 4) & 7;
            for(unsigned i = 0; i < 4; ++i)
            {
                const unsigned order[] = {OPERATOR1_HR, OPERATOR2_HR, OPERATOR3_HR, OPERATOR4_HR};
                const uint8_t *opdata = &idata[1 + 6 * i];
                unsigned opno = order[i];
                bool enable = (opdata[0] & 32) != 0;
                inst.OP[opno].attack = opdata[0] & 31;
                inst.OP[opno].decay1 = opdata[1] & 31;
                inst.OP[opno].ratescale = (opdata[1] >> 5) & 3;
                inst.OP[opno].decay2 = opdata[2] & 31;
                inst.OP[opno].detune = (opdata[2] >> 5) & 7;
                inst.OP[opno].release = opdata[3] & 15;
                inst.OP[opno].sustain = (opdata[3] >> 4);
                inst.OP[opno].level = opdata[4] & 127;
                inst.OP[opno].fmult = opdata[5] & 15;
                if((opdata[5] >> 4) != 8) // SSGEG 8 = disabled
                    inst.OP[opno].ssg_eg = 8 | ((opdata[5] >> 4) & 7);
                if(!enable) inst.OP[opno].level = 127;
            }
            break;
        }
        case 0x01: // FM LFO
        {
            uint8_t idata[2];
            if(file.read((char *)idata, sizeof(idata)) != sizeof(idata))
                return FfmtErrCode::ERR_BADFORMAT;

            inst.fm = idata[0] & 15; // LFO frequency ignored
            inst.am = idata[1] & 3;

            const unsigned order[] = {OPERATOR1_HR, OPERATOR2_HR, OPERATOR3_HR, OPERATOR4_HR};
            for(unsigned i = 0; i < 4; ++i) {
                unsigned opno = order[i];
                inst.OP[opno].am_enable = ((idata[1] >> (i + 4)) & 1) != 0;
            }
            break;
        }
        }

        file.seek(pos_end_subsec);
    }

    return FfmtErrCode::ERR_OK;
}

int Bamboo_BTI::formatInstCaps() const
{
    return (int)FormatCaps::FORMAT_CAPS_OPEN|(int)FormatCaps::FORMAT_CAPS_IMPORT;
}

QString Bamboo_BTI::formatInstName() const
{
    return "Bamboo Tracker instrument";
}

QString Bamboo_BTI::formatInstExtensionMask() const
{
    return "*.bti";
}

InstFormats Bamboo_BTI::formatInstId() const
{
    return FORMAT_INST_BAMBOO_BTI;
}
