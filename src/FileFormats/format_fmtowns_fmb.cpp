/*
 * OPN2 Bank Editor by Wohlstand, a free tool for music bank editing
 * Copyright (c) 2018-2026 Vitaly Novichkov <admin@wohlnet.ru>
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

#include "format_fmtowns_fmb.h"
#include "../common.h"

#include <QFileInfo>

#pragma pack(push, 1)
struct fmb_inst
{
    // Instrument name
    char    name[8];

    // Registers per operator

    uint8_t reg_dt_mul[4];
    uint8_t reg_tl[4];
    uint8_t reg_ks_ar[4];
    uint8_t reg_am_d1r[4];
    uint8_t reg_d2r[4];
    uint8_t reg_sl_rr[4];
    uint8_t reg_fb_alg;
    uint8_t reg_lfo_sens;

    // Padding
    uint8_t pad[14];
};

struct fmb_bank
{
    char bank_name[8];
    fmb_inst inst[128];
};
#pragma pack(pop)


bool FMTowns_FMB::detect(const QString &filePath, char *)
{
    QFileInfo i(filePath);
    return i.size() == sizeof(fmb_bank) || i.suffix().toLower() == "fmb";
}

FfmtErrCode FMTowns_FMB::loadFile(QString filePath, FmBank &bank)
{
    const size_t idata_size = sizeof(fmb_bank);
    uint8_t idata[idata_size];
    QFile file(filePath);

    if(!file.open(QIODevice::ReadOnly))
        return FfmtErrCode::ERR_NOFILE;

    if(file.size() < (int)idata_size)
        return FfmtErrCode::ERR_BADFORMAT;

    if(file.read(char_p(idata), idata_size) != (qint64)idata_size)
        return FfmtErrCode::ERR_BADFORMAT;

    fmb_bank *ibank = reinterpret_cast<fmb_bank *>(idata);
    if(!ibank)
        return FfmtErrCode::ERR_BADFORMAT;

    bank.reset(1, 1);

    memcpy(bank.Banks_Melodic[0].name, ibank->bank_name, 8);

    for(unsigned i = 0; i < 128; ++i)
    {
        FmBank::Instrument &ins = bank.Ins_Melodic[i];
        memset(&ins, 0, sizeof(ins));

        FfmtErrCode err = loadMemInst(&ibank->inst[i], ins);
        if(err != FfmtErrCode::ERR_OK)
            ins.is_blank = true;
    }

    return FfmtErrCode::ERR_OK;
}

FfmtErrCode FMTowns_FMB::saveFile(QString filePath, FmBank &bank)
{
    const size_t idata_size = sizeof(fmb_bank);
    uint8_t idata[idata_size];
    QFile file(filePath);

    if(!file.open(QIODevice::WriteOnly))
        return FfmtErrCode::ERR_NOFILE;

    fmb_bank *ibank = reinterpret_cast<fmb_bank *>(idata);

    memset(idata, 0, idata_size);
    memcpy(ibank->bank_name, bank.Banks_Melodic[0].name, 8);

    for(unsigned i = 0; i < (unsigned)std::min(128, (int)bank.Ins_Melodic_box.size()); ++i)
    {
        FmBank::Instrument &ins = bank.Ins_Melodic[i];

        FfmtErrCode err = saveMemInst(&ibank->inst[i], ins);

        if(err != FfmtErrCode::ERR_OK)
            return FfmtErrCode::ERR_BADFORMAT;
    }

    if(file.write(char_p(idata), idata_size) != idata_size)
        return FfmtErrCode::ERR_BADFORMAT;

    file.close();

    return FfmtErrCode::ERR_OK;
}

int FMTowns_FMB::formatCaps() const
{
    return (int)FormatCaps::FORMAT_CAPS_EVERYTHING|(int)FormatCaps::FORMAT_CAPS_MELODIC_ONLY;
}

QString FMTowns_FMB::formatName() const
{
    return "FM Towns FM Bank";
}

QString FMTowns_FMB::formatExtensionMask() const
{
    return "*.fmb";
}

QString FMTowns_FMB::formatDefaultExtension() const
{
    return "fmb";
}

BankFormats FMTowns_FMB::formatId() const
{
    return FORMAT_FMTOWNS_FMB;
}

FfmtErrCode FMTowns_FMB::loadMemInst(const fmb_inst *idata, FmBank::Instrument &inst)
{
    memset(&inst, 0, sizeof(FmBank::Instrument));

    const unsigned opnum[] = {OPERATOR1_HR, OPERATOR3_HR, OPERATOR2_HR, OPERATOR4_HR};

    for(unsigned i = 0; i < 4; ++i)
    {
        const unsigned op = opnum[i];
        inst.name[8] = '\0';
        memcpy(inst.name, idata->name, 8);

        inst.setRegDUMUL(op, idata->reg_dt_mul[i]);
        inst.setRegLevel(op, idata->reg_tl[i]);
        inst.setRegRSAt(op, idata->reg_ks_ar[i]);
        inst.setRegAMD1(op, idata->reg_am_d1r[i]);
        inst.setRegD2(op, idata->reg_d2r[i]);
        inst.setRegSysRel(op, idata->reg_sl_rr[i]);
    }

    inst.setRegFbAlg(idata->reg_fb_alg);
    inst.setRegPanLfoSens(idata->reg_lfo_sens);

    return FfmtErrCode::ERR_OK;
}

FfmtErrCode FMTowns_FMB::saveMemInst(fmb_inst *idata, const FmBank::Instrument &inst)
{
    memset(idata, 0, sizeof(fmb_inst));

    const unsigned opnum[] = {OPERATOR1_HR, OPERATOR3_HR, OPERATOR2_HR, OPERATOR4_HR};

    for(unsigned i = 0; i < 4; ++i)
    {
        const unsigned op = opnum[i];
        memcpy(idata->name, inst.name, 8);

        idata->reg_dt_mul[i] = inst.getRegDUMUL(op);
        idata->reg_tl[i] = inst.getRegLevel(op);
        idata->reg_ks_ar[i] = inst.getRegRSAt(op);
        idata->reg_am_d1r[i] = inst.getRegAMD1(op);
        idata->reg_d2r[i] = inst.getRegD2(op);
        idata->reg_sl_rr[i] = inst.getRegSysRel(op);
    }

    idata->reg_fb_alg = inst.getRegFbAlg();
    idata->reg_lfo_sens = inst.getRegPanLfoSens();

    return FfmtErrCode::ERR_OK;
}
