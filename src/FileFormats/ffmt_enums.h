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

#ifndef FFMT_ENUMS_H
#define FFMT_ENUMS_H

/**
 * @brief The bank formats enum
 */
enum BankFormats
{
    FORMAT_UNKNOWN         = -1,
    FORMAT_WOHLSTAND_OPN2  = 0,
    FORMAT_VGM_IMPORTER    = 1,
    FORMAT_GEMS_BNK        = 2,
    FORMAT_M2V_GYB_READ    = 3,
    FORMAT_M2V_GYB_WRITEv1 = 4,
    FORMAT_M2V_GYB_WRITEv2 = 5,
    FORMAT_M2V_GYB_WRITEv3 = 6,
    FORMAT_TOMSOFT_GIN     = 7,
    FORMAT_GYM_IMPORTER    = 8,
    FORMAT_SAXMAN_YMX      = 9,
    FORMAT_S98_IMPORTER    = 10,
    FORMAT_TX81Z_IMPORTER  = 11,
    FORMAT_OPM             = 12,
    FORMAT_MUCOM88_DAT     = 13,

    FORMATS_END,
    FORMATS_BEGIN = FORMAT_WOHLSTAND_OPN2,
};

enum InstFormats
{
    FORMAT_INST_UNKNOWN    = -1,
    FORMAT_INST_WOPN2      = 0,
    FORMAT_INST_TFI_MM     = 1,
    FORMAT_INST_DM_DMP     = 2,
    FORMAT_INST_GEMS_PAT   = 3,
    FORMAT_INST_Y12        = 4,
    FORMAT_INST_VGM_MM     = 5,
    FORMAT_INST_BAMBOO_BTI = 6,
};

enum class FormatCaps
{
    //! No capabilities, dummy
    FORMAT_CAPS_NOTHING = 0x00,
    //! Can be opened for editing
    FORMAT_CAPS_OPEN    = 0x01,
    //! Can be saved
    FORMAT_CAPS_SAVE    = 0x02,
    //! Can be opened to import data into currently opened bank
    FORMAT_CAPS_IMPORT  = 0x04,
    //! On every save the delay measures will be generated
    FORMAT_CAPS_NEEDS_MEASURE = 0x08,
    //! Format is able to keep only one bank only
    FORMAT_CAPS_GM_BANK = 0x10,

    //! Open/Save/Import capabilities
    FORMAT_CAPS_EVERYTHING = FORMAT_CAPS_OPEN|FORMAT_CAPS_SAVE|FORMAT_CAPS_IMPORT,
    //! Open/Save/Import capabilities
    FORMAT_CAPS_EVERYTHING_GM = FORMAT_CAPS_OPEN|FORMAT_CAPS_SAVE|FORMAT_CAPS_IMPORT|FORMAT_CAPS_GM_BANK
};

/**
 * @brief Error codes
 */
enum class FfmtErrCode
{
    //! Everything is OK
    ERR_OK=0,
    //! File wasn't opened because not exists or permission denied
    ERR_NOFILE,
    //! File format is corrupted/invalid/damaged
    ERR_BADFORMAT,
    //! Reading or Writing operation is not implemented for this file format
    ERR_NOT_IMPLEMENTED,
    //! Detected file format is not supported
    ERR_UNSUPPORTED_FORMAT,
    //! Any other error
    ERR_UNKNOWN
};

class QString;
namespace FileFormats
{
    /**
     * @brief Obtain a translated text from an error code.
     */
    QString getErrorText(FfmtErrCode err);
}

#endif // FFMT_ENUMS_H
