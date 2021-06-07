/*
 * OPN2 Bank Editor by Wohlstand, a free tool for music bank editing
 * Copyright (c) 2017-2021 Vitaly Novichkov <admin@wohlnet.ru>
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
//Human readable. The reason of de-facto swapped second and third operators
#define OPERATOR1_HR    0
#define OPERATOR2_HR    2
#define OPERATOR3_HR    1
#define OPERATOR4_HR    3
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
     * @brief Set everything to zero (one bank per melodic and percussion)
     */
    void reset();

    /**
     * @brief Set everything to zero and set count of banks
     * @param melodic_banks Count of melodic banks
     * @param percussion_banks Count of percussion banks
     */
    void reset(uint16_t melodic_banks, uint16_t percussion_banks);

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
        uint8_t detune;
        //! Frequency multiplication (from 0 to 15)
        uint8_t fmult;
        //! Volume level (from 0 to 127)
        uint8_t level;
        //! Key Scale level (from 0 to 3)
        uint8_t ratescale;
        //! Attacking level (from 0 to 31)
        uint8_t attack;
        //! Enable/Disable EFO affecting
        bool    am_enable;
        //! Decaying 1 level (from 0 to 31)
        uint8_t decay1;
        //! Decaying 2 level (from 0 to 31)
        uint8_t decay2;
        //! Sustain level (from 0 to 15)
        uint8_t sustain;
        //! SSG-EG value (from 0 to 15)
        uint8_t ssg_eg;
        //! Release level (from 0 to 15)
        uint8_t release;
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
        //! Note velocity offset
        int8_t velocity_offset;
        //! Number of milliseconds of produced sound while sustaining
        uint16_t ms_sound_kon;
        //! Number of milliseconds of produced sound while release
        uint16_t ms_sound_koff;
        //! Is instrument blank
        bool     is_blank;

        //! Frequency modulation sensitivity (0 or 7)
        uint8_t fm;
        //! Amplitude modulation sensitivity (0...3)
        uint8_t am;

        bool operator==(const Instrument &fb);
        bool operator!=(const Instrument &fb);

        /* ******** OPN2 merged values ******** */
        uint8_t getRegDUMUL(int OpID) const;
        void    setRegDUMUL(int OpID, uint8_t reg_dumul);

        uint8_t getRegLevel(int OpID) const;
        void    setRegLevel(int OpID, uint8_t reg_level);

        uint8_t getRegRSAt(int OpID) const;
        void    setRegRSAt(int OpID, uint8_t reg_rsat);

        uint8_t getRegAMD1(int OpID) const;
        void    setRegAMD1(int OpID, uint8_t reg_amd1);

        uint8_t getRegD2(int OpID) const;
        void    setRegD2(int OpID, uint8_t reg_d2);

        uint8_t getRegSysRel(int OpID) const;
        void    setRegSysRel(int OpID, uint8_t reg_sysrel);

        uint8_t getRegSsgEg(int OpID) const;
        void    setRegSsgEg(int OpID, uint8_t reg_ssgeg);

        uint8_t getRegFbAlg() const;
        void    setRegFbAlg(uint8_t reg_fbalg);

        uint8_t getRegLfoSens() const;
        void    setRegLfoSens(uint8_t reg_lfosens);
    };

    struct MidiBank
    {
        //! Custom bank name
        char name[33];
        //! MIDI bank MSB index
        uint8_t msb;
        //! MIDI bank LSB index
        uint8_t lsb;
    };

    bool    lfo_enabled     = false;
    uint8_t lfo_frequency   = 0;
    bool    opna_mode       = false;

    //Global chip LFO parameter
    uint8_t getRegLFO() const;
    void setRegLFO(uint8_t lfo_reg);

    uint8_t getBankFlags() const;
    void setBankFlags(uint8_t bank_flags);

    /**
     * @brief Get empty instrument entry
     * @return null-filled instrument entry
     */
    static Instrument emptyInst();

    /**
     * @brief Get blank instrument entry
     * @return blank instrument entry
     */
    static Instrument blankInst();

    /**
     * @brief Get empty bank meta-data entry
     * @return null-filled bank entry
     */
    static MidiBank emptyBank(uint16_t index = 0);

    inline int countMelodic() const { return Ins_Melodic_box.size(); }
    inline int countDrums() const   { return Ins_Percussion_box.size(); }

    /**
     * @brief Get the identified bank
     * @brief msb MIDI bank MSB index
     * @brief lsb MIDI bank LSB index
     * @brief percussive true iff it's a drum bank
     * @brief pBank unless null, receives a pointer to the MIDI bank instance
     * @brief pIns unless null, receives a pointer to the first instrument
     * @return true if the bank exists, false if it doesn't
     */
    bool getBank(uint8_t msb, uint8_t lsb, bool percussive,
                 MidiBank **pBank, Instrument **pIns);

    /**
     * @brief Get the identified bank, creating if necessary
     * @brief msb MIDI bank MSB index
     * @brief lsb MIDI bank LSB index
     * @brief percussive true iff it's a drum bank
     * @brief pBank unless null, receives a pointer to the MIDI bank instance
     * @brief pIns unless null, receives a pointer to the first instrument
     * @return true if the bank is created, false if it already exists
     */
    bool createBank(uint8_t msb, uint8_t lsb, bool percussive,
                    MidiBank **pBank, Instrument **pIns);

    //! Pointer to array of melodic instruments
    Instrument* Ins_Melodic;
    //! Pointer to array of percussion instruments
    Instrument* Ins_Percussion;
    //! Array of melodic instruments
    QVector<Instrument> Ins_Melodic_box;
    //! Array of percussion instruments
    QVector<Instrument> Ins_Percussion_box;
    //! Array of melodic MIDI bank meta-data per every index
    QVector<MidiBank>   Banks_Melodic;
    //! Array of percussion MIDI bank meta-data per every index
    QVector<MidiBank>   Banks_Percussion;
};

class TmpBank
{
public:
    TmpBank(FmBank &bank, int minMelodic, int minPercusive);

    //! Pointer to array of melodic instruments
    FmBank::Instrument* insMelodic;
    //! Pointer to array of percussion instruments
    FmBank::Instrument* insPercussion;
    //! Array of melodic instruments
    QVector<FmBank::Instrument> tmpMelodic;
    //! Array of percussion instruments
    QVector<FmBank::Instrument> tmpPercussion;
};

#endif // BANK_H
