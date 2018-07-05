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

#include "format_gems_pat.h"
#include "../common.h"

#include <QSet>
#include <QFileInfo>
#include <QByteArray>
#include <algorithm>


bool GEMS_PAT::detect(const QString &filePath, char * /*magic*/)
{
    //By name extension
    if(filePath.endsWith(".bnk", Qt::CaseInsensitive))
        return true;

    //By file size :-P
    QFileInfo f(filePath);
    if(f.size() == 10852)
        return true;

    return false;
}

FfmtErrCode GEMS_PAT::loadFile(QString filePath, FmBank &bank)
{
    uint8_t idata[10852];
    QFile file(filePath);

    if(!file.open(QIODevice::ReadOnly))
        return FfmtErrCode::ERR_NOFILE;

    if(file.size() != 10852)
        return FfmtErrCode::ERR_BADFORMAT;

    if(file.read(char_p(idata), 10852) != 10852)
        return FfmtErrCode::ERR_BADFORMAT;

    bank.reset(1, 0);

    FmBank::MidiBank &bankMeta = bank.Banks_Melodic[0];

    memset(bankMeta.name, 0, sizeof(bankMeta.name));
    memcpy(bankMeta.name, idata, strnlen((const char *)idata, 32));

    bankMeta.msb = 0;
    bankMeta.lsb = 0;

    for(unsigned i = 0; i < 128; ++i)
    {
        FmBank::Instrument &ins = bank.Ins_Melodic[i];
        memset(&ins, 0, sizeof(ins));

        FfmtErrCode err = loadMemInst(&idata[100 + i * 80], ins);
        if(err != FfmtErrCode::ERR_OK)
            ins.is_blank = true;
    }


    return FfmtErrCode::ERR_OK;
}

int GEMS_PAT::formatCaps() const
{
    return (int)FormatCaps::FORMAT_CAPS_OPEN|(int)FormatCaps::FORMAT_CAPS_IMPORT;
}

QString GEMS_PAT::formatName() const
{
    return "GEMS bank";
}

QString GEMS_PAT::formatExtensionMask() const
{
    return "*.bnk";
}

BankFormats GEMS_PAT::formatId() const
{
    return BankFormats::FORMAT_GEMS_BNK;
}

bool GEMS_PAT::detectInst(const QString &filePath, char * /*magic*/)
{
    //By name extension
    if(filePath.endsWith(".pat", Qt::CaseInsensitive))
        return true;

    //By file size :-P
    QFileInfo f(filePath);
    if(f.size() == 80)
        return true;

    return false;
}

FfmtErrCode GEMS_PAT::loadFileInst(QString filePath, FmBank::Instrument &inst, bool *isDrum)
{
    Q_UNUSED(isDrum);

    uint8_t idata[80];
    QFile file(filePath);

    if(!file.open(QIODevice::ReadOnly))
        return FfmtErrCode::ERR_NOFILE;

    if(file.size() != 80)
        return FfmtErrCode::ERR_BADFORMAT;

    if(file.read(char_p(idata), 80) != 80)
        return FfmtErrCode::ERR_BADFORMAT;

    FfmtErrCode err = loadMemInst(idata, inst);

    file.close();

    return err;
}

int GEMS_PAT::formatInstCaps() const
{
    return (int)FormatCaps::FORMAT_CAPS_OPEN|(int)FormatCaps::FORMAT_CAPS_IMPORT;
}

QString GEMS_PAT::formatInstName() const
{
    return "GEMS instrument";
}

QString GEMS_PAT::formatInstExtensionMask() const
{
    return "*.pat";
}

InstFormats GEMS_PAT::formatInstId() const
{
    return FORMAT_INST_GEMS_PAT;
}

FfmtErrCode GEMS_PAT::loadMemInst(const uint8_t idata[80], FmBank::Instrument &inst)
{
    unsigned formatCode = (idata[0] << 8) | idata[1];
    if(formatCode != 0)
        return FfmtErrCode::ERR_BADFORMAT;

    memset(&inst, 0, sizeof(FmBank::Instrument));

    memcpy(inst.name, (const char *)&idata[2], strnlen((const char *)&idata[2], 28));

    bool lfo_enabled = (idata[30] & (1 << 3)) != 0;
    unsigned lfo_frequency = idata[30] & 7;
    bool ch3_on = (idata[31] & (1 << 6)) != 0;

    (void)lfo_enabled;
    (void)lfo_frequency;
    (void)ch3_on;

    inst.feedback = (idata[32] >> 3) & 7;
    inst.algorithm = idata[32] & 7;

    bool left = idata[33] >> 7;
    bool right = (idata[33] >> 6) & 1;

    (void)left;
    (void)right;

    inst.am = (idata[33] >> 4) & 3;
    inst.fm = idata[33] & 7;

    for(unsigned i = 0; i < 4; ++i)
    {
        const unsigned opnum[] = {OPERATOR1_HR, OPERATOR3_HR, OPERATOR2_HR, OPERATOR4_HR};
        const unsigned op = opnum[i];

        //inst.OP[op].fmult = idata[34 + i * 6] & 15;
        //inst.OP[op].detune = (idata[34 + i * 6] >> 4) & 7;
        inst.setRegDUMUL(op, idata[34 + i * 6]);
        //inst.OP[op].level = idata[35 + i * 6] & 127;
        inst.setRegLevel(op, idata[35 + i * 6]);
        //inst.OP[op].ratescale = idata[36 + i * 6] >> 6;
        //inst.OP[op].attack = idata[36 + i * 6] & 31;
        inst.setRegRSAt(op, idata[36 + i * 6]);
        //inst.OP[op].am_enable = idata[37 + i * 6] >> 7;
        //inst.OP[op].decay1 = idata[37 + i * 6] & 31;
        inst.setRegAMD1(op, idata[37 + i * 6]);
        //inst.OP[op].decay2 = idata[38 + i * 6] & 31;
        inst.setRegD2(op, idata[38 + i * 6]);
        //inst.OP[op].release = idata[39 + i * 6] & 15;
        //inst.OP[op].sustain = (idata[39 + i * 6] >> 4) & 15;
        inst.setRegSysRel(op, idata[39 + i * 6]);
    }

    for(unsigned i = 0; i < 4; ++i)
    {
        const unsigned opnum[] = {OPERATOR4_HR, OPERATOR3_HR, OPERATOR1_HR, OPERATOR2_HR};
        const unsigned op = opnum[i];

        unsigned ch3f = (idata[58 + i * 2] << 8) | idata[59 + i * 2];
        (void)op;
        (void)ch3f;
    }

    for(unsigned i = 0; i < 4; ++i)
    {
        const unsigned opnum[] = {OPERATOR1_HR, OPERATOR2_HR, OPERATOR3_HR, OPERATOR4_HR};
        const unsigned op = opnum[i];

        bool enable = (idata[66] & (1 << i)) != 0;
        if(!enable)
            inst.OP[op].level = 127;
    }

    return FfmtErrCode::ERR_OK;
}
