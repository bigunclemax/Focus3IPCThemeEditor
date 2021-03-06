//
// Created by user on 14.06.2021.
//

#include "HeaderObjectsModel.h"

int HeaderObjectsModel::rowCount(const QModelIndex &parent) const {
    return (int) m_lines.size();
}

int HeaderObjectsModel::columnCount(const QModelIndex &parent) const {
    return COL_MAX;
}

QVariant HeaderObjectsModel::data(const QModelIndex &index, int role) const {

    if (role == Qt::DisplayRole) {

        if ((index.row() >= m_lines.size()) || (index.column() >= COL_MAX))
            return QVariant();

        const auto &line = m_lines[index.row()];
        switch (index.column()) {
            case width :
                return line.width;
            case height :
                return line.height;
            case X :
                return line.X;
            case Y :
                return line.Y;
            case type :
                return line.type;
            case z :
                return line.Z;
            case intensity :
                return line.intensity;
            case R :
                return line.R;
            case G :
                return line.G;
            case B :
                return line.B;
            case A :
                return line.palette_id;
            default:
                return QVariant();
        }
    }

    return QVariant();
}

QVariant HeaderObjectsModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (role == Qt::DisplayRole) {

        if (orientation == Qt::Horizontal) {
            return QString(colNamesToString(static_cast<enColumnsNames>(section)));
        } else {
            return section + 1;
        }

    }
    return QVariant();
}

Qt::ItemFlags HeaderObjectsModel::flags(const QModelIndex &index) const {
    return Qt::ItemIsEditable | QAbstractTableModel::flags(index);
}

bool HeaderObjectsModel::setData(const QModelIndex &index, const QVariant &value, int role) {
    if (role == Qt::EditRole) {
        if (!checkIndex(index))
            return false;

        bool ok;
        uint32_t v = value.toUInt(&ok);
        if (!ok)
            return false;

        switch (index.column()) {
            case width :
                m_lines[index.row()].width = v;
                break;
            case height :
                m_lines[index.row()].height = v;
                break;
            case X :
                m_lines[index.row()].X = v;
                break;
            case Y :
                m_lines[index.row()].Y = v;
                break;
            case type :
                if(v > std::numeric_limits<uint8_t>::max())
                    return false;
                m_lines[index.row()].type = v;
                break;
            case z :
                if(v > std::numeric_limits<uint8_t>::max())
                    return false;
                m_lines[index.row()].Z = v;
                break;
            case intensity :
                if(v > std::numeric_limits<uint8_t>::max())
                    return false;
                m_lines[index.row()].intensity = v;
                break;
            case R :
                if(v > std::numeric_limits<uint8_t>::max())
                    return false;
                m_lines[index.row()].R = v;
                break;
            case G :
                if(v > std::numeric_limits<uint8_t>::max())
                    return false;
                m_lines[index.row()].G = v;
                break;
            case B :
                if(v > std::numeric_limits<uint8_t>::max())
                    return false;
                m_lines[index.row()].B = v;
                break;
            case A :
                if(v > std::numeric_limits<uint8_t>::max())
                    return false;
                m_lines[index.row()].palette_id = v;
                break;
            default:
                return false;
        }

        return true;

    }
    return false;
}

void HeaderObjectsModel::importLines(const vector<ImageSection::HeaderRecord> &data) {

    m_lines = data;
    beginResetModel();
    endResetModel();
}

const vector<ImageSection::HeaderRecord> &HeaderObjectsModel::exportLines() const {
    return m_lines;
}
