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

#include "bank_editor.h"
#include "ui_bank_editor.h"

#include "importer.h"
#include "ui_importer.h"

#include <QtDebug>

void BankEditor::initAudio()
{
    qDebug() << "Init audioOut...";
    m_audioOut = new AudioOutRt(m_audioLatency * 1e-3, this);
    qDebug() << "Init Generator...";
    std::shared_ptr<Generator> generator(
        new Generator(uint32_t(m_audioOut->sampleRate()), m_currentChip));
    qDebug() << "Init Rt-Generator...";
    RealtimeGenerator *rtgenerator = new RealtimeGenerator(generator, this);
    qDebug() << "Seting pointer of RT Generator...";
    m_generator = rtgenerator;

    qDebug() << "Init signals and slots of keys...";
    //Test note
    connect(ui->testNote,  SIGNAL(pressed()),  m_generator,  SLOT(ctl_playNote()));
    connect(ui->testNote,  SIGNAL(released()), m_generator,  SLOT(ctl_noteOffAllChans()));
    //Test major chord
    connect(ui->testMajor, SIGNAL(pressed()),  m_generator, SLOT(ctl_playMajorChord()));
    connect(ui->testMajor, SIGNAL(released()), m_generator, SLOT(ctl_noteOffAllChans()));
    //Test minor chord
    connect(ui->testMinor, SIGNAL(pressed()),  m_generator, SLOT(ctl_playMinorChord()));
    connect(ui->testMinor, SIGNAL(released()), m_generator, SLOT(ctl_noteOffAllChans()));
    //Test augmented chord
    connect(ui->testAugmented, SIGNAL(pressed()),  m_generator, SLOT(ctl_playAugmentedChord()));
    connect(ui->testAugmented, SIGNAL(released()), m_generator, SLOT(ctl_noteOffAllChans()));
    //Test diminished chord
    connect(ui->testDiminished, SIGNAL(pressed()),  m_generator, SLOT(ctl_playDiminishedChord()));
    connect(ui->testDiminished, SIGNAL(released()), m_generator, SLOT(ctl_noteOffAllChans()));
    //Test major 7-chord
    connect(ui->testMajor7, SIGNAL(pressed()),  m_generator, SLOT(ctl_playMajor7Chord()));
    connect(ui->testMajor7, SIGNAL(released()), m_generator, SLOT(ctl_noteOffAllChans()));
    //Test minor 7-chord
    connect(ui->testMinor7, SIGNAL(pressed()),  m_generator, SLOT(ctl_playMinor7Chord()));
    connect(ui->testMinor7, SIGNAL(released()), m_generator, SLOT(ctl_noteOffAllChans()));
    //Shut up that annoying noice!
    connect(ui->shutUp, SIGNAL(clicked()), m_generator, SLOT(ctl_silence()));
    //Note to test
#if QT_VERSION >= 0x050000
    connect(ui->noteToTest, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), m_generator, &IRealtimeControl::changeNote);
#else
    connect(ui->noteToTest, SIGNAL(valueChanged(int)), m_generator, SLOT(changeNote(int)));
#endif
    m_generator->changeNote(ui->noteToTest->value());

    //LFO enable/disable, and LFO frequency
#if QT_VERSION >= 0x050000
    connect(ui->lfoEnable,      &QCheckBox::toggled,
            m_generator,        &IRealtimeControl::ctl_changeLFO);
    connect(ui->lfoFrequency,   static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            m_generator,        &IRealtimeControl::ctl_changeLFOfreq);
#else
    connect(ui->lfoEnable,      SIGNAL(toggled(bool)),
            m_generator,        SLOT(ctl_changeLFO(bool)));
    connect(ui->lfoFrequency,   SIGNAL(currentIndexChanged(int)),
            m_generator,        SLOT(ctl_changeLFOfreq(int)));
#endif

    //Generator's debug info
    connect(m_generator, SIGNAL(debugInfo(QString)), ui->debugBox, SLOT(setText(QString)));
    //Key pressed on piano bar
#if QT_VERSION >= 0x050000
    connect(ui->piano, &Piano::gotNote,     ui->noteToTest, &QSpinBox::setValue);
    connect(ui->piano, &Piano::pressed,     m_generator,    &IRealtimeControl::ctl_playNote);
    connect(ui->piano, &Piano::released,    m_generator,    &IRealtimeControl::ctl_stopNote);
#else
    connect(ui->piano, SIGNAL(gotNote(int)), ui->noteToTest, SLOT(setValue(int)));
    connect(ui->piano, SIGNAL(pressed()),   m_generator,    SLOT(ctl_playNote()));
    connect(ui->piano, SIGNAL(released()),  m_generator,    SLOT(ctl_stopNote()));
#endif
    //Piano on the importer dialog pressed
    m_importer->connect(m_importer->ui->piano, SIGNAL(gotNote(int)), ui->noteToTest, SLOT(setValue(int)));
    m_importer->connect(m_importer->ui->piano, SIGNAL(pressed()),    m_generator, SLOT(ctl_playNote()));
    m_importer->connect(m_importer->ui->piano, SIGNAL(released()),   m_generator, SLOT(ctl_stopNote()));
    //Test note button on the importer dialog box
    m_importer->connect(m_importer->ui->testNote,  SIGNAL(pressed()),  m_generator,  SLOT(ctl_playNote()));
    m_importer->connect(m_importer->ui->testNote,  SIGNAL(released()), m_generator,  SLOT(ctl_stopNote()));

    qDebug() << "Trying to start audio... (with dereferencing of RtGenerator!)";
    //Start generator!
    m_audioOut->start(*rtgenerator);

#ifdef ENABLE_MIDI
    qDebug() << "Trying to init MIDI-IN...";
    m_midiIn = new MidiInRt(*rtgenerator, this);
#endif
}

static bool keyToNote(int k, IRealtimeControl *generator)
{
    bool pn = false;

    switch(k)
    {
    case Qt::Key_Z:
        generator->changeNote(48);
        pn = true;
        break;
    case Qt::Key_S:
        generator->changeNote(49);
        pn = true;
        break;
    case Qt::Key_X:
        generator->changeNote(50);
        pn = true;
        break;
    case Qt::Key_D:
        generator->changeNote(51);
        pn = true;
        break;
    case Qt::Key_C:
        generator->changeNote(52);
        pn = true;
        break;
    case Qt::Key_V:
        generator->changeNote(53);
        pn = true;
        break;
    case Qt::Key_G:
        generator->changeNote(54);
        pn = true;
        break;
    case Qt::Key_B:
        generator->changeNote(55);
        pn = true;
        break;
    case Qt::Key_H:
        generator->changeNote(56);
        pn = true;
        break;
    case Qt::Key_N:
        generator->changeNote(57);
        pn = true;
        break;
    case Qt::Key_J:
        generator->changeNote(58);
        pn = true;
        break;
    case Qt::Key_M:
        generator->changeNote(59);
        pn = true;
        break;
    case Qt::Key_Q:
    case Qt::Key_Comma:
        generator->changeNote(60);
        pn = true;
        break;
    case Qt::Key_2:
    case Qt::Key_L:
        generator->changeNote(61);
        pn = true;
        break;
    case Qt::Key_W:
    case Qt::Key_Period:
        generator->changeNote(62);
        pn = true;
        break;
    case Qt::Key_3:
    case Qt::Key_Semicolon:
        generator->changeNote(63);
        pn = true;
        break;
    case Qt::Key_E:
    case Qt::Key_Slash:
        generator->changeNote(64);
        pn = true;
        break;
    case Qt::Key_R:
        generator->changeNote(65);
        pn = true;
        break;
    case Qt::Key_5:
        generator->changeNote(66);
        pn = true;
        break;
    case Qt::Key_T:
        generator->changeNote(67);
        pn = true;
        break;
    case Qt::Key_6:
        generator->changeNote(68);
        pn = true;
        break;
    case Qt::Key_Y:
        generator->changeNote(69);
        pn = true;
        break;
    case Qt::Key_7:
        generator->changeNote(70);
        pn = true;
        break;
    case Qt::Key_U:
        generator->changeNote(71);
        pn = true;
        break;
    case Qt::Key_I:
        generator->changeNote(72);
        pn = true;
        break;
    case Qt::Key_9:
        generator->changeNote(73);
        pn = true;
        break;
    case Qt::Key_O:
        generator->changeNote(74);
        pn = true;
        break;
    case Qt::Key_0:
        generator->changeNote(75);
        pn = true;
        break;
    case Qt::Key_P:
        generator->changeNote(76);
        pn = true;
        break;
    default:
        break;
    }

    return pn;
}

void BankEditor::keyPressEvent(QKeyEvent *event)
{
    if(event->isAutoRepeat())
        return;

    if(ui->melodic->isChecked())
    {
        if(keyToNote(event->key(), m_generator))
            m_generator->ctl_playNote();
    }
    else
    {
        switch(event->key())
        {
        case Qt::Key_Space:
            m_generator->ctl_playNote();
            break;
        default:
            break;
        }
    }

    QMainWindow::keyPressEvent(event);
}

void BankEditor::keyReleaseEvent(QKeyEvent *event)
{
    if(event->isAutoRepeat())
        return;

    if(ui->melodic->isChecked())
    {
        if(keyToNote(event->key(), m_generator))
            m_generator->ctl_stopNote();
    }
    else
    {
        switch(event->key())
        {
        case Qt::Key_Space:
            m_generator->ctl_noteOffAllChans();
            break;
        default:
            break;
        }
    }

    QMainWindow::keyReleaseEvent(event);
}
