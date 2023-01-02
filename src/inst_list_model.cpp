#include <QBrush>

#include "inst_list_model.h"
#include "bank.h"
#include "ins_names.h"

InstrumentsListModel::InstrumentsListModel(FmBank *bank, QObject *parent)
    : QAbstractListModel(parent),
      m_curBank(bank)
{
    Q_ASSERT(m_curBank);
}

void InstrumentsListModel::setPersecutionMode(bool en)
{
    beginResetModel();
    m_modePercussion = en;
    endResetModel();
}

void InstrumentsListModel::setShowAll(bool en)
{
    beginResetModel();
    m_showAllInstruments = en;
    endResetModel();
}

void InstrumentsListModel::switchBank(int index)
{
    beginResetModel();
    m_bankIndex = index;
    endResetModel();
}

void InstrumentsListModel::updateList()
{
    beginResetModel();
    m_bankIndex = 0;
    endResetModel();
}

void InstrumentsListModel::updateList(int begin, int end)
{
    QModelIndex l = createIndex(begin,0);
    QModelIndex r = createIndex(end ,0);
    emit dataChanged(l, r);
}

int InstrumentsListModel::rowCount(const QModelIndex &parent) const
{
    // For list models only the root node (an invalid parent) should return the list's size. For all
    // other (valid) parents, rowCount() should return 0 so that it does not become a tree model.
    if (parent.isValid())
        return 0;

    if(m_showAllInstruments)
    {
        if(m_modePercussion)
            return m_curBank->countDrums();
        else
            return m_curBank->countMelodic();
    }

    int total, off, ret;

    if(m_modePercussion)
        total = m_curBank->countDrums();
    else
        total = m_curBank->countMelodic();

    off = m_bankIndex * 128;

    if(off >= total)
        ret = 0;
    else if(off + 128 > total)
        ret = total - off;
    else
        ret = 128;

    return ret;
}

QVariant InstrumentsListModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid())
        return QVariant();

    int off = m_bankIndex * 128;
    int instIdx = off + index.row();

    int count = m_modePercussion ? m_curBank->countDrums() : m_curBank->countMelodic();
    FmBank::Instrument *ins = m_modePercussion ? m_curBank->Ins_Percussion : m_curBank->Ins_Melodic;

    if(instIdx > count)
        return QVariant();

    switch(role)
    {
    case Qt::DisplayRole:
    {
        char *n = ins[instIdx].name;
        if(n[0] != '\0')
            return QString::fromUtf8(n);
        else
            return getInstName(instIdx, false, m_modePercussion);
    }

    case INS_INDEX:
        return instIdx;

    case INS_BANK_ID:
        return m_bankIndex;

    case INS_INS_ID:
        return index.row();

    case Qt::ToolTipRole:
        return QString("Bank %1, ID: %2").arg(m_bankIndex).arg(index.row());

    case Qt::ForegroundRole:
        return QBrush(ins[off + index.row()].is_blank ? Qt::gray : Qt::black);

    default:
        return QVariant();
    }
}

Qt::ItemFlags InstrumentsListModel::flags(const QModelIndex &index) const
{
    if (index.isValid())
        return (QAbstractListModel::flags(index)|Qt::ItemIsEnabled|Qt::ItemIsSelectable);
    return Qt::ItemIsEnabled|Qt::ItemIsSelectable;
}

QString InstrumentsListModel::getInstName(int inst, bool isAuto, bool isPerc) const
{
    QString name = tr("<Unknown>");

    if(m_bankIndex >= 0)
    {
        if(isAuto)
            isPerc = m_modePercussion;

        QVector<FmBank::MidiBank> &b = isPerc ? m_curBank->Banks_Percussion : m_curBank->Banks_Melodic;

        int lsb = b[m_bankIndex].lsb;
        int msb = b[m_bankIndex].msb;
        MidiProgramId pr = MidiProgramId(isPerc, msb, lsb, inst % 128);
        unsigned spec = m_midiSpec;
        unsigned specObtained = kMidiSpecXG;
        const MidiProgram *p = getMidiProgram(pr, spec, &specObtained);
        p = p ? p : getFallbackProgram(pr, spec, &specObtained);
        name = p ? p->patchName : tr("<Reserved %1>").arg(inst % 128);
    }

    return name;
}
