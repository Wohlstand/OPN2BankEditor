/*
 * OPN2 Bank Editor by Wohlstand, a free tool for music bank editing
 * Copyright (c) 2017-2025 Vitaly Novichkov <admin@wohlnet.ru>
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

#include "operator_editor.h"
#include "ui_operator_editor.h"
#include <QStandardItemModel>

OperatorEditor::OperatorEditor(QWidget *parent)
    : QWidget(parent), m_ui(new Ui::OperatorEditor)
{
    Ui::OperatorEditor &ui = *m_ui;
    ui.setupUi(this);

    /* Hide first 7 SSG-EG items */
    QStandardItemModel *model = qobject_cast<QStandardItemModel *>(ui.op_ssgeg->model());
    for(int j = 1; j < 8; j++)
    {
        QStandardItem *item = model->item(j);
        item->setFlags(item->flags() & ~Qt::ItemIsEnabled);
    }

    connect(ui.op_attack, SIGNAL(valueChanged(int)), this, SIGNAL(operatorChanged()));
    connect(ui.op_decay1, SIGNAL(valueChanged(int)), this, SIGNAL(operatorChanged()));
    connect(ui.op_decay2, SIGNAL(valueChanged(int)), this, SIGNAL(operatorChanged()));
    connect(ui.op_sustain, SIGNAL(valueChanged(int)), this, SIGNAL(operatorChanged()));
    connect(ui.op_release, SIGNAL(valueChanged(int)), this, SIGNAL(operatorChanged()));
    connect(ui.op_am, SIGNAL(toggled(bool)), this, SIGNAL(operatorChanged()));
    connect(ui.op_level, SIGNAL(valueChanged(int)), this, SIGNAL(operatorChanged()));
    connect(ui.op_freqmult, SIGNAL(valueChanged(int)), this, SIGNAL(operatorChanged()));
    connect(ui.op_detune, SIGNAL(currentIndexChanged(int)), this, SIGNAL(operatorChanged()));
    connect(ui.op_ratescale, SIGNAL(valueChanged(int)), this, SIGNAL(operatorChanged()));
    connect(ui.op_ssgeg, SIGNAL(currentIndexChanged(int)), this, SIGNAL(operatorChanged()));
}

OperatorEditor::~OperatorEditor()
{
}

void OperatorEditor::onLanguageChanged()
{
    Ui::OperatorEditor &ui = *m_ui;
    ui.retranslateUi(this);
}

unsigned OperatorEditor::operatorNumber() const
{
    return m_operatorNumber;
}

void OperatorEditor::setOperatorNumber(unsigned n)
{
    m_operatorNumber = n;
}

void OperatorEditor::loadDataFromInst(const FmBank::Instrument &inst)
{
    unsigned operator_numbers[4] = {OPERATOR1_HR, OPERATOR2_HR, OPERATOR3_HR, OPERATOR4_HR};
    const FmBank::Operator &op = inst.OP[operator_numbers[m_operatorNumber]];

    Ui::OperatorEditor &ui = *m_ui;
    ui.op_attack->setValue(op.attack);
    ui.op_decay1->setValue(op.decay1);
    ui.op_decay2->setValue(op.decay2);
    ui.op_sustain->setValue(op.sustain);
    ui.op_release->setValue(op.release);
    ui.op_am->setChecked(op.am_enable);
    ui.op_freqmult->setValue(op.fmult);
    ui.op_level->setValue(op.level);
    ui.op_detune->setCurrentIndex(op.detune);
    ui.op_ratescale->setValue(op.ratescale);
    ui.op_ssgeg->setCurrentIndex(op.ssg_eg);
}

void OperatorEditor::writeDataToInst(FmBank::Instrument &inst) const
{
    unsigned operator_numbers[4] = {OPERATOR1_HR, OPERATOR2_HR, OPERATOR3_HR, OPERATOR4_HR};
    FmBank::Operator &op = inst.OP[operator_numbers[m_operatorNumber]];

    const Ui::OperatorEditor &ui = *m_ui;
    op.attack = uint8_t(ui.op_attack->value());
    op.decay1 = uint8_t(ui.op_decay1->value());
    op.decay2 = uint8_t(ui.op_decay2->value());
    op.sustain = uint8_t(ui.op_sustain->value());
    op.release = uint8_t(ui.op_release->value());
    op.am_enable = uint8_t(ui.op_am->isChecked());
    op.level = uint8_t(ui.op_level->value());
    op.fmult = uint8_t(ui.op_freqmult->value());
    op.detune = uint8_t(ui.op_detune->currentIndex());
    op.ratescale = uint8_t(ui.op_ratescale->value());
    op.ssg_eg = uint8_t(ui.op_ssgeg->currentIndex());
}
