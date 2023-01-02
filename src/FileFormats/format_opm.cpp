/*
 * OPN2 Bank Editor by Wohlstand, a free tool for music bank editing
 * Copyright (c) 2017-2022 Vitaly Novichkov <admin@wohlnet.ru>
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

#include "format_opm.h"
#include "../common.h"
#include "../version.h"

#include <QRegExp>
#include <QTextStream>

bool OPM::detect(const QString &filePath, char* magic)
{
    (void)magic;

    if(filePath.endsWith(".opm", Qt::CaseInsensitive))
        return true;

    return false;
}

FfmtErrCode OPM::loadFile(QString filePath, FmBank &bank)
{
    QFile file(filePath);

    if(!file.open(QFile::ReadOnly|QFile::Text))
        return FfmtErrCode::ERR_NOFILE;

    typedef QMap<unsigned, FmBank::Instrument> InstMap;

    FmBank::Instrument *curInst = nullptr;
    InstMap mapInsts;

    while(!file.atEnd()) {
        QString line = QString::fromUtf8(file.readLine());

        unsigned commentPos = line.indexOf("//");
        if((int)commentPos != -1)
            line.truncate(commentPos);

        line = line.trimmed();
        if(line.isEmpty())
            continue;

        if(line.startsWith("@:")) //header of a new instrument
        {
            line = line.mid(2).trimmed();
            QTextStream ts(&line);

            unsigned index = 0;
            ts >> index;
            ts.skipWhiteSpace();
            std::string name = ts.readLine().toStdString();

            if(ts.status() != QTextStream::Ok)
                return FfmtErrCode::ERR_BADFORMAT;

            curInst = &mapInsts[index];
            *curInst = FmBank::emptyInst();

            size_t namelen = name.size();
            namelen = (namelen < 32) ? namelen : 32;
            memcpy(curInst->name, name.data(), namelen);

            continue;
        }

        if(!curInst)
            return FfmtErrCode::ERR_BADFORMAT;

        if(line.startsWith("LFO:"))
        {
            // ignored
            continue;
        }

        if(line.startsWith("CH:"))
        {
            line = line.mid(3).trimmed();
            QTextStream ts(&line);

            unsigned pan, fl, con, ams, pms, slot, ne;
            ts >> pan >> fl >> con >> ams >> pms >> slot >> ne;

            if(ts.status() != QTextStream::Ok)
                return FfmtErrCode::ERR_BADFORMAT;

            curInst->feedback = fl;
            curInst->algorithm = con;
            continue;
        }

        // operator
        unsigned op =
            line.startsWith("M1:") ? 0 :
            line.startsWith("C1:") ? 1 :
            line.startsWith("M2:") ? 2 :
            line.startsWith("C2:") ? 3 : (unsigned)-1;

        if(op == (unsigned)-1)
            return FfmtErrCode::ERR_BADFORMAT;

        line = line.mid(3).trimmed();
        QTextStream ts(&line);

        unsigned ar, d1r, d2, rr, d1l, tl, ks, mul, dt1, dt2, ams_en;
        ts >> ar >> d1r >> d2 >> rr >> d1l >> tl >> ks >> mul >> dt1 >> dt2 >> ams_en;

        if(ts.status() != QTextStream::Ok)
            return FfmtErrCode::ERR_BADFORMAT;

        FmBank::Operator &o = curInst->OP[(op == 1) ? 2 : (op == 2) ? 1 : op];
        o.attack = ar;
        o.decay1 = d1r;
        o.decay2 = d2;
        o.release = rr;
        o.sustain = d1l;
        o.level = tl;
        o.ratescale = ks;
        o.fmult = mul;
        o.detune = dt1;
        o.am_enable = ams_en;
    }

    if(!mapInsts.empty())
    {
        unsigned instCount = mapInsts.lastKey() + 1;
        unsigned bankCount = (instCount + 127) / 128;

        if(bankCount >= 128 * 128)
            return FfmtErrCode::ERR_BADFORMAT;

        bank.reset(bankCount, 1);
        for(unsigned i = 0; i < bankCount; ++i)
        {
            bank.Banks_Melodic[i].msb = i / 128;
            bank.Banks_Melodic[i].lsb = i % 128;
        }

        for(unsigned i = 0; i < 128 * bankCount; ++i)
            bank.Ins_Melodic[i].is_blank = true;
        for(unsigned i = 0; i < 128; ++i)
            bank.Ins_Percussion[i].is_blank = true;

        for(InstMap::iterator it = mapInsts.begin(); it != mapInsts.end(); ++it)
        {
            unsigned index = it.key();
            bank.Ins_Melodic[index] = *it;
        }
    }

    return FfmtErrCode::ERR_OK;
}

FfmtErrCode OPM::saveFile(QString filePath, FmBank &bank)
{
    QFile file(filePath);

    if(!file.open(QFile::WriteOnly))
        return FfmtErrCode::ERR_NOFILE;

    QTextStream ts(&file);

    ts << "//OPM file writer - " PROGRAM_NAME " " VERSION "\r\n"
       << "//Free software program by " COMPANY " <" PGE_URL ">\r\n";

    unsigned melodics = bank.countMelodic();
    unsigned drums = bank.countDrums();

    unsigned opmIndex = 0;
    for(unsigned i = 0; i < melodics + drums; ++i)
    {
        const FmBank::Instrument &inst = (i < melodics) ?
            bank.Ins_Melodic[i] : bank.Ins_Percussion[i - melodics];
        if(inst.is_blank)
            continue;

        ts << "\r\n@:" << opmIndex << ' ' << inst.name << "\r\n"
            "LFO: 0 0 0 0 0\r\n"
            "CH: 64 " << inst.feedback << ' ' << inst.algorithm << " 0 0 120 0\r\n";

        for(unsigned op = 0; op < 4; ++op)
        {
            const FmBank::Operator &o = inst.OP[(op == 1) ? 2 : (op == 2) ? 1 : op];
            const char *names[4] = {"M1", "C1", "M2", "C2"};
            ts << names[op] << ": "
               << o.attack << ' ' << o.decay1 << ' ' << o.decay2 << ' '
               << o.release << ' ' << o.sustain << ' ' << o.level << ' '
               << o.ratescale << ' ' << o.fmult << ' ' << o.detune << " 0 "
               << (int)o.am_enable << "\r\n";
        }

        ++opmIndex;
    }

    return FfmtErrCode::ERR_OK;
}

int OPM::formatCaps() const
{
    return (int)FormatCaps::FORMAT_CAPS_EVERYTHING;
}

QString OPM::formatName() const
{
    return "OPM bank";
}

QString OPM::formatExtensionMask() const
{
    return "*.opm";
}

QString OPM::formatDefaultExtension() const
{
    return "opm";
}

BankFormats OPM::formatId() const
{
    return FORMAT_OPM;
}
