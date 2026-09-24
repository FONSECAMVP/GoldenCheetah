/*
 * Coach Module - Simple test entry point
 * This file is for testing the Coach module independently
 * In production, this is not used - Coach is loaded as a plugin
 */

#include <QApplication>
#include <QMainWindow>
#include "CoachChatWidget.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // Set application metadata
    app.setApplicationName("GoldenCheetah Coach");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("GoldenCheetah");

    // Create main window
    QMainWindow mainWindow;
    mainWindow.setWindowTitle("GoldenCheetah AI Coach");
    mainWindow.resize(800, 600);

    // Create and add the Coach Chat Widget
    // Note: In production, this would be instantiated with a real Context
    // For testing, we'd need to mock the Context
    // CoachChatWidget* coachWidget = new CoachChatWidget(nullptr);
    // mainWindow.setCentralWidget(coachWidget);

    mainWindow.show();

    return app.exec();
}
