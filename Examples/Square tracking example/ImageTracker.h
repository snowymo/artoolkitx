#pragma once

#include <vector>
#include <ARX/ARTrackable.h>
#include <ARX/ARTrackerSquare.h>
#include <ARX/ARController.h>

class ImageTracker
{
public:
	ImageTracker();
	ImageTracker(std::string imageName);
	~ImageTracker();

public:
	std::shared_ptr<ARTrackerSquare> getSquareTracker() { return m_squareTracker; };
	/**
	* Start trackable management so trackables can be added and removed.
	* @return       true if initialisation was OK, false if an error occured.
	*/
	bool initialiseBase();

	/**
	* Report whether artoolkit was initialized and a trackable can be added.
	* Trackables can be added once basic initialisation has occurred.
	* @return  true if adding a trackable is currently possible
	*/
	bool isInited();
	
	/**
	* Start video capture and tracking. (AR/NFT initialisation will begin on a subsequent call to update().)
	* @param vconf			Video configuration string.
	* @param cparaName		Camera parameters filename, or NULL if camera parameters file not being used.
	* @param cparaBuff		A byte-buffer containing contents of a camera parameters file, or NULL if a camera parameters file is being used.
	* @param cparaBuffLen	Length (in bytes) of cparaBuffLen, or 0 if a camera parameters file is being used.
	* @return				true if video capture and tracking was started, otherwise false.
	*/
	bool startRunning(const char* vconf, const char* cparaName, const char* cparaBuff, const long cparaBuffLen);

	/**
	* Performs tracking and updates all trackables. The latest frame from the current
	* video source is retrieved and analysed. Each trackable in the collection is updated with
	* new tracking information. The trackable info array is
	* iterated over, and detected trackables are matched up with those in the trackable collection. Each matched
	* trackable is updated with visibility and transformation information. Any trackables not detected are considered
	* not currently visible.
	*
	* @return				true if update completed successfully, false if an error occurred
	*/
	bool update();

	/**
	* Video capture and tracking stops, but trackables are still valid and can be configured.
	* @return				true if video capture and tracking was stopped, otherwise false.
	*/
	bool stopRunning();

	/**
	* Stop, if running. Remove all trackables, clean up all memory.
	* Starting again from this state requires initialiseBase() to be called again.
	* @return				true if shutdown was successful, otherwise false
	*/
	bool shutdown();

	/**
	* Adds a trackable as specified in the given configuration string. The format of the string can be
	* one of:
	*
	* - Square marker from pattern file: "single;pattern_file;pattern_width", e.g. "single;data/hiro.patt;80"
	* - Square marker from pattern passed in config: "single_buffer;pattern_width;buffer=[]", e.g. "single_buffer;80;buffer=234 221 237..."
	* - Square barcode marker: "single_barcode;barcode_id;pattern_width", e.g. "single_barcode;0;80"
	* - Multi-square marker: "multi;config_file", e.g. "multi;data/multi/marker.dat"
	* - NFT marker: "nft;nft_dataset_pathname", e.g. "nft;gibraltar"
	*
	* @param cfgs		The configuration string
	* @return			The UID of the trackable instantiated based on the configuration string, or -1 if an error occurred.
	*/
	int addTrackable(const std::string& cfgs);
	/**
	* Adds a trackable to the collection.
	* @param trackable	The trackable to add
	* @return			true if the trackable was added successfully, otherwise false
	*/
	bool addTrackable(ARTrackable* trackable);


	/**
	* Clears the collection of trackables.
	* @return				The number of trackables removed
	*/
	int removeAllTrackables();

	/**
	* Returns the number of currently loaded trackables.
	* @return				The number of currently loaded trackables.
	*/
	unsigned int countTrackables() const;

	/**
	* Searches the collection of trackables for the given ID.
	* @param UID             The UID of the trackables to find
	* @return                The found trackable, or NULL if no matching UID was found.
	*/
	ARTrackable* findTrackable(int UID);

	/**
	* Requests the capture of a new frame from the video source(s).
	* In the case of stereo video sources, capture from both sources will be attempted.
	* @return                The capture succeeded, or false if no frame was captured.
	*/
	bool capture();

private:
	std::vector<ARTrackable *> m_trackables;    ///< List of trackables.

	bool doSquareMarkerDetection;
	std::shared_ptr<ARTrackerSquare> m_squareTracker;

	typedef enum {
		NOTHING_INITIALISED,			///< No initialisation yet and no resources allocated.
		BASE_INITIALISED,				///< Trackable management initialised, trackables can be added.
		WAITING_FOR_VIDEO,				///< Waiting for video source to become ready.
		DETECTION_RUNNING				///< Video running, additional initialisation occurred, tracking running.
	} ARToolKitState;

	ARToolKitState state;				///< Current state of operation, progress through initialisation

	// image information
	int width, height, n;
	unsigned char* image;

	// camera parameter
	ARVideoSource *m_videoSource0;      ///< VideoSource providing video frames for tracking
	AR2VideoTimestampT m_updateFrameStamp0;

};

