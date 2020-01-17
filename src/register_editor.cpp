/*
 * OPN2 Bank Editor by Wohlstand, a free tool for music bank editing
 * Copyright (c) 2018-2020 Vitaly Novichkov <admin@wohlnet.ru>
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

#include "register_editor.h"
#include "ui_register_editor.h"
#include <QRegExpValidator>
#include <QDebug>
#include <algorithm>

class ByteValidator : public QValidator
{
public:
    explicit ByteValidator(QLineEdit **widgets, unsigned index, unsigned count);
    void fixup(QString &input) const override;
    State validate(QString &input, int &pos) const override;
private:
    QLineEdit **m_widgets = nullptr;
    unsigned m_index = 0;
    unsigned m_count = 0;
};

RegisterEditorDialog::RegisterEditorDialog(FmBank::Instrument *ins, QWidget *parent)
    : QDialog(parent),
      m_ui(new Ui::RegisterEditorDialog),
      m_ins(ins)
{
    Ui::RegisterEditorDialog *ui = m_ui.get();
    ui->setupUi(this);

    QLineEdit *regWidgets[RegisterCount] = {
        ui->reg3xOp1, ui->reg3xOp2, ui->reg3xOp3, ui->reg3xOp4,
        ui->reg4xOp1, ui->reg4xOp2, ui->reg4xOp3, ui->reg4xOp4,
        ui->reg5xOp1, ui->reg5xOp2, ui->reg5xOp3, ui->reg5xOp4,
        ui->reg6xOp1, ui->reg6xOp2, ui->reg6xOp3, ui->reg6xOp4,
        ui->reg7xOp1, ui->reg7xOp2, ui->reg7xOp3, ui->reg7xOp4,
        ui->reg8xOp1, ui->reg8xOp2, ui->reg8xOp3, ui->reg8xOp4,
        ui->reg9xOp1, ui->reg9xOp2, ui->reg9xOp3, ui->reg9xOp4,
        ui->regB0,    ui->regB4,
    };

    std::copy(regWidgets, regWidgets + RegisterCount, m_regWidgets);

    for(unsigned i = 0; i < RegisterCount; ++i)
    {
        QLineEdit *w = regWidgets[i];
        w->setValidator(new ByteValidator(m_regWidgets, i, RegisterCount));
    }
    loadInstrument();

    connect(this, SIGNAL(accepted()), this, SLOT(saveInstrument()));
}

RegisterEditorDialog::~RegisterEditorDialog()
{
}

static uint8_t getWidgetValue(QLineEdit *w)
{
    return (uint8_t)w->text().toUInt(nullptr, 16);
}

static void setWidgetValue(QLineEdit *w, uint8_t v)
{
    char text[3];
    snprintf(text, sizeof(text), "%02X", v);
    w->setText(text);
}

void RegisterEditorDialog::loadInstrument()
{
    const FmBank::Instrument &ins = *m_ins;
    QLineEdit **widgets = m_regWidgets;

    for(unsigned i = 0; i < 4; ++i)
    {
        const unsigned opnum[] = {OPERATOR1_HR, OPERATOR3_HR, OPERATOR2_HR, OPERATOR4_HR};
        const unsigned op = opnum[i];

        uint8_t values[7] = {
            ins.getRegDUMUL(op),
            ins.getRegLevel(op),
            ins.getRegRSAt(op),
            ins.getRegAMD1(op),
            ins.getRegD2(op),
            ins.getRegSysRel(op),
            ins.getRegSsgEg(op),
        };

        for (unsigned j = 0; j < 7; ++j)
        {
            QLineEdit *w = widgets[4 * j + i];
            setWidgetValue(w, values[j]);
        }
    }

    setWidgetValue(widgets[28], ins.getRegFbAlg());
    setWidgetValue(widgets[29], ins.getRegLfoSens());
}

void RegisterEditorDialog::saveInstrument()
{
    FmBank::Instrument &ins = *m_ins;
    QLineEdit **widgets = m_regWidgets;

    for(unsigned i = 0; i < 4; ++i)
    {
        const unsigned opnum[] = {OPERATOR1_HR, OPERATOR3_HR, OPERATOR2_HR, OPERATOR4_HR};
        const unsigned op = opnum[i];

        for (unsigned j = 0; j < 7; ++j)
        {
            QLineEdit *w = widgets[4 * j + i];
            uint8_t value = getWidgetValue(w);
            switch(j)
            {
            case 0: ins.setRegDUMUL(op, value); break;
            case 1: ins.setRegLevel(op, value); break;
            case 2: ins.setRegRSAt(op, value); break;
            case 3: ins.setRegAMD1(op, value); break;
            case 4: ins.setRegD2(op, value); break;
            case 5: ins.setRegSysRel(op, value); break;
            case 6: ins.setRegSsgEg(op, value); break;
            }
        }
    }

    ins.setRegFbAlg(getWidgetValue(widgets[28]));
    ins.setRegLfoSens(getWidgetValue(widgets[29]));
}

void RegisterEditorDialog::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange)
        onLanguageChanged();
    QDialog::changeEvent(event);
}

void RegisterEditorDialog::onLanguageChanged()
{
    m_ui->retranslateUi(this);
}

ByteValidator::ByteValidator(QLineEdit **widgets, unsigned index, unsigned count)
    : QValidator(widgets[index]),
      m_widgets(widgets), m_index(index), m_count(count)
{
}

void ByteValidator::fixup(QString &input) const
{
    for(QChar &ch : input)
        ch = ch.toUpper();
}

QValidator::State ByteValidator::validate(QString &input, int &pos) const
{
    QString result;
    unsigned length = input.size();
    result.reserve(length);

    // convert to uppercase discarding junk characters
    for (unsigned i = 0; i < length; ++i)
    {
        QChar ch = input[i].toUpper();
        if((ch >= '0' && ch <= '9') || (ch >= 'A' && ch <= 'F'))
            result.push_back(ch);
        else if((ch >= 'a' && ch <= 'f'))
            result.push_back(ch.toUpper());
        else if ((unsigned)pos > i)
            --pos;
    }

    // take 2 beggining characters at most
    length = result.size();
    input = result.left(std::min(2u, length));

    // pass rest to next editor
    QString rest = result.mid(std::min(2u, length));
    if(!rest.isEmpty())
    {
        if(m_index + 1 < m_count)
        {
            QLineEdit *next = m_widgets[m_index + 1];
            QMetaObject::invokeMethod(next, "setText", Qt::QueuedConnection, Q_ARG(QString, rest));
            QMetaObject::invokeMethod(next, "setFocus", Qt::QueuedConnection);
        }
    }

    return QValidator::Acceptable;
}
