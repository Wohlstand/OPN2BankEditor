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


void BankEditor::on_lfoEnable_clicked(bool checked)
{
    if(m_lock) return;
    if(!m_curInst) return;
    m_bank.lfo_enabled = checked;
}

void BankEditor::on_lfoFrequency_currentIndexChanged(int index)
{
    if(m_lock) return;
    if(!m_curInst) return;
    m_bank.lfo_frequency = uint8_t(index);
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
    sendPatch();
}

void BankEditor::on_algorithm_currentIndexChanged(int index)
{
    if(m_lock) return;
    if(!m_curInst) return;
    m_curInst->algorithm = uint8_t(index);
    sendPatch();
}

void BankEditor::on_amsens_currentIndexChanged(int index)
{
    if(m_lock) return;
    if(!m_curInst) return;
    m_curInst->am = uint8_t(index);
    sendPatch();
}

void BankEditor::on_fmsens_currentIndexChanged(int index)
{
    if(m_lock) return;
    if(!m_curInst) return;
    m_curInst->fm = uint8_t(index);
    sendPatch();
}

void BankEditor::on_perc_noteNum_valueChanged(int arg1)
{
    if(m_lock) return;
    if(!m_curInst) return;
    m_curInst->percNoteNum = uint8_t(arg1);
    if(ui->percussion->isChecked())
        ui->noteToTest->setValue(arg1);
    sendPatch();
}

void BankEditor::on_noteOffset1_valueChanged(int arg1)
{
    if(m_lock) return;
    if(!m_curInst) return;
    m_curInst->note_offset1 = short(arg1);
    sendPatch();
}


/* =============== OPERATOR 1 =============== */

void BankEditor::on_op1_attack_valueChanged(int arg1)
{
    if(m_lock) return;
    if(!m_curInst) return;
    m_curInst->OP[OPERATOR1_HR].attack = uint8_t(arg1);
    sendPatch();
}

void BankEditor::on_op1_decay1_valueChanged(int arg1)
{
    if(m_lock) return;
    if(!m_curInst) return;
    m_curInst->OP[OPERATOR1_HR].decay1 = uint8_t(arg1);
    sendPatch();
}

void BankEditor::on_op1_decay2_valueChanged(int arg1)
{
    if(m_lock) return;
    if(!m_curInst) return;
    m_curInst->OP[OPERATOR1_HR].decay2 = uint8_t(arg1);
    sendPatch();
}

void BankEditor::on_op1_sustain_valueChanged(int arg1)
{
    if(m_lock) return;
    if(!m_curInst) return;
    m_curInst->OP[OPERATOR1_HR].sustain = uint8_t(arg1);
    sendPatch();
}

void BankEditor::on_op1_release_valueChanged(int arg1)
{
    if(m_lock) return;
    if(!m_curInst) return;
    m_curInst->OP[OPERATOR1_HR].release = uint8_t(arg1);
    sendPatch();
}

void BankEditor::on_op1_am_toggled(bool checked)
{
    if(m_lock) return;
    if(!m_curInst) return;
    m_curInst->OP[OPERATOR1_HR].am_enable = uint8_t(checked);
    sendPatch();
}

void BankEditor::on_op1_level_valueChanged(int arg1)
{
    if(m_lock) return;
    if(!m_curInst) return;
    m_curInst->OP[OPERATOR1_HR].level = uint8_t(arg1);
    sendPatch();
}

void BankEditor::on_op1_freqmult_valueChanged(int arg1)
{
    if(m_lock) return;
    if(!m_curInst) return;
    m_curInst->OP[OPERATOR1_HR].fmult = uint8_t(arg1);
    sendPatch();
}

void BankEditor::on_op1_detune_currentIndexChanged(int index)
{
    if(m_lock) return;
    if(!m_curInst) return;
    m_curInst->OP[OPERATOR1_HR].detune = uint8_t(index);
    sendPatch();
}

void BankEditor::on_op1_ratescale_valueChanged(int arg1)
{
    if(m_lock) return;
    if(!m_curInst) return;
    m_curInst->OP[OPERATOR1_HR].ratescale = uint8_t(arg1);
    sendPatch();
}

void BankEditor::on_op1_ssgeg_currentIndexChanged(int index)
{
    if(m_lock) return;
    if(!m_curInst) return;
    m_curInst->OP[OPERATOR1_HR].ssg_eg = uint8_t(index);
    sendPatch();
}



/* =============== OPERATOR 2 =============== */


void BankEditor::on_op2_attack_valueChanged(int arg1)
{
    if(m_lock) return;
    if(!m_curInst) return;
    m_curInst->OP[OPERATOR2_HR].attack = uint8_t(arg1);
    sendPatch();
}

void BankEditor::on_op2_decay1_valueChanged(int arg1)
{
    if(m_lock) return;
    if(!m_curInst) return;
    m_curInst->OP[OPERATOR2_HR].decay1 = uint8_t(arg1);
    sendPatch();
}

void BankEditor::on_op2_decay2_valueChanged(int arg1)
{
    if(m_lock) return;
    if(!m_curInst) return;
    m_curInst->OP[OPERATOR2_HR].decay2 = uint8_t(arg1);
    sendPatch();
}

void BankEditor::on_op2_sustain_valueChanged(int arg1)
{
    if(m_lock) return;
    if(!m_curInst) return;
    m_curInst->OP[OPERATOR2_HR].sustain = uint8_t(arg1);
    sendPatch();
}

void BankEditor::on_op2_release_valueChanged(int arg1)
{
    if(m_lock) return;
    if(!m_curInst) return;
    m_curInst->OP[OPERATOR2_HR].release = uint8_t(arg1);
    sendPatch();
}

void BankEditor::on_op2_am_toggled(bool checked)
{
    if(m_lock) return;
    if(!m_curInst) return;
    m_curInst->OP[OPERATOR2_HR].am_enable = uint8_t(checked);
    sendPatch();
}

void BankEditor::on_op2_level_valueChanged(int arg1)
{
    if(m_lock) return;
    if(!m_curInst) return;
    m_curInst->OP[OPERATOR2_HR].level = uint8_t(arg1);
    sendPatch();
}

void BankEditor::on_op2_freqmult_valueChanged(int arg1)
{
    if(m_lock) return;
    if(!m_curInst) return;
    m_curInst->OP[OPERATOR2_HR].fmult = uint8_t(arg1);
    sendPatch();
}

void BankEditor::on_op2_detune_currentIndexChanged(int index)
{
    if(m_lock) return;
    if(!m_curInst) return;
    m_curInst->OP[OPERATOR2_HR].detune = uint8_t(index);
    sendPatch();
}

void BankEditor::on_op2_ratescale_valueChanged(int arg1)
{
    if(m_lock) return;
    if(!m_curInst) return;
    m_curInst->OP[OPERATOR2_HR].ratescale = uint8_t(arg1);
    sendPatch();
}

void BankEditor::on_op2_ssgeg_currentIndexChanged(int index)
{
    if(m_lock) return;
    if(!m_curInst) return;
    m_curInst->OP[OPERATOR2_HR].ssg_eg = uint8_t(index);
    sendPatch();
}



/* =============== OPERATOR 3 =============== */

void BankEditor::on_op3_attack_valueChanged(int arg1)
{
    if(m_lock) return;
    if(!m_curInst) return;
    m_curInst->OP[OPERATOR3_HR].attack = uint8_t(arg1);
    sendPatch();
}

void BankEditor::on_op3_decay1_valueChanged(int arg1)
{
    if(m_lock) return;
    if(!m_curInst) return;
    m_curInst->OP[OPERATOR3_HR].decay1 = uint8_t(arg1);
    sendPatch();
}

void BankEditor::on_op3_decay2_valueChanged(int arg1)
{
    if(m_lock) return;
    if(!m_curInst) return;
    m_curInst->OP[OPERATOR3_HR].decay2 = uint8_t(arg1);
    sendPatch();
}

void BankEditor::on_op3_sustain_valueChanged(int arg1)
{
    if(m_lock) return;
    if(!m_curInst) return;
    m_curInst->OP[OPERATOR3_HR].sustain = uint8_t(arg1);
    sendPatch();
}

void BankEditor::on_op3_release_valueChanged(int arg1)
{
    if(m_lock) return;
    if(!m_curInst) return;
    m_curInst->OP[OPERATOR3_HR].release = uint8_t(arg1);
    sendPatch();
}

void BankEditor::on_op3_am_toggled(bool checked)
{
    if(m_lock) return;
    if(!m_curInst) return;
    m_curInst->OP[OPERATOR3_HR].am_enable = uint8_t(checked);
    sendPatch();
}

void BankEditor::on_op3_level_valueChanged(int arg1)
{
    if(m_lock) return;
    if(!m_curInst) return;
    m_curInst->OP[OPERATOR3_HR].level = uint8_t(arg1);
    sendPatch();
}

void BankEditor::on_op3_freqmult_valueChanged(int arg1)
{
    if(m_lock) return;
    if(!m_curInst) return;
    m_curInst->OP[OPERATOR3_HR].fmult = uint8_t(arg1);
    sendPatch();
}

void BankEditor::on_op3_detune_currentIndexChanged(int index)
{
    if(m_lock) return;
    if(!m_curInst) return;
    m_curInst->OP[OPERATOR3_HR].detune = uint8_t(index);
    sendPatch();
}

void BankEditor::on_op3_ratescale_valueChanged(int arg1)
{
    if(m_lock) return;
    if(!m_curInst) return;
    m_curInst->OP[OPERATOR3_HR].ratescale = uint8_t(arg1);
    sendPatch();
}

void BankEditor::on_op3_ssgeg_currentIndexChanged(int index)
{
    if(m_lock) return;
    if(!m_curInst) return;
    m_curInst->OP[OPERATOR3_HR].ssg_eg = uint8_t(index);
    sendPatch();
}



/* =============== OPERATOR 4 =============== */

void BankEditor::on_op4_attack_valueChanged(int arg1)
{
    if(m_lock) return;
    if(!m_curInst) return;
    m_curInst->OP[OPERATOR4_HR].attack = uint8_t(arg1);
    sendPatch();
}

void BankEditor::on_op4_decay1_valueChanged(int arg1)
{
    if(m_lock) return;
    if(!m_curInst) return;
    m_curInst->OP[OPERATOR4_HR].decay1 = uint8_t(arg1);
    sendPatch();
}

void BankEditor::on_op4_decay2_valueChanged(int arg1)
{
    if(m_lock) return;
    if(!m_curInst) return;
    m_curInst->OP[OPERATOR4_HR].decay2 = uint8_t(arg1);
    sendPatch();
}

void BankEditor::on_op4_sustain_valueChanged(int arg1)
{
    if(m_lock) return;
    if(!m_curInst) return;
    m_curInst->OP[OPERATOR4_HR].sustain = uint8_t(arg1);
    sendPatch();
}

void BankEditor::on_op4_release_valueChanged(int arg1)
{
    if(m_lock) return;
    if(!m_curInst) return;
    m_curInst->OP[OPERATOR4_HR].release = uint8_t(arg1);
    sendPatch();
}

void BankEditor::on_op4_am_toggled(bool checked)
{
    if(m_lock) return;
    if(!m_curInst) return;
    m_curInst->OP[OPERATOR4_HR].am_enable = uint8_t(checked);
    sendPatch();
}

void BankEditor::on_op4_level_valueChanged(int arg1)
{
    if(m_lock) return;
    if(!m_curInst) return;
    m_curInst->OP[OPERATOR4_HR].level = uint8_t(arg1);
    sendPatch();
}

void BankEditor::on_op4_freqmult_valueChanged(int arg1)
{
    if(m_lock) return;
    if(!m_curInst) return;
    m_curInst->OP[OPERATOR4_HR].fmult = uint8_t(arg1);
    sendPatch();
}

void BankEditor::on_op4_detune_currentIndexChanged(int index)
{
    if(m_lock) return;
    if(!m_curInst) return;
    m_curInst->OP[OPERATOR4_HR].detune = uint8_t(index);
    sendPatch();
}

void BankEditor::on_op4_ratescale_valueChanged(int arg1)
{
    if(m_lock) return;
    if(!m_curInst) return;
    m_curInst->OP[OPERATOR4_HR].ratescale = uint8_t(arg1);
    sendPatch();
}

void BankEditor::on_op4_ssgeg_currentIndexChanged(int index)
{
    if(m_lock) return;
    if(!m_curInst) return;
    m_curInst->OP[OPERATOR4_HR].ssg_eg = uint8_t(index);
    sendPatch();
}
