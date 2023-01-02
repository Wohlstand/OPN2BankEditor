/*
 * OPN2 Bank Editor by Wohlstand, a free tool for music bank editing
 * Copyright (c) 2018-2022 Vitaly Novichkov <admin@wohlnet.ru>
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

class QFile;
class QIODevice;

/**
 * @brief Base class of GYB File Format readers and writers
 */
class Basic_M2V_GYB : public FmBankFormatBase
{
public:
    virtual ~Basic_M2V_GYB() {}

    QString formatExtensionMask() const override;
    QString formatDefaultExtension() const override;

protected:
    FfmtErrCode loadFileVersion1Or2(QFile &file, FmBank &bank, uint8_t version);
    FfmtErrCode loadFileVersion3(QFile &file, FmBank &bank);

    FfmtErrCode saveFileVersion1Or2(QFile &file, FmBank &bank, uint8_t version);
    FfmtErrCode saveFileVersion3(QFile &file, FmBank &bank);

private:
    FfmtErrCode saveFileVersion1Or2FakeChecksum(QIODevice &file, FmBank &bank, uint8_t version);
};

/**
 * @brief Reader of GYB File Format, any version
 */
class M2V_GYB_READ final : public Basic_M2V_GYB
{
public:
    bool        detect(const QString &filePath, char* magic) override;
    FfmtErrCode loadFile(QString filePath, FmBank &bank) override;
    int         formatCaps() const override;
    QString     formatName() const override;
    BankFormats formatId() const override;
};

/**
 * @brief Writer of GYB File Format, version 1
 */
class M2V_GYB_WRITEv1 final : public Basic_M2V_GYB
{
public:
    FfmtErrCode saveFile(QString filePath, FmBank &bank) override;
    int         formatCaps() const override;
    QString     formatName() const override;
    BankFormats formatId() const override;
};

/**
 * @brief Writer of GYB File Format, version 2
 */
class M2V_GYB_WRITEv2 final : public Basic_M2V_GYB
{
public:
    FfmtErrCode saveFile(QString filePath, FmBank &bank) override;
    int         formatCaps() const override;
    QString     formatName() const override;
    BankFormats formatId() const override;
};


/**
 * @brief Writer of GYB File Format, version 3
 */
class M2V_GYB_WRITEv3 final : public Basic_M2V_GYB
{
public:
    FfmtErrCode saveFile(QString filePath, FmBank &bank) override;
    int         formatCaps() const override;
    QString     formatName() const override;
    BankFormats formatId() const override;
};

#endif // FORMAT_M2V_GYB_H
