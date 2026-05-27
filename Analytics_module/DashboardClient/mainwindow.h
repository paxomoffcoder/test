#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QJsonDocument>

#include "networkclient.h"

class QComboBox;
class QPushButton;
class QLabel;
class QTableView;
class QtCharts::QChartView;
class JsonTableModel;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(const QString& baseUrl, QWidget* parent = nullptr);

private slots:
    void onRefreshClicked();
    void onMetricChanged();
    void onRequestSucceeded(const QJsonDocument& payload);
    void onRequestFailed(const QString& message);

private:
    void setupUi();
    void updateGroupByEnabled();
    void renderPayload(const QJsonDocument& payload);
    void renderArrayPayload(const QJsonArray& array);
    void renderObjectPayload(const QJsonObject& object);

    NetworkClient* m_client;
    JsonTableModel* m_tableModel;

    QComboBox* m_metricCombo;
    QComboBox* m_periodCombo;
    QComboBox* m_groupByCombo;
    QPushButton* m_refreshButton;
    QLabel* m_statusLabel;
    QTableView* m_tableView;
    QtCharts::QChartView* m_chartView;
};

#endif // MAINWINDOW_H
