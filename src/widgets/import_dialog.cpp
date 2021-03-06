/*
 * Copyright (C) 2014-2018 Christopho, Solarus - http://www.solarus-games.org
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
#include "widgets/gui_tools.h"
#include "widgets/import_dialog.h"
#include "editor_exception.h"
#include "editor_settings.h"
#include "file_tools.h"
#include <QFileDialog>
#include <QTimer>

namespace SolarusEditor {

/**
 * @brief Creates an import dialog.
 * @param destination_quest The destination quest to import files to.
 * It must exist.
 * @param parent Parent object or nullptr.
 */
ImportDialog::ImportDialog(Quest& destination_quest, QWidget* parent) :
  QDialog(parent),
  ui(),
  source_quest(),
  destination_quest(destination_quest),
  last_confirm_overwrite_file(QMessageBox::No) {

  ui.setupUi(this);

  Q_ASSERT(destination_quest.exists());

  ui.source_quest_tree_view->set_read_only(true);
  ui.source_quest_tree_view->set_opening_files_allowed(false);

  ui.destination_quest_field->setText(destination_quest.get_root_path());
  ui.destination_quest_tree_view->set_quest(destination_quest);
  ui.destination_quest_tree_view->set_opening_files_allowed(false);

  ui.missing_files_count_label->clear();

  connect(ui.source_quest_browse_button, SIGNAL(clicked(bool)),
          this, SLOT(browse_source_quest()));
  connect(&source_quest, SIGNAL(root_path_changed(QString)),
          this, SLOT(source_quest_root_path_changed()));
  connect(ui.source_quest_tree_view, SIGNAL(selected_path_changed(QString)),
          this, SLOT(source_quest_selected_path_changed()));
  connect(ui.destination_quest_tree_view, SIGNAL(rename_file_requested(Quest&, QString)),
          this, SIGNAL(destination_quest_rename_file_requested(Quest&, QString)));
  connect(ui.find_missing_button, SIGNAL(clicked(bool)),
          this, SLOT(find_missing_button_triggered()));
  connect(ui.import_button, SIGNAL(clicked(bool)),
          this, SLOT(import_button_triggered()));

  EditorSettings settings;
  QString last_source_quest_path = settings.get_value_string(EditorSettings::import_last_source_quest);
  source_quest.set_root_path(last_source_quest_path);
  if (!source_quest.exists()) {
    browse_source_quest();
  }
}

/**
 * @brief Returns the source quest where to import files from.
 * @return The source quest or an invalid quest if it was not set.
 */
const Quest& ImportDialog::get_source_quest() const {
  return source_quest;
}

/**
 * @brief Returns the destination quest where to import files to.
 * @return The destination quest.
 */
Quest& ImportDialog::get_destination_quest() const {
  return destination_quest;
}

/**
 * @brief Lets the user choose the source quest where to import from.
 */
void ImportDialog::browse_source_quest() {

  // Ask the quest path where to import from.
  EditorSettings settings;
  QString initial_value = settings.get_value_string(EditorSettings::import_last_source_quest);
  QString src_quest_path = QFileDialog::getExistingDirectory(
        this,
        tr("Select a quest where to import from"),
        initial_value,
        QFileDialog::ShowDirsOnly
  );

  if (src_quest_path.isEmpty()) {
    // Canceled.
    return;
  }

  if (src_quest_path == destination_quest.get_root_path()) {
    // Same quest.
    GuiTools::warning_dialog(tr("Source and destination quest are the same"));
    return;
  }

  source_quest.set_root_path(src_quest_path);
  if (!source_quest.exists()) {
    GuiTools::error_dialog(
          tr("No source quest was not found in folder '%1'").arg(src_quest_path));
    source_quest.set_root_path("");
  }
}

/**
 * @brief Slot called when the root path of the source quest has changed.
 */
void ImportDialog::source_quest_root_path_changed() {

  ui.source_quest_browse_field->setText(source_quest.get_root_path());
  ui.source_quest_tree_view->set_quest(source_quest);

  update_import_button();
  if (source_quest.exists()) {
    EditorSettings settings;
    settings.set_value(EditorSettings::import_last_source_quest, source_quest.get_root_path());
  }
}

/**
 * @brief Slot called when the selection has changed in the source quest.
 */
void ImportDialog::source_quest_selected_path_changed() {

  update_find_missing_button();
  update_import_button();
}

/**
 * @brief Updates the enabled state of the "Find missing" button.
 */
void ImportDialog::update_find_missing_button() {

  ui.find_missing_button->setEnabled(false);
  if (!source_quest.exists()) {
    return;
  }

  QString selected_path = ui.source_quest_tree_view->get_selected_path();
  if (selected_path.isEmpty() ||
      QFileInfo(selected_path).isDir()
  ) {
    ui.find_missing_button->setEnabled(true);
  }
}

/**
 * @brief Slot called when the user clicks the "Find missing" button.
 */
void ImportDialog::find_missing_button_triggered() {

  if (!source_quest.exists()) {
    return;
  }

  QString selected_source_path = ui.source_quest_tree_view->get_selected_path();
  QString initial_source_path = !selected_source_path.isEmpty() ?
        selected_source_path : source_quest.get_data_path();
  QStringList missing_source_paths;
  ui.source_quest_tree_view->expand_to_path(initial_source_path);  // TODO expand the directory
  find_source_paths_not_in_destination_quest(initial_source_path, missing_source_paths);

  ui.source_quest_tree_view->set_selected_paths(missing_source_paths);
  ui.missing_files_count_label->setText(
        missing_source_paths.isEmpty() ?
          tr("No candidates found") :
          tr("%1 candidates found").arg(missing_source_paths.size())
  );
}

/**
 * @brief Checks if a path and its children exist in the destination quest.
 *
 * Only the existence of files is checked, not their content.
 *
 * @param source_path The source path to check.
 * If it is a directory, it will be recursively explored.
 * @param[out] missing_source_paths List that will be appended with source
 * paths that have no equivalent in the destination quest.
 */
void ImportDialog::find_source_paths_not_in_destination_quest(
    const QString& source_path,
    QStringList& missing_source_paths
) {

  QFileInfo source_info(source_path);
  if (!source_info.exists()) {
    return;
  }
  if (source_info.isSymLink()) {
    return;
  }

  QString destination_path = source_to_destination_path(source_path);
  if (!QFileInfo(destination_path).exists()) {
    // Found a missing one.
    missing_source_paths << source_path;
  }

  if (source_info.isDir()) {
    // Recursively explore the directory.
    const QStringList& source_children_file_names = QDir(source_path).entryList();
    for (const QString& source_child_file_name : source_children_file_names) {
      if (source_child_file_name == "." ||
          source_child_file_name == ".." ) {
        continue;
      }
      QString source_child_path = QString("%1/%2").arg(source_path, source_child_file_name);
      find_source_paths_not_in_destination_quest(source_child_path, missing_source_paths);
    }
  }
}

/**
 * @brief Updates the enabled state of the import button.
 */
void ImportDialog::update_import_button() {

  ui.import_button->setEnabled(ui.source_quest_tree_view->selectionModel()->hasSelection());
}

/**
 * @brief Slot called when the user clicks the "Import" button.
 */
void ImportDialog::import_button_triggered() {

  last_confirm_overwrite_file = QMessageBox::No;
  paths_to_select.clear();
  ui.missing_files_count_label->clear();

  try {
    const QStringList& source_paths = ui.source_quest_tree_view->get_selected_paths();
    for (const QString& source_path : source_paths) {
      import_path(source_path);
    }
  }
  catch (const EditorException& ex) {
    GuiTools::error_dialog(ex.get_message());
  }

  destination_quest.get_database().save();
  ui.destination_quest_tree_view->setFocus();
  QTimer::singleShot(200, this, SLOT(select_recently_created_paths()));
}

/**
 * @brief Imports the given file or directory into the destination quest.
 * @param source_path Path or the file or directory to import.
 */
void ImportDialog::import_path(const QString& source_path) {

  QFileInfo source_info(source_path);
  if (source_info.isSymLink()) {
    throw EditorException(tr("Cannot import symbolic link '%1'").arg(source_path));
  }

  if (source_info.isDir()) {
    import_dir(source_info);
  }
  else {
    import_file(source_info);
  }
}

/**
 * @brief Imports the given file into the destination quest.
 * @param source_file File to import.
 */
void ImportDialog::import_file(const QFileInfo& source_info) {

  const QString& source_path = source_info.filePath();
  if (!source_info.exists()) {
    throw EditorException(QApplication::tr("Source file does not exist: '%1'").arg(source_path));
  }

  if (source_info.isDir()) {
    throw EditorException(QApplication::tr("Source path is a folder: '%1'").arg(source_path));
  }

  if (source_info.isSymLink()) {
    throw EditorException(QApplication::tr("Source file is a symbolic link: '%1'").arg(source_path));
  }

  if (!source_info.isReadable()) {
    throw EditorException(QApplication::tr("Source file cannot be read: '%1'").arg(source_path));
  }

  QString destination_path = source_to_destination_path(source_path);
  QFileInfo destination_info(destination_path);
  if (destination_info.exists()) {
    if (destination_info.isDir()) {
      throw EditorException(tr("Destination path already exists and is a folder: '%1'").arg(destination_path));
    }

    if (last_confirm_overwrite_file == QMessageBox::NoToAll) {
      return;
    }

    if (last_confirm_overwrite_file != QMessageBox::YesToAll) {
      last_confirm_overwrite_file = QMessageBox::question(
            this,
            tr("Destination file already exists"),
            tr("The destination file '%1' already exists.\nDo you want to overwrite it?").arg(destination_path),
            QMessageBox::Yes | QMessageBox::YesToAll | QMessageBox::No | QMessageBox::NoToAll | QMessageBox::Cancel
      );

      if (last_confirm_overwrite_file != QMessageBox::Yes &&
          last_confirm_overwrite_file != QMessageBox::YesToAll) {
        return;
      }
    }

    if (!QFile::remove(destination_path)) {
      throw EditorException(tr("Failed to remove existing file '%1'").arg(destination_path));
    }
  }

  // Copy author and license info.
  import_path_meta_information(source_path, destination_path);

  // Ensure that the destination directory exists.
  QString destination_parent_path = destination_info.path();  // Strip the file name part.
  FileTools::create_directories(destination_parent_path);

  if (!QFile::copy(source_path, destination_path)) {
    throw EditorException(tr("Failed to copy file '%1' to '%2'").arg(source_path, destination_path));
  }

  // Handle declared resources.
  ResourceType resource_type;
  QString element_id;
  if (source_quest.is_resource_element(source_path, resource_type, element_id)) {
    const QString& description = source_quest.get_database().get_description(resource_type, element_id);
    QuestDatabase& destination_database = destination_quest.get_database();
    destination_database.add(resource_type, element_id, description);
  }

  paths_to_select << destination_path;
}

/**
 * @brief Imports the given directory into the destination quest.
 * @param source_dir The directory to import.
 */
void ImportDialog::import_dir(const QFileInfo& source_info) {

  const QString& source_path = source_info.filePath();
  if (!source_info.exists()) {
    throw EditorException(QApplication::tr("Source folder does not exist: '%1'").arg(source_path));
  }

  if (!source_info.isDir()) {
    throw EditorException(QApplication::tr("Source path is not a folder: '%1'").arg(source_path));
  }

  if (!source_info.isReadable()) {
    throw EditorException(QApplication::tr("Source folder cannot be read: '%1'").arg(source_path));
  }

  QString destination_path = source_to_destination_path(source_path);
  QFileInfo destination_info(destination_path);
  if (destination_info.exists()) {
    if (!destination_info.isDir()) {
      throw EditorException(tr("Destination path already exists and is not a directory: '%1'").arg(destination_path));
    }

    ui.destination_quest_tree_view->expand_to_path(destination_path);

    if (last_confirm_overwrite_directory == QMessageBox::NoToAll) {
      return;
    }

    if (last_confirm_overwrite_directory != QMessageBox::YesToAll) {
      last_confirm_overwrite_directory = QMessageBox::question(
            this,
            tr("Destination folder already exists"),
            tr("The destination folder '%1' already exists.\nDo you want to merge it with the contents from the source folder?").arg(destination_path),
            QMessageBox::Yes | QMessageBox::YesToAll | QMessageBox::No | QMessageBox::NoToAll | QMessageBox::Cancel
      );

      if (last_confirm_overwrite_directory != QMessageBox::Yes &&
          last_confirm_overwrite_directory != QMessageBox::YesToAll) {
        return;
      }
    }
  }

  // Copy author and license info.
  import_path_meta_information(source_path, destination_path);

  // Create the directory itself.
  FileTools::create_directories(destination_path);

  // Copy children.
  const QStringList& source_children_file_names = QDir(source_path).entryList();
  for (const QString& source_child_file_name : source_children_file_names) {
    if (source_child_file_name == "." ||
        source_child_file_name == ".." ) {
      continue;
    }
    QString source_child_path = QString("%1/%2").arg(source_path, source_child_file_name);
    if (!QFileInfo(source_child_path).exists()) {
      // Path in the tree but not on the filesystem:
      // maybe a declared resource that is missing.
      continue;
    }
    import_path(source_child_path);
  }

  paths_to_select << destination_path;
}

/**
 * @brief Copies the author and license information from a file to another one.
 * @param source_path File or directory in the source quest.
 * @param destination_path File or directory in the destination quest.
 */
void ImportDialog::import_path_meta_information(
    const QString& source_path, const QString& destination_path) {

  QString source_relative_path = source_quest.get_path_relative_to_data_path(source_path);
  const QuestDatabase& source_database = source_quest.get_database();
  QString destination_relative_path = destination_quest.get_path_relative_to_data_path(destination_path);
  QuestDatabase& destination_database = destination_quest.get_database();
  destination_database.set_file_author(destination_relative_path,
                                       source_database.get_file_author(source_relative_path));
  destination_database.set_file_license(destination_relative_path,
                                        source_database.get_file_license(destination_relative_path));
}

/**
 * @brief Returns an equivalent path in the destination quest
 * from a source path.
 * @param source_path Path of a file or directory in the source quest.
 * @return The same path with the source quest replaced by the destination one.
 */
QString ImportDialog::source_to_destination_path(const QString& source_path) {

  QString relative_path = source_path.right(source_path.size() - source_quest.get_root_path().size());
  return destination_quest.get_root_path() + relative_path;
}

/**
 * @brief Selects paths that were just imported in the destination quest.
 */
void ImportDialog::select_recently_created_paths() {

  ui.destination_quest_tree_view->set_selected_paths(paths_to_select);
  for (const QString& path : paths_to_select) {
    ui.destination_quest_tree_view->expand_to_path(path);
  }
}

}
