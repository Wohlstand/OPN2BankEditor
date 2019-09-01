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

#ifndef TEXT_FORMAT_H
#define TEXT_FORMAT_H

#include "bank.h"
#include <string>
#include <vector>
#include <memory>

///
namespace TextFormatTokens {
class Token;
typedef std::unique_ptr<Token> TokenPtr;
}

///
class TextFormat
{
public:
    static const TextFormat &vopmFormat();
    static const TextFormat &pmdFormat();
    static const TextFormat &fmpFormat();
    static const TextFormat &notexFormat();
    static const TextFormat &nrtdrvFormat();

    static const std::vector<const TextFormat *> &allFormats();
    static const TextFormat *getFormatByName(const std::string &name);

public:
    bool isValid() const { return m_valid; }

    const std::string &name() const { return m_name; }
    void setName(const std::string &name) { m_name = name; }

    //
    void setLineComment(const std::string &com) { m_lineComment = com; }
    void setLineKeepPrefix(const std::string &prefix) { m_lineKeepPrefix = prefix; }

    //
    template <class T> TextFormat &operator<<(const T &token);
    TextFormat &operator<<(const char *tokenText);

    //
    std::string formatInstrument(const FmBank::Instrument &ins) const;
    bool parseInstrument(const char *text, FmBank::Instrument &ins) const;

protected:
    bool m_valid = true;
    std::string m_name;
    std::string m_lineComment; // a line comment syntax, most often `//` or `;`
    std::string m_lineKeepPrefix; // a prefix of text lines not to discard (`'` in the case of FMP)
    std::vector<TextFormatTokens::TokenPtr> m_tokens;
};

#endif
