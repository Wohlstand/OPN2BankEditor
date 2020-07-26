/*
 * OPN2 Bank Editor by Wohlstand, a free tool for music bank editing
 * Copyright (c) 2017-2020 Vitaly Novichkov <admin@wohlnet.ru>
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
#include <list>
#include <cstring>
#include <cctype>
#include <climits>
#include <cassert>

///
template <class T> GrammaticalTextFormat &GrammaticalTextFormat::operator<<(const T &token)
{
    m_valid = m_valid && token.isValid();
    m_tokens.emplace_back(new T(token));
    return *this;
}

GrammaticalTextFormat &GrammaticalTextFormat::operator<<(const char *tokenText)
{
    using namespace TextFormatTokens;

    if (Whitespace(tokenText).isValid())
        *this << Whitespace(tokenText);
    else
        *this << Symbol(tokenText);

    return *this;
}

///
std::string GrammaticalTextFormat::formatInstrument(const FmBank::Instrument &ins) const
{
    std::string text;

    if (!isValid())
        return text;

    using namespace TextFormatTokens;

    std::list<const Token *> tokens;
    for(const TokenPtr &token : m_tokens)
        tokens.push_back(token.get());

    while(!tokens.empty())
    {
        const Token *token = tokens.front();
        tokens.pop_front();

        switch(token->type())
        {
        default:
            text.append(token->text());
            break;
        case T_Val:
        {
            int value = static_cast<const Val &>(*token).parameter()->get(ins);
            char buffer[32];
            std::sprintf(buffer, static_cast<const Val &>(*token).format(), value);
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
        case T_Conditional:
        {
            const Conditional &cond = static_cast<const Conditional &>(*token);

            // go through the default branch of the conditional
            bool value = cond.defaultValue();
            const TokenList &branch = cond.eval(value);
            for(size_t i = branch.items.size(); i-- > 0;)
                tokens.push_front(branch.items[i].get());
            if(value)
                tokens.push_front(&cond.condition());

            break;
        }
        }
    }

    return text;
}

bool GrammaticalTextFormat::parseInstrument(const char *text, FmBank::Instrument &ins) const
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
    auto skipWhitespace = [&text, lc](const char *ws)
    {
        for(bool endWs = false; !endWs;)
        {
            if (std::strchr(ws, *text))
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

    auto readNextInt = [](const char *&text, int *dest) -> bool
    {
        char *end = nullptr;
        long value = std::strtol(text, &end, 10);
        if(!end || end == text || value < INT_MIN || value > INT_MAX)
            return false;
        text = end;
        if(dest)
            *dest = (int)value;
        return true;
    };

    ///
    using namespace TextFormatTokens;

    const char *defaultWhiteChars = TextFormatTokens::Whitespace("").whiteChars();

    std::list<const Token *> tokens;
    for(const TokenPtr &token : m_tokens)
        tokens.push_back(token.get());

    while(!tokens.empty())
    {
        const Token *token = tokens.front();
        tokens.pop_front();

        skipWhitespace(defaultWhiteChars);

        bool success = true;

        // conditional: a special case
        const Conditional *ifelse = nullptr;
        if(token->type() == T_Conditional)
        {
            ifelse = static_cast<const Conditional *>(token);
            token = &ifelse->condition();
        }

        switch(token->type())
        {
        case T_Symbol:
        {
            const char *sym = static_cast<const Symbol &>(*token).text();
            size_t len = strlen(sym);
            if(std::strncmp(text, sym, len))
            {
                success = false;
                break;
            }
            text += len;
            break;
        }
        case T_Whitespace:
        {
            // if there are special characters set having whitespace-like behavior
            const char *whiteChars = static_cast<const Whitespace &>(*token).whiteChars();
            skipWhitespace(whiteChars);
            break;
        }
        case T_Int:
        {
            int value;
            if (!readNextInt(text, &value))
                success = false;
            break;
        }
        case T_AlphaNumString:
        {
            std::string value;
            value.reserve(256);

            const char *pos = text;
            for(char c; (c = *pos) != '\0' && std::isalnum((unsigned char)c); ++pos)
                value.push_back(c);

            if(value.empty())
            {
                success = false;
                break;
            }

            text = pos;
            break;
        }
        case T_Val:
        {
            int value;
            if (!readNextInt(text, &value))
            {
                success = false;
                break;
            }
            const MetaParameter *mp = static_cast<const Val &>(*token).parameter();
            if (!mp)
            {
                success = false;
                break;
            }
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
            const char *pos = text;
            if(*pos++ != '"')
            {
                success = false;
                break;
            }

            std::string name;
            name.reserve(256);
            while(*pos != '\0' && *pos != '"')
                name.push_back(*pos++);

            if(*pos++ != '"')
            {
                success = false;
                break;
            }

            std::strncpy(ins.name, name.c_str(), 32);
            text = pos;
            break;
        }
        case T_Conditional:
            assert(false);
            break;
        }

        if(!success && !ifelse)
            return false;
        else if(ifelse)
        {
            for(const TokenSharedPtr &branchToken : ifelse->eval(success).items)
                tokens.push_front(branchToken.get());
        }
    }

    return true;
}

///
void CompositeTextFormat::setWriterFormat(const std::shared_ptr<TextFormat> &format)
{
    m_writeFormat = format;
}

void CompositeTextFormat::addReaderFormat(const std::shared_ptr<TextFormat> &format)
{
    m_readFormats.push_back(format);
}

std::string CompositeTextFormat::formatInstrument(const FmBank::Instrument &ins) const
{
    TextFormat *tf = m_writeFormat.get();
    return tf ? tf->formatInstrument(ins) : std::string();
}

bool CompositeTextFormat::parseInstrument(const char *text, FmBank::Instrument &ins) const
{
    for(const std::shared_ptr<TextFormat> &tf : m_readFormats)
    {
        if (tf->parseInstrument(text, ins))
            return true;
    }
    return false;
}

///
static GrammaticalTextFormat createVopmFormat()
{
    GrammaticalTextFormat tf;

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

static GrammaticalTextFormat createPmdOpnFormat()
{
    GrammaticalTextFormat tf;

    tf.setName("PMD (OPN)");
    tf.setLineComment(";");

    using namespace TextFormatTokens;

    tf << "@" << " " << Int() << " " << Val("alg") << " " << Val("fb")
       << " " << Conditional(
           TokenSharedPtr(new Symbol("=")),
           TokenList{} << Whitespace(" ") << NameString(),
           TokenList{})
       << "\n";

    // PMD instrument can have comma, handle it as whitespace separator
    auto sep = []() -> Whitespace { return Whitespace(" ", ", \t\r\n"); };
    auto optionalSep = []() -> Whitespace { return Whitespace("", ", \t\r\n"); };

    for(int o = 0; o < 4; ++o)
    {
        int op = MP_Operator1 + o;
        tf << sep() << Val("ar", op) << sep() << Val("d1r", op)
           << sep() << Val("d2r", op) << sep() << Val("rr", op)
           << sep() << Val("d1l", op) << sep() << Val("tl", op)
           << sep() << Val("rs", op) << sep() << Val("mul", op)
           << sep() << Val("dt", op) << sep() << Val("am", op)
           << optionalSep() << "\n";
    }

    return tf;
}

static GrammaticalTextFormat createPmdOpmFormat()
{
    GrammaticalTextFormat tf;

    tf.setName("PMD (OPM)");
    tf.setLineComment(";");

    using namespace TextFormatTokens;

    tf << "@" << " " << Int() << " " << Val("alg") << " " << Val("fb")
       << " " << Conditional(
           TokenSharedPtr(new Symbol("=")),
           TokenList{} << Whitespace(" ") << NameString(),
           TokenList{})
       << "\n";

    // PMD instrument can have comma, handle it as whitespace separator
    auto sep = []() -> Whitespace { return Whitespace(" ", ", \t\r\n"); };
    auto optionalSep = []() -> Whitespace { return Whitespace("", ", \t\r\n"); };

    for(int o = 0; o < 4; ++o)
    {
        int op = MP_Operator1 + o;
        tf << sep() << Val("ar", op) << sep() << Val("d1r", op)
           << sep() << Val("d2r", op) << sep() << Val("rr", op)
           << sep() << Val("d1l", op) << sep() << Val("tl", op)
           << sep() << Val("rs", op) << sep() << Val("mul", op)
           << sep() << Val("dt", op) << sep() << Int()
           << sep() << Val("am", op)
           << optionalSep() << "\n";
    }

    return tf;
}

static CompositeTextFormat createPmdFormat()
{
    CompositeTextFormat tf;
    tf.setName("PMD");

    std::shared_ptr<GrammaticalTextFormat> tfOpn(
        new GrammaticalTextFormat(createPmdOpnFormat()));
    std::shared_ptr<GrammaticalTextFormat> tfOpm(
        new GrammaticalTextFormat(createPmdOpmFormat()));

    // always write it as OPN
    tf.setWriterFormat(tfOpn);

    // try OPM first, then OPN; this order is unambiguous because of size
    tf.addReaderFormat(tfOpm);
    tf.addReaderFormat(tfOpn);

    return tf;
}

static GrammaticalTextFormat createFmp7FAFormat()
{
    GrammaticalTextFormat tf;

    tf.setName("FMP7FA");
    tf.setLineKeepPrefix("'");

    using namespace TextFormatTokens;

    // permissive parsing on this line
    tf << "'" << "@" << " " << Symbol("FA") << " " << AlphaNumString("0") << "\n";

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

static GrammaticalTextFormat createFmp7FFormat()
{
    GrammaticalTextFormat tf;

    tf.setName("FMP7F");
    tf.setLineKeepPrefix("'");

    using namespace TextFormatTokens;

    // permissive parsing on this line
    tf << "'" << "@" << " " << Symbol("F") << " " << AlphaNumString("0") << "\n";

    for(int o = 0; o < 4; ++o)
    {
        int op = MP_Operator1 + o;
        tf << "'" << "@"
           << " " << Val("ar", op) << "," << Val("d1r", op)
           << "," << Val("d2r", op) << "," << Val("rr", op)
           << "," << Val("d1l", op) << "," << Val("tl", op)
           << "," << Val("rs", op) << "," << Val("mul", op)
           << "," << Val("dt", op) << "\n";
    }

    tf << "'" << "@" << " " << Val("alg") << "," << Val("fb") << "\n";

    return tf;
}

static GrammaticalTextFormat createFmp4Format()
{
    GrammaticalTextFormat tf;

    tf.setName("FMP4");
    tf.setLineKeepPrefix("'");

    using namespace TextFormatTokens;

    // permissive parsing on this line
    tf << "'" << "@" << " " << AlphaNumString("0") << "\n";

    for(int o = 0; o < 4; ++o)
    {
        int op = MP_Operator1 + o;
        tf << "'" << "@"
           << " " << Val("ar", op) << "," << Val("d1r", op)
           << "," << Val("d2r", op) << "," << Val("rr", op)
           << "," << Val("d1l", op) << "," << Val("tl", op)
           << "," << Val("rs", op) << "," << Val("mul", op)
           << "," << Val("dt", op) << "\n";
    }

    tf << "'" << "@" << " " << Val("alg") << "," << Val("fb") << "\n";

    return tf;
}

static CompositeTextFormat createFmpFormat()
{
    CompositeTextFormat tf;
    tf.setName("FMP");

    std::shared_ptr<GrammaticalTextFormat> tfFmp7FA(
        new GrammaticalTextFormat(createFmp7FAFormat()));
    std::shared_ptr<GrammaticalTextFormat> tfFmp7F(
        new GrammaticalTextFormat(createFmp7FFormat()));
    std::shared_ptr<GrammaticalTextFormat> tfFmp4(
        new GrammaticalTextFormat(createFmp4Format()));

    // always write it as FMP7FA
    tf.setWriterFormat(tfFmp7FA);

    // try FMP7 (FA -> F) -> FMP4
    tf.addReaderFormat(tfFmp7FA);
    tf.addReaderFormat(tfFmp7F);
    tf.addReaderFormat(tfFmp4);

    return tf;
}

static GrammaticalTextFormat createNotexFormat()
{
    GrammaticalTextFormat tf;

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

static GrammaticalTextFormat createNrtdrvFormat()
{
    GrammaticalTextFormat tf;

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

static GrammaticalTextFormat createMucom88Format()
{
    GrammaticalTextFormat tf;

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

static GrammaticalTextFormat createmml2vgmFormat()
{
    GrammaticalTextFormat tf;

    tf.setName("MML2VGM");
    tf.setLineKeepPrefix("'");

    using namespace TextFormatTokens;

    // permissive parsing on this line
    tf << "'" << "@" << " " << AlphaNumString("N") << " " << AlphaNumString("000") << "\n";

    for(int o = 0; o < 4; ++o)
    {
        int op = MP_Operator1 + o;
        tf << "'" << "@"
           << " " << Val("ar", op) << "," << Val("d1r", op)
           << "," << Val("d2r", op) << "," << Val("rr", op)
           << "," << Val("d1l", op) << "," << Val("tl", op)
           << "," << Val("rs", op) << "," << Val("mul", op)
           << "," << Val("dt", op) << "," << Val("am", op) 
           << "," << Val("ssg", op) << "\n";
    }

    tf << "'" << "@" << " " << Val("alg") << "," << Val("fb") << "\n";

    return tf;
}

///
const TextFormat &TextFormats::vopmFormat()
{
    static GrammaticalTextFormat tf = createVopmFormat();
    return tf;
}

const TextFormat &TextFormats::pmdFormat()
{
    static CompositeTextFormat tf = createPmdFormat();
    return tf;
}

const TextFormat &TextFormats::fmpFormat()
{
    static CompositeTextFormat tf = createFmpFormat();
    return tf;
}

const TextFormat &TextFormats::notexFormat()
{
    static GrammaticalTextFormat tf = createNotexFormat();
    return tf;
}

const TextFormat &TextFormats::nrtdrvFormat()
{
    static GrammaticalTextFormat tf = createNrtdrvFormat();
    return tf;
}

const TextFormat &TextFormats::mucom88Format()
{
    static GrammaticalTextFormat tf = createMucom88Format();
    return tf;
}

const TextFormat &TextFormats::mml2vgmFormat()
{
    static GrammaticalTextFormat tf = createmml2vgmFormat();
    return tf;
}

const std::vector<const TextFormat *> &TextFormats::allFormats()
{
    static const std::vector<const TextFormat *> all = {
        &vopmFormat(),
        &pmdFormat(),
        &fmpFormat(),
        &notexFormat(),
        &nrtdrvFormat(),
        &mucom88Format(),
        &mml2vgmFormat(),
    };
    return all;
}

const TextFormat *TextFormats::getFormatByName(const std::string &name)
{
    for(const TextFormat *tf : allFormats())
        if(name == tf->name())
            return tf;
    return nullptr;
}
