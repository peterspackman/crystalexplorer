#include "taskmanagerwidget.h"
#include <QMessageBox>
#include <QHeaderView>
#include <QProgressBar>
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
    m_taskTable->setColumnCount(3); // Two columns: task description and progress
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

const QIcon& TaskManagerWidget::successIcon() const { return m_successIcon; }
void TaskManagerWidget::setSuccessIcon(const QIcon &icon) { m_successIcon = icon; }
const QIcon& TaskManagerWidget::failureIcon() const { return m_failureIcon; }
void TaskManagerWidget::setFailureIcon(const QIcon &icon) { m_failureIcon = icon; }

