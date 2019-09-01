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

#include "text_format.h"
#include "text_format_tokens.h"
#include "metaparameter.h"
#include <cstring>
#include <cctype>

///
template <class T> TextFormat &TextFormat::operator<<(const T &token)
{
    m_valid = m_valid && token.isValid();
    m_tokens.emplace_back(new T(token));
    return *this;
}

TextFormat &TextFormat::operator<<(const char *tokenText)
{
    using namespace TextFormatTokens;

    if (Whitespace(tokenText).isValid())
        *this << Whitespace(tokenText);
    else
        *this << Symbol(tokenText);

    return *this;
}

///
std::string TextFormat::formatInstrument(const FmBank::Instrument &ins) const
{
    std::string text;

    if (!isValid())
        return text;

    using namespace TextFormatTokens;

    for(const TokenPtr &token : m_tokens)
    {
        switch(token->type())
        {
        default:
            text.append(token->text());
            break;
        case T_Val:
        {
            int value = static_cast<Val &>(*token).parameter()->get(ins);
            char buffer[32];
            std::sprintf(buffer, static_cast<Val &>(*token).format(), value);
            text.append(buffer);
            break;
        }
        case T_NameString:
            text.append(ins.name);
            break;
        case T_QuotedNameString:
            text.push_back('"');
            text.append(ins.name);
            text.push_back('"');
            break;
        }
    }

    return text;
}

bool TextFormat::parseInstrument(const char *text, FmBank::Instrument &ins) const
{
    ins = FmBank::emptyInst();

    const std::string &lc = m_lineComment;
    const std::string &lkp = m_lineKeepPrefix;

    /// begin preprocess
    std::string lineFilterBuffer;
    if(!lkp.empty())
    {
        // delete lines not matching the prefix
        char c;
        const char *p = text;
        bool atLineStart = true;
        lineFilterBuffer.reserve(1024);
        while((c = *p) != '\0')
        {
            if(atLineStart && strncmp(p, lkp.c_str(), lkp.size()) != 0)
            {
                while(c != '\0' && c != '\r' && c != '\n')
                    c = *++p;
                if(c != '\0')
                {
                    ++p;
                    atLineStart = true;
                }
            }
            else
            {
                lineFilterBuffer.push_back(c);
                ++p;
                atLineStart = c == '\r' || c == '\n';
            }
        }
        text = lineFilterBuffer.c_str();
    }
    /// end preprocess

    ///
    auto skipWhitespace = [&text, lc]()
    {
        for(bool endWs = false; !endWs;)
        {
            if (std::isspace((unsigned char)*text))
                ++text;
            else if(!lc.empty() && std::strncmp(text, lc.data(), lc.size()) == 0)
            {
                text += lc.size();
                while(*text != '\0' && *text != '\r' && *text != '\n')
                    ++text;
            }
            else
                endWs = true;
        }
    };

    auto readUntilEolOrComment = [&text, lc]() -> std::string
    {
        const char *begin = text;
        while(*text != '\0' && *text != '\r' && *text != '\n' &&
              !(!lc.empty() && std::strncmp(text, lc.data(), lc.size()) == 0))
              ++text;
        return std::string(begin, text);
    };

    ///
    using namespace TextFormatTokens;

    for(const TokenPtr &token : m_tokens)
    {
        skipWhitespace();

        switch(token->type())
        {
        case T_Symbol:
        {
            const char *sym = static_cast<Symbol &>(*token).text();
            size_t len = strlen(sym);
            if(std::strncmp(text, sym, len))
                return false;
            text += len;
            break;
        }
        case T_Whitespace:
            break;
        case T_Int:
        {
            int value;
            unsigned count;
            if(std::sscanf(text, "%d%n", &value, &count) != 1)
                return false;
            text += count;
            break;
        }
        case T_AlphaNumString:
        {
            std::string value;
            value.reserve(256);
            for(char c; (c = *text) != '\0' && std::isalnum((unsigned char)c); ++text)
                value.push_back(c);
            if (value.empty())
                return false;
            break;
        }
        case T_Val:
        {
            int value;
            unsigned count;
            if(std::sscanf(text, "%d%n", &value, &count) != 1)
                return false;
            text += count;
            const MetaParameter *mp = static_cast<Val &>(*token).parameter();
            mp->set(ins, mp->clamp(value));
            break;
        }
        case T_NameString:
        {
            std::string name = readUntilEolOrComment();
            std::strncpy(ins.name, name.c_str(), 32);
            break;
        }
        case T_QuotedNameString:
        {
            if(*text++ != '"')
                return false;

            std::string name;
            name.reserve(256);
            while(*text != '\0' && *text != '"')
                name.push_back(*text++);

            if(*text++ != '"')
                return false;

            std::strncpy(ins.name, name.c_str(), 32);
            break;
        }
        }
    }

    return true;
}

///
static TextFormat createVopmFormat()
{
    TextFormat tf;

    tf.setName("VOPM");
    tf.setLineComment("//");

    using namespace TextFormatTokens;

    tf << "@" << ":"
              << " " << Int() << " " << NameString() << "\n"
       << "LFO" << ":"
                << " " << Int() << " " << Int()
                << " " << Int() << " " << Int()
                << " " << Int() << "\n"
       << "CH" << ":"
               << " " << Int(64) << " " << Val("fb")
               << " " << Val("alg") << " " << Int()
               << " " << Int() << " " << Int(120)
               << " " << Int() << "\n";

    for(int o = 0; o < 4; ++o)
    {
        const char *opnames[] = {"M1", "C1", "M2", "C2"};
        int op = MP_Operator1 + o;
        tf << opnames[o] << ":"
           << " " << Val("ar", op) << " " << Val("d1r", op)
           << " " << Val("d2r", op) << " " << Val("rr", op)
           << " " << Val("d1l", op) << " " << Val("tl", op)
           << " " << Val("rs", op) << " " << Val("mul", op)
           << " " << Val("dt", op) << " " << Int()
           << " " << Val("am", op) << "\n";
    }

    return tf;
}

static TextFormat createPmdFormat()
{
    TextFormat tf;

    tf.setName("PMD");
    tf.setLineComment(";");

    using namespace TextFormatTokens;

    tf << "@" << " " << Int() << " " << Val("alg") << " " << Val("fb") << "\n";

    for(int o = 0; o < 4; ++o)
    {
        int op = MP_Operator1 + o;
        tf << " " << Val("ar", op) << " " << Val("d1r", op)
           << " " << Val("d2r", op) << " " << Val("rr", op)
           << " " << Val("d1l", op) << " " << Val("tl", op)
           << " " << Val("rs", op) << " " << Val("mul", op)
           << " " << Val("dt", op) << " " << Val("am", op) << "\n";
    }

    return tf;
}

static TextFormat createFmpFormat()
{
    TextFormat tf;

    tf.setName("FMP");
    tf.setLineKeepPrefix("'");

    using namespace TextFormatTokens;

    // permissive parsing on this line
    tf << "'" << "@" << " " << AlphaNumString("FA") << " " << AlphaNumString("0") << "\n";

    for(int o = 0; o < 4; ++o)
    {
        int op = MP_Operator1 + o;
        tf << "'" << "@"
           << " " << Val("ar", op) << "," << Val("d1r", op)
           << "," << Val("d2r", op) << "," << Val("rr", op)
           << "," << Val("d1l", op) << "," << Val("tl", op)
           << "," << Val("rs", op) << "," << Val("mul", op)
           << "," << Val("dt", op) << "," << Val("am", op) << "\n";
    }

    tf << "'" << "@" << " " << Val("alg") << "," << Val("fb") << "\n";

    return tf;
}

static TextFormat createNotexFormat()
{
    TextFormat tf;

    tf.setName("NOTE.X");
    tf.setLineComment("//");

    using namespace TextFormatTokens;

    tf << "@" << Int(0, "%d") << " " << "=" << " " << "{" << "\n";

    for(int o = 0; o < 4; ++o)
    {
        int op = MP_Operator1 + o;
        tf << Val("ar", op) << "," << Val("d1r", op)
           << "," << Val("d2r", op) << "," << Val("rr", op)
           << "," << Val("d1l", op) << "," << Val("tl", op)
           << "," << Val("rs", op) << "," << Val("mul", op)
           << "," << Val("dt", op) << "," << Int()
           << "," << Val("am", op) << "\n";
    }

    tf << Val("alg") << "," << Val("fb") << "," << Int(15) << "\n"
       << "}" << "\n";

    return tf;
}

static TextFormat createNrtdrvFormat()
{
    TextFormat tf;

    tf.setName("NRTDRV");
    tf.setLineComment(";");

    using namespace TextFormatTokens;

    tf << "@" << Int() << " " << "{" << "\n";

    tf << Int() << "," << Val("alg") << "," << Val("fb") << "," << Int(15) << "\n";

    for(int o = 0; o < 4; ++o)
    {
        int op = MP_Operator1 + o;
        tf << Val("ar", op) << "," << Val("d1r", op)
           << "," << Val("d2r", op) << "," << Val("rr", op)
           << "," << Val("d1l", op) << "," << Val("tl", op)
           << "," << Val("rs", op) << "," << Val("mul", op)
           << "," << Val("dt", op) << "," << Int()
           << "," << Val("am", op) << "\n";
    }

    tf << "}" << "\n";

    return tf;
}

static TextFormat createMucom88Format()
{
    TextFormat tf;

    tf.setName("MUCOM88");
    tf.setLineComment(";");

    using namespace TextFormatTokens;

    tf << "@" << Int() << " " << ":" << " " << "{" << "\n";

    tf << Val("fb") << "," << Val("alg") << "\n";

    for(int o = 0; o < 4; ++o)
    {
        int op = MP_Operator1 + o;
        tf << Val("ar", op) << "," << Val("d1r", op)
           << "," << Val("d2r", op) << "," << Val("rr", op)
           << "," << Val("d1l", op) << "," << Val("tl", op)
           << "," << Val("rs", op) << "," << Val("mul", op)
           << "," << Val("dt", op);
        if (o == 3)
            tf << "," << QuotedNameString();
        tf << "\n";
    }

    tf << "}" << "\n";

    return tf;
}

///
const TextFormat &TextFormat::vopmFormat()
{
    static TextFormat tf = createVopmFormat();
    return tf;
}

const TextFormat &TextFormat::pmdFormat()
{
    static TextFormat tf = createPmdFormat();
    return tf;
}

const TextFormat &TextFormat::fmpFormat()
{
    static TextFormat tf = createFmpFormat();
    return tf;
}

const TextFormat &TextFormat::notexFormat()
{
    static TextFormat tf = createNotexFormat();
    return tf;
}

const TextFormat &TextFormat::nrtdrvFormat()
{
    static TextFormat tf = createNrtdrvFormat();
    return tf;
}

const TextFormat &TextFormat::mucom88Format()
{
    static TextFormat tf = createMucom88Format();
    return tf;
}

const std::vector<const TextFormat *> &TextFormat::allFormats()
{
    static const std::vector<const TextFormat *> all = {
        &vopmFormat(),
        &pmdFormat(),
        &fmpFormat(),
        &notexFormat(),
        &nrtdrvFormat(),
        &mucom88Format(),
    };
    return all;
}

const TextFormat *TextFormat::getFormatByName(const std::string &name)
{
    for(const TextFormat *tf : TextFormat::allFormats())
        if(name == tf->name())
            return tf;
    return nullptr;
}
