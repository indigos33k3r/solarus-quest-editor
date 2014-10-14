/*
 * Copyright (C) 2014 Christopho, Solarus - http://www.solarus-games.org
 *
 * Solarus Quest Editor is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Solarus Quest Editor is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef SOLARUSEDITOR_TILESET_MODEL_H
#define SOLARUSEDITOR_TILESET_MODEL_H

#include <solarus/entities/TilesetData.h>
#include <QAbstractItemModel>
#include <QCollator>
#include <map>

class Quest;

using TilesetData = Solarus::TilesetData;

// TODO move to a separate file
class NaturalComparator {

public:

  NaturalComparator() {
    collator.setNumericMode(true);
  }

  bool operator() (const QString& lhs, const QString& rhs) const {
    return collator.compare(lhs, rhs) < 0;
  }

private:

  QCollator collator;

};

/**
 * @brief Model that wraps a tileset.
 *
 * It makes the link between the editor and the tileset data of the
 * Solarus library.
 * Signals are sent when something changes in the wrapped tileset.
 * This model can be used as a model for a list view of tile patterns.
 *
 * TODO add selection info to this class.
 */
class TilesetModel : public QAbstractListModel {
  Q_OBJECT

public:

  // Creation.
  TilesetModel(
      Quest& quest, const QString& tileset_id, QObject* parent = nullptr);

  // QAbstractListModel interface.
  virtual int rowCount(const QModelIndex& parent) const override;
  virtual QVariant data(const QModelIndex& index, int role) const override;

  // Tileset data.
  QColor get_background_color() const;
  void set_background_color(const QColor& background_color);
  int get_num_patterns() const;
  int get_pattern_index(const QString& pattern_id) const;
  QString get_pattern_id(int index) const;

signals:

  void background_color_changed(const QColor& background_color);

public slots:

  void save() const;

private:

  void build_index_map();

  Quest& quest;                   /**< The quest the tileset belongs to. */
  const QString tileset_id;       /**< Id of the tileset. */
  TilesetData tileset;            /**< Tileset data wrapped by this model. */

  QStringList indexes_to_ids;     /**< Id of each pattern in the list. */
  std::map<QString, int, NaturalComparator>
      ids_to_indexes;             /**< Index in the list of each pattern. */

};

#endif
