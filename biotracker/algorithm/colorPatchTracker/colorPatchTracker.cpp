#include "colorPatchTracker.h"

#include "biotracker/tracking/algorithm/algorithms.h"

namespace {
auto _ = Algorithms::Registry::getInstance().register_tracker_type<ColorPatchTracker>("Color Patch Tracker");
}

ColorPatchTracker::ColorPatchTracker(Settings &settings, QWidget *parent)
    : TrackingAlgorithm(settings, parent) {
}


ColorPatchTracker::~ColorPatchTracker(void) {
}
void ColorPatchTracker::track(ulong, const cv::Mat &) {}
void ColorPatchTracker::paint(ProxyPaintObject &, const View &) {}
void ColorPatchTracker::reset() {}