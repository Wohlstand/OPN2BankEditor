/*
 * OPL Bank Editor by Wohlstand, a free tool for music bank editing
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

#ifndef IS_QT_4
#include <QtConcurrent/QtConcurrent>
#include <QFuture>
#endif
#include <QQueue>
#include <QProgressDialog>

#include <vector>
#include <chrono>
#include <cmath>
#include <memory>

#include "measurer.h"
#include "generator.h"

//Measurer is always needs for emulator
#include "chips/opn_chip_base.h"
#include "chips/nuked_opn2.h"
#include "chips/mame_opn2.h"
#include "chips/gens_opn2.h"

struct DurationInfo
{
    uint64_t    peak_amplitude_time;
    double      peak_amplitude_value;
    double      quarter_amplitude_time;
    double      begin_amplitude;
    double      interval;
    double      keyoff_out_time;
    int64_t     ms_sound_kon;
    int64_t     ms_sound_koff;
    bool        nosound;
    uint8_t     padding[7];
};

struct ChipEmulator
{
    OPNChipBase *opl;
    void setRate(uint32_t rate)
    {
        opl->setRate(rate, 7670454.0);
    }
    void WRITE_REG(uint8_t port, uint8_t address, uint8_t byte)
    {
        opl->writeReg(port, address, byte);
    }
};

static void MeasureDurations(FmBank::Instrument *in_p, OPNChipBase *chip)
{
    FmBank::Instrument &in = *in_p;
    std::vector<int16_t> stereoSampleBuf;

    const unsigned rate = 44100;
    const unsigned interval             = 150;
    const unsigned samples_per_interval = rate / interval;
    const int notenum = in.percNoteNum >= 128 ? (128 - in.percNoteNum) : in.percNoteNum;
    ChipEmulator opn;
    opn.opl = chip;

    opn.setRate(rate);
    opn.WRITE_REG(0, 0x22, 0x00);   //LFO off
    opn.WRITE_REG(0, 0x27, 0x0 );   //Channel 3 mode normal

    //Shut up all channels
    opn.WRITE_REG(0, 0x28, 0x00 );   //Note Off 0 channel
    opn.WRITE_REG(0, 0x28, 0x01 );   //Note Off 1 channel
    opn.WRITE_REG(0, 0x28, 0x02 );   //Note Off 2 channel
    opn.WRITE_REG(0, 0x28, 0x04 );   //Note Off 3 channel
    opn.WRITE_REG(0, 0x28, 0x05 );   //Note Off 4 channel
    opn.WRITE_REG(0, 0x28, 0x06 );   //Note Off 5 channel

    //Disable DAC
    opn.WRITE_REG(0, 0x2B, 0x0 );   //DAC off

    OPN_PatchSetup patch;

    for(int op = 0; op < 4; op++)
    {
        patch.OPS[op].data[0] = in.getRegDUMUL(op);
        patch.OPS[op].data[1] = in.getRegLevel(op);
        patch.OPS[op].data[2] = in.getRegRSAt(op);
        patch.OPS[op].data[3] = in.getRegAMD1(op);
        patch.OPS[op].data[4] = in.getRegD2(op);
        patch.OPS[op].data[5] = in.getRegSysRel(op);
        patch.OPS[op].data[6] = in.getRegSsgEg(op);
    }
    patch.fbalg    = in.getRegFbAlg();
    patch.lfosens  = in.getRegLfoSens();
    patch.finetune = static_cast<int8_t>(in.note_offset1);
    patch.tone     = 0;

    uint32_t c = 0;
    uint8_t port = (c <= 2) ? 0 : 1;
    uint8_t cc   = c % 3;

    for(uint8_t op = 0; op < 4; op++)
    {
        opn.WRITE_REG(port, 0x30 + (op * 4) + cc, patch.OPS[op].data[0]);
        opn.WRITE_REG(port, 0x40 + (op * 4) + cc, patch.OPS[op].data[1]);
        opn.WRITE_REG(port, 0x50 + (op * 4) + cc, patch.OPS[op].data[2]);
        opn.WRITE_REG(port, 0x60 + (op * 4) + cc, patch.OPS[op].data[3]);
        opn.WRITE_REG(port, 0x70 + (op * 4) + cc, patch.OPS[op].data[4]);
        opn.WRITE_REG(port, 0x80 + (op * 4) + cc, patch.OPS[op].data[5]);
        opn.WRITE_REG(port, 0x90 + (op * 4) + cc, patch.OPS[op].data[6]);
    }
    opn.WRITE_REG(port, 0xB0 + cc, patch.fbalg);
    opn.WRITE_REG(port, 0xB4 + cc,  0xC0);


    {
        double hertz = 321.88557 * std::exp(0.057762265 * (notenum + in.note_offset1));
        uint16_t x2 = 0x0000;
        if(hertz < 0 || hertz > 262143)
        {
            std::fprintf(stderr, "MEASURER WARNING: Why does note %d + finetune %d produce hertz %g?          \n",
                         notenum, in.note_offset1, hertz);
            hertz = 262143;
        }

        while(hertz >= 2047.5)
        {
            hertz /= 2.0;    // Calculate octave
            x2 += 0x800;
        }

        x2 += static_cast<uint32_t>(hertz + 0.5);

        // Keyon the note
        opn.WRITE_REG(port, 0xA4 + cc, (x2>>8) & 0xFF);//Set frequency and octave
        opn.WRITE_REG(port, 0xA0 + cc,  x2 & 0xFF);

        opn.WRITE_REG(0, 0x28, 0xF0 + uint8_t((c <= 2) ? c : c + 1));
    }

    const unsigned max_silent = 6;
    const unsigned max_on  = 40;
    const unsigned max_off = 60;

    const double min_coefficient_on = 0.2;
    const double min_coefficient_off = 0.1;

    // For up to 40 seconds, measure mean amplitude.
    std::vector<double> amplitudecurve_on;
    double highest_sofar = 0;
    short sound_min = 0, sound_max = 0;
    for(unsigned period = 0; period < max_on * interval; ++period)
    {
        stereoSampleBuf.clear();
        stereoSampleBuf.resize(samples_per_interval * 2, 0);

        opn.opl->generate(stereoSampleBuf.data(), samples_per_interval);

        double mean = 0.0;
        for(unsigned long c = 0; c < samples_per_interval; ++c)
        {
            short s = stereoSampleBuf[c * 2];
            mean += s;
            if(sound_min > s) sound_min = s;
            if(sound_max < s) sound_max = s;
        }
        mean /= samples_per_interval;
        double std_deviation = 0;
        for(unsigned long c = 0; c < samples_per_interval; ++c)
        {
            double diff = (stereoSampleBuf[c * 2] - mean);
            std_deviation += diff * diff;
        }
        std_deviation = std::sqrt(std_deviation / samples_per_interval);
        amplitudecurve_on.push_back(std_deviation);
        if(std_deviation > highest_sofar)
            highest_sofar = std_deviation;

        if((period > max_silent * interval) &&
            ((std_deviation < highest_sofar * min_coefficient_on) ||
             (sound_min >= -1 && sound_max <= 1))
        )
            break;
    }

    // Keyoff the note
    {
        uint8_t cc = static_cast<uint8_t>(c % 6);
        opn.WRITE_REG(0, 0x28, (c <= 2) ? cc : cc + 1);
    }

    // Now, for up to 60 seconds, measure mean amplitude.
    std::vector<double> amplitudecurve_off;
    for(unsigned period = 0; period < max_off * interval; ++period)
    {
        stereoSampleBuf.clear();
        stereoSampleBuf.resize(samples_per_interval * 2);

        opn.opl->generate(stereoSampleBuf.data(), samples_per_interval);

        double mean = 0.0;
        for(unsigned long c = 0; c < samples_per_interval; ++c)
        {
            short s = stereoSampleBuf[c * 2];
            mean += s;
            if(sound_min > s) sound_min = s;
            if(sound_max < s) sound_max = s;
        }
        mean /= samples_per_interval;
        double std_deviation = 0;
        for(unsigned long c = 0; c < samples_per_interval; ++c)
        {
            double diff = (stereoSampleBuf[c * 2] - mean);
            std_deviation += diff * diff;
        }
        std_deviation = std::sqrt(std_deviation / samples_per_interval);
        amplitudecurve_off.push_back(std_deviation);

        if(std_deviation < highest_sofar * min_coefficient_off)
            break;

        if((period > max_silent * interval) && (sound_min >= -1 && sound_max <= 1))
            break;
    }

    /* Analyze the results */
    double begin_amplitude        = amplitudecurve_on[0];
    double peak_amplitude_value   = begin_amplitude;
    size_t peak_amplitude_time    = 0;
    size_t quarter_amplitude_time = amplitudecurve_on.size();
    size_t keyoff_out_time        = 0;

    for(size_t a = 1; a < amplitudecurve_on.size(); ++a)
    {
        if(amplitudecurve_on[a] > peak_amplitude_value)
        {
            peak_amplitude_value = amplitudecurve_on[a];
            peak_amplitude_time  = a;
        }
    }
    for(size_t a = peak_amplitude_time; a < amplitudecurve_on.size(); ++a)
    {
        if(amplitudecurve_on[a] <= peak_amplitude_value * min_coefficient_on)
        {
            quarter_amplitude_time = a;
            break;
        }
    }
    for(size_t a = 0; a < amplitudecurve_off.size(); ++a)
    {
        if(amplitudecurve_off[a] <= peak_amplitude_value * min_coefficient_off)
        {
            keyoff_out_time = a;
            break;
        }
    }

    if(keyoff_out_time == 0 && amplitudecurve_on.back() < peak_amplitude_value * min_coefficient_off)
        keyoff_out_time = quarter_amplitude_time;

    DurationInfo result;
    result.peak_amplitude_time = peak_amplitude_time;
    result.peak_amplitude_value = peak_amplitude_value;
    result.begin_amplitude = begin_amplitude;
    result.quarter_amplitude_time = (double)quarter_amplitude_time;
    result.keyoff_out_time = (double)keyoff_out_time;

    result.ms_sound_kon  = (int64_t)(quarter_amplitude_time * 1000.0 / interval);
    result.ms_sound_koff = (int64_t)(keyoff_out_time        * 1000.0 / interval);
    result.nosound = (peak_amplitude_value < 0.5) || ((sound_min >= -1) && (sound_max <= 1));

    in.ms_sound_kon = (uint16_t)result.ms_sound_kon;
    in.ms_sound_koff = (uint16_t)result.ms_sound_koff;
    in.is_blank = result.nosound;
}

static void MeasureDurationsDefault(FmBank::Instrument *in_p)
{
    MameOPN2 chip;
    MeasureDurations(in_p, &chip);
}

static void MeasureDurationsBenchmark(FmBank::Instrument *in_p, OPNChipBase *chip, QVector<Measurer::BenchmarkResult> *result)
{
    std::chrono::steady_clock::time_point start, stop;
    Measurer::BenchmarkResult res;
    start = std::chrono::steady_clock::now();
    MeasureDurations(in_p, chip);
    stop  = std::chrono::steady_clock::now();
    res.elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count();
    res.name = QString::fromUtf8(chip->emulatorName());
    result->push_back(res);
}

static void MeasureDurationsBenchmarkRunner(FmBank::Instrument *in_p, QVector<Measurer::BenchmarkResult> *result)
{
    std::vector<std::shared_ptr<OPNChipBase > > emuls =
    {
        std::shared_ptr<OPNChipBase>(new NukedOPN2),
        std::shared_ptr<OPNChipBase>(new MameOPN2),
        std::shared_ptr<OPNChipBase>(new GensOPN2)
    };
    for(std::shared_ptr<OPNChipBase> &p : emuls)
        MeasureDurationsBenchmark(in_p, p.get(), result);
}

Measurer::Measurer(QWidget *parent) :
    QObject(parent),
    m_parentWindow(parent)
{}

Measurer::~Measurer()
{}

static void insertOrBlank(FmBank::Instrument &ins, const FmBank::Instrument &blank, QQueue<FmBank::Instrument *> &tasks)
{
    ins.is_blank = false;
    if(memcmp(&ins, &blank, sizeof(FmBank::Instrument)) != 0)
        tasks.enqueue(&ins);
    else
    {
        ins.is_blank = true;
        ins.ms_sound_kon = 0;
        ins.ms_sound_koff = 0;
    }
}

bool Measurer::doMeasurement(FmBank &bank, FmBank &bankBackup, bool forceReset)
{
    QQueue<FmBank::Instrument *> tasks;
    FmBank::Instrument blank = FmBank::emptyInst();

    int i = 0;
    for(i = 0; i < bank.Ins_Melodic_box.size() && i < bankBackup.Ins_Melodic_box.size(); i++)
    {
        FmBank::Instrument &ins1 = bank.Ins_Melodic_box[i];
        FmBank::Instrument &ins2 = bankBackup.Ins_Melodic_box[i];
        if(forceReset || (ins1.ms_sound_kon == 0) || (memcmp(&ins1, &ins2, sizeof(FmBank::Instrument)) != 0))
            insertOrBlank(ins1, blank, tasks);
    }
    for(; i < bank.Ins_Melodic_box.size(); i++)
        insertOrBlank(bank.Ins_Melodic_box[i], blank, tasks);

    for(i = 0; i < bank.Ins_Percussion_box.size() && i < bankBackup.Ins_Percussion_box.size(); i++)
    {
        FmBank::Instrument &ins1 = bank.Ins_Percussion_box[i];
        FmBank::Instrument &ins2 = bankBackup.Ins_Percussion_box[i];
        if(forceReset || (ins1.ms_sound_kon == 0) || (memcmp(&ins1, &ins2, sizeof(FmBank::Instrument)) != 0))
            insertOrBlank(ins1, blank, tasks);
    }
    for(; i < bank.Ins_Percussion_box.size(); i++)
        insertOrBlank(bank.Ins_Percussion_box[i], blank, tasks);

    if(tasks.isEmpty())
        return true;// Nothing to do! :)

    QProgressDialog m_progressBox(m_parentWindow);
    m_progressBox.setWindowModality(Qt::WindowModal);
    m_progressBox.setWindowTitle(tr("Sounding delay calculation"));
    m_progressBox.setLabelText(tr("Please wait..."));

    #ifndef IS_QT_4
    QFutureWatcher<void> watcher;
    watcher.connect(&m_progressBox, SIGNAL(canceled()), &watcher, SLOT(cancel()));
    watcher.connect(&watcher, SIGNAL(progressRangeChanged(int,int)), &m_progressBox, SLOT(setRange(int,int)));
    watcher.connect(&watcher, SIGNAL(progressValueChanged(int)), &m_progressBox, SLOT(setValue(int)));

    watcher.setFuture(QtConcurrent::map(tasks, &MeasureDurationsDefault));

    m_progressBox.exec();
    watcher.waitForFinished();

    tasks.clear();

    // Apply all calculated values into backup store to don't re-calculate same stuff
    for(i = 0; i < bank.Ins_Melodic_box.size() && i < bankBackup.Ins_Melodic_box.size(); i++)
    {
        FmBank::Instrument &ins1 = bank.Ins_Melodic_box[i];
        FmBank::Instrument &ins2 = bankBackup.Ins_Melodic_box[i];
        ins2.ms_sound_kon  = ins1.ms_sound_kon;
        ins2.ms_sound_koff = ins1.ms_sound_koff;
        ins2.is_blank = ins1.is_blank;
    }
    for(i = 0; i < bank.Ins_Percussion_box.size() && i < bankBackup.Ins_Percussion_box.size(); i++)
    {
        FmBank::Instrument &ins1 = bank.Ins_Percussion_box[i];
        FmBank::Instrument &ins2 = bankBackup.Ins_Percussion_box[i];
        ins2.ms_sound_kon  = ins1.ms_sound_kon;
        ins2.ms_sound_koff = ins1.ms_sound_koff;
        ins2.is_blank = ins1.is_blank;
    }

    return !watcher.isCanceled();

    #else
    m_progressBox.setMaximum(tasks.size());
    m_progressBox.setValue(0);
    int count = 0;
    foreach(FmBank::Instrument *ins, tasks)
    {
        MeasureDurationsDefault(ins);
        m_progressBox.setValue(++count);
        if(m_progressBox.wasCanceled())
            return false;
    }
    return true;
    #endif
}

bool Measurer::doMeasurement(FmBank::Instrument &instrument)
{
    QProgressDialog m_progressBox(m_parentWindow);
    m_progressBox.setWindowModality(Qt::WindowModal);
    m_progressBox.setWindowTitle(tr("Sounding delay calculation"));
    m_progressBox.setLabelText(tr("Please wait..."));

#ifndef IS_QT_4
    QFutureWatcher<void> watcher;
    watcher.connect(&m_progressBox, SIGNAL(canceled()), &watcher, SLOT(cancel()));
    watcher.connect(&watcher, SIGNAL(progressRangeChanged(int,int)), &m_progressBox, SLOT(setRange(int,int)));
    watcher.connect(&watcher, SIGNAL(progressValueChanged(int)), &m_progressBox, SLOT(setValue(int)));

    watcher.setFuture(QtConcurrent::run(&MeasureDurationsDefault, &instrument));

    m_progressBox.exec();

    watcher.waitForFinished();

    return !watcher.isCanceled();

#else
    m_progressBox.show();
    MeasureDurationsDefault(&instrument);
    return true;
#endif
}

bool Measurer::runBenchmark(FmBank::Instrument &instrument, QVector<BenchmarkResult> &result)
{
    QProgressDialog m_progressBox(m_parentWindow);
    m_progressBox.setWindowModality(Qt::WindowModal);
    m_progressBox.setWindowTitle(tr("Benchmarking emulators"));
    m_progressBox.setLabelText(tr("Please wait..."));
    m_progressBox.setCancelButton(nullptr);


#ifndef IS_QT_4
    QFutureWatcher<void> watcher;
    watcher.connect(&m_progressBox, SIGNAL(canceled()), &watcher, SLOT(cancel()));
    watcher.connect(&watcher, SIGNAL(progressRangeChanged(int,int)), &m_progressBox, SLOT(setRange(int,int)));
    watcher.connect(&watcher, SIGNAL(progressValueChanged(int)), &m_progressBox, SLOT(setValue(int)));
    watcher.connect(&watcher, SIGNAL(finished()), &m_progressBox, SLOT(accept()));

    watcher.setFuture(QtConcurrent::run(MeasureDurationsBenchmarkRunner, &instrument, &result));
    m_progressBox.exec();
    watcher.waitForFinished();
#else
    m_progressBox.show();
    MeasureDurationsBenchmarkRunner(&instrument, &result);
#endif

    return true;
}
