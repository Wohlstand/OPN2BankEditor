/*
 * OPN2 Bank Editor by Wohlstand, a free tool for music bank editing
 * Copyright (c) 2017-2018 Vitaly Novichkov <admin@wohlnet.ru>
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

#include <memory>
#include <list>

#include "../common.h"

#include "ffmt_factory.h"

#include "format_wohlstand_opn2.h"
#include "format_vgm_import.h"
#include "format_gym_import.h"
#include "format_tfi.h"
#include "format_deflemask_dmp.h"
#include "format_gems_pat.h"
#include "format_m2v_gyb.h"
#include "format_tomsoft_gin.h"

typedef std::unique_ptr<FmBankFormatBase> FmBankFormatBase_uptr;
typedef std::list<FmBankFormatBase_uptr>  FmBankFormatsL;

//! Bank formats
static FmBankFormatsL g_formats;
//! Single-Instrument formats
static FmBankFormatsL g_formatsInstr;

static void registerBankFormat(FmBankFormatBase *format)
{
    g_formats.push_back(FmBankFormatBase_uptr(format));
}

static void registerInstFormat(FmBankFormatBase *format)
{
    g_formatsInstr.push_back(FmBankFormatBase_uptr(format));
}

void FmBankFormatFactory::registerAllFormats()
{
    g_formats.clear();
    g_formatsInstr.clear();
    registerBankFormat(new WohlstandOPN2());
    registerInstFormat(new WohlstandOPN2());
    registerBankFormat(new VGM_Importer());
    registerBankFormat(new GYM_Importer());
    registerInstFormat(new TFI_MM());
    registerInstFormat(new DefleMask());
    registerBankFormat(new GEMS_PAT());
    registerInstFormat(new GEMS_PAT());
    registerBankFormat(new M2V_GYB());
    registerBankFormat(new Tomsoft_GIN());
}



QString FmBankFormatFactory::getSaveFiltersList()
{
    QString formats;
    for(FmBankFormatBase_uptr &p : g_formats)
    {
        Q_ASSERT(p.get());//It must be non-null!
        if(p->formatCaps() & (int)FormatCaps::FORMAT_CAPS_SAVE)
        {
            formats.append(QString("%1 (%2);;").arg(p->formatName()).arg(p->formatExtensionMask()));
        }
    }
    if(formats.endsWith(";;"))
        formats.remove(formats.size()-2, 2);
    return formats;
}

QString FmBankFormatFactory::getOpenFiltersList(bool import)
{
    QString out;
    QString masks;
    QString formats;
    //! Look for importable or openable formats?
    FormatCaps dst = import ?
                FormatCaps::FORMAT_CAPS_IMPORT :
                FormatCaps::FORMAT_CAPS_OPEN;

    for(FmBankFormatBase_uptr &p : g_formats)
    {
        Q_ASSERT(p.get());//It must be non-null!
        if(p->formatCaps() & (int)dst)
        {
            //Don't add duplicated extensions into "supported" list
            if(!masks.contains(p->formatExtensionMask()))
            {
                if(!masks.isEmpty())
                    masks.append(' ');
                masks.append(p->formatExtensionMask());
            }
            formats.append(QString("%1 (%2);;").arg(p->formatName()).arg(p->formatExtensionMask()));
        }
    }
    out.append(QString("Supported bank files (%1);;").arg(masks));
    out.append(formats);
    out.push_back("All files (*.*)");
    return out;
}

QString FmBankFormatFactory::getInstOpenFiltersList(bool import)
{
    QString out;
    QString masks;
    QString formats;
    //! Look for importable or openable formats?
    FormatCaps dst = import ?
                FormatCaps::FORMAT_CAPS_IMPORT :
                FormatCaps::FORMAT_CAPS_OPEN;
    for(FmBankFormatBase_uptr &p : g_formatsInstr)
    {
        Q_ASSERT(p.get());//It must be non-null!
        if(p->formatInstCaps() & (int)dst)
        {
            //Don't add duplicated extensions into "supported" list
            if(!masks.contains(p->formatInstExtensionMask()))
            {
                if(!masks.isEmpty())
                    masks.append(' ');
                masks.append(p->formatInstExtensionMask());
            }
            formats.append(QString("%1 (%2);;").arg(p->formatInstName()).arg(p->formatInstExtensionMask()));
        }
    }
    out.append(QString("Supported instrument files (%1);;").arg(masks));
    out.append(formats);
    out.push_back("All files (*.*)");
    return out;
}

QString FmBankFormatFactory::getInstSaveFiltersList()
{
    QString formats;
    for(FmBankFormatBase_uptr &p : g_formatsInstr)
    {
        Q_ASSERT(p.get());//It must be non-null!
        if(p->formatInstCaps() & (int)FormatCaps::FORMAT_CAPS_SAVE)
        {
            formats.append(QString("%1 (%2);;").arg(p->formatInstName()).arg(p->formatInstExtensionMask()));
        }
    }
    if(formats.endsWith(";;"))
        formats.remove(formats.size()-2, 2);
    return formats;
}

static void fillBankFmtList(QList<const FmBankFormatBase *> &dest, FmBankFormatsL &fmts)
{
    for(FmBankFormatBase_uptr &p : fmts)
        dest.push_back(p.get());
}

QList<const FmBankFormatBase *> FmBankFormatFactory::allBankFormats()
{
    QList<const FmBankFormatBase *> fullList;
    fillBankFmtList(fullList, g_formats);
    return fullList;
}

QList<const FmBankFormatBase *> FmBankFormatFactory::allInstrumentFormats()
{
    QList<const FmBankFormatBase *> fullList;
    fillBankFmtList(fullList, g_formatsInstr);
    return fullList;
}

BankFormats FmBankFormatFactory::getFormatFromFilter(QString filter)
{
    for(FmBankFormatBase_uptr &p : g_formats)
    {
        Q_ASSERT(p.get());//It must be non-null!
        QString f = QString("%1 (%2)").arg(p->formatName()).arg(p->formatExtensionMask());
        if(f == filter)
            return p->formatId();
    }
    return BankFormats::FORMAT_UNKNOWN;
}

QString FmBankFormatFactory::getFilterFromFormat(BankFormats format, int requiredCaps)
{
    for(FmBankFormatBase_uptr &p : g_formats)
    {
        Q_ASSERT(p.get());//It must be non-null!
        if((p->formatId() == format) && (p->formatCaps() & requiredCaps))
            return QString("%1 (%2)").arg(p->formatName()).arg(p->formatExtensionMask());
    }
    return "UNKNOWN";
}

InstFormats FmBankFormatFactory::getInstFormatFromFilter(QString filter)
{
    for(FmBankFormatBase_uptr &p : g_formatsInstr)
    {
        Q_ASSERT(p.get());//It must be non-null!
        QString f = QString("%1 (%2)").arg(p->formatInstName()).arg(p->formatInstExtensionMask());
        if(f == filter)
            return p->formatInstId();
    }
    return InstFormats::FORMAT_INST_UNKNOWN;
}

QString FmBankFormatFactory::getInstFilterFromFormat(InstFormats format, int requiredCaps)
{
    for(FmBankFormatBase_uptr &p : g_formatsInstr)
    {
        Q_ASSERT(p.get());//It must be non-null!
        if((p->formatInstId() == format) && (p->formatInstCaps() & requiredCaps))
            return QString("%1 (%2)").arg(p->formatInstName()).arg(p->formatInstExtensionMask());
    }
    return "UNKNOWN";
}

bool FmBankFormatFactory::isImportOnly(BankFormats format)
{
    for(FmBankFormatBase_uptr &p : g_formats)
    {
        Q_ASSERT(p.get());//It must be non-null!
        if(p->formatId() == format)
            return (p->formatCaps() == (int)FormatCaps::FORMAT_CAPS_IMPORT);
    }
    return false;
}



FfmtErrCode FmBankFormatFactory::OpenBankFile(QString filePath, FmBank &bank, BankFormats *recent)
{
    char magic[32];
    getMagic(filePath, magic, 32);

    FfmtErrCode err = FfmtErrCode::ERR_UNSUPPORTED_FORMAT;
    BankFormats fmt = BankFormats::FORMAT_UNKNOWN;

    for(FmBankFormatBase_uptr &p : g_formats)
    {
        Q_ASSERT(p.get());//It must be non-null!
        if((p->formatCaps() & (int)FormatCaps::FORMAT_CAPS_OPEN) && p->detect(filePath, magic))
        {
            err = p->loadFile(filePath, bank);
            fmt = p->formatId();
            break;
        }
    }
    if(recent)
        *recent = fmt;

    return err;
}

FfmtErrCode FmBankFormatFactory::ImportBankFile(QString filePath, FmBank &bank, BankFormats *recent)
{
    char magic[32];
    getMagic(filePath, magic, 32);

    FfmtErrCode err = FfmtErrCode::ERR_UNSUPPORTED_FORMAT;
    BankFormats fmt = BankFormats::FORMAT_UNKNOWN;

    for(FmBankFormatBase_uptr &p : g_formats)
    {
        Q_ASSERT(p.get());//It must be non-null!
        if((p->formatCaps() & (int)FormatCaps::FORMAT_CAPS_IMPORT) && p->detect(filePath, magic))
        {
            err = p->loadFile(filePath, bank);
            fmt = p->formatId();
            break;
        }
    }

    if(recent)
        *recent = fmt;
    return err;
}

FfmtErrCode FmBankFormatFactory::SaveBankFile(QString filePath, FmBank &bank, BankFormats dest)
{
    FfmtErrCode err = FfmtErrCode::ERR_UNSUPPORTED_FORMAT;
    for(FmBankFormatBase_uptr &p : g_formats)
    {
        Q_ASSERT(p.get());//It must be non-null!
        if((p->formatCaps() & (int)FormatCaps::FORMAT_CAPS_SAVE) && (p->formatId() == dest))
        {
            err = p->saveFile(filePath, bank);
            break;
        }
    }
    return err;
}

FfmtErrCode FmBankFormatFactory::OpenInstrumentFile(QString filePath,
                                         FmBank::Instrument &ins,
                                         InstFormats *recent,
                                         bool *isDrum,
                                         bool import)
{
    char magic[32];
    getMagic(filePath, magic, 32);

    FfmtErrCode err = FfmtErrCode::ERR_UNSUPPORTED_FORMAT;
    InstFormats fmt = InstFormats::FORMAT_INST_UNKNOWN;
    FormatCaps dst = import ?
                FormatCaps::FORMAT_CAPS_IMPORT :
                FormatCaps::FORMAT_CAPS_OPEN;
    for(FmBankFormatBase_uptr &p : g_formatsInstr)
    {
        Q_ASSERT(p.get());//It must be non-null!
        if((p->formatInstCaps() & (int)dst) && p->detectInst(filePath, magic))
        {
            err = p->loadFileInst(filePath, ins, isDrum);
            fmt = p->formatInstId();
            break;
        }
    }
    if(recent)
        *recent = fmt;

    return err;
}

FfmtErrCode FmBankFormatFactory::SaveInstrumentFile(QString filePath, FmBank::Instrument &ins, InstFormats dest, bool isDrum)
{
    FfmtErrCode err = FfmtErrCode::ERR_UNSUPPORTED_FORMAT;
    for(FmBankFormatBase_uptr &p : g_formatsInstr)
    {
        Q_ASSERT(p.get());//It must be non-null!
        if((p->formatInstCaps() & (int)FormatCaps::FORMAT_CAPS_SAVE) && (p->formatInstId() == dest))
        {
            err = p->saveFileInst(filePath, ins, isDrum);
            break;
        }
    }
    return err;
}
