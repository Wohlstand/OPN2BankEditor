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

#include "text_format_tokens.h"
#include "metaparameter.h"
#include <cstdio>
#include <cstring>
#include <cctype>

namespace TextFormatTokens {

///
bool Whitespace::isValid() const
{
    for(const char *p = m_text; *p != '\0'; ++p)
    {
        if(!std::strchr(m_whiteChars, *p))
            return false;
    }
    return true;
}

///
Conditional::Conditional(TokenSharedPtr cond, TokenList ifTrue, TokenList ifFalse, bool defaultValue)
    : m_cond(std::move(cond)), m_ifTrue(std::move(ifTrue)), m_ifFalse(std::move(ifFalse)), m_defaultValue(defaultValue)
{
}

///
Int::Int(int defaultValue, const char *format)
    : m_defaultValue(defaultValue), m_format(format)
{
}

const char *Int::text() const
{
    std::sprintf(m_buf, m_format, m_defaultValue);
    return m_buf;
}

///
Val::Val(const char *id, unsigned flags, const char *format)
    : m_format(format)
{
    for(const MetaParameter &mp : MP_instrument)
    {
        if((mp.flags & flags) == flags && !std::strcmp(id, mp.name))
        {
            m_param = &mp;
            break;
        }
    }
}

const char *Val::text() const
{
    if(!isValid())
        return "";

    return m_param->name;
}

bool Val::isValid() const
{
    return m_param != nullptr;
}

} // namespace TextFormatTokens
