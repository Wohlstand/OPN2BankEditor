/*
 * OPN2 Bank Editor by Wohlstand, a free tool for music bank editing
 * Copyright (c) 2017-2023 Vitaly Novichkov <admin@wohlnet.ru>
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

#ifndef METAPARAMETER_H
#define METAPARAMETER_H

#include "bank.h"

struct MetaParameter
{
    typedef int (Get)(const FmBank::Instrument &);
    typedef void (Set)(FmBank::Instrument &, int);

    MetaParameter() {}
    MetaParameter(const char *name, Get *get, Set *set, int min, int max, unsigned flags)
        : name(name), get(get), set(set), min(min), max(max), flags(flags) {}

    const char *const name = nullptr;
    Get *const get = nullptr;
    Set *const set = nullptr;
    int min = 0;
    int max = 0;
    const unsigned flags = 0;

    int clamp(int value) const
    {
        value = (value > min) ? value : min;
        value = (value < max) ? value : max;
        return value;
    }
};

enum MetaParameterFlag
{
    MP_None          = 0,
    MP_Measure       = 32,
    MP_Operator1     = 4,  /* all operators set the 3rd flag bit */
    MP_Operator2     = (MP_Operator1 + 1),
    MP_Operator3     = (MP_Operator1 + 2),
    MP_Operator4     = (MP_Operator1 + 3),

    MP_OperatorMask = MP_Operator1|MP_Operator2|MP_Operator3|MP_Operator4
};

static const MetaParameter MP_instrument[] =
{
#define G(x) (+[](const FmBank::Instrument &ins) -> int { return (x); })
#define S(x) (+[](FmBank::Instrument &ins, int val) { (x) = val; })
#define GS(x) G(x), S(x)

    {"alg", GS(ins.algorithm), 0, 7, MP_None},
    {"fb", GS(ins.feedback), 0, 7, MP_None},
    {"ams", GS(ins.am), 0, 3, MP_None},
    {"fms", GS(ins.fm), 0, 7, MP_None},
#define OP(n, flags)                                                           \
    {"ar", GS(ins.OP[n].attack), 0, 31, (flags)}, \
    {"d1r", GS(ins.OP[n].decay1), 0, 31, (flags)}, \
    {"d2r", GS(ins.OP[n].decay2), 0, 31, (flags)}, \
    {"d1l", GS(ins.OP[n].sustain), 0, 15, (flags)}, \
    {"rr", GS(ins.OP[n].release), 0, 15, (flags)}, \
    {"tl", GS(ins.OP[n].level), 0, 127, (flags)}, \
    {"rs", GS(ins.OP[n].ratescale), 0, 3, (flags)}, \
    {"mul", GS(ins.OP[n].fmult), 0, 15, (flags)}, \
    {"dt", GS(ins.OP[n].detune), 0, 7, (flags)}, \
    {"am", GS(ins.OP[n].am_enable), 0, 1, (flags)}, \
    {"ssg", GS(ins.OP[n].ssg_eg), 0, 15, (flags)}
    OP(OPERATOR1_HR, MP_Operator1),
    OP(OPERATOR2_HR, MP_Operator2),
    OP(OPERATOR3_HR, MP_Operator3),
    OP(OPERATOR4_HR, MP_Operator4),
#undef OP
    {"note", GS(ins.note_offset1), -128, 127, MP_None},
    {"vel", GS(ins.velocity_offset), -128, 127, MP_None},
    {"pk", GS(ins.percNoteNum), 0, 127, MP_None},
    {"kon", GS(ins.ms_sound_kon), 0, 65535, MP_Measure},
    {"koff", GS(ins.ms_sound_koff), 0, 65535, MP_Measure},

#undef G
#undef S
#undef GS
};

#endif // METAPARAMETER_H
