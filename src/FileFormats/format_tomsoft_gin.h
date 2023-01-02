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

#ifndef FORMAT_TOMSOFT_GIN_H
#define FORMAT_TOMSOFT_GIN_H

#include "ffmt_base.h"

typedef struct tagInstrumentFileItem INSTRUMENTFILEITEM;
typedef struct tagInstrumentFile INSTRUMENTFILE;

/**
 * @brief Reader and Writer of the GEMS File Format
 */
class Tomsoft_GIN final : public FmBankFormatBase
{
public:
    bool        detect(const QString &filePath, char* magic) override;
    FfmtErrCode loadFile(QString filePath, FmBank &bank) override;
    FfmtErrCode saveFile(QString filePath, FmBank &bank) override;
    int         formatCaps() const override;
    QString     formatName() const override;
    QString     formatExtensionMask() const override;
    QString     formatDefaultExtension() const override;
    BankFormats formatId() const override;

private:
    FfmtErrCode loadMemInst(const INSTRUMENTFILEITEM *idata, FmBank::Instrument &inst);
    FfmtErrCode saveMemInst(INSTRUMENTFILEITEM *idata, const FmBank::Instrument &inst);
};

#endif // FORMAT_TOMSOFT_GIN_H
