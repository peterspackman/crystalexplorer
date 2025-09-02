#pragma once
#include "pairinteractiontablemodel.h"
#include "scene.h"
#include <QWidget>

class QTabWidget;
class QLabel;
class QTableView;
class QShowEvent;
class QTextBrowser;

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
  void forceUpdate();

signals:
  void currentModelChanged(const QString &modelName);

protected:
  void showEvent(QShowEvent *event) override;

private slots:
  void onTabChanged(int index);

private:
  void updateContent();
  void showNoDataMessage();
  void setupTableForModel(const QString &model);
  void setupCopyAction();
  QString generateCitationHtml(const QString &model);
  QWidget* createTableWithCitations(const QString &model);

  void showHeaderContextMenu(const QPoint &pos);
  QMenu *m_headerContextMenu{nullptr};

  QAction *m_copyAction{nullptr};
  Scene *m_scene{nullptr};
  QTabWidget *m_tabWidget{nullptr};
  QLabel *m_noDataLabel{nullptr};
  QTextBrowser *m_citationBrowser{nullptr};
  InteractionInfoSettings m_settings;
  QHash<QString, PairInteractionTableModel *> m_models;
  QHash<QString, QTableView *> m_views;
  QHash<QString, QTextBrowser *> m_citationBrowsers;
};
