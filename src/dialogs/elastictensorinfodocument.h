#pragma once
#include "elastic_tensor_results.h"
#include <QWidget>
#include <QTextEdit>
#include <QTextCursor>
#include <QPushButton>

class Scene;

class ElasticTensorInfoDocument : public QWidget {
    Q_OBJECT

public:
    explicit ElasticTensorInfoDocument(QWidget *parent = nullptr);
    void updateScene(Scene *scene);

public slots:
    void updateElasticTensor(ElasticTensorResults *tensor);
    void forceUpdate();

signals:
    void calculateElasticTensorRequested(const QString &modelName, double cutoffRadius);

private slots:
    void onCalculateButtonClicked();

private:
    void setupUI();
    void populateDocument();
    void resetCursorToBeginning();
    void updateButtonState();

    void insertTensorMatrices(QTextCursor &cursor, ElasticTensorResults *tensor);
    void insertAverageProperties(QTextCursor &cursor, ElasticTensorResults *tensor);
    void insertEigenvalues(QTextCursor &cursor, ElasticTensorResults *tensor);
    void insertExtremaAndDirections(QTextCursor &cursor, ElasticTensorResults *tensor);

    Scene *m_scene{nullptr};
    ElasticTensorResults *m_currentTensor{nullptr};

    QPushButton *m_calculateButton{nullptr};
    QTextEdit *m_contents{nullptr};
};