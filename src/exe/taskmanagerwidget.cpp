#include "taskmanagerwidget.h"
#include <QMessageBox>
#include <QHeaderView>
#include <QProgressBar>
#include <QTreeWidget>
#include <QMenu>
#include <QTextEdit>
#include <QFontDatabase>
#include "mocktask.h"


QTableWidgetItem * makeItem(const QString &text="") {
    auto * item = new QTableWidgetItem(text);
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    return item;
}


QString taskName(TaskID taskId, Task *task) {
    QString name;
    if(task) name = task->property("name").toString();
    if(name.isEmpty()) name = taskId.toString();
    return name;
}

TaskManagerWidget::TaskManagerWidget(TaskManager *manager, QWidget *parent) : QWidget(parent), m_taskManager(manager) {
    setupUi();
    connectSignals();
}

TaskManagerWidget::TaskManagerWidget(QWidget *parent) : QWidget(parent), m_taskManager(new TaskManager(this)) {
    setupUi();
    connectSignals();
}

void TaskManagerWidget::setupUi() {
    m_successIcon = style()->standardIcon(QStyle::SP_DialogYesButton);
    m_failureIcon = style()->standardIcon(QStyle::SP_DialogNoButton);
    auto *layout = new QVBoxLayout(this);
    m_taskTable = new QTableWidget(this);
    m_taskTable->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_taskTable, &QWidget::customContextMenuRequested, this, &TaskManagerWidget::contextMenu);
    m_taskTable->setColumnCount(3); // Two columns: task description and progress
    m_taskTable->setColumnWidth(0, 300); // Adjust as needed
    m_taskTable->setColumnWidth(1, 300); // Adjust as needed
    m_taskTable->setHorizontalHeaderLabels(QStringList() << "Task ID" << "Description" << "Progress");
    m_taskTable->verticalHeader()->setVisible(false);
    m_taskTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_taskTable->setShowGrid(false);
    m_taskTable->setSelectionBehavior(QAbstractItemView::SelectRows);

    m_stopTaskButton = new QPushButton(tr("Stop Task"), this);
    m_startTaskButton = new QPushButton(tr("Start Task"), this);

    layout->addWidget(m_taskTable);
    layout->addWidget(m_stopTaskButton);
    layout->addWidget(m_startTaskButton);

    setLayout(layout);
    resize(800, 600);
}

void TaskManagerWidget::connectSignals() {
    connect(m_taskManager, &TaskManager::taskComplete, this, &TaskManagerWidget::onTaskComplete);
    connect(m_taskManager, &TaskManager::taskError, this, &TaskManagerWidget::onTaskError);

    connect(m_taskManager, &TaskManager::taskAdded, this, &TaskManagerWidget::onTaskAdded);


    connect(m_stopTaskButton, &QPushButton::clicked, this, &TaskManagerWidget::onStopTaskClicked);
    connect(m_startTaskButton, &QPushButton::clicked, this, [&](){
	Task* mockTask = new MockTask();
	TaskID taskId = m_taskManager->add(mockTask);
    });
}

void TaskManagerWidget::onTaskAdded(TaskID taskId) {
    Task* task = m_taskManager->get(taskId);
    if (task) {
        int row = m_taskTable->rowCount();
	m_rowTasks.append(taskId);
        m_taskTable->insertRow(row);

        m_taskTable->setItem(row, TaskIdColumn,
			     makeItem(taskName(taskId, task)));
        m_taskTable->setItem(row, TaskDescriptionColumn, makeItem("Starting..."));

        QProgressBar *progressBar = new QProgressBar();
        progressBar->setRange(0, 100);
        m_taskTable->setCellWidget(row, TaskProgressColumn, progressBar);

        // Map task ID to row for easy access later
        m_taskItems.insert(taskId, row);

        // Connect task signals to update UI as needed
        connect(task, &Task::progress, this, [this, taskId](int percentage) { onTaskProgress(taskId, percentage); });
        connect(task, &Task::progressText, this, [this, taskId](const QString &desc) { onTaskProgressText(taskId, desc); });
        connect(task, &Task::completed, this, [this, taskId] { onTaskComplete(taskId); });
        connect(task, &Task::errorOccurred, this, [this, taskId](const QString &error) { onTaskError(taskId, error); });
    }
}

void TaskManagerWidget::onTaskComplete(TaskID taskId) {
    int row = m_taskItems.value(taskId, -1);
    if (row != -1) {
        QTableWidgetItem* item = m_taskTable->item(row, TaskIdColumn);
        if (item) {
	    Task* task = m_taskManager->get(taskId);
            item->setText(taskName(taskId, task));
        }

        QTableWidgetItem* descItem = m_taskTable->item(row, TaskDescriptionColumn);
        if (descItem) {
            descItem->setText("Task finished");
        }

        QProgressBar* progressBar = qobject_cast<QProgressBar*>(m_taskTable->cellWidget(row, 1));
        if (progressBar) {
            progressBar->setValue(100);
        }
	m_taskTable->removeCellWidget(row, TaskProgressColumn);
	QTableWidgetItem* item2 = makeItem();
	item2->setText("Complete");
	item2->setIcon(m_successIcon);
        m_taskTable->setItem(row, TaskProgressColumn, item2);
    }
}

void TaskManagerWidget::onTaskError(TaskID taskId, const QString &error) {
    int row = m_taskItems.value(taskId, -1);
    if (row != -1) {
        QTableWidgetItem* item = m_taskTable->item(row, TaskIdColumn);
        if (item) {
	    Task* task = m_taskManager->get(taskId);
            item->setText(taskName(taskId, task));
        }

	QTableWidgetItem* descItem = m_taskTable->item(row, TaskDescriptionColumn);
        if (descItem) {
            descItem->setText(error);
        }

	m_taskTable->removeCellWidget(row, TaskProgressColumn);

	QTableWidgetItem* item2 = makeItem();
	item2->setText("Failure");
	item2->setIcon(m_failureIcon);
        m_taskTable->setItem(row, TaskProgressColumn, item2);
    }
}


void TaskManagerWidget::onTaskProgress(TaskID taskId, int percentage) {
    int row = m_taskItems.value(taskId, -1);
    if (row != -1) {
        QProgressBar *progressBar = qobject_cast<QProgressBar *>(m_taskTable->cellWidget(row, TaskProgressColumn));
        if (progressBar) {
            progressBar->setValue(percentage);
        }
    }
}

void TaskManagerWidget::onTaskProgressText(TaskID taskId, const QString &desc) {
    // Assuming you want to append or update the text in the first column
    int row = m_taskItems.value(taskId, -1);
    if (row != -1) {
        QTableWidgetItem* item = m_taskTable->item(row, TaskDescriptionColumn);
        if (item) {
            item->setText(desc);
        }
    }
}

void TaskManagerWidget::onStopTaskClicked() {
    int row = m_taskTable->currentRow();
    if (row > -1 && row < m_rowTasks.size()) {
        TaskID taskId = m_rowTasks[row];
        Task* task = m_taskManager->get(taskId);
        if (task) {
            task->stop();
        }
    }
}

const QIcon& TaskManagerWidget::successIcon() const {
    return m_successIcon;
}

void TaskManagerWidget::setSuccessIcon(const QIcon &icon) {
    m_successIcon = icon;
}

const QIcon& TaskManagerWidget::failureIcon() const { 
    return m_failureIcon;
}

void TaskManagerWidget::setFailureIcon(const QIcon &icon) {
    m_failureIcon = icon;
}

void TaskManagerWidget::contextMenu(const QPoint &pos) {
    QModelIndex index = m_taskTable->indexAt(pos);
    if (!index.isValid())
        return;

    QMenu contextMenu;
    
    QAction actionShowProperties("Show Properties", this);
    connect(&actionShowProperties, &QAction::triggered, [this, index]() {
        // Implement showing properties for the item in row index.row()
        showPropertiesForRow(index.row());
    });
    
    contextMenu.addAction(&actionShowProperties);
    
    // Show the menu at the position mapped to global from the table view's coordinate system
    contextMenu.exec(m_taskTable->viewport()->mapToGlobal(pos));
}

void TaskManagerWidget::showPropertiesForRow(int row) {
    if (row < 0 || row >= m_rowTasks.size()) return;
    TaskID taskId = m_rowTasks[row];
    Task* task = m_taskManager->get(taskId);
    if(!task) return;

    const auto &properties = task->properties();
    QDialog* dialog = new QDialog;
    dialog->setWindowTitle(QString("%1 Properties").arg(taskName(taskId, task)));

    QTreeWidget* treeWidget = new QTreeWidget(dialog);
    treeWidget->setHeaderLabels(QStringList() << "Property" << "Value");
    treeWidget->setColumnWidth(0, 300); // Adjust as needed
    treeWidget->setWordWrap(true);

    // Define a maximum length for display
    const int maxLength = 72; 

    for (auto it = properties.constBegin(); it != properties.constEnd(); ++it) {
	QTreeWidgetItem* item = new QTreeWidgetItem(treeWidget);
	item->setText(0, it.key());
	QString fullText = it.value().toString();
	QString displayText = fullText;
	if (fullText.length() > maxLength) {
	    displayText = "Double click to show full text...";
	    // Use setData to store the full text in the item, for retrieval on click
	    item->setData(1, Qt::UserRole, QVariant(fullText));
	}
	item->setText(1, displayText);
	item->setTextAlignment(1, Qt::AlignTop); // Ensure alignment for multiline text
    }


    QVBoxLayout* layout = new QVBoxLayout(dialog);
    layout->addWidget(treeWidget);

    // Add a Close button to the dialog
    QPushButton* closeButton = new QPushButton("Close", dialog);
    QObject::connect(closeButton, &QPushButton::clicked, dialog, &QDialog::accept);
    QObject::connect(treeWidget, &QTreeWidget::itemDoubleClicked, this, &TaskManagerWidget::onPropertyItemDoubleClicked);
    layout->addWidget(closeButton);

    dialog->setLayout(layout);
    dialog->resize(800, 600);
    dialog->exec(); // Show the dialog modally
}

void TaskManagerWidget::onPropertyItemDoubleClicked(QTreeWidgetItem* item, int column) {
    if (column != 1) return; // Only handle clicks in the "Value" column
    QVariant fullTextData = item->data(1, Qt::UserRole);
    if (!fullTextData.isValid()) return; // No full text stored for this item

    QString fullText = fullTextData.toString();

    // Create and show the dialog with the full text
    QDialog* textDialog = new QDialog(this);
    textDialog->setWindowTitle(item->text(0)); // Use the property name as the window title
    QVBoxLayout* layout = new QVBoxLayout(textDialog);

    QTextEdit* textEdit = new QTextEdit;
    textEdit->setText(fullText);
    textEdit->setReadOnly(true); // Make sure the text is read-only
    layout->addWidget(textEdit);

    QFont fixedFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    textEdit->setFont(fixedFont);

    // Calculate width for 100 characters
    QFontMetrics metrics(fixedFont);
    int widthFor100Chars = metrics.horizontalAdvance('X') * 100;
    textEdit->setMinimumWidth(widthFor100Chars);


    QPushButton* closeButton = new QPushButton("Close", textDialog);
    QObject::connect(closeButton, &QPushButton::clicked, textDialog, &QDialog::accept);
    layout->addWidget(closeButton);

    textDialog->setLayout(layout);
    textDialog->exec();
}
