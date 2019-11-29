/*
 * OPN2 Bank Editor by Wohlstand, a free tool for music bank editing
 * Copyright (c) 2018-2019 Vitaly Novichkov <admin@wohlnet.ru>
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

#ifndef FORMAT_M2V_GYB_H
#define FORMAT_M2V_GYB_H

#include "ffmt_base.h"

/**
 * @brief Reader and Writer of the GYB File Format
 */
class M2V_GYB final : public FmBankFormatBase
{
public:
    bool        detect(const QString &filePath, char* magic) override;
    FfmtErrCode loadFile(QString filePath, FmBank &bank) override;
    FfmtErrCode saveFile(QString filePath, FmBank &bank) override;
    int         formatCaps() const override;
    QString     formatName() const override;
    QString     formatExtensionMask() const override;
    BankFormats formatId() const override;

protected:
    FfmtErrCode saveFileWithVersion(QString filePath, FmBank &bank, uint8_t version);
};

#endif // FORMAT_M2V_GYB_H
