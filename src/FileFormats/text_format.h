/*
 * OPN2 Bank Editor by Wohlstand, a free tool for music bank editing
 * Copyright (c) 2017-2021 Vitaly Novichkov <admin@wohlnet.ru>
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
    virtual ~TextFormat() {}

    const std::string &name() const { return m_name; }
    void setName(const std::string &name) { m_name = name; }

    virtual std::string formatInstrument(const FmBank::Instrument &ins) const = 0;
    virtual bool parseInstrument(const char *text, FmBank::Instrument &ins) const = 0;

protected:
    std::string m_name;
};

///
namespace TextFormats
{
    const TextFormat &vopmFormat();
    const TextFormat &pmdFormat();
    const TextFormat &fmpFormat();
    const TextFormat &notexFormat();
    const TextFormat &nrtdrvFormat();
    const TextFormat &mucom88Format();
    const TextFormat &mml2vgmFormat();

    const std::vector<const TextFormat *> &allFormats();
    const TextFormat *getFormatByName(const std::string &name);
};

///
class GrammaticalTextFormat : public TextFormat
{
public:
    bool isValid() const { return m_valid; }

    //
    void setLineComment(const std::string &com) { m_lineComment = com; }
    void setLineKeepPrefix(const std::string &prefix) { m_lineKeepPrefix = prefix; }

    //
    template <class T> GrammaticalTextFormat &operator<<(const T &token);
    GrammaticalTextFormat &operator<<(const char *tokenText);

    //
    std::string formatInstrument(const FmBank::Instrument &ins) const override;
    bool parseInstrument(const char *text, FmBank::Instrument &ins) const override;

protected:
    bool m_valid = true;
    std::string m_lineComment; // a line comment syntax, most often `//` or `;`
    std::string m_lineKeepPrefix; // a prefix of text lines not to discard (`'` in the case of FMP)
    std::vector<TextFormatTokens::TokenPtr> m_tokens;
};

///
class CompositeTextFormat : public TextFormat
{
public:
    void setWriterFormat(const std::shared_ptr<TextFormat> &format);
    void addReaderFormat(const std::shared_ptr<TextFormat> &format);

    std::string formatInstrument(const FmBank::Instrument &ins) const override;
    bool parseInstrument(const char *text, FmBank::Instrument &ins) const override;

private:
    std::shared_ptr<TextFormat> m_writeFormat;
    std::vector<std::shared_ptr<TextFormat>> m_readFormats;
};

#endif
