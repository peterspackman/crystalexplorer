#pragma once
#include "elastic_tensor_results.h"
#include <QWidget>
#include <QTextEdit>
#include <QTextCursor>

class Scene;

class ElasticTensorInfoDocument : public QWidget {
    Q_OBJECT
    
public:
    explicit ElasticTensorInfoDocument(QWidget *parent = nullptr);
    void updateScene(Scene *scene);
    
public slots:
    void updateElasticTensor(ElasticTensorResults *tensor);
    void forceUpdate();
    
private:
    void setupUI();
    void populateDocument();
    void resetCursorToBeginning();
    
    void insertTensorMatrices(QTextCursor &cursor, ElasticTensorResults *tensor);
    void insertAverageProperties(QTextCursor &cursor, ElasticTensorResults *tensor);
    void insertEigenvalues(QTextCursor &cursor, ElasticTensorResults *tensor);
    void insertExtremaAndDirections(QTextCursor &cursor, ElasticTensorResults *tensor);
    
    Scene *m_scene{nullptr};
    ElasticTensorResults *m_currentTensor{nullptr};
    
    QTextEdit *m_contents{nullptr};
};