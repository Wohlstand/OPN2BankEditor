/*
 * OPN2 Bank Editor by Wohlstand, a free tool for music bank editing
 * Copyright (c) 2018-2020 Vitaly Novichkov <admin@wohlnet.ru>
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

#include "format_s98_import.h"
#include "ym2612_to_wopi.h"
#include "../common.h"
#include <memory>

bool S98_Importer::detect(const QString & /*filePath*/, char* magic)
{
    return !memcmp(magic, "S98", 3);
}

enum S98Device
{
    NONE    = 0,
    YM2149  = 1,
    YM2203  = 2,
    YM2612  = 3,
    YM2608  = 4,
    YM2151  = 5,
    YM2413  = 6,
    YM3526  = 7,
    YM3812  = 8,
    YMF262  = 9,
    AY8910  = 15,
    SN76489 = 16,
};

template <class T>
bool readUIntLE(QIODevice &in, T *p)
{
    uint8_t buf[sizeof(T)];
    if(in.read((char *)buf, sizeof(T)) != sizeof(T))
        return false;
    T x = 0;
    for(size_t i = 0; i < sizeof(T); ++i)
        x = (x << 8) | buf[sizeof(T) - 1 - i];
    *p = x;
    return true;
}

FfmtErrCode S98_Importer::loadFile(QString filePath, FmBank &bank)
{
    QFile file(filePath);
    if(!file.open(QIODevice::ReadOnly))
        return FfmtErrCode::ERR_NOFILE;

    char magic[4];
    if(file.read(magic, 4) != 4 || memcmp(magic, "S98", 3) != 0)
        return FfmtErrCode::ERR_BADFORMAT;

    int version = magic[3] - '0';
    if(version != 1 && version != 3)
        return FfmtErrCode::ERR_BADFORMAT;

    char temp[4];

    file.read(temp, 4);  // timer info 1
    file.read(temp, 4);  // timer info 2

    uint32_t compressing;
    if (!readUIntLE(file, &compressing) || compressing != 0)
        return FfmtErrCode::ERR_BADFORMAT;

    uint32_t songNameOff;
    uint32_t dumpDataOff;
    uint32_t loopPointOff;
    if (!readUIntLE(file, &songNameOff) || !readUIntLE(file, &dumpDataOff) ||
        !readUIntLE(file, &loopPointOff))
        return FfmtErrCode::ERR_BADFORMAT;

    QVector<uint32_t> devices;

    if(version == 1)
    {
        devices.push_back(S98Device::YM2608);
    }
    else if(version == 3)
    {
        uint32_t deviceCount;
        if (!readUIntLE(file, &deviceCount))
            return FfmtErrCode::ERR_BADFORMAT;
        if (deviceCount == 0)
            devices.push_back(S98Device::YM2608);
        else
        {
            for(uint32_t i = 0; i < deviceCount; ++i)
            {
                uint32_t type, clock, pan;
                if (!readUIntLE(file, &type) || !readUIntLE(file, &clock) ||
                    !readUIntLE(file, &pan) || file.read(temp, 4) != 4)
                    return FfmtErrCode::ERR_BADFORMAT;
                devices.push_back(type);
            }
        }
    }

    std::string songName;
    if(songNameOff != 0)
    {
        // TODO: parse it
    }

    struct DeviceMapping {
        uint32_t devno;
        bool extend;
    };

    std::vector<DeviceMapping> dmap;
    for(uint32_t d = 0, n = devices.size(); d < n; ++d)
    {
        switch(devices[d])
        {
        case YM2612: case YM2608: case YMF262: {
            DeviceMapping m;
            m.devno = d;
            m.extend = false;
            dmap.push_back(m);
            m.extend = true;
            dmap.push_back(m);
            break;
        }
        default:
            DeviceMapping m;
            m.devno = d;
            m.extend = false;
            dmap.push_back(m);
            break;
        }
    }

    std::vector<std::unique_ptr<RawYm2612ToWopi>> opnChips(devices.size());
    for(uint32_t d = 0, n = devices.size(); d < n; ++d)
    {
        uint32_t type = devices[d];
        if(type == S98Device::YM2612 || type == S98Device::YM2608)
            opnChips[d].reset(new RawYm2612ToWopi);
    }

    file.seek(dumpDataOff);

    for(;;) {
        uint8_t mapno;
        if(file.read((char *)&mapno, 1) != 1)
            return FfmtErrCode::ERR_BADFORMAT;

        if(mapno == 0xfd) // end
            break;
        if(mapno == 0xff || mapno == 0xfe) // 1-sync/n-sync
        {
            for(uint32_t d = 0, n = devices.size(); d < n; ++d)
                if(RawYm2612ToWopi *c = opnChips[d].get())
                    c->doAnalyzeState();
            if(mapno == 0xfe)
            {
                // skip vlq
                uint8_t b = 0x80;
                while(b & 0x80)
                    if(file.read((char *)&b, 1) != 1)
                        return FfmtErrCode::ERR_BADFORMAT;
            }
            continue;
        }
        if(mapno >= 0x80) // reserved
            return FfmtErrCode::ERR_BADFORMAT;

        if(mapno >= dmap.size())
            return FfmtErrCode::ERR_BADFORMAT;

        uint8_t data[2];
        if(file.read((char *)data, 2) != 2)
            return FfmtErrCode::ERR_BADFORMAT;

        const DeviceMapping &m = dmap[mapno];
        RawYm2612ToWopi *chip = opnChips[m.devno].get();
        if(!chip)
            continue;

        chip->passReg(m.extend ? 1 : 0, data[0], data[1]);
    }

    std::vector<FmBank::Instrument> insts;

    for(uint32_t d = 0, n = devices.size(); d < n; ++d)
    {
        RawYm2612ToWopi *chip = opnChips[d].get();
        if(!chip)
            continue;
        const QList<FmBank::Instrument> &instsChip = chip->caughtInstruments();
        insts.insert(insts.end(), instsChip.begin(), instsChip.end());
    }

    bank.reset((insts.size() + 127) / 128, 1);
    for(FmBank::Instrument &ins : bank.Ins_Melodic_box)
        ins.is_blank = true;
    for(FmBank::Instrument &ins : bank.Ins_Percussion_box)
        ins.is_blank = true;
    for(size_t i = 0, n = insts.size(); i < n; ++i)
        bank.Ins_Melodic[i] = insts[i];

    return FfmtErrCode::ERR_OK;
}

int S98_Importer::formatCaps() const
{
    return (int)FormatCaps::FORMAT_CAPS_IMPORT;
}

QString S98_Importer::formatName() const
{
    return "Sound 98";
}

QString S98_Importer::formatModuleName() const
{
    return "Sound 98 importer";
}

QString S98_Importer::formatExtensionMask() const
{
    return "*.s98";
}

BankFormats S98_Importer::formatId() const
{
    return BankFormats::FORMAT_S98_IMPORTER;
}
