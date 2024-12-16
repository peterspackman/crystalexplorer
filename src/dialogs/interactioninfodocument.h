#pragma once

#include "scene.h"
#include <QWidget>

class QTabWidget;
class QLabel;
class PairInteractions;
class QTextCursor;
class QShowEvent;

struct InteractionInfoSettings {
  QString colorScheme{"Viridis"};
  int distancePrecision{2};
  int energyPrecision{1};
};

class InteractionInfoDocument : public QWidget {
  Q_OBJECT

public:
  explicit InteractionInfoDocument(QWidget *parent = nullptr);
  void updateScene(Scene *scene);

public slots:
  void updateSettings(InteractionInfoSettings);

signals:
  void currentModelChanged(const QString &modelName);

protected:
  void showEvent(QShowEvent *event) override;

private slots:
  void onTabChanged(int index);

private:
  void updateContent();
  void showNoDataMessage();
  void insertInteractionEnergiesForModel(PairInteractions *results,
                                         QTextCursor &cursor,
                                         const QString &model);
  QList<QString> getOrderedComponents(const QSet<QString> &uniqueComponents);
  void updateTableColors();

  Scene *m_scene{nullptr};
  QTabWidget *m_tabWidget;
  QLabel *m_noDataLabel;
  InteractionInfoSettings m_settings;
};
