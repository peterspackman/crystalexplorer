#define CATCH_CONFIG_RUNNER
#include <catch2/catch_session.hpp>
#include <QCoreApplication>
#include <QThreadPool>

int main(int argc, char* argv[]) {
    // Create QCoreApplication to provide event loop for Qt signals/slots
    QCoreApplication app(argc, argv);

    // Initialize thread pool
    QThreadPool::globalInstance()->setMaxThreadCount(4);

    // Run Catch2 tests
    int result = Catch::Session().run(argc, argv);

    return result;
}
