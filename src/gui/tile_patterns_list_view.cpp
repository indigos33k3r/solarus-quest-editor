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
#include "gui/tile_patterns_list_model.h"
#include "gui/tile_patterns_list_view.h"

/**
 * @brief Creates an empty tile patterns list view.
 * @param parent The parent object or nullptr.
 */
TilePatternsListView::TilePatternsListView(QWidget* parent) :
  QListView(parent),
  tileset(nullptr),
  model(nullptr) {

  setIconSize(QSize(48, 48));
  setUniformItemSizes(true);
  setViewMode(IconMode);
  setResizeMode(Adjust);
}

/**
 * @brief Returns the tileset represented in this list view.
 * @return The tileset.
 */
const TilesetData* TilePatternsListView::get_tileset() const {
  return tileset;
}

/**
 * @brief Sets the tileset represented in this list view.
 * @param tileset The tileset.
 */
void TilePatternsListView::set_tileset(TilesetData* tileset) {

  this->tileset = tileset;
  this->model = new TilePatternsListModel(*tileset, this);
  setModel(this->model);
}