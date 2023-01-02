#ifndef INSTRUMENTSLISTMODEL_H
#define INSTRUMENTSLISTMODEL_H

#include <QAbstractListModel>
class FmBank;

#define INS_INDEX   (Qt::UserRole)
#define INS_BANK_ID (Qt::UserRole + 1)
#define INS_INS_ID  (Qt::UserRole + 2)


class InstrumentsListModel : public QAbstractListModel
{
    Q_OBJECT
    //! Bank storage used as a data source
    FmBank *m_curBank = nullptr;
    //! Toggle between melodic and percussion
    bool m_modePercussion = false;
    //! Show all instruments of all banks
    bool m_showAllInstruments = false;
    //! Current bank Index
    int m_bankIndex = 0;

    unsigned int m_midiSpec = 16;

public:
    explicit InstrumentsListModel(FmBank *bank, QObject *parent = nullptr);

    void setPersecutionMode(bool en);
    void setShowAll(bool en);
    void switchBank(int index);

    void updateList();
    void updateList(int begin, int end);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;

private:
    QString getInstName(int inst, bool isAuto, bool isPerc) const;
};

#endif // INSTRUMENTSLISTMODEL_H
