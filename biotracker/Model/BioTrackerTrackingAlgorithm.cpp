#include "BioTrackerTrackingAlgorithm.h"

BioTrackerTrackingAlgorithm::BioTrackerTrackingAlgorithm(QObject *parent, ITrackedComponentFactory *factory) :
    ITrackingAlgorithm(parent)
{
    setTrackedComponentFactory(factory);
}

void BioTrackerTrackingAlgorithm::doTracking(cv::Mat image)
{
    cv::Mat mat;
    cv::applyColorMap(image, mat, cv::COLORMAP_JET);

    Q_EMIT emitCvMatA(mat, QString("tracker"));

//    QString str;
//    str.resize(6);
//    for (int s = 0; s < 6 ; ++s) {
//        str[s] = QChar('A' + char(qrand() % ('Z' - 'A')));
//    }

//    m_TrackedElement = dynamic_cast<TrackedElement *>(m_TrackedComponentFactory->getNewTrackedElement());
//    m_TrackedElement->setObjectName(str);


}