/*
 * OPN2 Bank Editor by Wohlstand, a free tool for music bank editing
 * Copyright (c) 2018-2023 Vitaly Novichkov <admin@wohlnet.ru>
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

#define FREQ_BANK_NUMBER    0x5210
#define BANK_VERSION        0


#pragma pack(push, 1)
typedef struct tagOPStruct
{
//    uint8_t Reserve1:1;     /*0x30*/
//    uint8_t Detune:3;
//    uint8_t FreqMul:4;
    uint8_t DetuneFMul;

//    uint8_t Reserve2:1;     /*0x40*/
//    uint8_t TLevel:7;
    uint8_t TLevel;

//    uint8_t Rate:2;         /*0x50*/
//    uint8_t Reserve3:1;
//    uint8_t AttRate:5;
    uint8_t RsAttRate;

//    uint8_t LFOAmpEnable:1;  /*0x60*/
//    uint8_t Reserve4:2;
//    uint8_t D1R:5;
    uint8_t LFOAmRateDec1;

//    uint8_t Reserve5:3;     /*0x70*/
//    uint8_t D2R:5;
    uint8_t Decay2;

//    uint8_t D1L:4;          /*0x80*/
//    uint8_t Release:4;
    uint8_t SusRel;

//    uint8_t Reserve6:4;     /*0x90*/
//    uint8_t SSGEG:4;
    uint8_t SSGEG;
} OPSTRUCT;

typedef struct tagInstrumentFileItem
{
//    uint16_t Reserve1:2; /*0xa0*/
//    uint16_t FreqBank:3;
//    uint16_t FreqNumber:11;
    uint16_t FreqBankNumber;

//    uint8_t Reserve2:4; /*0x22*/
//    uint8_t LFOEnable:1;
//    uint8_t LFOFreq:3;
    uint8_t LFOEnFreq;

//    uint8_t Reserve3:2; /*0xb0*/
//    uint8_t FeedBack:3;
//    uint8_t Algorit:3;
    uint8_t FeedBackAlg;

//    uint8_t Reserve4:2; /*0xb4*/
//    uint8_t LFOAMS:2;
//    uint8_t Reserve5:1;
//    uint8_t LFOFM:3;
    uint8_t LFOSens;

    OPSTRUCT OPRegister[4];
} INSTRUMENTFILEITEM;

typedef struct tagInstrumentFile
{
    char InstrumentID[80]; //Tomsoft Studio.SEGA Genesys Instrument.1.00a. E-mail:tomsoft@cmmail.com
    uint16_t nID;   //Version;
    INSTRUMENTFILEITEM Item[128];
} INSTRUMENTFILE;
#pragma pack(pop)


bool Tomsoft_GIN::detect(const QString &/*filePath*/, char * magic)
{
    return (memcmp(magic_tomsoft_gin, magic, 4) == 0);
}

FfmtErrCode Tomsoft_GIN::loadFile(QString filePath, FmBank &bank)
{
    const size_t idata_size = sizeof(INSTRUMENTFILE);
    uint8_t idata[idata_size];
    QFile file(filePath);

    if(!file.open(QIODevice::ReadOnly))
        return FfmtErrCode::ERR_NOFILE;

    if(file.size() < (int)idata_size)
        return FfmtErrCode::ERR_BADFORMAT;

    if(file.read(char_p(idata), idata_size) != (qint64)idata_size)
        return FfmtErrCode::ERR_BADFORMAT;

    INSTRUMENTFILE *ibank = reinterpret_cast<INSTRUMENTFILE *>(idata);
    if(!ibank)
        return FfmtErrCode::ERR_BADFORMAT;

    bank.reset(1, 1);

    if(strncmp(ibank->InstrumentID, magic_tomsoft_gin, 0x48) != 0)
        return FfmtErrCode::ERR_BADFORMAT;

    // Bank keeps LFO setup for every instrument, however as these settings are chip global,
    // take data from the first instrument only
    bank.lfo_enabled =   (ibank->Item[0].LFOEnFreq >> 4) & 0x01;
    bank.lfo_frequency = (ibank->Item[0].LFOEnFreq >> 5) & 0x07;

    for(unsigned i = 0; i < 128; ++i)
    {
        FmBank::Instrument &ins = bank.Ins_Melodic[i];
        memset(&ins, 0, sizeof(ins));

        FfmtErrCode err = loadMemInst(&ibank->Item[i], ins);
        if(err != FfmtErrCode::ERR_OK)
            ins.is_blank = true;
    }

    return FfmtErrCode::ERR_OK;
}

FfmtErrCode Tomsoft_GIN::saveFile(QString filePath, FmBank &bank)
{
    const size_t idata_size = sizeof(INSTRUMENTFILE);
    uint8_t idata[idata_size];
    QFile file(filePath);

    if(!file.open(QIODevice::WriteOnly))
        return FfmtErrCode::ERR_NOFILE;

    INSTRUMENTFILE *ibank = reinterpret_cast<INSTRUMENTFILE *>(idata);

    memset(idata, 0, idata_size);
    memset(ibank->InstrumentID, 0xCC, 80); // In original files after the magic number the CC-bytes folowing
    strncpy(ibank->InstrumentID, magic_tomsoft_gin, strlen(magic_tomsoft_gin) + 1);
    ibank->nID = BANK_VERSION;

    for(unsigned i = 0; i < (unsigned)std::min(128, bank.Ins_Melodic_box.size()); ++i)
    {
        FmBank::Instrument &ins = bank.Ins_Melodic[i];

        FfmtErrCode err = saveMemInst(&ibank->Item[i], ins);
        if(err != FfmtErrCode::ERR_OK)
            return FfmtErrCode::ERR_BADFORMAT;

        ibank->Item[0].LFOEnFreq = ((bank.lfo_enabled << 4) & (0x01 << 4)) | ((bank.lfo_frequency << 5) & (0x07 << 5));
    }

    if(file.write(char_p(idata), idata_size) != idata_size)
        return FfmtErrCode::ERR_BADFORMAT;

    file.close();

    return FfmtErrCode::ERR_OK;
}

int Tomsoft_GIN::formatCaps() const
{
    return (int)FormatCaps::FORMAT_CAPS_EVERYTHING|(int)FormatCaps::FORMAT_CAPS_MELODIC_ONLY;
}

QString Tomsoft_GIN::formatName() const
{
    return "Tomsoft GIN bank";
}

QString Tomsoft_GIN::formatExtensionMask() const
{
    return "*.gin";
}

QString Tomsoft_GIN::formatDefaultExtension() const
{
    return "gin";
}

BankFormats Tomsoft_GIN::formatId() const
{
    return BankFormats::FORMAT_TOMSOFT_GIN;
}

FfmtErrCode Tomsoft_GIN::loadMemInst(const INSTRUMENTFILEITEM *idata, FmBank::Instrument &inst)
{
    memset(&inst, 0, sizeof(FmBank::Instrument));

    // All bit fields has inverted location in their bytes because of M$' compiler
    // Example:
    //    How actually on the chip:   00AA0BBB
    //    How was saved in the file:  BBB0AA00
    // Note, the order of bits is same, the order of locations is only inverted

    // idata->FreqBankNumber is always 0101001000010000

    inst.feedback  = (idata->FeedBackAlg >> 2) & 0x07;
    inst.algorithm = (idata->FeedBackAlg >> 5) & 0x07;
    inst.am = (idata->LFOSens >> 2) & 0x03;
    inst.fm = (idata->LFOSens >> 5) & 0x07;

    const unsigned opnum[] = {OPERATOR1_HR, OPERATOR3_HR, OPERATOR2_HR, OPERATOR4_HR};

    for(unsigned i = 0; i < 4; ++i)
    {
        const unsigned op = opnum[i];
        auto &o = inst.OP[op];
        auto &d = idata->OPRegister[i];

        o.detune    = (d.DetuneFMul >> 1) & 0x07;
        o.fmult     = (d.DetuneFMul >> 4) & 0x0F;

        o.level     = (d.TLevel >> 1) & 0x7F;

        o.ratescale = (d.RsAttRate) & 0x03;
        o.attack    = (d.RsAttRate >> 3) & 0x1F;

        o.am_enable = (d.LFOAmRateDec1) & 0x01;
        o.decay1    = (d.LFOAmRateDec1 >> 3) & 0x1F;

        o.decay2    = (d.Decay2 >> 3) & 0x1F;

        o.sustain   = (d.SusRel) & 0x0F;
        o.release   = (d.SusRel >> 4) & 0x0F;

        o.ssg_eg    = (d.SSGEG >> 4) & 0x0F;
    }

    return FfmtErrCode::ERR_OK;
}

FfmtErrCode Tomsoft_GIN::saveMemInst(INSTRUMENTFILEITEM *idata, const FmBank::Instrument &inst)
{
    memset(idata, 0, sizeof(INSTRUMENTFILEITEM));

    // All bit fields has inverted location in their bytes because of M$' compiler
    // Example:
    //    How actually on the chip:   00AA0BBB
    //    How was saved in the file:  BBB0AA00
    // Note, the order of bits is same, the order of locations is only inverted

    idata->FreqBankNumber = FREQ_BANK_NUMBER;
    idata->LFOEnFreq = 0;
    idata->FeedBackAlg = ((inst.feedback << 2) & (0x07 << 2)) | ((inst.algorithm << 5) & (0x07 << 5));
    idata->LFOSens = ((inst.am << 2) & (0x03 << 2)) | ((inst.fm << 5) & (0x07 << 5));

    const unsigned opnum[] = {OPERATOR1_HR, OPERATOR3_HR, OPERATOR2_HR, OPERATOR4_HR};

    for(unsigned i = 0; i < 4; ++i)
    {
        const unsigned op = opnum[i];
        auto &o = inst.OP[op];
        auto &d = idata->OPRegister[i];

        d.DetuneFMul = ((o.detune << 1) & (0x07 << 1)) | ((o.fmult << 4) & (0x0F << 4));
        d.TLevel = ((o.level << 1) & (0x7F << 1));
        d.RsAttRate = (o.ratescale & 0x03) | ((o.attack << 3) & (0x1F << 3));
        d.LFOAmRateDec1 = (o.am_enable & 0x01) | ((o.decay1 << 3) & (0x1F << 3));
        d.Decay2 = ((o.decay2 << 3) & (0x1F << 3));
        d.SusRel = (o.sustain & 0x0F) | ((o.release << 4) & (0x0F << 4));
        d.SSGEG = ((o.ssg_eg << 4) & (0x0F << 4));
    }

    return FfmtErrCode::ERR_OK;
}
