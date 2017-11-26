/*
 * OPN2 Bank Editor by Wohlstand, a free tool for music bank editing
 * Copyright (c) 2017 Vitaly Novichkov <admin@wohlnet.ru>
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

#include "format_tfi.h"
#include "../common.h"

#include <QSet>
#include <QFileInfo>
#include <QByteArray>
#include <algorithm>


bool TFI_MM::detectInst(const QString &filePath, char * /*magic*/)
{
    //By name extension
    if(filePath.endsWith(".tfi", Qt::CaseInsensitive))
        return true;

    //By file size :-P
    QFileInfo f(filePath);
    if(f.size() == 42)
        return true;

    return false;
}

/*
=========================
      Detune values
=========================
    Exact meaning for detune values and how they affect tuning
    are not clearly outlined in any literature regarding
    the YM2203 and may need further research to identify.
=========================
    $00 = -3
    $01 = -2
    $02 = -1
    $03 = no detune
    $04 = +1
    $05 = +2
    $06 = +3
=========================
*/

static uint8_t opn1_to_opn2[7]
{
    5, 6, 7, 0, 1, 2, 3
};

static uint8_t opn2_to_opn1[8]
{
    3, 4, 5, 6, 3, 0, 1, 2
};

static uint8_t detuneToOPN2(uint8_t d)
{
    if(d > 7) return 0;
    return opn1_to_opn2[d];
}

static uint8_t detuneFromOPN2(uint8_t d)
{
    if(d > 8) return 3;
    return opn2_to_opn1[d];
}

FfmtErrCode TFI_MM::loadFileInst(QString filePath, FmBank::Instrument &inst, bool *isDrum)
{
    uint8_t idata[42];
    QFile file(filePath);

    if(!file.open(QIODevice::ReadOnly))
        return FfmtErrCode::ERR_NOFILE;

    if(file.size() != 42)
        return FfmtErrCode::ERR_BADFORMAT;

    if(file.read(char_p(idata), 42) != 42)
        return FfmtErrCode::ERR_BADFORMAT;

    memset(&inst, 0, sizeof(FmBank::Instrument));

    inst.algorithm = idata[0] & 0x07;
    inst.feedback  = idata[1] & 0x07;
    for(size_t op = 0; op < 4; op++)
    {
        uint8_t *op_off = idata + 2 + (op * 10);
        //0x02 	OP0 Multiplier 	$00-$0F
        inst.OP[op].fmult = op_off[0] & 0x0F;
        //0x03 	OP0 Detune 	$00-$06
        inst.OP[op].detune = detuneToOPN2(op_off[1]);
        //0x04 	OP0 Total Level 	$00-$7F
        inst.OP[op].level = op_off[2] & 0x7F;
        //0x05 	OP0 Rate Scaling 	$00-$03
        inst.OP[op].ratescale = op_off[3] & 0x03;
        //0x06 	OP0 Attack Rate 	$00-$1F
        inst.OP[op].attack = op_off[4] & 0x7F;
        //0x07 	OP0 Decay Rate 	$00-$1F
        inst.OP[op].decay1 = op_off[5] & 0x7F;
        //0x08 	OP0 Sustain Rate 	$00-$1F
        inst.OP[op].decay2 = op_off[6] & 0x7F;
        //0x09 	OP0 Release Rate 	$00-$0F
        inst.OP[op].release = op_off[7] & 0x0F;
        //0x0A 	OP0 Sustain Level 	$00-$0F
        inst.OP[op].sustain = op_off[8] & 0x0F;
        //0x0B 	OP0 SSG-EG 	$00, $08-$0F
        inst.OP[op].ssg_eg = op_off[9] & 0x0F;
    }

    if(isDrum)
        *isDrum = false;

    QByteArray name = QFileInfo(filePath).baseName().toUtf8();
    strncpy(inst.name, name.data(), name.size() < 32 ? (size_t)name.size() : 32);

    file.close();

    return FfmtErrCode::ERR_OK;
}

FfmtErrCode TFI_MM::saveFileInst(QString filePath, FmBank::Instrument &inst, bool /*isDrum*/)
{
    uint8_t idata[42];
    QFile file(filePath);

    if(!file.open(QIODevice::WriteOnly))
        return FfmtErrCode::ERR_NOFILE;

    memset(idata, 0, 42);
    idata[0] = inst.algorithm;
    idata[1] = inst.feedback;
    for(size_t op = 0; op < 4; op++)
    {
        uint8_t *op_off = idata + 2 + (op * 10);
        //0x02 	OP0 Multiplier 	$00-$0F
        op_off[0] = inst.OP[op].fmult;
        //0x03 	OP0 Detune 	$00-$06
        op_off[1] = detuneFromOPN2(inst.OP[op].detune);
        //0x04 	OP0 Total Level 	$00-$7F
        op_off[2] = inst.OP[op].level;
        //0x05 	OP0 Rate Scaling 	$00-$03
        op_off[3] = inst.OP[op].ratescale;
        //0x06 	OP0 Attack Rate 	$00-$1F
        op_off[4] = inst.OP[op].attack;
        //0x07 	OP0 Decay Rate 	$00-$1F
        op_off[5] = inst.OP[op].decay1;
        //0x08 	OP0 Sustain Rate 	$00-$1F
        op_off[6] = inst.OP[op].decay2;
        //0x09 	OP0 Release Rate 	$00-$0F
        op_off[7] = inst.OP[op].release;
        //0x0A 	OP0 Sustain Level 	$00-$0F
        op_off[8] = inst.OP[op].sustain;
        //0x0B 	OP0 SSG-EG 	$00, $08-$0F
        op_off[9] = inst.OP[op].ssg_eg;
    }

    if(file.write(char_p(idata), 42) != 42)
        return FfmtErrCode::ERR_BADFORMAT;

    file.close();

    return FfmtErrCode::ERR_OK;
}

int TFI_MM::formatInstCaps() const
{
    return (int)FormatCaps::FORMAT_CAPS_EVERYTHING;
}

QString TFI_MM::formatInstName() const
{
    return "TFM Music Maker instrument";
}

QString TFI_MM::formatInstExtensionMask() const
{
    return "*.tfi";
}

InstFormats TFI_MM::formatInstId() const
{
    return FORMAT_INST_TFI_MM;
}
