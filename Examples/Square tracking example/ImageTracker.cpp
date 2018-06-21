#include "ImageTracker.h"
#include <ARX/Error.h>

//zhenyi
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image/stb_image_write.h"


ImageTracker::ImageTracker()
	:width(0), height(0), n(0), image(NULL), m_videoSource0(NULL), state(NOTHING_INITIALISED), m_updateFrameStamp0({ 0,0 })
{
}


ImageTracker::ImageTracker(std::string imageName)
	: m_videoSource0(NULL), state(NOTHING_INITIALISED), m_updateFrameStamp0({ 0,0 })
{
	image = stbi_load(imageName.c_str(), &width, &height, &n, 0);
	// debug
	stbi_write_png("output.png", width, height, n, image, n * width);
}

ImageTracker::~ImageTracker()
{
	stbi_image_free(image);
}

bool ImageTracker::initialiseBase()
{
	// At present, trackers are hard-coded. In the future, we'll dynamically
	// load trackers as required.
	m_squareTracker = std::shared_ptr<ARTrackerSquare>(new ARTrackerSquare);
	if (!m_squareTracker->initialize()) {
		ARLOGe("Error initialising square marker tracker.\n");
		goto bail;
	}

	state = BASE_INITIALISED;

	ARLOGd("ARX::ARController::initialiseBase() done.\n");
	return true;

bail:
	return false;
}

bool ImageTracker::isInited()
{
	// Check we are in a valid state to add a trackable (i.e. base initialisation has occurred)
	return (state != NOTHING_INITIALISED);
}

bool ImageTracker::startRunning(const char* vconf, const char* cparaName, const char* cparaBuff, const long cparaBuffLen)
{
	ARLOGi("Starting...\n");

	// Check for initialization before starting video
	if (state != BASE_INITIALISED) {
		ARLOGe("Start running called but base not initialised.\n");
		return false;
	}

	m_videoSource0 = new ARVideoSource;
	if (!m_videoSource0) {
		ARLOGe("No video source.\n");
		return false;
	}

	m_videoSource0->configure(vconf, false, cparaName, cparaBuff, cparaBuffLen);

	if (!m_videoSource0->open()) {
		if (m_videoSource0->getError() == ARX_ERROR_DEVICE_UNAVAILABLE) {
			ARLOGe("Video source unavailable.\n");
			//setError(ARX_ERROR_DEVICE_UNAVAILABLE);
		}
		else {
			ARLOGe("Unable to open video source.\n");
		}
		delete m_videoSource0;
		m_videoSource0 = NULL;
		return false;
	}

	
	state = WAITING_FOR_VIDEO;
	

	ARLOGd("ARController::startRunning(): done.\n");
	return true;
}

bool ImageTracker::update()
{
	ARLOGd("ARX::ARController::update().\n");

	if (state != DETECTION_RUNNING) {
		if (state != WAITING_FOR_VIDEO) {
			// State is NOTHING_INITIALISED or BASE_INITIALISED.
			ARLOGe("Update called but not yet started.\n");
			return false;

		}
		else {

			// First check there is a video source and it's open.
			// zhenyi: use static image right now
			if (!image) {
				ARLOGe("No image.\n");
				return false;
			}

			state = DETECTION_RUNNING;
		}
	}

	// Checkout frame(s).
 	AR2VideoBufferT *image0, *image1 = NULL;
 	image0 = m_videoSource0->checkoutFrameIfNewerThan(m_updateFrameStamp0);
	image0->buff = image;
// 	if (!image0) {
// 		return true;
// 	}
//	m_updateFrameStamp0 = image0->time;

	//
	// Tracker updates.
	//

	bool ret = true;

	if (doSquareMarkerDetection) {
		if (!m_squareTracker->isRunning()) {
			ret = m_squareTracker->start(m_videoSource0->getCameraParameters(), m_videoSource0->getPixelFormat());

			if (!ret) goto done;
		}
		m_squareTracker->update(image0, image1, m_trackables);
	}
done:
	// Checkin frames.
//	m_videoSource0->checkinFrame();


	ARLOGd("ARX::ARController::update(): done.\n");

	return ret;
}

bool ImageTracker::stopRunning()
{
	ARLOGd("ARX::ARController::stopRunning()\n");
	if (state != DETECTION_RUNNING && state != WAITING_FOR_VIDEO) {
		ARLOGe("Stop running called but not running.\n");
		return false;
	}

	m_squareTracker->stop();
	if (m_videoSource0) {
		if (m_videoSource0->isOpen()) {
			m_videoSource0->close();
		}
		delete m_videoSource0;
		m_videoSource0 = NULL;
	}


	state = BASE_INITIALISED;

	ARLOGd("ARX::ARController::stopRunning(): done.\n");
	return true;
}

bool ImageTracker::shutdown()
{
	ARLOGd("ARX::ARController::shutdown()\n");
	do {
		switch (state) {
		case DETECTION_RUNNING:
		case WAITING_FOR_VIDEO:
			ARLOGd("ARX::ARController::shutdown(): DETECTION_RUNNING or WAITING_FOR_VIDEO, forcing stop.\n");
			stopRunning();
			break;

		case BASE_INITIALISED:
			if (countTrackables() > 0) {
				ARLOGd("ARX::ARController::shutdown(): BASE_INITIALISED, cleaning up trackables.\n");
				removeAllTrackables();
			}

			m_squareTracker->terminate();
			m_squareTracker.reset();

			state = NOTHING_INITIALISED;
			// Fall though.
		case NOTHING_INITIALISED:
			ARLOGd("ARX::ARController::shutdown(): NOTHING_INITIALISED, complete\n");
			break;
		}
	} while (state != NOTHING_INITIALISED);

	ARLOGi("artoolkitX finished.\n");
	return true;
}

int ImageTracker::addTrackable(const std::string& cfgs)
{
	if (!isInited()) {
		ARLOGe("Error: Cannot add trackable. artoolkitX not initialised\n");
		return -1;
	}

	std::istringstream iss(cfgs);
	std::string token;
	std::vector<std::string> config;
	while (std::getline(iss, token, ';')) {
		config.push_back(token);
	}

	// First token is trackable type. Required.
	if (config.size() < 1) {
		ARLOGe("Error: invalid configuration string. Could not find trackable type.\n");
		return -1;
	}

	// Until we have a registry, have to manually request from all trackers.
	ARTrackable *trackable;
	if ((trackable = m_squareTracker->newTrackable(config)) != nullptr) {
	}
	if (!trackable) {
		ARLOGe("Error: Failed to load trackable.\n");
		return -1;
	}
	if (!addTrackable(trackable)) {
		return -1;
	}
	return trackable->UID;
}

bool ImageTracker::addTrackable(ARTrackable* trackable)
{
	ARLOGd("ARController::addTrackable(): called\n");
	if (!isInited()) {
		ARLOGe("Error: Cannot add trackable. artoolkitX not initialised.\n");
		return false;
	}

	if (!trackable) {
		ARLOGe("ARController::addTrackable(): NULL trackable.\n");
		return false;
	}

	m_trackables.push_back(trackable);

			if (trackable->type == ARTrackable::SINGLE || trackable->type == ARTrackable::MULTI) {
				if (!doSquareMarkerDetection)
					ARLOGi("First square marker trackable added; enabling square marker tracker.\n");
				doSquareMarkerDetection = true;
			}

	ARLOGi("Added trackable (UID=%d), total trackables loaded: %d.\n", trackable->UID, countTrackables());
	return true;
}

int ImageTracker::removeAllTrackables()
{
	unsigned int count = countTrackables();

	for (std::vector<ARTrackable *>::iterator it = m_trackables.begin(); it != m_trackables.end(); ++it) {
		m_squareTracker->deleteTrackable(&(*it));
	}
	m_trackables.clear();
	doSquareMarkerDetection = false;
	ARLOGi("Removed all %d trackables.\n", count);

	return count;
}

unsigned int ImageTracker::countTrackables() const
{
	return ((unsigned int)m_trackables.size());
}

ARTrackable* ImageTracker::findTrackable(int UID)
{
	std::vector<ARTrackable *>::const_iterator it = m_trackables.begin();
	while (it != m_trackables.end()) {
		if ((*it)->UID == UID) return (*it);
		++it;
	}
	return NULL;
}

bool ImageTracker::capture()
{
	// First check there is a video source and it's open.
	if (!m_videoSource0 || !m_videoSource0->isOpen() ) {
		ARLOGe("No video source or video source is closed.\n");
		return false;
	}

	if (!m_videoSource0->captureFrame()) {
		return false;
	}


	return true;
}

bool ARController::stopRunning()
{
	ARLOGd("ARX::ARController::stopRunning()\n");
	if (state != DETECTION_RUNNING && state != WAITING_FOR_VIDEO) {
		ARLOGe("Stop running called but not running.\n");
		return false;
	}

	m_squareTracker->stop();
	if (m_videoSource0) {
		if (m_videoSource0->isOpen()) {
			m_videoSource0->close();
		}
		delete m_videoSource0;
		m_videoSource0 = NULL;
	}

	state = BASE_INITIALISED;

	ARLOGd("ARX::ARController::stopRunning(): done.\n");
	return true;
}

