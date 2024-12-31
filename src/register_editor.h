/*
 * OPN2 Bank Editor by Wohlstand, a free tool for music bank editing
 * Copyright (c) 2018-2025 Vitaly Novichkov <admin@wohlnet.ru>
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

#include "bank.h"
#include <QDialog>
#include <memory>

namespace Ui { class RegisterEditorDialog; }
class QLineEdit;

class RegisterEditorDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RegisterEditorDialog(FmBank::Instrument *ins, QWidget *parent = nullptr);
    ~RegisterEditorDialog();

private:
    void loadInstrument();
private slots:
    void saveInstrument();

protected:
    void changeEvent(QEvent *event) override;

private:
    /**
     * @brief Updates the text to display after a language change
     */
    void onLanguageChanged();

private:
    const std::unique_ptr<Ui::RegisterEditorDialog> m_ui;
    FmBank::Instrument *m_ins = nullptr;
    enum { RegisterCount = 30 };
    QLineEdit *m_regWidgets[RegisterCount] = {};
};
