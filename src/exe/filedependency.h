#pragma once
#include <QString>
#include <QList>

struct FileDependency {
    FileDependency(const QString &src) : source(src), dest(src) {}
    FileDependency(const QString &src, const QString &dest) : source(src), dest(dest) {}
    QString source;
    QString dest;
};

using FileDependencyList = QList<FileDependency>;
