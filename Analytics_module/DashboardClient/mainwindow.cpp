#include "mainwindow.h"
#include "jsontablemodel.h"

#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QBarSeries>
#include <QtCharts/QBarSet>

#include <QComboBox>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonObject>
#include <QLabel>
#include <QPushButton>
#include <QSplitter>
#include <QTableView>
#include <QVBoxLayout>
#include <QWidget>

MainWindow::MainWindow(const QString& baseUrl, QWidget* parent)
    : QMainWindow(parent)
    , m_client(new NetworkClient(baseUrl, this))
    , m_tableModel(new JsonTableModel(this))
{
    setupUi();

    connect(m_client, &NetworkClient::requestSucceeded, this, &MainWindow::onRequestSucceeded);
    connect(m_client, &NetworkClient::requestFailed, this, &MainWindow::onRequestFailed);

    onRefreshClicked();
}

void MainWindow::setupUi()
{
    setWindowTitle("Analytics Dashboard Client");
    resize(1000, 650);

    QWidget* central = new QWidget(this);
    QVBoxLayout* root = new QVBoxLayout(central);

    QHBoxLayout* controls = new QHBoxLayout;
    m_metricCombo = new QComboBox(this);
    m_metricCombo->addItems({
        "Summary",
        "Active Users Dynamics",
        "New Registrations",
        "Messages Total",
        "Messages Dynamics",
        "LLM Total",
        "LLM Dynamics",
        "LLM Performance"
    });

    m_periodCombo = new QComboBox(this);
    m_periodCombo->addItems({"day", "week", "month"});

    m_groupByCombo = new QComboBox(this);
    m_groupByCombo->addItems({"hour", "day", "week"});

    m_refreshButton = new QPushButton("Refresh", this);
    m_statusLabel = new QLabel("Ready", this);

    controls->addWidget(new QLabel("Metric:", this));
    controls->addWidget(m_metricCombo);
    controls->addWidget(new QLabel("Period:", this));
    controls->addWidget(m_periodCombo);
    controls->addWidget(new QLabel("Group by:", this));
    controls->addWidget(m_groupByCombo);
    controls->addWidget(m_refreshButton);
    controls->addStretch();

    m_tableView = new QTableView(this);
    m_tableView->setModel(m_tableModel);
    m_tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    m_chartView = new QtCharts::QChartView(new QtCharts::QChart(), this);
    m_chartView->setRenderHint(QPainter::Antialiasing);

    QSplitter* splitter = new QSplitter(Qt::Horizontal, this);
    splitter->addWidget(m_chartView);
    splitter->addWidget(m_tableView);
    splitter->setStretchFactor(0, 2);
    splitter->setStretchFactor(1, 1);

    root->addLayout(controls);
    root->addWidget(splitter);
    root->addWidget(m_statusLabel);

    setCentralWidget(central);

    connect(m_refreshButton, &QPushButton::clicked, this, &MainWindow::onRefreshClicked);
    connect(m_metricCombo, &QComboBox::currentTextChanged, this, &MainWindow::onMetricChanged);

    updateGroupByEnabled();
}

void MainWindow::onMetricChanged()
{
    updateGroupByEnabled();
}

void MainWindow::updateGroupByEnabled()
{
    const QString metric = m_metricCombo->currentText();
    const bool isDynamics = metric.contains("Dynamics");
    m_groupByCombo->setEnabled(isDynamics);
}

void MainWindow::onRefreshClicked()
{
    m_statusLabel->setText("Loading...");

    const QString metric = m_metricCombo->currentText();
    const QString period = m_periodCombo->currentText();
    const QString groupBy = m_groupByCombo->currentText();

    NetworkClient::Metric selected = NetworkClient::Metric::Summary;

    if (metric == "Summary") selected = NetworkClient::Metric::Summary;
    else if (metric == "Active Users Dynamics") selected = NetworkClient::Metric::ActiveUsersDynamics;
    else if (metric == "New Registrations") selected = NetworkClient::Metric::NewRegistrations;
    else if (metric == "Messages Total") selected = NetworkClient::Metric::MessagesTotal;
    else if (metric == "Messages Dynamics") selected = NetworkClient::Metric::MessagesDynamics;
    else if (metric == "LLM Total") selected = NetworkClient::Metric::LlmTotal;
    else if (metric == "LLM Dynamics") selected = NetworkClient::Metric::LlmDynamics;
    else if (metric == "LLM Performance") selected = NetworkClient::Metric::LlmPerformance;

    m_client->fetchMetric(selected, period, groupBy);
}

void MainWindow::onRequestSucceeded(const QJsonDocument& payload)
{
    m_statusLabel->setText("Loaded successfully");
    renderPayload(payload);
}

void MainWindow::onRequestFailed(const QString& message)
{
    m_statusLabel->setText(message);
}

void MainWindow::renderPayload(const QJsonDocument& payload)
{
    if (payload.isArray())
    {
        renderArrayPayload(payload.array());
        return;
    }

    if (payload.isObject())
    {
        renderObjectPayload(payload.object());
    }
}

void MainWindow::renderArrayPayload(const QJsonArray& array)
{
    m_tableModel->setRows(array, {"Bucket", "Count"}, {"bucket", "count"});

    auto* chart = new QtCharts::QChart();
    auto* series = new QtCharts::QBarSeries(chart);
    auto* set = new QtCharts::QBarSet("count", series);

    for (const QJsonValue& value : array)
    {
        const QJsonObject point = value.toObject();
        *set << point.value("count").toDouble();
    }

    series->append(set);
    chart->addSeries(series);
    chart->setTitle("Dynamics");
    chart->createDefaultAxes();

    m_chartView->setChart(chart);
}

void MainWindow::renderObjectPayload(const QJsonObject& object)
{
    QJsonArray rows;
    for (auto it = object.constBegin(); it != object.constEnd(); ++it)
    {
        QJsonObject row;
        row["metric"] = it.key();
        row["value"] = it.value();
        rows.append(row);
    }

    m_tableModel->setRows(rows, {"Metric", "Value"}, {"metric", "value"});

    auto* chart = new QtCharts::QChart();
    auto* series = new QtCharts::QBarSeries(chart);
    auto* set = new QtCharts::QBarSet("value", series);

    for (const QJsonValue& value : rows)
    {
        *set << value.toObject().value("value").toDouble();
    }

    series->append(set);
    chart->addSeries(series);
    chart->setTitle("Metrics");
    chart->createDefaultAxes();

    m_chartView->setChart(chart);
}
