QT += testlib core widgets

SOURCES = testCoachTools.cpp

# Static methods under test (no Context dependency at runtime)
GC_OBJS = Coach/GCToolExecutor Coach/ToolConfirmCard Coach/PlanPreviewCard

include(../../unittests.pri)
