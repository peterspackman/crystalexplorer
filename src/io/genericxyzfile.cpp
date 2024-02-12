#include "genericxyzfile.h"
#include <QTextStream>
#include <QDebug>


const GenericXYZFile::VectorType &GenericXYZFile::column(const QString &name) const {
    auto loc = m_columns.find(name);
    if(loc == m_columns.end()) return m_emptyColumn;
    return loc->second;
}

bool GenericXYZFile::readFromFile(const QString &fileName) {
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
	return false;

    QTextStream in(&file);
    QString content = in.readAll();
    file.close();

    return readFromString(content);
}

bool GenericXYZFile::readFromString(const QString &content) {
    QString contentCopy = content;
    QTextStream stream(&contentCopy, QIODevice::ReadOnly);

    bool ok{false};
    int lineCount{0};

    QString line = stream.readLine();
    if (!line.isNull()) {
	lineCount = line.toInt(&ok);
	if (!ok || lineCount <= 0) {
	    m_error = "Invalid number of entries";
	    return false;
	}
    } else {
	m_error = "Invalid format or empty file";
	return false;
    }

    // Read column names and initlialize columns
    if (stream.atEnd()) {
	m_error = "Unexpected end of file at line 1";
	return false;
    }

    line = stream.readLine();
    auto columnNames = line.split(' ', Qt::SkipEmptyParts);

    if(columnNames.size() < 1) {
	m_error = "Expected at least 1 column name on line 2";
	return false;
    }
    for (const QString &columnName : columnNames) {
	m_columns[columnName] = VectorType(lineCount);
    }

    int n = 0;
    while (!stream.atEnd()) {
	line = stream.readLine();
	QStringList values = line.split(' ', Qt::SkipEmptyParts);

	if (values.size() != columnNames.size()) {
	    m_error = QString("Invalid number of columns on line %1 found: %2 expected: %3")
		.arg(n + 2)
		.arg(values.size())
		.arg(columnNames.size());
	    return false;
	}

	for (int i = 0; i < values.size(); ++i) {
	    m_columns[columnNames[i]](n) = values[i].toFloat();
	}
	n++;
    }

    return true;
}

QString GenericXYZFile::getErrorString() const {
    return m_error;
}

QStringList GenericXYZFile::columnNames() const {
    QStringList result;
    for(const auto &kv: m_columns) {
	result.append(kv.first);
    }
    return result;
}
