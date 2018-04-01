#include "mame_opn2.h"
#include "mame/mame_ym2612fm.h"
#include <cstdlib>
#include <assert.h>

#include <QtDebug>

MameOPN2::MameOPN2() :
    OPNChipBase()
{
    chip = ym2612_init(NULL, (int)m_clock, (int)m_rate, NULL, NULL);
}

MameOPN2::MameOPN2(const MameOPN2 &c) :
    OPNChipBase(c)
{
    chip = ym2612_init(NULL, (int)m_clock, (int)m_rate, NULL, NULL);
}

MameOPN2::~MameOPN2()
{
    ym2612_shutdown(chip);
}

void MameOPN2::setRate(uint32_t rate, uint32_t clock)
{
    OPNChipBase::setRate(rate, clock);
    ym2612_shutdown(chip);
    chip = ym2612_init(NULL, (int)clock, (int)rate, NULL, NULL);
    ym2612_reset_chip(chip);
}

void MameOPN2::reset()
{
    ym2612_reset_chip(chip);
}

void MameOPN2::reset(uint32_t rate, uint32_t clock)
{
    setRate(rate, clock);
}

void MameOPN2::writeReg(uint32_t port, uint16_t addr, uint8_t data)
{
    ym2612_write(chip, 0 + (int)(port) * 2, (uint8_t)addr);
    ym2612_write(chip, 1 + (int)(port) * 2, data);
}

inline int16_t Limit2Short(INT32 Value)
{
    INT32 NewValue;

    NewValue = Value;
    if (NewValue < -0x8000)
        NewValue = -0x8000;
    if (NewValue > 0x7FFF)
        NewValue = 0x7FFF;

    return (int16_t)NewValue;
}

int MameOPN2::generate(int16_t *output, size_t frames)
{
    size_t i = 0;
    //YM2612GenerateStream(NULL, output, (uint32_t)frames);
    stream_sample_t *buffer[2];

    for(i = 0; i < 2; i++)
        buffer[i] = (stream_sample_t*)malloc(sizeof(stream_sample_t) * frames);

    ym2612_update_one(chip, buffer, (int)frames);
    for(i = 0; i < frames; i++)
    {
        *output++ = Limit2Short(buffer[0][i]);
        *output++ = Limit2Short(buffer[1][i]);
    }

    for(i = 0; i < 2; i++)
        free(buffer[i]);
    return (int)frames;
}
