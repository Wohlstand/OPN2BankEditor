// SPDX-License-Identifier: BSD-3-Clause

// Ported to C by Valley Bell within 5 minutes for NeoLogiX
// Note: This file should be included in the .gyb file loader
#include <cstdint>

static uint32_t CalcGYBChecksum(uint32_t FileSize, const uint8_t *FileData)
{
    uint32_t CurPos;
    uint8_t ChkByt1;
    uint8_t ChkByt2;
    uint8_t ChkByt3;
    uint8_t BytMask;
    uint16_t InsCount;
    uint32_t TempLng;
    uint32_t QSum;
    uint8_t ChkArr[0x04];

    // nineko really made a crazy checksum formula here
    ChkByt2 = 0;
    TempLng = FileSize;
    while(TempLng)
    {
        ChkByt2 += (TempLng % 10);
        TempLng /= 10;
    }
    ChkByt2 *= 3;

    ChkByt3 = 0;
    QSum = 0;
    BytMask = static_cast<uint8_t>(1 << (FileSize & 0x07));
    for(CurPos = 0x00; CurPos < FileSize; CurPos++)
    {
        if((FileData[CurPos] & BytMask) == BytMask)
            ChkByt3 ++;
        QSum += FileData[CurPos];
    }
    InsCount = FileData[0x03] + FileData[0x04];    // Melody + Drum Instruments
    ChkByt1 = static_cast<uint8_t>((FileSize + QSum) % (InsCount + 1));
    QSum %= 999;

    ChkArr[0x00] = ChkByt2 + (QSum % 37);
    ChkArr[0x01] = ChkByt1;
    ChkArr[0x02] = ChkByt3;
    // This formula is ... just ... crazy
    // [ (q*x1*x2*x3) + (q*x1*x2) + (q*x2*x3) + (q*x1*x3) + x1+x2+x3+84 ] % 199
    ChkArr[0x03] = ((QSum * (ChkByt1 + 1) * (ChkByt2 + 2) * (ChkByt3 + 3)) +
                    (QSum * (ChkByt1 + 1) * (ChkByt2 + 2)) +
                    (QSum * (ChkByt2 + 2) * (ChkByt3 + 3)) +
                    (QSum * (ChkByt1 + 1) * (ChkByt3 + 3)) +
                    ChkByt1 + ChkByt2 + ChkByt3 + 84) % 199;

    return *((uint32_t *)ChkArr);
}
