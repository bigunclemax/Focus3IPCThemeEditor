//
// Created by user on 14.06.2021.
//

#ifndef FOCUSIPC_HEADEROBJECTSMODEL_H
#define FOCUSIPC_HEADEROBJECTSMODEL_H

#include <QAbstractTableModel>
#include <ImageSection.h>

class HeaderObjectsModel : public QAbstractTableModel {

    Q_OBJECT

    enum enColumnsNames {
        width,
        height,
        X,
        Y,
        type,
        z,
        intensity,
        R,
        G,
        B,
        A,
        COL_MAX
    };

    [[nodiscard]] static inline const char *colNamesToString(enColumnsNames n) {
        switch (n) {
            case width :
                return "Width";
            case height :
                return "Height";
            case X :
                return "X";
            case Y :
                return "Y";
            case type :
                return "Type";
            case z :
                return "Z index";
            case intensity :
                return "Intensity";
            case R :
                return "R";
            case G :
                return "G";
            case B :
                return "B";
            case A :
                return "Palette";
            default:
                return "";
        }
    }

public:

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;

    void importLines(const vector <ImageSection::HeaderRecord> &data);
    [[nodiscard]] const vector <ImageSection::HeaderRecord> & exportLines() const;

private:
    vector <ImageSection::HeaderRecord> m_lines{};

};

#endif //FOCUSIPC_HEADEROBJECTSMODEL_H
