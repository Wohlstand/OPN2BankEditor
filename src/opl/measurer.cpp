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

#ifndef M_PI
#define M_PI    3.14159265358979323846
#endif

//Measurer is always needs for emulator
#include "chips/nuked_opn2.h"
#include "chips/mame_opn2.h"
#include "chips/gens_opn2.h"
#include "chips/gx_opn2.h"

//typedef NukedOPN2 DefaultOPN2;
typedef MameOPN2 DefaultOPN2;

typedef Measurer::DurationInfo DurationInfo;

template <class T>
class AudioHistory
{
    std::unique_ptr<T[]> m_data;
    size_t m_index = 0;  // points to the next write slot
    size_t m_length = 0;
    size_t m_capacity = 0;

public:
    size_t size() const { return m_length; }
    size_t capacity() const { return m_capacity; }
    const T *data() const { return &m_data[m_index + m_capacity - m_length]; }

    void reset(size_t capacity)
    {
        m_data.reset(new T[2 * capacity]());
        m_index = 0;
        m_length = 0;
        m_capacity = capacity;
    }

    void clear()
    {
        m_length = 0;
    }

    void add(const T &item)
    {
        T *data = m_data.get();
        const size_t capacity = m_capacity;
        size_t index = m_index;
        data[index] = item;
        data[index + capacity] = item;
        m_index = (index + 1 != capacity) ? (index + 1) : 0;
        size_t length = m_length + 1;
        m_length = (length < capacity) ? length : capacity;
    }
};

static void HannWindow(double *w, unsigned n)
{
    for (unsigned i = 0; i < n; ++i)
        w[i] = 0.5 * (1.0 - std::cos(2 * M_PI * i / (n - 1)));
}

static double MeasureRMS(const double *signal, const double *window, unsigned length)
{
    double mean = 0;
#pragma omp simd reduction(+: mean)
    for(unsigned i = 0; i < length; ++i)
        mean += window[i] * signal[i];
    mean /= length;

    double rms = 0;
#pragma omp simd reduction(+: rms)
    for(unsigned i = 0; i < length; ++i)
    {
        double diff = window[i] * signal[i] - mean;
        rms += diff * diff;
    }
    rms = std::sqrt(rms / (length - 1));

    return rms;
}

#ifdef DEBUG_WRITE_AMPLITUDE_PLOT
static bool WriteAmplitudePlot(
    const std::string &fileprefix,
    const std::vector<double> &amps_on,
    const std::vector<double> &amps_off,
    double timestep)
{
    std::string datafile = fileprefix + ".dat";
    std::string gpfile_on_off[2] =
        { fileprefix + "-on.gp",
          fileprefix + "-off.gp" };
    const char *plot_title[2] =
        { "Key-On Amplitude", "Key-Off Amplitude" };

#if !defined(_WIN32)
    size_t datafile_base = datafile.rfind("/");
#else
    size_t datafile_base = datafile.find_last_of("/\\");
#endif
    datafile_base = (datafile_base == datafile.npos) ? 0 : (datafile_base + 1);

    size_t n_on = amps_on.size();
    size_t n_off = amps_off.size();
    size_t n = (n_on > n_off) ? n_on : n_off;

    std::ofstream outs;

    outs.open(datafile);
    if(outs.bad())
        return false;
    for(size_t i = 0; i < n; ++i)
    {
        const double nan = std::numeric_limits<double>::quiet_NaN();
        double values[2] =
            { (i < n_on) ? amps_on[i] : nan,
              (i < n_off) ? amps_off[i] : nan };
        outs << i * timestep;
        for(unsigned j = 0; j < 2; ++j)
        {
            if(!std::isnan(values[j]))
                outs << ' ' << values[j];
            else
                outs << " m";
        }
        outs << '\n';
    }
    outs.flush();
    if(outs.bad())
        return false;
    outs.close();

    for(unsigned i = 0; i < 2; ++i)
    {
        outs.open(gpfile_on_off[i]);
        if(outs.bad())
            return false;
        outs << "set datafile missing \"m\"\n";
        outs << "plot \"" << datafile.substr(datafile_base) <<  "\""
            " u 1:" << 2 + i << " w linespoints pt 4"
            " t \"" << plot_title[i] << "\"\n";
        outs.flush();
        if(outs.bad())
            return false;
        outs.close();
    }

    return true;
}
#endif

static const unsigned g_outputRate = 53267;

struct TinySynth
{
    //! Context of the chip emulator
    OPNChipBase *m_chip;
    //! Count of playing notes
    unsigned m_notesNum;
    //! MIDI note to play
    int m_notenum;
    //! Centy detune
    int8_t  m_fineTune;
    //! Half-tone offset
    int16_t m_noteOffsets[2];

    //! Absolute channel
    uint32_t    m_c;
    //! Port of OPN2 chip
    uint8_t     m_port;
    //! Relative channel
    uint8_t     m_cc;

    void resetChip()
    {
        m_chip->setRate(g_outputRate, 7670454);

        m_chip->writeReg(0, 0x22, 0x00);   //LFO off
        m_chip->writeReg(0, 0x27, 0x0 );   //Channel 3 mode normal

        //Shut up all channels
        m_chip->writeReg(0, 0x28, 0x00 );   //Note Off 0 channel
        m_chip->writeReg(0, 0x28, 0x01 );   //Note Off 1 channel
        m_chip->writeReg(0, 0x28, 0x02 );   //Note Off 2 channel
        m_chip->writeReg(0, 0x28, 0x04 );   //Note Off 3 channel
        m_chip->writeReg(0, 0x28, 0x05 );   //Note Off 4 channel
        m_chip->writeReg(0, 0x28, 0x06 );   //Note Off 5 channel

        //Disable DAC
        m_chip->writeReg(0, 0x2B, 0x0 );   //DAC off
    }

    void setInstrument(const FmBank::Instrument *in_p)
    {
        const FmBank::Instrument &in = *in_p;
        OPN_PatchSetup patch;

        m_notenum = in.percNoteNum >= 128 ? (in.percNoteNum - 128) : in.percNoteNum;
        if(m_notenum == 0)
            m_notenum = 25;
        m_notesNum = 1;
        m_fineTune = 0;
        m_noteOffsets[0] = in.note_offset1;
        //m_noteOffsets[1] = in.note_offset2;

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
        patch.lfosens  = 0;//Disable LFO sensitivity for clear measure
        patch.finetune = static_cast<int8_t>(in.note_offset1);
        patch.tone     = 0;

        m_c = 0;
        m_port = (m_c <= 2) ? 0 : 1;
        m_cc   = m_c % 3;

        for(uint8_t op = 0; op < 4; op++)
        {
            m_chip->writeReg(m_port, 0x30 + (op * 4) + m_cc, patch.OPS[op].data[0]);
            m_chip->writeReg(m_port, 0x40 + (op * 4) + m_cc, patch.OPS[op].data[1]);
            m_chip->writeReg(m_port, 0x50 + (op * 4) + m_cc, patch.OPS[op].data[2]);
            m_chip->writeReg(m_port, 0x60 + (op * 4) + m_cc, patch.OPS[op].data[3]);
            m_chip->writeReg(m_port, 0x70 + (op * 4) + m_cc, patch.OPS[op].data[4]);
            m_chip->writeReg(m_port, 0x80 + (op * 4) + m_cc, patch.OPS[op].data[5]);
            m_chip->writeReg(m_port, 0x90 + (op * 4) + m_cc, patch.OPS[op].data[6]);
        }
        m_chip->writeReg(m_port, 0xB0 + m_cc, patch.fbalg);
        m_chip->writeReg(m_port, 0xB4 + m_cc, 0xC0);
    }

    void noteOn()
    {
        double hertz = 321.88557 * std::exp(0.057762265 * (m_notenum + m_noteOffsets[0]));
        uint16_t x2 = 0x0000;
        if(hertz < 0 || hertz > 262143)
        {
            std::fprintf(stderr, "MEASURER WARNING: Why does note %d + note-offset %d produce hertz %g?          \n",
                         m_notenum, m_noteOffsets[0], hertz);
            hertz = 262143;
        }

        while(hertz >= 2047.5)
        {
            hertz /= 2.0;    // Calculate octave
            x2 += 0x800;
        }
        x2 += static_cast<uint32_t>(hertz + 0.5);

        // Keyon the note
        m_chip->writeReg(m_port, 0xA4 + m_cc, (x2>>8) & 0xFF);//Set frequency and octave
        m_chip->writeReg(m_port, 0xA0 + m_cc,  x2 & 0xFF);

        m_chip->writeReg(0, 0x28, 0xF0 + uint8_t((m_c <= 2) ? m_c : m_c + 1));
    }

    void noteOff()
    {
        // Keyoff the note
        uint8_t cc = static_cast<uint8_t>(m_c % 6);
        m_chip->writeReg(0, 0x28, (m_c <= 2) ? cc : cc + 1);
    }

    void generate(int16_t *output, size_t frames)
    {
        m_chip->generate(output, frames);
    }
};

static void BenchmarkChip(FmBank::Instrument *in_p, OPNChipBase *chip)
{
    TinySynth synth;
    synth.m_chip = chip;
    synth.resetChip();
    synth.setInstrument(in_p);

    const unsigned interval             = 150;
    const unsigned samples_per_interval = g_outputRate / interval;
    const unsigned max_on  = 10;
    const unsigned max_off = 20;

    unsigned max_period_on = max_on * interval;
    unsigned max_period_off = max_off * interval;

    const size_t audioBufferLength = 256;
    const size_t audioBufferSize = 2 * audioBufferLength;
    int16_t audioBuffer[audioBufferSize];

    synth.noteOn();
    for(unsigned period = 0; period < max_period_on; ++period)
    {
        for(unsigned i = 0; i < samples_per_interval;)
        {
            size_t blocksize = samples_per_interval - i;
            blocksize = (blocksize < audioBufferLength) ? blocksize : audioBufferLength;
            synth.generate(audioBuffer, blocksize);
            i += blocksize;
        }
    }

    synth.noteOff();
    for(unsigned period = 0; period < max_period_off; ++period)
    {
        for(unsigned i = 0; i < samples_per_interval;)
        {
            size_t blocksize = samples_per_interval - i;
            blocksize = (blocksize < 256) ? blocksize : 256;
            synth.generate(audioBuffer, blocksize);
            i += blocksize;
        }
    }
}

static void ComputeDurations(const FmBank::Instrument *in_p, DurationInfo *result_p, OPNChipBase *chip)
{
    const FmBank::Instrument &in = *in_p;
    DurationInfo &result = *result_p;

    AudioHistory<double> audioHistory;

    const unsigned interval             = 150;
    const unsigned samples_per_interval = g_outputRate / interval;

    const double historyLength = 0.1;  // maximum duration to memorize (seconds)
    audioHistory.reset(std::ceil(historyLength * g_outputRate));

#if defined(ENABLE_PLOTS) || defined(DEBUG_WRITE_AMPLITUDE_PLOT)
    const double timestep = (double)samples_per_interval / g_outputRate;  // interval between analysis steps (seconds)
#endif
#if defined(ENABLE_PLOTS)
    result.amps_timestep = timestep;
#endif

    std::unique_ptr<double[]> window;
    window.reset(new double[audioHistory.capacity()]);
    unsigned winsize = 0;

    TinySynth synth;
    synth.m_chip = chip;
    synth.resetChip();
    synth.setInstrument(&in);
    synth.noteOn();

    /* For capturing */
    const unsigned max_silent = 6;
    const unsigned max_on  = 40;
    const unsigned max_off = 60;

    unsigned max_period_on = max_on * interval;
    unsigned max_period_off = max_off * interval;

    const double min_coefficient_on = 0.008;
    const double min_coefficient_off = 0.1;

    unsigned windows_passed_on = 0;
    unsigned windows_passed_off = 0;

    /* For Analyze the results */
    double begin_amplitude        = 0;
    double peak_amplitude_value   = 0;
    size_t peak_amplitude_time    = 0;
    size_t quarter_amplitude_time = max_period_on;
    bool   quarter_amplitude_time_found = false;
    size_t keyoff_out_time        = 0;
    bool   keyoff_out_time_found  = false;

    const size_t audioBufferLength = 256;
    const size_t audioBufferSize = 2 * audioBufferLength;
    int16_t audioBuffer[audioBufferSize];

    // For up to 40 seconds, measure mean amplitude.
    double highest_sofar = 0;
    short sound_min = 0, sound_max = 0;

#if defined(ENABLE_PLOTS)
    std::vector<double> &amplitudecurve_on = result.amps_on;
    amplitudecurve_on.clear();
    amplitudecurve_on.reserve(max_period_on);
#elif defined(DEBUG_AMPLITUDE_PEAK_VALIDATION) || defined(DEBUG_WRITE_AMPLITUDE_PLOT)
    std::vector<double> amplitudecurve_on;
    amplitudecurve_on.reserve(max_period_on);
#endif
    for(unsigned period = 0; period < max_period_on; ++period, ++windows_passed_on)
    {
        for(unsigned i = 0; i < samples_per_interval;)
        {
            size_t blocksize = samples_per_interval - i;
            blocksize = (blocksize < audioBufferLength) ? blocksize : audioBufferLength;
            synth.generate(audioBuffer, blocksize);
            for (unsigned j = 0; j < blocksize; ++j)
            {
                int16_t s = audioBuffer[2 * j];
                audioHistory.add(s);
                if(sound_min > s) sound_min = s;
                if(sound_max < s) sound_max = s;
            }
            i += blocksize;
        }

        if(winsize != audioHistory.size())
        {
            winsize = audioHistory.size();
            HannWindow(window.get(), winsize);
        }

        double rms = MeasureRMS(audioHistory.data(), window.get(), winsize);
        /* ======== Peak time detection ======== */
        if(period == 0)
        {
            begin_amplitude = rms;
            peak_amplitude_value = rms;
            peak_amplitude_time = 0;
        }
        else if(rms > peak_amplitude_value)
        {
            peak_amplitude_value = rms;
            peak_amplitude_time  = period;
            // In next step, update the quater amplitude time
            quarter_amplitude_time_found = false;
        }
        else if(!quarter_amplitude_time_found && (rms <= peak_amplitude_value * min_coefficient_on))
        {
            quarter_amplitude_time = period;
            quarter_amplitude_time_found = true;
        }
        /* ======== Peak time detection =END==== */
#if defined(ENABLE_PLOTS) || defined(DEBUG_AMPLITUDE_PEAK_VALIDATION) || defined(DEBUG_WRITE_AMPLITUDE_PLOT)
        amplitudecurve_on.push_back(rms);
#endif
        if(rms > highest_sofar)
            highest_sofar = rms;

        if((period > max_silent * interval) &&
           ( (rms < highest_sofar * min_coefficient_on) || (sound_min >= -1 && sound_max <= 1) )
        )
            break;
    }

    if(!quarter_amplitude_time_found)
        quarter_amplitude_time = windows_passed_on;

#ifdef DEBUG_AMPLITUDE_PEAK_VALIDATION
    char outBufOld[250];
    char outBufNew[250];
    std::memset(outBufOld, 0, 250);
    std::memset(outBufNew, 0, 250);

    std::snprintf(outBufOld, 250, "Peak: beg=%g, peakv=%g, peakp=%zu, q=%zu",
                begin_amplitude,
                peak_amplitude_value,
                peak_amplitude_time,
                quarter_amplitude_time);

    /* Detect the peak time */
    begin_amplitude        = amplitudecurve_on[0];
    peak_amplitude_value   = begin_amplitude;
    peak_amplitude_time    = 0;
    quarter_amplitude_time = amplitudecurve_on.size();
    keyoff_out_time        = 0;
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

    std::snprintf(outBufNew, 250, "Peak: beg=%g, peakv=%g, peakp=%zu, q=%zu",
                begin_amplitude,
                peak_amplitude_value,
                peak_amplitude_time,
                quarter_amplitude_time);

    if(memcmp(outBufNew, outBufOld, 250) != 0)
    {
        qDebug() << "Pre: " << outBufOld << "\n" <<
                    "Pos: " << outBufNew;
    }
#endif

    if(windows_passed_on >= max_period_on)
    {
        // Just Keyoff the note
        synth.noteOff();
    }
    else
    {
        // Reset the emulator and re-run the "ON" simulation until reaching the peak time
        synth.resetChip();
        synth.setInstrument(&in);
        synth.noteOn();

        audioHistory.reset(std::ceil(historyLength * g_outputRate));
        for(unsigned period = 0;
            ((period < peak_amplitude_time) || (period == 0)) && (period < max_period_on);
            ++period)
        {
            for(unsigned i = 0; i < samples_per_interval;)
            {
                size_t blocksize = samples_per_interval - i;
                blocksize = (blocksize < audioBufferLength) ? blocksize : audioBufferLength;
                synth.generate(audioBuffer, blocksize);
                for (unsigned j = 0; j < blocksize; ++j)
                    audioHistory.add(audioBuffer[2 * j]);
                i += blocksize;
            }
        }
        synth.noteOff();
    }

    // Now, for up to 60 seconds, measure mean amplitude.
#if defined(ENABLE_PLOTS)
    std::vector<double> &amplitudecurve_off = result.amps_off;
    amplitudecurve_off.clear();
    amplitudecurve_off.reserve(max_period_on);
#elif defined(DEBUG_AMPLITUDE_PEAK_VALIDATION) || defined(DEBUG_WRITE_AMPLITUDE_PLOT)
    std::vector<double> amplitudecurve_off;
    amplitudecurve_off.reserve(max_period_off);
#endif
    for(unsigned period = 0; period < max_period_off; ++period, ++windows_passed_off)
    {
        for(unsigned i = 0; i < samples_per_interval;)
        {
            size_t blocksize = samples_per_interval - i;
            blocksize = (blocksize < 256) ? blocksize : 256;
            synth.generate(audioBuffer, blocksize);
            for (unsigned j = 0; j < blocksize; ++j)
            {
                int16_t s = audioBuffer[2 * j];
                audioHistory.add(s);
                if(sound_min > s) sound_min = s;
                if(sound_max < s) sound_max = s;
            }
            i += blocksize;
        }

        if(winsize != audioHistory.size())
        {
            winsize = audioHistory.size();
            HannWindow(window.get(), winsize);
        }

        double rms = MeasureRMS(audioHistory.data(), window.get(), winsize);
        /* ======== Find Key Off time ======== */
        if(!keyoff_out_time_found && (rms <= peak_amplitude_value * min_coefficient_off))
        {
            keyoff_out_time = period;
            keyoff_out_time_found = true;
        }
        /* ======== Find Key Off time ==END=== */
#if defined(ENABLE_PLOTS) || defined(DEBUG_AMPLITUDE_PEAK_VALIDATION) || defined(DEBUG_WRITE_AMPLITUDE_PLOT)
        amplitudecurve_off.push_back(rms);
#endif
        if(rms < highest_sofar * min_coefficient_off)
            break;

        if((period > max_silent * interval) && (sound_min >= -1 && sound_max <= 1))
            break;
    }

#ifdef DEBUG_WRITE_AMPLITUDE_PLOT
    WriteAmplitudePlot(
        "/tmp/amplitude", amplitudecurve_on, amplitudecurve_off, timestep);
#endif

#ifdef DEBUG_AMPLITUDE_PEAK_VALIDATION
    size_t debug_peak_old = keyoff_out_time;

    /* Analyze the final results */
    for(size_t a = 0; a < amplitudecurve_off.size(); ++a)
    {
        if(amplitudecurve_off[a] <= peak_amplitude_value * min_coefficient_off)
        {
            keyoff_out_time = a;
            break;
        }
    }

    if(debug_peak_old != keyoff_out_time)
    {
        qDebug() << "KeyOff time is 1:" << debug_peak_old << " and 2:" << keyoff_out_time;
    }
#endif

    result.peak_amplitude_time = peak_amplitude_time;
    result.peak_amplitude_value = peak_amplitude_value;
    result.begin_amplitude = begin_amplitude;
    result.quarter_amplitude_time = (double)quarter_amplitude_time;
    result.keyoff_out_time = (double)keyoff_out_time;

    result.ms_sound_kon  = (int64_t)(quarter_amplitude_time * 1000.0 / interval);
    result.ms_sound_koff = (int64_t)(keyoff_out_time        * 1000.0 / interval);
    result.nosound = (peak_amplitude_value < 0.5) || ((sound_min >= -1) && (sound_max <= 1));
}

static void ComputeDurationsDefault(const FmBank::Instrument *in, DurationInfo *result)
{
    DefaultOPN2 chip;
    ComputeDurations(in, result, &chip);
}

static void MeasureDurations(FmBank::Instrument *in_p, OPNChipBase *chip)
{
    FmBank::Instrument &in = *in_p;
    DurationInfo result;
    ComputeDurations(&in, &result, chip);
    in.ms_sound_kon = (uint16_t)result.ms_sound_kon;
    in.ms_sound_koff = (uint16_t)result.ms_sound_koff;
    in.is_blank = result.nosound;
}

static void MeasureDurationsDefault(FmBank::Instrument *in_p)
{
    DefaultOPN2 chip;
    MeasureDurations(in_p, &chip);
}

static void MeasureDurationsBenchmark(FmBank::Instrument *in_p, OPNChipBase *chip, QVector<Measurer::BenchmarkResult> *result)
{
    std::chrono::steady_clock::time_point start, stop;
    Measurer::BenchmarkResult res;
    start = std::chrono::steady_clock::now();
    BenchmarkChip(in_p, chip);
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
        std::shared_ptr<OPNChipBase>(new GXOPN2),
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
    watcher.connect(&watcher, SIGNAL(finished()), &m_progressBox, SLOT(accept()));

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
    watcher.connect(&watcher, SIGNAL(finished()), &m_progressBox, SLOT(accept()));

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

bool Measurer::doComputation(const FmBank::Instrument &instrument, DurationInfo &result)
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
    watcher.connect(&watcher, SIGNAL(finished()), &m_progressBox, SLOT(accept()));

    watcher.setFuture(QtConcurrent::run(&ComputeDurationsDefault, &instrument, &result));
    m_progressBox.exec();
    watcher.waitForFinished();

    return !watcher.isCanceled();

#else
    m_progressBox.show();
    ComputeDurationsDefault(&instrument, &result);
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
