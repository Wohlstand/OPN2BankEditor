/*
 * OPN2 Bank Editor by Wohlstand, a free tool for music bank editing
 * Copyright (c) 2017 Vitaly Novichkov <admin@wohlnet.ru>
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

#ifndef BANK_H
#define BANK_H

#include <QVector>

/* *********** FM Operator indexes *********** */
#define OPERATOR1    0
#define OPERATOR2    1
#define OPERATOR3    2
#define OPERATOR4    3
/* *********** FM Operator indexes *end******* */

/**
 * @brief OPL instrument bank container
 */
class FmBank
{
public:
    FmBank();
    FmBank(const FmBank &fb);
    FmBank &operator=(const FmBank &fb);
    bool operator==(const FmBank &fb);
    bool operator!=(const FmBank &fb);

    /**
     * @brief Set everything to zero
     */
    void reset();

    /**
     * @brief Operator specs
     */
    struct Operator
    {
        /*
         * --- About chip global properties bytes ---
         *  LFO
         *     4-bits unused
         *     1-bit LFO on/off
         *     3-bits LFO frequency (0 to 7)
         *
         *  Timer A
         *      8-bit MSB
         *      6-bits unused
         *      2-bit LSB
         *
         *          They should be set in the order 24H, 25H
         *          10-bit: 18 × (1024 - Timer A) microseconds
         *          Timer A = all 1′s -> 18 µs = 0.018 ms
         *          Timer A = all 0′s -> 18,400 µs = 18.4 ms
         *
         * Timer B
         *      8-bit   288 × (256 - Timer B ) microseconds
         *
         *          Timer B = all 1′s -> 0.288 ms
         *          Timer B = all 0′s -> 73.44 ms
         *
         * Timers; Ch 3/6 mode
         *      1-bit must be always 0
         *      1-bit 0 - Channel 3 is same as others, 1 - channel #3 has 4 separated frequencies
         *      1-bit Reset B
         *      1-bit Reset A
         *      1-bit Enable A
         *      1-bit Enable B
         *      1-bit Load B
         *      1-bit Load A
         *
         * --- About operator data bytes ---
         *
         * DAC sample (must be 8-bit PCM)
         *      8-bit DAC amplitude value of each sample
         *
         * Detune / Multiple bits
         *      1-bit unused
         *      3-bits detune level
         *      4-bits Frequency multiplication
         *
         * Total Level (Attenuation)
         *      1-bit unused
         *      7-bits Level (0 max, 127 minimal)
         *
         * Rate Scale/Attack
         *      2-bits Rate Scale
         *      1-bit  unused
         *      5-bits Attack (0 max, 31 minimal)
         *
         * Decay1 (Decay before Systain) / Amplitude Modulation
         *      1-bit  Amplitude Modulation
         *      2-bits unused
         *      5-bits Decay1 (0 max, 31 minimal)
         *
         * Decay2 (Decay while Systain)
         *      3-bits unused
         *      5-bits Decay2 (0 max, 31 minimal)
         *
         * Systain / Release
         *      4-bits Systain
         *      4-bits Release
         *
         * Proprietary / SSG-EG
         *      4-bits Proprietary
         *      4-bits SSG-EG
         *
         * Feedback / Algorithm
         *      2-bits unused
         *      3-bits Feedback
         *      3-bits Algorithm
         *
         * Stereo (unused in the banks) / LFO Sensitivity (Maybe let modulate it by "Modulation" MIDI controller?)
         *      1-bit Left speaker  (Don't save into file, must be controlled by MIDI panarame)
         *      1-bit Right speaker (Don't save into file, must be controlled by MIDI panarame)
         *      2-bits Amplitude Modulation sensitivity (0...3)
         *      1-bit unused
         *      3-bits Frequency Modulation sensitibity (Maybe handle by modulation MIDI controller) (0...7)
         */

        //! Detune level (from 0 to 7)
        unsigned char detune;
        //! Frequency multiplication (from 0 to 15)
        unsigned char fmult;
        //! Volume level (from 0 to 127)
        unsigned char level;
        //! Key Scale level (from 0 to 3)
        unsigned char ratescale;
        //! Attacking level (from 0 to 31)
        unsigned char attack;
        //! Enable/Disable EFO affecting
        bool          am_enable;
        //! Decaying 1 level (from 0 to 31)
        unsigned char decay1;
        //! Decaying 2 level (from 0 to 31)
        unsigned char decay2;
        //! Sustain level (from 0 to 15)
        unsigned char sustain;
        //! SSG-EG value (from 0 to 15)
        unsigned char ssg_eg;
        //! Release level (from 0 to 15)
        unsigned char release;
    };

    /**
     * @brief Instrument specs
     */
    struct Instrument
    {
        //! Custom instrument name
        char name[33];
        //! FM operators
        Operator OP[4];
        //! Feedback
        uint8_t feedback;
        //! Algorithm
        uint8_t algorithm;
        //! Play only this note number independent from a requested key
        uint8_t percNoteNum;
        //! Note offset (first operator pair)
        int16_t note_offset1;

        //! Frequency modulation sensitivity (0 or 7)
        uint8_t fm;
        //! Amplitude modulation sensitivity (0...3)
        uint8_t am;

        bool operator==(const Instrument &fb);
        bool operator!=(const Instrument &fb);

        /* ******** OPN2 merged values ******** */
        uint8_t getRegDUMUL(int OpID);
        void    setRegDUMUL(int OpID, uint8_t reg_dumul);

        uint8_t getRegLevel(int OpID);
        void    setRegLevel(int OpID, uint8_t reg_level);

        uint8_t getRegRSAt(int OpID);
        void    setRegRSAt(int OpID, uint8_t reg_rsat);

        uint8_t getRegAMD1(int OpID);
        void    setRegAMD1(int OpID, uint8_t reg_amd1);

        uint8_t getRegD2(int OpID);
        void    setRegD2(int OpID, uint8_t reg_d2);

        uint8_t getRegSysRel(int OpID);
        void    setRegSysRel(int OpID, uint8_t reg_sysrel);

        uint8_t getRegSsgEg(int OpID);
        void    setRegSsgEg(int OpID, uint8_t reg_ssgeg);

        uint8_t getRegFbAlg();
        void    setRegFbAlg(uint8_t reg_ssgeg);

        uint8_t getRegLfoSens();
        void    setRegLfoSens(uint8_t reg_lfosens);
    };

    bool            lfo_enabled     = false;
    unsigned char   lfo_frequency   = 0;

    //Global chip LFO parameter
    unsigned char getRegLFO();
    void setRegLFO(unsigned char lfo_reg);

    /**
     * @brief Get empty instrument entry
     * @return null-filled instrument entry
     */
    static Instrument emptyInst();

    inline int countMelodic()   { return Ins_Melodic_box.size(); }

    inline int countDrums()     { return Ins_Percussion_box.size(); }

    //! Pointer to array of melodic instruments
    Instrument* Ins_Melodic;
    //! Pointer to array of percussion instruments
    Instrument* Ins_Percussion;
    //! Array of melodic instruments
    QVector<Instrument> Ins_Melodic_box;
    //! Array of percussion instruments
    QVector<Instrument> Ins_Percussion_box;
};

#endif // BANK_H
