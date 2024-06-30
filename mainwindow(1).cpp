#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QPainter>
#include <QtMath>
#include <QPixmap>
#include <QScriptEngine>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_pushButton_clicked()
{
    QString function = ui->lineEdit->text();
    bool ok_min, ok_max;
    double x_min = ui->lineEdit_xMin->text().toDouble(&ok_min);
    double x_max = ui->lineEdit_xMax->text().toDouble(&ok_max);

    if (!ok_min || !ok_max) {
        QMessageBox::warning(this, "Input Error", "Please enter valid x_min and x_max values.");
        return;
    }

    plotFunction(function, x_min, x_max);
}

// Static functions for math operations
static QScriptValue scriptSin(QScriptContext *context, QScriptEngine *engine)
{
    return qSin(context->argument(0).toNumber());
}

static QScriptValue scriptCos(QScriptContext *context, QScriptEngine *engine)
{
    return qCos(context->argument(0).toNumber());
}

static QScriptValue scriptTan(QScriptContext *context, QScriptEngine *engine)
{
    return qTan(context->argument(0).toNumber());
}

static QScriptValue scriptLog(QScriptContext *context, QScriptEngine *engine)
{
    return qLn(context->argument(0).toNumber());
}

static QScriptValue scriptExp(QScriptContext *context, QScriptEngine *engine)
{
    return qExp(context->argument(0).toNumber());
}

void MainWindow::plotFunction(const QString &function, double x_min, double x_max)
{
    QPixmap pixmap(ui->plotLabel->size());
    pixmap.fill(Qt::white);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);

    QPen pen(Qt::black);
    pen.setWidth(2);
    painter.setPen(pen);

    int width = ui->plotLabel->width();
    int height = ui->plotLabel->height();

    // Define the step based on the number of pixels
    double step = (x_max - x_min) / width;

    // Draw axes with a suitable scaling factor
    double y_min = std::numeric_limits<double>::infinity();
    double y_max = -std::numeric_limits<double>::infinity();

    // Parse and evaluate the function
    QScriptEngine engine;

    // Register math functions
    QScriptValue math = engine.newObject();
    math.setProperty("sin", engine.newFunction(scriptSin));
    math.setProperty("cos", engine.newFunction(scriptCos));
    math.setProperty("tan", engine.newFunction(scriptTan));
    math.setProperty("log", engine.newFunction(scriptLog));
    math.setProperty("exp", engine.newFunction(scriptExp));
    engine.globalObject().setProperty("math", math);

    for (double x = x_min; x <= x_max; x += step)
    {
        engine.globalObject().setProperty("x", x);
        QScriptValue result = engine.evaluate("with(math) { " + function + " }");
        if (result.isNumber()) {
            double y = result.toNumber();
            if (y < y_min) y_min = y;
            if (y > y_max) y_max = y;
        }
    }

    // Limit y range to prevent extreme values from ruining the plot
    double y_limit = 10;
    y_min = qMax(y_min, -y_limit);
    y_max = qMin(y_max, y_limit);
    double y_range = y_max - y_min;
    if (y_range == 0) y_range = 1; // Prevent division by zero

    // Get actual plot area size
    QRect plotRect = ui->plotLabel->rect();

    // Center position for axes
    double x_center = (0 - x_min) / (x_max - x_min) * width;
    double y_center = height - (0 - y_min) / y_range * height;

    // Draw the axes
    painter.drawLine(plotRect.left(), y_center, plotRect.right(), y_center); // x-axis
    painter.drawLine(x_center, plotRect.top(), x_center, plotRect.bottom()); // y-axis

    // Draw ticks on x-axis
    for (int i = x_min; i <= x_max; i++) {
        int x_pos = (i - x_min) / (x_max - x_min) * width;
        painter.drawLine(x_pos, y_center - 5, x_pos, y_center + 5); // x-axis ticks
        painter.drawText(x_pos - 10, y_center + 20, QString::number(i));
    }

    // Draw ticks on y-axis
    for (int i = qCeil(y_min); i <= qFloor(y_max); i++) {
        int y_pos = height - (i - y_min) / y_range * height;
        painter.drawLine(x_center - 5, y_pos, x_center + 5, y_pos); // y-axis ticks
        painter.drawText(x_center + 10, y_pos + 5, QString::number(i));
    }

    // Plot the function
    QPolygonF points;
    for (double x = x_min; x <= x_max; x += step)
    {
        engine.globalObject().setProperty("x", x);
        QScriptValue result = engine.evaluate("with(math) { " + function + " }");
        if (result.isNumber()) {
            double y = result.toNumber();
            if (y > y_limit) y = y_limit;
            if (y < -y_limit) y = -y_limit;
            points << QPointF((x - x_min) / (x_max - x_min) * width,
                              height - (y - y_min) / y_range * height);
        }
    }

    painter.drawPolyline(points);

    ui->plotLabel->setPixmap(pixmap);
}
