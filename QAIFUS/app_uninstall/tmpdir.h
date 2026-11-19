#ifndef TMPDIR_H
#define TMPDIR_H

#define TMP_NAME "gradgrind_AppUnin"

#include <QString>
#include <QDir>

QString get_tmp_path() {
    return QDir::tempPath();
}

#endif // TMPDIR_H
