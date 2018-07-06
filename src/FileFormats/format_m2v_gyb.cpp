/*
 * OPN2 Bank Editor by Wohlstand, a free tool for music bank editing
 * Copyright (c) 2018 Vitaly Novichkov <admin@wohlnet.ru>
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

#include "format_m2v_gyb.h"
#include "../common.h"

#include <QSet>
#include <QFileInfo>
#include <QByteArray>
#include <algorithm>


bool M2V_GYB::detect(const QString &filePath, char *magic)
{
    //By name extension
    if(filePath.endsWith(".gyb", Qt::CaseInsensitive))
        return true;

    //By magic
    if(magic[0] == 0x1a && magic[1] == 0x0c && magic[2] == 0x02)
        return true;

    return false;
}

FfmtErrCode M2V_GYB::loadFile(QString filePath, FmBank &bank)
{
    QFile file(filePath);

    if(!file.open(QIODevice::ReadOnly))
        return FfmtErrCode::ERR_NOFILE;

    uint8_t header[5];
    if(file.read(char_p(header), 5) != 5)
        return FfmtErrCode::ERR_BADFORMAT;

    if(!(header[0] == 0x1a && header[1] == 0x0c && header[2] == 0x02))
        return FfmtErrCode::ERR_BADFORMAT;

    const unsigned melo_count = header[3];
    const unsigned drum_count = header[4];

    if (melo_count > 128 || drum_count > 128)
        return FfmtErrCode::ERR_BADFORMAT;

    const unsigned total_count = melo_count + drum_count;
    uint8_t ins_map[2 * 128];

    if(file.read(char_p(ins_map), sizeof(ins_map)) != sizeof(ins_map))
        return FfmtErrCode::ERR_BADFORMAT;

    bank.reset(1, 1);

    memset(&bank.Banks_Melodic[0], 0, sizeof(FmBank::MidiBank));
    memset(&bank.Banks_Percussion[0], 0, sizeof(FmBank::MidiBank));
    for(unsigned i = 0; i < 128; ++i)
    {
        bank.Ins_Melodic[i].is_blank = true;
        bank.Ins_Percussion[i].is_blank = true;
    }

    for(unsigned ins_index = 0; ins_index < total_count; ++ins_index)
    {
        bool isdrum = ins_index >= melo_count;

        uint8_t idata[32];
        if(file.read(char_p(idata), 32) != 32)
            return FfmtErrCode::ERR_BADFORMAT;

        unsigned gm = ~0u;
        // search for GM assignment in map
        for(unsigned map_index = 0; gm == ~0u && map_index < 128; ++map_index)
        {
            if(ins_map[2 * map_index + isdrum] == ((!isdrum) ? ins_index : (ins_index - melo_count)))
                gm = map_index;
        }
        // if not assigned, use index but only if it's a free GM slot
        if(gm == ~0u)
        {
            unsigned candidate_gm = ((!isdrum) ? ins_index : (ins_index - melo_count));
            bool is_free = ins_map[2 * candidate_gm + isdrum] >= 128;
            if(is_free)
                gm = candidate_gm;
        }
        // if cannot be assigned, drop
        if(gm == ~0u)
            continue;  // not assigned

        if(ins_index == 0)
            bank.setRegLFO(idata[0]);

        FmBank::Instrument *ins = isdrum ?
            &bank.Ins_Percussion[gm] : &bank.Ins_Melodic[gm];

        ins->is_blank = false;

        if(isdrum)
            ins->percNoteNum = idata[31] & 127;
        else
            ins->note_offset1 = idata[31] & 127;

        for (unsigned op_index = 0; op_index < 4; ++op_index)
        {
            const unsigned opnum[4] = {OPERATOR1_HR, OPERATOR3_HR, OPERATOR2_HR, OPERATOR4_HR};
            unsigned op = opnum[op_index];

            ins->setRegDUMUL(op, idata[1 + op_index]);
            ins->setRegLevel(op, idata[5 + op_index]);
            ins->setRegRSAt(op, idata[9 + op_index]);
            ins->setRegAMD1(op, idata[13 + op_index]);
            ins->setRegD2(op, idata[17 + op_index]);
            ins->setRegSysRel(op, idata[21 + op_index]);
            ins->setRegSsgEg(op, idata[25 + op_index]);
        }
        ins->setRegFbAlg(idata[29]);
        ins->setRegLfoSens(idata[30]);
    }

    if (file.skip(1) != 1)
        return FfmtErrCode::ERR_BADFORMAT;

    for(unsigned ins_index = 0; ins_index < total_count; ++ins_index)
    {
        bool isdrum = ins_index >= melo_count;

        uint8_t text_size;
        if(file.read(char_p(&text_size), 1) != 1)
            return FfmtErrCode::ERR_BADFORMAT;

        unsigned gm = ~0u;
        // search for GM assignment in map
        for(unsigned map_index = 0; gm == ~0u && map_index < 128; ++map_index)
        {
            if(ins_map[2 * map_index + isdrum] == ((!isdrum) ? ins_index : (ins_index - melo_count)))
                gm = map_index;
        }
        // if not assigned, use index but only if it's a free GM slot
        if(gm == ~0u)
        {
            unsigned candidate_gm = ((!isdrum) ? ins_index : (ins_index - melo_count));
            bool is_free = ins_map[2 * candidate_gm + isdrum] >= 128;
            if(is_free)
                gm = candidate_gm;
        }
        // if cannot be assigned, drop
        if(gm == ~0u)
        {
            if(file.skip(text_size) != text_size)
                return FfmtErrCode::ERR_BADFORMAT;
            continue;  // not assigned
        }

        unsigned text_skip = 0;
        if(text_size > 32)
        {
            text_skip = text_size - 32;
            text_size = 32;
        }

        FmBank::Instrument *ins = isdrum ?
            &bank.Ins_Percussion[gm] : &bank.Ins_Melodic[gm];

        if(file.read(ins->name, text_size) != text_size)
            return FfmtErrCode::ERR_BADFORMAT;

        if(file.skip(text_skip) != text_skip)
            return FfmtErrCode::ERR_BADFORMAT;
    }

    return FfmtErrCode::ERR_OK;
}

int M2V_GYB::formatCaps() const
{
    return (int)FormatCaps::FORMAT_CAPS_OPEN|(int)FormatCaps::FORMAT_CAPS_IMPORT;
}

QString M2V_GYB::formatName() const
{
    return "GYB bank";
}

QString M2V_GYB::formatExtensionMask() const
{
    return "*.gyb";
}

BankFormats M2V_GYB::formatId() const
{
    return BankFormats::FORMAT_M2V_GYB;
}
