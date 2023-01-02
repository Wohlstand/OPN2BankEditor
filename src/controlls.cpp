/*
 * OPN2 Bank Editor by Wohlstand, a free tool for music bank editing
 * Copyright (c) 2017-2023 Vitaly Novichkov <admin@wohlnet.ru>
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
#include "operator_editor.h"
#include <cmath>

/**
 TODO: Replace everything here with one huge function that adds lambdas as signals
       and adds "blockSignal(bool)" call into the signal
       that will replace `m_lock` workaround crap
 */

void BankEditor::on_chipType_currentIndexChanged(int index)
{
    if(m_lock) return;
    m_bank.opna_mode = (index > 0);
    m_currentChipFamily = m_bank.opna_mode ? OPNChip_OPNA : OPNChip_OPN2;
    m_generator->ctl_switchChip(m_currentChip, static_cast<int>(m_currentChipFamily));
}

void BankEditor::on_lfoEnable_clicked(bool checked)
{
    if(m_lock) return;
    m_bank.lfo_enabled = checked;
}

void BankEditor::on_lfoFrequency_currentIndexChanged(int index)
{
    if(m_lock) return;
    m_bank.lfo_frequency = uint8_t(index);
}

void BankEditor::on_volumeSlider_valueChanged(int value)
{
    m_generator->ctl_changeVolume((unsigned)value);
}

void BankEditor::on_insName_textChanged(const QString &arg1)
{
    if(m_lock) return;
    if(!m_curInst) return;
    strncpy(m_curInst->name, arg1.toUtf8().data(), 32);
}

void BankEditor::on_insName_editingFinished()
{
    if(m_lock) return;
    if(!m_curInst) return;
    QString arg1 = ui->insName->text();
    strncpy(m_curInst->name, arg1.toUtf8().data(), 32);
    reloadInstrumentNames();
}

void BankEditor::on_feedback1_valueChanged(int arg1)
{
    if(m_lock) return;
    if(!m_curInst) return;
    m_curInst->feedback = uint8_t(arg1);
    afterChangeControlValue();
}

void BankEditor::on_algorithm_currentIndexChanged(int index)
{
    if(m_lock) return;
    if(!m_curInst) return;
    m_curInst->algorithm = uint8_t(index);
    afterChangeControlValue();
}

void BankEditor::on_amsens_currentIndexChanged(int index)
{
    if(m_lock) return;
    if(!m_curInst) return;
    m_curInst->am = uint8_t(index);
    afterChangeControlValue();
}

void BankEditor::on_fmsens_currentIndexChanged(int index)
{
    if(m_lock) return;
    if(!m_curInst) return;
    m_curInst->fm = uint8_t(index);
    afterChangeControlValue();
}

void BankEditor::on_perc_noteNum_valueChanged(int arg1)
{
    if(m_lock) return;
    if(!m_curInst) return;
    m_curInst->percNoteNum = uint8_t(arg1);
    if(ui->percussion->isChecked())
        ui->noteToTest->setValue(arg1);
    afterChangeControlValue();
}

void BankEditor::on_fixedNote_clicked(bool checked)
{
    if(m_lock) return;
    if(!m_curInst) return;
    m_curInst->is_fixed_note = checked;
    afterChangeControlValue();
}

void BankEditor::on_noteOffset1_valueChanged(int arg1)
{
    if(m_lock) return;
    if(!m_curInst) return;
    m_curInst->note_offset1 = short(arg1);
    afterChangeControlValue();
}

void BankEditor::onOperatorChanged()
{
    if(m_lock) return;
    if(!m_curInst) return;
    static_cast<OperatorEditor *>(sender())->writeDataToInst(*m_curInst);
    afterChangeControlValue();
}

void BankEditor::afterChangeControlValue()
{
    Q_ASSERT(m_curInst);
    if(m_curInst->is_blank)
    {
        m_curInst->is_blank = false;
        syncInstrumentBlankness();
    }
    sendPatch();
}

void BankEditor::on_pitchBendSlider_valueChanged(int value)
{
    int bend = (int)std::lround(value * (8192.0 / 100.0));
    m_generator->ctl_pitchBend(bend);
}

void BankEditor::on_pitchBendSlider_sliderReleased()
{
    ui->pitchBendSlider->setValue(0);  // spring back to middle position
}

void BankEditor::on_holdButton_toggled(bool checked)
{
    m_generator->ctl_hold(checked);
}
