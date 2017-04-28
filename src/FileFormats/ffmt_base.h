/*
 * OPL Bank Editor by Wohlstand, a free tool for music bank editing
 * Copyright (c) 2016 Vitaly Novichkov <admin@wohlnet.ru>
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

#ifndef FMBANKFORMATBASE_H
#define FMBANKFORMATBASE_H

#include <QString>
#include "../bank.h"

/*!
 * \brief Base class provides errors enum and commonly used headers
 */
class FmBankFormatBase
{
public:
    /**
     * @brief The bank formats enum
     */
    enum Formats
    {
        FORMAT_UNKNOWN = -1,
        FORMAT_WOHLSTAND_OPN2 =   0,
        FORMAT_VGM_IMPORTER =   1,

        FORMATS_END,
        FORMATS_BEGIN = FORMAT_WOHLSTAND_OPN2,
    };

    enum InsFormats
    {
        FORMAT_INST_UNKNOWN = -1,
        FORMAT_INST_OPN2    = 0,
    };

    static QString getSaveFiltersList();
    static QString getOpenFiltersList();

    static Formats getFormatFromFilter(QString filter);
    static QString getFilterFromFormat(Formats format);

    /**
     * @brief Is given format designed for the instrument importing only
     * @param format file format identificator
     * @return true if this format is instrument import only
     */
    static bool isImportOnly(Formats format);

    /**
     * @brief Error codes
     */
    enum ErrCode
    {
        //! Everything is OK
        ERR_OK=0,
        //! File wasn't opened because not exists or permission denied
        ERR_NOFILE,
        //! File format is corrupted/invalid/damaged
        ERR_BADFORMAT,
        //! Reading or Writing operation is not implemented for this file format
        ERR_NOT_IMLEMENTED,
        //! Detected file format is not supported
        ERR_UNSUPPORTED_FORMAT,
        //! Any other error
        ERR_UNKNOWN
    };

    static int  OpenBankFile(QString filePath, FmBank &bank, Formats *recent=0);
    static int  OpenInstrumentFile(QString filePath, FmBank::Instrument &ins, InsFormats *recent=0, bool *isDrum = 0);
};

#endif // FMBANKFORMATBASE_H
