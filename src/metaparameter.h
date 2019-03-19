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

#ifndef METAPARAMETER_H
#define METAPARAMETER_H

#include "bank.h"

struct MetaParameter
{
    typedef int (Get)(const FmBank::Instrument &);

    MetaParameter() {}
    MetaParameter(const char *name, Get *get, int min, int max, unsigned flags)
        : name(name), get(get), min(min), max(max), flags(flags) {}

    const char *const name = nullptr;
    Get *const get = nullptr;
    int min = 0;
    int max = 0;
    const unsigned flags = 0;
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

    {"alg", G(ins.algorithm), 0, 7, MP_None},
    {"fb", G(ins.feedback), 0, 7, MP_None},
    {"ams", G(ins.am), 0, 3, MP_None},
    {"fms", G(ins.fm), 0, 7, MP_None},
#define OP(n, flags)                                                           \
    {"ar", G(ins.OP[n].attack), 0, 31, (flags)}, \
    {"d1r", G(ins.OP[n].decay1), 0, 31, (flags)}, \
    {"d2r", G(ins.OP[n].decay2), 0, 31, (flags)}, \
    {"d1l", G(ins.OP[n].sustain), 0, 15, (flags)}, \
    {"rr", G(ins.OP[n].release), 0, 15, (flags)}, \
    {"tl", G(ins.OP[n].level), 0, 127, (flags)}, \
    {"rs", G(ins.OP[n].ratescale), 0, 3, (flags)}, \
    {"mul", G(ins.OP[n].fmult), 0, 15, (flags)}, \
    {"dt", G(ins.OP[n].detune), 0, 7, (flags)}, \
    {"am", G(ins.OP[n].am_enable), 0, 1, (flags)}, \
    {"ssg", G(ins.OP[n].ssg_eg), 0, 15, (flags)}
    OP(OPERATOR1_HR, MP_Operator1),
    OP(OPERATOR2_HR, MP_Operator2),
    OP(OPERATOR3_HR, MP_Operator3),
    OP(OPERATOR4_HR, MP_Operator4),
#undef OP
    {"note", G(ins.note_offset1), -128, 127, MP_None},
    {"vel", G(ins.velocity_offset), -128, 127, MP_None},
    {"pk", G(ins.percNoteNum), 0, 127, MP_None},
    {"kon", G(ins.ms_sound_kon), 0, 65535, MP_Measure},
    {"koff", G(ins.ms_sound_koff), 0, 65535, MP_Measure},

#undef G
};

#endif // METAPARAMETER_H
