#pragma once

#include <QElapsedTimer>
#include <QString>
#include <QDebug>
#include <QMap>
#include <vector>

// Define CX_ENABLE_PERFORMANCE_TIMING in CMake or as a compile flag to enable timing
// e.g., cmake -DCX_ENABLE_PERFORMANCE_TIMING=ON or add -DCX_ENABLE_PERFORMANCE_TIMING to compiler flags

class PerformanceTimer {
public:
    struct TimingData {
        QString name;
        qint64 duration_ns;
        double duration_ms() const { return duration_ns / 1000000.0; }
    };

    static PerformanceTimer& instance() {
        static PerformanceTimer timer;
        return timer;
    }

    void startTiming(const QString &name) {
        m_currentTimings[name].start();
    }

    void endTiming(const QString &name) {
        if (!m_currentTimings.contains(name)) {
            qWarning() << "PerformanceTimer: No start timing found for" << name;
            return;
        }
        
        qint64 elapsed = m_currentTimings[name].nsecsElapsed();
        m_frameTimings.push_back({name, elapsed});
        m_currentTimings.remove(name);
        
        // Keep running average for frequently measured timings
        if (m_averages.contains(name)) {
            m_averages[name] = (m_averages[name] * 0.9) + (elapsed / 1000000.0 * 0.1);
        } else {
            m_averages[name] = elapsed / 1000000.0;
        }
    }

    void startFrame() {
        m_frameTimings.clear();
        m_frameTimer.start();
    }

    void endFrame() {
        qint64 totalFrame = m_frameTimer.nsecsElapsed();
        m_frameTimings.push_back({"Total Frame", totalFrame});
        
        if (m_enabledOutput && m_frameCount % m_outputFrequency == 0) {
            printFrameTimings();
        }
        m_frameCount++;
    }

    void printFrameTimings() const {
        qDebug() << "=== Frame" << m_frameCount << "Performance ===";
        for (const auto &timing : m_frameTimings) {
            qDebug() << QString("%1: %2 ms").arg(timing.name, -20).arg(timing.duration_ms(), 6, 'f', 3);
        }
        
        qDebug() << "=== Running Averages ===";
        for (auto it = m_averages.constBegin(); it != m_averages.constEnd(); ++it) {
            qDebug() << QString("%1: %2 ms").arg(it.key(), -20).arg(it.value(), 6, 'f', 3);
        }
        qDebug() << "";
    }

    void setEnabled(bool enabled) { m_enabledOutput = enabled; }
    void setOutputFrequency(int frames) { m_outputFrequency = frames; }
    
    const std::vector<TimingData>& getLastFrameTimings() const { return m_frameTimings; }
    const QMap<QString, double>& getAverages() const { return m_averages; }

private:
    PerformanceTimer() = default;
    
    QMap<QString, QElapsedTimer> m_currentTimings;
    std::vector<TimingData> m_frameTimings;
    QMap<QString, double> m_averages;
    QElapsedTimer m_frameTimer;
    
    bool m_enabledOutput{false};
    int m_outputFrequency{60}; // Print every 60 frames
    int m_frameCount{0};
};

// RAII helper for automatic timing
class ScopedTimer {
public:
    explicit ScopedTimer(const QString &name) : m_name(name) {
        PerformanceTimer::instance().startTiming(m_name);
    }
    
    ~ScopedTimer() {
        PerformanceTimer::instance().endTiming(m_name);
    }

private:
    QString m_name;
};

// Convenience macros
#ifdef CX_ENABLE_PERFORMANCE_TIMING
  #define PERF_TIMER_START(name) PerformanceTimer::instance().startTiming(name)
  #define PERF_TIMER_END(name) PerformanceTimer::instance().endTiming(name)
  #define PERF_SCOPED_TIMER(name) ScopedTimer _timer(name)
  #define PERF_FRAME_START() PerformanceTimer::instance().startFrame()
  #define PERF_FRAME_END() PerformanceTimer::instance().endFrame()
  #define PERF_TIMER_SET_ENABLED(enabled) PerformanceTimer::instance().setEnabled(enabled)
  #define PERF_TIMER_SET_FREQUENCY(frames) PerformanceTimer::instance().setOutputFrequency(frames)
#else
  // No-op macros when performance timing is disabled
  #define PERF_TIMER_START(name) ((void)0)
  #define PERF_TIMER_END(name) ((void)0)
  #define PERF_SCOPED_TIMER(name) ((void)0)
  #define PERF_FRAME_START() ((void)0)
  #define PERF_FRAME_END() ((void)0)
  #define PERF_TIMER_SET_ENABLED(enabled) ((void)0)
  #define PERF_TIMER_SET_FREQUENCY(frames) ((void)0)
#endif