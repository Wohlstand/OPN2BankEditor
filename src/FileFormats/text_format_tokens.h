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

#ifndef TEXT_FORMAT_TOKENS_H
#define TEXT_FORMAT_TOKENS_H

struct MetaParameter;

namespace TextFormatTokens {

///
enum Type
{
    T_Symbol,
    T_Whitespace,
    T_Int,
    T_Val,
    T_NameString,
    T_QuotedNameString,
};

///
class Token
{
public:
    virtual ~Token() {}
    virtual Type type() const = 0;
    virtual bool isValid() const { return true; }
    virtual const char *text() const = 0;
};

///
class StaticText : public Token
{
public:
    explicit StaticText(const char *text) : m_text(text) {}
    const char *text() const override { return m_text; }

protected:
    const char *const m_text;
};

///
class Symbol : public StaticText
{
public:
    explicit Symbol(const char *text) : StaticText(text) {}
    Type type() const override { return T_Symbol; }
};

///
class Whitespace : public StaticText
{
public:
    explicit Whitespace(const char *text) : StaticText(text) {}
    Type type() const override { return T_Whitespace; }
    bool isValid() const override;
};

///
class Int : public Token
{
public:
    explicit Int(int defaultValue = 0);
    Type type() const override { return T_Int; }
    const char *text() const override;

private:
    int m_defaultValue;
    mutable char m_buf[32];
};

///
class Val : public Token
{
public:
    explicit Val(const char *id, unsigned flags = 0);
    Type type() const override { return T_Val; }
    const char *text() const override;
    bool isValid() const override;

    const MetaParameter *parameter() const { return m_param; }

private:
    const MetaParameter *m_param = nullptr;
};

///
class NameString : public Token
{
public:
    Type type() const override { return T_NameString; }
    const char *text() const override { return "Untitled"; }
    bool isValid() const override { return true; }
};

class QuotedNameString : public Token
{
public:
    Type type() const override { return T_QuotedNameString; }
    const char *text() const override { return "\"Untitled\""; }
    bool isValid() const override { return true; }
};

} // namespace TextFormatTokens

#endif // TEXT_FORMAT_TOKENS_H
